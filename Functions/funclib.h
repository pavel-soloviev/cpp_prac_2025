#pragma once

#include <cmath>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace funcs {


class TFunction;
using FunctionPtr = std::shared_ptr<TFunction>;

// абстрактный класс для функций
class TFunction {
public:
    virtual ~TFunction() = default;

    virtual double operator()(double x) const = 0;

    virtual double Derivative(double x) const = 0;

    virtual std::string ToString() const = 0;

    virtual FunctionPtr Clone() const = 0;
};

//Базовые функции

// тождественная: f(x) = x
class IdentityFunction : public TFunction {
public:
    double operator()(double x) const override {
        return x;
    }

    double Derivative(double) const override {
        return 1.0;
    }

    std::string ToString() const override {
        return "x";
    }

    FunctionPtr Clone() const override {
        return std::make_shared<IdentityFunction>(*this);
    }
};

// константа: f(x) = c
class ConstantFunction : public TFunction {
public:
    explicit ConstantFunction(double c) : value_(c) {}

    double operator()(double) const override {
        return value_;
    }

    double Derivative(double) const override {
        return 0.0;
    }

    std::string ToString() const override {
        std::ostringstream out;
        out << value_;
        return out.str();
    }

    FunctionPtr Clone() const override {
        return std::make_shared<ConstantFunction>(*this);
    }

private:
    double value_;
};

// степенная: f(x) = x^p
class PowerFunction : public TFunction {
public:
    explicit PowerFunction(double power) : power_(power) {}

    double operator()(double x) const override {
        return std::pow(x, power_);
    }

    double Derivative(double x) const override {
        if (power_ == 0.0) {
            return 0.0;
        }
        return power_ * std::pow(x, power_ - 1.0);
    }

    std::string ToString() const override {
        std::ostringstream out;
        out << "x^" << power_;
        return out.str();
    }

    FunctionPtr Clone() const override {
        return std::make_shared<PowerFunction>(*this);
    }

private:
    double power_;
};

// экспонента: f(x) = e^x
class ExpFunction : public TFunction {
public:
    double operator()(double x) const override {
        return std::exp(x);
    }

    double Derivative(double x) const override {
        return std::exp(x);
    }

    std::string ToString() const override {
        return "exp(x)";
    }

    FunctionPtr Clone() const override {
        return std::make_shared<ExpFunction>(*this);
    }
};

// полином: f(x) = a0 + a1 x + a2 x^2 + ...
class PolynomialFunction : public TFunction {
public:
    explicit PolynomialFunction(std::vector<double> coeffs)
        : coeffs_(std::move(coeffs)) {}

    double operator()(double x) const override {
        double result = 0.0;
        double power_of_x = 1.0;

        for (double c : coeffs_) {
            result += c * power_of_x;
            power_of_x *= x;
        }
        return result;
    }

    double Derivative(double x) const override {
        double result = 0.0;
        double power_of_x = 1.0;

        for (std::size_t i = 1; i < coeffs_.size(); ++i) {
            result += static_cast<double>(i) * coeffs_[i] * power_of_x;
            power_of_x *= x;
        }
        return result;
    }

    std::string ToString() const override {
        bool first = true;
        std::ostringstream out;
        for (std::size_t i = 0; i < coeffs_.size(); ++i) {
            double c = coeffs_[i];
            if (c == 0.0) {
                continue;
            }
            if (!first) {
                out << " + ";
            }
            first = false;
            if (i == 0) {
                out << c;
            } else {
                out << c << "*x^" << i;
            }
        }
        if (first) {  // все коэффициенты нулевые
            return "0";
        }
        return out.str();
    }

    FunctionPtr Clone() const override {
        return std::make_shared<PolynomialFunction>(*this);
    }

private:
    std::vector<double> coeffs_;
};


enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div
};

// f(x) = left(x) op right(x)
class BinaryFunction : public TFunction {
public:
    BinaryFunction(FunctionPtr left, BinaryOp op, FunctionPtr right)
        : left_(std::move(left))
        , right_(std::move(right))
        , op_(op) 
    {
        if (!left_ || !right_) {
            throw std::logic_error("Null operand in BinaryFunction");
        }
    }

    double operator()(double x) const override {
        const double a = (*left_)(x);
        const double b = (*right_)(x);

        switch (op_) {
        case BinaryOp::Add: return a + b;
        case BinaryOp::Sub: return a - b;
        case BinaryOp::Mul: return a * b;
        case BinaryOp::Div:
            // деление на 0 обрабатывает сам double
            return a / b;
        }
        throw std::logic_error("Unknown binary operation");
    }

