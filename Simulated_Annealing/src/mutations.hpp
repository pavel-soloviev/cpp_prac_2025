#pragma once
#include "schedule.hpp"

struct SwapInProc : IMutation {
    void apply(ISolution& s, std::mt19937_64& rng) override; // swap двух работ в одном Gj
};

struct MoveBetweenProcs : IMutation {
    void apply(ISolution& s, std::mt19937_64& rng) override; // вырезать из G_a, вставить в G_b
};

struct ReassignGreedy : IMutation {
    void apply(ISolution& s, std::mt19937_64& rng) override; // перекинуть работу на другой проц. в лучшую позицию локально по ΔK2
};