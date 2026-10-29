/*
 * ida_e_volta.c — Compara o custo de um lote com o de uma ida e volta de
 * mensagem do mesmo tamanho (item "Granularidade" do enunciado).
 *
 * Com 2 processos: o rank 0 manda {primeira, qtd} (como TAG_TAREFA) e o
 * rank 1 responde com 2 + G inteiros (como TAG_RESULTADO), muitas vezes.
 * Depois o rank 0 mede o tempo de processar uma tarefa.
 *
 * Compilar:  mpicc -O2 -Wall -o ida_e_volta src/ida_e_volta.c
 * Mesmo no':  srun --exclusive -N 1 -n 2 ./ida_e_volta 100000 10 intra
 * Dois nos:   srun --exclusive -N 2 -n 2 ./ida_e_volta 100000 10 inter
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "monte_carlo.h"

#define TAG_IDA   1
#define TAG_VOLTA 2
#define REPETICOES 2000

int main(int argc, char *argv[]) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2 || argc < 4) {
        if (rank == 0)
            fprintf(stderr, "Uso: mpirun -np 2 %s <pontos_por_tarefa> <G> <rotulo>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }
    int pontos = atoi(argv[1]);
    int G      = atoi(argv[2]);

    int lote[2] = {0, G};
    int *resposta = calloc(2 + G, sizeof *resposta);

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();
    for (int i = 0; i < REPETICOES; i++) {
        if (rank == 0) {
            MPI_Send(lote, 2, MPI_INT, 1, TAG_IDA, MPI_COMM_WORLD);
            MPI_Recv(resposta, 2 + G, MPI_INT, 1, TAG_VOLTA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            MPI_Recv(lote, 2, MPI_INT, 0, TAG_IDA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(resposta, 2 + G, MPI_INT, 0, TAG_VOLTA, MPI_COMM_WORLD);
        }
    }
    double ida_volta = (MPI_Wtime() - t0) / REPETICOES;

    if (rank == 0) {
        int n = 200;                        /* media sobre 200 tarefas */
        long long soma = 0;
        double t1 = MPI_Wtime();
        for (int i = 0; i < n; i++)
            soma += tarefa_monte_carlo(i, pontos);
        double t_tarefa = (MPI_Wtime() - t1) / n;

        printf("IDAVOLTA caso=%s pontos=%d G=%d bytes=%d t_tarefa=%.9f t_lote=%.9f "
               "ida_volta=%.9f razao=%.1f (soma=%lld)\n",
               argv[3], pontos, G, (int) ((2 + G) * sizeof(int)), t_tarefa,
               t_tarefa * G, ida_volta, t_tarefa * G / ida_volta, soma);
    }

    free(resposta);
    MPI_Finalize();
    return 0;
}
