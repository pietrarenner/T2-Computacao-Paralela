/*
 * pingpong.c — Compara o custo de um lote com o de uma ida e volta de
 * mensagem do mesmo tamanho (item "Granularidade" do enunciado).
 *
 * Com 2 processos: o rank 0 manda {first, size} (como TAG_WORK) e o rank 1
 * responde com 2 + grain inteiros (como TAG_RESULT), muitas vezes. Depois o
 * rank 0 mede o tempo de processar uma tarefa.
 *
 * Compilar:  mpicc -O2 -Wall -o pingpong pingpong.c
 * Mesmo no':  srun --exclusive -N 1 -n 2 ./pingpong 20000 10 intra
 * Dois nos:   srun --exclusive -N 2 -n 2 ./pingpong 20000 10 inter
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "montecarlo.h"

#define TAG_PING 1
#define TAG_PONG 2
#define REPS 2000

int main(int argc, char *argv[]) {
    int rank, num_processes;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    if (num_processes != 2 || argc < 4) {
        if (rank == 0)
            fprintf(stderr, "Uso: mpirun -np 2 %s <points_per_task> <grain> <label>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }
    int n     = atoi(argv[1]);
    int grain = atoi(argv[2]);

    int work[2] = {0, grain};
    int *reply = calloc(2 + grain, sizeof *reply);

    MPI_Barrier(MPI_COMM_WORLD);
    double tic = MPI_Wtime();
    for (int i = 0; i < REPS; i++) {
        if (rank == 0) {
            MPI_Send(work, 2, MPI_INT, 1, TAG_PING, MPI_COMM_WORLD);
            MPI_Recv(reply, 2 + grain, MPI_INT, 1, TAG_PONG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            MPI_Recv(work, 2, MPI_INT, 0, TAG_PING, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(reply, 2 + grain, MPI_INT, 0, TAG_PONG, MPI_COMM_WORLD);
        }
    }
    double rtt = (MPI_Wtime() - tic) / REPS;

    if (rank == 0) {
        int tasks = 200;                        /* media sobre 200 tarefas */
        long long sum = 0;
        double t = MPI_Wtime();
        for (int i = 0; i < tasks; i++)
            sum += compute_task(i, n);
        double task_time = (MPI_Wtime() - t) / tasks;

        printf("PINGPONG case=%s points=%d grain=%d bytes=%d task_time=%.9f batch_time=%.9f "
               "rtt=%.9f ratio=%.1f (sum=%lld)\n",
               argv[3], n, grain, (int) ((2 + grain) * sizeof(int)), task_time,
               task_time * grain, rtt, task_time * grain / rtt, sum);
    }

    free(reply);
    MPI_Finalize();
    return 0;
}
