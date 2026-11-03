#include "avl.h"
#include <stdio.h>
#include <stdlib.h>

/* ---------------- criacao/destruicao de nos ---------------- */

static NoAVL *novo_no(Registro registro) {
    NoAVL *no = (NoAVL *) malloc(sizeof(NoAVL));
    if (no == NULL) {
        fprintf(stderr, "Erro: memoria insuficiente (avl)\n");
        exit(1);
    }
    no->registro = registro;
    no->esquerda = NULL;
    no->direita = NULL;
    no->altura = 1;
    return no;
}

static void destruir_rec(NoAVL *no) {
    if (no == NULL) return;
    destruir_rec(no->esquerda);
    destruir_rec(no->direita);
    free(no);
}

void avl_inicializar(ArvoreAVL *arvore) {
    arvore->raiz = NULL;
    arvore->tamanho = 0;
    avl_zerar_metricas(arvore);
}

void avl_destruir(ArvoreAVL *arvore) {
    destruir_rec(arvore->raiz);
    arvore->raiz = NULL;
    arvore->tamanho = 0;
}

void avl_zerar_metricas(ArvoreAVL *arvore) {
    arvore->rotacoes = 0;
    arvore->comparacoes = 0;
}

size_t avl_memoria_bytes(const ArvoreAVL *arvore) {
    return (size_t) arvore->tamanho * sizeof(NoAVL);
}

/* ---------------- utilitarios de altura/balanceamento ---------------- */

static int altura(const NoAVL *no) {
    return no ? no->altura : 0;
}

static int maior(int a, int b) {
    return a > b ? a : b;
}

static int fator_balanceamento(const NoAVL *no) {
    return no ? altura(no->esquerda) - altura(no->direita) : 0;
}

static void atualizar_altura(NoAVL *no) {
    no->altura = 1 + maior(altura(no->esquerda), altura(no->direita));
}

int avl_altura_total(const ArvoreAVL *arvore) {
    return altura(arvore->raiz);
}

/* ---------------- rotacoes ---------------- */

static NoAVL *rot_direita(ArvoreAVL *arvore, NoAVL *y) {
    NoAVL *x = y->esquerda;
    NoAVL *t2 = x->direita;
    x->direita = y;
    y->esquerda = t2;
    atualizar_altura(y);
    atualizar_altura(x);
    arvore->rotacoes++;
    return x;
}

static NoAVL *rot_esquerda(ArvoreAVL *arvore, NoAVL *x) {
    NoAVL *y = x->direita;
    NoAVL *t2 = y->esquerda;
    y->esquerda = x;
    x->direita = t2;
    atualizar_altura(x);
    atualizar_altura(y);
    arvore->rotacoes++;
    return y;
}

static NoAVL *rebalancear(ArvoreAVL *arvore, NoAVL *no) {
    atualizar_altura(no);
    int fb = fator_balanceamento(no);

    if (fb > 1 && fator_balanceamento(no->esquerda) >= 0) {          /* Esq-Esq */
        return rot_direita(arvore, no);
    }
    if (fb > 1 && fator_balanceamento(no->esquerda) < 0) {           /* Esq-Dir */
        no->esquerda = rot_esquerda(arvore, no->esquerda);
        return rot_direita(arvore, no);
    }
    if (fb < -1 && fator_balanceamento(no->direita) <= 0) {          /* Dir-Dir */
        return rot_esquerda(arvore, no);
    }
    if (fb < -1 && fator_balanceamento(no->direita) > 0) {           /* Dir-Esq */
        no->direita = rot_direita(arvore, no->direita);
        return rot_esquerda(arvore, no);
    }
    return no;
}

/* ---------------- insercao ---------------- */

static NoAVL *inserir_rec(ArvoreAVL *arvore, NoAVL *no, Registro registro) {
    if (no == NULL) {
        arvore->tamanho++;
        return novo_no(registro);
    }

    arvore->comparacoes++;
    int cmp = comparar_registro(&registro, &no->registro);
    if (cmp < 0) {
        no->esquerda = inserir_rec(arvore, no->esquerda, registro);
    } else if (cmp > 0) {
        no->direita = inserir_rec(arvore, no->direita, registro);
    } else {
        no->registro = registro;   /* chave ja existe: atualiza (nao acontece, id e unico) */
        return no;
    }

    return rebalancear(arvore, no);
}

void avl_inserir(ArvoreAVL *arvore, Registro registro) {
    arvore->raiz = inserir_rec(arvore, arvore->raiz, registro);
}

/* ---------------- busca ---------------- */

const Registro *avl_buscar(ArvoreAVL *arvore, const Registro *chave) {
    NoAVL *no = arvore->raiz;
    while (no != NULL) {
        arvore->comparacoes++;
        int cmp = comparar_registro(chave, &no->registro);
        if (cmp == 0) return &no->registro;
        no = (cmp < 0) ? no->esquerda : no->direita;
    }
    return NULL;
}

/* ---------------- remocao ---------------- */

/* Desliga o menor no da subarvore (sem liberar), rebalanceando na volta.
 * O no desligado e devolvido em *minimo. */
