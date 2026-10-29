/*
 * monte_carlo.h — A tarefa do saco de trabalho, usada pelas versoes
 * sequencial e paralela (o mesmo codigo garante o mesmo resultado).
 *
 * Problema: estimar pi por Monte Carlo. Ideia original do repositorio
 * https://github.com/JasonPollman/MPI-Monte-Carlo (ver original/).
 *
 * Uma TAREFA e' sortear 'pontos' pontos (x, y) no quadrado [0,1] x [0,1]
 * e contar quantos caem dentro do quarto de circulo x^2 + y^2 <= 1.
 *
 * Diferenca para o original: la' a semente vinha do relogio (srand(tempo)),
 * entao cada execucao dava um resultado diferente. Aqui a semente da tarefa
 * i e' o proprio i, e rand_r guarda o estado na variavel 'semente' em vez de
 * num estado global. Assim o resultado de cada tarefa nao depende de qual
 * escravo a processou nem da ordem: a saida e' igual a da versao sequencial.
 */

#ifndef MONTE_CARLO_H
#define MONTE_CARLO_H

#include <stdlib.h>

/* Processa a tarefa de indice 'tarefa' e devolve quantos pontos acertaram. */
static inline int tarefa_monte_carlo(int tarefa, int pontos) {
    unsigned int semente = (unsigned int) tarefa + 1;
    int acertos = 0;
    for (int p = 0; p < pontos; p++) {
        double x = (double) rand_r(&semente) / RAND_MAX;
        double y = (double) rand_r(&semente) / RAND_MAX;
        if (x * x + y * y <= 1.0)
            acertos++;
    }
    return acertos;
}

/*
 * Grandezas verificaveis do resultado, comparadas com a versao sequencial:
 *   total    = soma dos acertos (da' a estimativa de pi);
 *   checksum = soma de (i+1) * acertos[i]. So' bate com a sequencial se cada
 *              resultado voltou para a sua posicao de origem no saco.
 */
static inline void confere(int ntarefas, const int *acertos, long long *total, long long *checksum) {
    *total = 0;
    *checksum = 0;
    for (int i = 0; i < ntarefas; i++) {
        *total += acertos[i];
        *checksum += (long long) (i + 1) * acertos[i];
    }
}

#endif
