#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <random>
#include <thread>
#include <vector>
#include <map>
#include <coroutine>
#include <string>
#include <ranges>
#include <chrono>
#include <thread>

#include "formatter.cpp"
#include "smart_ptr.cpp"
#include "logger.cpp"


namespace view = std::ranges::views;

struct Task {
    struct promise_type {
        size_t result;
        
        Task get_return_object() {
            return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(size_t value) {
            result = value;
        }
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
    ~Task() { if (handle) handle.destroy(); }

    void resume() {
        if (handle && !handle.done())
            handle.resume();
    }

    bool done() const {
        return !handle || handle.done();
    }
    
    size_t get_result() const {
        return handle.promise().result;
    }

    // Добавляем поддержку co_await для Task
    bool await_ready() const noexcept {
        return done(); // Если корутина завершена, готовы сразу
    }

    void await_suspend(std::coroutine_handle<> awaiting_coro) noexcept {
        resume();
    }

    size_t await_resume() const noexcept {
        return get_result(); // Возвращаем результат когда корутина завершена
    }
};

// Перемешаем
template <typename T>
void simple_shuffle(T &container)
{
    std::mt19937 g(rand());
    std::shuffle(container.begin(), container.end(), g);
}

struct NightActions
{
    // флаги действий для парсинга ночных действий

    unsigned int players_num;                                // Общее количество игроков
    bool doctors_action;                                     // Флаг действия доктора
    unsigned int doctors_choice;                             // Выбор доктора - кого лечим??
    bool commissar_action;                                   // Флаг действия комиссара
    unsigned int commissar_choice;                           // Выбор комиссара (кого проверить)
    bool journalist_action;                                  // Флаг действия журналиста
    std::pair<unsigned int, unsigned int> journalist_choice; // Пара игроков для проверки
    bool commisarfan_action;                                 // Флаг действия поелонницы коммисара
    unsigned int commisarfan_choice;                         // Кого проверяет поклонница коммисара

    // для каждого игрока храним список тех, кто на него напал
    std::vector<std::vector<unsigned int>> killers;

    explicit NightActions(unsigned int players_num_) : players_num(players_num_)
    {
        for (size_t i = 0; i < players_num; i++)
        {
            killers.push_back({}); // инициализация пустыми списками
        }

        commissar_action = false;
        doctors_action = false;
        journalist_action = false;
        commisarfan_action = false;
    }

    void reset()
    {

        for (size_t i = 0; i < killers.size(); i++)
        {
            killers[i].clear();
        }

        commissar_action = false;
        doctors_action = false;
        journalist_action = false;
        commisarfan_action = false;
    }
};

class Player
{
public:
    Player(size_t id_p) : id(id_p)
    {
        alive = true;           // Игрок жив при создании
        is_real_player = false; // По дефолту - компьютерный игрок
        is_boss = false;        // По дуфолту - не босс мафии
    };

    virtual ~Player() {};

    // голосование (возвращает корутину)
    virtual Task vote(std::vector<size_t> alive_ids)
    {
        if (is_real_player)
        {
            // Для реального игрока просто вызываем его метод
            co_return co_await vote_player(alive_ids);
        }
        else
        {
            // Для ИИ вызываем синхронный метод и возвращаем результат
            size_t result;
            vote_ai(alive_ids, result);
            co_return result;
        }
    }

    // ночное действие -> корутина
    virtual Task act(std::vector<size_t> alive_ids,
                     NightActions &night_actions,
                     std::vector<SmartPtr<Player>> players)
    {
        if (is_real_player)
        {
            // Для реального игрока просто вызываем его метод
            co_await act_player(alive_ids, night_actions, players);
            co_return 0;
        }
        else
        {
            // Для ИИ вызываем синхронный метод
            act_ai(alive_ids, night_actions, players);
            co_return 0;
        }
    }

    virtual void vote_ai(std::vector<size_t> &alive_ids, size_t &value) = 0;

    // Человек голосует
    virtual Task vote_player(std::vector<size_t> &alive_ids)
    {
        std::cout << "Пришло время дневного голосования. За кого вы голосуете?" << std::endl;

        for (auto i : alive_ids)
        {
            std::cout << i << " ";
        }
        std::cout << std::endl;
        
        size_t res;
        std::cin >> res;
        co_return res;
    }

    virtual void act_ai(std::vector<size_t> &alive_ids,
                        NightActions &night_actions,
                        std::vector<SmartPtr<Player>> players) = 0;

    virtual Task act_player(std::vector<size_t> &alive_ids,
                            NightActions &night_actions,
                            std::vector<SmartPtr<Player>> players) = 0;

    bool alive;          // жив?
    bool is_real_player; // это человек?
    bool is_boss;        // это босс мафии?
    size_t id;
    std::vector<size_t> known_mafia{}; // список мафий для типов "мафия" и "комиссар", так как они голосуют по особенному
    std::string team;                  // Команда ("civilian", "mafia", "maniac")
    std::string role;                  // Роль
};

class Civilian : public Player
{
public:
    Civilian(size_t id_p) : Player(id_p)
    {
        team = "civilian";
        role = "civilian";
    }
    virtual ~Civilian() {};

