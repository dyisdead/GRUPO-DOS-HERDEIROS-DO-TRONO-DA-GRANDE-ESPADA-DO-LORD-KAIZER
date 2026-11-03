#include "rubro_negra.h"
#include <stdio.h>
#include <stdlib.h>

/* ---------------- criação/destruição ---------------- */

static NoRN *alocar_no(void) {
    NoRN *no = (NoRN *) malloc(sizeof(NoRN));
    if (no == NULL) {
        fprintf(stderr, "Erro: memoria insuficiente (rubro-negra)\n");
        exit(1);
    }
    return no;
}

void rn_zerar_metricas(ArvoreRN *arvore) {
    arvore->rotacoes = 0;
    arvore->recoloracoes = 0;
    arvore->comparacoes = 0;
}

size_t rn_memoria_bytes(const ArvoreRN *arvore) {
    return (size_t) (arvore->tamanho + 1) * sizeof(NoRN);   /* +1 = sentinela NIL */
}

void rn_inicializar(ArvoreRN *arvore) {
    arvore->nil = alocar_no();
    arvore->nil->cor = PRETO;
    arvore->nil->esquerda = arvore->nil;
    arvore->nil->direita = arvore->nil;
    arvore->nil->pai = arvore->nil;
    arvore->raiz = arvore->nil;
    arvore->tamanho = 0;
    rn_zerar_metricas(arvore);
}

static void destruir_rec(ArvoreRN *arvore, NoRN *no) {
    if (no == arvore->nil) return;
    destruir_rec(arvore, no->esquerda);
    destruir_rec(arvore, no->direita);
    free(no);
}

void rn_destruir(ArvoreRN *arvore) {
    destruir_rec(arvore, arvore->raiz);
    arvore->raiz = NULL;
    arvore->tamanho = 0;
    free(arvore->nil);
    arvore->nil = NULL;
}

/* ---------------- rotações ---------------- */

static void rot_esquerda(ArvoreRN *arvore, NoRN *x) {
    NoRN *y = x->direita;
    x->direita = y->esquerda;
    if (y->esquerda != arvore->nil) y->esquerda->pai = x;
    y->pai = x->pai;
    if (x->pai == arvore->nil) {
        arvore->raiz = y;
    } else if (x == x->pai->esquerda) {
        x->pai->esquerda = y;
    } else {
        x->pai->direita = y;
    }
    y->esquerda = x;
    x->pai = y;
    arvore->rotacoes++;
}

static void rot_direita(ArvoreRN *arvore, NoRN *x) {
    NoRN *y = x->esquerda;
    x->esquerda = y->direita;
    if (y->direita != arvore->nil) y->direita->pai = x;
    y->pai = x->pai;
    if (x->pai == arvore->nil) {
        arvore->raiz = y;
    } else if (x == x->pai->direita) {
        x->pai->direita = y;
    } else {
        x->pai->esquerda = y;
    }
    y->direita = x;
    x->pai = y;
    arvore->rotacoes++;
}

/* ---------------- inserção ---------------- */

static void corrigir_insercao(ArvoreRN *arvore, NoRN *z) {
    while (z->pai->cor == VERMELHO) {
        if (z->pai == z->pai->pai->esquerda) {
            NoRN *tio = z->pai->pai->direita;
            if (tio->cor == VERMELHO) {                      /* Caso 1 */
                z->pai->cor = PRETO;
                tio->cor = PRETO;
                z->pai->pai->cor = VERMELHO;
                arvore->recoloracoes += 3;
                z = z->pai->pai;
            } else {
                if (z == z->pai->direita) {                   /* Caso 2 */
                    z = z->pai;
                    rot_esquerda(arvore, z);
                }
                z->pai->cor = PRETO;                            /* Caso 3 */
                z->pai->pai->cor = VERMELHO;
                arvore->recoloracoes += 2;
                rot_direita(arvore, z->pai->pai);
            }
        } else {
            NoRN *tio = z->pai->pai->esquerda;
            if (tio->cor == VERMELHO) {                      /* Caso 1 (espelhado) */
                z->pai->cor = PRETO;
                tio->cor = PRETO;
                z->pai->pai->cor = VERMELHO;
                arvore->recoloracoes += 3;
                z = z->pai->pai;
            } else {
                if (z == z->pai->esquerda) {                   /* Caso 2 (espelhado) */
                    z = z->pai;
                    rot_direita(arvore, z);
                }
                z->pai->cor = PRETO;                            /* Caso 3 (espelhado) */
                z->pai->pai->cor = VERMELHO;
                arvore->recoloracoes += 2;
                rot_esquerda(arvore, z->pai->pai);
            }
        }
    }
    arvore->raiz->cor = PRETO;
}

