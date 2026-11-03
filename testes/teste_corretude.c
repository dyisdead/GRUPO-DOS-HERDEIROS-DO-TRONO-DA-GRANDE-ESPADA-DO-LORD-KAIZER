/*
 * Testes de corretude das duas arvores (AVL e Rubro-Negra).
 *
 * Cobre: insercao, busca, remocao, os 3 percursos, casos extremos, os dados
 * reais do dataset e testes de SABOTAGEM (corrompemos a arvore de proposito e
 * conferimos que o validador detecta -- prova que o validador nao e de enfeite).
 *
 * Todas as invariantes sao conferidas programaticamente por avl_validar() e
 * rn_validar(), que percorrem a arvore inteira.
 *
 * Compilar e rodar (na raiz do projeto):
 *     make testes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../src/registro.h"
#include "../src/avl.h"
#include "../src/rubro_negra.h"
#include "../src/dataset.h"

/* ---------------- mini framework de teste ---------------- */

static int falhas = 0;

/* Diferente do assert(), continua funcionando com -DNDEBUG e mostra onde falhou. */
#define CHECAR(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "FALHOU: %s  (%s:%d)\n", #cond, __FILE__, __LINE__); \
        falhas++; \
        return; \
    } \
} while (0)

/* valida e, se falhar, mostra qual invariante quebrou */
static int avl_ok(const ArvoreAVL *a) {
    const char *motivo;
    if (avl_validar(a, &motivo)) return 1;
    fprintf(stderr, "  AVL invalida: %s\n", motivo);
    return 0;
}

static int rn_ok(const ArvoreRN *a) {
    const char *motivo;
    if (rn_validar(a, &motivo)) return 1;
    fprintf(stderr, "  Rubro-Negra invalida: %s\n", motivo);
    return 0;
}

static double log2_(double x) { return log(x) / log(2.0); }

/* ---------------- geracao de registros sinteticos ---------------- */

static Registro registro_sintetico(long id, long total_stats, const char *nome) {
    Registro r;
    memset(&r, 0, sizeof(r));
    r.id = id;
    if (nome != NULL) {
        snprintf(r.nome, TAM_NOME, "%s", nome);
    } else {
        snprintf(r.nome, TAM_NOME, "Criatura%d", rand() % 50);
    }
    snprintf(r.genero, TAM_GENERO, "%s", (rand() % 2) ? "macho" : "femea");
    r.shiny = (rand() % 100) < 5;
    for (int k = 0; k < NUM_ATAQUES; k++) snprintf(r.ataques[k], TAM_ATAQUE, "Golpe%d", k + 1);
    r.ivs = 1 + rand() % 100;
    r.total_stats = (total_stats >= 0) ? total_stats : (100 + rand() % 500);
    return r;
}

/* ---------------- Caso 1: arvore vazia ---------------- */
static void teste_arvore_vazia(void) {
    Registro chave = registro_sintetico(1, 100, "X");

    ArvoreAVL avl; avl_inicializar(&avl);
    CHECAR(avl_ok(&avl));
    CHECAR(avl_buscar(&avl, &chave) == NULL);
    CHECAR(avl_remover(&avl, &chave) == false);
    CHECAR(avl_altura_total(&avl) == 0);
    avl_destruir(&avl);

    ArvoreRN rn; rn_inicializar(&rn);
    CHECAR(rn_ok(&rn));
    CHECAR(rn_buscar(&rn, &chave) == NULL);
    CHECAR(rn_remover(&rn, &chave) == false);
    CHECAR(rn_altura_total(&rn) == 0);
    rn_destruir(&rn);

    printf("OK - caso 1 (arvore vazia)\n");
}

/* ---------------- Caso 2: unico elemento ---------------- */
static void teste_unico_elemento(void) {
    Registro r = registro_sintetico(1, -1, NULL);
    Registro saida[1];

    ArvoreAVL avl; avl_inicializar(&avl);
    avl_inserir(&avl, r);
    CHECAR(avl_ok(&avl) && avl.tamanho == 1 && avl_altura_total(&avl) == 1);
    CHECAR(avl_buscar(&avl, &r) != NULL);
    CHECAR(avl_em_ordem(&avl, saida) == 1 && saida[0].id == r.id);
    CHECAR(avl_remover(&avl, &r) == true);
    CHECAR(avl_ok(&avl) && avl.tamanho == 0 && avl_altura_total(&avl) == 0);
    CHECAR(avl_remover(&avl, &r) == false);              /* remover de novo: nao existe mais */
    avl_destruir(&avl);

    ArvoreRN rn; rn_inicializar(&rn);
    rn_inserir(&rn, r);
    CHECAR(rn_ok(&rn) && rn.tamanho == 1 && rn_altura_total(&rn) == 1);
    CHECAR(rn_buscar(&rn, &r) != NULL);
    CHECAR(rn_em_ordem(&rn, saida) == 1 && saida[0].id == r.id);
    CHECAR(rn_remover(&rn, &r) == true);
    CHECAR(rn_ok(&rn) && rn.tamanho == 0 && rn_altura_total(&rn) == 0);
    CHECAR(rn_remover(&rn, &r) == false);
    rn_destruir(&rn);

    printf("OK - caso 2 (unico elemento)\n");
}

