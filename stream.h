#include <map>
#include <valarray>
#include <vector>

namespace stream
{
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

    template <typename InputExpr>
    auto count(InputExpr e)
    {
        using State = vector<int>;
        using Output = State;
        auto func = [](State &state, const typename InputExpr::Output &) -> Output
        { state += 1; return state; };
        return UnaryOperator<InputExpr, State, Output, func>{e};
    }

    template <typename InputExpr>
    auto accumulate(InputExpr e)
    {
        using State = typename InputExpr::Output;
        using Output = State;
        auto func = [](State &state, const typename InputExpr::Output &i) -> Output
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

    template <typename ExprNumerator, typename ExprDenominator>
    auto divide(ExprNumerator num, ExprDenominator den)
    {
        using Output = ExprNumerator::Output;
        auto func = [](const typename ExprNumerator::Output &numerator, const typename ExprDenominator::Output &denominator) -> Output
        { return numerator / (1. * denominator); };
        return StatelessBinaryOperator<ExprNumerator, ExprDenominator, Output, func>{num, den};
    };

    template <typename InputExpr>
    auto mean(InputExpr e)
    {
        return divide(accumulate(e), count(e));
    }

} // namespace stream
