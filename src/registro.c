#include "registro.h"
#include <string.h>

int comparar_registro(const Registro *a, const Registro *b) {
    /* 1) total_stats */
    if (a->total_stats != b->total_stats) {
        return (a->total_stats < b->total_stats) ? -1 : 1;
    }
    /* 2) nome (desempate lexicográfico) */
    int cmp_nome = strcmp(a->nome, b->nome);
    if (cmp_nome != 0) {
        return (cmp_nome < 0) ? -1 : 1;
    }
    /* 3) id (desempate final, sempre único) */
    if (a->id != b->id) {
        return (a->id < b->id) ? -1 : 1;
    }
    return 0;
}
