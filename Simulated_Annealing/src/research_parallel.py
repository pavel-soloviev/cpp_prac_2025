#!/usr/bin/env python3
import subprocess
import time
import random
import re
from pathlib import Path

import matplotlib.pyplot as plt


BINARY = "./sa_sched"      # путь к бинарнику
LAW = "logoverlinear"               # Больцман
M = 80
N = 32000
DUR_MIN = 3
DUR_MAX = 20

NPROCS_LIST = [2, 4, 6, 8, 10, 12, 14, 16]
REPEATS = 1
SEED_BASE = 42

TMP_CSV = Path("par_input_M80_N32000.csv")


# ГЕНЕРАЦИЯ ВХОДНЫХ ДАННЫХ

def write_instance_csv(path: Path, M: int, N: int, dur_lo: int, dur_hi: int, seed: int):
    """Генерирует один CSV-файл с фиксированным seed."""
    rnd = random.Random(seed)
    durations = [rnd.randint(dur_lo, dur_hi) for _ in range(N)]
    with path.open("w") as f:
        f.write(f"{M},{N},\n")
        f.write(",".join(str(x) for x in durations) + ",\n")


# ЗАПУСК ПАРАЛЛЕЛЬНОГО ИО

def run_sa_parallel(nproc: int, seed: int) -> tuple[float, float | None]:
    """
    Запускает параллельный ИО:
      sa_sched --mode=par --input ... --nproc N --law LAW --seed SEED
    Возвращает (время, K2).
    """
    cmd = [
        BINARY,
        "--mode=par",
        "--input", str(TMP_CSV),
        "--nproc", str(nproc),
        "--law", LAW,
        "--seed", str(seed),
    ]

    t0 = time.perf_counter()
    proc = subprocess.run(cmd, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, text=True)
    t1 = time.perf_counter()
    wall = t1 - t0

    out = proc.stdout + "\n" + proc.stderr

    # Парсим Best K2
    m = re.search(r"Best K2:\s*([0-9]+(?:\.[0-9]+)?(?:e[+\-]?\d+)?)", out, re.IGNORECASE)
    k2 = float(m.group(1)) if m else None

    return wall, k2


# ПОСТРОЕНИЕ ГРАФИКОВ

def plot_lines(x, y, title, xlabel, ylabel, filename):
    plt.figure(figsize=(8, 5))
    plt.plot(x, y, marker="o", linestyle="-")
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.grid(True, linestyle="--", alpha=0.6)
    plt.tight_layout()
    plt.savefig(filename, dpi=200)
    print(f"Saved plot: {filename}")
    plt.close()


def main():
    # 1) Генерируем один общий CSV для всех запусков
    print(f"Generating input CSV: M={M}, N={N}")
    write_instance_csv(TMP_CSV, M, N, DUR_MIN, DUR_MAX, SEED_BASE)

    avg_times = []
    avg_k2 = []

    # 2) Перебираем числа процессов
    for nproc in NPROCS_LIST:
        times = []
        k2s = []

        print(f"\n=== Nproc = {nproc} ===")

        for rep in range(REPEATS):
            seed = SEED_BASE + rep
            wall, k2 = run_sa_parallel(nproc, seed)
            times.append(wall)
            k2s.append(k2)
            print(f"  run {rep+1}: time={wall:.2f}s  K2={k2}")

        mean_time = sum(times) / len(times)
        mean_k2 = sum(k2s) / len(k2s) if all(k is not None for k in k2s) else None

        print(f"--> mean time = {mean_time:.2f}s, mean K2 = {mean_k2}")

        avg_times.append(mean_time)
        avg_k2.append(mean_k2)

    # 3) Строим графики
    plot_lines(
        NPROCS_LIST,
        avg_times,
        title=f"Время работы vs Nproc (M={M}, N={N}, law={LAW})",
        xlabel="Nproc (число процессов)",
        ylabel="Среднее время, сек",
        filename=f"par_time_vs_nproc_{LAW}.png",
    )

    plot_lines(
        NPROCS_LIST,
        avg_k2,
        title=f"K2 vs Nproc (M={M}, N={N}, law={LAW})",
        xlabel="Nproc (число процессов)",
        ylabel="Средний K2",
        filename=f"par_k2_vs_nproc_{LAW}.png",
    )


if __name__ == "__main__":
    main()
