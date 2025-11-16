#pragma once
#include <vector>
// #include <deque>
#include <memory>
#include <random>
#include <string>

// ---- Параметры ИО ----
struct SAParams {
    double T0{100.0};
    double Tmin{1e-2};
    size_t itersPerT{1};
    size_t patienceK{100};
    unsigned long long seed{42};
};

// Данные задачи
struct Instance {
    unsigned N{0}, M{0};
    std::vector<unsigned> t; // длительности работ size=N
};

//Интерфейсы
struct ISolution {
    virtual ~ISolution() = default;
    virtual double objective() const = 0;
    virtual std::unique_ptr<ISolution> clone() const = 0;
    virtual void serialize(std::vector<unsigned char>& out) const = 0;
    virtual bool deserialize(const unsigned char* p, size_t n) = 0;
};

struct IMutation {
    virtual ~IMutation() = default;
    virtual void apply(ISolution& s, std::mt19937_64& rng) = 0;
};

struct ITemp {
    virtual ~ITemp() = default;
    virtual void reset(double T0) = 0;
    virtual double value() const = 0;
    virtual void next() = 0;
};

// API core.cpp
bool load_instance_csv(const std::string& path, Instance& I);

std::unique_ptr<ISolution> make_initial_solution(const Instance& I, unsigned long long seed);
std::unique_ptr<IMutation> create_default_mutation(double pSwap=0.6, double pMove=0.4);
std::unique_ptr<ITemp>     create_temp_by_name(const std::string& law);

std::unique_ptr<ISolution> run_sequential(std::unique_ptr<ISolution> init,
                                          std::unique_ptr<IMutation> mut,
                                          std::unique_ptr<ITemp> temp,
                                          SAParams P);

void   pretty_print_solution(const ISolution& S, const Instance& I);
double objective_of(const ISolution& S);

// для параллельного обмена
std::vector<unsigned char> solution_to_bytes(const ISolution& S);
std::unique_ptr<ISolution> solution_from_bytes(const Instance& I,
                                               const std::vector<unsigned char>& b);

// API parallel.cpp
std::unique_ptr<ISolution> run_parallel(const Instance& I, SAParams P,
                                        unsigned nproc, unsigned outerPatience,
                                        const std::string& law);
