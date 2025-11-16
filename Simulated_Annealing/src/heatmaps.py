#!/usr/bin/env python3
import subprocess
import time
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import csv
import re
import random


BIN = "./sa_sched"       # путь к бинарнику
REPEATS = 5
SEED_BASE = 42

M_VALUES = [20, 40, 60, 80, 100]
N_VALUES = [4000, 8000, 16000, 32000]

LAW = "cauchy"    # закон охлаждения

TMP_DIR = Path("tmp_heatmap_inputs")
TMP_DIR.mkdir(exist_ok=True)


# Генерация входного файла CSV
def make_instance_csv(path: Path, M, N, dur_lo=3, dur_hi=20, seed=42):
    rnd = random.Random(seed)
    durations = [rnd.randint(dur_lo, dur_hi) for _ in range(N)]
    with path.open("w") as f:
        f.write(f"{M},{N},\n")
        f.write(",".join(map(str, durations)) + ",\n")


# Запуск ИО и парсинг результата
def run_sa(bin_path, csv_path, law, seed):
    cmd = [
        str(bin_path),
        "--mode=seq",
        "--input", str(csv_path),
        "--law", law,
        "--seed", str(seed)
    ]

    t0 = time.perf_counter()
    proc = subprocess.run(cmd, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, text=True)
    t1 = time.perf_counter()

    wall = t1 - t0
    out = proc.stdout + "\n" + proc.stderr

    m = re.search(r"Best K2:\s*([0-9]+(?:\.[0-9]+)?(?:e[+\-]?\d+)?)", out)
    best_k2 = float(m.group(1)) if m else None

    return wall, best_k2


# Основной расчёт тепловых карт
def compute_heatmaps():
    T = np.zeros((len(M_VALUES), len(N_VALUES)))
    K = np.zeros((len(M_VALUES), len(N_VALUES)))

    for i, M in enumerate(M_VALUES):
        for j, N in enumerate(N_VALUES):

            csv_path = TMP_DIR / f"inst_{M}_{N}.csv"
            make_instance_csv(csv_path, M, N, seed=SEED_BASE)

            times = []
            k2s = []

            print(f"=== M={M} N={N} ===")

            for rep in range(REPEATS):
                seed = SEED_BASE + rep
                wall, k2 = run_sa(BIN, csv_path, LAW, seed)
                times.append(wall)
                k2s.append(k2)

                print(f"  rep {rep+1}: time={wall:.3f}s  K2={k2}")

            T[i, j] = np.mean(times)
            K[i, j] = np.mean(k2s)

            print(f"--> mean time = {T[i, j]:.3f}s   mean K2 = {K[i, j]:.3f}\n")

    return T, K


# Рисование тепловых карт
def draw_heatmap(matrix, title, xticks, yticks, cmap):
    plt.figure(figsize=(10, 6))
    ax = sns.heatmap(
        matrix,
        annot=True,
        fmt=".1f",
        cmap=cmap,
        xticklabels=xticks,
        yticklabels=yticks,
        linewidths=0.5,
        linecolor="gray",
        cbar_kws={'label': title}
    )
    ax.set_xlabel("N")
    ax.set_ylabel("M")
    ax.set_title(title)
    plt.tight_layout()
    plt.savefig(f"{title}.png", dpi=200)
    plt.close()


def main():
    print(f"=== Heatmap for law = {LAW} ===")

    T, K = compute_heatmaps()

    draw_heatmap(
        T,
        title=f"Среднее время работы (law={LAW})",
        xticks=N_VALUES,
        yticks=M_VALUES,
        cmap="Blues"
    )

    draw_heatmap(
        K,
        title=f"Средний K2 (law={LAW})",
        xticks=N_VALUES,
        yticks=M_VALUES,
        cmap="Purples"
    )


if __name__ == "__main__":
    main()
