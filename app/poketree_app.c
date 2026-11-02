/*
 * PokeTree - aplicativo grafico (AVL vs Rubro-Negra)
 *
 * Um unico executavel: carrega o dataset (500 mil registros) nas duas arvores
 * do projeto e sobe um servidor HTTP local (127.0.0.1) que serve a interface
 * (web/index.html embutido) e uma pequena API JSON. Ao iniciar, abre a janela
 * do app (Edge/Chrome em modo --app, ou o navegador padrao).
 *
 * As imagens dos Pokemon sao carregadas pelo navegador direto do repositorio
 * de sprites da PokeAPI (github.com/PokeAPI/sprites).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #include <shellapi.h>
  typedef SOCKET sock_t;
  #define SEND_FLAGS 0
  #define CLOSESOCK closesocket
#else
  #include <sys/select.h>
  #include <sys/time.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <signal.h>
  typedef int sock_t;
  #define INVALID_SOCKET (-1)
  #define SEND_FLAGS MSG_NOSIGNAL
  #define CLOSESOCK close
#endif

#include "../src/registro.h"
#include "../src/avl.h"
#include "../src/rubro_negra.h"
#include "../src/dataset.h"
#include "index_html.h"

#define HIST 30
#define PORTA_INICIAL 8765

/* ------------------------------------------------------------------ */
/* Estado global                                                       */
/* ------------------------------------------------------------------ */
static Registro *ds = NULL;          /* dataset completo */
static long ds_n = 0;
static unsigned char *presente = NULL; /* presente[i] = registro i esta nas arvores */
static long cursor_livre = 0;

static ArvoreAVL avl;
static ArvoreRN rn;
static long nos = 0;

static long ult_avl = 0, ult_rn = 0;        /* comparacoes da ultima busca */
static long ra[HIST], rr[HIST];             /* ultimas rotacoes (AVL) / recoloracoes (RN) por operacao */
static int na = 0, nr = 0;

static int h_cache_avl = 0, h_cache_rn = 0;
static bool h_sujo = true;

/* mapa nome -> id da PokeAPI (base_especies.csv) */
static long *base_id = NULL;
static char (*base_nome)[TAM_NOME] = NULL;
static int base_n = 0;

static time_t sair_em = 0;

/* ------------------------------------------------------------------ */
/* Buffer de texto                                                     */
/* ------------------------------------------------------------------ */
typedef struct { char *p; size_t n, cap; } Buf;

static void b_grow(Buf *b, size_t extra) {
    if (b->n + extra + 1 > b->cap) {
        size_t nc = b->cap ? b->cap : 4096;
        while (nc < b->n + extra + 1) nc *= 2;
        b->p = (char *)realloc(b->p, nc);
        if (!b->p) { fprintf(stderr, "sem memoria\n"); exit(1); }
        b->cap = nc;
    }
}
static void b_puts(Buf *b, const char *s) {
    size_t l = strlen(s);
    b_grow(b, l);
    memcpy(b->p + b->n, s, l + 1);
    b->n += l;
}
static void b_printf(Buf *b, const char *fmt, ...) {
    char tmp[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    b_puts(b, tmp);
}
static void b_str(Buf *b, const char *s) {
    b_puts(b, "\"");
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { char t[3] = {'\\', (char)c, 0}; b_puts(b, t); }
        else if (c < 0x20) { char t[8]; snprintf(t, sizeof t, "\\u%04x", c); b_puts(b, t); }
        else { char t[2] = {(char)c, 0}; b_puts(b, t); }
    }
    b_puts(b, "\"");
}

/* ------------------------------------------------------------------ */
/* Utilidades                                                          */
/* ------------------------------------------------------------------ */
static bool ci_contem(const char *texto, const char *busca) {
    size_t lb = strlen(busca);
    if (lb == 0) return true;
    for (; *texto; texto++) {
        size_t i = 0;
        while (i < lb && texto[i] && tolower((unsigned char)texto[i]) == tolower((unsigned char)busca[i])) i++;
        if (i == lb) return true;
    }
    return false;
}

static long pokeapi_id(const Registro *r) {
    for (int i = 0; i < base_n; i++)
        if (strcmp(base_nome[i], r->nome) == 0) return base_id[i];
    return 0;
}

