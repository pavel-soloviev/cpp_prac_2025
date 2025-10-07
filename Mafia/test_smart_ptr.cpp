#include <iostream>
#include <cassert>
#include "smart_ptr.cpp"

struct Dummy {
    int value;
    Dummy(int v = 0) : value(v) { std::cout << "Dummy(" << value << ") constructed\n"; }
    ~Dummy() { std::cout << "Dummy(" << value << ") destroyed\n"; }
};

int main() {
    std::cout << "=== TEST 1: Basic creation ===\n";
    {
        SmartPtr<Dummy> p1(new Dummy(42));
        assert(p1.use_count() == 1);
        assert(p1->value == 42);
        std::cout << "OK\n";
    } // p1 выходит из области видимости — объект должен быть уничтожен

    std::cout << "=== TEST 2: Copy semantics ===\n";
    {
        SmartPtr<Dummy> p1(new Dummy(10));
        SmartPtr<Dummy> p2 = p1;
        assert(p1.use_count() == 2);
        assert(p2.use_count() == 2);
        assert(p1.get() == p2.get());
        std::cout << "OK\n";
    } // оба выходят из области видимости — объект уничтожается

    std::cout << "=== TEST 3: Assignment operator ===\n";
    {
        SmartPtr<Dummy> p1(new Dummy(1));
        SmartPtr<Dummy> p2(new Dummy(2));
        p2 = p1;
        assert(p1.use_count() == 2);
        assert(p2.use_count() == 2);
        assert(p1.get() == p2.get());
        std::cout << "OK\n";
    } // объект 1 уничтожен при перезаписи p2, потом и общий объект — в конце

    std::cout << "=== TEST 4: reset() ===\n";
    {
        SmartPtr<Dummy> p(new Dummy(100));
        assert(p.use_count() == 1);
        p.reset(new Dummy(200));
        assert(p.use_count() == 1);
        assert(p->value == 200);
        p.reset();
        assert(p.get() == nullptr);
        std::cout << "OK\n";
    }

    std::cout << "=== TEST 5: swap() ===\n";
    {
        SmartPtr<Dummy> p1(new Dummy(1));
        SmartPtr<Dummy> p2(new Dummy(2));
        p1.swap(p2);
        assert(p1->value == 2);
        assert(p2->value == 1);
        std::cout << "OK\n";
    }

    std::cout << "=== TEST 6: use_count() across copies ===\n";
    {
        SmartPtr<Dummy> a(new Dummy(99));
        {
            SmartPtr<Dummy> b = a;
            SmartPtr<Dummy> c = b;
            assert(a.use_count() == 3);
        }
        assert(a.use_count() == 1);
        std::cout << "OK\n";
    }

    std::cout << "=== TEST 7: Comparison operators ===\n";
    {
        SmartPtr<Dummy> a(new Dummy(10));
        SmartPtr<Dummy> b = a;
        SmartPtr<Dummy> c(new Dummy(20));
        assert(a == b);
        assert(a != c);
        assert((a < c) || (a > c)); // сравнение по адресам
        std::cout << "OK\n";
    }

    std::cout << "=== TEST 8: make_smart_ptr() ===\n";
    {
        auto p = make_smart_ptr<Dummy>(777);
        assert(p->value == 777);
        assert(p.use_count() == 1);
        std::cout << "OK\n";
    }

    std::cout << "=== ALL TESTS PASSED SUCCESSFULLY ===\n";
    return 0;
}
