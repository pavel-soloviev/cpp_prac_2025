#pragma once
#include "sa.hpp"

struct GeomTemp : ITempSchedule {
    explicit GeomTemp(double a=0.95): alpha(a) {}
    void reset(double T0) override { T=T0; }
    double current() const override { return T; }
    void next() override { T *= alpha; }
    double T{1.0}, alpha{0.95};
};