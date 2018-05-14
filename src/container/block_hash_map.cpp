#include "container/block_hash_map.hpp"

namespace ska
{
namespace detailv6
{
__m128i sherwood_v6_constants<ConstantArray>::sixteen_ones;
__m128i sherwood_v6_constants<ConstantArray>::sixteen_distance_starts;
__m128i sherwood_v6_constants<ConstantArray>::lookup_starts[32];
__m128i sherwood_v6_constants<ZeroStart>::sixteen_ones;
__m128i sherwood_v6_constants<ZeroStart>::sixteen_distance_starts;
}
}

#include "test/include_test.hpp"


TEST(block_hash_map, simple)
{
    ska::block_hash_map<int, int> a;
    ASSERT_EQ(a.end(), a.begin());
    ASSERT_EQ(a.end(), a.find(5));
    a.emplace(5, 5);
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(5, a.find(5)->second);
}
TEST(block_hash_map, simple_power_of_two)
{
    ska::block_hash_map<int, int, ska::power_of_two_std_hash<int>> a;
    ASSERT_EQ(a.end(), a.begin());
    ASSERT_EQ(a.end(), a.find(5));
    a.emplace(5, 5);
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(5, a.find(5)->second);
}
TEST(block_hash_map, enter_many)
{
    ska::block_hash_map<int, int> a;
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
TEST(block_hash_map, erase)
{
    ska::block_hash_map<int, int> a;
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
TEST(block_hash_map, iterate_erase)
{
    ska::block_hash_map<int, int> a;
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

TEST(block_hash_map, reallocate)
{
    ska::block_hash_map<int, int> a;
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

TEST(block_hash_map, range_erase)
{
    ska::block_hash_map<int, int> a;
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

TEST(block_hash_map, range_erase_power_of_two)
{
    ska::block_hash_map<int, int, ska::power_of_two_std_hash<int>> a;
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

TEST(block_hash_map, fuzz_find)
{
    ska::block_hash_map<int, int, ska::power_of_two_std_hash<int>> a;
    a[10496] = 1;
    a[-256] = 2;
    a.emplace(687865856, 3);
    a.emplace(16711680, 4);
    a.emplace(671088640, 5);
    a[18176] = 6;
    a.emplace(1077739520, 7);
    a.emplace(0, 8);
    a.emplace(1191902976, 9);
    a.emplace(-302055424, 10);
    a[9984] = 11;
    a.emplace(1157627904, 12);
    a.emplace(-49920, 13);
    a[10813440] = 14;
    a.emplace(1037172736, 15);
    a[-16777216] = 16;
    a[2555904] = 17;
    a[17664] = 18;
    auto third = std::next(a.begin(), 3);
    auto twelvth = std::next(a.begin(), 12);
    a.erase(third, twelvth);
    for (auto it = a.begin(), end = a.end(); it != end; ++it)
    {
        ASSERT_EQ(it, a.find(it->first));
    }
}

TEST(block_hash_map, fuzz_find2)
{
    ska::block_hash_map<int, int, ska::power_of_two_std_hash<int>> a;
    int count = 1;
    a.emplace(-16774656, count++);
    a.emplace(-16741591, count++);
    a.emplace(1049087, count++);
    a.emplace(65528, count++);
    a.emplace(-256, count++);
    a.emplace(687865856, count++);
    a.emplace(16711680, count++);
    a.emplace(671088640, count++);
    a.emplace(18176, count++);
    a.emplace(1077739520, count++);
    a.emplace(-1040187392, count++);
    a.emplace(1191902976, count++);
    a.emplace(-302055424, count++);
    a.emplace(9984, count++);
    a.emplace(704643072, count++);
    a.emplace(16449280, count++);
    a.emplace(1193803776, count++);
    a.emplace(71, count++);
    a.emplace(4209920, count++);
    a.emplace(12713984, count++);
    a.emplace(2815, count++);
    a.emplace(-1326579968, count++);
    a.emplace(39, count++);
    a.emplace(654311424, count++);
    a.emplace(-858993460, count++);
    a.emplace(30198988, count++);
    a.emplace(17235968, count++);
    a.emplace(1078001663, count++);
    a.emplace(32768, count++);
    a.emplace(1073742087, count++);
    a.emplace(16775168, count++);
    a.emplace(8, count++);
    a.emplace(9, count++);
    a.emplace(10, count++);
    a.emplace(11, count++);
    a.emplace(12, count++);
    a.emplace(13, count++);
    a.emplace(14, count++);
    a.emplace(15, count++);
    a.emplace(16, count++);
    a.emplace(17, count++);
    a.emplace(18, count++);
    a.emplace(19, count++);
    a.emplace(20, count++);
    a.emplace(21, count++);
    a.emplace(-16711616, count++);
    a.emplace(2701376, count++);
    a.emplace(1929408512, count++);
    a.emplace(1684369519, count++);
    a.emplace(3, count++);
    a.emplace(4, count++);
    a.emplace(5, count++);
    a.emplace(6, count++);
    a.emplace(7, count++);
    a.emplace(22, count++);
    a.emplace(23, count++);
    a.emplace(24, count++);
    a.emplace(25, count++);
    a.emplace(26, count++);
    a.emplace(27, count++);
    a.emplace(28, count++);
    a.emplace(29, count++);
    a.emplace(30, count++);
    a.emplace(31, count++);
    a.emplace(32, count++);
    a.emplace(33, count++);
    a.emplace(34, count++);
    a.emplace(35, count++);
    a.emplace(36, count++);
    a.emplace(37, count++);
    a.emplace(38, count++);
    a.emplace(40, count++);
    a.emplace(41, count++);
    a.emplace(42, count++);
    a.emplace(43, count++);
    a.emplace(44, count++);
    a.emplace(45, count++);
    a.emplace(46, count++);
    a.emplace(47, count++);
    a.emplace(134742016, count++);
    a.emplace(0, count++);
    a.emplace(1, count++);
    a.emplace(2, count++);
    a.emplace(48, count++);
    a.emplace(49, count++);
    a.emplace(50, count++);
    a.emplace(51, count++);
    a.emplace(52, count++);
    a.emplace(53, count++);
    a.emplace(54, count++);
    a.emplace(55, count++);
    a.emplace(56, count++);
    a.emplace(57, count++);
    a.emplace(58, count++);
    a.emplace(59, count++);
    a.emplace(60, count++);
    a.emplace(61, count++);
    a.emplace(62, count++);
    a.emplace(63, count++);
    a.emplace(64, count++);
    a.emplace(65, count++);
    a.emplace(66, count++);
    a.emplace(67, count++);
    a.emplace(68, count++);
    a.emplace(69, count++);
    a.emplace(70, count++);
    a.emplace(72, count++);
    a.emplace(73, count++);
    a.emplace(74, count++);
    a.emplace(75, count++);
    a.emplace(76, count++);
    a.emplace(77, count++);
    a.emplace(78, count++);
    a.emplace(79, count++);
    a.emplace(80, count++);
    a.emplace(81, count++);
    a.emplace(82, count++);
    a.emplace(83, count++);
    a.emplace(84, count++);
    a.emplace(85, count++);
    a.emplace(86, count++);
    a.emplace(87, count++);
    a.emplace(88, count++);
    a.emplace(89, count++);
    a.emplace(90, count++);
    a.emplace(91, count++);
    a.emplace(92, count++);
    a.emplace(93, count++);
    a.emplace(94, count++);
    a.emplace(95, count++);
    a.emplace(96, count++);
    a.emplace(97, count++);
    a.emplace(98, count++);
    a.emplace(99, count++);
    a.emplace(100, count++);
    a.emplace(101, count++);
    a.emplace(102, count++);
    a.emplace(103, count++);
    a.emplace(104, count++);
    a.emplace(1073777547, count++);
    a.emplace(-1953759233, count++);
    auto eighteenth = std::next(a.begin(), 18);
    auto fifty_fourth = std::next(a.begin(), 54);
    a.erase(eighteenth, fifty_fourth);
    a.erase(17235968);
    a.erase(32768);
    a.erase(16775168);
    a.erase(16);
    a.erase(64);
    a.erase(-16711616);
    a.erase(2701376);
    a.erase(1929408512);
    a.erase(32);
    a.erase(80);
    a.erase(134742016);
    a.erase(0);
    a.erase(96);
    a.erase(48);
    a.erase(18);
    a.erase(34);
    a.erase(2);
    a.erase(49);
    a.erase(50);
    a.erase(17);
    a.erase(65);
    a.erase(66);
    a.erase(33);
    a.erase(81);
    a.erase(82);
    a.erase(1);
    a.erase(97);
    a.erase(98);
    a.erase(19);
    a.erase(3);
    a.erase(35);
    a.erase(51);
    a.erase(67);
    a.erase(83);
    a.erase(99);
    a.erase(20);
    a.emplace(1728002314, count++);
    auto ninth = std::next(a.begin(), 9);
    auto sixteenth = std::next(a.begin(), 16);
    a.erase(ninth, sixteenth);
    a.erase(-302055424);
    a.erase(9984);
    a.erase(704643072);
    a.erase(16449280);
    a.erase(1193803776);
    a.erase(4209920);
    a.erase(12713984);
    a.emplace(100663295, count++);
    a.emplace(1701984569, count++);
    a.emplace(-4096, count++);
    a.emplace(-1962868737, count++);
    a.emplace(0, count++);
    a.emplace(-872405248, count++);
    a.emplace(460, count++);
    a.emplace(-16776953, count++);
    a.emplace(-2147483584, count++);
    a.emplace(-2130706688, count++);
    a.emplace(6302207, count++);
    a.emplace(48, count++);
    a.emplace(49, count++);
    a.emplace(50, count++);
    a.emplace(51, count++);
    a.emplace(64, count++);
    a.emplace(65, count++);
    a.emplace(66, count++);
    a.emplace(67, count++);
    a.emplace(80, count++);
    a.emplace(81, count++);
    a.emplace(82, count++);
    a.emplace(83, count++);
    a.emplace(2560, count++);
    a.emplace(-1960247041, count++);
    a.emplace(33489151, count++);
    a.emplace(-1953789184, count++);
    a.emplace(1, count++);
    a.emplace(2, count++);
    a.emplace(3, count++);
    a.emplace(-65281, count++);
    a.emplace(-1953789042, count++);
    a.emplace(16, count++);
    a.emplace(17, count++);
    a.emplace(18, count++);
    a.emplace(19, count++);
    a.emplace(20, count++);
    a.emplace(99, count++);
    a.emplace(105, count++);
    a.emplace(106, count++);
    a.erase(std::next(a.begin(), 97), std::next(a.begin(), 97));
    a.erase(std::next(a.begin(), 7), std::next(a.begin(), 97));
    for (auto it = a.begin(), end = a.end(); it != end; ++it)
    {
        ASSERT_EQ(it, a.find(it->first));
    }
}
