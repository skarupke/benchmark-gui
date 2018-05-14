#include "math/halton_sequence.hpp"


#ifndef DISABLE_GTEST
#include "test/include_test.hpp"

TEST(halton_sequence, float)
{
    ASSERT_EQ(0.0f, HaltonValue<2>(0));
    ASSERT_EQ(0.5f, HaltonValue<2>(1));
    ASSERT_EQ(0.25f, HaltonValue<2>(2));
    ASSERT_EQ(0.75f, HaltonValue<2>(3));
    ASSERT_EQ(0.125f, HaltonValue<2>(4));
    ASSERT_EQ(0.625f, HaltonValue<2>(5));
    ASSERT_EQ(0.375f, HaltonValue<2>(6));
    ASSERT_EQ(0.875f, HaltonValue<2>(7));

    ASSERT_EQ(0.0f, HaltonValue<3>(0));
    ASSERT_FLOAT_EQ(1.0f / 3.0f, HaltonValue<3>(1));
    ASSERT_FLOAT_EQ(2.0f / 3.0f, HaltonValue<3>(2));
    ASSERT_FLOAT_EQ(1.0f / 9.0f, HaltonValue<3>(3));
    ASSERT_FLOAT_EQ(4.0f / 9.0f, HaltonValue<3>(4));
    ASSERT_FLOAT_EQ(7.0f / 9.0f, HaltonValue<3>(5));
    ASSERT_FLOAT_EQ(2.0f / 9.0f, HaltonValue<3>(6));
    ASSERT_FLOAT_EQ(5.0f / 9.0f, HaltonValue<3>(7));
    ASSERT_FLOAT_EQ(8.0f / 9.0f, HaltonValue<3>(8));
}

TEST(halton_sequence, int)
{
    ASSERT_EQ(0, HaltonValue<2>(8, 0));
    ASSERT_EQ(4, HaltonValue<2>(8, 1));
    ASSERT_EQ(2, HaltonValue<2>(8, 2));
    ASSERT_EQ(6, HaltonValue<2>(8, 3));
    ASSERT_EQ(1, HaltonValue<2>(8, 4));
    ASSERT_EQ(5, HaltonValue<2>(8, 5));
    ASSERT_EQ(3, HaltonValue<2>(8, 6));
    ASSERT_EQ(7, HaltonValue<2>(8, 7));
    ASSERT_EQ(0, HaltonValue<2>(8, 8));
}

TEST(halton_sequence, shuffle_in_halton_order)
{
    std::vector<int> result = shuffle_in_halton_order(std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7 });
    ASSERT_EQ(0, result[0]);
    ASSERT_EQ(4, result[1]);
    ASSERT_EQ(2, result[2]);
    ASSERT_EQ(6, result[3]);
    ASSERT_EQ(1, result[4]);
    ASSERT_EQ(5, result[5]);
    ASSERT_EQ(3, result[6]);
    ASSERT_EQ(7, result[7]);
}

TEST(halton_sequence, shuffle_in_halton_order_not_power_of_two)
{
    std::vector<int> result = shuffle_in_halton_order(std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    ASSERT_EQ(0, result[0]);
    ASSERT_EQ(8, result[1]);
    ASSERT_EQ(4, result[2]);
    ASSERT_EQ(2, result[3]);
    ASSERT_EQ(6, result[4]);
    ASSERT_EQ(1, result[5]);
    ASSERT_EQ(9, result[6]);
    ASSERT_EQ(5, result[7]);
    ASSERT_EQ(3, result[8]);
    ASSERT_EQ(7, result[9]);
}

#endif