static void carregar_base_especies(void) {
    FILE *f = fopen("dataset/base_especies.csv", "r");
    if (!f) return;
    int cap = 2048;
    base_id = (long *)malloc(sizeof(long) * cap);
    base_nome = malloc((size_t)cap * TAM_NOME);
    static char linha[65536];
    if (!fgets(linha, sizeof linha, f)) { fclose(f); return; } /* cabecalho */
    while (fgets(linha, sizeof linha, f)) {
        char *c1 = strchr(linha, ',');
        if (!c1) continue;
        char *c2 = strchr(c1 + 1, ',');
        if (!c2) continue;
        char *c3 = strchr(c2 + 1, ',');
        if (!c3) continue;
        *c1 = 0; *c3 = 0;
        if (base_n >= cap) continue;
        base_id[base_n] = strtol(linha, NULL, 10);
        snprintf(base_nome[base_n], TAM_NOME, "%s", c2 + 1);
        base_n++;
    }
    fclose(f);
}

static void push_hist(long *arr, int *n, long v) {
    if (*n == HIST) { memmove(arr, arr + 1, sizeof(long) * (HIST - 1)); (*n)--; }
    arr[(*n)++] = v;
}

/* ------------------------------------------------------------------ */
/* Operacoes nas duas arvores (medindo rotacoes e comparacoes)         */
/* ------------------------------------------------------------------ */
static void zerar_arvores(void) {
    if (avl.raiz) avl_destruir(&avl);
    if (rn.nil) rn_destruir(&rn);
    avl_inicializar(&avl);
    rn_inicializar(&rn);
    nos = 0; ult_avl = ult_rn = 0;
    na = nr = 0;
    memset(presente, 0, (size_t)ds_n);
    cursor_livre = 0;
    h_sujo = true;
}

static void inserir_registro(long idx, long *d_rot_avl, long *d_rot_rn, long *d_rec_rn) {
    long r0 = avl.rotacoes;
    avl_inserir(&avl, ds[idx]);
    long da = avl.rotacoes - r0;
    push_hist(ra, &na, da);

    r0 = rn.rotacoes;
    long rc0 = rn.recoloracoes;
    rn_inserir(&rn, ds[idx]);
    long dr = rn.rotacoes - r0, dc = rn.recoloracoes - rc0;
    push_hist(rr, &nr, dc);

    presente[idx] = 1;
    nos++;
    h_sujo = true;
    if (d_rot_avl) *d_rot_avl = da;
    if (d_rot_rn) *d_rot_rn = dr;
    if (d_rec_rn) *d_rec_rn = dc;
}

static void carregar_n(long n, double *t_avl, double *t_rn) {
    if (n <= 0 || n > ds_n) n = ds_n;
    zerar_arvores();
    clock_t t0 = clock();
    for (long i = 0; i < n; i++) {
        long r0 = avl.rotacoes;
        avl_inserir(&avl, ds[i]);
        push_hist(ra, &na, avl.rotacoes - r0);
    }
    *t_avl = (double)(clock() - t0) / CLOCKS_PER_SEC;
    t0 = clock();
    for (long i = 0; i < n; i++) {
        long rc0 = rn.recoloracoes;
        rn_inserir(&rn, ds[i]);
        push_hist(rr, &nr, rn.recoloracoes - rc0);
        presente[i] = 1;
    }
    *t_rn = (double)(clock() - t0) / CLOCKS_PER_SEC;
    nos = n;
    cursor_livre = n;
    h_sujo = true;
}

/* ------------------------------------------------------------------ */
/* JSON: estado, registro, vistas das arvores                          */
/* ------------------------------------------------------------------ */
static void atualizar_alturas(void) {
    if (!h_sujo) return;
    h_cache_avl = avl_altura_total(&avl);
    h_cache_rn = rn_altura_total(&rn);
    h_sujo = false;
}

static void json_hist(Buf *b, const long *a, int n) {
    b_puts(b, "[");
    for (int i = 0; i < n; i++) b_printf(b, "%s%ld", i ? "," : "", a[i]);
    b_puts(b, "]");
}

