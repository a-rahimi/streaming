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
    struct
    {
        std::vector<std::string_view> ids;
        std::valarray<float> values;
    } packets[] = {
        {{"a", "b"},
         {1., 2.}},
        {{"a", "c"},
         {10., 20.}},
        {{"a", "b", "c"},
         {100., 20., 200.}},
    };

    stream::Stream<float> s;
    auto processor = stream::mean(s);

    for (const auto &packet : packets)
    {
        processor.reset_states(packet.ids.size());
        processor.restore_states(packet.ids);
        auto v = processor.eval(packet.values);
        std::cout << v << std::endl;
        processor.stash_states(packet.ids);
    }
}
