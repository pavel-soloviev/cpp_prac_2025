#define DEBUG_MODE
#define DEBUG_RANDOM

#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <random>
// #include <memory>
#include <vector>
#include <map>
// #include <set>
#include <coroutine>
// #include <future>
#include <string>
#include <ranges>

#include "formatter.cpp"
#include "smart_ptr.cpp"
#include "logger.cpp"


namespace view = std::ranges::views;


struct promise;
struct Task {
    struct promise_type { 
        Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    std::coroutine_handle<promise_type> handle;

    Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    Task(const Task&) = delete;
    Task(Task&& other) noexcept : handle(other.handle) { other.handle = nullptr; } 
    ~Task() { if (handle) handle.destroy(); }
};


template<typename T>
void print(T value) {
    std::cout << Format(value) << std::endl;
}


template<typename T>
void simple_shuffle(T& container) {
    std::mt19937 g(rand());
    std::shuffle(container.begin(), container.end(), g);
}


struct NightActions {
    unsigned int players_num;
    bool doctors_action;
    unsigned int doctors_choice;
    bool commissar_action;
    unsigned int commissar_choice;
    bool journalist_action;
    std::pair<unsigned int, unsigned int> journalist_choice;
    bool samurai_action;
    unsigned int samurai_choice;
    std::vector<std::vector<unsigned int>> killers;

    explicit NightActions(unsigned int players_num_) : players_num(players_num_) {
        killers.resize(players_num);
        reset(); 
    }
    
    void reset() { 
        for (auto& k : killers) k.clear();
        commissar_action = doctors_action = journalist_action = samurai_action = false;
    }
};


class Player {
public:
    Player(size_t id_p) : id(id_p) {
        alive = true;
        is_real_player = false;
        is_boss = false;
    };
    virtual ~Player() {};
    virtual Task vote(std::vector<size_t> alive_ids, size_t& value) {
        if (is_real_player) vote_player(alive_ids, value);
        else vote_ai(alive_ids, value);
        co_return;
    }
    virtual Task act(std::vector<size_t> alive_ids,
                     NightActions& night_actions,
                     std::vector<SmartPtr<Player>> players) {
        if (is_real_player) act_player(alive_ids, night_actions, players);
        else act_ai(alive_ids, night_actions, players);
        co_return;
    }
    virtual void vote_ai(std::vector<size_t>& alive_ids, size_t& value) = 0;
    virtual void vote_player(std::vector<size_t>& alive_ids, size_t& value) {
        std::cout << "Voting! Choose which candidate to vote for from the following:" << std::endl;
        for (auto i : alive_ids) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
        size_t res;
        std::cin >> res;
        value = res;
        return;
    }
    virtual void act_ai(std::vector<size_t>& alive_ids,
                        NightActions& night_actions,
                        std::vector<SmartPtr<Player>> players) = 0;
    virtual void act_player(std::vector<size_t>& alive_ids,
                            NightActions& night_actions,
                            std::vector<SmartPtr<Player>> players) = 0;

    bool alive;
    bool is_real_player;
    bool is_boss;
    size_t id;
    std::vector<size_t> known_mafia{};
    std::string team;
    std::string role;
};

class Civilian : public Player {
public:
    Civilian(size_t id_p) : Player(id_p) {
        team = "civilian";
        role = "civilian";
    }
    virtual ~Civilian() {};

    virtual void vote_ai(std::vector<size_t>& alive_ids, size_t& value) override {
        simple_shuffle(alive_ids);
        for (size_t i = 0; i < alive_ids.size(); ++i) {
            if (alive_ids[i] != id) {
                value = alive_ids[i];
                return;
            }
        }
        if (!alive_ids.empty()) value = alive_ids.front();
        else value = id;
    }
    virtual void act_ai(std::vector<size_t>&, NightActions&, std::vector<SmartPtr<Player>>) override {
        return;
    }
    virtual void act_player(std::vector<size_t>&, NightActions&, std::vector<SmartPtr<Player>>) override {
        return;
    }
};

class Commissar : public Civilian {
    std::vector<size_t> already_checked;
    std::vector<size_t> known_civilian;
public:
    Commissar(size_t id_p) : Civilian(id_p) {
        already_checked = {id};
        known_civilian = {id};
        role = "commissar";
    }
    virtual ~Commissar() {};

