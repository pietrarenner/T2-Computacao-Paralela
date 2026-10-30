# Mesmas flags de otimizacao nas versoes sequencial e paralela.
CC     = gcc
MPICC  = mpicc
CFLAGS = -O2 -Wall -std=gnu11

all: MonteCarloSequential MPIMonteCarlo pingpong

MonteCarloSequential: MonteCarloSequential.c montecarlo.h
	$(CC) $(CFLAGS) -o $@ $<

MPIMonteCarlo: MPIMonteCarlo.c montecarlo.h
	$(MPICC) $(CFLAGS) -o $@ $<

pingpong: pingpong.c montecarlo.h
	$(MPICC) $(CFLAGS) -o $@ $<

# Confere que a versao MPI da' o mesmo resultado que a sequencial.
test: all
	./scripts/test_local.sh

clean:
	rm -f MonteCarloSequential MPIMonteCarlo pingpong

.PHONY: all test clean