    virtual void vote_ai(std::vector<size_t> &alive_ids, size_t &value) override
    {
        simple_shuffle(alive_ids);
        size_t i = 0;
        while (i < alive_ids.size())
        {
            if (alive_ids[i] != id)
            {
                value = alive_ids[i];
                return;
            }
            i++;
        }
        value = 0;
        return;
    }

    // Мирный ночью спит
    virtual void act_ai(std::vector<size_t> &, NightActions &, std::vector<SmartPtr<Player>>) override
    {
        return;
    }

    virtual Task act_player(std::vector<size_t> &, NightActions &, std::vector<SmartPtr<Player>>) override
    {
        std::cout << "Вы мирный житель. Ночью вы спите." << std::endl;
        co_return 0;
    }
};

class Commissar : public Civilian
{
    std::vector<size_t> already_checked; // Уже проверенные игроки
    std::vector<size_t> known_civilian;  // Известные мирные жители
public:
    Commissar(size_t id_p) : Civilian(id_p)
    {
        already_checked = {id}; // Начинаем с проверки себя
        known_civilian = {id};  // Себя знаем как мирного
        role = "commissar";
    }
    virtual ~Commissar() {};

    virtual void vote_ai(std::vector<size_t> &alive_ids, size_t &value) override
    {
        simple_shuffle(alive_ids);

        // Сначала голосуем против известных мафиози, если они живы
        for (size_t i = 0; i < known_mafia.size(); i++)
        {
            if (std::find(alive_ids.begin(), alive_ids.end(), known_mafia[i]) != alive_ids.end())
            {
                value = known_mafia[i]; // Голосуем против мафиози
                return;
            }
        }
        // Если известных мафиози нет, голосуем как обычный житель
        Civilian::vote_ai(alive_ids, value);
        return;
    }

    virtual void act_ai(std::vector<size_t> &alive_ids,
                        NightActions &night_actions,
                        std::vector<SmartPtr<Player>> players) override
    {

        // Если есть известные мафиози среди живых - стреляем в них
        for (size_t i = 0; i < known_mafia.size(); i++)
        {
            if (std::find(alive_ids.begin(), alive_ids.end(), known_mafia[i]) != alive_ids.end())
            {
                night_actions.killers[known_mafia[i]].push_back(id); // читаетс как в игрока (i) [он же мафия] стрелял игрок id [он же комиссарр]
                return;
            }
        }

        // Иначе проверяем нового игрока

        for (const auto &i : alive_ids)
        {

            // Ищем еще не проверенного игрока
            if (std::find(already_checked.begin(), already_checked.end(), i) == already_checked.end())
            {
                already_checked.push_back(i); // Добавляем в проверенные

                // Запоминаем результат
                if (players[i]->team == "mafia")
                {
                    known_mafia.push_back(i);
                }
                else
                {
                    known_civilian.push_back(i);
                }

                night_actions.commissar_action = true;
                night_actions.commissar_choice = i;
                return;
            }
        }
        return;
    }

    virtual Task act_player(std::vector<size_t> &alive_ids,
                            NightActions &night_actions,
                            std::vector<SmartPtr<Player>> players) override
    {
        while (true)
        {
            std::cout << "Вы коммисар! Выберите действие: ВЫСТРЕЛИТЬ (s) или ПРОВЕРИТЬ (c)." << std::endl;
            std::string choice;
            std::cin >> choice;
            std::cout << "Выберите цель:" << std::endl;
            for (auto i : alive_ids)
            {
                std::cout << i << " ";
            }
            std::cout << std::endl;
            size_t shoot_check;
            std::cin >> shoot_check;

            if (choice == "shoot" || choice == "s")
            {
                night_actions.killers[shoot_check].push_back(id);
                co_return 0;
            }
            else if (choice == "check" || choice == "c")
            {
                std::cout << "Игрок " << shoot_check
                          << ((players[shoot_check]->team == "mafia") ? "мафия" : "не мафия") << std::endl;
                night_actions.commissar_action = true;
                night_actions.commissar_choice = shoot_check;
                co_return 0;
            }
            else
            {
                std::cout << "Неверное действие!" << std::endl;
            }
        }
    }
};

class Doctor : public Civilian
{
    size_t last_heal; // ID последнего вылеченного игрока
public:
    Doctor(size_t id_p) : Civilian(id_p)
    {
        role = "doctor";
        last_heal = std::numeric_limits<size_t>::max(); // сразу ставим максимально возможное значение
    }
    virtual ~Doctor() {}

    virtual void act_ai(std::vector<size_t> &alive_ids,
                        NightActions &night_actions,
                        std::vector<SmartPtr<Player>>) override
    {
        simple_shuffle(alive_ids);
        // Ищем игрока, которого не лечили в прошлую ночь и его же лечимы
        for (size_t i = 0; i < alive_ids.size(); i++)
        {
            if (alive_ids[i] != last_heal)
            {
                night_actions.doctors_action = true;
                night_actions.doctors_choice = alive_ids[i];
                last_heal = alive_ids[i];
                return;
            }
        }
    }

