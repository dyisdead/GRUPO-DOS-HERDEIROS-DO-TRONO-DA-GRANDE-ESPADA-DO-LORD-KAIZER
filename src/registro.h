/*
 * Modelo de dado do dataset PokeTree e o comparador da chave composta.
 *
 * Cada registro e um Pokemon individual (especie real da PokeAPI + atributos
 * sorteados: genero, shiny, IV e 4 golpes).
 *
 * Chave composta:
 *     chave = (total_stats, nome, id)
 *
 * Prioridade de comparacao:
 *     1) total_stats  -> criterio principal
 *     2) nome         -> desempate
 *     3) id           -> desempate final (unico e sequencial, nunca empata)
 *
 * Por que composta? total_stats so assume ~1.000 valores distintos em 500 mil
 * registros, entao sozinho geraria chaves repetidas.
 */

#ifndef REGISTRO_H
#define REGISTRO_H

#include <stdbool.h>

#define TAM_NOME    64
#define TAM_GENERO  16
#define TAM_ATAQUE  32
#define NUM_ATAQUES 4

typedef struct {
    long id;
    char nome[TAM_NOME];
    char genero[TAM_GENERO];    /* "macho", "femea" ou "sem genero" */
    bool shiny;
    char ataques[NUM_ATAQUES][TAM_ATAQUE];
    int ivs;                    /* 1 a 100 */
    long total_stats;
} Registro;

/*
 * Comparador central da chave composta (total_stats, nome, id).
 * Retorna <0 se a < b, >0 se a > b, 0 se as chaves sao iguais
 * (o que so acontece se for o mesmo registro, ja que id e unico).
 */
int comparar_registro(const Registro *a, const Registro *b);

#endif /* REGISTRO_H */
