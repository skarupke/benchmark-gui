#include "math/powers_of_two.hpp"

#ifndef DISABLE_GTEST
#include "test/include_test.hpp"

TEST(next_power_of_two, cases)
{
    ASSERT_EQ(0u, next_power_of_two(0));
    ASSERT_EQ(1u, next_power_of_two(1));
    ASSERT_EQ(2u, next_power_of_two(2));
    ASSERT_EQ(8u, next_power_of_two(8));
    ASSERT_EQ(16u, next_power_of_two(10));
    ASSERT_EQ(512u, next_power_of_two(300));
}

TEST(log2, cases)
{
    ASSERT_EQ(0u, log2(1));
    ASSERT_EQ(1u, log2(2));
    ASSERT_EQ(3u, log2(8));
    ASSERT_EQ(3u, log2(10));
    ASSERT_EQ(8u, log2(300));
}

#endif


