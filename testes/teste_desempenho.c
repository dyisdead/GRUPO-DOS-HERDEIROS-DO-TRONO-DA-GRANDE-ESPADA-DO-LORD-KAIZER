/*
 * Testes de desempenho: AVL x Rubro-Negra com os dados reais do dataset.
 *
 * Para cada tamanho n e para cada ordem de insercao (aleatoria, crescente,
 * decrescente):
 *   - usa os n primeiros registros de dataset/pokemon_dataset.csv
 *     (na ordem "crescente"/"decrescente" eles sao ordenados pela chave antes:
 *     pior caso para uma BST simples, que viraria uma lista)
 *   - sorteia UMA vez as chaves de busca e de remocao, e usa as MESMAS nas duas
 *     arvores e em todas as repeticoes (comparacao justa e reproduzivel)
 *   - descarta 1 execucao de aquecimento e roda 5 repeticoes
 *   - fases medidas separadamente:
 *       insercao  -> n registros
 *       busca     -> 10.000 buscas (metade existentes, metade inexistentes, misturadas)
 *       remocao   -> 10% dos registros, em ordem aleatoria
 *   - valida as invariantes de cada arvore depois da insercao e da remocao
 *
 * Saida (formato "longo"):
 *     resultados/resultados_desempenho.csv  -> bateria completa (usada nos graficos)
 *     resultados/resultados_rapido.csv      -> quando os tamanhos sao passados na linha
 *                                              de comando (nao apaga a bateria completa)
 * Colunas:
 *     arvore,ordem,n,metrica,media,desvio_padrao
 *
 * Metricas:
 *   t_insercao, t_busca_media, t_remocao            tempo em segundos
 *   t_insercao_por_op, t_remocao_por_op             tempo medio por operacao
 *   comparacoes_insercao_media / _busca_media / _remocao_media
 *                                                   comparacoes de chave por operacao
 *   rotacoes_insercao, rotacoes_remocao             total na fase
 *   recoloracoes_insercao, recoloracoes_remocao     total na fase (so RN; AVL = 0)
 *   altura                                          depois da insercao
 *   memoria_bytes, bytes_por_no                     memoria dos nos depois da insercao
 *   invariantes_ok                                  1 se validou em todas as repeticoes
 *
 * Uso (na raiz do projeto):
 *     ./teste_desempenho                  # tamanhos padrao (1k a 500k)
 *     ./teste_desempenho 1000 10000       # so esses tamanhos (teste rapido)
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "../src/registro.h"
#include "../src/avl.h"
#include "../src/rubro_negra.h"
#include "../src/dataset.h"

#define REPETICOES   5
#define NUM_BUSCAS   10000

static const long TAMANHOS_PADRAO[] = {1000, 5000, 10000, 50000, 100000, 300000, 500000};
static const int NUM_TAMANHOS_PADRAO = 7;

static const char *ORDENS[3] = {"aleatoria", "crescente", "decrescente"};

/* ---------------- utilitarios ---------------- */

static double agora_segundos(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
}

static int comparar_crescente(const void *a, const void *b) {
    return comparar_registro((const Registro *) a, (const Registro *) b);
}

static int comparar_decrescente(const void *a, const void *b) {
    return comparar_registro((const Registro *) b, (const Registro *) a);
}

static void embaralhar(long *v, long n) {
    for (long i = n - 1; i > 0; i--) {
        long j = rand() % (i + 1);
        long t = v[i]; v[i] = v[j]; v[j] = t;
    }
}

/* ---------------- cenario: dados + chaves sorteadas uma unica vez ---------------- */

typedef struct {
    Registro *registros;     /* ordem de insercao */
    long n;
    Registro *buscas;        /* NUM_BUSCAS chaves (metade existe, metade nao) */
    Registro *remocoes;      /* 10% dos registros, em ordem aleatoria */
    long qtd_remocoes;
} Cenario;

