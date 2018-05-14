#include "math/my_math.hpp"



#ifndef DISABLE_GTEST
#include "test/include_test.hpp"


TEST(sum_to_n, simple)
{
    ASSERT_EQ(1, sum_to(1));
    ASSERT_EQ(1 + 2, sum_to(2));
    ASSERT_EQ(1 + 2 + 3, sum_to(3));
    ASSERT_EQ(1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9, sum_to(9));
    ASSERT_EQ(1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10, sum_to(10));
}

#endif
