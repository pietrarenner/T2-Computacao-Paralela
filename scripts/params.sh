# Parametros comuns a todos os jobs (incluido pelos scripts .sbatch).
# AJUSTE TASKS depois de rodar seq.sbatch para que T1 fique entre 30 e 60 s.
# Medido no Xeon E5520 do atlantica: 180000 tarefas -> T1 = 64,3 s.
POINTS=20000       # pontos por tarefa = custo de uma tarefa (~0,36 ms no atlantica)
TASKS=180000       # tarefas da entrada de referencia (escalabilidade forte)
GRAIN=10           # granularidade escolhida (tarefas por mensagem), definida
                   # pela varredura do gran.sbatch
GRAINS="1 2 5 10 20 50 100 500 2000 5000"   # varredura do gran.sbatch: de bem menor
                   # a bem maior (180000/5000 = 36 lotes para 31 escravos)
REPS=10
PROCS="2 4 8 16 32 64"

# Roda a versao MPI com P processos divididos igualmente entre os 2 nos:
# P/2 por no' (com P=2, o mestre fica num no' e o escravo no outro).
# Se P/2 passar do numero de CPUs do no' (SLURM_CPUS_ON_NODE), usa
# --overcommit para permitir mais processos que CPUs.
run_mpi() {   # run_mpi <P> <tasks> <grain>
    local per_node=$(($1 / 2)) extra=""
    [ "$per_node" -gt "${SLURM_CPUS_ON_NODE:-32}" ] && extra="--overcommit"
    srun -N 2 -n "$1" --ntasks-per-node=$per_node $extra \
         ./MPIMonteCarlo "$2" "$POINTS" "$3"
}
