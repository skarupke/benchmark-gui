#include "util/radix_sort.hpp"

#ifndef DISABLE_TESTS
#include <test/include_test.hpp>

/*#ifndef SIMPLE_COMMON_PREFIX
using detail::PartitionByCommonPrefix;

TEST(common_prefix, first_character_different)
{
    std::vector<std::string> a = { "a", "b" };
    auto common_prefix = PartitionByCommonPrefix(a.begin(), a.end(), 0, detail::IdentityFunctor(), detail::IdentityFunctor());
    ASSERT_EQ(0u, common_prefix.common_prefix_index);
    ASSERT_EQ(a.begin(), common_prefix.shorter_elements);
    ASSERT_EQ(a.begin(), common_prefix.elements_ending_after_common_prefix);
}

TEST(common_prefix, second_character_different_start_at_0)
{
    std::vector<std::string> a = { "ab", "ac" };
    auto common_prefix = PartitionByCommonPrefix(a.begin(), a.end(), 0, detail::IdentityFunctor(), detail::IdentityFunctor());
    ASSERT_EQ(1u, common_prefix.common_prefix_index);
    ASSERT_EQ(a.begin(), common_prefix.shorter_elements);
    ASSERT_EQ(a.begin(), common_prefix.elements_ending_after_common_prefix);
}
TEST(common_prefix, longer_then_shorter)
{
    std::vector<std::string> a = { "abc", "ab", "a" };
    auto common_prefix = PartitionByCommonPrefix(a.begin(), a.end(), 1, detail::IdentityFunctor(), detail::IdentityFunctor());
    ASSERT_EQ(2u, common_prefix.common_prefix_index);
    ASSERT_EQ(a.begin() + 1, common_prefix.shorter_elements);
    ASSERT_EQ(a.begin() + 2, common_prefix.elements_ending_after_common_prefix);
    ASSERT_EQ("a", a[0]);
    ASSERT_EQ("ab", a[1]);
    ASSERT_EQ("abc", a[2]);
}

TEST(common_prefix, second_character_different_start_at_1)
{
    std::vector<std::string> a = { "ab", "ac" };
    auto common_prefix = PartitionByCommonPrefix(a.begin(), a.end(), 1, detail::IdentityFunctor(), detail::IdentityFunctor());
    ASSERT_EQ(1u, common_prefix.common_prefix_index);
    ASSERT_EQ(a.begin(), common_prefix.shorter_elements);
    ASSERT_EQ(a.begin(), common_prefix.elements_ending_after_common_prefix);
}

TEST(common_prefix, second_character_different_start_at_0_also_shorter)
{
    std::vector<std::string> a = { "ab", "ac", "a" };
    auto common_prefix = PartitionByCommonPrefix(a.begin(), a.end(), 0, detail::IdentityFunctor(), detail::IdentityFunctor());
    ASSERT_EQ(1u, common_prefix.common_prefix_index);
    ASSERT_EQ(a.begin(), common_prefix.shorter_elements);
    ASSERT_EQ(a.begin() + 1, common_prefix.elements_ending_after_common_prefix);
    ASSERT_EQ("a", a.front());
}

TEST(common_prefix, second_character_different_start_at_1_also_shorter)
{
    std::vector<std::string> a = { "ab", "ac", "a" };
    auto common_prefix = PartitionByCommonPrefix(a.begin(), a.end(), 1, detail::IdentityFunctor(), detail::IdentityFunctor());
    ASSERT_EQ(1u, common_prefix.common_prefix_index);
    ASSERT_EQ(a.begin() + 1, common_prefix.shorter_elements);
    ASSERT_EQ(a.begin() + 1, common_prefix.elements_ending_after_common_prefix);
    ASSERT_EQ("a", a.front());
}

TEST(common_prefix, second_character_different_start_at_0_also_shorter_and_empty)
{
    std::vector<std::string> a = { "ab", "ac", "a", "" };
    auto common_prefix = PartitionByCommonPrefix(a.begin(), a.end(), 0, detail::IdentityFunctor(), detail::IdentityFunctor());
    ASSERT_EQ(1u, common_prefix.common_prefix_index);
    ASSERT_EQ(a.begin() + 1, common_prefix.shorter_elements);
    ASSERT_EQ(a.begin() + 2, common_prefix.elements_ending_after_common_prefix);
    ASSERT_EQ("", a[0]);
    ASSERT_EQ("a", a[1]);
}

TEST(common_prefix, fifth_character_different)
{
    std::vector<std::string> a = { "abcde", "abcdd", "abcd", "abcde" };
    auto common_prefix = PartitionByCommonPrefix(a.begin(), a.end(), 1, detail::IdentityFunctor(), detail::IdentityFunctor());
    ASSERT_EQ(4u, common_prefix.common_prefix_index);
    ASSERT_EQ(a.begin(), common_prefix.shorter_elements);
    ASSERT_EQ(a.begin() + 1, common_prefix.elements_ending_after_common_prefix);
    ASSERT_EQ("abcd", a.front());
}

TEST(common_prefix, error_case)
{
    std::vector<std::string> a = { "sexagenarianpoorestWyomingite", "sexuallyjeepsdietitian", "", "" };
    auto common_prefix = PartitionByCommonPrefix(a.begin(), a.end(), 0, detail::IdentityFunctor(), detail::IdentityFunctor());
    ASSERT_EQ(3u, common_prefix.common_prefix_index);
    ASSERT_EQ(a.begin() + 2, common_prefix.shorter_elements);
    ASSERT_EQ(a.begin() + 2, common_prefix.elements_ending_after_common_prefix);
    ASSERT_EQ("", a[0]);
    ASSERT_EQ("", a[1]);
}
#endif*/

#endif
