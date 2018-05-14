#include "container/bytell_hash_map.hpp"


#ifndef DISABLE_GTEST
#include "test/include_test.hpp"

TEST(bytell_hash_map, simple)
{
    ska::bytell_hash_map<int, int> a;
    ASSERT_EQ(a.end(), a.begin());
    ASSERT_EQ(a.end(), a.find(5));
    a[5] = 1;
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(1, a.find(5)->second);
}

TEST(bytell_hash_map, simple_power_of_two)
{
    ska::bytell_hash_map<int, int, ska::power_of_two_std_hash<int>> a;
    ASSERT_EQ(a.end(), a.begin());
    ASSERT_EQ(a.end(), a.find(5));
    a[5] = 1;
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(1, a.find(5)->second);
}

TEST(bytell_hash_map, overflow_power_of_two)
{
    ska::bytell_hash_map<int, int, ska::power_of_two_std_hash<int>> a;
    ASSERT_EQ(a.end(), a.begin());
    ASSERT_EQ(a.end(), a.find(-1));
    a[-1] = 1;
    ASSERT_NE(a.end(), a.find(-1));
    ASSERT_EQ(1, a.find(-1)->second);
}


TEST(bytell_hash_map, copy_empty)
{
    ska::bytell_hash_map<int, int> a;
    ska::bytell_hash_map<int, int> b(a);
    ASSERT_EQ(a, b);
}

TEST(bytell_hash_map, conflict)
{
    ska::bytell_hash_map<int, int> a;
    ASSERT_EQ(a.end(), a.find(5));
    a[5] = 1;
    int other_key = 2 * a.bucket_count() + 5;
    int third_key = 3 * a.bucket_count() + 5;
    a[other_key] = 2;
    a[third_key] = 3;
    a[0] = 4;
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_NE(a.end(), a.find(other_key));
    ASSERT_NE(a.end(), a.find(third_key));
    ASSERT_NE(a.end(), a.find(0));
    ASSERT_EQ(1, a.find(5)->second);
    ASSERT_EQ(2, a.find(other_key)->second);
    ASSERT_EQ(3, a.find(third_key)->second);
    ASSERT_EQ(4, a.find(0)->second);
}

TEST(bytell_hash_map, erase)
{
    ska::bytell_hash_map<int, int> a;
    a.emplace(5, 6);
    a.emplace(7, 8);
    a.emplace(8, 9);
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_NE(a.end(), a.find(7));
    ASSERT_NE(a.end(), a.find(8));
    ASSERT_EQ(6, a.find(5)->second);
    ASSERT_EQ(8, a.find(7)->second);
    ASSERT_EQ(9, a.find(8)->second);
    ASSERT_EQ(3u, a.size());
    ASSERT_TRUE(a.erase(7));
    ASSERT_EQ(2u, a.size());
    ASSERT_EQ(a.end(), a.find(7));
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_NE(a.end(), a.find(8));
    ASSERT_EQ(6, a.find(5)->second);
    ASSERT_EQ(9, a.find(8)->second);
}

TEST(bytell_hash_map, conflicting_erase)
{
    ska::bytell_hash_map<int, int> a;
    a.emplace(5, 6);
    a.emplace(7, 8);
    a.emplace(8, 9);
    int conflicting_key = a.bucket_count() * 2 + 5;
    int conflicting_key2 = a.bucket_count() * 3 + 5;
    a.emplace(conflicting_key, 10);
    a.emplace(conflicting_key2, 13);
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_NE(a.end(), a.find(7));
    ASSERT_NE(a.end(), a.find(8));
    ASSERT_NE(a.end(), a.find(conflicting_key));
    ASSERT_NE(a.end(), a.find(conflicting_key2));
    ASSERT_EQ(6, a.find(5)->second);
    ASSERT_EQ(8, a.find(7)->second);
    ASSERT_EQ(9, a.find(8)->second);
    ASSERT_EQ(10, a.find(conflicting_key)->second);
    ASSERT_EQ(13, a.find(conflicting_key2)->second);
    ASSERT_EQ(5u, a.size());
    ASSERT_TRUE(a.erase(7));
    ASSERT_EQ(4u, a.size());
    ASSERT_EQ(a.end(), a.find(7));
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_NE(a.end(), a.find(8));
    ASSERT_NE(a.end(), a.find(conflicting_key));
    ASSERT_NE(a.end(), a.find(conflicting_key2));
    ASSERT_EQ(6, a.find(5)->second);
    ASSERT_EQ(9, a.find(8)->second);
    ASSERT_EQ(10, a.find(conflicting_key)->second);
    ASSERT_EQ(13, a.find(conflicting_key2)->second);
    ASSERT_TRUE(a.erase(conflicting_key));
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_NE(a.end(), a.find(8));
    ASSERT_EQ(a.end(), a.find(conflicting_key));
    ASSERT_NE(a.end(), a.find(conflicting_key2));
    ASSERT_EQ(6, a.find(5)->second);
    ASSERT_EQ(9, a.find(8)->second);
    ASSERT_EQ(13, a.find(conflicting_key2)->second);
}

