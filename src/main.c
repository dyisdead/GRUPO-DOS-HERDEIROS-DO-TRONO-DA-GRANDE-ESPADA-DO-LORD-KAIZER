/*
 * PokeTree - programa principal (terminal).
 *
 * Carrega o dataset nas duas arvores (AVL e Rubro-Negra) e oferece um menu
 * para demonstrar as operacoes pedidas no trabalho: insercao, busca, remocao,
 * percursos, validacao das invariantes e metricas.
 *
 * Uso:
 *     ./poketree_cli            -> carrega o dataset inteiro (500 mil)
 *     ./poketree_cli 10000      -> carrega so os primeiros 10.000 registros
 *
 * Como a chave e composta (total_stats, nome, id), o menu localiza o
 * registro pelo id (vetor indexado pelo id) e depois busca nas arvores
 * usando a chave completa.
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "registro.h"
#include "avl.h"
#include "rubro_negra.h"
#include "dataset.h"

static ArvoreAVL avl;
static ArvoreRN rn;

/* Indice id -> registro, so para o menu achar a chave completa a partir do id */
static Registro *por_id = NULL;   /* por_id[id] */
static char *existe = NULL;       /* existe[id] = 1 se o id esta nas arvores */
static long capacidade = 0;       /* tamanho dos vetores acima */
static long proximo_id = 1;       /* id usado na proxima insercao manual */

static double agora_us(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e6 + t.tv_nsec / 1e3;
}

static void garantir_capacidade(long id)
{
    if (id < capacidade) return;
    long nova = capacidade ? capacidade : 1024;
    while (nova <= id) nova *= 2;
    por_id = realloc(por_id, nova * sizeof *por_id);
    existe = realloc(existe, nova);
    if (!por_id || !existe) { fprintf(stderr, "Sem memoria.\n"); exit(1); }
    memset(existe + capacidade, 0, nova - capacidade);
    capacidade = nova;
}

static void ler_linha(const char *pergunta, char *buf, size_t tam)
{
    printf("%s", pergunta);
    fflush(stdout);
    if (!fgets(buf, (int)tam, stdin)) { buf[0] = '\0'; return; }
    buf[strcspn(buf, "\r\n")] = '\0';
}

static long ler_long(const char *pergunta)
{
    char buf[64];
    ler_linha(pergunta, buf, sizeof buf);
    char *fim;
    long v = strtol(buf, &fim, 10);
    return (fim == buf) ? -1 : v;
}

static void imprimir_registro(const Registro *r)
{
    printf("  #%-7ld %-22s %-10s %s IV=%-3d total_stats=%ld\n"
           "           ataques: %s / %s / %s / %s\n",
           r->id, r->nome, r->genero, r->shiny ? "shiny" : "     ",
           r->ivs, r->total_stats,
           r->ataques[0], r->ataques[1], r->ataques[2], r->ataques[3]);
}

static void mostrar_status(void)
{
    printf("\n  Nos: AVL=%ld  RN=%ld | Altura: AVL=%d  RN=%d | "
           "Memoria: AVL=%.1f MB  RN=%.1f MB\n",
           avl.tamanho, rn.tamanho, avl_altura_total(&avl), rn_altura_total(&rn),
           avl_memoria_bytes(&avl) / 1048576.0, rn_memoria_bytes(&rn) / 1048576.0);
}

static void inserir_nas_duas(const Registro *r)
{
    garantir_capacidade(r->id);
    por_id[r->id] = *r;
    existe[r->id] = 1;
    avl_inserir(&avl, *r);
    rn_inserir(&rn, *r);
    if (r->id >= proximo_id) proximo_id = r->id + 1;
}

/* ---------------------------------------------------------------- opcoes */

static void opcao_buscar_id(void)
{
    long id = ler_long("ID: ");
    if (id <= 0) { printf("  ID invalido.\n"); return; }

    /* Se o id nao existe, monta uma chave qualquer so pra mostrar a busca falhando */
    Registro chave;
    memset(&chave, 0, sizeof chave);
    chave.id = id;
    if (id < capacidade && existe[id]) chave = por_id[id];

    avl_zerar_metricas(&avl);
    rn_zerar_metricas(&rn);
    double t0 = agora_us();
    const Registro *ra = avl_buscar(&avl, &chave);
    double t1 = agora_us();
    const Registro *rr = rn_buscar(&rn, &chave);
    double t2 = agora_us();

    if (ra) imprimir_registro(ra);
    else    printf("  Registro #%ld nao encontrado.\n", id);
    printf("  AVL: %s, %ld comparacoes, %.2f us\n", ra ? "achou" : "nao achou", avl.comparacoes, t1 - t0);
    printf("  RN : %s, %ld comparacoes, %.2f us\n", rr ? "achou" : "nao achou", rn.comparacoes, t2 - t1);
}

