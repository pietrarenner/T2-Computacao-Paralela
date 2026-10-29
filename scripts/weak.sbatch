#!/bin/bash
#SBATCH -J t2-weak
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH --ntasks-per-node=32
#SBATCH -t 04:00:00
#SBATCH -o results/raw/%x-%j.out
# Escalabilidade fraca: tarefas = TAREFAS * P, mesmo custo por tarefa (PONTOS).
source scripts/params.sh
echo "# nos=$SLURM_JOB_NODELIST"
for P in $PROCESSOS; do
    for r in $(seq $REPETICOES); do roda_mpi $P $((TAREFAS * P)) $G; done
done
