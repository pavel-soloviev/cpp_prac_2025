#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Генератор входных данных (CSV) для sa_sched.

Формат CSV:
  1-я строка: "M,N,"
  2-я строка: "t1,t2,...,tN,"

Параметры:
  --procs / -m : число процессоров (M)
  --jobs  / -n : число работ (N)
  --tmin  / -a : минимальная длительность
  --tmax  / -b : максимальная длительность
  --out        : путь к выходному файлу (по умолчанию sample.csv)
  --seed       : число для воспроизводимости

Пример:
  ./gen_instance.py -m 3 -n 8 -a 1 -b 10 --out sample.csv
"""

import argparse
import random


def main():
    ap = argparse.ArgumentParser(description="Генератор входных данных для sa_sched (CSV)")
    ap.add_argument("-m", "--procs", type=int, required=True, help="число процессоров M")
    ap.add_argument("-n", "--jobs", type=int, required=True, help="число работ N")
    ap.add_argument("-a", "--tmin", type=int, required=True, help="минимальная длительность")
    ap.add_argument("-b", "--tmax", type=int, required=True, help="максимальная длительность")
    ap.add_argument("--out", default="sample.csv", help="выходной CSV файл")
    ap.add_argument("--seed", type=int, default=None, help="seed для воспроизводимости")
    args = ap.parse_args()

    if args.seed is not None:
        random.seed(args.seed)
    if args.procs <= 0 or args.jobs <= 0:
        ap.error("M и N должны быть положительными")
    if args.tmin <= 0 or args.tmax < args.tmin:
        ap.error("некорректный диапазон длительностей")

    durations = [random.randint(args.tmin, args.tmax) for _ in range(args.jobs)]

    with open(args.out, "w", encoding="utf-8") as f:
        f.write(f"{args.procs},{args.jobs},\n")
        f.write(",".join(str(x) for x in durations) + ",\n")

    print(f"Сгенерирован файл: {args.out}")
    print(f"Процессоров: {args.procs}, работ: {args.jobs}, диапазон: [{args.tmin}, {args.tmax}]")
    print(f"Пример содержимого:\n{args.procs},{args.jobs},\n{','.join(map(str, durations))},")


if __name__ == "__main__":
    main()