static void json_estado(Buf *b) {
    atualizar_alturas();
    b_printf(b, "{\"nos\":%ld,\"total\":%ld,", nos, ds_n);
    b_printf(b, "\"avl\":{\"altura\":%d,\"rot\":%ld,\"comp\":%ld,\"ult\":%ld,\"mem\":%.0f,\"hist\":",
             h_cache_avl, avl.rotacoes, avl.comparacoes, ult_avl, (double)avl_memoria_bytes(&avl));
    json_hist(b, ra, na);
    b_printf(b, "},\"rn\":{\"altura\":%d,\"rot\":%ld,\"rec\":%ld,\"comp\":%ld,\"ult\":%ld,\"mem\":%.0f,\"hist\":",
             h_cache_rn, rn.rotacoes, rn.recoloracoes, rn.comparacoes, ult_rn, (double)rn_memoria_bytes(&rn));
    json_hist(b, rr, nr);
    b_puts(b, "}}");
}

static void json_registro(Buf *b, const Registro *r) {
    b_printf(b, "{\"id\":%ld,\"nome\":", r->id);
    b_str(b, r->nome);
    b_puts(b, ",\"genero\":"); b_str(b, r->genero);
    b_printf(b, ",\"shiny\":%d,\"ivs\":%d,\"total\":%ld,\"pid\":%ld,\"ataques\":[",
             r->shiny ? 1 : 0, r->ivs, r->total_stats, pokeapi_id(r));
    for (int i = 0; i < NUM_ATAQUES; i++) {
        if (i) b_puts(b, ",");
        b_str(b, r->ataques[i]);
    }
    b_puts(b, "]}");
}

static long cont_avl(const NoAVL *n) {
    return n ? 1 + cont_avl(n->esquerda) + cont_avl(n->direita) : 0;
}
static long cont_rn(const NoRN *n) {
    return n != rn.nil ? 1 + cont_rn(n->esquerda) + cont_rn(n->direita) : 0;
}

/* Vista: expande os 3 primeiros niveis + o caminho ate o alvo; o resto vira {"z":tamanho}. */
static void vista_avl(Buf *b, const NoAVL *n, int d, bool caminho, const Registro *alvo, bool tudo) {
    int c = 0; bool achou = false;
    if (caminho) { c = comparar_registro(alvo, &n->registro); achou = (c == 0); }
    b_printf(b, "{\"k\":%ld,\"i\":%ld,\"n\":", n->registro.total_stats, n->registro.id);
    b_str(b, n->registro.nome);
    if (caminho) b_puts(b, ",\"p\":1");
    if (achou) b_puts(b, ",\"f\":1");
    for (int lado = 0; lado < 2; lado++) {
        const NoAVL *ch = lado == 0 ? n->esquerda : n->direita;
        if (!ch) continue;
        bool cam = caminho && !achou && ((c < 0) == (lado == 0));
        b_puts(b, lado == 0 ? ",\"l\":" : ",\"r\":");
        if (cam || d < 2 || tudo) vista_avl(b, ch, d + 1, cam, alvo, tudo);
        else b_printf(b, "{\"z\":%ld}", cont_avl(ch));
    }
    b_puts(b, "}");
}

static void vista_rn(Buf *b, const NoRN *n, int d, bool caminho, const Registro *alvo, bool tudo) {
    int c = 0; bool achou = false;
    if (caminho) { c = comparar_registro(alvo, &n->registro); achou = (c == 0); }
    b_printf(b, "{\"k\":%ld,\"i\":%ld,\"o\":%d,\"n\":", n->registro.total_stats, n->registro.id,
             n->cor == VERMELHO ? 1 : 0);
    b_str(b, n->registro.nome);
    if (caminho) b_puts(b, ",\"p\":1");
    if (achou) b_puts(b, ",\"f\":1");
    for (int lado = 0; lado < 2; lado++) {
        const NoRN *ch = lado == 0 ? n->esquerda : n->direita;
        if (ch == rn.nil) continue;
        bool cam = caminho && !achou && ((c < 0) == (lado == 0));
        b_puts(b, lado == 0 ? ",\"l\":" : ",\"r\":");
        if (cam || d < 2 || tudo) vista_rn(b, ch, d + 1, cam, alvo, tudo);
        else b_printf(b, "{\"z\":%ld}", cont_rn(ch));
    }
    b_puts(b, "}");
}

