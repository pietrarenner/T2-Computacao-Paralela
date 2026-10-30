#!/bin/bash
# Confere localmente (sem Slurm) que MPIMonteCarlo da' os mesmos hits e
# checksum que MonteCarloSequential, com P e numero de tarefas que nao sao
# multiplos, menos tarefas que escravos e grain maior que o saco.
# Uso (na raiz do repositorio, depois do make): ./scripts/test_local.sh
MPIRUN="mpirun --oversubscribe"
[ "$(id -u)" = 0 ] && MPIRUN="$MPIRUN --allow-run-as-root"
POINTS=20000
failed=0
for c in "2 1 1" "3 997 1" "5 997 7" "7 1000 13" "4 2 1" "6 3 100" "8 5000 64" "3 0 1"; do
    set -- $c; P=$1; TASKS=$2; GRAIN=$3
    expected=$(./MonteCarloSequential "$TASKS" "$POINTS" | grep -o 'hits=[0-9]* checksum=[0-9]*')
    got=$($MPIRUN -np "$P" ./MPIMonteCarlo "$TASKS" "$POINTS" "$GRAIN" | grep '^RESULT' | grep -o 'hits=[0-9]* checksum=[0-9]*')
    if [ "$expected" = "$got" ]; then r=OK; else r=FAILED; failed=1; fi
    echo "$r  P=$P tasks=$TASKS grain=$GRAIN  seq[$expected]  mpi[$got]"
done
exit $failed