    virtual Task act_player(std::vector<size_t> &alive_ids,
                            NightActions &night_actions,
                            std::vector<SmartPtr<Player>>) override
    {
        std::cout << "Вы доктор! Ваша задача спасать игроков от смерти." << std::endl
                  << "Выберите кого хотите вылечить:" << std::endl;
        for (auto i : alive_ids)
        {
            std::cout << i << " ";
        }
        std::cout << std::endl;

        while (true)
        {
            size_t choice;
            std::cin >> choice;
            if (last_heal == choice)
            {
                std::cout << "Вы лечили этого игрока прошлой ночью!" << std::endl;
                continue;
            }
            else
            {
                night_actions.doctors_action = true;
                night_actions.doctors_choice = choice;
                last_heal = choice;
                co_return 0;
            }
        }
    }
};

class Journalist : public Civilian
{
public:
    Journalist(size_t id_p) : Civilian(id_p)
    {
        role = "journalist";
    }
    virtual ~Journalist() {}

    // проверяет двух случайных игроков
    virtual void act_ai(std::vector<size_t> &alive_ids,
                        NightActions &night_actions,
                        std::vector<SmartPtr<Player>>) override
    {
        simple_shuffle(alive_ids);
        // Перебираем все пары ЖИВЫХ игроков (кроме себя)
        for (const auto &i : alive_ids)
        {
            for (const auto &j : alive_ids)
            {
                if (i > j && i != id && j != id)
                {
                    night_actions.journalist_action = true;
                    night_actions.journalist_choice = {i, j};
                }
            }
        }
    }

    virtual Task act_player(std::vector<size_t> &alive_ids,
                            NightActions &night_actions,
                            std::vector<SmartPtr<Player>> players) override
    {
        std::cout << "Вы журналист! Выберите двух игроков для сранения их ролей:" << std::endl;
        for (auto i : alive_ids)
        {
            std::cout << i << " ";
        }

        std::cout << std::endl;

        while (true)
        {
            size_t first, second;
            std::cin >> first >> second;
            if (first >= players.size() || second >= players.size()) {
                std::cout << "Неверные ID игроков! Доступные ID: 0-" << players.size()-1 << std::endl;
                continue;
            }
            if (first != id && second != id)
            {
                // Проверяем, одной ли команды игроки
                if (players[first]->team == players[second]->team)
                {
                    std::cout << "Они на одной стороне)" << std::endl;
                }
                else
                {
                    std::cout << "Они играют за разные команды)" << std::endl;
                }
                night_actions.journalist_action = true;
                night_actions.journalist_choice = {first, second};
                co_return 0;
            }
            else
            {
                std::cout << "Нельзя сравнивать с собой)" << std::endl;
                continue;
            }
        }
    }
};


class Mafia : public Player
{
public:
    Mafia(size_t id_p) : Player(id_p)
    {
        team = "mafia";
        role = "mafia";
    }
    virtual ~Mafia() {}

    // голосует против НЕ мафов
    virtual void vote_ai(std::vector<size_t> &alive_ids, size_t &value) override
    {
        simple_shuffle(alive_ids);
        size_t i = 0;
        // Ищем первого игрока, который не мафия
        while (i < alive_ids.size())
        {
            // Очевидно, что мафия знает, кто мафия, поэтому может голосовать не в маию
            if (std::find(known_mafia.begin(), known_mafia.end(), alive_ids[i]) == known_mafia.end())
            {
                value = alive_ids[i];
                return;
            }
            i++;
        }
        value = 0;
        return;
    }

    // только босс мафии выбирает жертву
    virtual void act_ai(std::vector<size_t> &alive_ids,
                        NightActions &night_actions,
                        std::vector<SmartPtr<Player>>) override
    {
        if (is_boss)
        { // Только босс мафии совершает убийство
            simple_shuffle(alive_ids);
            size_t i = 0;
            // Ищем не-мафиози для убийства
            while (i < alive_ids.size())
            {
                if (std::find(known_mafia.begin(), known_mafia.end(), alive_ids[i]) == known_mafia.end())
                {
                    std::cout << "Мафия выбрала свою цель!" << std::endl;
                    night_actions.killers[alive_ids[i]].push_back(id);
                    return;
                }
                i++;
            }
        }
    }

    virtual Task act_player(std::vector<size_t> &alive_ids,
                            NightActions &night_actions,
                            std::vector<SmartPtr<Player>>) override
    {
        std::cout << "Клан мафии состоит из:" << std::endl;
        for (auto i : known_mafia)
        {
            std::cout << i << " ";
        }
        std::cout << std::endl;

        if (is_boss)
        {
            std::cout << "Вы Босс мафии. Вы решаете кого убить этой ночью" << std::endl
                      << "Выберите одного игрока из списка:" << std::endl;
            for (auto i : alive_ids)
            {
                std::cout << i << " ";
            }
            std::cout << std::endl;
            size_t choice;
            std::cin >> choice;
            night_actions.killers[choice].push_back(id);
        }
        else
        {
            std::cout << "Доверься Боссу. Он сделает верный выбор!" << std::endl;
        }
        co_return 0;
    }
};

class CommissarFan : public Civilian
{
    bool found_commissar;           // уже нашла комиссара?
    std::vector<size_t> checked;    // кого уже проверяла
public:
    CommissarFan(size_t id_p) : Civilian(id_p)
    {
        role = "commisarfan";
        found_commissar = false;
        checked.push_back(id); // не проверяет саму себя
    }

