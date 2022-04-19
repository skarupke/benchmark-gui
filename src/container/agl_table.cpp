#include "container/agl_table.hpp"


#ifndef DISABLE_TESTS
#include "test/include_test.hpp"
#include <random>

TEST(agl_table, empty)
{
    ska::agl_hash_map<int, int> a;
    for (int i = 0; i < 50; ++i)
        ASSERT_EQ(a.end(), a.find(i));
}
TEST(agl_table, simple)
{
    ska::agl_hash_map<int, int> a;
    ASSERT_EQ(a.end(), a.find(5));
    a[5] = 6;
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(6, a.find(5)->second);
}

TEST(agl_table, many_inserts)
{
    ska::agl_hash_map<int, int> a;
    for (int i = 0; i < 50; ++i)
    {
        a[2 * i] = 4 * i;
    }
    auto end = a.end();
    for (int i = 0; i < 50; ++i)
    {
        ASSERT_EQ(end, a.find(2 * i + 1));
        ASSERT_NE(end, a.find(2 * i));
        ASSERT_EQ(4 * i, a.find(2 * i)->second);
    }
}

TEST(agl_table, iter)
{
    ska::agl_hash_map<int, int> a;
    for (int i = 0; i < 10; ++i)
    {
        a[i] = 2 * i;
    }
    std::vector<std::pair<int, int>> data(a.begin(), a.end());
    std::sort(data.begin(), data.end());
    for (int i = 0; i < 10; ++i)
    {
        ASSERT_EQ(i, data[i].first);
        ASSERT_EQ(2 * i, data[i].second);
    }
}

TEST(agl_table, many_inserts_bool)
{
    ska::agl_hash_map<int, int, std::hash<int>, std::equal_to<int>, std::allocator<std::pair<int, int>>, ska::detailv11::Bool> a;
    for (int i = 0; i < 50; ++i)
    {
        a[2 * i] = 4 * i;
    }
    auto end = a.end();
    for (int i = 0; i < 50; ++i)
    {
        ASSERT_EQ(end, a.find(2 * i + 1));
        ASSERT_NE(end, a.find(2 * i));
        ASSERT_EQ(4 * i, a.find(2 * i)->second);
    }
}

TEST(agl_table, iter_bool)
{
    ska::agl_hash_map<int, int, std::hash<int>, std::equal_to<int>, std::allocator<std::pair<int, int>>, ska::detailv11::Bool> a;
    for (int i = 0; i < 10; ++i)
    {
        a[i] = 2 * i;
    }
    std::vector<std::pair<int, int>> data(a.begin(), a.end());
    std::sort(data.begin(), data.end());
    for (int i = 0; i < 10; ++i)
    {
        ASSERT_EQ(i, data[i].first);
        ASSERT_EQ(2 * i, data[i].second);
    }
}

#endif
