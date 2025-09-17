#pragma once
#include <vector>
#include <random>
#include <chrono>
#include "Roles.hpp"
#include "SmartPtr.hpp"

class Game {
private:
    std::vector<SmartPtr<Player>> players;

public:
    Game(int N);

    void start();
};