static Cenario montar_cenario(const Registro *base, long n, int ordem) {
    Cenario c;
    srand((unsigned int) (n * 3 + ordem));     /* reproduzivel */

    c.n = n;
    c.registros = (Registro *) malloc(sizeof(Registro) * (size_t) n);
    memcpy(c.registros, base, sizeof(Registro) * (size_t) n);
    if (ordem == 1) qsort(c.registros, (size_t) n, sizeof(Registro), comparar_crescente);
    if (ordem == 2) qsort(c.registros, (size_t) n, sizeof(Registro), comparar_decrescente);

    long *idx = (long *) malloc(sizeof(long) * (size_t) n);
    for (long i = 0; i < n; i++) idx[i] = i;

    /* buscas: metade registros existentes, metade chaves que nao existem */
    embaralhar(idx, n);
    c.buscas = (Registro *) malloc(sizeof(Registro) * NUM_BUSCAS);
    long existentes = (n < NUM_BUSCAS / 2) ? n : NUM_BUSCAS / 2;
    for (long k = 0; k < NUM_BUSCAS; k++) {
        if (k < existentes) {
            c.buscas[k] = c.registros[idx[k]];
        } else {
            /* mesmo total_stats de um registro real, mas id que nao existe:
             * a busca desce ate o fundo da arvore em vez de parar logo */
            c.buscas[k] = c.registros[idx[k % n]];
            c.buscas[k].id = -(k + 1);
        }
    }
    for (long i = NUM_BUSCAS - 1; i > 0; i--) {        /* mistura existentes e nao */
        long j = rand() % (i + 1);
        Registro t = c.buscas[i]; c.buscas[i] = c.buscas[j]; c.buscas[j] = t;
    }

    /* remocoes: 10% sorteados */
    embaralhar(idx, n);
    c.qtd_remocoes = n / 10 > 0 ? n / 10 : 1;
    c.remocoes = (Registro *) malloc(sizeof(Registro) * (size_t) c.qtd_remocoes);
    for (long k = 0; k < c.qtd_remocoes; k++) c.remocoes[k] = c.registros[idx[k]];

    free(idx);
    return c;
}

static void liberar_cenario(Cenario *c) {
    free(c->registros);
    free(c->buscas);
    free(c->remocoes);
}

/* ---------------- resultado de uma execucao ---------------- */

enum {
    M_T_INSERCAO, M_T_INSERCAO_OP, M_T_BUSCA, M_T_REMOCAO, M_T_REMOCAO_OP,
    M_CMP_INSERCAO, M_CMP_BUSCA, M_CMP_REMOCAO,
    M_ROT_INSERCAO, M_ROT_REMOCAO, M_REC_INSERCAO, M_REC_REMOCAO,
    M_ALTURA, M_MEMORIA, M_BYTES_NO,
    NUM_METRICAS
};

static const char *NOME_METRICA[NUM_METRICAS] = {
    "t_insercao", "t_insercao_por_op", "t_busca_media", "t_remocao", "t_remocao_por_op",
    "comparacoes_insercao_media", "comparacoes_busca_media", "comparacoes_remocao_media",
    "rotacoes_insercao", "rotacoes_remocao", "recoloracoes_insercao", "recoloracoes_remocao",
    "altura", "memoria_bytes", "bytes_por_no",
};

typedef struct {
    double v[NUM_METRICAS];
    int invariantes_ok;
} Execucao;

static Execucao rodar_avl(const Cenario *c) {
    Execucao e;
    memset(&e, 0, sizeof(e));
    ArvoreAVL a;
    avl_inicializar(&a);

    /* insercao */
    double t0 = agora_segundos();
    for (long i = 0; i < c->n; i++) avl_inserir(&a, c->registros[i]);
    e.v[M_T_INSERCAO] = agora_segundos() - t0;
    e.v[M_CMP_INSERCAO] = (double) a.comparacoes / (double) c->n;
    e.v[M_ROT_INSERCAO] = (double) a.rotacoes;
    e.v[M_ALTURA] = avl_altura_total(&a);
    e.v[M_MEMORIA] = (double) avl_memoria_bytes(&a);
    e.invariantes_ok = avl_validar(&a, NULL);

    /* busca */
    avl_zerar_metricas(&a);
    t0 = agora_segundos();
    for (int k = 0; k < NUM_BUSCAS; k++) avl_buscar(&a, &c->buscas[k]);
    e.v[M_T_BUSCA] = (agora_segundos() - t0) / NUM_BUSCAS;
    e.v[M_CMP_BUSCA] = (double) a.comparacoes / NUM_BUSCAS;

    /* remocao */
    avl_zerar_metricas(&a);
    t0 = agora_segundos();
    for (long k = 0; k < c->qtd_remocoes; k++) avl_remover(&a, &c->remocoes[k]);
    e.v[M_T_REMOCAO] = agora_segundos() - t0;
    e.v[M_CMP_REMOCAO] = (double) a.comparacoes / (double) c->qtd_remocoes;
    e.v[M_ROT_REMOCAO] = (double) a.rotacoes;
    e.invariantes_ok = e.invariantes_ok && avl_validar(&a, NULL)
                       && a.tamanho == c->n - c->qtd_remocoes;

    avl_destruir(&a);
    return e;
}

