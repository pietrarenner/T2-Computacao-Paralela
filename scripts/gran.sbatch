#!/bin/bash
#SBATCH -J t2-gran
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH -t 01:00:00
#SBATCH -o results/raw/%x-%j.out
# Granularidade e balanceamento, P = 32:
# (a) tempo de um lote x ida e volta da mensagem, no mesmo no' e entre nos
# (b) tempo total com grain bem menor, escolhido e bem maior
# (c) linhas SLAVE de cada execucao: tarefas e tempo ocupado por escravo
source scripts/params.sh
echo "# nodes=$SLURM_JOB_NODELIST"
for g in $GRAIN_SMALL $GRAIN $GRAIN_LARGE; do
    srun --exclusive -N 1 -n 2 ./pingpong $POINTS $g intra
    srun --exclusive -N 2 -n 2 --ntasks-per-node=1 ./pingpong $POINTS $g inter
done
for g in $GRAIN_SMALL $GRAIN $GRAIN_LARGE; do
    for r in $(seq $REPS); do run_mpi 32 $TASKS $g; done
done
