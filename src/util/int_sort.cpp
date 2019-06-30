#include <alloca.h>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>

template<typename It, typename Compare>
void IndexSort(It begin, It end, Compare compare)
{
    size_t num_items = end - begin;
    if (num_items <= 1)
        return;
    size_t num_items_minus_one = num_items - 1;
    using value_type = typename std::iterator_traits<It>::value_type;
    value_type * stack_values = static_cast<value_type *>(alloca(num_items * sizeof(value_type)));
    std::uninitialized_move(begin, end, stack_values);
    int * indices = static_cast<int *>(alloca(num_items * sizeof(int)));
    std::iota(indices, indices + num_items_minus_one, 0);
    indices[num_items_minus_one] = (num_items * num_items_minus_one) / 2;
    for (size_t i = 0; i < num_items_minus_one; ++i)
    {
        for (size_t j = i + 1; j < num_items_minus_one; ++j)
        {
            bool less = compare(stack_values[j], stack_values[i]);
            indices[i] += less;
            indices[j] -= less;
        }
        bool less = compare(stack_values[num_items_minus_one], stack_values[i]);
        indices[i] += less;
    }
    for (size_t i = 0;; ++i)
    {
        begin[indices[i]] = std::move(stack_values[i]);
        if (i == num_items_minus_one)
            break;
        indices[num_items_minus_one] -= indices[i];
    }
    std::destroy_n(stack_values, num_items);
}
template<typename It>
void IndexSort(It begin, It end)
{
    IndexSort(begin, end, std::less<>());
}

#ifndef DISABLE_GTEST

#include <vector>
#include <gtest/gtest.h>
#include <random>

template<typename T>
::testing::AssertionResult AssertIndexSortWorks(const std::vector<T> & to_sort)
{
    std::vector<T> tmp = to_sort;
    IndexSort(tmp.begin(), tmp.end());
    if (!std::is_sorted(tmp.begin(), tmp.end()))
        return ::testing::AssertionFailure() << "IntSort didn't sort";
    if (!std::is_permutation(tmp.begin(), tmp.end(), to_sort.begin(), to_sort.end()))
        return ::testing::AssertionFailure() << "IntSort didn't return the right values";
    return ::testing::AssertionSuccess();
}

TEST(index_sort, int_cases)
{
    std::vector<int> empty;
    ASSERT_TRUE(AssertIndexSortWorks(empty));

    std::vector<int> five = { -2, -5, 0, 100, 2 };
    ASSERT_TRUE(AssertIndexSortWorks(five));
}

TEST(index_sort, random_cases)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution;
    std::vector<int> to_sort;
    for (int i = 0; i < 32; ++i)
    {
        to_sort.push_back(distribution(randomness));
        ASSERT_TRUE(AssertIndexSortWorks(to_sort));
    }
}

TEST(index_sort, strings)
{
    std::vector<std::string> cases = { "foo", "bar", "baz", "hello", "world", "and a long string for good measure" };
    ASSERT_TRUE(AssertIndexSortWorks(cases));
}

TEST(index_sort, string_unicode)
{
    std::vector<std::string> order_a = { "néeimitativetangerinesweltersMendezcutunwontedBroadways", "naiveHonopportunistsfazedcoefficienteroding" };
    ASSERT_TRUE(AssertIndexSortWorks(order_a));
    std::vector<std::string> order_b = { "naiveHonopportunistsfazedcoefficienteroding", "néeimitativetangerinesweltersMendezcutunwontedBroadways" };
    ASSERT_TRUE(AssertIndexSortWorks(order_b));
}

#endif
