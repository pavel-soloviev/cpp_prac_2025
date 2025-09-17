#pragma once
#include <string>
#include <thread>
#include <iostream>
#include "SmartPtr.hpp"

class Player {
protected:
    std::string name;
    bool alive = true;
public:
    Player(const std::string& n) : name(n) {}
    virtual ~Player() = default;

    virtual void act() = 0;   // ночное действие
    virtual void vote() = 0;  // дневное голосование

    std::string getName() const { return name; }
    bool isAlive() const { return alive; }
    void kill() { alive = false; }
};
