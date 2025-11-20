#include "sa.hpp"
#include <numeric>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <deque>

//Реализация решения
struct ScheduleSolution final : ISolution {
    explicit ScheduleSolution(const Instance* I) : inst(I) {
        H.assign(inst->N * inst->M, 0);
        G.assign(inst->M, {});
    }

    std::vector<unsigned char> H;                  // бинарная матрица N×M
    std::vector<std::deque<unsigned>> G;           // порядки на процессорах
    const Instance* inst;

    // рандомно инициализируем расписание
    void randomize(unsigned long long seed) {
        std::vector<unsigned> jobs(inst->N);
        std::iota(jobs.begin(), jobs.end(), 0);
        std::mt19937_64 rng(seed);
        std::shuffle(jobs.begin(), jobs.end(), rng);
        for (auto& g : G) g.clear();
        for (unsigned i : jobs) {
            unsigned j = rng()%inst->M;
            auto& Gj = G[j];
            size_t pos = Gj.empty()? 0 : (rng() % (Gj.size()+1));
            Gj.insert(Gj.begin()+pos, i);
        }
        rebuildHFromOrders();
    }

    void rebuildHFromOrders() {
        std::fill(H.begin(), H.end(), 0);
        for (unsigned j=0;j<inst->M;++j)
            for (unsigned i: G[j]) H[i*inst->M + j] = 1;
    }
    // void rebuildOrdersFromH() {
    //     for (auto& g: G) g.clear();
    //     for (unsigned i=0;i<inst->N;++i)
    //         for (unsigned j=0;j<inst->M;++j)
    //             if (H[i*inst->M + j]) G[j].push_back(i);
    // }
    double objective() const override {
        unsigned long long sum = 0;
        for (unsigned j=0;j<inst->M;++j) {
            unsigned long long acc = 0;
            for (unsigned i: G[j]) {
                acc += inst->t[i];
                sum += acc;
            }
        }
        return static_cast<double>(sum);
    }
    // делаем глубокую копию чтобы уметь мутировать копию и сравнивать с исходным решением
    std::unique_ptr<ISolution> clone() const override {
        auto p = std::make_unique<ScheduleSolution>(inst);
        p->H = H;
        p->G = G;
        return p;
    }

    // кодируем условие и расписание массивом байт для передачи по сокетам
    void serialize(std::vector<unsigned char>& out) const override {
        // чтобы уметь дописывать байты в конец массива
        auto push = [&](const void* ptr, size_t n){
            auto b = static_cast<const unsigned char*>(ptr);
            out.insert(out.end(), b, b+n);
        };
        push(&inst->M, sizeof(inst->M));
        push(&inst->N, sizeof(inst->N));
        for (auto v: inst->t) push(&v, sizeof(v));
        for (unsigned j=0;j<inst->M;++j) {
            unsigned sz = (unsigned)G[j].size();
            push(&sz, sizeof(sz));
            for (unsigned i: G[j]) push(&i, sizeof(i));
        }
    }

    bool deserialize(const unsigned char* p, size_t n) override {
        size_t ofs=0;
        auto take=[&](void* dst,size_t k){
            if (ofs+k>n) return false; 
            std::memcpy(dst,p+ofs,k); 
            ofs+=k; 
            return true;
        };
        unsigned M=0,N=0;
        if(!take(&M,sizeof(M)) || !take(&N,sizeof(N))) return false;
        if (M!=inst->M || N!=inst->N) return false;
        std::vector<unsigned> t(N);
        for (unsigned i=0;i<N;++i) if(!take(&t[i],sizeof(unsigned))) return false;
        if (t != inst->t) return false;
        G.assign(M,{});
        for (unsigned j=0;j<M;++j) {
            unsigned sz=0; if(!take(&sz,sizeof(sz))) return false;
            for (unsigned k=0;k<sz;++k){ unsigned x; if(!take(&x,sizeof(x))) return false; G[j].push_back(x); }
        }
        rebuildHFromOrders();
        return true;
    }
};