static NoAVL *desligar_minimo(ArvoreAVL *arvore, NoAVL *no, NoAVL **minimo) {
    if (no->esquerda == NULL) {
        *minimo = no;
        return no->direita;
    }
    no->esquerda = desligar_minimo(arvore, no->esquerda, minimo);
    return rebalancear(arvore, no);
}

static NoAVL *remover_rec(ArvoreAVL *arvore, NoAVL *no, const Registro *chave, bool *removido) {
    if (no == NULL) {
        return NULL;               /* chave nao existe */
    }

    arvore->comparacoes++;
    int cmp = comparar_registro(chave, &no->registro);
    if (cmp < 0) {
        no->esquerda = remover_rec(arvore, no->esquerda, chave, removido);
    } else if (cmp > 0) {
        no->direita = remover_rec(arvore, no->direita, chave, removido);
    } else {
        *removido = true;
        arvore->tamanho--;
        if (no->esquerda == NULL || no->direita == NULL) {   /* 0 ou 1 filho */
            NoAVL *filho = no->esquerda ? no->esquerda : no->direita;
            free(no);
            return filho;
        }
        /* 2 filhos: o sucessor (menor da direita) assume o lugar deste no */
        NoAVL *sucessor;
        no->direita = desligar_minimo(arvore, no->direita, &sucessor);
        no->registro = sucessor->registro;
        free(sucessor);
    }

    if (!*removido) return no;     /* nada mudou neste caminho */
    return rebalancear(arvore, no);
}

bool avl_remover(ArvoreAVL *arvore, const Registro *chave) {
    bool removido = false;
    arvore->raiz = remover_rec(arvore, arvore->raiz, chave, &removido);
    return removido;
}

/* ---------------- percursos ---------------- */

static void pre_ordem_rec(const NoAVL *no, Registro *saida, long *idx) {
    if (no == NULL) return;
    saida[(*idx)++] = no->registro;
    pre_ordem_rec(no->esquerda, saida, idx);
    pre_ordem_rec(no->direita, saida, idx);
}

static void em_ordem_rec(const NoAVL *no, Registro *saida, long *idx) {
    if (no == NULL) return;
    em_ordem_rec(no->esquerda, saida, idx);
    saida[(*idx)++] = no->registro;
    em_ordem_rec(no->direita, saida, idx);
}

static void pos_ordem_rec(const NoAVL *no, Registro *saida, long *idx) {
    if (no == NULL) return;
    pos_ordem_rec(no->esquerda, saida, idx);
    pos_ordem_rec(no->direita, saida, idx);
    saida[(*idx)++] = no->registro;
}

long avl_pre_ordem(const ArvoreAVL *arvore, Registro *saida) {
    long idx = 0;
    pre_ordem_rec(arvore->raiz, saida, &idx);
    return idx;
}

long avl_em_ordem(const ArvoreAVL *arvore, Registro *saida) {
    long idx = 0;
    em_ordem_rec(arvore->raiz, saida, &idx);
    return idx;
}

long avl_pos_ordem(const ArvoreAVL *arvore, Registro *saida) {
    long idx = 0;
    pos_ordem_rec(arvore->raiz, saida, &idx);
    return idx;
}

/* ---------------- validacao ---------------- */

/*
 * Confere a subarvore de 'no'. 'min' e 'max' sao os limites que a chave do no
 * precisa respeitar (herdados dos ancestrais; NULL = sem limite).
 * Devolve a altura real em *alt e soma os nos em *contagem.
 */
static bool validar_rec(const NoAVL *no, const Registro *min, const Registro *max,
                        int *alt, long *contagem, const char **motivo) {
    if (no == NULL) {
        *alt = 0;
        return true;
    }
    (*contagem)++;

    if ((min && comparar_registro(&no->registro, min) <= 0) ||
        (max && comparar_registro(&no->registro, max) >= 0)) {
        *motivo = "ordem de BST violada (no fora do intervalo dos ancestrais)";
        return false;
    }

    int alt_e, alt_d;
    if (!validar_rec(no->esquerda, min, &no->registro, &alt_e, contagem, motivo)) return false;
    if (!validar_rec(no->direita, &no->registro, max, &alt_d, contagem, motivo)) return false;

    *alt = 1 + maior(alt_e, alt_d);
    if (no->altura != *alt) {
        *motivo = "altura guardada no no diferente da altura real";
        return false;
    }
    int fb = alt_e - alt_d;
    if (fb < -1 || fb > 1) {
        *motivo = "fator de balanceamento fora de [-1, 1]";
        return false;
    }
    return true;
}

bool avl_validar(const ArvoreAVL *arvore, const char **motivo) {
    const char *m = "ok";
    int alt;
    long contagem = 0;
    bool ok = validar_rec(arvore->raiz, NULL, NULL, &alt, &contagem, &m);
    if (ok && contagem != arvore->tamanho) {
        m = "quantidade de nos diferente do campo 'tamanho'";
        ok = false;
    }
    if (motivo) *motivo = m;
    return ok;
}