static void opcao_buscar_nome(void)
{
    char nome[TAM_NOME];
    ler_linha("Nome (ou parte dele): ", nome, sizeof nome);
    if (!nome[0]) return;
    for (char *p = nome; *p; p++) *p = (char)tolower((unsigned char)*p);

    long achados = 0;
    for (long id = 1; id < proximo_id && achados < 20; id++) {
        if (id >= capacidade || !existe[id]) continue;
        char tmp[TAM_NOME];
        strcpy(tmp, por_id[id].nome);
        for (char *p = tmp; *p; p++) *p = (char)tolower((unsigned char)*p);
        if (strstr(tmp, nome)) { imprimir_registro(&por_id[id]); achados++; }
    }
    if (!achados) printf("  Nenhum registro com esse nome.\n");
    else printf("  (mostrando ate 20; use o ID para buscar nas arvores)\n");
}

static void opcao_inserir(void)
{
    Registro r;
    memset(&r, 0, sizeof r);
    r.id = proximo_id;

    ler_linha("Nome: ", r.nome, sizeof r.nome);
    if (!r.nome[0]) { printf("  Cancelado.\n"); return; }
    ler_linha("Genero (macho/femea/sem genero): ", r.genero, sizeof r.genero);
    if (!r.genero[0]) strcpy(r.genero, "sem genero");
    r.shiny = ler_long("Shiny (0/1): ") == 1;
    for (int i = 0; i < NUM_ATAQUES; i++) {
        char p[32];
        snprintf(p, sizeof p, "Ataque %d: ", i + 1);
        ler_linha(p, r.ataques[i], sizeof r.ataques[i]);
    }
    r.ivs = (int)ler_long("IV (1-100): ");
    if (r.ivs < 1 || r.ivs > 100) r.ivs = 50;
    r.total_stats = ler_long("total_stats: ");
    if (r.total_stats < 0) r.total_stats = 0;

    avl_zerar_metricas(&avl);
    rn_zerar_metricas(&rn);
    inserir_nas_duas(&r);

    printf("  Inserido como #%ld.\n", r.id);
    printf("  AVL: %ld comparacoes, %ld rotacoes\n", avl.comparacoes, avl.rotacoes);
    printf("  RN : %ld comparacoes, %ld rotacoes, %ld recoloracoes\n",
           rn.comparacoes, rn.rotacoes, rn.recoloracoes);
}

static void opcao_remover(void)
{
    long id = ler_long("ID para remover: ");
    if (id <= 0 || id >= capacidade || !existe[id]) {
        printf("  Registro #%ld nao esta nas arvores.\n", id);
        return;
    }
    avl_zerar_metricas(&avl);
    rn_zerar_metricas(&rn);
    bool ok_a = avl_remover(&avl, &por_id[id]);
    bool ok_r = rn_remover(&rn, &por_id[id]);
    existe[id] = 0;

    printf("  Removido: AVL=%s RN=%s\n", ok_a ? "sim" : "nao", ok_r ? "sim" : "nao");
    printf("  AVL: %ld comparacoes, %ld rotacoes\n", avl.comparacoes, avl.rotacoes);
    printf("  RN : %ld comparacoes, %ld rotacoes, %ld recoloracoes\n",
           rn.comparacoes, rn.rotacoes, rn.recoloracoes);
}