    virtual ~CommissarFan() {}

    virtual void act_ai(std::vector<size_t> &alive_ids,
                        NightActions &night_actions,
                        std::vector<SmartPtr<Player>> players) override
    {
        if (found_commissar)
            return; // если уже нашла комиссара — больше не действует

        simple_shuffle(alive_ids);
        for (const auto &i : alive_ids)
        {
            if (std::find(checked.begin(), checked.end(), i) == checked.end() && i != id)
            {
                checked.push_back(i);
                if (players[i]->role == "commissar")
                {
                    std::cout << "Поклонница комиссара нашла своего кумира!" << std::endl;
                    found_commissar = true;
                }
                else
                {
                    std::cout << "Поклонница комиссара проверила игрока " << i
                              << " — не комиссар." << std::endl;
                }
                night_actions.commisarfan_action = true;
                night_actions.commisarfan_choice = i;
                return;
            }
        }
        

    }

    virtual Task act_player(std::vector<size_t> &alive_ids,
                            NightActions &night_actions,
                            std::vector<SmartPtr<Player>> players) override
    {
        if (found_commissar)
        {
            std::cout << "Вы уже нашли комиссара. Спите спокойно этой ночью." << std::endl;
            co_return 0;
        }

        std::cout << "Вы — поклонница комиссара! Выберите игрока, чтобы проверить, не он ли комиссар:" << std::endl;
        for (const auto &i : alive_ids)
        {
            std::cout << i << " ";
        }
        std::cout << std::endl;

        size_t choice;
        std::cin >> choice;

        if (choice == id)
        {
            std::cout << "Вы не можете проверять себя!" << std::endl;
            co_return 0;
        }

        checked.push_back(choice);
        if (players[choice]->role == "commissar")
        {
            std::cout << "Игрок " << choice << " — комиссар! Вы успокоились и больше не будете проверять ночью." << std::endl;
            found_commissar = true;
        }
        else
        {
            std::cout << "Игрок " << choice << " — не комиссар." << std::endl;
        }
        night_actions.commisarfan_action = true;
        night_actions.commisarfan_choice = choice;
        co_return 0;
    }
};


class Bull : public Mafia
{
public:
    Bull(size_t id_p) : Mafia(id_p)
    {
        role = "bull"; // Специальная роль в мафии
    }
    virtual ~Bull() {}
};

class Maniac : public Player
{
public:
    Maniac(size_t id_p) : Player(id_p)
    {
        team = "maniac";
        role = "maniac";
    }
    virtual ~Maniac() {};

    // голосует против любого другого игрока
    virtual void vote_ai(std::vector<size_t> &alive_ids, size_t &value) override
    {
        simple_shuffle(alive_ids);
        size_t i = 0;
        while (i < alive_ids.size())
        {
            if (alive_ids[i] != id)
            {
                value = alive_ids[i];
                return;
            }
            i++;
        }
        value = 0;
        return;
    }

    // убивает случайного игрока
    virtual void act_ai(std::vector<size_t> &alive_ids,
                        NightActions &night_actions,
                        std::vector<SmartPtr<Player>> players) override
    {
        simple_shuffle(alive_ids);
        size_t i = 0;
        while (i < alive_ids.size())
        {
            if (alive_ids[i] != id && players[alive_ids[i]] -> role != "bull")
            { // Не может убить себя и быка
                std::cout << "Маньяк выбрал свою цель!" << std::endl;
                night_actions.killers[alive_ids[i]].push_back(id);
                return;
            }
            i++;
        }
    }

