#!/bin/bash
#SBATCH -J t2-strong
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH --ntasks-per-node=32
#SBATCH -t 02:00:00
#SBATCH -o results/raw/%x-%j.out
# Escalabilidade forte: entrada de referencia fixa, P = 2 .. 64.
source scripts/params.sh
echo "# nos=$SLURM_JOB_NODELIST"; mpicc --version | head -1
for P in $PROCESSOS; do
    for r in $(seq $REPETICOES); do roda_mpi $P $TAREFAS $G; done
done
