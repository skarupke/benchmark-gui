#include "container/google_block_hash_map.hpp"

#ifndef DISABLE_GTEST
#include "test/include_test.hpp"


TEST(google_block_hash_map, simple)
{
    ska::google_block_hash_map<int, int> a;
    ASSERT_EQ(a.end(), a.begin());
    ASSERT_EQ(a.end(), a.find(5));
    a.emplace(5, 5);
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(5, a.find(5)->second);
}
TEST(google_block_hash_map, empty)
{
    ska::google_block_hash_map<int, int> a;
    ASSERT_EQ(a.end(), a.begin());
    ASSERT_TRUE(a.empty());
}
TEST(google_block_hash_map, simple_power_of_two)
{
    ska::google_block_hash_map<int, int, ska::power_of_two_std_hash<int>> a;
    ASSERT_EQ(a.end(), a.begin());
    ASSERT_EQ(a.end(), a.find(5));
    a.emplace(5, 5);
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(5, a.find(5)->second);
}
TEST(google_block_hash_map, enter_many)
{
    ska::google_block_hash_map<int, int> a;
    for (int i = 0; i < 20; ++i)
    {
        a.emplace(i, i);
    }
    ASSERT_LE(a.bucket_count(), 32u);
    for (int i = 0; i < 20; ++i)
    {
        ASSERT_NE(a.end(), a.find(i));
        ASSERT_EQ(i, a.find(i)->second);
    }
}
TEST(google_block_hash_map, erase)
{
    ska::google_block_hash_map<int, int> a;
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
TEST(google_block_hash_map, iterate_erase)
{
    ska::google_block_hash_map<int, int> a;
    a.emplace(5, 6);
    a.emplace(7, 8);
    a.emplace(8, 9);
    auto begin = a.begin();
    ASSERT_EQ(3u, a.size());
    ASSERT_EQ(3, std::distance(begin, a.end()));
    ASSERT_NE(a.end(), begin);
    begin = a.erase(begin);
    ASSERT_EQ(2u, a.size());
    ASSERT_EQ(2, std::distance(begin, a.end()));
    begin = a.erase(begin);
    ASSERT_EQ(1u, a.size());
    ASSERT_EQ(1, std::distance(begin, a.end()));
    begin = a.erase(begin);
    ASSERT_EQ(0u, a.size());
    ASSERT_EQ(a.end(), begin);
    ASSERT_TRUE(a.empty());
}

TEST(google_block_hash_map, reallocate)
{
    ska::google_block_hash_map<int, int> a;
    std::vector<int> keys;
    for (int i = 0; i < 500; ++i)
    {
        keys.push_back(i * a.bucket_count());
        ASSERT_TRUE(a.emplace(keys.back(), i).second);
        ASSERT_EQ(static_cast<size_t>(i + 1), a.size());
    }
    ASSERT_EQ(keys.size(), a.size());
    int i = 0;
    for (int key : keys)
    {
        ASSERT_NE(a.end(), a.find(key));
        ASSERT_EQ(i++, a.find(key)->second);
        ASSERT_FALSE(a.emplace(key, 0).second);
    }
}

TEST(google_block_hash_map, range_erase)
{
    ska::google_block_hash_map<int, int> a;
    std::vector<int> keys;
    for (int i = 0; i < 1000; ++i)
    {
        keys.push_back(i * a.bucket_count());
        ASSERT_TRUE(a.emplace(keys.back(), i).second);
        ASSERT_EQ(static_cast<size_t>(i + 1), a.size());
    }
    ASSERT_EQ(keys.size(), a.size());
    auto begin_delete = std::next(a.begin(), 10);
    auto end_delete = std::next(a.begin(), 20);
    std::map<int, int> to_delete(begin_delete, end_delete);
    a.erase(begin_delete, end_delete);
    ASSERT_EQ(keys.size() - 10, a.size());
    for (int key : keys)
    {
        if (to_delete.find(key) == to_delete.end())
        {
            ASSERT_NE(a.end(), a.find(key));
            ASSERT_FALSE(a.emplace(key, 0).second);
        }
        else
        {
            ASSERT_EQ(a.end(), a.find(key));
        }
    }
}

TEST(google_block_hash_map, range_erase_power_of_two)
{
    ska::google_block_hash_map<int, int, ska::power_of_two_std_hash<int>> a;
    std::vector<int> keys;
    for (int i = 0; i < 100; ++i)
    {
        keys.push_back(i * a.bucket_count());
        ASSERT_TRUE(a.emplace(keys.back(), i).second);
        ASSERT_EQ(static_cast<size_t>(i + 1), a.size());
    }
    ASSERT_EQ(keys.size(), a.size());
    auto begin_delete = std::next(a.begin(), 10);
    auto end_delete = std::next(a.begin(), 20);
    std::map<int, int> to_delete(begin_delete, end_delete);
    a.erase(begin_delete, end_delete);
    ASSERT_EQ(keys.size() - 10, a.size());
    for (int key : keys)
    {
        if (to_delete.find(key) == to_delete.end())
        {
            ASSERT_NE(a.end(), a.find(key));
            ASSERT_FALSE(a.emplace(key, 0).second);
        }
        else
        {
            ASSERT_EQ(a.end(), a.find(key));
        }
    }
}

TEST(google_block_hash_map, erase_begin)
{
    ska::google_block_hash_map<int, int> a;
    a.emplace(638517248, 1);
    a.emplace(3866624, 2);
    a.emplace(-256, 3);
    a.emplace(256, 4);
    a.emplace(-65024, 5);
    a.emplace(16711680, 6);
    a.emplace(8388608, 7);
    a.emplace(2621440, 8);
    a.emplace(0, 9);
    a.emplace(655360000, 10);
    a.emplace(637534208, 11);
    a.emplace(2560000, 12);
    a.emplace(640024576, 13);
    a.emplace(50331648, 14);
    a.emplace(134217728, 15);
    a.emplace(67108864, 16);
    a.emplace(-555876352, 17);
    a.erase(a.begin(), std::next(a.begin()));
    a.erase(638517248);
    for (auto it = a.begin(), end = a.end(); it != end; ++it)
    {
        ASSERT_NE(a.end(), a.find(it->first));
    }
}
TEST(google_block_hash_map, too_many_tombstones)
{
    ska::google_block_hash_map<int, int> a;
    a.emplace();
    a.clear();
    for (size_t i = 0; i < a.bucket_count() / 16; ++i)
    {
        a.emplace(0 * a.bucket_count() + i, 1);
        a.emplace(1 * a.bucket_count() + i, 2);
        a.emplace(2 * a.bucket_count() + i, 3);
        a.emplace(3 * a.bucket_count() + i, 4);
        a.emplace(4 * a.bucket_count() + i, 5);
        a.emplace(5 * a.bucket_count() + i, 6);
        a.emplace(6 * a.bucket_count() + i, 7);
        a.emplace(7 * a.bucket_count() + i, 8);
        a.emplace(8 * a.bucket_count() + i, 9);
        a.emplace(9 * a.bucket_count() + i, 10);
        a.emplace(10 * a.bucket_count() + i, 11);
        a.emplace(11 * a.bucket_count() + i, 12);
        a.emplace(12 * a.bucket_count() + i, 13);
        a.emplace(13 * a.bucket_count() + i, 14);
        a.emplace(14 * a.bucket_count() + i, 15);
        a.emplace(15 * a.bucket_count() + i, 16);
        while (!a.empty())
        {
            a.erase(a.begin());
        }
    }
    ASSERT_EQ(a.end(), a.find(5));
}

#endif
