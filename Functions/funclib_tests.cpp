// funclib_tests.cpp

#include <gtest/gtest.h>
// #include <gmock/gmock.h>

#include "funclib.h"

using namespace funcs;

// ======================== 1. Фабрика: создание объектов и ошибки ========================

TEST(FunctionFactoryTests, CreatesIdentityAndExp) {
    FunctionFactory factory;

    auto ident = factory.Create("ident");
    ASSERT_NE(ident, nullptr);
    EXPECT_EQ(ident->ToString(), "x");
    EXPECT_DOUBLE_EQ((*ident)(5.0), 5.0);
    EXPECT_DOUBLE_EQ(ident->Derivative(123.0), 1.0);

    auto expf = factory.Create("exp");
    ASSERT_NE(expf, nullptr);
    EXPECT_EQ(expf->ToString(), "exp(x)");
    EXPECT_NEAR((*expf)(1.0), std::exp(1.0), 1e-12);
    EXPECT_NEAR(expf->Derivative(1.0), std::exp(1.0), 1e-12);
}

TEST(FunctionFactoryTests, CreatesConstAndPower) {
    FunctionFactory factory;

    auto c = factory.Create("const", 2.5);
    ASSERT_NE(c, nullptr);
    EXPECT_DOUBLE_EQ((*c)(0.0), 2.5);
    EXPECT_DOUBLE_EQ(c->Derivative(10.0), 0.0);

    std::ostringstream oss;
    oss << 2.5;
    EXPECT_EQ(c->ToString(), oss.str());

    auto p = factory.Create("power", 2.0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->ToString(), "x^2");
    EXPECT_DOUBLE_EQ((*p)(3.0), 9.0);
    EXPECT_DOUBLE_EQ(p->Derivative(3.0), 2.0 * 3.0);
}

TEST(FunctionFactoryTests, CreatesPolynomial) {
    FunctionFactory factory;

    // f(x) = 7 + 0*x + 3*x^2 + 15*x^3
    std::vector<double> coeffs{7.0, 0.0, 3.0, 15.0};
    auto poly = factory.Create("polynomial", coeffs);
    ASSERT_NE(poly, nullptr);

    double x = 2.0;
    double expected =
        7.0 + 0.0 * x + 3.0 * x * x + 15.0 * x * x * x;
    EXPECT_DOUBLE_EQ((*poly)(x), expected);

    // f'(x) = 0 + 0 + 2*3*x + 3*15*x^2
    double expected_der =
        2.0 * 3.0 * x + 3.0 * 15.0 * x * x;
    EXPECT_DOUBLE_EQ(poly->Derivative(x), expected_der);

    EXPECT_EQ(poly->ToString(), "7 + 3*x^2 + 15*x^3");
}

TEST(FunctionFactoryTests, ThrowsOnUnknownTypes) {
    FunctionFactory factory;

    EXPECT_THROW(factory.Create("unknown"), std::logic_error);
    EXPECT_THROW(factory.Create("oops", 1.0), std::logic_error);
    EXPECT_THROW(factory.Create("poly?", std::vector<double>{1.0, 2.0}), std::logic_error);
}

// ======================== 2. Базовые функции: значение, производная, строка ========================

TEST(BasicFunctionsTests, IdentityFunctionWorks) {
    IdentityFunction f;
    EXPECT_DOUBLE_EQ(f(0.0), 0.0);
    EXPECT_DOUBLE_EQ(f(10.0), 10.0);
    EXPECT_DOUBLE_EQ(f.Derivative(123.0), 1.0);
    EXPECT_EQ(f.ToString(), "x");
}

TEST(BasicFunctionsTests, ConstantFunctionWorks) {
    ConstantFunction f(3.14);
    EXPECT_DOUBLE_EQ(f(-100.0), 3.14);
    EXPECT_DOUBLE_EQ(f.Derivative(0.0), 0.0);

    std::ostringstream oss;
    oss << 3.14;
    EXPECT_EQ(f.ToString(), oss.str());
}

TEST(BasicFunctionsTests, PowerFunctionWorks) {
    PowerFunction f(3.0); // x^3
    EXPECT_DOUBLE_EQ(f(2.0), 8.0);
    EXPECT_DOUBLE_EQ(f.Derivative(2.0), 3.0 * 4.0); // 3*x^2
    EXPECT_EQ(f.ToString(), "x^3");
}

TEST(BasicFunctionsTests, ExpFunctionWorks) {
    ExpFunction f;
    double x = 1.5;
    EXPECT_NEAR(f(x), std::exp(x), 1e-12);
    EXPECT_NEAR(f.Derivative(x), std::exp(x), 1e-12);
    EXPECT_EQ(f.ToString(), "exp(x)");
}

TEST(BasicFunctionsTests, PolynomialFunctionWorks) {
    // f(x) = 1 + 2x + 3x^2
    PolynomialFunction f({1.0, 2.0, 3.0});
    double x = 5.0;

    double expected = 1.0 + 2.0 * x + 3.0 * x * x;
    double expected_der = 2.0 + 6.0 * x;

    EXPECT_DOUBLE_EQ(f(x), expected);
    EXPECT_DOUBLE_EQ(f.Derivative(x), expected_der);
    EXPECT_EQ(f.ToString(), "1 + 2*x^1 + 3*x^2");
}

