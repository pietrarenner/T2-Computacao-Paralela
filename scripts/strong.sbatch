#!/bin/bash
#SBATCH -J t2-strong
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH -t 02:00:00
#SBATCH -o results/raw/%x-%j.out
# Escalabilidade forte: entrada de referencia fixa, P = 2 .. 64.
source scripts/params.sh
echo "# nodes=$SLURM_JOB_NODELIST"; mpicc --version | head -1
for P in $PROCS; do
    for r in $(seq $REPS); do run_mpi $P $TASKS $GRAIN; done
done