/* Busca (com contagem de comparacoes) + vista das duas arvores. id = 0 -> sem alvo. */
static void resp_ver(Buf *b, long id) {
    const Registro *alvo = (id >= 1 && id <= ds_n) ? &ds[id - 1] : NULL;
    bool tem = alvo && presente[id - 1];
    long ca = 0, cr = 0;
    bool fa = false, fr = false;

    if (alvo) {
        long c0 = avl.comparacoes;
        fa = avl_buscar(&avl, alvo) != NULL;
        ca = avl.comparacoes - c0; ult_avl = ca;
        c0 = rn.comparacoes;
        fr = rn_buscar(&rn, alvo) != NULL;
        cr = rn.comparacoes - c0; ult_rn = cr;
    }
    atualizar_alturas();
    b_puts(b, "{\"ok\":1,");
    if (alvo) {
        b_puts(b, "\"reg\":");
        json_registro(b, alvo);
        b_printf(b, ",\"presente\":%d,", tem ? 1 : 0);
    }
    bool tudo = nos <= 127;
    b_printf(b, "\"avl\":{\"achou\":%d,\"comp\":%ld,\"altura\":%d,\"arv\":", fa, ca, h_cache_avl);
    if (avl.raiz) vista_avl(b, avl.raiz, 0, alvo != NULL, alvo, tudo); else b_puts(b, "null");
    b_printf(b, "},\"rn\":{\"achou\":%d,\"comp\":%ld,\"altura\":%d,\"arv\":", fr, cr, h_cache_rn);
    if (rn.raiz != rn.nil) vista_rn(b, rn.raiz, 0, alvo != NULL, alvo, tudo); else b_puts(b, "null");
    b_puts(b, "},\"estado\":");
    json_estado(b);
    b_puts(b, "}");
}

static void resp_erro(Buf *b, const char *msg) {
    b_puts(b, "{\"ok\":0,\"erro\":");
    b_str(b, msg);
    b_puts(b, "}");
}

/* ------------------------------------------------------------------ */
/* Rotas da API                                                        */
/* ------------------------------------------------------------------ */
static bool qget(const char *qs, const char *key, char *out, size_t sz) {
    size_t kl = strlen(key);
    const char *p = qs;
    while (*p) {
        if (strncmp(p, key, kl) == 0 && p[kl] == '=') {
            p += kl + 1;
            size_t i = 0;
            while (*p && *p != '&' && i + 1 < sz) {
                if (*p == '%' && isxdigit((unsigned char)p[1]) && isxdigit((unsigned char)p[2])) {
                    char h[3] = {p[1], p[2], 0};
                    out[i++] = (char)strtol(h, NULL, 16);
                    p += 3;
                } else if (*p == '+') { out[i++] = ' '; p++; }
                else out[i++] = *p++;
            }
            out[i] = 0;
            return true;
        }
        while (*p && *p != '&') p++;
        if (*p == '&') p++;
    }
    return false;
}

static bool so_digitos(const char *s) {
    if (!*s) return false;
    for (; *s; s++) if (!isdigit((unsigned char)*s)) return false;
    return true;
}

static void api_buscar(Buf *b, const char *qs) {
    char q[256] = "";
    qget(qs, "q", q, sizeof q);
    /* trim */
    char *ini = q;
    while (*ini == ' ') ini++;
    size_t l = strlen(ini);
    while (l > 0 && ini[l - 1] == ' ') ini[--l] = 0;
    if (!*ini) { resp_erro(b, "Digite um ID ou nome para buscar."); return; }

    if (so_digitos(ini + (*ini == '#'))) {
        long id = strtol(ini + (*ini == '#'), NULL, 10);
        if (id < 1 || id > ds_n) { resp_erro(b, "ID fora do intervalo do dataset."); return; }
        b_printf(b, "{\"ok\":1,\"modo\":\"id\",\"total\":1,\"lista\":[{\"id\":%ld,\"nome\":", id);
        b_str(b, ds[id - 1].nome);
        b_printf(b, ",\"total\":%ld,\"presente\":%d}]}", ds[id - 1].total_stats, presente[id - 1] ? 1 : 0);
        return;
    }
    long total = 0;
    b_puts(b, "{\"ok\":1,\"modo\":\"nome\",\"lista\":[");
    for (long i = 0; i < ds_n; i++) {
        if (!presente[i] || !ci_contem(ds[i].nome, ini)) continue;
        if (total < 300) {
            b_printf(b, "%s{\"id\":%ld,\"nome\":", total ? "," : "", ds[i].id);
            b_str(b, ds[i].nome);
            b_printf(b, ",\"total\":%ld,\"presente\":1}", ds[i].total_stats);
        }
        total++;
    }
    b_printf(b, "],\"total\":%ld}", total);
}

