#pragma once
#include "schedule.hpp"
#include <string>

bool load_instance_csv(const std::string& path, Instance& I);
bool save_instance_csv(const std::string& path, const Instance& I);
Instance generate_instance(uint32_t N, uint32_t M, uint32_t tmin, uint32_t tmax, uint64_t seed);