/* ---------------- Caso 3: mesmo total_stats e mesmo nome ---------------- */
static void teste_chaves_repetidas(void) {
    enum { N = 200 };
    Registro regs[N];
    for (int i = 0; i < N; i++) regs[i] = registro_sintetico(i + 1, 300, "Pikachu");

    Registro saida[N];

    ArvoreAVL avl; avl_inicializar(&avl);
    for (int i = 0; i < N; i++) avl_inserir(&avl, regs[i]);
    CHECAR(avl_ok(&avl) && avl.tamanho == N);
    for (int i = 0; i < N; i++) CHECAR(avl_buscar(&avl, &regs[i]) != NULL);
    CHECAR(avl_em_ordem(&avl, saida) == N);
    for (int i = 0; i < N; i++) CHECAR(saida[i].id == i + 1);   /* desempate pelo id */
    avl_destruir(&avl);

    ArvoreRN rn; rn_inicializar(&rn);
    for (int i = 0; i < N; i++) rn_inserir(&rn, regs[i]);
    CHECAR(rn_ok(&rn) && rn.tamanho == N);
    for (int i = 0; i < N; i++) CHECAR(rn_buscar(&rn, &regs[i]) != NULL);
    CHECAR(rn_em_ordem(&rn, saida) == N);
    for (int i = 0; i < N; i++) CHECAR(saida[i].id == i + 1);
    rn_destruir(&rn);

    printf("OK - caso 3 (mesmo total_stats e mesmo nome: desempata pelo id)\n");
}

/* ---------------- Caso 4: inserir a mesma chave duas vezes ---------------- */
static void teste_insercao_duplicada(void) {
    Registro r = registro_sintetico(7, 500, "Mew");

    ArvoreAVL avl; avl_inicializar(&avl);
    avl_inserir(&avl, r);
    r.ivs = 99;                                  /* mesma chave, conteudo novo */
    avl_inserir(&avl, r);
    CHECAR(avl_ok(&avl) && avl.tamanho == 1);
    CHECAR(avl_buscar(&avl, &r)->ivs == 99);     /* atualizou em vez de duplicar */
    avl_destruir(&avl);

    r.ivs = 1;
    ArvoreRN rn; rn_inicializar(&rn);
    rn_inserir(&rn, r);
    r.ivs = 99;
    rn_inserir(&rn, r);
    CHECAR(rn_ok(&rn) && rn.tamanho == 1);
    CHECAR(rn_buscar(&rn, &r)->ivs == 99);
    rn_destruir(&rn);

    printf("OK - caso 4 (mesma chave duas vezes: atualiza, nao duplica)\n");
}

/* ---------------- Caso 5: ordem crescente/decrescente ---------------- */
static void teste_ordem_monotona(void) {
    const int n = 20000;
    /* limites teoricos de altura */
    double limite_avl = 1.4405 * log2_(n + 2) - 0.3277;   /* AVL: ~1,44 log2(n) */
    double limite_rn = 2.0 * log2_(n + 1);                /* RN: 2 log2(n+1)    */

    for (int ordem = 0; ordem < 2; ordem++) {   /* 0 = crescente, 1 = decrescente */
        ArvoreAVL avl; avl_inicializar(&avl);
        ArvoreRN rn; rn_inicializar(&rn);

        for (int i = 0; i < n; i++) {
            long total = (ordem == 0) ? i : (n - 1 - i);
            Registro r = registro_sintetico(i + 1, total, "Magikarp");
            avl_inserir(&avl, r);
            rn_inserir(&rn, r);
        }

        CHECAR(avl_ok(&avl));
        CHECAR(rn_ok(&rn));
        /* uma BST simples teria altura n = 20000 aqui */
        CHECAR(avl_altura_total(&avl) <= limite_avl);
        CHECAR(rn_altura_total(&rn) <= limite_rn);

        avl_destruir(&avl);
        rn_destruir(&rn);
    }

    printf("OK - caso 5 (ordem crescente/decrescente nao degenera; altura <= limite teorico)\n");
}