    virtual void vote_ai(std::vector<size_t>& alive_ids, size_t& value) override {
        simple_shuffle(alive_ids);
        for (size_t i = 0; i < known_mafia.size(); i++) {
            if (std::find(alive_ids.begin(), alive_ids.end(), known_mafia[i]) != alive_ids.end()) {
                value = known_mafia[i];
                return;
            }
        }
        Civilian::vote_ai(alive_ids, value);
        return;
    }
    virtual void act_ai(std::vector<size_t>& alive_ids,
                        NightActions& night_actions,
                        std::vector<SmartPtr<Player>> players) override {
        for (size_t i = 0; i < known_mafia.size(); i++) {
            if (std::find(alive_ids.begin(), alive_ids.end(), known_mafia[i]) != alive_ids.end()) {
                night_actions.killers[known_mafia[i]].push_back(id);
                return;
            }
        }
        auto max_id = *std::max_element(alive_ids.begin(), alive_ids.end());
        for (size_t i = 0; i <= max_id; i++) {
            if (std::find(already_checked.begin(), already_checked.end(), i) == already_checked.end()) {
                already_checked.push_back(i);
                if (players[i]->team == "mafia") {
                    known_mafia.push_back(i);
                } else {
                    known_civilian.push_back(i);
                }
                night_actions.commissar_action = true;
                night_actions.commissar_choice = i;
                return;
            }
        }
        return;
    }
    virtual void act_player(std::vector<size_t>& alive_ids,
                            NightActions& night_actions,
                            std::vector<SmartPtr<Player>> players) override {
        while (true) {
            std::cout << "Choose an action: shoot (s) or check (c)." << std::endl;
            std::string choice;
            std::cin >> choice;
            std::cout << "Choose one of:" << std::endl;
            for (auto i : alive_ids) {
                std::cout << i << " ";
            }
            std::cout << std::endl;
            size_t shoot_check;
            std::cin >> shoot_check;
            if (choice == "shoot" || choice == "s") {
                night_actions.killers[shoot_check].push_back(id);
                return;
            } else if (choice == "check" || choice == "c") {
                std::cout << "Player " << shoot_check << " is "
                          << ((players[shoot_check]->team == "mafia") ? "mafia" : "not mafia") << std::endl;
                night_actions.commissar_action = true;
                night_actions.commissar_choice = shoot_check;
                return;
            } else {
                std::cout << "Incorrect action!" << std::endl;
            }
        }
    }
};


class Doctor : public Civilian {
    size_t last_heal;
public:
    Doctor(size_t id_p) : Civilian(id_p) {
        role = "doctor";
        last_heal = std::numeric_limits<size_t>::max();
    }
    virtual ~Doctor() {}

    virtual void act_ai(std::vector<size_t>& alive_ids,
                        NightActions& night_actions,
                        std::vector<SmartPtr<Player>>) override {
        simple_shuffle(alive_ids);
        for (size_t i = 0; i < alive_ids.size(); i++) {
            if (alive_ids[i] != last_heal) {
                night_actions.doctors_action = true;
                night_actions.doctors_choice = alive_ids[i];
                last_heal = alive_ids[i];
                return;
            }
        }
    }
    virtual void act_player(std::vector<size_t>& alive_ids,
                            NightActions& night_actions,
                            std::vector<SmartPtr<Player>>) override {
        std::cout << "Who do you want to heal?" << std::endl
                  << "Choose one of:" << std::endl;
        for (auto i : alive_ids) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
        std::cout << "But it shouldn't be the same player you healed last time." << std::endl;
        while (true) {
            size_t choice;
            std::cin >> choice;
            if (last_heal == choice) {
                std::cout << "You already healed this player last time." << std::endl;
                continue;
            } else {
                night_actions.doctors_action = true;
                night_actions.doctors_choice = choice;
                last_heal = choice;
                return;
            }
        }
    }
};


class Journalist : public Civilian {
public:
    Journalist(size_t id_p) : Civilian(id_p) {
        role = "journalist";
    }
    virtual ~Journalist() {}