    // версия для реального игрока-маньяка
    virtual Task act_player(std::vector<size_t> &alive_ids,
                            NightActions &night_actions,
                            std::vector<SmartPtr<Player>> players) override
    {
        std::cout << "Ты маньяк, кого ты выберешь своей жертвой этой ночью?" << std::endl;
        for (auto i : alive_ids)
        {
            if (players[i] -> role != "bull")
                std::cout << i << " ";
        }
        std::cout << std::endl;
        size_t choice;
        std::cin >> choice;
        night_actions.killers[choice].push_back(id);
        co_return 0;
    }
};

template <typename T>
concept PlayerConcept = requires(T player,
                                 std::vector<size_t> ids,
                                 size_t value,
                                 NightActions night_actions,
                                 std::vector<SmartPtr<Player>> players) {
    // Проверяю, что у ведущего классы, для которых определены методы vote и act в объектах

    // тип T имеет  vote
    { player.vote(ids) } -> std::same_as<Task>;
    // тип T имеет  act
    { player.act(ids, night_actions, players) } -> std::same_as<Task>;
};

// Считывание ролей из конфигурационного файла
std::vector<std::string> parseRolesFromConfigSimple(const std::string& configPath) {
    std::vector<std::string> roles;
    
    // Проверка существования файла
    std::ifstream file(configPath);
    if (!file.is_open()) {
        throw std::runtime_error("Config file not found: " + configPath);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue; // Пропускаем строки без разделителя
        }
        
        std::string roleName = line.substr(0, colonPos);
        roleName.erase(0, roleName.find_first_not_of(" \t"));
        roleName.erase(roleName.find_last_not_of(" \t") + 1);
        
        std::string countStr = line.substr(colonPos + 1);
        countStr.erase(0, countStr.find_first_not_of(" \t"));
        countStr.erase(countStr.find_last_not_of(" \t") + 1);
        
        try {
            int count = std::stoi(countStr);
            
            for (int i = 0; i < count; ++i) {
                roles.push_back(roleName);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Warning: Invalid number format for role '" << roleName << "': " << countStr << std::endl;
        }
    }
    
    file.close();
    return roles;
}

// Класс ведущего игру
template <PlayerConcept Player>
class Game
{
public:
    Logger *logger;
    std::vector<SmartPtr<Player>> players{}; // массив игроков
    unsigned int players_num;                      // количество игроков
    unsigned int mafia_modifier;                   // Модификатор для расчета количества мафии

    // Доступные роли
    std::vector<std::string> civilian_roles{"commissar", "doctor", "journalist", "commisarfan"};
    std::vector<std::string> mafia_roles{"bull"};

    // size_t commisarfan_id = std::numeric_limits<size_t>::max();
    size_t bull_id = std::numeric_limits<size_t>::max();

    explicit Game(unsigned int players_num_, unsigned int mafia_modifier_ = 3) : players_num(players_num_),
                                                                                 mafia_modifier(mafia_modifier_)
    {
    }

    // добавлениt случайных ролей
    void add_random_roles(
        std::vector<std::string> roles,
        size_t limit,
        std::string default_role,
        std::vector<std::string> &result)
    {
        std::mt19937 g(rand());
        size_t i = 0;
        std::shuffle(roles.begin(), roles.end(), g);
        while (i < limit)
        {
            if (i < roles.size())
            {
                result.push_back(roles[i]); // специальную роль
            }
            else
            {
                result.push_back(default_role); // роль по умолчанию
            }
            i++;
        }
    }

    // Расчёт количества ролей каждого класса
    std::vector<std::string> get_random_roles()
    {
        unsigned int mafia_num = players_num / mafia_modifier; // Расчет количества мафии
        std::vector<std::string> random_roles;

        add_random_roles(mafia_roles, mafia_num, "mafia", random_roles);
        random_roles.push_back("maniac");
        add_random_roles(civilian_roles, players_num - mafia_num - 1, "civilian", random_roles);

        std::mt19937 g(rand());
        std::shuffle(random_roles.begin(), random_roles.end(), g);
        return random_roles;
    }