/* ---------------- Caso 6: os 3 percursos ---------------- */
static void teste_percursos(void) {
    /* arvore conhecida: inserindo 1..7 em ordem, a AVL fica perfeitamente cheia
     *            4
     *        2       6
     *      1   3   5   7
     */
    ArvoreAVL avl; avl_inicializar(&avl);
    for (int i = 1; i <= 7; i++) avl_inserir(&avl, registro_sintetico(i, i, "P"));
    CHECAR(avl_ok(&avl) && avl_altura_total(&avl) == 3);

    Registro s[7];
    long esperado_pre[7] = {4, 2, 1, 3, 6, 5, 7};
    long esperado_em[7]  = {1, 2, 3, 4, 5, 6, 7};
    long esperado_pos[7] = {1, 3, 2, 5, 7, 6, 4};

    CHECAR(avl_pre_ordem(&avl, s) == 7);
    for (int i = 0; i < 7; i++) CHECAR(s[i].id == esperado_pre[i]);
    CHECAR(avl_em_ordem(&avl, s) == 7);
    for (int i = 0; i < 7; i++) CHECAR(s[i].id == esperado_em[i]);
    CHECAR(avl_pos_ordem(&avl, s) == 7);
    for (int i = 0; i < 7; i++) CHECAR(s[i].id == esperado_pos[i]);
    avl_destruir(&avl);

    /* RN: em-ordem sempre ordenado; pre-ordem comeca pela raiz; pos-ordem termina nela */
    ArvoreRN rn; rn_inicializar(&rn);
    for (int i = 1; i <= 7; i++) rn_inserir(&rn, registro_sintetico(i, i, "P"));
    CHECAR(rn_ok(&rn));
    CHECAR(rn_em_ordem(&rn, s) == 7);
    for (int i = 0; i < 7; i++) CHECAR(s[i].id == esperado_em[i]);
    CHECAR(rn_pre_ordem(&rn, s) == 7 && s[0].id == rn.raiz->registro.id);
    CHECAR(rn_pos_ordem(&rn, s) == 7 && s[6].id == rn.raiz->registro.id);
    rn_destruir(&rn);

    printf("OK - caso 6 (pre-ordem, em-ordem e pos-ordem)\n");
}

/* ---------------- Caso 7: insercoes e remocoes misturadas ---------------- */
static void teste_operacoes_misturadas(void) {
    const int n = 20000;
    Registro *regs = (Registro *) malloc(sizeof(Registro) * n);
    char *presente = (char *) calloc(n, 1);
    for (int i = 0; i < n; i++) regs[i] = registro_sintetico(i + 1, -1, NULL);

    ArvoreAVL avl; avl_inicializar(&avl);
    ArvoreRN rn; rn_inicializar(&rn);
    long tamanho_esperado = 0;

    /* 60.000 operacoes aleatorias; valida a cada 1.000 */
    for (int op = 1; op <= 60000; op++) {
        int i = rand() % n;
        if (rand() % 3 != 0) {                  /* 2/3 insercao */
            avl_inserir(&avl, regs[i]);
            rn_inserir(&rn, regs[i]);
            if (!presente[i]) { presente[i] = 1; tamanho_esperado++; }
        } else {                                /* 1/3 remocao */
            bool esperado = presente[i];
            CHECAR(avl_remover(&avl, &regs[i]) == esperado);
            CHECAR(rn_remover(&rn, &regs[i]) == esperado);
            if (esperado) { presente[i] = 0; tamanho_esperado--; }
        }
        if (op % 1000 == 0) {
            CHECAR(avl_ok(&avl) && rn_ok(&rn));
            CHECAR(avl.tamanho == tamanho_esperado && rn.tamanho == tamanho_esperado);
        }
    }

    /* busca bate exatamente com o que deveria estar na arvore */
    for (int i = 0; i < n; i++) {
        CHECAR((avl_buscar(&avl, &regs[i]) != NULL) == presente[i]);
        CHECAR((rn_buscar(&rn, &regs[i]) != NULL) == presente[i]);
    }

    free(regs);
    free(presente);
    avl_destruir(&avl);
    rn_destruir(&rn);

    printf("OK - caso 7 (60.000 insercoes/remocoes aleatorias misturadas)\n");
}

