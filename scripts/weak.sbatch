#!/bin/bash
#SBATCH -J t2-weak
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH -t 08:00:00
#SBATCH -o results/raw/%x-%j.out
#SBATCH --nodelist=atlantica[03-04]
# Escalabilidade fraca: tarefas = TASKS * P, mesmo custo por tarefa (POINTS).
source scripts/params.sh
echo "# nodes=$SLURM_JOB_NODELIST cpus_per_node=$SLURM_CPUS_ON_NODE"
for P in $PROCS; do
    for r in $(seq $REPS); do run_mpi $P $((TASKS * P)) $GRAIN; done
done
