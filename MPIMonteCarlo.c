/*
 * MPIMonteCarlo.c — Monte Carlo para pi no modelo mestre/escravo.
 *
 * O mestre (rank 0) guarda o saco de tarefas 0, 1, ..., num_tasks-1. Os
 * escravos pedem trabalho, recebem um lote de 'grain' tarefas seguidas,
 * processam e devolvem os resultados; cada devolucao vale como pedido do
 * proximo lote.
 *
 * Mensagens (todas de inteiros, exceto as estatisticas do fim):
 *   escravo -> mestre  TAG_REQUEST       {0, 0}                    primeiro pedido
 *   escravo -> mestre  TAG_RESULT        {first, size, c_0 .. c_size-1}
 *   mestre -> escravo  TAG_WORK          {first, size}             lote de tarefas
 *   mestre -> escravo  TAG_KILL          {0, 0}                    saco vazio
 *   escravo -> mestre  TAG_COMPUTATIONS  {ocupado, comunicacao, idas e voltas}
 *                                         (double[3]), so' no fim
 *   escravo -> mestre  TAG_NAME          nome do no' (char), so' no fim
 *
 * Os valores 0, 200, 300 e 400 sao os mesmos tags do original.
 *
 * Compilar:  mpicc -O2 -Wall -o MPIMonteCarlo MPIMonteCarlo.c
 * Local:     mpirun -np 4 ./MPIMonteCarlo 1000 20000 10
 * No LAD:    srun --exclusive -N 2 -n 32 ./MPIMonteCarlo 180000 20000 10
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "montecarlo.h"

#define TAG_KILL          0   /* mestre avisa que o saco acabou: escravo termina */
#define TAG_REQUEST     100   /* escravo pede o primeiro lote                     */
#define TAG_WORK        200   /* mestre manda um lote                             */
#define TAG_RESULT      300   /* escravo devolve um lote = pede o proximo         */
#define TAG_COMPUTATIONS 400  /* estatisticas do escravo, fora da regiao medida   */
#define TAG_NAME        500

int rank, num_processes;
int num_tasks, n, grain;      /* tarefas, pontos por tarefa, tarefas por mensagem */
char processor_name[MPI_MAX_PROCESSOR_NAME];
MPI_Status status;

void master_io() {
    int *counts = malloc(num_tasks * sizeof *counts);             /* o saco      */
    int *msg = malloc((2 + grain) * sizeof *msg);
    int *computations = calloc(num_processes, sizeof *computations); /* por escravo */
    double *stopped_at = calloc(num_processes, sizeof *stopped_at);
    int next = 0;                      /* primeira tarefa ainda nao distribuida  */
    int received = 0;                  /* tarefas cujo resultado ja' voltou      */
    int active = num_processes - 1;    /* escravos que ainda nao receberam KILL  */

    MPI_Barrier(MPI_COMM_WORLD);       /* largada junta */
    double tic = MPI_Wtime();
    double toc = tic;

    while (active > 0) {
        /* Atende quem ficar pronto primeiro; a tag diz se e' o primeiro
         * pedido ou um resultado. */
        MPI_Recv(msg, 2 + grain, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                 MPI_COMM_WORLD, &status);
        int slave = status.MPI_SOURCE;

        if (status.MPI_TAG == TAG_RESULT) {
            int first = msg[0], size = msg[1];
            for (int k = 0; k < size; k++)        /* volta para a posicao */
                counts[first + k] = msg[2 + k];   /* de origem no saco    */
            received += size;
            if (received == num_tasks)
                toc = MPI_Wtime();                /* chegou o ultimo resultado */
        }

        if (next < num_tasks) {
            /* O ultimo lote pode ter menos de 'grain' tarefas: num_tasks nao
             * precisa ser multiplo de grain nem do numero de escravos. */
            int work[2];
            work[0] = next;
            work[1] = (num_tasks - next < grain) ? num_tasks - next : grain;
            MPI_Send(work, 2, MPI_INT, slave, TAG_WORK, MPI_COMM_WORLD);
            next += work[1];
            computations[slave] += work[1];
        } else {
            /* Saco vazio: este escravo ja' devolveu tudo o que recebeu. */
            int kill[2] = {0, 0};
            MPI_Send(kill, 2, MPI_INT, slave, TAG_KILL, MPI_COMM_WORLD);
            stopped_at[slave] = MPI_Wtime() - tic;
            active--;
        }
    }
    double elapsed = toc - tic;

    /* ---------- daqui para baixo nada e' medido ---------- */
    long long total, checksum;
    checksums(num_tasks, counts, &total, &checksum);
    double pi = 4.0 * total / ((double) num_tasks * n);
    printf("RESULT version=mpi P=%d tasks=%d points=%d grain=%d time=%.6f "
           "hits=%lld checksum=%lld pi=%.10f\n",
           num_processes, num_tasks, n, grain, elapsed, total, checksum, pi);
    printf("MASTER host=%s\n", processor_name);

    /* Os escravos so' mandam TAG_COMPUTATIONS e TAG_NAME depois desta
     * barreira, para que essas mensagens nao sejam pegas pelo MPI_ANY_TAG do
     * laco acima enquanto o mestre ainda atende outros escravos. */
    MPI_Barrier(MPI_COMM_WORLD);
    for (int i = 1; i < num_processes; i++) {
        double stats[3];               /* busy, comm, idas e voltas */
        char slave_name[MPI_MAX_PROCESSOR_NAME];
        MPI_Recv(stats, 3, MPI_DOUBLE, i, TAG_COMPUTATIONS, MPI_COMM_WORLD, &status);
        MPI_Recv(slave_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, i, TAG_NAME,
                 MPI_COMM_WORLD, &status);
        printf("SLAVE rank=%d host=%s tasks=%d busy=%.6f stop_at=%.6f comm=%.6f rtt=%.9f\n",
               i, slave_name, computations[i], stats[0], stopped_at[i], stats[1],
               stats[1] / stats[2]);
    }

    free(counts);
    free(msg);
    free(computations);
    free(stopped_at);
}

