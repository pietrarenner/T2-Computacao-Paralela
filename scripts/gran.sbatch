#!/bin/bash
#SBATCH -J t2-gran
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH -t 01:00:00
#SBATCH -o results/raw/%x-%j.out
#SBATCH --nodelist=atlantica[03-04]
# Granularidade e balanceamento, P = 32:
# Tudo sai das proprias execucoes do MPIMonteCarlo com 32 processos:
# (a) tempo de uma tarefa (busy/tasks) e ida e volta medida por cada escravo
#     (rtt), separando escravos no no' do mestre e no outro no'
# (b) tempo total para cada grain da varredura GRAINS (bem menor .. bem maior);
#     o GRAIN usado na forte e na fraca e' escolhido a partir desta tabela
# (c) linhas SLAVE de cada execucao: tarefas e tempo ocupado por escravo
source scripts/params.sh
echo "# nodes=$SLURM_JOB_NODELIST cpus_per_node=$SLURM_CPUS_ON_NODE"
for g in $GRAINS; do
    for r in $(seq $REPS); do run_mpi 32 $TASKS $g; done
done
