#include "container/unordered_map.hpp"

#ifndef DISABLE_GTEST
#include <test/include_test.hpp>
#include <unordered_map>

TEST(ska_unordered_map, simple)
{
    ska::unordered_map<int, int> a;
    ASSERT_EQ(a.end(), a.begin());
    ASSERT_EQ(0u, a.size());
    ASSERT_EQ(a.end(), a.find(5));
    a[5] = 1;
    auto found = a.find(5);
    ASSERT_NE(a.end(), found);
    ++found;
    ASSERT_EQ(a.end(), found);
    ASSERT_EQ(1u, a.size());
}

TEST(ska_unordered_map, grow)
{
    ska::unordered_map<int, int> a;
    a[6225988] = 1;
    a[-620805120] = 2;
    a[1600510682] = 3;
    a.emplace(7330418, 4);
    a.emplace(28507972, 5);
    a.emplace(0, 6);
    ASSERT_NE(a.end(), a.find(6225988));
    ASSERT_NE(a.end(), a.find(-620805120));
    ASSERT_NE(a.end(), a.find(1600510682));
    ASSERT_NE(a.end(), a.find(7330418));
    ASSERT_NE(a.end(), a.find(28507972));
    ASSERT_NE(a.end(), a.find(0));
    ASSERT_EQ(1, a.find(6225988)->second);
    ASSERT_EQ(2, a.find(-620805120)->second);
    ASSERT_EQ(3, a.find(1600510682)->second);
    ASSERT_EQ(4, a.find(7330418)->second);
    ASSERT_EQ(5, a.find(28507972)->second);
    ASSERT_EQ(6, a.find(0)->second);
    ASSERT_EQ(6u, a.size());
}

TEST(ska_unordered_map, fuzz_find2)
{
    ska::unordered_map<int, int> a;
    a[-1241513821] = 1;
    a.emplace(1598887680, 2);
    auto begin = a.begin();
    ASSERT_NE(a.end(), begin);
    ++begin;
    ASSERT_NE(a.end(), begin);
    ASSERT_EQ(2u, a.size());
    a.erase(begin, begin);
    ASSERT_EQ(2u, a.size());
}

/*TEST(std_unordered_map, simple)
{
    std::unordered_map<int, int> a;
    ASSERT_EQ(a.end(), a.begin());
    ASSERT_EQ(0u, a.size());
    ASSERT_EQ(a.end(), a.find(5));
    a[5] = 1;
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(1u, a.size());
}*/

#endif

