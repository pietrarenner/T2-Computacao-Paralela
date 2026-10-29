#!/bin/bash
#SBATCH -J t2-gran
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH --ntasks-per-node=32
#SBATCH -t 01:00:00
#SBATCH -o results/raw/%x-%j.out
# Granularidade e balanceamento, P = 32:
# (a) tempo de um lote x ida e volta da mensagem, no mesmo no' e entre nos
# (b) tempo total com G bem menor, escolhido e bem maior
# (c) linhas ESCRAVO de cada execucao: tarefas e tempo ocupado por escravo
source scripts/params.sh
echo "# nos=$SLURM_JOB_NODELIST"
for g in $G_MENOR $G $G_MAIOR; do
    srun --exclusive -N 1 -n 2 ./ida_e_volta $PONTOS $g intra
    srun --exclusive -N 2 -n 2 --ntasks-per-node=1 ./ida_e_volta $PONTOS $g inter
done
for g in $G_MENOR $G $G_MAIOR; do
    for r in $(seq $REPETICOES); do roda_mpi 32 $TAREFAS $g; done
done
