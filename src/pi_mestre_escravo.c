/*
 * pi_mestre_escravo.c — Monte Carlo para pi no modelo mestre/escravo.
 *
 * O mestre (rank 0) guarda o saco de tarefas 0, 1, ..., ntarefas-1. Os
 * escravos pedem trabalho, recebem um lote de G tarefas seguidas, processam
 * e devolvem os resultados; cada devolucao vale como pedido do proximo lote.
 *
 * Mensagens (todas de inteiros):
 *   escravo -> mestre  TAG_PEDIDO     {0, 0}                      primeiro pedido
 *   escravo -> mestre  TAG_RESULTADO  {primeira, qtd, a_0 .. a_qtd-1}
 *   mestre -> escravo  TAG_TAREFA     {primeira, qtd}             lote de tarefas
 *   mestre -> escravo  TAG_FIM        {0, 0}                      saco vazio
 *   escravo -> mestre  TAG_TEMPO      tempo ocupado (double), so' no fim
 *   escravo -> mestre  TAG_NOME       nome do no' (char), so' no fim
 *
 * Compilar:  mpicc -O2 -Wall -o pi_mestre_escravo src/pi_mestre_escravo.c
 * Local:     mpirun -np 4 ./pi_mestre_escravo 1000 100000 10
 * No LAD:    srun --exclusive -N 2 -n 32 ./pi_mestre_escravo 150000 100000 10
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "monte_carlo.h"

#define TAG_PEDIDO    1   /* escravo pede o primeiro lote                     */
#define TAG_RESULTADO 2   /* escravo devolve um lote = pede o proximo         */
#define TAG_TAREFA    3   /* mestre manda um lote                             */
#define TAG_FIM       4   /* mestre avisa que o saco acabou: escravo termina  */
#define TAG_TEMPO     5   /* estatisticas do escravo, fora da regiao medida   */
#define TAG_NOME      6