//Мутации 
static unsigned urand(std::mt19937_64& r, unsigned hi) {
    std::uniform_int_distribution<unsigned> U(0, hi);
    return U(r);
}
struct MutSwapInProc final : IMutation {
    void apply(ISolution& s0, std::mt19937_64& rng) override {
        auto& s = static_cast<ScheduleSolution&>(s0);
        if (s.inst->M==0) return;
        unsigned j = urand(rng, s.inst->M-1);
        if (s.G[j].size()<2) return;
        unsigned a = urand(rng, (unsigned)s.G[j].size()-1);
        unsigned b = urand(rng, (unsigned)s.G[j].size()-1);
        if (a==b) b = (b+1)%s.G[j].size();
        std::swap(s.G[j][a], s.G[j][b]);
        s.rebuildHFromOrders();
    }
};
struct MutMoveAcross final : IMutation {
    void apply(ISolution& s0, std::mt19937_64& rng) override {
        auto& s = static_cast<ScheduleSolution&>(s0);
        if (s.inst->M<2) return;
        unsigned ja = urand(rng, s.inst->M-1);
        unsigned jb = urand(rng, s.inst->M-1);
        if (ja==jb || s.G[ja].empty()) return;
        unsigned pos = urand(rng, (unsigned)s.G[ja].size()-1);
        unsigned job = s.G[ja][pos];
        s.G[ja].erase(s.G[ja].begin()+pos);
        unsigned ins = s.G[jb].empty()? 0 : urand(rng, (unsigned)s.G[jb].size());
        s.G[jb].insert(s.G[jb].begin()+ins, job);
        s.rebuildHFromOrders();
    }
};
struct MutateMixed final : IMutation {
    double pSwap, pMove;
    MutateMixed(double a, double b): pSwap(a), pMove(b) {}
    void apply(ISolution& s, std::mt19937_64& rng) override {
        std::uniform_real_distribution<double> U(0.0, 1.0);
        if (U(rng) < pSwap) {
            MutSwapInProc().apply(s, rng);
        }
        else {
            MutMoveAcross().apply(s, rng);
        }
    }
};

//Температурные законы
struct BoltzmanTemp final : ITemp {
    void reset(double T0) override { T0_ = T0; k = 1; }
    double value() const override  { return T0_ / std::log(1.0+k); }
    void next() override           { ++k; }
    double T0_{1.0}; unsigned k{1};
};
struct CauchyTemp final : ITemp {
    void reset(double T0) override { T0_ = T0; k = 0; }
    double value() const override  { return T0_ / (1.0 + (double)k); }
    void next() override           { ++k; }
    double T0_{1.0}; unsigned k{0};
};
struct LogOverLinearTemp final : ITemp {
    void reset(double T0) override { T0_ = T0; k = 1; }
    double value() const override  { return T0_ * std::log(1.0 + k) / (1.0 + (double)k); }
    void next() override           { ++k; }
    double T0_{1.0}; unsigned k{1};
};