    virtual void act_ai(std::vector<size_t>& alive_ids,
                        NightActions& night_actions,
                        std::vector<SmartPtr<Player>>) override {
        simple_shuffle(alive_ids);
        for (size_t i = 0; i < alive_ids.size() - 1; i++) {
            for (size_t j = i + 1; j < alive_ids.size(); j++) {
                if (i != id && j != id) {
                    night_actions.journalist_action = true;
                    night_actions.journalist_choice = {i, j};
                }
            }
        }
    }
    virtual void act_player(std::vector<size_t>& alive_ids,
                            NightActions& night_actions,
                            std::vector<SmartPtr<Player>> players) override {
        std::cout << "Choose two of:" << std::endl;
        for (auto i : alive_ids) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
        std::cout << "Who do you want to check?" << std::endl
                  << "But it shouldn't be you." << std::endl;
        while (true) {
            size_t i, j;
            std::cin >> i >> j;
            if (i != id && j != id) {
                if (players[i]->team == players[j]->team) {
                    std::cout << "These players are from the same team" << std::endl;
                } else {
                    std::cout << "These players are from different teams" << std::endl;
                }
                night_actions.journalist_action = true;
                night_actions.journalist_choice = {i, j};
                return;
            } else {
                std::cout << "It shouldn't be you!" << std::endl;
                continue;
            }
        }
    }
};


class Samurai : public Civilian {
public:
    Samurai(size_t id_p) : Civilian(id_p) {
        role = "samurai";
    }
    virtual ~Samurai() {}

    virtual void act_ai(std::vector<size_t>& alive_ids,
                        NightActions& night_actions,
                        std::vector<SmartPtr<Player>>) override {
        simple_shuffle(alive_ids);
        for (size_t i = 0; i < alive_ids.size(); i++) {
            night_actions.samurai_action = true;
            night_actions.samurai_choice = i;
        }
    }
    virtual void act_player(std::vector<size_t>& alive_ids,
                            NightActions& night_actions,
                            std::vector<SmartPtr<Player>>) override {
        std::cout << "Who do you want to protect?" << std::endl
                  << "Choose one of:" << std::endl;
        for (auto i : alive_ids) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
        size_t choice;
        std::cin >> choice;
        night_actions.samurai_action = true;
        night_actions.samurai_choice = choice;
    }
};


class Mafia : public Player {
public:
    Mafia(size_t id_p) : Player(id_p) {
        team = "mafia";
        role = "mafia";
    }
    virtual ~Mafia() {}

    virtual void vote_ai(std::vector<size_t>& alive_ids, size_t& value) override {
        simple_shuffle(alive_ids);
        size_t i = 0;
        while (i < alive_ids.size()) {
            if (std::find(known_mafia.begin(), known_mafia.end(), alive_ids[i]) == known_mafia.end()) {
                value = alive_ids[i];
                return;
            }
            i++;
        }
        value = alive_ids.front();
        return;
    }
    virtual void act_ai(std::vector<size_t>& alive_ids,
                        NightActions& night_actions,
                        std::vector<SmartPtr<Player>>) override {
        if (is_boss) {
            simple_shuffle(alive_ids);
            size_t i = 0;
            while (i < alive_ids.size()) {
                if (std::find(known_mafia.begin(), known_mafia.end(), alive_ids[i]) == known_mafia.end()) {
                    night_actions.killers[alive_ids[i]].push_back(id);
                    return;
                }
                i++;
            }
        }
    }
    virtual void act_player(std::vector<size_t>& alive_ids,
                            NightActions& night_actions,
                            std::vector<SmartPtr<Player>>) override {
        std::cout << "Mafia:" << std::endl;
        for (auto i : known_mafia) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
        if (is_boss) {
            std::cout << "You are a mafia boss. Who will the mafia kill on your orders?" << std::endl
                      << "Choose one of:" << std::endl;
            for (auto i : alive_ids) {
                std::cout << i << " ";
            }
            std::cout << std::endl;
            size_t choice;
            std::cin >> choice;
            night_actions.killers[choice].push_back(id);
        } else {
            std::cout << "You are not a mafia boss. Tonight you will not decide who the mafia will kill." << std::endl;
        }
    }
};


class Bull : public Mafia {
public:
    Bull(size_t id_p) : Mafia(id_p) {
        role = "bull";
    }
    virtual ~Bull() {}
};


class Maniac : public Player {
public:
    Maniac(size_t id_p) : Player(id_p) {
        team = "maniac";
        role = "maniac";
    }
    virtual ~Maniac() {};

