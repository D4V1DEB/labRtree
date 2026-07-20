import csv
import os

import matplotlib.pyplot as plt

BASE = os.path.dirname(os.path.abspath(__file__))
RESULTS = os.path.join(BASE, "results")
FIGS = os.path.join(RESULTS, "figures")
os.makedirs(FIGS, exist_ok=True)

COLORS = {"RTree": "#d62728", "RStarTree": "#1f77b4"}


def load_csv(name):
    path = os.path.join(RESULTS, name)
    if not os.path.exists(path):
        raise SystemExit(f"Falta el archivo: {path}")
    rows = []
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for r in reader:
            rows.append(r)
    return rows


def to_float(v):
    try:
        return float(v)
    except (TypeError, ValueError):
        return 0.0


def save(fig, fname):
    out = os.path.join(FIGS, fname)
    fig.savefig(out, dpi=150, bbox_inches="tight")
    print(f"  -> {out}")


# experimento 1
def plot_experiment1():
    print("[Exp1] Distribucion de datos")
    rows = load_csv("experimento1.csv")

    # (a) nodos visitados promedio
    dists = []
    for r in rows:
        if r["distribucion"] not in dists:
            dists.append(r["distribucion"])

    rtree_vals, rstar_vals = [], []
    for d in dists:
        for r in rows:
            if r["distribucion"] == d and r["arbol"] == "RTree":
                rtree_vals.append(to_float(r["nodos_visitados_prom"]))
            if r["distribucion"] == d and r["arbol"] == "RStarTree":
                rstar_vals.append(to_float(r["nodos_visitados_prom"]))

    fig, ax = plt.subplots(figsize=(7, 5))
    x = range(len(dists))
    w = 0.35
    ax.bar([i - w / 2 for i in x], rtree_vals, width=w,
           color=COLORS["RTree"], label="RTree")
    ax.bar([i + w / 2 for i in x], rstar_vals, width=w,
           color=COLORS["RStarTree"], label="RStarTree")
    ax.set_xticks(list(x))
    ax.set_xticklabels(dists)
    ax.set_title("Exp1 - Nodos visitados promedio por busqueda")
    ax.set_ylabel("Nodos visitados")
    ax.set_xlabel("Distribucion")
    ax.legend()
    ax.grid(axis="y", alpha=0.3)
    save(fig, "exp1_nodos.png")
    plt.close(fig)

    # (b) tiempo de insercion
    fig, ax = plt.subplots(figsize=(9, 5))
    labels, vals, cols = [], [], []
    for r in rows:
        labels.append(f"{r['distribucion']}\n{r['arbol']}")
        vals.append(to_float(r["tiempo_insercion_ms"]))
        cols.append(COLORS[r["arbol"]])
    ax.bar(range(len(labels)), vals, color=cols)
    ax.set_xticks(range(len(labels)))
    ax.set_xticklabels(labels)
    ax.set_title("Exp1 - Tiempo de insercion (ms)")
    ax.set_ylabel("Tiempo (ms)")
    ax.grid(axis="y", alpha=0.3)
    save(fig, "exp1_tiempo.png")
    plt.close(fig)


# experimento 2
def plot_experiment2():
    print("[Exp2] Escalabilidad")
    rows = load_csv("experimento2.csv")

    rtree_n, rtree_ins, rtree_nod = [], [], []
    rstar_n, rstar_ins, rstar_nod = [], [], []
    for r in rows:
        n = to_float(r["N"])
        if r["arbol"] == "RTree":
            rtree_n.append(n)
            rtree_ins.append(to_float(r["tiempo_insercion_ms"]))
            rtree_nod.append(to_float(r["nodos_visitados_prom"]))
        else:
            rstar_n.append(n)
            rstar_ins.append(to_float(r["tiempo_insercion_ms"]))
            rstar_nod.append(to_float(r["nodos_visitados_prom"]))

    # (a) Tiempo de insercion vs N
    fig, ax = plt.subplots(figsize=(7, 5))
    ax.plot(rtree_n, rtree_ins, "o-", color=COLORS["RTree"], label="RTree")
    ax.plot(rstar_n, rstar_ins, "o-", color=COLORS["RStarTree"], label="RStarTree")
    ax.set_xscale("log")
    ax.set_title("Exp2 - Tiempo de insercion vs N")
    ax.set_xlabel("N (puntos, escala log)")
    ax.set_ylabel("Tiempo insercion (ms)")
    ax.legend()
    ax.grid(alpha=0.3)
    save(fig, "exp2_insercion.png")
    plt.close(fig)

    # (b) Nodos visitados vs N
    fig, ax = plt.subplots(figsize=(7, 5))
    ax.plot(rtree_n, rtree_nod, "o-", color=COLORS["RTree"], label="RTree")
    ax.plot(rstar_n, rstar_nod, "o-", color=COLORS["RStarTree"], label="RStarTree")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_title("Exp2 - Nodos visitados vs N")
    ax.set_xlabel("N (puntos, escala log)")
    ax.set_ylabel("Nodos visitados (log)")
    ax.legend()
    ax.grid(alpha=0.3)
    save(fig, "exp2_nodos.png")
    plt.close(fig)


# experimento 3
def plot_experiment3():
    print("[Exp3] Capacidad de nodo")
    rows = load_csv("experimento3.csv")

    rtree_me, rtree_alt, rtree_nod = [], [], []
    rstar_me, rstar_alt, rstar_nod = [], [], []
    for r in rows:
        me = to_float(r["max_entries"])
        if r["arbol"] == "RTree":
            rtree_me.append(me)
            rtree_alt.append(to_float(r["altura"]))
            rtree_nod.append(to_float(r["nodos_visitados_prom"]))
        else:
            rstar_me.append(me)
            rstar_alt.append(to_float(r["altura"]))
            rstar_nod.append(to_float(r["nodos_visitados_prom"]))

    # (a) Altura vs max_entries
    fig, ax = plt.subplots(figsize=(7, 5))
    ax.plot(rtree_me, rtree_alt, "o-", color=COLORS["RTree"], label="RTree")
    ax.plot(rstar_me, rstar_alt, "o-", color=COLORS["RStarTree"], label="RStarTree")
    ax.set_title("Exp3 - Altura del arbol vs max_entries")
    ax.set_xlabel("max_entries")
    ax.set_ylabel("Altura")
    ax.legend()
    ax.grid(alpha=0.3)
    save(fig, "exp3_altura.png")
    plt.close(fig)

    # (b) Nodos visitados vs max_entries
    fig, ax = plt.subplots(figsize=(7, 5))
    ax.plot(rtree_me, rtree_nod, "o-", color=COLORS["RTree"], label="RTree")
    ax.plot(rstar_me, rstar_nod, "o-", color=COLORS["RStarTree"], label="RStarTree")
    ax.set_xscale("log")
    ax.set_title("Exp3 - Nodos visitados vs max_entries")
    ax.set_xlabel("max_entries (log)")
    ax.set_ylabel("Nodos visitados")
    ax.legend()
    ax.grid(alpha=0.3)
    save(fig, "exp3_nodos.png")
    plt.close(fig)


def main():
    print("Generando graficas desde", RESULTS)
    plot_experiment1()
    plot_experiment2()
    plot_experiment3()
    print("Listo. Figuras en:", FIGS)


if __name__ == "__main__":
    main()
