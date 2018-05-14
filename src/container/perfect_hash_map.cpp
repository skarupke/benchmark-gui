#include "container/perfect_hash_map.hpp"


#include "test/include_test.hpp"

TEST(perfect_hash, simple)
{
    int i = 0;
    std::vector<std::pair<int, int>> keys =
    {
        { 5, i++ },
        { 3, i++ },
        { 2, i++ },
        { 4, i++ },
        { 6, i++ },
        { 1, i++ },
        { 8, i++ },
        { 16, i++ },
        { 32, i++ },
        { 48, i++ },
        { 64, i++ },
        { 80, i++ },
        { 96, i++ },
        { 128, i++ }
    };
    ska::perfect_hash_table<int, int> table(keys.begin(), keys.end());
    ASSERT_EQ(0, table[5]);
    ASSERT_EQ(6, table[8]);
    ASSERT_EQ(8, table[32]);
}
TEST(perfect_hash, wont_fit)
{
    int i = 0;
    std::vector<std::pair<int, int>> keys =
    {
        { 5, i++ },
        { 3, i++ },
        { 2, i++ },
        { 4, i++ },
        { 6, i++ },
        { 1, i++ },
        { 8, i++ },
        { 16, i++ },
        { 32, i++ },
        { 48, i++ },
        { 64, i++ },
        { 80, i++ },
        { 96, i++ },
        { 128, i++ },
        { 256, i++ },
        { 512, i++ },
        { 1024, i++ },
        { 2048, i++ },
        { 4096, i++ },
        { 8192, i++ }
    };
    ska::perfect_hash_table<int, int> table(keys.begin(), keys.end());
    ASSERT_EQ(0, table[5]);
    ASSERT_EQ(6, table[8]);
    ASSERT_EQ(8, table[32]);
    ASSERT_EQ(15, table[512]);
    ASSERT_EQ(18, table[4096]);
}