    virtual void vote_ai(std::vector<size_t>& alive_ids, size_t& value) override {
        simple_shuffle(alive_ids);
        size_t i = 0;
        while (i < alive_ids.size()) {
            if (alive_ids[i] != id) {
                value = alive_ids[i];
                return;
            }
            i++;
        }
        value = alive_ids.front();
        return;
    }
    virtual void act_ai(std::vector<size_t>& alive_ids,
                        NightActions& night_actions,
                        std::vector<SmartPtr<Player>>) override {
        simple_shuffle(alive_ids);
        size_t i = 0;
        while (i < alive_ids.size()) {
            if (alive_ids[i] != id) {
                night_actions.killers[alive_ids[i]].push_back(id);
                return;
            }
            i++;
        }
    }
    virtual void act_player(std::vector<size_t>& alive_ids,
                            NightActions& night_actions,
                            std::vector<SmartPtr<Player>>) override {
        std::cout << "Choose one of:" << std::endl;
        for (auto i : alive_ids) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
        size_t choice;
        std::cin >> choice;
        night_actions.killers[choice].push_back(id);
    }
};



template<typename T>
concept PlayerConcept = requires(T player,
                                 std::vector<size_t> ids,
                                 size_t value,
                                 NightActions night_actions,
                                 std::vector<SmartPtr<Player>> players) {
    { player.vote(ids, value) } -> std::same_as<Task>;
    { player.act(ids, night_actions, players) } -> std::same_as<Task>;
};


template<PlayerConcept Player>
class Game {
public:
    Logger* logger;
    std::vector<SmartPtr<Player>> players {};
    unsigned int players_num;
    unsigned int mafia_modifier;
    std::vector<std::string> civilian_roles {"commissar", "doctor", "journalist", "samurai"};
    std::vector<std::string> mafia_roles {"bull"};
    size_t samurai_id;
    size_t bull_id;
// "civilian", "mafia", "maniac"

    explicit Game(unsigned int players_num_, unsigned int mafia_modifier_ = 3) :
        players_num(players_num_),
        mafia_modifier(mafia_modifier_)
    {}

    void add_random_roles(
            std::vector<std::string> roles,
            size_t limit,
            std::string default_role,
            std::vector<std::string>& result) {
        std::mt19937 g(rand());
        size_t i = 0;
        std::shuffle(roles.begin(), roles.end(), g);
        while (i < limit) {
            if (i < roles.size()) {
                result.push_back(roles[i]);
            } else {
                result.push_back(default_role);
            }
            i++;
        }
    }

    std::vector<std::string> get_random_roles() {
        unsigned int mafia_num = players_num / mafia_modifier;
        std::vector<std::string> random_roles;

        // for (auto a : buf) std::cout << a << " "; std::cout << std::endl;
        add_random_roles(mafia_roles, mafia_num, "mafia", random_roles);
        random_roles.push_back("maniac");
        add_random_roles(civilian_roles, players_num - mafia_num - 1, "civilian", random_roles);
        std::mt19937 g(rand());
        std::shuffle(random_roles.begin(), random_roles.end(), g);
        return random_roles;
    }