/* ---------------- Caso 8: remover tudo ate esvaziar ---------------- */
static void teste_remover_tudo(void) {
    const int n = 3000;
    Registro *regs = (Registro *) malloc(sizeof(Registro) * n);
    for (int i = 0; i < n; i++) regs[i] = registro_sintetico(i + 1, -1, NULL);

    ArvoreAVL avl; avl_inicializar(&avl);
    ArvoreRN rn; rn_inicializar(&rn);
    for (int i = 0; i < n; i++) { avl_inserir(&avl, regs[i]); rn_inserir(&rn, regs[i]); }

    /* embaralha a ordem de remocao (Fisher-Yates) */
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Registro tmp = regs[i]; regs[i] = regs[j]; regs[j] = tmp;
    }

    /* valida depois de CADA remocao */
    for (int i = 0; i < n; i++) {
        CHECAR(avl_remover(&avl, &regs[i]) == true);
        CHECAR(avl_ok(&avl));
        CHECAR(rn_remover(&rn, &regs[i]) == true);
        CHECAR(rn_ok(&rn));
    }

    CHECAR(avl.tamanho == 0 && avl_altura_total(&avl) == 0);
    CHECAR(rn.tamanho == 0 && rn_altura_total(&rn) == 0);

    free(regs);
    avl_destruir(&avl);
    rn_destruir(&rn);

    printf("OK - caso 8 (remover tudo ate esvaziar, validando a cada remocao)\n");
}

/* ---------------- Caso 9: sabotagem -- o validador precisa reclamar ---------------- */
static void teste_validador_detecta_erros(void) {
    const char *motivo;

    /* --- AVL --- */
    ArvoreAVL avl; avl_inicializar(&avl);
    for (int i = 1; i <= 15; i++) avl_inserir(&avl, registro_sintetico(i, i * 10, "S"));
    CHECAR(avl_validar(&avl, NULL));

    /* (a) quebra a ordem: neto da esquerda com chave maior que a raiz */
    long original = avl.raiz->esquerda->direita->registro.total_stats;
    avl.raiz->esquerda->direita->registro.total_stats = 10000;
    CHECAR(!avl_validar(&avl, &motivo) && strstr(motivo, "ordem") != NULL);
    avl.raiz->esquerda->direita->registro.total_stats = original;

    /* (b) altura guardada errada */
    avl.raiz->esquerda->altura += 5;
    CHECAR(!avl_validar(&avl, &motivo) && strstr(motivo, "altura") != NULL);
    avl.raiz->esquerda->altura -= 5;

    /* (c) desbalanceia: pendura uma subarvore so na esquerda */
    NoAVL *folha = avl.raiz;
    while (folha->esquerda) folha = folha->esquerda;
    for (int k = 0; k < 3; k++) {
        NoAVL *novo = (NoAVL *) calloc(1, sizeof(NoAVL));
        novo->registro = registro_sintetico(-(k + 1), -(k + 1), "S");
        novo->altura = 1;
        folha->esquerda = novo;
        folha = novo;
    }
    avl.tamanho += 3;
    CHECAR(!avl_validar(&avl, &motivo));   /* altura ou balanceamento: qualquer um serve */
    avl_destruir(&avl);

    /* --- Rubro-Negra --- */
    ArvoreRN rn; rn_inicializar(&rn);
    for (int i = 1; i <= 31; i++) rn_inserir(&rn, registro_sintetico(i, i * 10, "S"));
    CHECAR(rn_validar(&rn, NULL));

    /* (a) raiz vermelha */
    rn.raiz->cor = VERMELHO;
    CHECAR(!rn_validar(&rn, &motivo) && strstr(motivo, "propriedade 2") != NULL);
    rn.raiz->cor = PRETO;

    /* (b) altura preta desigual: pinta um filho preto da raiz de vermelho */
    NoRN *filho = rn.raiz->esquerda;
    Cor cor_original = filho->cor;
    filho->cor = (cor_original == PRETO) ? VERMELHO : PRETO;
    CHECAR(!rn_validar(&rn, &motivo));
    filho->cor = cor_original;

    /* (c) ponteiro pai quebrado */
    NoRN *pai_original = rn.raiz->direita->pai;
    rn.raiz->direita->pai = rn.raiz->esquerda;
    CHECAR(!rn_validar(&rn, &motivo) && strstr(motivo, "pai") != NULL);
    rn.raiz->direita->pai = pai_original;

    /* (d) ordem quebrada */
    long orig = rn.raiz->direita->registro.total_stats;
    rn.raiz->direita->registro.total_stats = -1;
    CHECAR(!rn_validar(&rn, &motivo) && strstr(motivo, "ordem") != NULL);
    rn.raiz->direita->registro.total_stats = orig;

    /* (e) tamanho errado */
    rn.tamanho++;
    CHECAR(!rn_validar(&rn, &motivo) && strstr(motivo, "tamanho") != NULL);
    rn.tamanho--;

    CHECAR(rn_validar(&rn, NULL));        /* desfeitas as sabotagens, volta a ser valida */
    rn_destruir(&rn);

    printf("OK - caso 9 (sabotagem: os validadores detectam arvores corrompidas)\n");
}

