#include "util/regions_sort_copy.hpp"

#include <random>


#ifndef DISABLE_GTEST
#include <test/include_test.hpp>

static constexpr std::ptrdiff_t DefaultAmericanFlagSortLimit = boost_ska_sort::sort::detail_ska_sort::default_sort_settings::american_flag_sort_upper_limit;
template<std::ptrdiff_t AmericanFlagSortLimit, std::ptrdiff_t SingleThreadedLimit = boost_ska_sort::sort::detail_ska_sort::default_sort_settings::single_threaded_sort_upper_limit>
struct RegionSortTestSetting : boost_ska_sort::sort::detail_ska_sort::default_sort_settings
{
    static constexpr std::ptrdiff_t american_flag_sort_upper_limit = AmericanFlagSortLimit;
    static constexpr std::ptrdiff_t single_threaded_sort_upper_limit = SingleThreadedLimit;
};

using AlwaysParallelSetting = RegionSortTestSetting<DefaultAmericanFlagSortLimit, 1>;
using ParallelSkaByteSortSetting = RegionSortTestSetting<1, 1>;
using ParallelAmericanFlagSortSetting = RegionSortTestSetting<std::numeric_limits<std::ptrdiff_t>::max(), 1>;

template<typename ToSort>
::testing::AssertionResult test_all_configurations(const std::vector<ToSort> & to_sort)
{
    std::vector<ToSort> copy = to_sort;
    {
        boost_ska_sort::sort::regions_sort(copy.begin(), copy.end());
        if (!std::is_sorted(copy.begin(), copy.end()))
            return ::testing::AssertionFailure() << "normal overload didn't sort";
    }
    copy = to_sort;
    {
        boost_ska_sort::sort::detail_ska_sort::ska_sort_with_settings<AlwaysParallelSetting>(copy.begin(), copy.end());
        if (!std::is_sorted(copy.begin(), copy.end()))
            return ::testing::AssertionFailure() << "parallel overload didn't sort";
    }
    copy = to_sort;
    {
        boost_ska_sort::sort::detail_ska_sort::ska_sort_with_settings<ParallelSkaByteSortSetting>(copy.begin(), copy.end());
        if (!std::is_sorted(copy.begin(), copy.end()))
            return ::testing::AssertionFailure() << "parallel byte sort overload didn't sort";
    }
    copy = to_sort;
    {
        boost_ska_sort::sort::detail_ska_sort::ska_sort_with_settings<ParallelAmericanFlagSortSetting>(copy.begin(), copy.end());
        if (!std::is_sorted(copy.begin(), copy.end()))
            return ::testing::AssertionFailure() << "parallel american flag sort overload didn't sort";
    }
    return ::testing::AssertionSuccess();
}

TEST(regions_sort, empty)
{
    std::vector<int> to_sort;
    ASSERT_TRUE(test_all_configurations(to_sort));
}

TEST(regions_sort, simple)
{
    std::vector<int> to_sort = { 5, 2, 4, 6, 9, 1, 2, 4, 6, 9, 5, 2, 4, 6, 9, 1, 2, 4, 6, 9, 5, 2, 4, 6, 9, 1, 2, 4, 6, 9, 1, 2, 4, 6, 9, 1, 2, 4, 6, 9, 5, 2, 4, 6, 9, 1, 2, 4, 6, 9, 5, 2, 4, 6, 9, 1, 2, 4, 6, 9, 1 };
    ASSERT_TRUE(test_all_configurations(to_sort));
}

TEST(regions_sort, small)
{
    std::vector<int> to_sort = { 5, 2, 4, 6 };
    ASSERT_TRUE(test_all_configurations(to_sort));
}

TEST(regions_sort, large_random)
{
    std::vector<int> to_sort;
    to_sort.reserve(16384);
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution;
    while (to_sort.size() != to_sort.capacity())
        to_sort.push_back(distribution(randomness));
    ASSERT_TRUE(test_all_configurations(to_sort));
}

#endif

