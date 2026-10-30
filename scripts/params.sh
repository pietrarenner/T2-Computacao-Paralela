# Parametros comuns a todos os jobs (incluido pelos scripts .sbatch).
# AJUSTE TASKS depois de rodar seq.sbatch para que T1 fique entre 30 e 60 s.
# Medido localmente: ~12 ns por ponto => 180000 x 20000 pontos ~ 43 s.
POINTS=20000       # pontos por tarefa = custo de uma tarefa (~0,25 ms)
TASKS=180000       # tarefas da entrada de referencia (escalabilidade forte)
GRAIN=10           # granularidade escolhida (tarefas por mensagem)
GRAIN_SMALL=1      # bem menor, para o experimento de granularidade
GRAIN_LARGE=2000   # bem maior: 180000/2000 = 90 lotes para 31 escravos
REPS=10
PROCS="2 4 8 16 32 64"

# Roda a versao MPI com P processos divididos igualmente entre os 2 nos:
# P/2 por no' (com P=2, o mestre fica num no' e o escravo no outro).
# Ate' P=32 sao 16 por no' = 1 por nucleo fisico; P=64 usa o Hyper-Threading.
run_mpi() {   # run_mpi <P> <tasks> <grain>
    srun --exclusive -N 2 -n "$1" --ntasks-per-node=$(($1 / 2)) \
         ./MPIMonteCarlo "$2" "$POINTS" "$3"
}
