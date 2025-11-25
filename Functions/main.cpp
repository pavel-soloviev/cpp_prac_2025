#include <iostream>
#include <vector>

#include "funclib.h"

int main() {
    using namespace funcs;

    FunctionFactory factory;

    // auto f = factory.Create("power", 2.0);   // x^2

    // // 7 + 3 * x^2 + 15 * x^3
    // auto g = factory.Create("polynomial", std::vector<double>{7.0, 0.0, 3.0, 15.0});

    // std::vector<FunctionPtr> cont;
    // cont.push_back(f);
    // cont.push_back(g);

    // for (const auto& ptr : cont) {
    //     std::cout
    //         << ptr->ToString()
    //         << " for x = 10 is "
    //         << (*ptr)(10.0)
    //         << '\n';
    // }

    // // Сложное выражение p(x) = x^2 + (7 + 3 x^2 + 15 x^3)
    // auto p = (*f) + (*g);

    // std::cout << "p(x) = " << p.ToString() << '\n';
    // std::cout << "p'(1) = " << p.Derivative(1.0) << '\n'; //2*1 + 0 + 6*1 + 45*1 = 53

    // // Пример градиентного спуска для f(x) = x^2 - 4
    // auto f2 = factory.Create("power", 2.0);
    // auto c4 = factory.Create("const", 4.0);
    // auto eq = f2 - c4;   // x^2 - 4

    // double root = GradientDescentRoot(eq, 10.0, 0.0001, 10000);
    // std::cout << "root ~ " << root << ", f(root) = " << (*eq)(root) << '\n';

    auto f1 = factory.Create("polynomial", std::vector<double>{2.0, 1.0});
    auto f2 = factory.Create("polynomial", std::vector<double>{-3.0, 1.0});
    auto f = f1 * f2;

    std::cout << (*f)(8) << std::endl;
    std::cout << f->Derivative(12) << std::endl;

    double root = GradientDescentRoot(f, 12, 0.001, 10000);
    std::cout << "root ~ " << root << std::endl;
    std::cout << f->ToString() << std::endl;


    return 0;
}
