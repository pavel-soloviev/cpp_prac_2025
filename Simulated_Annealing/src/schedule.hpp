#pragma once
#include "sa.hpp"
#include <deque>

struct Instance {
    uint32_t N{0}, M{0};
    std::vector<uint32_t> t; // size=N
};

struct ScheduleSolution : ISolution {
    explicit ScheduleSolution(const Instance* inst) : inst_(inst) {
        H.assign(inst_->N * inst_->M, 0);
        G.resize(inst_->M);
    }

    // Бинарная матрица N×M (row-major: i*M + j)
    std::vector<uint8_t> H;
    // Порядки работ на процессорах
    std::vector<std::deque<uint32_t>> G;

    // Инварианты пересборки
    void rebuildHFromOrders();
    void rebuildOrdersFromH();

    // ISolution
    double objective() const override; // К2 = sum_i C_i
    std::unique_ptr<ISolution> clone() const override;
    void randomize(std::mt19937_64& rng) override;
    void serialize(std::vector<uint8_t>& out) const override;
    bool deserialize(const uint8_t* p, size_t n) override;

    const Instance* inst_{nullptr};
};

// Утилита оценки К2 (префиксные суммы по каждому Gj)
inline uint64_t evalK2(const ScheduleSolution& S) {
    uint64_t sum = 0;
    for (const auto& Gj : S.G) {
        uint64_t acc = 0;
        for (uint32_t i : Gj) { acc += S.inst_->t[i]; sum += acc; }
    }
    return sum;
}