// ======================== 3. Арифметические выражения (BinaryFunction + операторы) ========================

TEST(BinaryFunctionTests, MakeBinaryThrowsOnNullOperands) {
    FunctionPtr nonNull = std::make_shared<IdentityFunction>();
    FunctionPtr nullPtr;

    EXPECT_THROW(MakeBinary(nullPtr, BinaryOp::Add, nonNull), std::logic_error);
    EXPECT_THROW(MakeBinary(nonNull, BinaryOp::Sub, nullPtr), std::logic_error);
    EXPECT_THROW(MakeBinary(nullPtr, BinaryOp::Mul, nullPtr), std::logic_error);
}

TEST(BinaryFunctionTests, PointerOperatorsComputeCorrectly) {
    FunctionFactory factory;

    auto x = factory.Create("ident");       // x
    auto c = factory.Create("const", 2.0);  // 2

    auto sum  = x + c;   // x + 2
    auto diff = x - c;   // x - 2
    auto prod = x * c;   // 2x
    auto quot = x / c;   // x/2

    ASSERT_NE(sum, nullptr);
    ASSERT_NE(diff, nullptr);
    ASSERT_NE(prod, nullptr);
    ASSERT_NE(quot, nullptr);

    double xv = 4.0;

    EXPECT_DOUBLE_EQ((*sum)(xv),  xv + 2.0);
    EXPECT_DOUBLE_EQ((*diff)(xv), xv - 2.0);
    EXPECT_DOUBLE_EQ((*prod)(xv), xv * 2.0);
    EXPECT_DOUBLE_EQ((*quot)(xv), xv / 2.0);

    // производные:
    EXPECT_DOUBLE_EQ(sum->Derivative(xv),  1.0 + 0.0);
    EXPECT_DOUBLE_EQ(diff->Derivative(xv), 1.0 - 0.0);
    EXPECT_DOUBLE_EQ(prod->Derivative(xv), 1.0 * 2.0 + xv * 0.0);
    EXPECT_DOUBLE_EQ(quot->Derivative(xv), (1.0 * 2.0 - xv * 0.0) / (2.0 * 2.0));

    // строковое представление
    EXPECT_EQ(sum->ToString(),  "(x + 2)");
    EXPECT_EQ(diff->ToString(), "(x - 2)");
    EXPECT_EQ(prod->ToString(), "(x * 2)");
    EXPECT_EQ(quot->ToString(), "(x / 2)");
}

TEST(BinaryFunctionTests, ReferenceOperatorsProduceBinaryFunction) {
    ConstantFunction c1(3.0); // 3
    ConstantFunction c2(5.0); // 5

    BinaryFunction sum = c1 + c2; // (3 + 5)
    EXPECT_DOUBLE_EQ(sum(0.0), 8.0);
    EXPECT_DOUBLE_EQ(sum.Derivative(1.0), 0.0);

    std::string s = sum.ToString();
    EXPECT_NE(s.find("3"), std::string::npos);
    EXPECT_NE(s.find("+"), std::string::npos);
    EXPECT_NE(s.find("5"), std::string::npos);
}

// ======================== 4. Градиентный спуск: поиск корня ========================

TEST(GradientDescentTests, ThrowsOnNullPointer) {
    FunctionPtr f;
    EXPECT_THROW(GradientDescentRoot(f, 0.0, 0.1, 10), std::logic_error);
}

TEST(GradientDescentTests, FindsPositiveRootForQuadraticFromPointer) {
    FunctionFactory factory;

    auto x2 = factory.Create("power", 2.0);  // x^2
    auto c4 = factory.Create("const", 4.0);  // 4

    // f(x) = x^2 - 4
    FunctionPtr f = x2 - c4;
    ASSERT_NE(f, nullptr);

    double x0 = 10.0;          // старт справа
    double alpha = 0.01;
    std::size_t iters = 20000;

    double root = GradientDescentRoot(f, x0, alpha, iters);

    EXPECT_NEAR((*f)(root), 0.0, 1e-3);
    EXPECT_NEAR(root, 2.0, 1e-2);
}

TEST(GradientDescentTests, FindsNegativeRootForQuadraticFromReference) {
    FunctionFactory factory;

    auto x2 = factory.Create("power", 2.0);  // x^2
    auto c4 = factory.Create("const", 4.0);  // 4

    FunctionPtr f_ptr = x2 - c4;   // x^2 - 4
    ASSERT_NE(f_ptr, nullptr);
    TFunction& f = *f_ptr;         // безопасно: shared_ptr держит объект

    double x0 = -10.0;         // старт слева
    double alpha = 0.01;
    std::size_t iters = 20000;

    double root = GradientDescentRoot(f, x0, alpha, iters);

    EXPECT_NEAR(f(root), 0.0, 1e-3);
    EXPECT_NEAR(root, -2.0, 1e-2);
}