static Execucao rodar_rn(const Cenario *c) {
    Execucao e;
    memset(&e, 0, sizeof(e));
    ArvoreRN a;
    rn_inicializar(&a);

    double t0 = agora_segundos();
    for (long i = 0; i < c->n; i++) rn_inserir(&a, c->registros[i]);
    e.v[M_T_INSERCAO] = agora_segundos() - t0;
    e.v[M_CMP_INSERCAO] = (double) a.comparacoes / (double) c->n;
    e.v[M_ROT_INSERCAO] = (double) a.rotacoes;
    e.v[M_REC_INSERCAO] = (double) a.recoloracoes;
    e.v[M_ALTURA] = rn_altura_total(&a);
    e.v[M_MEMORIA] = (double) rn_memoria_bytes(&a);
    e.invariantes_ok = rn_validar(&a, NULL);

    rn_zerar_metricas(&a);
    t0 = agora_segundos();
    for (int k = 0; k < NUM_BUSCAS; k++) rn_buscar(&a, &c->buscas[k]);
    e.v[M_T_BUSCA] = (agora_segundos() - t0) / NUM_BUSCAS;
    e.v[M_CMP_BUSCA] = (double) a.comparacoes / NUM_BUSCAS;

    rn_zerar_metricas(&a);
    t0 = agora_segundos();
    for (long k = 0; k < c->qtd_remocoes; k++) rn_remover(&a, &c->remocoes[k]);
    e.v[M_T_REMOCAO] = agora_segundos() - t0;
    e.v[M_CMP_REMOCAO] = (double) a.comparacoes / (double) c->qtd_remocoes;
    e.v[M_ROT_REMOCAO] = (double) a.rotacoes;
    e.v[M_REC_REMOCAO] = (double) a.recoloracoes;
    e.invariantes_ok = e.invariantes_ok && rn_validar(&a, NULL)
                       && a.tamanho == c->n - c->qtd_remocoes;

    rn_destruir(&a);
    return e;
}

/* ---------------- media e desvio padrao (amostral) ---------------- */

static void media_e_desvio(const double *v, int n, double *media, double *desvio) {
    double soma = 0;
    for (int i = 0; i < n; i++) soma += v[i];
    *media = soma / n;
    double q = 0;
    for (int i = 0; i < n; i++) q += (v[i] - *media) * (v[i] - *media);
    *desvio = (n > 1) ? sqrt(q / (n - 1)) : 0.0;
}

/* ---------------- bateria completa ---------------- */

static FILE *abrir_csv(const char *nome_arquivo, char *caminho, size_t tam) {
    /* da raiz do projeto (make) ou de dentro de /testes */
    static const char *pastas[] = {"resultados/", "../resultados/"};
    for (int i = 0; i < 2; i++) {
        snprintf(caminho, tam, "%s%s", pastas[i], nome_arquivo);
        FILE *f = fopen(caminho, "w");
        if (f) return f;
    }
    return NULL;
}

