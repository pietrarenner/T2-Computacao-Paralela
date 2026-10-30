/*
 * montecarlo.h — A tarefa do saco de trabalho, usada pelas versoes
 * sequencial e paralela.
 *
 * Problema: estimar pi por Monte Carlo. Base: repositorio
 * https://github.com/JasonPollman/MPI-Monte-Carlo.
 *
 * Uma TAREFA é sortear n pontos (x, y) no quadrado [0,1] x [0,1] e contar
 * quantos caem dentro do quarto de circulo x^2 + y^2 <= 1.
 *
 * Diferenca para o original: a semente vinha do relogio (srand(tempo)),
 * entao cada execucao dava um resultado diferente. Aqui a semente da tarefa
 * é o proprio indice da tarefa, e rand_r guarda o estado na variavel 'seed'
 * em vez de num estado global. Assim o resultado de cada tarefa nao depende
 * de qual escravo a processou nem da ordem: a saida é igual a da versao
 * sequencial.
 */

#ifndef MONTECARLO_H
#define MONTECARLO_H

#include <stdlib.h>

/* Processa a tarefa de indice 'task' com n pontos e devolve quantos
 * pontos cairam dentro do circulo. */
static inline int compute_task(int task, int n) {
    unsigned int seed = (unsigned int) task + 1;
    int count = 0;
    for (int i = 0; i < n; i++) {
        double x = (double) rand_r(&seed) / RAND_MAX;
        double y = (double) rand_r(&seed) / RAND_MAX;
        if (x * x + y * y <= 1.0)
            count++;
    }
    return count;
}

/*
 * Grandezas verificaveis do resultado, comparadas com a versao sequencial:
 *   total    = soma das contagens (da' a estimativa de pi);
 *   checksum = soma de (i+1) * counts[i]. So' bate com a sequencial se cada
 *              resultado voltou para a sua posicao de origem no saco.
 */
static inline void checksums(int num_tasks, const int *counts, long long *total, long long *checksum) {
    *total = 0;
    *checksum = 0;
    for (int i = 0; i < num_tasks; i++) {
        *total += counts[i];
        *checksum += (long long) (i + 1) * counts[i];
    }
}

#endif
