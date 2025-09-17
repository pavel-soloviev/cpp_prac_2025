#include "Game.hpp"
#include <iostream>
#include <thread>

Game::Game(int N) {
    // примитивная раздача ролей
    players.push_back(SmartPtr<Player>(new Mafia("Мафия1")));
    players.push_back(SmartPtr<Player>(new Doctor("Доктор")));
    players.push_back(SmartPtr<Player>(new Commissar("Комиссар")));
    players.push_back(SmartPtr<Player>(new Maniac("Маньяк")));

    for (int i = 4; i < N; i++) {
        players.push_back(SmartPtr<Player>(new Citizen("Житель" + std::to_string(i))));
    }
}

void Game::start() {
    std::cout << "Игра началась!\n";

    // Простейшая симуляция 2 раундов
    for (int round = 1; round <= 2; round++) {
        std::cout << "\nРаунд " << round << " — Ночь:\n";
        std::vector<std::thread> threads;
        for (auto& p : players) {
            if (p->isAlive()) {
                threads.emplace_back([&p]() { p->act(); });
            }
        }
        for (auto& t : threads) t.join();

        std::cout << "\nРаунд " << round << " — День:\n";
        for (auto& p : players) {
            if (p->isAlive()) p->vote();
        }
    }
}
