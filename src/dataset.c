#include "dataset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_LINHA 1024
#define NUM_CAMPOS 10

static int arquivo_existe(const char *caminho) {
    FILE *f = fopen(caminho, "r");
    if (f == NULL) return 0;
    fclose(f);
    return 1;
}

const char *dataset_localizar(void) {
    const char *env = getenv("POKETREE_DATASET");
    if (env != NULL && arquivo_existe(env)) return env;

    static const char *candidatos[] = {
        "dataset/pokemon_dataset.csv",
        "../dataset/pokemon_dataset.csv",
    };
    for (int i = 0; i < 2; i++) {
        if (arquivo_existe(candidatos[i])) return candidatos[i];
    }
    return NULL;
}

/* Divide a linha em ate NUM_CAMPOS campos (troca ',' por '\0'). Retorna quantos achou. */
static int dividir_campos(char *linha, char *campos[NUM_CAMPOS]) {
    int n = 0;
    char *p = linha;
    campos[n++] = p;
    while (*p != '\0' && n < NUM_CAMPOS) {
        if (*p == ',') {
            *p = '\0';
            campos[n++] = p + 1;
        }
        p++;
    }
    return n;
}

static void tirar_quebra_de_linha(char *s) {
    size_t t = strlen(s);
    while (t > 0 && (s[t - 1] == '\n' || s[t - 1] == '\r')) s[--t] = '\0';
}

long dataset_carregar(const char *caminho, long maximo, Registro **saida) {
    *saida = NULL;
    FILE *f = fopen(caminho, "r");
    if (f == NULL) return -1;

    long capacidade = (maximo > 0) ? maximo : 1024;
    long quantidade = 0;
    Registro *vetor = (Registro *) malloc(sizeof(Registro) * (size_t) capacidade);
    if (vetor == NULL) { fclose(f); return -1; }

    char linha[TAM_LINHA];
    if (fgets(linha, sizeof(linha), f) == NULL) {      /* descarta o cabecalho */
        fclose(f);
        *saida = vetor;
        return 0;
    }

    while ((maximo <= 0 || quantidade < maximo) && fgets(linha, sizeof(linha), f) != NULL) {
        tirar_quebra_de_linha(linha);
        if (linha[0] == '\0') continue;

        char *c[NUM_CAMPOS];
        if (dividir_campos(linha, c) != NUM_CAMPOS) continue;   /* linha malformada: ignora */

        if (quantidade == capacidade) {
            capacidade *= 2;
            Registro *novo = (Registro *) realloc(vetor, sizeof(Registro) * (size_t) capacidade);
            if (novo == NULL) { free(vetor); fclose(f); return -1; }
            vetor = novo;
        }

        Registro *r = &vetor[quantidade++];
        memset(r, 0, sizeof(*r));
        r->id = atol(c[0]);
        snprintf(r->nome,   TAM_NOME,   "%s", c[1]);
        snprintf(r->genero, TAM_GENERO, "%s", c[2]);
        r->shiny = (c[3][0] == '1');
        for (int k = 0; k < NUM_ATAQUES; k++) {
            snprintf(r->ataques[k], TAM_ATAQUE, "%s", c[4 + k]);
        }
        r->ivs = atoi(c[8]);
        r->total_stats = atol(c[9]);
    }

    fclose(f);
    *saida = vetor;
    return quantidade;
}