TEST(bytell_hash_map, range_erase)
{
    ska::bytell_hash_map<int, int> a;
    a.emplace(5, 6);
    a.emplace(7, 8);
    a.emplace(8, 9);
    int conflicting_key = a.bucket_count() * 2 + 5;
    int conflicting_key2 = a.bucket_count() * 3 + 5;
    a.emplace(conflicting_key, 10);
    a.emplace(conflicting_key2, 13);
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_NE(a.end(), a.find(7));
    ASSERT_NE(a.end(), a.find(8));
    ASSERT_NE(a.end(), a.find(conflicting_key));
    ASSERT_NE(a.end(), a.find(conflicting_key2));
    ASSERT_EQ(6, a.find(5)->second);
    ASSERT_EQ(8, a.find(7)->second);
    ASSERT_EQ(9, a.find(8)->second);
    ASSERT_EQ(10, a.find(conflicting_key)->second);
    ASSERT_EQ(13, a.find(conflicting_key2)->second);
    ASSERT_EQ(5u, a.size());
    std::vector<std::pair<int, int>> to_keep(a.begin(), std::next(a.begin()));
    to_keep.insert(to_keep.end(), std::next(a.begin(), 3), a.end());
    std::vector<std::pair<int, int>> to_erase(std::next(a.begin()), std::next(a.begin(), 3));
    ASSERT_EQ(5u, to_keep.size() + to_erase.size());
    a.erase(std::next(a.begin()), std::next(a.begin(), 3));
    ASSERT_EQ(3u, a.size());
    for (std::pair<int, int> x : to_keep)
    {
        ASSERT_NE(a.end(), a.find(x.first));
        ASSERT_EQ(x.second, a.find(x.first)->second);
    }
    for (std::pair<int, int> x : to_erase)
    {
        ASSERT_EQ(a.end(), a.find(x.first));
    }
}

TEST(bytell_hash_map, fuzz_find1)
{
    ska::bytell_hash_map<int, int> a;
    a[1002625754] = 1;
    ASSERT_EQ(1u, a.size());
    a[699248090] = 2;
    ASSERT_EQ(2u, a.size());
    a[699248090] = 3;
    ASSERT_EQ(2u, a.size());
    a[543257294] = 4;
    ASSERT_EQ(3u, a.size());
    a[2003788026] = 5;
    ASSERT_EQ(4u, a.size());
    a[2218595] = 6;
    ASSERT_EQ(5u, a.size());
    ASSERT_NE(a.end(), a.find(1002625754));
    ASSERT_NE(a.end(), a.find(699248090));
    ASSERT_NE(a.end(), a.find(543257294));
    ASSERT_NE(a.end(), a.find(2003788026));
    ASSERT_NE(a.end(), a.find(2218595));
    size_t count = std::distance(a.begin(), a.end());
    ASSERT_EQ(5u, count);
}

TEST(bytell_hash_map, full_map)
{
    ska::bytell_hash_map<int, int, ska::power_of_two_std_hash<int>> a;
    int num_keys = 1024;
    for (int i = 0; i < num_keys; ++i)
    {
        a.emplace(i, i);
    }
    ASSERT_LT(0.2f, a.load_factor());
    a.emplace(1024 * 1024 * 1024, 100);
    ASSERT_LT(0.1f, a.load_factor());
    ASSERT_EQ(size_t(num_keys + 1), a.size());
}
#include "math/fibonacci.h"
TEST(bytell_hash_map, jump_distances_power_of_two)
{
    for (int i = 2; i <= 64; i *= 2)
    {
        std::map<int, int> counts;
        for (int j = 0; j < i; ++j)
            counts[j];

        std::vector<size_t> distances(std::begin(ska::detailv8::sherwood_v8_constants::jump_distances), std::end(ska::detailv8::sherwood_v8_constants::jump_distances));

        for (size_t distance : distances)
        {
            ++counts[distance % i];
        }
        for (std::pair<int, int> entry : counts)
        {
            ASSERT_NE(0, entry.second);
        }
    }
    for (int i = 128; i <= 128*1024; i *= 2)
    {
        std::map<int, int> counts;
        std::vector<size_t> distances(std::begin(ska::detailv8::sherwood_v8_constants::jump_distances), std::end(ska::detailv8::sherwood_v8_constants::jump_distances));

        for (size_t distance : distances)
        {
            int mod = distance % i;
            ++counts[mod];
        }
        float limit = i == 128 ? 2.0f : i == 256 ? 1.75f : i == 512 ? 1.5f : i == 1024 ? 1.25f : 1.0f;
        float sum_of_squares = 0;
        for (std::pair<int, int> entry : counts)
        {
            float wrong = std::max(0.0f, entry.second - limit);
            sum_of_squares += wrong * wrong;
        }
        ASSERT_GT(10.0f, sum_of_squares);
    }
}

TEST(bytell_hash_map, DISABLED_jump_distances_prime)
{
    ska::prime_number_hash_policy policy;
    size_t i = 2;
    for (; i < 100000; policy.next_size_over(++i))
    {
        std::map<size_t, int> counts;

        std::vector<size_t> distances(std::begin(ska::detailv8::sherwood_v8_constants::jump_distances), std::end(ska::detailv8::sherwood_v8_constants::jump_distances));

        for (size_t distance : distances)
        {
            ++counts[distance % i];
        }
        std::cout << "\n\n" << i << ":\n";
        for (std::pair<int, int> entry : counts)
        {
            std::cout << entry.first << ": " << entry.second << '\n';
        }
        std::cout.flush();
        int num_non_zero = counts.size();
        EXPECT_LT(i * 0.8f, num_non_zero);
    }
}

#endif
