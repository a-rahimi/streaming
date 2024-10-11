#include <iostream>

#include "stream.h"

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::valarray<T> &v)
{
    os << "[";
    for (auto i : v)
    {
        os << i << ", ";
    }
    os << "]";
    return os;
}

int main()
{
    stream::Stream<float> s;

    auto m = stream::mean(s);
    m.reset_states(3);

    std::cout << m.eval({1, 2, 3}) << std::endl;
    std::cout << m.eval({2, 3, 4}) << std::endl;

    m.stash_states(std::vector<std::string_view>{"a", "b", "c"});
    std::cout << m.eval({3, 4, 5}) << std::endl;
    std::cout << m.eval({3, 4, 5}) << std::endl;

    m.restore_states(std::vector<std::string_view>{"a", "b", "c"});
    std::cout << m.eval({3, 4, 5}) << std::endl;
}
