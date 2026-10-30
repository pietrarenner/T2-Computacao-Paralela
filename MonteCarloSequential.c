/*
 * MonteCarloSequential.c — O saco de tarefas processado por um processo so'.
 * E' a versao de referencia (T1) do speed-up.
 *
 * Compilar:  gcc -O2 -Wall -o MonteCarloSequential MonteCarloSequential.c
 * Executar:  ./MonteCarloSequential <num_tasks> <points_per_task>
 *            ./MonteCarloSequential 180000 20000
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "montecarlo.h"

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <num_tasks> <points_per_task>\n", argv[0]);
        return 1;
    }
    int num_tasks = atoi(argv[1]);
    int n         = atoi(argv[2]);   /* pontos por tarefa */

    int *counts = malloc(num_tasks * sizeof *counts);   /* fora da medicao */
    if (counts == NULL) {
        fprintf(stderr, "Memoria insuficiente para %d tarefas\n", num_tasks);
        return 1;
    }

    double tic = now();
    for (int i = 0; i < num_tasks; i++)
        counts[i] = compute_task(i, n);
    double toc = now();

    long long total, checksum;
    checksums(num_tasks, counts, &total, &checksum);
    double pi = 4.0 * total / ((double) num_tasks * n);

    /* linha lida por scripts/analyze.py */
    printf("RESULT version=seq P=1 tasks=%d points=%d grain=0 time=%.6f "
           "hits=%lld checksum=%lld pi=%.10f\n",
           num_tasks, n, toc - tic, total, checksum, pi);

    free(counts);
    return 0;
}
