#include <iostream>

#include <gtest/gtest.h>

#include "stream.h"

TEST(ScalarStreams, StreamingAverage)
{
    stream::Stream<float> s;
    auto m = stream::mean(s);

    m.reset_states(1);
    EXPECT_EQ(m.eval({1})[0], 1);
    EXPECT_EQ(m.eval({2})[0], (1. + 2.) / 2);
    EXPECT_EQ(m.eval({3})[0], (1. + 2. + 3.) / 3);
}

TEST(VectorStreams, StreamingAverage)
{
    stream::Stream<float> s;
    auto m = stream::mean(s);

    m.reset_states(3);

    auto v = m.eval({1, 2, 3});
    EXPECT_TRUE((v == std::valarray<float>({1, 2, 3})).min());

    v = m.eval({2, 3, 4});
    EXPECT_TRUE((v == std::valarray<float>({(1. + 2.) / 2, (2. + 3.) / 2, (3. + 4.) / 2})).min());

    v = m.eval({3, 4, 5});
    EXPECT_TRUE((v == std::valarray<float>({(1. + 2. + 3.) / 3, (2. + 3. + 4.) / 3, (3. + 4. + 5.) / 3})).min());

    v = m.eval({3, 4, 5});
    EXPECT_TRUE((v == std::valarray<float>({(1. + 2. + 3. + 3.) / 4, (2. + 3. + 4. + 4.) / 4, (3. + 4. + 5. + 5.) / 4})).min());
}

TEST(VectorStreams, StashStates)
{
    stream::Stream<float> s;
    auto m = stream::mean(s);

    m.reset_states(3);

    auto v = m.eval({1, 2, 3});
    v = m.eval({2, 3, 4});

    m.stash_states(std::vector<std::string_view>{"a", "b", "c"});

    v = m.eval({3, 4, 5});
    EXPECT_TRUE((v == std::valarray<float>({(1. + 2. + 3.) / 3, (2. + 3. + 4.) / 3, (3. + 4. + 5.) / 3})).min());

    m.restore_states(std::vector<std::string_view>{"a", "b", "c"});

    v = m.eval({3, 4, 5});
    EXPECT_TRUE((v == std::valarray<float>({(1. + 2. + 3.) / 3, (2. + 3. + 4.) / 3, (3. + 4. + 5.) / 3})).min());
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}