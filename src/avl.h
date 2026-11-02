/*
 * Arvore AVL para o dataset PokeTree.
 * Balanceamento estrito: fator de balanceamento de cada no sempre entre -1 e 1.
 */

#ifndef AVL_H
#define AVL_H

#include <stdbool.h>
#include <stddef.h>
#include "registro.h"

typedef struct NoAVL {
    Registro registro;
    struct NoAVL *esquerda;
    struct NoAVL *direita;
    int altura;
} NoAVL;

typedef struct {
    NoAVL *raiz;
    long tamanho;             /* quantidade de nos na arvore */

    /* metricas de desempenho (acumulam ate chamar avl_zerar_metricas) */
    long rotacoes;            /* rotacoes simples (uma dupla conta 2) */
    long comparacoes;         /* comparacoes de chave em inserir/buscar/remover */
} ArvoreAVL;

void avl_inicializar(ArvoreAVL *arvore);
void avl_destruir(ArvoreAVL *arvore);           /* libera todos os nos */
void avl_zerar_metricas(ArvoreAVL *arvore);

void avl_inserir(ArvoreAVL *arvore, Registro registro);
bool avl_remover(ArvoreAVL *arvore, const Registro *chave);
const Registro *avl_buscar(ArvoreAVL *arvore, const Registro *chave);

int    avl_altura_total(const ArvoreAVL *arvore);
size_t avl_memoria_bytes(const ArvoreAVL *arvore); /* tamanho * sizeof(NoAVL) */

/*
 * Valida TODAS as invariantes da AVL, percorrendo a arvore inteira:
 *   1) ordem de BST: todo no da subarvore esquerda < no < toda a direita
 *   2) o campo 'altura' guardado em cada no bate com a altura real
 *   3) fator de balanceamento de cada no entre -1 e 1
 *   4) a quantidade de nos bate com 'tamanho'
 * Retorna true se tudo estiver certo. Se 'motivo' nao for NULL, recebe
 * uma descricao da primeira invariante violada (ou "ok").
 */
bool avl_validar(const ArvoreAVL *arvore, const char **motivo);

/* Percursos: preenchem 'saida' (ja alocado com espaco >= tamanho) e retornam a
 * quantidade de registros escritos. */
long avl_pre_ordem(const ArvoreAVL *arvore, Registro *saida);
long avl_em_ordem(const ArvoreAVL *arvore, Registro *saida);
long avl_pos_ordem(const ArvoreAVL *arvore, Registro *saida);

#endif /* AVL_H */