    void init_players(std::vector<std::string> roles) {
        players.clear();
        logger = new Logger{"init.log"};
        logger->log(Loglevel::INFO, "--- INIT ---");

        std::cout << "Do you want to play? Select the number of the player (from 0 to " << roles.size() - 1
                  << ") you want to play or -1 if you don't want to." << std::endl;
        int choice;
        std::cin >> choice;
        std::vector<size_t> mafia_buf{};
        for (size_t i = 0; i < roles.size(); i++) {
            auto role = roles[i];
            if (role == "civilian") {
                logger->log(Loglevel::INFO,
                        TPrettyPrinter().f("Player ").f(i).f(" is civilian").Str());
                players.push_back(SmartPtr<Player>(new Civilian{i}));
            } else if (role == "mafia") {
                logger->log(Loglevel::INFO,
                        TPrettyPrinter().f("Player ").f(i).f(" is mafia").Str());
                mafia_buf.push_back(i);
                players.push_back(SmartPtr<Player>(new Mafia{i}));
            } else if (role == "maniac") {
                logger->log(Loglevel::INFO,
                        TPrettyPrinter().f("Player ").f(i).f(" is maniac").Str());
                players.push_back(SmartPtr<Player>(new Maniac{i}));
            } else if (role == "bull") {
                logger->log(Loglevel::INFO,
                        TPrettyPrinter().f("Player ").f(i).f(" is bull").Str());
                players.push_back(SmartPtr<Player>(new Bull{i}));
                mafia_buf.push_back(i);
                bull_id = i;
            } else if (role == "commissar") {
                logger->log(Loglevel::INFO,
                        TPrettyPrinter().f("Player ").f(i).f(" is commissar").Str());
                players.push_back(SmartPtr<Player>(new Commissar{i}));
            } else if (role == "doctor") {
                logger->log(Loglevel::INFO,
                        TPrettyPrinter().f("Player ").f(i).f(" is doctor").Str());
                players.push_back(SmartPtr<Player>(new Doctor{i}));
            } else if (role == "journalist") {
                logger->log(Loglevel::INFO,
                        TPrettyPrinter().f("Player ").f(i).f(" is journalist").Str());
                players.push_back(SmartPtr<Player>(new Journalist{i}));
            } else if (role == "samurai") {
                logger->log(Loglevel::INFO,
                        TPrettyPrinter().f("Player ").f(i).f(" is samurai").Str());
                players.push_back(SmartPtr<Player>(new Samurai{i}));
                samurai_id = i;
            }
            if (std::cmp_equal(choice, i)) {
                players[i]->is_real_player = true;
                std::cout << "You are " << players[i]->role << "!" << std::endl;
                if (players[i]->role == "samurai") {
                    std::cout << "Wake up, Samurai! We have a city to burn!" << std::endl;
                }
            }
        }
        for (auto i : mafia_buf) {
            players[i]->known_mafia.insert(players[i]->known_mafia.end(), mafia_buf.begin(),
                    mafia_buf.end());
        }
        simple_shuffle(mafia_buf);
        players[mafia_buf[0]]->is_boss = true;
        delete logger;
    }

    void reelection_mafia_boss() {
        auto mafia = players |
                     view::filter([](auto p) { return p->alive; }) |
                     view::filter([](auto p) { return p->team == "mafia"; });
        if (!mafia.empty() &&
                (mafia | view::filter([](auto p) { return p->is_boss; })).empty()) {
            std::vector<SmartPtr<Player>> mafia_vec{mafia.begin(), mafia.end()};
            simple_shuffle(mafia_vec);
            mafia_vec[0]->is_boss = true;
        }
    }

    std::string game_status() {
        auto alives = players | view::filter([](auto p) { return p->alive; });
        size_t alive_count = std::ranges::distance(alives);
    
        logger->log(Loglevel::INFO, TPrettyPrinter().f("Alive players: ").f(alive_count).Str());
        if (alives.empty()) {
            // Everyone die
            return "draw";
        } else {
            auto mafia = alives | view::filter([](auto p) { return p->team == "mafia"; });
            if (mafia.empty()) {
                // Mafia die
                if ((alives | view::filter([](auto p) { return p->team == "maniac"; })).empty()) {
                    // Maniac die
                    return "civilian";
                } else {
                    // Maniac is alive
                    if (std::ranges::distance(alives) >= 3) {
                        return "continue";
                    } else {
                        return "maniac";
                    }
                }
            } else {
                // Mafia is alive
                if ((alives | view::filter([](auto p) { return p->team == "maniac"; })).empty()) {
                    // Maniac die
                    auto alives_num = std::ranges::distance(alives);
                    auto mafia_num = std::ranges::distance(mafia);
                    if (alives_num <= mafia_num * 2) {
                        return "mafia";
                    } else {
                        return "continue";
                    }
                } else {
                    // Maniac is alive
                    return "continue";
                }
            }
        }
    }


