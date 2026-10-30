# T2 – Mestre/escravo com MPI: Monte Carlo para pi

Estimativa de pi por Monte Carlo no modelo mestre/escravo, para o cluster
Atlântica do LAD. Ponto de partida:
[JasonPollman/MPI-Monte-Carlo](https://github.com/JasonPollman/MPI-Monte-Carlo).
Os arquivos originais estão preservados em `original/`; a versão nova mantém
os mesmos nomes de arquivo, de funções (`master_io`, `slave_io`), de variáveis
(`rank`, `num_processes`, `n`, `count`, `seed`, `computations`) e os mesmos
valores de tag do original (0, 200, 300, 400).

## Modelagem (`MPIMonteCarlo.c`)

- **Tarefa** (`montecarlo.h`, `compute_task`): sortear `n` pontos com
  `rand_r(&seed)`, em que `seed` é o índice da tarefa, e contar quantos caem no
  quarto de círculo. O resultado não depende de qual escravo processou a
  tarefa, então a saída é igual à da versão sequencial.
- **Saco:** tarefas `0 .. num_tasks-1`. O mestre guarda `next` (a próxima a
  distribuir) e o vetor `counts[]`.
- **Granularidade:** cada mensagem leva um lote `{first, size}` com
  `size = grain` tarefas seguidas. O último lote pode ser menor (linha 75).
- **Tags** (linhas 29-34): `TAG_KILL` (0), `TAG_REQUEST` (100), `TAG_WORK`
  (200), `TAG_RESULT` (300), `TAG_COMPUTATIONS` (400) e `TAG_NAME` (500).
  As duas últimas são estatísticas, enviadas fora da região medida.
- **Iniciativa do escravo:** primeiro pedido na linha 126; cada resultado
  (linha 143) também vale como pedido.
- **Ordem de chegada:** `MPI_Recv` com `MPI_ANY_SOURCE` (linha 57).
- **Ordem do saco:** `first` vai e volta com o lote, e o mestre grava em
  `counts[first + k]` (linha 64). O checksum `Σ (i+1)·counts[i]` só bate com o
  da versão sequencial se a ordem foi mantida.
- **Término:** saco vazio → `TAG_KILL` (linha 82); o escravo sai do laço (linha 130).

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
ajuste `TASKS` para que T1 fique entre 30 e 60 s:

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
Escalabilidade fraca: `tasks = TASKS × P`, com os mesmos `POINTS` por tarefa.

## Tabelas e gráficos

```bash
python3 scripts/analyze.py results/raw results
```

Gera `results/tabelas.md`, os CSVs, `results/speedup.png` e
`results/eficiencia.png` (precisa de `matplotlib` só para os gráficos).

## Arquivos

| arquivo | conteúdo |
|---|---|
| `montecarlo.h` | a tarefa e o checksum, usados pelas duas versões |
| `MonteCarloSequential.c` | versão sequencial (T1) |
| `MPIMonteCarlo.c` | versão mestre/escravo |
| `pingpong.c` | tempo de um lote × ida e volta de mensagem |
| `scripts/*.sbatch` | jobs Slurm; saídas brutas em `results/raw/` |
| `scripts/analyze.py` | mediana/mín/máx, speed-up, eficiências, balanceamento, gráficos |
| `original/` | código original do repositório base |
