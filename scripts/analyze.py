#!/usr/bin/env python3
"""Recalcula tabelas e gráficos do T2 a partir das saídas brutas do Slurm.

Uso: python3 scripts/analyze.py [results/raw] [results]

Lê results/raw/t2-{seq,strong,weak,gran}-*.out e gera em results/:
  tabelas.md          todas as tabelas (mediana, mínimo, máximo)
  strong.csv weak.csv gran.csv slaves_G*.csv
  speedup.png         speed-up da forte com a reta ideal
  eficiencia.png      eficiência da forte e da fraca
"""
import csv
import glob
import os
import re
import statistics
import sys
from collections import defaultdict

RAW = sys.argv[1] if len(sys.argv) > 1 else "results/raw"
OUT = sys.argv[2] if len(sys.argv) > 2 else "results"

# Nucleos fisicos nos 2 nos (atlantica: 2 x 8). Acima disso entra o HT.
PHYS_CORES = 16

KV = re.compile(r"(\w+)=(\S+)")


def parse_kv(line):
    d = dict(KV.findall(line))
    for k, v in d.items():
        try:
            d[k] = int(v)
        except ValueError:
            try:
                d[k] = float(v)
            except ValueError:
                pass
    return d


def read_runs(prefix):
    """Lista de execuções; cada uma = RESULT + linhas SLAVE que a seguem."""
    runs, bench = [], []
    for path in sorted(glob.glob(os.path.join(RAW, f"{prefix}-*.out"))):
        with open(path) as f:
            for line in f:
                if line.startswith("RESULT "):
                    runs.append({**parse_kv(line), "slaves": [], "file": path})
                elif line.startswith("SLAVE ") and runs:
                    runs[-1]["slaves"].append(parse_kv(line))
                elif line.startswith("MASTER ") and runs:
                    runs[-1]["master"] = parse_kv(line)["host"]
    return runs, bench


def stats(values):
    return statistics.median(values), min(values), max(values)


def fmt(x, nd=3):
    return f"{x:.{nd}f}"


def md_table(header, rows):
    out = ["| " + " | ".join(header) + " |", "|" + "---|" * len(header)]
    out += ["| " + " | ".join(str(c) for c in r) + " |" for r in rows]
    return "\n".join(out) + "\n"