    // Инициализация игроков с заданными ролями
    void init_players(std::vector<std::string> roles)
    {
        players.clear();
        logger = new Logger{"start.log"};
        logger->log(Loglevel::INFO, "===================================== Игра началась!!! =====================================");

        std::cout << "=====================================" << " Доступные роли " << "=====================================" << std::endl;
        for (size_t i = 0; i < roles.size(); i++)
        {
            std::cout << i << ": " << roles[i] << std::endl;
        }

        // Запрос у пользователя, хочет ли он играть
        std::cout << "Если хочешь сыграть в игру выбери роль (от 0 до " << roles.size() - 1
                  << ") или -1, если хочешь просто понаблюдать." << std::endl;

        int choice;
        std::cin >> choice;
        std::string human_role = "";

        if (choice != -1)
        {
            //std::cout << "-1 INPUT" << std::endl;
            human_role = roles[choice];

            // Это нужно, чтобы когда реальный игрок играл за кого-то он не знал, кто есть кто
            simple_shuffle(roles);
        }

        std::vector<size_t> mafia_buf{}; //  буфер для ID мафиози

        size_t i = 0;

        // for (auto role: roles) {
        //     std::cout << role;
        // }
        // Создание по распределениею
        for (const auto &role : roles)
        {
            //std::cout << "TEEEEST" << std::endl;
            //auto role = roles[i];
            if (role == "civilian")
            {
                logger->log(Loglevel::INFO,
                            TPrettyPrinter().f("Player ").f(i).f(" is civilian").Str());
                players.push_back(SmartPtr<Player>(new Civilian{i}));
            }
            else if (role == "mafia")
            {
                logger->log(Loglevel::INFO,
                            TPrettyPrinter().f("Player ").f(i).f(" is mafia").Str());
                mafia_buf.push_back(i);
                players.push_back(SmartPtr<Player>(new Mafia{i}));
            }
            else if (role == "maniac")
            {
                logger->log(Loglevel::INFO,
                            TPrettyPrinter().f("Player ").f(i).f(" is maniac").Str());
                players.push_back(SmartPtr<Player>(new Maniac{i}));
            }
            else if (role == "bull")
            {
                logger->log(Loglevel::INFO,
                            TPrettyPrinter().f("Player ").f(i).f(" is bull").Str());
                players.push_back(SmartPtr<Player>(new Bull{i}));
                mafia_buf.push_back(i);
                bull_id = i;
            }
            else if (role == "commissar")
            {
                logger->log(Loglevel::INFO,
                            TPrettyPrinter().f("Player ").f(i).f(" is commissar").Str());
                players.push_back(SmartPtr<Player>(new Commissar{i}));
            }
            else if (role == "doctor")
            {
                logger->log(Loglevel::INFO,
                            TPrettyPrinter().f("Player ").f(i).f(" is doctor").Str());
                players.push_back(SmartPtr<Player>(new Doctor{i}));
            }
            else if (role == "journalist")
            {
                logger->log(Loglevel::INFO,
                            TPrettyPrinter().f("Player ").f(i).f(" is journalist").Str());
                players.push_back(SmartPtr<Player>(new Journalist{i}));
            }
            else if (role == "commisarfan")
            {
                logger->log(Loglevel::INFO,
                            TPrettyPrinter().f("Player ").f(i).f(" is commisarfan").Str());
                players.push_back(SmartPtr<Player>(new CommissarFan{i}));
                // commisarfan_id = i;
            }

           ++i;
        }

        // В какую-то мафию или какого-то мирного или другую роль помечаем живым игроком
        for (const auto &pl : players)
        {
            if (human_role == pl->role)
            {
                pl->is_real_player = true;
                std::cout << "Запомните свой ID, он может понадобиться. ID = " << std::to_string(pl->id) << std::endl;
                break;
            }
        }

        // Для мафий в их списки известных мафиози добавляем других мафов (нужно в ночных действиях)
        for (auto i : mafia_buf)
        {
            players[i]->known_mafia.insert(players[i]->known_mafia.end(), mafia_buf.begin(),
                                           mafia_buf.end());
        }

        // босс случаен
        simple_shuffle(mafia_buf);
        players[mafia_buf[0]]->is_boss = true;
        delete logger;
    }

    // если босс убит -- перевыбор
    void reelection_mafia_boss()
    {
        // ranes для фильтрации живых
        auto mafia = players |
                     view::filter([](auto p)
                                  { return p->alive; }) |
                     view::filter([](auto p)
                                  { return p->team == "mafia"; });

        // Если есть живые мафиози, но нет босса - выбираем нового
        if (!mafia.empty() &&
            (mafia | view::filter([](auto p)
                                  { return p->is_boss; }))
                .empty())
        {
            std::vector<SmartPtr<Player>> mafia_vec{mafia.begin(), mafia.end()};
            simple_shuffle(mafia_vec);
            mafia_vec[0]->is_boss = true;
        }
    }

    // текущий статус игры
    std::string game_status()
    {
        auto alives = players | view::filter([](auto p)
                                             { return p->alive; }); // живые к данному моменту

        if (alives.empty())
        {
            // ничья
            return "draw";
        }
        else
        {
            // Фильтруем живых мафиози
            auto mafia = alives | view::filter([](auto p)
                                               { return p->team == "mafia"; });

            if (mafia.empty())
            {
                // Мафия мертва
                auto maniac_alive = alives | view::filter([](auto p)
                                                          { return p->team == "maniac"; });
                if (maniac_alive.empty())
                {
                    // Маньяк тоже мертв, значит, мирные победили
                    return "civilian";
                }
                else
                {
                    // Маньяк жив. Проверим, победил ли он??
                    if (std::ranges::distance(alives) >= 3)
                    {
                        return "continue"; // Игра продолжается
                    }
                    else
                    {
                        return "maniac"; // Победа маньяка
                    }
                }
            }
            else
            {
                // Мафия жива
                auto maniac_alive = alives | view::filter([](auto p)
                                                          { return p->team == "maniac"; });
                if (maniac_alive.empty())
                {
                    // Маньяк мертв - проверяем условия победы мафии
                    auto alives_num = std::ranges::distance(alives);
                    auto mafia_num = std::ranges::distance(mafia);
                    if (alives_num <= mafia_num * 2)
                    {
                        return "mafia"; // Победа мафии
                    }
                    else
                    {
                        return "continue"; // Игра продолжается
                    }
                }
                else
                {
                    // Маньяк жив, значит, ещё не победа мафии
                    return "continue";
                }
            }
        }
    }