//Последовательный ИО
class SARunner {
public:
    SARunner(std::unique_ptr<ISolution> init,
             std::unique_ptr<IMutation> mut,
             std::unique_ptr<ITemp> temp,
             SAParams P)
    : cur_(std::move(init)), best_(cur_->clone()),
      mut_(std::move(mut)), temp_(std::move(temp)), P_(P), rng_(P.seed) {
        temp_->reset(P_.T0);
    }
    std::unique_ptr<ISolution> run() {
        double bestVal = best_->objective();
        size_t noimp = 0;
        while (temp_->value() > P_.Tmin && noimp < P_.patienceK) {
            for (size_t it=0; it<P_.itersPerT; ++it) {
                auto cand = cur_->clone();
                mut_->apply(*cand, rng_);
                double dc = cand->objective() - cur_->objective();
                bool accept = (dc <= 0.0);
                if (!accept) {
                    double p = std::exp(-dc / std::max(1e-16, temp_->value()));
                    std::uniform_real_distribution<double> U(0.0,1.0);
                    accept = (U(rng_) <= p);
                }
                if (accept) cur_.swap(cand);
                double v = cur_->objective();
                if (v < bestVal) {
                    best_ = cur_->clone();
                    bestVal = v;
                    noimp = 0;
                }
            }
            temp_->next();
            ++noimp;
        }
        return std::move(best_);
    }
private:
    std::unique_ptr<ISolution> cur_, best_;
    std::unique_ptr<IMutation> mut_;
    std::unique_ptr<ITemp> temp_;
    SAParams P_;
    std::mt19937_64 rng_;
};

//Утилиты и API
//загружает экземпляр задачи из CSV-файла в структуру Instance
bool load_instance_csv(const std::string& path, Instance& I) {
    FILE* f = std::fopen(path.c_str(), "r");
    if (!f) return false;
    unsigned M,N;
    if (std::fscanf(f, "%u,%u,\n", &M, &N) != 2) {
        std::fclose(f);
        return false;
    }
    I.M = M;
    I.N = N;
    I.t.assign(N, 0);
    for (unsigned i=0;i<N;++i) {
        unsigned x=0;
        if (std::fscanf(f, "%u,", &x) != 1) { 
            std::fclose(f);
            return false;
        }
        I.t[i] = x;
    }
    std::fclose(f);
    return true;
}

std::unique_ptr<ISolution> make_initial_solution(const Instance& I, unsigned long long seed) {
    auto s = std::make_unique<ScheduleSolution>(&I);
    s->randomize(seed);
    return s;
}
std::unique_ptr<IMutation> create_default_mutation(double pSwap, double pMove) {
    return std::make_unique<MutateMixed>(pSwap, pMove);
}
std::unique_ptr<ITemp> create_temp_by_name(const std::string& law) {
    if (law=="logoverlinear") return std::make_unique<LogOverLinearTemp>();
    if (law=="boltzman")           return std::make_unique<BoltzmanTemp>();
    if (law=="cauchy")        return std::make_unique<CauchyTemp>();
    return std::make_unique<LogOverLinearTemp>();
}
std::unique_ptr<ISolution> run_sequential(std::unique_ptr<ISolution> init,
                                          std::unique_ptr<IMutation> mut,
                                          std::unique_ptr<ITemp> temp,
                                          SAParams P) {
    SARunner sa(std::move(init), std::move(mut), std::move(temp), P);
    return sa.run();
}
void pretty_print_solution(const ISolution& S, const Instance& I) {
    auto tmp = std::make_unique<ScheduleSolution>(&I);
    std::vector<unsigned char> buf;
    S.serialize(buf);
    if (!tmp->deserialize(buf.data(), buf.size())) {
        std::cout << "(cannot pretty print)\n";
        return;
    }
    for (unsigned j=0;j<I.M;++j) {
        std::cout << "P" << j << ":";
        for (size_t k=0;k<tmp->G[j].size();++k) {
            unsigned i = tmp->G[j][k];
            std::cout << " J" << i << "(" << I.t[i] << ")";
            if (k+1<tmp->G[j].size()) std::cout << " ->";
        }
        std::cout << "\n";
    }
}
double objective_of(const ISolution& S) {
    return S.objective();
}

std::vector<unsigned char> solution_to_bytes(const ISolution& S) {
    std::vector<unsigned char> b;
    S.serialize(b);
    return b;
}
std::unique_ptr<ISolution> solution_from_bytes(const Instance& I, const std::vector<unsigned char>& b) {
    auto p = std::make_unique<ScheduleSolution>(&I);
    if (!p->deserialize(b.data(), b.size())) return nullptr;
    return p;
}

