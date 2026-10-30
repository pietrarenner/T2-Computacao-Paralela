#!/bin/bash
#SBATCH -J t2-seq
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -t 01:00:00
#SBATCH -o results/raw/%x-%j.out
#SBATCH --nodelist=atlantica03
# T1: versao sequencial, mesmas flags da paralela, 1 no' exclusivo.
source scripts/params.sh
echo "# nodes=$SLURM_JOB_NODELIST"; lscpu; gcc --version | head -1
for r in $(seq $REPS); do ./MonteCarloSequential $TASKS $POINTS; done