def write_csv(name, header, rows):
    with open(os.path.join(OUT, name), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


def check_equal(runs, label):
    """Todas as execuções da mesma entrada devem ter o mesmo hits/checksum."""
    by_input = defaultdict(set)
    for r in runs:
        by_input[(r["tasks"], r["points"])].add((r["hits"], r["checksum"]))
    bad = {k: v for k, v in by_input.items() if len(v) > 1}
    if bad:
        print(f"ATENÇÃO [{label}]: resultados diferentes para a mesma entrada: {bad}")
    return by_input


def node_split(run):
    """Quantos processos (mestre + escravos) ficaram em cada nó."""
    count = defaultdict(int)
    if "master" in run:
        count[run["master"]] += 1
    for s in run["slaves"]:
        count[s["host"]] += 1
    return " + ".join(f"{n}:{c}" for n, c in sorted(count.items()))


def main():
    os.makedirs(OUT, exist_ok=True)
    md = []

    # ---------------- sequencial (T1) ----------------
    seq, _ = read_runs("t2-seq")
    if not seq:
        sys.exit(f"Nenhuma saída t2-seq em {RAW}")
    t1_med, t1_min, t1_max = stats([r["time"] for r in seq])
    ref = seq[0]
    ref_key = (ref["hits"], ref["checksum"])
    check_equal(seq, "seq")
    md.append("## Sequencial (T1)\n")
    md.append(md_table(
        ["tarefas", "pontos/tarefa", "T1 mediana (s)", "mín", "máx", "hits", "checksum", "PI"],
        [[ref["tasks"], ref["points"], fmt(t1_med), fmt(t1_min), fmt(t1_max),
          ref["hits"], ref["checksum"], ref["pi"]]]))

    # ---------------- forte ----------------
    strong, _ = read_runs("t2-strong")
    rows, plot_strong = [], []
    by_p = defaultdict(list)
    for r in strong:
        by_p[r["P"]].append(r)
    for p in sorted(by_p):
        rs = by_p[p]
        med, mn, mx = stats([r["time"] for r in rs])
        s = t1_med / med
        equiv = all((r["hits"], r["checksum"]) == ref_key for r in rs)
        rows.append([p, rs[0]["grain"], fmt(med), fmt(mn), fmt(mx), fmt(s, 2),
                     fmt(s / p, 3), fmt(s / (p - 1), 3), node_split(rs[0]),
                     "sim" if equiv else "NÃO"])
        plot_strong.append((p, s, s / p))
    hdr = ["P", "G", "T_P mediana (s)", "mín", "máx", "speed-up", "E = S/P",
           "S/(P-1)", "processos por nó", "hits/checksum = seq"]
    md.append("## Escalabilidade forte\n")
    md.append(md_table(hdr, rows))
    write_csv("strong.csv", hdr, rows)

    # ---------------- fraca ----------------
    weak, _ = read_runs("t2-weak")
    check_equal(weak, "weak")
    rows, plot_weak = [], []
    by_p = defaultdict(list)
    for r in weak:
        by_p[r["P"]].append(r)
    for p in sorted(by_p):
        rs = by_p[p]
        med, mn, mx = stats([r["time"] for r in rs])
        work = rs[0]["tasks"] * rs[0]["points"]
        rows.append([p, rs[0]["tasks"], f"{work:.3e}", fmt(rs[0]["tasks"] / ref["tasks"], 1),
                     fmt(med), fmt(mn), fmt(mx), fmt(t1_med / med, 3)])
        plot_weak.append((p, t1_med / med))
    hdr = ["P", "tarefas", "pontos totais", "trabalho / ref", "T_P mediana (s)",
           "mín", "máx", "E_fraca = T1/T_P"]
    md.append("## Escalabilidade fraca\n")
    md.append(md_table(hdr, rows))
    write_csv("weak.csv", hdr, rows)

    # ---------------- granularidade ----------------
    gran, _ = read_runs("t2-gran")

    by_g = defaultdict(list)
    for r in gran:
        by_g[r["grain"]].append(r)
    rows = []
    for g in sorted(by_g):
        rs = by_g[g]
        med, mn, mx = stats([r["time"] for r in rs])
        equiv = all((r["hits"], r["checksum"]) == ref_key for r in rs) if rs[0]["tasks"] == ref["tasks"] else "-"
        nmsg = -(-rs[0]["tasks"] // g)
        rows.append([g, nmsg, fmt(med), fmt(mn), fmt(mx), fmt(t1_med / med, 2),
                     "sim" if equiv is True else ("NÃO" if equiv is False else "-")])
    hdr = ["G", "lotes", "T_P mediana (s)", "mín", "máx", "speed-up", "hits/checksum = seq"]
    pg = gran[0]["P"] if gran else "?"
    md.append(f"## Granularidade (b): P = {pg}\n")
    md.append(md_table(hdr, rows))
    write_csv("gran.csv", hdr, rows)

    # (c) balanceamento: execução mediana de cada G
    md.append(f"## Granularidade (a) e balanceamento (c): por escravo, execução mediana de cada G, P = {pg}\n")
    hdr = ["G", "tarefas mín", "tarefas máx", "ocupado mín (s)", "ocupado máx (s)",
           "ocioso no fim máx (s)", "ocupação média", "tempo/tarefa (ms)",
           "ida e volta (ms)", "ida e volta mesmo nó (ms)", "ida e volta outro nó (ms)",
           "lote / ida e volta"]
    rows = []
    for g in sorted(by_g):
        rs = sorted(by_g[g], key=lambda r: r["time"])
        r = rs[len(rs) // 2]
        sl = r["slaves"]
        idle_end = [max(0.0, r["time"] - s["stop_at"]) for s in sl]
        busy = [s["busy"] for s in sl]
        # Item (a), medido na própria execução com P processos:
        # tempo de uma tarefa = tempo ocupado / tarefas; ida e volta = média
        # medida por cada escravo entre enviar o pedido/resultado e receber a
        # resposta (mensagens + fila no mestre). Saídas antigas sem "rtt"
        # usam uma estimativa a partir de stop_at e busy.
        task_ms = 1e3 * sum(busy) / max(1, sum(s["tasks"] for s in sl))
        if all("rtt" in s for s in sl):
            waits = [s["rtt"] for s in sl]
            same = [s["rtt"] for s in sl if s["host"] == r.get("master")]
            other = [s["rtt"] for s in sl if s["host"] != r.get("master")]
        else:
            waits = [(s["stop_at"] - s["busy"]) / -(-s["tasks"] // g) for s in sl if s["tasks"]]
            same = other = []
        mean_ms = lambda v: fmt(1e3 * statistics.mean(v), 3) if v else "-"
        rows.append([g, min(s["tasks"] for s in sl), max(s["tasks"] for s in sl),
                     fmt(min(busy)), fmt(max(busy)), fmt(max(idle_end)),
                     fmt(statistics.mean(busy) / r["time"], 3), fmt(task_ms, 3),
                     mean_ms(waits), mean_ms(same), mean_ms(other),
                     fmt(task_ms * g / (1e3 * statistics.mean(waits)), 1)])
        write_csv(f"slaves_G{g}.csv", ["rank", "host", "tasks", "busy_s", "stop_at_s", "idle_end_s", "rtt_s"],
                  [[s["rank"], s["host"], s["tasks"], s["busy"], s["stop_at"], i, s.get("rtt", "")]
                   for s, i in zip(sl, idle_end)])
    md.append(md_table(hdr, rows))

    with open(os.path.join(OUT, "tabelas.md"), "w") as f:
        f.write("\n".join(md))
    print("\n".join(md))

    # ---------------- gráficos ----------------
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib não instalado: gráficos não gerados (pip install matplotlib)")
        return

    if not plot_strong:
        print("sem saídas t2-strong: gráficos não gerados")
        return
    ps = [p for p, _, _ in plot_strong]
    fig, ax = plt.subplots(figsize=(4.5, 3.2))
    ax.plot(ps, ps, "--", color="gray", label="ideal (S = P)")
    ax.plot(ps, [s for _, s, _ in plot_strong], "o-", label="medido")
    if PHYS_CORES in ps:
        ax.axvline(PHYS_CORES, color="lightgray", lw=0.8)  # acima: HT
    ax.set_xscale("log", base=2)
    ax.set_yscale("log", base=2)
    ax.set_xticks(ps, [str(p) for p in ps])
    ax.set_yticks([1] + ps, ["1"] + [str(p) for p in ps])
    ax.set_xlabel("processos (P)")
    ax.set_ylabel("speed-up")
    ax.legend()
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "speedup.png"), dpi=200)

    fig, ax = plt.subplots(figsize=(4.5, 3.2))
    ax.plot(ps, [e for _, _, e in plot_strong], "o-", label="forte (S/P)")
    if plot_weak:
        ax.plot([p for p, _ in plot_weak], [e for _, e in plot_weak], "s-", label="fraca (T1/T_P)")
    ax.axhline(1.0, ls="--", color="gray")
    if PHYS_CORES in ps:
        ax.axvline(PHYS_CORES, color="lightgray", lw=0.8)  # acima: HT
    ax.set_xscale("log", base=2)
    ax.set_xticks(ps, [str(p) for p in ps])
    ax.set_ylim(0, 1.1)
    ax.set_xlabel("processos (P)")
    ax.set_ylabel("eficiência")
    ax.legend()
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "eficiencia.png"), dpi=200)


if __name__ == "__main__":
    main()
