#include "util/algorithm.hpp"


#ifndef DISABLE_TESTS
#include "test/include_test.hpp"

TEST(mismatch_ignoring, simple)
{
    auto is_space = [](char c){ return c == ' '; };
    std::string a = "abc";
    std::string b = "abc";
    auto mismatch = mismatch_ignoring(a.begin(), a.end(), b.begin(), b.end(), is_space);
    ASSERT_EQ(std::make_pair(a.end(), b.end()), mismatch);
    a = "ab c ";
    b = " ab c";
    mismatch = mismatch_ignoring(a.begin(), a.end(), b.begin(), b.end(), is_space);
    ASSERT_EQ(std::make_pair(a.end(), b.end()), mismatch);
    a = "a b c";
    b = "a bc";
    mismatch = mismatch_ignoring(a.begin(), a.end(), b.begin(), b.end(), is_space);
    ASSERT_EQ('b', *mismatch.first);
    ASSERT_EQ('b', *mismatch.second);
}

TEST(max_element_transformed, simple)
{
    std::vector<int> a = { 1, -2, 3, -4 };
    auto max2 = max_element_transformed(a.begin(), a.end(), [](int x){ return x * 2; });
    ASSERT_NE(a.end(), max2.first);
    ASSERT_EQ(3, *max2.first);
    ASSERT_EQ(6, max2.second);
    auto maxabs = max_element_transformed(a.begin(), a.end(), [](int x){ return std::abs(x); });
    ASSERT_NE(a.end(), maxabs.first);
    ASSERT_EQ(-4, *maxabs.first);
    ASSERT_EQ(4, maxabs.second);
}
TEST(max_element_transformed, equal)
{
    std::vector<int> a = { 1, 1 };
    auto max = max_element_transformed(a.begin(), a.end(), [](int x){ return x * 2; });
    ASSERT_EQ(a.begin(), max.first);
    ASSERT_EQ(1, *max.first);
    ASSERT_EQ(2, max.second);
}

TEST(partial_sort_pointer, simple)
{
    std::vector<int> a = { 5, 4, 7, 1, 2, 4, 9, 5, 3, 2 };
    std::array<int *, 4> sorted;
    auto end = partial_sort_pointer(a.begin(), a.end(), sorted.begin(), sorted.end());
    ASSERT_EQ(sorted.end(), end);
    ASSERT_EQ(1, *sorted[0]);
    ASSERT_EQ(2, *sorted[1]);
    ASSERT_EQ(2, *sorted[2]);
    ASSERT_EQ(3, *sorted[3]);
}

TEST(partial_sort_pointer, too_short)
{
    std::vector<int> a = { 5, 4, 7 };
    std::array<int *, 4> sorted;
    auto end = partial_sort_pointer(a.begin(), a.end(), sorted.begin(), sorted.end());
    ASSERT_EQ(sorted.end() - 1, end);
    ASSERT_EQ(4, *sorted[0]);
    ASSERT_EQ(5, *sorted[1]);
    ASSERT_EQ(7, *sorted[2]);
}

TEST(partial_sort_pointer, top_seven)
{
    std::vector<int> a = { 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    std::array<int *, 7> sorted;
    auto end = partial_sort_pointer(a.begin(), a.end(), sorted.begin(), sorted.end());
    ASSERT_EQ(sorted.end(), end);
    ASSERT_EQ(1, *sorted[0]);
    ASSERT_EQ(2, *sorted[1]);
    ASSERT_EQ(3, *sorted[2]);
    ASSERT_EQ(4, *sorted[3]);
    ASSERT_EQ(5, *sorted[4]);
    ASSERT_EQ(6, *sorted[5]);
    ASSERT_EQ(7, *sorted[6]);
}

#endif