void rn_inserir(ArvoreRN *arvore, Registro registro) {
    NoRN *novo = alocar_no();
    novo->registro = registro;
    novo->cor = VERMELHO;
    novo->esquerda = arvore->nil;
    novo->direita = arvore->nil;

    NoRN *y = arvore->nil;
    NoRN *x = arvore->raiz;
    bool lado_esquerdo = false;
    while (x != arvore->nil) {
        y = x;
        arvore->comparacoes++;
        int cmp = comparar_registro(&registro, &x->registro);
        if (cmp < 0) {
            x = x->esquerda;
        } else if (cmp > 0) {
            x = x->direita;
        } else {
            x->registro = registro;   /* chave ja existe: atualiza (nao acontece, id e unico) */
            free(novo);
            return;
        }
        lado_esquerdo = (cmp < 0);
    }

    /* 'lado_esquerdo' guarda o resultado da ultima comparacao: nao precisa comparar de novo */
    novo->pai = y;
    if (y == arvore->nil) {
        arvore->raiz = novo;
    } else if (lado_esquerdo) {
        y->esquerda = novo;
    } else {
        y->direita = novo;
    }
    arvore->tamanho++;

    corrigir_insercao(arvore, novo);
}

/* ---------------- busca ---------------- */

const Registro *rn_buscar(ArvoreRN *arvore, const Registro *chave) {
    NoRN *x = arvore->raiz;
    while (x != arvore->nil) {
        arvore->comparacoes++;
        int cmp = comparar_registro(chave, &x->registro);
        if (cmp == 0) return &x->registro;
        x = (cmp < 0) ? x->esquerda : x->direita;
    }
    return NULL;
}

/* ---------------- remoção ---------------- */

static void transplantar(ArvoreRN *arvore, NoRN *u, NoRN *v) {
    if (u->pai == arvore->nil) {
        arvore->raiz = v;
    } else if (u == u->pai->esquerda) {
        u->pai->esquerda = v;
    } else {
        u->pai->direita = v;
    }
    v->pai = u->pai;
}

static NoRN *minimo(ArvoreRN *arvore, NoRN *no) {
    while (no->esquerda != arvore->nil) no = no->esquerda;
    return no;
}

