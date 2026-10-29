#!/bin/bash
# Confere localmente (sem Slurm) que pi_mestre_escravo da' os mesmos acertos e
# checksum que pi_sequencial, com P e numero de tarefas que nao sao multiplos,
# menos tarefas que escravos e G maior que o saco.
# Uso (na raiz do repositorio, depois do make): ./scripts/teste_local.sh
MPIRUN="mpirun --oversubscribe"
[ "$(id -u)" = 0 ] && MPIRUN="$MPIRUN --allow-run-as-root"
PONTOS=20000
falhou=0
for caso in "2 1 1" "3 997 1" "5 997 7" "7 1000 13" "4 2 1" "6 3 100" "8 5000 64" "3 0 1"; do
    set -- $caso; P=$1; T=$2; G=$3
    esperado=$(./pi_sequencial "$T" "$PONTOS" | grep -o 'acertos=[0-9]* checksum=[0-9]*')
    obtido=$($MPIRUN -np "$P" ./pi_mestre_escravo "$T" "$PONTOS" "$G" | grep '^RESULTADO' | grep -o 'acertos=[0-9]* checksum=[0-9]*')
    if [ "$esperado" = "$obtido" ]; then r=OK; else r=FALHOU; falhou=1; fi
    echo "$r  P=$P tarefas=$T G=$G  seq[$esperado]  mpi[$obtido]"
done
exit $falhou
