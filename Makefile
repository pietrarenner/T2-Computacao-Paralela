# Mesmas flags de otimizacao nas versoes sequencial e paralela.
CC     = gcc
MPICC  = mpicc
CFLAGS = -O2 -Wall -std=gnu11

all: pi_sequencial pi_mestre_escravo ida_e_volta

pi_sequencial: src/pi_sequencial.c src/monte_carlo.h
	$(CC) $(CFLAGS) -o $@ $<

pi_mestre_escravo: src/pi_mestre_escravo.c src/monte_carlo.h
	$(MPICC) $(CFLAGS) -o $@ $<

ida_e_volta: src/ida_e_volta.c src/monte_carlo.h
	$(MPICC) $(CFLAGS) -o $@ $<

# Confere que a versao MPI da' o mesmo resultado que a sequencial.
test: all
	./scripts/teste_local.sh

clean:
	rm -f pi_sequencial pi_mestre_escravo ida_e_volta

.PHONY: all test clean
