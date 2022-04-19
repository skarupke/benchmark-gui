#include "container/agl_fib_table.hpp"

#ifndef DISABLE_TESTS
#include "test/include_test.hpp"
#include <random>

TEST(agl_fib_table, empty)
{
    ska::agl_fib_hash_map<int, int> a;
    for (int i = 0; i < 50; ++i)
        ASSERT_EQ(a.end(), a.find(i));
}
TEST(agl_fib_table, simple)
{
    ska::agl_fib_hash_map<int, int> a;
    ASSERT_EQ(a.end(), a.find(5));
    a[5] = 6;
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(6, a.find(5)->second);
}

TEST(agl_fib_table, many_inserts)
{
    ska::agl_fib_hash_map<int, int> a;
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

TEST(agl_fib_table, iter)
{
    ska::agl_fib_hash_map<int, int> a;
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

TEST(agl_fib_table, many_more_inserts)
{
    ska::agl_fib_hash_map<int, int> a;
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 randomness(5);
    std::vector<int> keys;
    keys.reserve(13000);
    while (keys.size() != keys.capacity())
    {
        keys.push_back(distribution(randomness));
        a[keys.back()] = static_cast<int>(keys.size());
    }
    auto end = a.end();
    std::cout << a.load_factor() << std::endl;
    for (int key : keys)
    {
        ASSERT_NE(end, a.find(key));
    }
}

#endif