int main(int argc, char *argv[]) {
    int rank, size, tam_nome;
    char nome[MPI_MAX_PROCESSOR_NAME];
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(nome, &tam_nome);

    if (size < 2 || argc < 4) {
        if (rank == 0)
            fprintf(stderr, "Uso: mpirun -np <P >= 2> %s <num_tarefas> <pontos_por_tarefa> <G>\n"
                            "     G = tarefas por mensagem (granularidade)\n", argv[0]);
        MPI_Finalize();
        return 1;
    }
    /* Todos os processos leem os parametros da linha de comando. */
    int ntarefas = atoi(argv[1]);
    int pontos   = atoi(argv[2]);
    int G        = atoi(argv[3]);
    if (G < 1) G = 1;

    /* Buffer de resultado: 2 inteiros de cabecalho + ate' G resultados. */
    int *msg = malloc((2 + G) * sizeof *msg);

    if (rank == 0) {
        /* ======================= papel do mestre ======================= */
        int *acertos = malloc(ntarefas * sizeof *acertos);   /* o saco      */
        int *tarefas_do = calloc(size, sizeof *tarefas_do);  /* por escravo */
        double *fim_em = calloc(size, sizeof *fim_em);       /* quando parou */
        int proxima = 0;          /* primeira tarefa ainda nao distribuida  */
        int recebidas = 0;        /* tarefas cujo resultado ja' voltou      */
        int ativos = size - 1;    /* escravos que ainda nao receberam FIM   */

        MPI_Barrier(MPI_COMM_WORLD);          /* largada junta */
        double t0 = MPI_Wtime();
        double t_ultimo = t0;

        while (ativos > 0) {
            /* Atende quem ficar pronto primeiro; a tag diz se e' o primeiro
             * pedido ou um resultado. */
            MPI_Recv(msg, 2 + G, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                     MPI_COMM_WORLD, &status);
            int escravo = status.MPI_SOURCE;

            if (status.MPI_TAG == TAG_RESULTADO) {
                int primeira = msg[0], qtd = msg[1];
                for (int k = 0; k < qtd; k++)          /* volta para a posicao */
                    acertos[primeira + k] = msg[2 + k];  /* de origem no saco    */
                recebidas += qtd;
                if (recebidas == ntarefas)
                    t_ultimo = MPI_Wtime();            /* chegou o ultimo resultado */
            }

            if (proxima < ntarefas) {
                /* O ultimo lote pode ter menos de G tarefas: ntarefas nao
                 * precisa ser multiplo de G nem do numero de escravos. */
                int lote[2];
                lote[0] = proxima;
                lote[1] = (ntarefas - proxima < G) ? ntarefas - proxima : G;
                MPI_Send(lote, 2, MPI_INT, escravo, TAG_TAREFA, MPI_COMM_WORLD);
                proxima += lote[1];
                tarefas_do[escravo] += lote[1];
            } else {
                /* Saco vazio: este escravo ja' devolveu tudo o que recebeu. */
                int vazio[2] = {0, 0};
                MPI_Send(vazio, 2, MPI_INT, escravo, TAG_FIM, MPI_COMM_WORLD);
                fim_em[escravo] = MPI_Wtime() - t0;
                ativos--;
            }
        }
        double tempo = t_ultimo - t0;

        /* ---------- daqui para baixo nada e' medido ---------- */
        long long total, checksum;
        confere(ntarefas, acertos, &total, &checksum);
        double pi = 4.0 * total / ((double) ntarefas * pontos);
        printf("RESULTADO versao=mpi P=%d tarefas=%d pontos=%d G=%d tempo=%.6f "
               "acertos=%lld checksum=%lld pi=%.10f\n",
               size, ntarefas, pontos, G, tempo, total, checksum, pi);
        printf("MESTRE no=%s\n", nome);

        /* Os escravos so' mandam TAG_TEMPO e TAG_NOME depois desta barreira,
         * para que essas mensagens nao sejam pegas pelo MPI_ANY_TAG do laco
         * acima enquanto o mestre ainda atende outros escravos. */
        MPI_Barrier(MPI_COMM_WORLD);
        for (int e = 1; e < size; e++) {
            double ocupado;
            char nome_escravo[MPI_MAX_PROCESSOR_NAME];
            MPI_Recv(&ocupado, 1, MPI_DOUBLE, e, TAG_TEMPO, MPI_COMM_WORLD, &status);
            MPI_Recv(nome_escravo, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, e, TAG_NOME,
                     MPI_COMM_WORLD, &status);
            printf("ESCRAVO rank=%d no=%s tarefas=%d ocupado=%.6f fim_em=%.6f\n",
                   e, nome_escravo, tarefas_do[e], ocupado, fim_em[e]);
        }
        free(acertos);
        free(tarefas_do);
        free(fim_em);
    } else {
        /* ======================= papel do escravo ====================== */
        double ocupado = 0.0;     /* tempo gasto calculando */
        int lote[2] = {0, 0};

        MPI_Barrier(MPI_COMM_WORLD);

        /* A iniciativa e' do escravo: comeca pedindo trabalho. */
        MPI_Send(lote, 2, MPI_INT, 0, TAG_PEDIDO, MPI_COMM_WORLD);

        while (1) {
            MPI_Recv(lote, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == TAG_FIM)
                break;

            int primeira = lote[0], qtd = lote[1];
            double t = MPI_Wtime();
            for (int k = 0; k < qtd; k++)
                msg[2 + k] = tarefa_monte_carlo(primeira + k, pontos);
            ocupado += MPI_Wtime() - t;

            /* Devolve o lote com o indice da primeira tarefa; isso tambem
             * pede o proximo lote. */
            msg[0] = primeira;
            msg[1] = qtd;
            MPI_Send(msg, 2 + qtd, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
        }

        MPI_Barrier(MPI_COMM_WORLD);    /* ver o comentario no mestre */
        MPI_Send(&ocupado, 1, MPI_DOUBLE, 0, TAG_TEMPO, MPI_COMM_WORLD);
        MPI_Send(nome, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, TAG_NOME, MPI_COMM_WORLD);
    }

    free(msg);
    MPI_Finalize();
    return 0;
}
