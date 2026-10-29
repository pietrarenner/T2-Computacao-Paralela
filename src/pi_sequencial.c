/*
 * pi_sequencial.c — O saco de tarefas processado por um processo so'.
 * E' a versao de referencia (T1) do speed-up.
 *
 * Compilar:  gcc -O2 -Wall -o pi_sequencial src/pi_sequencial.c
 * Executar:  ./pi_sequencial <num_tarefas> <pontos_por_tarefa>
 *            ./pi_sequencial 150000 100000
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "monte_carlo.h"

static double agora(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <num_tarefas> <pontos_por_tarefa>\n", argv[0]);
        return 1;
    }
    int ntarefas = atoi(argv[1]);
    int pontos   = atoi(argv[2]);

    int *acertos = malloc(ntarefas * sizeof *acertos);   /* fora da medicao */
    if (acertos == NULL) {
        fprintf(stderr, "Memoria insuficiente para %d tarefas\n", ntarefas);
        return 1;
    }

    double t1 = agora();
    for (int i = 0; i < ntarefas; i++)
        acertos[i] = tarefa_monte_carlo(i, pontos);
    double t2 = agora();

    long long total, checksum;
    confere(ntarefas, acertos, &total, &checksum);
    double pi = 4.0 * total / ((double) ntarefas * pontos);

    /* linha lida por scripts/analyze.py */
    printf("RESULTADO versao=seq P=1 tarefas=%d pontos=%d G=0 tempo=%.6f "
           "acertos=%lld checksum=%lld pi=%.10f\n",
           ntarefas, pontos, t2 - t1, total, checksum, pi);

    free(acertos);
    return 0;
}
