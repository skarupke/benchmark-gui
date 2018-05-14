#include "container/microsoft_unordered_map"

#ifndef DISABLE_GTEST
#include <test/include_test.hpp>

TEST(microsoft_unordered_map, simple)
{
    ms_std::unordered_map<int, int> a;
    ASSERT_EQ(0u, a.size());
    ASSERT_EQ(a.end(), a.find(5));
    a[5] = 2;
    ASSERT_EQ(1u, a.size());
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(2, a.find(5)->second);
    a.erase(5);
    ASSERT_EQ(0u, a.size());
    ASSERT_EQ(a.end(), a.find(5));
}

#endif
