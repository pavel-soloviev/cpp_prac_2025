#!/usr/bin/env python3
import argparse
import csv
import random
import subprocess
import time
from pathlib import Path
import re

LAWS = ["logoverlinear", "boltzman", "cauchy"]


# генерация входного CSV
def write_instance_csv(path: Path, M: int, N: int, dur_lo: int, dur_hi: int, seed: int):
    rnd = random.Random(seed)
    durations = [rnd.randint(dur_lo, dur_hi) for _ in range(N)]
    with path.open("w") as f:
        f.write(f"{M},{N},\n")
        f.write(",".join(map(str, durations)) + ",\n")


# запуск SA
def run_sa_seq(bin_path: Path, input_csv: Path, law: str, seed: int):
    """
    Запускает ИО без каких-либо параметров — только --mode=seq, --input и --law.
    Все параметры температуры берутся из твоих DEFAULT-PARAMS (main.cpp → SAParams P).
    """
    cmd = [
        str(bin_path),
        "--mode=seq",
        "--input", str(input_csv),
        "--law", law,
        "--seed", str(seed)
    ]

    t0 = time.perf_counter()
    proc = subprocess.run(cmd, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE,
                          text=True)
    t1 = time.perf_counter()

    wall = t1 - t0
    full_out = proc.stdout + "\n" + proc.stderr

    # Парсим Best K2: ...
    m = re.search(
        r"Best K2:\s*([0-9]+(?:\.[0-9]+)?(?:e[+\-]?\d+)?)",
        full_out,
        re.IGNORECASE
    )

    best_k2 = float(m.group(1)) if m else None

    return wall, best_k2, full_out


# поиск тяжёлых случаев
def find_heavy_cases(bin_path, Ms, Ns, laws, time_threshold,
                     dur_lo, dur_hi, seed, out_csv):
    out_csv.parent.mkdir(parents=True, exist_ok=True)
    results = []
    heavy = []

    with out_csv.open("w") as f:
        wr = csv.writer(f)
        wr.writerow(["M", "N", "law", "time_sec", "K2"])

        tmp_dir = Path("tmp_inputs")
        tmp_dir.mkdir(exist_ok=True)

        for M in Ms:
            for N in Ns:
                csv_path = tmp_dir / f"inst_{M}_{N}.csv"
                write_instance_csv(csv_path, M, N, dur_lo, dur_hi, seed)

                for law in laws:
                    wall, k2, out = run_sa_seq(bin_path, csv_path, law, seed)
                    print(f"[scan] M={M:3d} N={N:4d} law={law:13s} time={wall:8.2f}s  K2={k2}")
                    wr.writerow([M, N, law, wall, k2])

                    if wall > time_threshold:
                        heavy.append(dict(M=M, N=N, law=law, time=wall, k2=k2))

    return heavy


def analyze_heavy(heavy):
    if not heavy:
        print("\n❗ Не найдено ни одного случая, работающего > threshold.")
        return

    print("\n===== ТЯЖЁЛЫЕ СЛУЧАИ (time > threshold) =====")

    # группировка по (M,N)
    groups = {}
    for r in heavy:
        groups.setdefault((r["M"], r["N"]), []).append(r)

    for (M, N), recs in sorted(groups.items()):
        print(f"\nM={M}, N={N}: ")

        # какой закон самый долгий?
        worst = max(recs, key=lambda r: r["time"])
        print(f"  Самое долгое время: {worst['time']:.2f}s  закон={worst['law']}  K2={worst['k2']}")

        # сводка
        for r in sorted(recs, key=lambda x: x["law"]):
            print(f"    {r['law']:13s}  time={r['time']:.2f}s  K2={r['k2']}")


def parse_int_list(s):
    return [int(x) for x in s.split(",")]


def main():
    ap = argparse.ArgumentParser(description="Исследование последовательного ИО (упрощённый скрипт)")
    ap.add_argument("--bin", type=Path, default=Path("./sa_sched"))
    ap.add_argument("--M", type=str, default="20, 40, 60, 80, 100")
    ap.add_argument("--N", type=str, default="1000,10000,50000,100000")
    ap.add_argument("--laws", type=str, default="logoverlinear,boltzman,cauchy")
    ap.add_argument("--dur-lo", type=int, default=3)
    ap.add_argument("--dur-hi", type=int, default=20)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--threshold", type=float, default=60.0)
    ap.add_argument("--out", type=Path, default=Path("seq_results_simple.csv"))

    args = ap.parse_args()

    if args.bin.is_absolute():
        bin_path = args.bin
    else:
        script_dir = Path(__file__).resolve().parent
        bin_path = (script_dir / args.bin).resolve()

    Ms = parse_int_list(args.M)
    Ns = parse_int_list(args.N)
    laws = [x.strip() for x in args.laws.split(",")]

    print("=== Поиск тяжёлых случаев последовательного ИО ===")
    heavy = find_heavy_cases(
        bin_path,
        Ms, Ns, laws,
        time_threshold=args.threshold,
        dur_lo=args.dur_lo,
        dur_hi=args.dur_hi,
        seed=args.seed,
        out_csv=args.out
    )

    print(f"\nВсе данные → {args.out}")
    analyze_heavy(heavy)


if __name__ == "__main__":
    main()
