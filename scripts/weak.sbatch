#!/bin/bash
#SBATCH -J t2-weak
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH --ntasks-per-node=32
#SBATCH -t 04:00:00
#SBATCH -o results/raw/%x-%j.out
# Escalabilidade fraca: tarefas = TASKS * P, mesmo custo por tarefa (POINTS).
source scripts/params.sh
echo "# nodes=$SLURM_JOB_NODELIST"
for P in $PROCS; do
    for r in $(seq $REPS); do run_mpi $P $((TASKS * P)) $GRAIN; done
done
