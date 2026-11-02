/*
 * Leitura do dataset PokeTree (dataset/pokemon_dataset.csv).
 *
 * Formato (uma linha de cabecalho + 1 registro por linha, separado por virgula):
 *   id,nome,genero,shiny,ataque1,ataque2,ataque3,ataque4,ivs,total_stats
 *
 * O arquivo e gerado por dataset/gerar_dataset.py a partir dos dados reais
 * da PokeAPI (especies, variantes, stats base e golpes).
 */

#ifndef DATASET_H
#define DATASET_H

#include "registro.h"

/*
 * Procura o CSV nos lugares usuais (nesta ordem):
 *   1) variavel de ambiente POKETREE_DATASET
 *   2) dataset/pokemon_dataset.csv           (rodando da raiz do projeto)
 *   3) ../dataset/pokemon_dataset.csv        (rodando de dentro de /testes)
 * Retorna o caminho encontrado, ou NULL se nao achou.
 */
const char *dataset_localizar(void);

/*
 * Le o CSV para um vetor alocado com malloc (quem chama libera com free).
 * 'maximo' limita quantos registros ler (<= 0 le todos).
 * Retorna a quantidade lida, ou -1 se nao conseguiu abrir o arquivo.
 */
long dataset_carregar(const char *caminho, long maximo, Registro **saida);

#endif /* DATASET_H */
