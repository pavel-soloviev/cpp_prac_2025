#pragma once
#include <memory>
#include <vector>
#include <random>

struct SAParams {
    double T0{1.0}, Tmin{1e-3};
    size_t itersPerT{100};
    size_t patienceK{100}; // seq: 100 без улучшений (параллельный задаст своё)
    uint64_t seed{42};
};

struct ISolution {
    virtual ~ISolution() = default;
    virtual double objective() const = 0;              // Критерий (К2)
    virtual std::unique_ptr<ISolution> clone() const = 0;
    virtual void randomize(std::mt19937_64& rng) = 0;  // старт
    // Для обмена между процессами (вариант 2): простая сериализация
    virtual void serialize(std::vector<uint8_t>& out) const = 0;
    virtual bool deserialize(const uint8_t* p, size_t n) = 0;
};

struct IMutation {
    virtual ~IMutation() = default;
    virtual void apply(ISolution& s, std::mt19937_64& rng) = 0;
};

struct ITempSchedule {
    virtual ~ITempSchedule() = default;
    virtual void reset(double T0) = 0;
    virtual double current() const = 0;
    virtual void next() = 0;
};

class SimulatedAnnealing {
public:
    SimulatedAnnealing(std::unique_ptr<ISolution> init,
                       std::unique_ptr<IMutation> mut,
                       std::unique_ptr<ITempSchedule> temp,
                       SAParams p);
    std::unique_ptr<ISolution> run(); // возвращает лучшее найденное
private:
    std::unique_ptr<ISolution> cur_, best_;
    std::unique_ptr<IMutation> mut_;
    std::unique_ptr<ITempSchedule> temp_;
    SAParams P_;
    std::mt19937_64 rng_;
};