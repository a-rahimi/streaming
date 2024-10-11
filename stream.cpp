#include <iostream>
#include <initializer_list>
#include <tuple>
#include <type_traits>

// For now, vector<T> is the same as T. But eventually, it'll be a group of
// T's.
template <typename T>
using vector = T;

template <typename T>
struct Stream
{
    using InputType = vector<T>;
    using Output = vector<T>;

    static auto eval(const InputType &i)
    {
        return i;
    }
};

template <typename Derived, typename InputExpr, typename _State>
struct UnaryOperator
{
    using InputType = typename InputExpr::InputType;
    using State = _State;

    InputExpr input_expr;
    State state;

    auto eval(const InputType &i)
    {
        return Derived::apply(state, input_expr.eval(i));
    }
};

template <typename Derived, typename InputExpr1, typename InputExpr2>
struct StatelessBinaryOperator
{
    using InputType = typename InputExpr1::InputType;

    InputExpr1 input_expr1;
    InputExpr2 input_expr2;

    auto eval(const InputType &i)
    {
        return Derived::apply(input_expr1.eval(i), input_expr2.eval(i));
    }
};

template <typename InputExpr>
struct Count : UnaryOperator<Count<InputExpr>, InputExpr, vector<int>>
{
    using Base = UnaryOperator<Count<InputExpr>, InputExpr, vector<int>>;

    using Output = Base::State;

    Count(InputExpr e) : Base{e} {}

    static Output apply(Base::State &state, const typename InputExpr::Output &)
    {
        state += 1;
        return state;
    }
};

template <typename InputExpr>
struct Accumulate : UnaryOperator<Accumulate<InputExpr>, InputExpr, typename InputExpr::Output>
{
    using Base = UnaryOperator<Accumulate<InputExpr>, InputExpr, typename InputExpr::Output>;
    using Output = Base::State;

    Accumulate(InputExpr e) : Base{e} {}

    static Output apply(Base::State &state, const typename InputExpr::Output &i)
    {
        state += i;
        return state;
    }
};

template <typename ExprNumerator, typename ExprDenominator>
struct Divide : StatelessBinaryOperator<Divide<ExprNumerator, ExprDenominator>, ExprNumerator, ExprDenominator>
{
    using Base = StatelessBinaryOperator<Divide<ExprNumerator, ExprDenominator>, ExprNumerator, ExprDenominator>;
    using Output = ExprNumerator::Output;

    Divide(ExprNumerator e1, ExprDenominator e2) : Base{e1, e2} {}

    static Output apply(const typename ExprNumerator::Output &numerator, const typename ExprDenominator::Output &denominator)
    {
        return numerator / denominator;
    }
};

template <typename InputExpr>
auto mean(InputExpr e)
{
    return Divide(Accumulate(e), Count(e));
}

int main()
{
    Stream<float> stream;

    auto m = mean(stream);

    std::cout << m.eval(1) << " expected: " << 1. / 1 << std::endl;
    std::cout << m.eval(2) << " expected: " << (1. + 2.) / 2 << std::endl;
    std::cout << m.eval(3) << " expected: " << (1. + 2. + 3.) / 3 << std::endl;
    std::cout << m.eval(4) << " expected: " << (1. + 2. + 3. + 4.) / 4 << std::endl;
}
