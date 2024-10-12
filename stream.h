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

    template <typename InputExpr, typename State, typename _Output, _Output (*func)(State &, const typename InputExpr::Output &)>
    struct UnaryOperator
    {
        using InputType = typename InputExpr::InputType;
        using Output = _Output;

        InputExpr input_expr;
        State state;
        std::map<std::string_view, typename State::value_type> state_storage;

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
    auto make_unary_operator(InputExpr e, Function func)
    {
        return UnaryOperator<InputExpr, typename lambda_signature<Function>::state_type, typename lambda_signature<Function>::return_type, func>{e};
    }

    template <InputExpr InputExpr>
    auto count(InputExpr e)
    {
        return make_unary_operator(
            e,
            [](vector<int> &state, const typename InputExpr::Output &) -> vector<int>
            { state += 1; return state; });
    }

    template <InputExpr InputExpr>
    auto accumulate(InputExpr e)
    {
        return make_unary_operator(e,
                                   [](typename InputExpr::Output &state, const typename InputExpr::Output &i)
                                   { state += i; return state; });
    };

    template <InputExpr InputExpr1, InputExpr InputExpr2, typename _Output, _Output (*func)(const typename InputExpr1::Output &, const typename InputExpr2::Output &)>
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

    template <InputExpr Expr1, InputExpr Expr2, typename Function>
    auto make_stateless_binary_operator(Expr1 e1, Expr2 e2, Function func)
    {
        return StatelessBinaryOperator<Expr1, Expr2, typename stateless_binary_lambda_signature<Function>::return_type, func>{e1, e2};
    }

    template <InputExpr ExprNumerator, InputExpr ExprDenominator>
    auto divide(ExprNumerator num, ExprDenominator den)
    {
        return make_stateless_binary_operator(num, den,
                                              [](const typename ExprNumerator::Output &numerator, const typename ExprDenominator::Output &denominator) -> typename ExprNumerator::Output
                                              { return numerator / (1. * denominator); });
    };

    auto mean(InputExpr auto e)
    {
        return divide(accumulate(e), count(e));
    }

} // namespace stream