static void api_carregar(Buf *b, const char *qs) {
    char v[32] = "";
    qget(qs, "n", v, sizeof v);
    long n = strtol(v, NULL, 10);
    double ta, tr;
    carregar_n(n, &ta, &tr);
    b_printf(b, "{\"ok\":1,\"n\":%ld,\"t_avl\":%.3f,\"t_rn\":%.3f,\"estado\":", nos, ta, tr);
    json_estado(b);
    b_puts(b, "}");
}

static void api_inserir(Buf *b) {
    while (cursor_livre < ds_n && presente[cursor_livre]) cursor_livre++;
    if (cursor_livre >= ds_n) {
        resp_erro(b, "Todos os registros do dataset ja estao nas arvores.");
        return;
    }
    long idx = cursor_livre;
    long da, dr, dc;
    inserir_registro(idx, &da, &dr, &dc);
    b_printf(b, "{\"ok\":1,\"id\":%ld,\"rot_avl\":%ld,\"rot_rn\":%ld,\"rec_rn\":%ld,\"nome\":", ds[idx].id, da, dr, dc);
    b_str(b, ds[idx].nome);
    b_puts(b, "}");
}

static void api_remover(Buf *b, const char *qs) {
    char v[32] = "";
    qget(qs, "id", v, sizeof v);
    const char *s = v + (v[0] == '#');
    if (!so_digitos(s)) { resp_erro(b, "Informe um ID numerico para remover."); return; }
    long id = strtol(s, NULL, 10);
    if (id < 1 || id > ds_n) { resp_erro(b, "ID fora do intervalo do dataset."); return; }
    if (!presente[id - 1]) { resp_erro(b, "Esse ID nao esta nas arvores (ja foi removido)."); return; }

    const Registro *r = &ds[id - 1];
    long r0 = avl.rotacoes;
    bool oa = avl_remover(&avl, r);
    long da = avl.rotacoes - r0;
    push_hist(ra, &na, da);

    r0 = rn.rotacoes;
    long rc0 = rn.recoloracoes;
    bool orn = rn_remover(&rn, r);
    long dr = rn.rotacoes - r0, dc = rn.recoloracoes - rc0;
    push_hist(rr, &nr, dc);

    if (!oa || !orn) { resp_erro(b, "Falha ao remover o registro."); return; }
    presente[id - 1] = 0;
    if (id - 1 < cursor_livre) cursor_livre = id - 1;
    nos--;
    h_sujo = true;
    b_printf(b, "{\"ok\":1,\"id\":%ld,\"rot_avl\":%ld,\"rot_rn\":%ld,\"rec_rn\":%ld,\"nome\":", id, da, dr, dc);
    b_str(b, r->nome);
    b_puts(b, "}");
}

/* ------------------------------------------------------------------ */
/* HTTP                                                                */
/* ------------------------------------------------------------------ */
static void enviar_tudo(sock_t s, const char *dados, size_t n) {
    size_t env = 0;
    while (env < n) {
        int r = (int)send(s, dados + env, (int)(n - env > 65536 ? 65536 : n - env), SEND_FLAGS);
        if (r <= 0) return;
        env += (size_t)r;
    }
}