    void main_loop()
    {
        unsigned int day_number = 0;
        std::string cur_status = "";

        while (true)
        {

            logger = new Logger{"day_" + std::to_string(day_number) + ".log"};
            logger->log(Loglevel::INFO, "==== DAY " + std::to_string(day_number) + " ====");
            std::cout << std::endl;
            std::cout << "===================================== ДЕНЬ" << std::to_string(day_number) << " =====================================" << std::endl;

            std::cout << "Эти игроки до сих пор живы: ";
            for (const auto &pl : players)
            {
                if (pl->alive)
                {
                    std::cout << pl->id << " ";
                }
            }
            std::cout << std::endl;

            std::cout << "===================================== !ГОЛОСОВАНИЕ! =====================================" << std::endl;

            // голосуем
            day_vote();
            // выбиарем нового босса мафии
            reelection_mafia_boss();
            // статус игры
            cur_status = game_status();
            if (cur_status != "continue")
            {
                delete logger;
                break;
            }

            // ночь
            std::cout << std::endl;
            std::cout << "===================================== НОЧЬ" << std::to_string(day_number) << " =====================================" << std::endl;

            night_act();
            reelection_mafia_boss();
            cur_status = game_status();
            if (cur_status != "continue")
            {
                delete logger;
                break;
            }

            day_number++;
            delete logger;
        }

        // Итоговый ресультат
        logger = new Logger{"result.log"};
        if (cur_status == "draw")
        {
            logger->log(Loglevel::INFO, "This city is terrible, I highly recommend not living here. It's simply unbelievable, they shot each other in just a couple of nights, a city of corpses.");
            logger->log(Loglevel::INFO, "DRAW!");
            logger->log(Loglevel::INFO, "Alives: they are all dead...");
        }
        else if (cur_status == "mafia")
        {
            logger->log(Loglevel::INFO, "This city is mired in crime, the mafia has gained the upper hand and now controls the city.");
            logger->log(Loglevel::INFO, "MAFIA WIN");
            std::cout << "===================================== Мафия победила =====================================" << std::endl;
        }
        else if (cur_status == "maniac")
        {
            logger->log(Loglevel::INFO, "He escaped from the mental hospital and systematically and gradually killed every inhabitant of the city. Neither the mafia nor the sheriff could stop him. He is a maniac.");
            logger->log(Loglevel::INFO, "MANIAC WINS");
            std::cout << "===================================== Маньяк победил =====================================" << std::endl;
        }
        else if (cur_status == "civilian")
        {
            logger->log(Loglevel::INFO, "This city is absolutely safe to live in. Peaceful residents organized a successful democratic election, rooted out a mafia clan, and identified a serial killer.");
            logger->log(Loglevel::INFO, "CIVILIANS WIN");
            std::cout << "===================================== Мирные жители победили =====================================" << std::endl;
        }

        // Записываем выживших игроков
        logger->log(Loglevel::INFO, "Alives:");
        for (const auto &player : players)
        {
            if (player->alive)
            {
                logger->log(Loglevel::INFO, TPrettyPrinter().f("Player ").f(player->id).f(" - ").f(player->role).Str());
            }
        }
        delete logger;
    }