void slave_io() {
    int *msg = malloc((2 + grain) * sizeof *msg);
    double busy = 0.0;                 /* tempo gasto calculando */
    double comm = 0.0;                 /* tempo entre enviar um pedido e receber a resposta */
    int round_trips = 0;               /* quantas idas e voltas foram medidas */
    int work[2] = {0, 0};

    MPI_Barrier(MPI_COMM_WORLD);

    /* A iniciativa e' do escravo: comeca pedindo trabalho. */
    double t_send = MPI_Wtime();
    MPI_Send(work, 2, MPI_INT, 0, TAG_REQUEST, MPI_COMM_WORLD);

    while (1) {
        MPI_Recv(work, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        /* Ida e volta real nesta configuracao: mensagens + espera na fila
         * do mestre (item "a" da granularidade). */
        comm += MPI_Wtime() - t_send;
        round_trips++;
        if (status.MPI_TAG == TAG_KILL)
            break;

        int first = work[0], size = work[1];
        double t = MPI_Wtime();
        for (int k = 0; k < size; k++)
            msg[2 + k] = compute_task(first + k, n);
        busy += MPI_Wtime() - t;

        /* Devolve o lote com o indice da primeira tarefa; isso tambem
         * pede o proximo lote. */
        msg[0] = first;
        msg[1] = size;
        t_send = MPI_Wtime();
        MPI_Send(msg, 2 + size, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);       /* ver o comentario em master_io */
    double stats[3] = {busy, comm, round_trips};
    MPI_Send(stats, 3, MPI_DOUBLE, 0, TAG_COMPUTATIONS, MPI_COMM_WORLD);
    MPI_Send(processor_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, TAG_NAME, MPI_COMM_WORLD);
    free(msg);
}

int main(int argc, char *argv[]) {
    int name_len;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Get_processor_name(processor_name, &name_len);

    if (num_processes < 2 || argc < 4) {
        if (rank == 0)
            fprintf(stderr, "Uso: mpirun -np <P >= 2> %s <num_tasks> <points_per_task> <grain>\n"
                            "     grain = tarefas por mensagem (granularidade)\n", argv[0]);
        MPI_Finalize();
        return 1;
    }
    /* Todos os processos leem os parametros da linha de comando (o original
     * lia com scanf no mestre e fazia MPI_Bcast). */
    num_tasks = atoi(argv[1]);
    n         = atoi(argv[2]);
    grain     = atoi(argv[3]);
    if (grain < 1) grain = 1;

    if (rank == 0) master_io(); else slave_io();

    MPI_Finalize();
    return 0;
}
