#include "container/flat_hash_map.hpp"

#if 0
// code for the slides
template<typename T>
struct hashtable_entry
{
    int8_t distance_from_desired = -1;

    union { T value; };
};
#endif

#ifndef DISABLE_GTEST
#include "test/include_test.hpp"

TEST(flat_hash_map, simple)
{
    ska::flat_hash_map<int, int> a;
    ASSERT_EQ(a.end(), a.find(5));
    a[5] = 3;
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(3, a.find(5)->second);
    ASSERT_EQ(a.end(), a.find(6));
    a[6] = 4;
    ASSERT_NE(a.end(), a.find(6));
    ASSERT_EQ(4, a.find(6)->second);
    a.erase(5);
    ASSERT_EQ(a.end(), a.find(5));
    ASSERT_EQ(1u, a.size());
}

TEST(flat_hash_map, fuzz_find)
{
    ska::flat_hash_map<int, int> a;
    a[1701669236] = 0;
    a[-1448498775] = 2;
    a[-1448498934] = 3;
    a[1873389993] = 4;
    a.erase(std::next(a.begin()), std::next(a.begin()));

    size_t size = a.size();
    size_t double_check_size = 0;
    for (auto it = a.begin(), end = a.end(); it != end; ++it)
        ++double_check_size;
    ASSERT_EQ(size, double_check_size);
}

#endif
