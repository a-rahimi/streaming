#include <concepts>
#include <map>
#include <valarray>
#include <vector>

namespace stream
{
    template <typename T>
    concept InputExpr = requires(T e) {
        // an InputExpr::eval must have an InputType and an OutputType, and an
        // apply() method that takes the former and returns the latter.
        { e.eval(std::declval<typename T::InputType>()) } -> std::same_as<typename T::Output>;
    };

    // The vector type use throughout. For now, we use valarray, but xTensor is
    // problem a better choice.
    template <typename T>
    using vector = std::valarray<T>;

    template <typename T>
    struct Stream
    {
        using InputType = vector<T>;
        using Output = vector<T>;

        auto eval(const InputType &i) { return i; }

        void reset_states(size_t state_dim) {}
        void stash_states(const std::vector<std::string_view> &ids) {}
        void restore_states(const std::vector<std::string_view> &ids) {}
    };

    template <typename T>
    struct lambda_signature : public lambda_signature<decltype(&T::operator())>
    {
    };

    template <typename ClassType, typename ReturnType, typename State, typename Input>
    struct lambda_signature<ReturnType (ClassType::*)(State &, const Input &) const>
    {
        using return_type = ReturnType;
        using state_type = State;
        using input_type = Input;
    };

    template <InputExpr InputExpr, typename Function>
    struct UnaryOperator
    {
        using InputType = typename InputExpr::InputType;
        using Output = lambda_signature<Function>::return_type;
        using State = lambda_signature<Function>::state_type;

        InputExpr input_expr;
        Function func;
        State state;
        std::map<std::string_view, typename State::value_type> state_storage;

        UnaryOperator(InputExpr e, Function f) : input_expr(e), func(f) {}

        Output eval(const InputType &i)
        {
            return func(state, input_expr.eval(i));
        }

        void reset_states(size_t state_dim)
        {
            state.resize(state_dim);
            input_expr.reset_states(state_dim);
        }

        void stash_states(const std::vector<std::string_view> &ids)
        {
            if (ids.size() != state.size())
            {
                throw std::runtime_error("stash_states: ids.size() != state.size()");
            }
            size_t i = 0;
            for (auto id : ids)
            {
                state_storage[id] = state[i++];
            }

            input_expr.stash_states(ids);
        }

        void restore_states(const std::vector<std::string_view> &ids)
        {
            if (ids.size() != state.size())
            {
                throw std::runtime_error("restore_states: ids.size() != state.size()");
            }
            size_t i = 0;
            for (auto id : ids)
            {
                state[i++] = state_storage[id];
            }

            input_expr.restore_states(ids);
        }
    };

    template <InputExpr InputExpr>
    auto count(InputExpr e)
    {
        return UnaryOperator(
            e,
            [](vector<int> &state, const typename InputExpr::Output &) -> vector<int>
            { state += 1; return state; });
    }

    template <InputExpr InputExpr>
    auto accumulate(InputExpr e)
    {
        return UnaryOperator(e,
                             [](typename InputExpr::Output &state, const typename InputExpr::Output &i)
                             { state += i; return state; });
    };

    template <typename T>
    struct stateless_binary_lambda_signature : public stateless_binary_lambda_signature<decltype(&T::operator())>
    {
    };

    template <typename ClassType, typename ReturnType, typename Input1, typename Input2>
    struct stateless_binary_lambda_signature<ReturnType (ClassType::*)(const Input1 &, const Input2 &) const>
    {
        using return_type = ReturnType;
        using input1_type = Input1;
        using input2_type = Input2;
    };
    template <InputExpr InputExpr1, InputExpr InputExpr2, typename Function>
    struct StatelessBinaryOperator
    {
        using InputType = typename InputExpr1::InputType;
        using Output = stateless_binary_lambda_signature<Function>::return_type;

        InputExpr1 input_expr1;
        InputExpr2 input_expr2;
        Function func;

        StatelessBinaryOperator(InputExpr1 e1, InputExpr2 e2, Function f) : input_expr1(e1), input_expr2(e2), func(f) {}

        auto eval(const InputType &i)
        {
            return func(input_expr1.eval(i), input_expr2.eval(i));
        }
        void reset_states(size_t state_dim)
        {
            input_expr1.reset_states(state_dim);
            input_expr2.reset_states(state_dim);
        }
        void stash_states(const std::vector<std::string_view> &ids)
        {
            input_expr1.stash_states(ids);
            input_expr2.stash_states(ids);
        }
        void restore_states(const std::vector<std::string_view> &ids)
        {
            input_expr1.restore_states(ids);
            input_expr2.restore_states(ids);
        }
    };

    template <InputExpr ExprNumerator, InputExpr ExprDenominator>
    auto divide(ExprNumerator num, ExprDenominator den)
    {
        return StatelessBinaryOperator(num, den,
                                       [](const typename ExprNumerator::Output &numerator, const typename ExprDenominator::Output &denominator) -> typename ExprNumerator::Output
                                       { return numerator / (1. * denominator); });
    };

    auto mean(InputExpr auto e)
    {
        return divide(accumulate(e), count(e));
    }

} // namespace stream