/* ---------------- Caso 10: dados REAIS (PokeAPI) ---------------- */
static void teste_dataset_real(void) {
    const char *caminho = dataset_localizar();
    if (caminho == NULL) {
        printf("AVISO - teste com dados reais PULADO: dataset nao encontrado.\n"
               "        Gere com: python dataset/gerar_dataset.py\n");
        return;
    }

    const long n_max = 100000;
    Registro *regs = NULL;
    long n = dataset_carregar(caminho, n_max, &regs);
    CHECAR(n > 0);

    /* sanidade dos dados lidos do CSV */
    for (long i = 0; i < n; i++) {
        CHECAR(regs[i].id == i + 1);
        CHECAR(regs[i].nome[0] != '\0');
        CHECAR(strcmp(regs[i].genero, "macho") == 0 || strcmp(regs[i].genero, "femea") == 0 ||
               strcmp(regs[i].genero, "sem genero") == 0);
        CHECAR(regs[i].ivs >= 1 && regs[i].ivs <= 100);
        CHECAR(regs[i].total_stats > 0);
        CHECAR(regs[i].ataques[0][0] != '\0');
    }

    ArvoreAVL avl; avl_inicializar(&avl);
    ArvoreRN rn; rn_inicializar(&rn);
    for (long i = 0; i < n; i++) { avl_inserir(&avl, regs[i]); rn_inserir(&rn, regs[i]); }
    CHECAR(avl_ok(&avl) && avl.tamanho == n);
    CHECAR(rn_ok(&rn) && rn.tamanho == n);

    /* em-ordem: n registros, estritamente crescentes pela chave (total_stats, nome, id) */
    Registro *saida = (Registro *) malloc(sizeof(Registro) * (size_t) n);
    CHECAR(avl_em_ordem(&avl, saida) == n);
    for (long i = 1; i < n; i++) CHECAR(comparar_registro(&saida[i - 1], &saida[i]) < 0);
    CHECAR(rn_em_ordem(&rn, saida) == n);
    for (long i = 1; i < n; i++) CHECAR(comparar_registro(&saida[i - 1], &saida[i]) < 0);

    /* todos os registros sao encontrados e voltam com o mesmo conteudo */
    for (long i = 0; i < n; i++) {
        const Registro *a = avl_buscar(&avl, &regs[i]);
        const Registro *b = rn_buscar(&rn, &regs[i]);
        CHECAR(a != NULL && b != NULL);
        CHECAR(memcmp(a, &regs[i], sizeof(Registro)) == 0);
        CHECAR(memcmp(b, &regs[i], sizeof(Registro)) == 0);
    }

    /* remocao em 10 lotes de 1% (registros espalhados), validando apos CADA lote */
    long removidos = 0;
    for (int lote = 0; lote < 10; lote++) {
        for (long i = lote; i < n; i += 100) {
            CHECAR(avl_remover(&avl, &regs[i]) == true);
            CHECAR(rn_remover(&rn, &regs[i]) == true);
            removidos++;
        }
        CHECAR(avl_ok(&avl));
        CHECAR(rn_ok(&rn));
    }
    for (long i = 0; i < n; i++) {
        bool removido = (i % 100) < 10;
        CHECAR((avl_buscar(&avl, &regs[i]) != NULL) == !removido);
        CHECAR((rn_buscar(&rn, &regs[i]) != NULL) == !removido);
    }
    CHECAR(avl.tamanho == n - removidos && rn.tamanho == n - removidos);

    printf("OK - caso 10 (dados reais da PokeAPI: %ld registros de %s)\n", n, caminho);
    printf("     altura: AVL=%d  RN=%d (log2(n) = %.1f) | %ld removidos e arvores ainda validas\n",
           avl_altura_total(&avl), rn_altura_total(&rn), log2_((double) n), removidos);

    free(saida);
    free(regs);
    avl_destruir(&avl);
    rn_destruir(&rn);
}

int main(void) {
    srand(42);

    teste_arvore_vazia();
    teste_unico_elemento();
    teste_chaves_repetidas();
    teste_insercao_duplicada();
    teste_ordem_monotona();
    teste_percursos();
    teste_operacoes_misturadas();
    teste_remover_tudo();
    teste_validador_detecta_erros();
    teste_dataset_real();

    if (falhas > 0) {
        printf("\n%d teste(s) FALHARAM.\n", falhas);
        return 1;
    }
    printf("\nTodos os testes de corretude passaram.\n");
    return 0;
}
