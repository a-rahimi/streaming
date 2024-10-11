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

template <typename InputExpr, typename _State, typename _Output, _Output (*func)(_State &, const typename InputExpr::Output &)>
struct UnaryOperator
{
    using InputType = typename InputExpr::InputType;
    using State = _State;
    using Output = _Output;

    InputExpr input_expr;
    State state;

    Output eval(const InputType &i)
    {
        return func(state, input_expr.eval(i));
    }
};

template <typename InputExpr>
auto Count(InputExpr e)
{
    using State = vector<int>;
    using Output = State;
    auto func = [](State &state, const typename InputExpr::Output &)
    { state += 1; return state; };
    return UnaryOperator<InputExpr, State, Output, func>{e};
}

template <typename InputExpr>
auto Accumulate(InputExpr e)
{
    using State = typename InputExpr::Output;
    using Output = State;
    auto func = [](State &state, const typename InputExpr::Output &i)
    { state += i; return state; };
    return UnaryOperator<InputExpr, State, Output, func>{e};
};

template <typename InputExpr1, typename InputExpr2, typename _Output, _Output (*func)(const typename InputExpr1::Output &, const typename InputExpr2::Output &)>
struct StatelessBinaryOperator
{
    using InputType = typename InputExpr1::InputType;
    using Output = _Output;

    InputExpr1 input_expr1;
    InputExpr2 input_expr2;

    auto eval(const InputType &i)
    {
        return func(input_expr1.eval(i), input_expr2.eval(i));
    }
};

template <typename ExprNumerator, typename ExprDenominator>
auto Divide(ExprNumerator num, ExprDenominator den)
{
    using Output = ExprNumerator::Output;
    auto func = [](const typename ExprNumerator::Output &numerator, const typename ExprDenominator::Output &denominator)
    { return numerator / denominator; };
    return StatelessBinaryOperator<ExprNumerator, ExprDenominator, Output, func>{num, den};
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
