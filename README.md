# T2 – Mestre/escravo com MPI: Monte Carlo para pi

Estimativa de pi por Monte Carlo no modelo mestre/escravo, para o cluster
Atlântica do LAD. Ponto de partida:
[JasonPollman/MPI-Monte-Carlo](https://github.com/JasonPollman/MPI-Monte-Carlo)
(arquivos originais preservados em `original/`).

## Modelagem

- **Tarefa** (`src/monte_carlo.h`): sortear `pontos` pontos com
  `rand_r(&semente)`, sendo a semente o índice da tarefa, e contar quantos
  caem no quarto de círculo. Como a semente depende só do índice, o resultado
  não depende de qual escravo processou a tarefa: a saída é igual à da versão
  sequencial.
- **Saco:** tarefas `0 .. ntarefas-1`. O mestre guarda `proxima` (a próxima a
  distribuir) e o vetor `acertos[]`.
- **Granularidade `G`:** cada mensagem leva um lote `{primeira, qtd}` com
  `qtd = G` tarefas seguidas (o último lote pode ser menor, `src/pi_mestre_escravo.c:93`).
- **Tags** (`src/pi_mestre_escravo.c:26-31`): `TAG_PEDIDO`, `TAG_RESULTADO`,
  `TAG_TAREFA`, `TAG_FIM`, além de `TAG_TEMPO`/`TAG_NOME` (estatísticas, fora da medição).
- **Iniciativa do escravo:** primeiro pedido na linha 140; cada resultado
  (linha 157) também vale como pedido.
- **Ordem de chegada:** `MPI_Recv` com `MPI_ANY_SOURCE` (linha 75).
- **Ordem do saco:** `primeira` vai e volta com o lote, e o mestre grava em
  `acertos[primeira + k]` (linha 82). O checksum `Σ (i+1)·acertos[i]` só bate
  com o sequencial se a ordem foi mantida.
- **Término:** saco vazio → `TAG_FIM` (linha 100); o escravo sai do laço (linha 144).

Chamadas MPI usadas: `MPI_Init`, `MPI_Comm_rank`, `MPI_Comm_size`,
`MPI_Get_processor_name`, `MPI_Send`, `MPI_Recv`, `MPI_Barrier`, `MPI_Wtime`
e `MPI_Finalize`.

## Compilar e testar localmente

```bash
make
```

```bash
make test
```

## Rodar no cluster

Os parâmetros ficam em `scripts/params.sh`. Rode primeiro a sequencial e
ajuste `TAREFAS` para que T1 fique entre 30 e 60 s:

```bash
make && mkdir -p results/raw
```

```bash
sbatch scripts/seq.sbatch
```

```bash
sbatch scripts/strong.sbatch
```

```bash
sbatch scripts/weak.sbatch
```

```bash
sbatch scripts/gran.sbatch
```

Distribuição: 2 nós exclusivos e `P/2` processos por nó. Até P = 32 isso dá
no máximo 1 processo por núcleo físico; P = 64 usa Hyper-Threading.
Escalabilidade fraca: `tarefas = TAREFAS × P`, com os mesmos `PONTOS` por tarefa.

## Tabelas e gráficos

```bash
python3 scripts/analyze.py results/raw results
```

Gera `results/tabelas.md`, os CSVs, `results/speedup.png` e
`results/eficiencia.png` (precisa de `matplotlib` só para os gráficos).

## Arquivos

| arquivo | conteúdo |
|---|---|
| `src/monte_carlo.h` | a tarefa e o checksum, usados pelas duas versões |
| `src/pi_sequencial.c` | versão sequencial (T1) |
| `src/pi_mestre_escravo.c` | versão mestre/escravo |
| `src/ida_e_volta.c` | tempo de um lote × ida e volta de mensagem |
| `scripts/*.sbatch` | jobs Slurm; saídas brutas em `results/raw/` |
| `scripts/analyze.py` | mediana/mín/máx, speed-up, eficiências, balanceamento, gráficos |
