#include "container/linear_search_map.hpp"


#ifndef DISABLE_GTEST
#include "test/include_test.hpp"

TEST(linear_search_map, simple)
{
    ska::linear_search_map<int, int> a;
    ASSERT_EQ(a.end(), a.begin());
    ASSERT_EQ(0u, a.size());
    ASSERT_EQ(a.end(), a.find(10));
    a.emplace(10, 1);
    a[20] = 2;
    ASSERT_EQ(2u, a.size());
    ASSERT_NE(a.end(), a.find(10));
    ASSERT_EQ(1, a.find(10)->second);
    ASSERT_EQ(2, a.find(20)->second);
}


#endif