static void opcao_percursos(void)
{
    printf("  1) pre-ordem  2) em ordem  3) pos-ordem\n");
    long tipo = ler_long("Percurso: ");
    long qtd = ler_long("Quantos registros mostrar (ex.: 10): ");
    if (qtd <= 0) qtd = 10;
    if (avl.tamanho == 0) { printf("  Arvores vazias.\n"); return; }

    Registro *va = malloc(avl.tamanho * sizeof *va);
    Registro *vr = malloc(rn.tamanho * sizeof *vr);
    if (!va || !vr) { free(va); free(vr); printf("  Sem memoria.\n"); return; }

    long na, nr;
    switch (tipo) {
    case 1: na = avl_pre_ordem(&avl, va); nr = rn_pre_ordem(&rn, vr); break;
    case 3: na = avl_pos_ordem(&avl, va); nr = rn_pos_ordem(&rn, vr); break;
    default: na = avl_em_ordem(&avl, va); nr = rn_em_ordem(&rn, vr); tipo = 2; break;
    }

    printf("\n  AVL (primeiros %ld de %ld):\n", qtd < na ? qtd : na, na);
    for (long i = 0; i < qtd && i < na; i++)
        printf("    #%-7ld %-22s total_stats=%ld\n", va[i].id, va[i].nome, va[i].total_stats);
    printf("  RN (primeiros %ld de %ld):\n", qtd < nr ? qtd : nr, nr);
    for (long i = 0; i < qtd && i < nr; i++)
        printf("    #%-7ld %-22s total_stats=%ld\n", vr[i].id, vr[i].nome, vr[i].total_stats);

    if (tipo == 2) {
        /* Em ordem as duas arvores precisam dar exatamente a mesma sequencia */
        int iguais = (na == nr);
        for (long i = 0; iguais && i < na; i++)
            iguais = comparar_registro(&va[i], &vr[i]) == 0;
        printf("  Sequencia em ordem identica nas duas arvores: %s\n", iguais ? "sim" : "NAO");
    } else {
        printf("  (pre/pos-ordem diferem entre as arvores porque o formato delas e diferente)\n");
    }
    free(va);
    free(vr);
}

static void opcao_validar(void)
{
    const char *motivo_a, *motivo_r;
    double t0 = agora_us();
    bool ok_a = avl_validar(&avl, &motivo_a);
    bool ok_r = rn_validar(&rn, &motivo_r);
    double t1 = agora_us();
    printf("  AVL: %s (%s)\n", ok_a ? "VALIDA" : "INVALIDA", motivo_a);
    printf("  RN : %s (%s)\n", ok_r ? "VALIDA" : "INVALIDA", motivo_r);
    printf("  (verificacao completa em %.1f ms)\n", (t1 - t0) / 1000.0);
}

/* ------------------------------------------------------------------ main */

int main(int argc, char **argv)
{
    long maximo = (argc > 1) ? atol(argv[1]) : 0;

    printf("==============================================\n");
    printf("  PokeTree - AVL vs Rubro-Negra (terminal)\n");
    printf("==============================================\n");

    const char *caminho = dataset_localizar();
    if (!caminho) {
        fprintf(stderr, "Nao achei dataset/pokemon_dataset.csv. Rode da raiz do projeto "
                        "ou defina POKETREE_DATASET.\n");
        return 1;
    }

    Registro *dados = NULL;
    long n = dataset_carregar(caminho, maximo, &dados);
    if (n < 0) { fprintf(stderr, "Erro ao ler %s\n", caminho); return 1; }
    printf("Lidos %ld registros de %s\n", n, caminho);

    avl_inicializar(&avl);
    rn_inicializar(&rn);

    double t0 = agora_us();
    for (long i = 0; i < n; i++) inserir_nas_duas(&dados[i]);
    double t1 = agora_us();
    free(dados);

    printf("Carregado nas duas arvores em %.2f s\n", (t1 - t0) / 1e6);
    printf("  AVL: %ld rotacoes\n", avl.rotacoes);
    printf("  RN : %ld rotacoes, %ld recoloracoes\n", rn.rotacoes, rn.recoloracoes);

    for (;;) {
        mostrar_status();
        printf("\n  1) Buscar por ID\n"
               "  2) Procurar por nome\n"
               "  3) Inserir novo Pokemon\n"
               "  4) Remover por ID\n"
               "  5) Percursos (pre / em / pos-ordem)\n"
               "  6) Validar invariantes\n"
               "  0) Sair\n");
        long op = ler_long("> ");
        if (feof(stdin)) op = 0;

        switch (op) {
        case 1: opcao_buscar_id();   break;
        case 2: opcao_buscar_nome(); break;
        case 3: opcao_inserir();     break;
        case 4: opcao_remover();     break;
        case 5: opcao_percursos();   break;
        case 6: opcao_validar();     break;
        case 0:
            avl_destruir(&avl);
            rn_destruir(&rn);
            free(por_id);
            free(existe);
            printf("Tchau!\n");
            return 0;
        default: printf("  Opcao invalida.\n");
        }
    }
}