    void main_loop() {
        unsigned int day_number = 0;
        std::string cur_status = "";
        while (true) {
            logger = new Logger{"day_" + std::to_string(day_number) + ".log"};
            logger->log(Loglevel::INFO, "--- DAY " + std::to_string(day_number) + " ---");
            /////////////////////////////////////////////////////////////
            std::string alive_str = "Alive players: ";
            for (auto& player : players) {
                if (player->alive) {
                    alive_str += std::to_string(player->id) + " (" + player->role + "), ";
                }
            }
            if (alive_str.size() > 2) {
                alive_str.pop_back();
                alive_str.pop_back();
            }
            logger->log(Loglevel::INFO, alive_str);
            //////////////////////////////////////////////////////////
            day_vote();
            reelection_mafia_boss();
            cur_status = game_status();
            if (cur_status != "continue") {
                delete logger;
                break;
            }
            night_act();
            reelection_mafia_boss();
            cur_status = game_status();
            if (cur_status != "continue") {
                delete logger;
                break;
            }
            day_number++;
            delete logger;
        }
        logger = new Logger{"result.log"};
        if (cur_status == "draw") {
            logger->log(Loglevel::INFO, "No one survived the brutal shootouts and nighttime murders... The city died out...");
            logger->log(Loglevel::INFO, "DRAW!");
            logger->log(Loglevel::INFO, "Alives: ---");
            delete logger;
            return;
        }
        if (cur_status == "mafia") {
            logger->log(Loglevel::INFO, "The mafia has taken over this city and no one can stop them anymore. The mafia never dies!");
            logger->log(Loglevel::INFO, "MAFIA WIN");
        } else if (cur_status == "maniac") {
            logger->log(Loglevel::INFO, "Neither the mafia, nor the peaceful civilian, nor the sheriff could stop the crazy loner in the night...");
            logger->log(Loglevel::INFO, "MANIAC WINS");
        } else if (cur_status == "civilian") {
            logger->log(Loglevel::INFO, "The city sleeps peacefully. The citizens united and fought back against the mafia and the maniac.");
            logger->log(Loglevel::INFO, "CIVILIANS WIN");
        }
        logger->log(Loglevel::INFO, "Alives:");
        for (const auto& player : players) {
            if (player->alive) {
                logger->log(Loglevel::INFO, TPrettyPrinter().f("Player ").f(player->id).f(" - ").f(player->role).Str());
            }
        }
        delete logger;
    }

    void day_vote() {
        auto alives = players | view::filter([](auto p) { return p->alive; });
        auto alives_ids_rng = alives | view::transform([](auto p) { return p->id; });
        std::vector<size_t> alives_ids{alives_ids_rng.begin(), alives_ids_rng.end()};
        std::map<size_t, unsigned int> votes{};
        for (auto id: alives_ids) {
            votes[id] = 0;
        }

#ifndef DEBUG_RANDOM
        //// Using futures
        std::vector<std::future<void>> futures {};
        for (const auto& player : alives) {
            futures.push_back(
                std::async(std::launch::async, [&alives_ids, &player, &votes, this]() {
                    size_t value = 0;
                    player->vote(alives_ids, value);
                    votes[value]++;
                    logger->log(Loglevel::INFO,
                            TPrettyPrinter().f("Player ").f(player->id).f(" voted for player ").f(value).Str());
                })
            );
        }
        for (auto& future : futures) {
            future.get();
        }
        ////
#else
        //// Not using futures
        std::vector<SmartPtr<Player>> sh_alives{alives.begin(), alives.end()};
        simple_shuffle(sh_alives);
        for (const auto& player : sh_alives) {
            size_t value = 0;
            player->vote(alives_ids, value);
            votes[value]++;
            logger->log(Loglevel::INFO,
                    TPrettyPrinter().f("Player ").f(player->id).f(" voted for player ").f(value).Str());
        }
        ////
#endif
        auto key_val = std::max_element(votes.begin(), votes.end(),
                [](const std::pair<int, int>& p1, const std::pair<int, int>& p2) {
                    return p1.second < p2.second;
                });
        players[key_val->first]->alive = false;
        logger->log(Loglevel::INFO,
                TPrettyPrinter().f("Player ").f(key_val->first).f(" was executed by order of the city.").Str());
    }

