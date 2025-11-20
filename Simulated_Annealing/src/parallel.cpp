#include "sa.hpp"
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <iostream>

// помощники чтения/записи
// гарантированно пишем в канал все байты до конца
static bool write_all(int fd, const void* buf, size_t n) {
    const unsigned char* p = static_cast<const unsigned char*>(buf);
    size_t off = 0;
    while (off < n) {
        ssize_t w = ::write(fd, p+off, n-off);
        if (w < 0) { if (errno==EINTR) continue; return false; }
        if (w == 0) return false;
        off += (size_t)w;
    }
    return true;
}
static bool read_all(int fd, void* buf, size_t n) {
    unsigned char* p = static_cast<unsigned char*>(buf);
    size_t got = 0;
    while (got < n) {
        ssize_t r = ::read(fd, p+got, n-got);
        if (r < 0) { if (errno==EINTR) continue; return false; }
        if (r == 0) return false;
        got += (size_t)r;
    }
    return true;
}


enum : uint32_t { OP_BEST=1, OP_STOP=2, OP_RESULT=3 };

static std::unique_ptr<ITemp> make_temp_by_law(const std::string& law) {
    return create_temp_by_name(law);
}

// небольшой "джиттер" стартового решения
static void jitter_solution(ISolution& s, unsigned nJit, unsigned long long seed) {
    std::mt19937_64 rng(seed);
    auto mut = create_default_mutation();
    for (unsigned k=0;k<nJit;++k) mut->apply(s, rng);
}

// цикл воркера: получает best → локально улучшает → возвращает результат
static void worker_loop(int fd, const Instance& I, SAParams P, std::string law, unsigned wid) {
    for (;;) {
        uint32_t op = 0;
        if (!read_all(fd, &op, sizeof(op))) _exit(0);

        if (op == OP_STOP) {
            _exit(0);
        } else if (op == OP_BEST) {
            uint32_t sz = 0;
            if (!read_all(fd, &sz, sizeof(sz))) _exit(0);
            std::vector<unsigned char> buf(sz);
            if (sz && !read_all(fd, buf.data(), sz)) _exit(0);

            // восстановим решение
            auto base = solution_from_bytes(I, buf);
            if (!base) _exit(0);

            // джиттер от глобального best
            jitter_solution(*base, 3u, P.seed + 7919ULL*wid);

            // локальный запуск SA
            auto mut = create_default_mutation();
            auto tmp = make_temp_by_law(law);
            auto loc = run_sequential(std::move(base), std::move(mut), std::move(tmp), P);

            // сериализуем локальный best
            auto bytes = solution_to_bytes(*loc);
            uint32_t out_sz = (uint32_t)bytes.size();

            uint32_t r = OP_RESULT;
            if (!write_all(fd, &r, sizeof(r)) ||
                !write_all(fd, &out_sz, sizeof(out_sz)) ||
                (out_sz && !write_all(fd, bytes.data(), out_sz))) {
                _exit(0);
            }
        } else {
            _exit(0);
        }
    }
}

// мастер
std::unique_ptr<ISolution> run_parallel(const Instance& I, SAParams P,
                                        unsigned nproc, unsigned outerPatience,
                                        const std::string& law)
{
    if (nproc == 0) nproc = 1;

    // создаём начальный best
    auto init = make_initial_solution(I, P.seed);
    auto mut  = create_default_mutation();
    auto tmp  = create_temp_by_name(law);
    auto best = run_sequential(std::move(init), std::move(mut), std::move(tmp), P);
    double bestObj = objective_of(*best);

    // создаём воркеров один раз
    std::vector<int> socks(nproc, -1);
    std::vector<pid_t> pids(nproc, -1);

    for (unsigned i=0;i<nproc;++i) {
        int sd[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sd) != 0) {
            perror("socketpair");
            break;
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(sd[0]);
            close(sd[1]);
            break;
        }
        if (pid == 0) {
            close(sd[0]);
            worker_loop(sd[1], I, P, law, i+1);
            _exit(0);
        }
        close(sd[1]);
        socks[i] = sd[0];
        pids[i]  = pid;
    }

    // барьерные раунды
    size_t noimp = 0;
    while (noimp < outerPatience) {
        // Broadcast best всем воркерам
        auto bytes = solution_to_bytes(*best);
        uint32_t sz = (uint32_t)bytes.size();
        for (unsigned i=0;i<nproc;++i) {
            uint32_t op = OP_BEST;
            if (!write_all(socks[i], &op, sizeof(op)) ||
                !write_all(socks[i], &sz, sizeof(sz)) ||
                (sz && !write_all(socks[i], bytes.data(), sz))) {
                // воркер умер — пропустим
            }
        }

        bool improved = false;
        // Gather кандидатов
        for (unsigned i=0;i<nproc;++i) {
            uint32_t op = 0;
            if (!read_all(socks[i], &op, sizeof(op))) continue;
            if (op != OP_RESULT) continue;
            uint32_t rsz = 0;
            if (!read_all(socks[i], &rsz, sizeof(rsz))) continue;
            std::vector<unsigned char> buf(rsz);
            if (rsz && !read_all(socks[i], buf.data(), rsz)) continue;

            auto cand = solution_from_bytes(I, buf);
            if (!cand) continue;
            double v = objective_of(*cand);
            if (v < bestObj) {
                best = std::move(cand);
                bestObj = v;
                improved = true;
            }
        }
        if (improved) {
            noimp = 0; 
            std::cout << "New Best K2: " << objective_of(*best) << "\n";
            // std::cout << "new best!!!" << std::endl;
            // pretty_print_solution(*best, I);
        } else {
            std::cout << "no improve(((\n";
            ++noimp;
            // std::cout << "no improve(((" << std::endl;
        }
    }

    // послать STOP
    for (unsigned i=0;i<nproc;++i) {
        if (socks[i] >= 0) {
            uint32_t op = OP_STOP;
            write_all(socks[i], &op, sizeof(op));
            close(socks[i]);
        }
    }
    for (unsigned i=0;i<nproc;++i) {
        if (pids[i] > 0) {
            int st=0;
            waitpid(pids[i], &st, 0);
        }
    }

    return best;
}


