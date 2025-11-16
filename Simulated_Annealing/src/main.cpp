#include "sa.hpp"
#include <string>
#include <iostream>
#include <cstring>

static std::unique_ptr<ITemp> make_temp_by_name_cli(const std::string& law) {
    return create_temp_by_name(law);
}
static void usage() {
    std::cout <<
      "Usage: sa_sched --mode=seq|par --input <file>\n"
      "                [--nproc N] [--outer K]\n"
      "                [--T0 v] [--Tmin v] [--iters k] [--patience K]\n"
      "                [--law logoverlinear|boltzman|cauchy] [--seed s]\n";
}

int main(int argc, char** argv) {
    std::string mode = "seq";
    std::string input;
    unsigned nproc = 1, outerK = 10;
    double T0 = 100.0, Tmin = 1e-2;
    size_t iters = 1, patience = 100;
    std::string law = "logoverlinear";
    unsigned long long seed = 42;

    for (int i=1;i<argc;++i) {
        if (!std::strncmp(argv[i],"--mode=",7)) mode = argv[i]+7;
        else if (!std::strncmp(argv[i],"--input",7)) input = argv[++i];
        else if (!std::strncmp(argv[i],"--nproc",7)) nproc = std::stoul(argv[++i]);
        else if (!std::strncmp(argv[i],"--outer",7)) outerK = std::stoul(argv[++i]);
        else if (!std::strncmp(argv[i],"--T0",4)) T0 = std::stod(argv[++i]);
        else if (!std::strncmp(argv[i],"--Tmin",6)) Tmin = std::stod(argv[++i]);
        else if (!std::strncmp(argv[i],"--iters",7)) iters = std::stoul(argv[++i]);
        else if (!std::strncmp(argv[i],"--patience",10)) patience = std::stoul(argv[++i]);
        else if (!std::strncmp(argv[i],"--law",5)) law = argv[++i];
        else if (!std::strncmp(argv[i],"--seed",6)) seed = std::stoull(argv[++i]);
        else { usage(); return 1; }
    }
    if (input.empty()) {
        usage();
        return 1;
    }

    Instance I{};
    if (!load_instance_csv(input, I)) {
        std::cerr << "Failed to read instance: " << input << "\n"; return 2;
    }
    SAParams P{T0, Tmin, iters, patience, seed};

    if (mode == "seq" || nproc == 1) {
        auto init = make_initial_solution(I, seed);
        auto mut  = create_default_mutation();
        auto temp = make_temp_by_name_cli(law);
        auto best = run_sequential(std::move(init), std::move(mut), std::move(temp), P);
        std::cout << "Best K2: " << objective_of(*best) << "\n";
        pretty_print_solution(*best, I);
        return 0;
    }

    if (mode == "par") {
        auto best = run_parallel(I, P, nproc, outerK, law);
        std::cout << "Best K2: " << objective_of(*best) << "\n";
        pretty_print_solution(*best, I);
        return 0;
    }

    usage();
    return 1;
}



