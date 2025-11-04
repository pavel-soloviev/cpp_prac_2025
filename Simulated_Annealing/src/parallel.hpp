#pragma once
#include "schedule.hpp"
#include "sa.hpp"
#include <string>

// Очень простой обмен: мастер слушает PF_UNIX, воркеры подключаются.
// Периодически шлют BEST; мастер хранит globalBest и рассылает его всем.
// Останов: 10 внешних итераций без улучшения (из задания).
// seq-режим: просто запускаем один локальный ИО без форка.
struct ParParams { uint32_t nproc{4}; uint32_t outerPatience{10}; std::string sockPath{"/tmp/sa.sock"}; };

int run_sequential(const Instance& I, SAParams sa, unsigned seedOverride,
                   std::unique_ptr<IMutation> mut,
                   std::unique_ptr<ITempSchedule> temp);

int run_parallel(const Instance& I, SAParams sa, ParParams pp,
                 std::unique_ptr<IMutation> mutProto,
                 std::unique_ptr<ITempSchedule> tempProto);