    double Derivative(double x) const override {
        const double f = (*left_)(x);
        const double g = (*right_)(x);
        const double df = left_->Derivative(x);
        const double dg = right_->Derivative(x);

        switch (op_) {
        case BinaryOp::Add: return df + dg;
        case BinaryOp::Sub: return df - dg;
        case BinaryOp::Mul: return df * g + f * dg;
        case BinaryOp::Div:
            return (df * g - f * dg) / (g * g);
        }
        throw std::logic_error("Unknown binary operation");
    }

    std::string ToString() const override {
        char symbol = '?';
        switch (op_) {
        case BinaryOp::Add: symbol = '+'; break;
        case BinaryOp::Sub: symbol = '-'; break;
        case BinaryOp::Mul: symbol = '*'; break;
        case BinaryOp::Div: symbol = '/'; break;
        }
        std::ostringstream out;
        out << "(" << left_->ToString() << " " << symbol << " " << right_->ToString() << ")";
        return out.str();
    }

    FunctionPtr Clone() const override {
        return std::make_shared<BinaryFunction>(left_->Clone(), op_, right_->Clone());
    }

private:
    FunctionPtr left_;
    FunctionPtr right_;
    BinaryOp op_;
};

// Вспомогательная функция для создания бинарных выражений
inline FunctionPtr MakeBinary(FunctionPtr left, BinaryOp op, FunctionPtr right) {
    if (!left || !right) {
        throw std::logic_error("Null operand in arithmetic expression");
    }
    return std::make_shared<BinaryFunction>(std::move(left), op, std::move(right));
}

// Операторы для указателей на функции (чтобы писать f + g)
inline FunctionPtr operator+(FunctionPtr lhs, FunctionPtr rhs) {
    return MakeBinary(std::move(lhs), BinaryOp::Add, std::move(rhs));
}

inline FunctionPtr operator-(FunctionPtr lhs, FunctionPtr rhs) {
    return MakeBinary(std::move(lhs), BinaryOp::Sub, std::move(rhs));
}

inline FunctionPtr operator*(FunctionPtr lhs, FunctionPtr rhs) {
    return MakeBinary(std::move(lhs), BinaryOp::Mul, std::move(rhs));
}

inline FunctionPtr operator/(FunctionPtr lhs, FunctionPtr rhs) {
    return MakeBinary(std::move(lhs), BinaryOp::Div, std::move(rhs));
}

// операторы для ссылок (чтобы можно было писать *f + *g)
inline BinaryFunction operator+(const TFunction& lhs, const TFunction& rhs) {
    return BinaryFunction(lhs.Clone(), BinaryOp::Add, rhs.Clone());
}

inline BinaryFunction operator-(const TFunction& lhs, const TFunction& rhs) {
    return BinaryFunction(lhs.Clone(), BinaryOp::Sub, rhs.Clone());
}

inline BinaryFunction operator*(const TFunction& lhs, const TFunction& rhs) {
    return BinaryFunction(lhs.Clone(), BinaryOp::Mul, rhs.Clone());
}

inline BinaryFunction operator/(const TFunction& lhs, const TFunction& rhs) {
    return BinaryFunction(lhs.Clone(), BinaryOp::Div, rhs.Clone());
}

// Фабрика
class FunctionFactory {
public:
    // тождественная и экспонента
    FunctionPtr Create(const std::string& type) const {
        if (type == "ident") {
            return std::make_shared<IdentityFunction>();
        }
        if (type == "exp") {
            return std::make_shared<ExpFunction>();
        }
        throw std::logic_error("Unsupported function type: " + type);
    }

    // константа и степенная
    FunctionPtr Create(const std::string& type, double value) const {
        if (type == "const") {
            return std::make_shared<ConstantFunction>(value);
        }
        if (type == "power") {
            return std::make_shared<PowerFunction>(value);
        }
        throw std::logic_error("Unsupported function type with double parameter: " + type);
    }

    // Полином
    FunctionPtr Create(const std::string& type, const std::vector<double>& coeffs) const {
        if (type == "polynomial") {
            return std::make_shared<PolynomialFunction>(coeffs);
        }
        throw std::logic_error("Unsupported function type with vector parameter: " + type);
    }
};

// Градиентный спуск для поиска корня f(x) = 0 (по сути минимизируем функцию f(x)^2)
// x_{k+1} = x_k - alpha * f(x_k) * f'(x_k)
inline double GradientDescentRoot(
    const TFunction& f,
    double x0,
    double alpha,
    std::size_t iterations)
{
    double x = x0;
    for (std::size_t i = 0; i < iterations; ++i) {
        const double fx = f(x);
        const double dfx = f.Derivative(x);
        x -= alpha * fx * dfx;
    }
    return x;
}

inline double GradientDescentRoot(
    const FunctionPtr& f,
    double x0,
    double alpha,
    std::size_t iterations)
{
    if (!f) {
        throw std::logic_error("Null function in GradientDescentRoot");
    }
    return GradientDescentRoot(*f, x0, alpha, iterations);
}
}
