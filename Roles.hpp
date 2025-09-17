#pragma once
#include "Player.hpp"

class Citizen : public Player {
public:
    using Player::Player;
    void act() override {
        // гражданин ночью ничего не делает
    }
    void vote() override {
        std::cout << name << " голосует против случайного игрока.\n";
    }
};

class Mafia : public Player {
public:
    using Player::Player;
    void act() override {
        std::cout << name << " выбирает жертву ночью (мафия).\n";
    }
    void vote() override {
        std::cout << name << " голосует днем.\n";
    }
};

class Doctor : public Player {
public:
    using Player::Player;
    void act() override {
        std::cout << name << " выбирает, кого лечить.\n";
    }
    void vote() override {
        std::cout << name << " голосует днем.\n";
    }
};

class Commissar : public Player {
public:
    using Player::Player;
    void act() override {
        std::cout << name << " проверяет или стреляет в игрока.\n";
    }
    void vote() override {
        std::cout << name << " голосует днем.\n";
    }
};

class Maniac : public Player {
public:
    using Player::Player;
    void act() override {
        std::cout << name << " убивает любого ночью.\n";
    }
    void vote() override {
        std::cout << name << " голосует днем.\n";
    }
};