static int rodar_bateria(const long *tamanhos, int num_tamanhos, const char *nome_csv) {
    const char *caminho_dataset = dataset_localizar();
    if (caminho_dataset == NULL) {
        fprintf(stderr, "Erro: dataset nao encontrado (dataset/pokemon_dataset.csv).\n"
                        "Gere com:  python dataset/gerar_dataset.py\n");
        return 1;
    }
    long maior = 0;
    for (int i = 0; i < num_tamanhos; i++) if (tamanhos[i] > maior) maior = tamanhos[i];

    Registro *dataset = NULL;
    long total = dataset_carregar(caminho_dataset, maior, &dataset);
    if (total <= 0) {
        fprintf(stderr, "Erro: nao foi possivel ler registros de %s\n", caminho_dataset);
        free(dataset);
        return 1;
    }
    printf("Dataset: %s (%ld registros carregados)\n", caminho_dataset, total);
    printf("sizeof(NoAVL) = %zu bytes | sizeof(NoRN) = %zu bytes | sizeof(Registro) = %zu bytes\n\n",
           sizeof(NoAVL), sizeof(NoRN), sizeof(Registro));

    char caminho_csv[256];
    FILE *f = abrir_csv(nome_csv, caminho_csv, sizeof(caminho_csv));
    if (f == NULL) {
        fprintf(stderr, "Erro: crie a pasta 'resultados' na raiz do projeto e tente de novo.\n");
        free(dataset);
        return 1;
    }
    fprintf(f, "arvore,ordem,n,metrica,media,desvio_padrao\n");

    printf("%-10s %-11s %8s %8s %9s %9s %10s %7s %s\n",
           "arvore", "ordem", "n", "altura", "ins(ms)", "busca(us)", "rotacoes", "MB", "invariantes");

    int tudo_ok = 1;
    for (int it = 0; it < num_tamanhos; it++) {
        long n = tamanhos[it];
        if (n > total) {
            printf("n=%ld PULADO (dataset so tem %ld registros)\n", n, total);
            continue;
        }
        for (int io = 0; io < 3; io++) {
            Cenario c = montar_cenario(dataset, n, io);

            for (int ia = 0; ia < 2; ia++) {
                const char *nome = (ia == 0) ? "AVL" : "RubroNegra";
                Execucao ex[REPETICOES];

                if (ia == 0) rodar_avl(&c); else rodar_rn(&c);     /* aquecimento (descartado) */
                for (int r = 0; r < REPETICOES; r++) {
                    ex[r] = (ia == 0) ? rodar_avl(&c) : rodar_rn(&c);
                }

                int ok = 1;
                double medias[NUM_METRICAS];
                for (int r = 0; r < REPETICOES; r++) {
                    ex[r].v[M_T_INSERCAO_OP] = ex[r].v[M_T_INSERCAO] / (double) n;
                    ex[r].v[M_T_REMOCAO_OP] = ex[r].v[M_T_REMOCAO] / (double) c.qtd_remocoes;
                    ex[r].v[M_BYTES_NO] = ex[r].v[M_MEMORIA] / (double) n;
                    ok = ok && ex[r].invariantes_ok;
                }
                for (int m = 0; m < NUM_METRICAS; m++) {
                    double vals[REPETICOES], desvio;
                    for (int r = 0; r < REPETICOES; r++) vals[r] = ex[r].v[m];
                    media_e_desvio(vals, REPETICOES, &medias[m], &desvio);
                    fprintf(f, "%s,%s,%ld,%s,%.10g,%.10g\n",
                            nome, ORDENS[io], n, NOME_METRICA[m], medias[m], desvio);
                }
                fprintf(f, "%s,%s,%ld,invariantes_ok,%d,0\n", nome, ORDENS[io], n, ok);
                tudo_ok = tudo_ok && ok;

                printf("%-10s %-11s %8ld %8.0f %9.2f %9.3f %10.0f %7.1f %s\n",
                       nome, ORDENS[io], n, medias[M_ALTURA], medias[M_T_INSERCAO] * 1e3,
                       medias[M_T_BUSCA] * 1e6, medias[M_ROT_INSERCAO],
                       medias[M_MEMORIA] / (1024.0 * 1024.0), ok ? "ok" : "FALHOU");
            }
            liberar_cenario(&c);
        }
    }

    fclose(f);
    free(dataset);
    printf("\nResultados salvos em %s\n", caminho_csv);
    if (strstr(caminho_csv, "rapido") == NULL) {
        printf("Para gerar os graficos:  python graficos/gerar_graficos.py\n");
    }
    return tudo_ok ? 0 : 1;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        int num = argc - 1;
        long *tamanhos = (long *) malloc(sizeof(long) * (size_t) num);
        for (int i = 0; i < num; i++) tamanhos[i] = atol(argv[i + 1]);
        /* tamanhos escolhidos na mao: arquivo separado, para nao apagar a bateria completa */
        int r = rodar_bateria(tamanhos, num, "resultados_rapido.csv");
        free(tamanhos);
        return r;
    }
    return rodar_bateria(TAMANHOS_PADRAO, NUM_TAMANHOS_PADRAO, "resultados_desempenho.csv");
}