static void responder(sock_t s, int codigo, const char *tipo, const char *corpo, size_t n) {
    char cab[512];
    const char *txt = codigo == 200 ? "OK" : codigo == 204 ? "No Content" : codigo == 404 ? "Not Found" : "Bad Request";
    int hl = snprintf(cab, sizeof cab,
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Cache-Control: no-store\r\nConnection: close\r\n\r\n", codigo, txt, tipo, n);
    enviar_tudo(s, cab, (size_t)hl);
    if (n) enviar_tudo(s, corpo, n);
}

static void tratar(sock_t s, char *req) {
    char metodo[8], alvo[2048];
    if (sscanf(req, "%7s %2047s", metodo, alvo) != 2 || strcmp(metodo, "GET") != 0) {
        responder(s, 400, "text/plain", "bad request", 11);
        return;
    }
    char *qs = strchr(alvo, '?');
    if (qs) *qs++ = 0; else qs = alvo + strlen(alvo);

    if (strcmp(alvo, "/api/sair") == 0) {
        sair_em = time(NULL) + 4;      /* cancelado se a pagina recarregar */
        responder(s, 204, "text/plain", "", 0);
        return;
    }
    sair_em = 0;

    if (strcmp(alvo, "/") == 0 || strcmp(alvo, "/index.html") == 0) {
        responder(s, 200, "text/html; charset=utf-8", (const char *)index_html, index_html_len);
        return;
    }
    if (strncmp(alvo, "/api/", 5) != 0) {
        responder(s, strcmp(alvo, "/favicon.ico") == 0 ? 204 : 404, "text/plain", "", 0);
        return;
    }

    Buf b = {0};
    const char *rota = alvo + 5;
    if (strcmp(rota, "estado") == 0) {
        b_puts(&b, "{\"ok\":1,\"estado\":"); json_estado(&b); b_puts(&b, "}");
    } else if (strcmp(rota, "ver") == 0) {
        char v[32] = "0";
        qget(qs, "id", v, sizeof v);
        resp_ver(&b, strtol(v, NULL, 10));
    } else if (strcmp(rota, "buscar") == 0) {
        api_buscar(&b, qs);
    } else if (strcmp(rota, "carregar") == 0) {
        api_carregar(&b, qs);
    } else if (strcmp(rota, "inserir") == 0) {
        api_inserir(&b);
    } else if (strcmp(rota, "remover") == 0) {
        api_remover(&b, qs);
    } else {
        resp_erro(&b, "rota desconhecida");
    }
    responder(s, 200, "application/json; charset=utf-8", b.p, b.n);
    free(b.p);
}

/* ------------------------------------------------------------------ */
/* Sistema: pasta do executavel, navegador, sockets                    */
/* ------------------------------------------------------------------ */
static void ir_para_pasta_do_exe(void) {
#ifdef _WIN32
    char caminho[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, caminho, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        char *p = strrchr(caminho, '\\');
        if (p) { *p = 0; SetCurrentDirectoryA(caminho); }
    }
#else
    char caminho[4096];
    ssize_t n = readlink("/proc/self/exe", caminho, sizeof caminho - 1);
    if (n > 0) {
        caminho[n] = 0;
        char *p = strrchr(caminho, '/');
        if (p) { *p = 0; if (chdir(caminho) != 0) { /* segue na pasta atual */ } }
    }
#endif
}

static void abrir_janela(const char *url) {
    if (getenv("POKETREE_NAO_ABRIR")) return;
#ifdef _WIN32
    char arg[256];
    snprintf(arg, sizeof arg, "--app=%s --window-size=1672,941", url);
    const char *navs[] = {"msedge.exe", "chrome.exe", NULL};
    for (int i = 0; navs[i]; i++) {
        HINSTANCE r = ShellExecuteA(NULL, "open", navs[i], arg, NULL, SW_SHOWNORMAL);
        if ((INT_PTR)r > 32) return;
    }
    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#else
    char cmd[1024];
    snprintf(cmd, sizeof cmd,
        "for b in google-chrome google-chrome-stable chromium chromium-browser microsoft-edge brave-browser; do "
        "if command -v $b >/dev/null 2>&1; then $b --app=%s --window-size=1672,941 >/dev/null 2>&1 & exit 0; fi; done; "
        "(xdg-open %s >/dev/null 2>&1 || true) &", url, url);
    if (system(cmd) != 0) { /* sem navegador: o usuario abre o endereco manualmente */ }
#endif
}

#define MAX_CLI 32
typedef struct { sock_t s; char buf[8192]; int n; time_t desde; } Cli;

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    ir_para_pasta_do_exe();
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\n==============================================\n");
    printf("  PokeTree - AVL vs Rubro-Negra (UNICID)\n");
    printf("==============================================\n");

    const char *caminho = dataset_localizar();
    if (!caminho) {
        fprintf(stderr, "Erro: dataset/pokemon_dataset.csv nao encontrado ao lado do executavel.\n");
        printf("Pressione Enter para sair...\n");
        getchar();
        return 1;
    }
    printf("Lendo %s ...\n", caminho);
    ds_n = dataset_carregar(caminho, 0, &ds);
    if (ds_n <= 0) { fprintf(stderr, "Erro ao ler o dataset.\n"); getchar(); return 1; }
    presente = (unsigned char *)calloc((size_t)ds_n, 1);
    carregar_base_especies();
    avl_inicializar(&avl);
    rn_inicializar(&rn);

    printf("Montando as arvores com %ld registros ...\n", ds_n);
    double ta, tr;
    carregar_n(ds_n, &ta, &tr);
    atualizar_alturas();
    {
        const char *m1 = NULL, *m2 = NULL;
        bool v1 = avl_validar(&avl, &m1), v2 = rn_validar(&rn, &m2);
        printf("  Invariantes: AVL %s | Rubro-Negra %s\n", v1 ? "OK" : "FALHOU", v2 ? "OK" : "FALHOU");
    }
    printf("  AVL:         %.2f s | altura %d | %ld rotacoes\n", ta, h_cache_avl, avl.rotacoes);
    printf("  Rubro-Negra: %.2f s | altura %d | %ld rotacoes, %ld recoloracoes\n", tr, h_cache_rn, rn.rotacoes, rn.recoloracoes);

    sock_t ls = socket(AF_INET, SOCK_STREAM, 0);
    int um = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char *)&um, sizeof um);
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof ad);
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int porta = PORTA_INICIAL;
    for (; porta < PORTA_INICIAL + 50; porta++) {
        ad.sin_port = htons((unsigned short)porta);
        if (bind(ls, (struct sockaddr *)&ad, sizeof ad) == 0) break;
    }
    if (porta >= PORTA_INICIAL + 50 || listen(ls, 16) != 0) {
        fprintf(stderr, "Erro: nao foi possivel abrir uma porta local.\n");
        return 1;
    }
    char url[64];
    snprintf(url, sizeof url, "http://127.0.0.1:%d/", porta);
    printf("\nApp rodando em %s\n", url);
    printf("Feche a janela do app (ou Ctrl+C) para encerrar.\n");
    abrir_janela(url);

    Cli cli[MAX_CLI];
    for (int i = 0; i < MAX_CLI; i++) cli[i].s = INVALID_SOCKET;

    for (;;) {
        fd_set rf;
        FD_ZERO(&rf);
        FD_SET(ls, &rf);
        sock_t maxfd = ls;
        for (int i = 0; i < MAX_CLI; i++)
            if (cli[i].s != INVALID_SOCKET) { FD_SET(cli[i].s, &rf); if (cli[i].s > maxfd) maxfd = cli[i].s; }
        struct timeval tv = {0, 300000};
        int r = select((int)maxfd + 1, &rf, NULL, NULL, &tv);
        time_t agora = time(NULL);

        if (r > 0 && FD_ISSET(ls, &rf)) {
            sock_t c = accept(ls, NULL, NULL);
            if (c != INVALID_SOCKET) {
                int livre = -1;
                for (int i = 0; i < MAX_CLI; i++) if (cli[i].s == INVALID_SOCKET) { livre = i; break; }
                if (livre < 0) CLOSESOCK(c);
                else { cli[livre].s = c; cli[livre].n = 0; cli[livre].desde = agora; }
            }
        }
        for (int i = 0; i < MAX_CLI; i++) {
            if (cli[i].s == INVALID_SOCKET) continue;
            bool fechar = false;
            if (r > 0 && FD_ISSET(cli[i].s, &rf)) {
                int n = (int)recv(cli[i].s, cli[i].buf + cli[i].n, (int)sizeof(cli[i].buf) - 1 - cli[i].n, 0);
                if (n <= 0) fechar = true;
                else {
                    cli[i].n += n;
                    cli[i].buf[cli[i].n] = 0;
                    if (strstr(cli[i].buf, "\r\n\r\n")) { tratar(cli[i].s, cli[i].buf); fechar = true; }
                    else if (cli[i].n >= (int)sizeof(cli[i].buf) - 1) fechar = true;
                }
            }
            if (!fechar && agora - cli[i].desde > 10) fechar = true;   /* conexoes ociosas (preconnect) */
            if (fechar) { CLOSESOCK(cli[i].s); cli[i].s = INVALID_SOCKET; }
        }
        if (sair_em && agora >= sair_em) {
            printf("Janela fechada. Encerrando.\n");
            break;
        }
    }
    return 0;
}