    void day_vote()
    {
        auto alives = players | view::filter([](auto p) { return p->alive; });
        auto alives_ids_rng = alives | view::transform([](auto p) { return p->id; });
        std::vector<size_t> alives_ids{alives_ids_rng.begin(), alives_ids_rng.end()};

        std::map<size_t, unsigned int> votes{};
        for (auto id : alives_ids)
        {
            votes[id] = 0;
        }

        // Вектор для хранения задач голосования
        std::vector<std::pair<SmartPtr<Player>, Task>> voting_tasks;
        
        // Запускаем все корутины голосования
        for (const auto &player : players)
        {
            if (player->alive)
            {
                auto task = player->vote(alives_ids);
                voting_tasks.emplace_back(player, std::move(task));
            }
        }

        // Обрабатываем корутины пока все не завершатся
        bool all_done = false;
        bool real_player_message_shown = false; // Флаг для отслеживания показа сообщения
        
        while (!all_done)
        {
            all_done = true;
            
            for (auto& [player, task] : voting_tasks)
            {
                if (!task.done())
                {
                    if (player->is_real_player && !real_player_message_shown)
                    {
                        // Показываем сообщение только один раз за цикл
                        std::cout << ">>> Ожидаем голос реального игрока " << player->id << "..." << std::endl;
                        real_player_message_shown = true;
                    }
                    
                    task.resume();
                    all_done = false;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Собираем результаты
        for (auto& [player, task] : voting_tasks)
        {
            size_t vote_result = task.get_result();
            votes[vote_result]++;
            logger->log(Loglevel::INFO,
                        TPrettyPrinter().f("Player ").f(player->id).f(" voted for player ").f(vote_result).Str());
            std::cout << TPrettyPrinter().f("Игрок ").f(player->id).f(" голосует за игрока ").f(vote_result).Str() << std::endl;
        }

        auto key_val = std::max_element(votes.begin(), votes.end(),
                                        [](const std::pair<int, int> &p1, const std::pair<int, int> &p2)
                                        {
                                            return p1.second < p2.second;
                                        });

        players[key_val->first]->alive = false;
        logger->log(Loglevel::INFO,
                    TPrettyPrinter().f("Player ").f(key_val->first).f(" was hanged in the city square by peaceful means of democracy and voting.").Str());
        std::cout << TPrettyPrinter().f("Игрок ").f(key_val->first).f(" убит. За него проголосовало наибольшее число граждан.").Str() << std::endl
                << std::endl;
    }

    void night_act()
    {
        auto alives = players | view::filter([](auto p) { return p->alive; });
        auto alives_ids_rng = alives | view::transform([](auto p) { return p->id; });
        std::vector<size_t> alives_ids{alives_ids_rng.begin(), alives_ids_rng.end()};

        NightActions night_actions{players_num};
        night_actions.reset();

        // Вектор для хранения задач ночных действий
        std::vector<std::pair<SmartPtr<Player>, Task>> action_tasks;
        
        // Запускаем все корутины ночных действий
        for (const auto &player : alives)
        {
            auto task = player->act(alives_ids, night_actions, players);
            action_tasks.emplace_back(player, std::move(task));
        }

        // Обрабатываем корутины пока все не завершатся
        bool real_player_message_shown = false;
        bool all_done = false;
        while (!all_done)
        {
            all_done = true;
            
            for (auto& [player, task] : action_tasks)
            {
                if (!task.done())
                {
                    if (player->is_real_player && !real_player_message_shown)
                    {
                        std::cout << ">>> Ожидаем ночное действие реального игрока " << player->id << "..." << std::endl;
                        real_player_message_shown = true;
                    }
                    task.resume();
                    all_done = false;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Бык -спец маф, которого не может завалить маньяк
        for (size_t i = 0; i < night_actions.killers[bull_id].size(); i++)
        {
            auto killer_id = night_actions.killers[bull_id][i];
            if (players[killer_id]->role == "maniac")
            {
                night_actions.killers[bull_id].erase(night_actions.killers[bull_id].begin() + i);
                break;
            }
        }

        // обработка ночных действий

        if (night_actions.commissar_action)
        {
            logger->log(Loglevel::INFO, TPrettyPrinter().f("Commissar checked player ").f(night_actions.commissar_choice).f(". He was a ").f(players[night_actions.commissar_choice]->role).Str());
            std::cout << "Этой ночью коммисар проверил игрока " << std::to_string(night_actions.commissar_choice) << std::endl;
        }

        if (night_actions.doctors_action)
        {
            logger->log(Loglevel::INFO, TPrettyPrinter().f("Doctor healed player ").f(night_actions.doctors_choice).Str());
            // Лечение снимает все атаки с игрока
            night_actions.killers[night_actions.doctors_choice].clear();
            std::cout << "Этой ночью доктор вылечил игрока " << std::to_string(night_actions.doctors_choice) << std::endl;
        }

        if (night_actions.journalist_action)
        {
            logger->log(Loglevel::INFO, TPrettyPrinter().f("Journalist checked players ").f(night_actions.journalist_choice.first).f(" and ").f(night_actions.journalist_choice.second).Str());
            std::cout << "Этой ночью журналист сравнил игроков " << std::to_string(night_actions.journalist_choice.first) << " and " << std::to_string(night_actions.journalist_choice.second) << std::endl;
        }

        if (night_actions.commisarfan_action)
        {
            logger->log(Loglevel::INFO, TPrettyPrinter().f("Commisarfan checked player ").f(night_actions.commisarfan_choice).Str());
            std::cout << "Этой ночью поклонница коммисара проверила игрока " << std::to_string(night_actions.commisarfan_choice) << std::endl;
        }
        // обработка убийстви
        for (size_t i = 0; i < players_num; i++)
        {
            if (!night_actions.killers[i].empty())
            {
                auto log_message = TPrettyPrinter().f("Player ").f(i).f(" was killed by ").Str();
                std::string msg = "игрок " + std::to_string(i) + " был убит ";
                for (size_t j = 0; j < night_actions.killers[i].size(); j++)
                {
                    log_message += players[night_actions.killers[i][j]]->role;
                    msg += players[night_actions.killers[i][j]]->role;

                    log_message += (j == night_actions.killers[i].size() - 1) ? "" : ", ";
                    msg += (j == night_actions.killers[i].size() - 1) ? "" : ", ";
                }
                // убиваем
                players[i]->alive = false;

                logger->log(Loglevel::INFO, log_message);
                std::cout << "Этой ночью " << msg << std::endl;
            }
        }
    }
};

int main(void)
{
    // int i = 7;
    // std::time_t result = std::time(nullptr);
    //  std::srand((int) result);

    std::srand(5);
    std::cout << "========== SRAND = " << 5 << " ==========" << std::endl;

    std::string check;
    std::cout << "Выберите как будут определятся роли в игре. Автоматически (g) или через конфигурационный файл (f)" << std::endl;
    std::cin >> check;
    std::vector<std::string> roles;

    // default инициализация
    auto game = Game<Player>(1); 



    if (check == "g")
    {
        unsigned int n;
        std::cout << "Сколько всего будет игроков?" << std::endl;
        std::cin >> n;

        game = Game<Player>(n);
        roles = game.get_random_roles();
    }
    else
    {
        roles = parseRolesFromConfigSimple("./config.yaml");
        game = Game<Player>(roles.size());
    }

    // инициализация игроков
    game.init_players(roles);

    game.main_loop();

    return 0;
}