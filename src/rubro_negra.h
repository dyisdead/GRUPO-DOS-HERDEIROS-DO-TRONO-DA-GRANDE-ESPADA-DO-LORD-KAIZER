/*
 * Arvore Rubro-Negra para o dataset PokeTree.
 * Implementacao classica (Cormen/CLRS) com no sentinela NIL.
 */

#ifndef RUBRO_NEGRA_H
#define RUBRO_NEGRA_H

#include <stdbool.h>
#include <stddef.h>
#include "registro.h"

typedef enum { PRETO, VERMELHO } Cor;

typedef struct NoRN {
    Registro registro;
    Cor cor;
    struct NoRN *esquerda;
    struct NoRN *direita;
    struct NoRN *pai;
} NoRN;

typedef struct {
    NoRN *nil;            /* sentinela: representa todas as folhas */
    NoRN *raiz;
    long tamanho;         /* quantidade de nos (sem contar o NIL) */

    /* metricas de desempenho (acumulam ate chamar rn_zerar_metricas) */
    long rotacoes;
    long recoloracoes;
    long comparacoes;     /* comparacoes de chave em inserir/buscar/remover */
} ArvoreRN;

void rn_inicializar(ArvoreRN *arvore);
void rn_destruir(ArvoreRN *arvore);
void rn_zerar_metricas(ArvoreRN *arvore);

void rn_inserir(ArvoreRN *arvore, Registro registro);
bool rn_remover(ArvoreRN *arvore, const Registro *chave);
const Registro *rn_buscar(ArvoreRN *arvore, const Registro *chave);

int    rn_altura_total(const ArvoreRN *arvore);
size_t rn_memoria_bytes(const ArvoreRN *arvore); /* (tamanho + 1 NIL) * sizeof(NoRN) */

/*
 * Valida TODAS as invariantes da Rubro-Negra, percorrendo a arvore inteira:
 *   1) todo no e vermelho ou preto
 *   2) a raiz e preta
 *   3) as folhas (NIL) sao pretas
 *   4) no vermelho nao tem filho vermelho
 *   5) todo caminho de um no ate as folhas tem a mesma quantidade de pretos
 * e tambem a estrutura:
 *   - ordem de BST (esquerda < no < direita, respeitando os ancestrais)
 *   - ponteiros 'pai' consistentes (filho->pai == no; raiz->pai == NIL)
 *   - a quantidade de nos bate com 'tamanho'
 * Se 'motivo' nao for NULL, recebe a primeira invariante violada (ou "ok").
 */
bool rn_validar(const ArvoreRN *arvore, const char **motivo);

long rn_pre_ordem(const ArvoreRN *arvore, Registro *saida);
long rn_em_ordem(const ArvoreRN *arvore, Registro *saida);
long rn_pos_ordem(const ArvoreRN *arvore, Registro *saida);

#endif /* RUBRO_NEGRA_H */