static void corrigir_remocao(ArvoreRN *arvore, NoRN *x) {
    while (x != arvore->raiz && x->cor == PRETO) {
        if (x == x->pai->esquerda) {
            NoRN *w = x->pai->direita;
            if (w->cor == VERMELHO) {                              /* Caso 1 */
                w->cor = PRETO;
                x->pai->cor = VERMELHO;
                arvore->recoloracoes += 2;
                rot_esquerda(arvore, x->pai);
                w = x->pai->direita;
            }
            if (w->esquerda->cor == PRETO && w->direita->cor == PRETO) {  /* Caso 2 */
                w->cor = VERMELHO;
                arvore->recoloracoes += 1;
                x = x->pai;
            } else {
                if (w->direita->cor == PRETO) {                     /* Caso 3 */
                    w->esquerda->cor = PRETO;
                    w->cor = VERMELHO;
                    arvore->recoloracoes += 2;
                    rot_direita(arvore, w);
                    w = x->pai->direita;
                }
                w->cor = x->pai->cor;                                 /* Caso 4 */
                x->pai->cor = PRETO;
                w->direita->cor = PRETO;
                arvore->recoloracoes += 3;
                rot_esquerda(arvore, x->pai);
                x = arvore->raiz;
            }
        } else {
            NoRN *w = x->pai->esquerda;
            if (w->cor == VERMELHO) {                              /* Caso 1 (espelhado) */
                w->cor = PRETO;
                x->pai->cor = VERMELHO;
                arvore->recoloracoes += 2;
                rot_direita(arvore, x->pai);
                w = x->pai->esquerda;
            }
            if (w->direita->cor == PRETO && w->esquerda->cor == PRETO) {  /* Caso 2 */
                w->cor = VERMELHO;
                arvore->recoloracoes += 1;
                x = x->pai;
            } else {
                if (w->esquerda->cor == PRETO) {                     /* Caso 3 (espelhado) */
                    w->direita->cor = PRETO;
                    w->cor = VERMELHO;
                    arvore->recoloracoes += 2;
                    rot_esquerda(arvore, w);
                    w = x->pai->esquerda;
                }
                w->cor = x->pai->cor;                                 /* Caso 4 (espelhado) */
                x->pai->cor = PRETO;
                w->esquerda->cor = PRETO;
                arvore->recoloracoes += 3;
                rot_direita(arvore, x->pai);
                x = arvore->raiz;
            }
        }
    }
    x->cor = PRETO;
}

bool rn_remover(ArvoreRN *arvore, const Registro *chave) {
    NoRN *z = arvore->raiz;
    while (z != arvore->nil) {
        arvore->comparacoes++;
        int cmp = comparar_registro(chave, &z->registro);
        if (cmp == 0) break;
        z = (cmp < 0) ? z->esquerda : z->direita;
    }
    if (z == arvore->nil) return false;   /* não encontrado */

    NoRN *y = z;
    Cor cor_original_y = y->cor;
    NoRN *x;

    if (z->esquerda == arvore->nil) {
        x = z->direita;
        transplantar(arvore, z, z->direita);
    } else if (z->direita == arvore->nil) {
        x = z->esquerda;
        transplantar(arvore, z, z->esquerda);
    } else {
        y = minimo(arvore, z->direita);
        cor_original_y = y->cor;
        x = y->direita;
        if (y->pai == z) {
            x->pai = y;
        } else {
            transplantar(arvore, y, y->direita);
            y->direita = z->direita;
            y->direita->pai = y;
        }
        transplantar(arvore, z, y);
        y->esquerda = z->esquerda;
        y->esquerda->pai = y;
        y->cor = z->cor;
    }

    free(z);
    arvore->tamanho--;

    if (cor_original_y == PRETO) {
        corrigir_remocao(arvore, x);
    }
    return true;
}

/* ---------------- percursos ---------------- */

static void pre_ordem_rec(ArvoreRN *arvore, NoRN *no, Registro *saida, long *idx) {
    if (no == arvore->nil) return;
    saida[(*idx)++] = no->registro;
    pre_ordem_rec(arvore, no->esquerda, saida, idx);
    pre_ordem_rec(arvore, no->direita, saida, idx);
}

static void em_ordem_rec(ArvoreRN *arvore, NoRN *no, Registro *saida, long *idx) {
    if (no == arvore->nil) return;
    em_ordem_rec(arvore, no->esquerda, saida, idx);
    saida[(*idx)++] = no->registro;
    em_ordem_rec(arvore, no->direita, saida, idx);
}

static void pos_ordem_rec(ArvoreRN *arvore, NoRN *no, Registro *saida, long *idx) {
    if (no == arvore->nil) return;
    pos_ordem_rec(arvore, no->esquerda, saida, idx);
    pos_ordem_rec(arvore, no->direita, saida, idx);
    saida[(*idx)++] = no->registro;
}

long rn_pre_ordem(const ArvoreRN *arvore, Registro *saida) {
    long idx = 0;
    pre_ordem_rec((ArvoreRN *) arvore, arvore->raiz, saida, &idx);
    return idx;
}