    void night_act() {
        auto alives = players | view::filter([](auto p) { return p->alive; });
        auto alives_ids_rng = alives | view::transform([](auto p) { return p->id; });
        std::vector<size_t> alives_ids{alives_ids_rng.begin(), alives_ids_rng.end()};
        NightActions night_actions{players_num};
        night_actions.reset();

#ifndef DEBUG_RANDOM
        //// Using futures
        std::vector<std::future<void>> futures {};
        for (const auto& player : alives) {
            futures.push_back(
                std::async(std::launch::async, [&alives_ids, &player, &night_actions, this]() {
                    player->act(alives_ids, night_actions, players);
                })
            );
        }
        for (auto& future : futures) {
            future.get();
        }
        ////
#else
        //// Not using futures
        for (const auto& player : alives) {
            player->act(alives_ids, night_actions, players);
        }
        ////
#endif

        // print(night_actions.killers);

        // Bull avoid maniac's attempt to kill him
        for (size_t i = 0; i < night_actions.killers[bull_id].size(); i++) {
            auto killer_id = night_actions.killers[bull_id][i];
            if (players[killer_id]->role == "maniac") {
                night_actions.killers[bull_id].erase(night_actions.killers[bull_id].begin() + i);
                break;
            }
        }
        if (night_actions.commissar_action) {
            logger->log(Loglevel::INFO, TPrettyPrinter().f("Commissar checked player ").f(night_actions.commissar_choice).f(
                        ". He was a ").f(players[night_actions.commissar_choice]->role).Str());
        }
        if (night_actions.doctors_action) {
            logger->log(Loglevel::INFO, TPrettyPrinter().f("Doctor healed player ").f(night_actions.doctors_choice).Str());
            night_actions.killers[night_actions.doctors_choice].clear();
        }
        if (night_actions.journalist_action) {
            logger->log(Loglevel::INFO, TPrettyPrinter().f("Journalist checked players ").f(
                        night_actions.journalist_choice.first).f(" and ").f(
                        night_actions.journalist_choice.second).Str());
        }
        if (night_actions.samurai_action) {
            logger->log(Loglevel::INFO, TPrettyPrinter().f("Samurai protected player ").f(night_actions.samurai_choice).Str());
            auto& killers_list = night_actions.killers[night_actions.samurai_choice];
            if (!killers_list.empty()) {
                simple_shuffle(killers_list);
                night_actions.killers[killers_list[0]].push_back(samurai_id);
                night_actions.killers[samurai_id].push_back(samurai_id);
                night_actions.killers[night_actions.samurai_choice].clear();
            }
        }

        // print(night_actions.killers);

        for (size_t i = 0; i < players_num; i++) {
            if (!night_actions.killers[i].empty()) {
                auto log_message = TPrettyPrinter().f("Player ").f(i).f(" was killed by ").Str();
                for (size_t j = 0; j < night_actions.killers[i].size(); j++) {
                    log_message += players[night_actions.killers[i][j]]->role;
                    log_message += (j == night_actions.killers[i].size() - 1) ? "" : ", ";
                }
                players[i]->alive = false;
                logger->log(Loglevel::INFO, log_message);
            }
        }

    }
};


int main(void) {
    std::srand(static_cast<unsigned int>(time(NULL)));
    auto game = Game<Player>(10);
    auto roles = game.get_random_roles();
    game.init_players(roles);
    game.main_loop();
    return 0;
}