long rn_em_ordem(const ArvoreRN *arvore, Registro *saida) {
    long idx = 0;
    em_ordem_rec((ArvoreRN *) arvore, arvore->raiz, saida, &idx);
    return idx;
}

long rn_pos_ordem(const ArvoreRN *arvore, Registro *saida) {
    long idx = 0;
    pos_ordem_rec((ArvoreRN *) arvore, arvore->raiz, saida, &idx);
    return idx;
}

/* ---------------- altura ---------------- */

static int altura_rec(const ArvoreRN *arvore, NoRN *no) {
    if (no == arvore->nil) return 0;
    int alt_e = altura_rec(arvore, no->esquerda);
    int alt_d = altura_rec(arvore, no->direita);
    return 1 + (alt_e > alt_d ? alt_e : alt_d);
}

int rn_altura_total(const ArvoreRN *arvore) {
    return altura_rec(arvore, arvore->raiz);
}

/* ---------------- validacao ---------------- */

/*
 * Confere a subarvore de 'no'. 'min'/'max' sao os limites herdados dos
 * ancestrais (NULL = sem limite). Devolve a altura preta (contando o NIL)
 * em *altura_preta e soma os nos em *contagem.
 */
static bool validar_rec(const ArvoreRN *arvore, const NoRN *no,
                        const Registro *min, const Registro *max,
                        int *altura_preta, long *contagem, const char **motivo) {
    if (no == arvore->nil) {
        *altura_preta = 1;                                    /* NIL conta como preto */
        return true;
    }
    (*contagem)++;

    if (no->cor != PRETO && no->cor != VERMELHO) {
        *motivo = "propriedade 1: no com cor invalida";
        return false;
    }
    if ((min && comparar_registro(&no->registro, min) <= 0) ||
        (max && comparar_registro(&no->registro, max) >= 0)) {
        *motivo = "ordem de BST violada (no fora do intervalo dos ancestrais)";
        return false;
    }
    if ((no->esquerda != arvore->nil && no->esquerda->pai != no) ||
        (no->direita != arvore->nil && no->direita->pai != no)) {
        *motivo = "ponteiro 'pai' inconsistente";
        return false;
    }
    if (no->cor == VERMELHO &&
        (no->esquerda->cor == VERMELHO || no->direita->cor == VERMELHO)) {
        *motivo = "propriedade 4: no vermelho com filho vermelho";
        return false;
    }

    int alt_e, alt_d;
    if (!validar_rec(arvore, no->esquerda, min, &no->registro, &alt_e, contagem, motivo)) return false;
    if (!validar_rec(arvore, no->direita, &no->registro, max, &alt_d, contagem, motivo)) return false;
    if (alt_e != alt_d) {
        *motivo = "propriedade 5: caminhos com quantidade diferente de nos pretos";
        return false;
    }
    *altura_preta = alt_e + (no->cor == PRETO ? 1 : 0);
    return true;
}

bool rn_validar(const ArvoreRN *arvore, const char **motivo) {
    const char *m = "ok";
    bool ok = true;
    int alt_preta;
    long contagem = 0;

    if (arvore->nil->cor != PRETO) {
        m = "propriedade 3: sentinela NIL nao e preto";
        ok = false;
    } else if (arvore->raiz->cor != PRETO) {
        m = "propriedade 2: raiz nao e preta";
        ok = false;
    } else if (arvore->raiz != arvore->nil && arvore->raiz->pai != arvore->nil) {
        m = "ponteiro 'pai' da raiz deveria ser NIL";
        ok = false;
    } else if (!validar_rec(arvore, arvore->raiz, NULL, NULL, &alt_preta, &contagem, &m)) {
        ok = false;
    } else if (contagem != arvore->tamanho) {
        m = "quantidade de nos diferente do campo 'tamanho'";
        ok = false;
    }
    if (motivo) *motivo = m;
    return ok;
}
