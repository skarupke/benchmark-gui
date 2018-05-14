#include "benchmark/benchmark.h"
#include <algorithm>
#include <vector>
#include <iostream>
#include <random>

template<typename It>
inline void iter_swap3(It a, It b, It c)
{
    auto tmp = std::move(*a);
    *a = std::move(*b);
    *b = std::move(*c);
    *c = std::move(tmp);
}

template<typename It, typename Comp>
inline void median3(It a, It b, It c, Comp && comp)
{
    if (comp(*b, *a))
    {
        if (comp(*c, *b))
        {
            // c < b < a
            std::iter_swap(a, c);
        }
        else if (comp(*c, *a))
        {
            // b < c < a
            iter_swap3(a, b, c);
        }
        else
        {
            // b < a < c
            std::iter_swap(a, b);
        }
    }
    else if (comp(*c, *a))
    {
        // c < a < b
        iter_swap3(a, c, b);
    }
    else if (comp(*c, *b))
    {
        // a < c < b
        std::iter_swap(b, c);
    }
}

template<typename It, typename Comp>
inline void lower_median4(It a, It b, It c, It d, Comp && comp)
{
    if (comp(*d, *c))
        std::iter_swap(c, d);
    if (comp(*d, *b))
        std::iter_swap(b, d);
    median3(a, b, c, comp);
}

template<typename It, typename Comp>
inline void upper_median4(It a, It b, It c, It d, Comp && comp)
{
    if (comp(*b, *a))
        std::iter_swap(a, b);
    if (comp(*c, *a))
        std::iter_swap(a, c);
    median3(b, c, d, comp);
}

template<typename It, typename Comp>
It partition_on(It begin, It to_partition, It end, Comp && comp)
{
    auto mid_val = std::move(*to_partition);
    *to_partition = std::move(*begin);
    It it = begin + 1;
    for (;;)
    {
        --end;
        for (;;)
        {
            if (it > end)
                goto end_loop;
            else if (!comp(*it, mid_val))
                break;
            ++it;
        }
        while (comp(mid_val, *end))
        {
            --end;
            if (it >= end)
                goto end_loop;
        }
        std::iter_swap(it, end);
        ++it;
    }
    end_loop:
    --it;
    *begin = std::move(*it);
    *it = std::move(mid_val);
    return it;
}

template<typename It, typename Comp>
It expand_partition(It begin, It begin_mid, It mid, It end_mid, It end, Comp && comp)
{
    auto mid_val = std::move(*mid);
    for (--end, --end_mid;; ++begin, --end)
    {
        for (;; ++begin)
        {
            if (begin == begin_mid)
                goto first_done;
            if (comp(mid_val, *begin))
                break;
        }
        for (;; --end)
        {
            if (end == end_mid)
                goto second_done;
            if (comp(*end, mid_val))
                break;
        }
        std::iter_swap(begin, end);
    }
    first_done:
    for (;;)
    {
        if (comp(*end, mid_val))
        {
            *mid = std::move(*end);
            if (mid == end_mid)
            {
                ++end_mid;
                if (end_mid == end)
                {
                    *end = std::move(mid_val);
                    return end;
                }
            }
            ++mid;
            *end = std::move(*mid);
        }
        else
        {
            --end;
            if (end <= end_mid)
            {
                *mid = std::move(mid_val);
                return mid;
            }
        }
    }
    for (;;)
    {
        if (comp(mid_val, *begin))
        {
second_done:
            *mid = std::move(*begin);
            if (mid == begin_mid)
            {
                --begin_mid;
                if (begin_mid == begin)
                {
                    *begin = std::move(mid_val);
                    return begin;
                }
            }
            --mid;
            *begin = std::move(*mid);
        }
        else
        {
            ++begin;
            if (begin >= begin_mid)
            {
                *mid = std::move(mid_val);
                return mid;
            }
        }
    }
}

template<typename It, typename PartitionStep, typename Comp>
void quickselect(It begin, It nth, It end, PartitionStep && partition, Comp && comp)
{
    for (;;)
    {
        It mid = partition(begin, nth, end, comp);
        if (mid == nth)
            return;
        else if (mid > nth)
            end = mid;
        else
            begin = mid + 1;
    }
}

struct repeated_step_adaptive
{
    template<typename It, typename Comp>
    It operator()(It begin, It nth, It end, Comp && comp) const
    {
        std::ptrdiff_t num_elements = end - begin;
        if (num_elements <= 1)
            return nth;
        else if (num_elements < 12)
            return partition_on(begin, begin + num_elements / 2, end, comp);
        std::ptrdiff_t num_nth = nth - begin;
        if (num_nth <= num_elements / 12)
            return partition_far_left(begin, end, num_elements, num_nth, comp);
        else if (num_nth * 16 < num_elements * 7)
            return partition_left(begin, end, num_elements, num_nth, comp);
        else if (num_nth >= num_elements - num_elements / 12)
            return partition_far_right(begin, end, num_elements, num_nth, comp);
        else if (num_nth * 16 > num_elements * 9)
            return partition_right(begin, end, num_elements, num_nth, comp);
        else
            return partition_middle(begin, end, num_elements, num_nth, comp);
    }

    template<typename It, typename Comp>
    __attribute__((noinline)) It partition_middle(It begin, It end, std::ptrdiff_t num_elements, std::ptrdiff_t num_nth, Comp && comp) const
    {
        std::ptrdiff_t one_ninth = num_elements / 9;
        std::ptrdiff_t one_third = one_ninth * 3;
        It one_third_it = begin + one_third;
        for (It it0 = begin, it1 = one_third_it, it2 = one_third_it + one_third; it0 != one_third_it; ++it0, ++it1, ++it2)
        {
            median3(it0, it1, it2, comp);
        }
        It four_ninth_it = one_third_it + one_ninth;
        It five_ninth_it = four_ninth_it + one_ninth;
        for (It it0 = one_third_it, it1 = four_ninth_it, it2 = five_ninth_it; it0 != four_ninth_it; ++it0, ++it1, ++it2)
        {
            median3(it0, it1, it2, comp);
        }
        It new_mid = four_ninth_it + ((num_nth * one_ninth) / num_elements);
        quickselect(four_ninth_it, new_mid, five_ninth_it, *this, comp);
        return expand_partition(begin, four_ninth_it, new_mid, five_ninth_it, end, comp);
    }

    template<typename It, typename Comp>
    __attribute__((noinline)) It partition_left(It begin, It end, std::ptrdiff_t num_elements, std::ptrdiff_t num_nth, Comp && comp) const
    {
        std::ptrdiff_t three_twelvth = num_elements / 4;
        It three_twelvth_it = begin + three_twelvth;
        for (It it0 = begin, it1 = three_twelvth_it, it2 = three_twelvth_it + three_twelvth, it3 = it2 + three_twelvth; it0 != three_twelvth_it; ++it0, ++it1, ++it2, ++it3)
        {
            lower_median4(it0, it1, it2, it3, comp);
        }
        std::ptrdiff_t one_twelvth = three_twelvth / 3;
        It four_twelvth_it = three_twelvth_it + one_twelvth;
        It five_twelvth_it = four_twelvth_it + one_twelvth;
        for (It it0 = three_twelvth_it, it1 = four_twelvth_it, it2 = five_twelvth_it; it0 != four_twelvth_it; ++it0, ++it1, ++it2)
        {
            median3(it0, it1, it2, comp);
        }
        It new_mid = four_twelvth_it + ((num_nth * one_twelvth) / num_elements);
        quickselect(four_twelvth_it, new_mid, five_twelvth_it, *this, comp);
        return expand_partition(begin, four_twelvth_it, new_mid, five_twelvth_it, end, comp);
    }

    template<typename It, typename Comp>
    __attribute__((noinline)) It partition_right(It begin, It end, std::ptrdiff_t num_elements, std::ptrdiff_t num_nth, Comp && comp) const
    {
        std::ptrdiff_t three_twelvth = num_elements / 4;
        It three_twelvth_it = begin + three_twelvth;
        for (It it0 = begin, it1 = three_twelvth_it, it2 = three_twelvth_it + three_twelvth, it3 = it2 + three_twelvth; it0 != three_twelvth_it; ++it0, ++it1, ++it2, ++it3)
        {
            upper_median4(it0, it1, it2, it3, comp);
        }
        std::ptrdiff_t one_twelvth = three_twelvth / 3;
        It six_twelvth_it = three_twelvth_it + three_twelvth;
        It seven_twelvth_it = six_twelvth_it + one_twelvth;
        It eight_twelvth_it = seven_twelvth_it + one_twelvth;
        for (It it0 = six_twelvth_it, it1 = seven_twelvth_it, it2 = eight_twelvth_it; it0 != seven_twelvth_it; ++it0, ++it1, ++it2)
        {
            median3(it0, it1, it2, comp);
        }
        It new_mid = seven_twelvth_it + ((num_nth * one_twelvth) / num_elements);
        quickselect(seven_twelvth_it, new_mid, eight_twelvth_it, *this, comp);
        return expand_partition(begin, seven_twelvth_it, new_mid, eight_twelvth_it, end, comp);
    }

    template<typename It, typename Comp>
    __attribute__((noinline)) It partition_far_left(It begin, It end, std::ptrdiff_t num_elements, std::ptrdiff_t num_nth, Comp && comp) const
    {
        std::ptrdiff_t three_twelvth = num_elements / 4;
        It three_twelvth_it = begin + three_twelvth;
        for (It it0 = begin, it1 = three_twelvth_it, it2 = three_twelvth_it + three_twelvth, it3 = it2 + three_twelvth; it0 != three_twelvth_it; ++it0, ++it1, ++it2, ++it3)
        {
            lower_median4(it0, it1, it2, it3, comp);
        }
        std::ptrdiff_t one_twelvth = three_twelvth / 3;
        It four_twelvth_it = three_twelvth_it + one_twelvth;
        for (It it0 = three_twelvth_it, it1 = four_twelvth_it, it2 = it1 + one_twelvth; it0 != four_twelvth_it; ++it0, ++it1, ++it2)
        {
            if (comp(*it1, *it0))
                std::iter_swap(it0, it1);
            if (comp(*it2, *it0))
                std::iter_swap(it0, it2);
        }
        It new_mid = three_twelvth_it + ((num_nth * one_twelvth) / num_elements);
        quickselect(three_twelvth_it, new_mid, four_twelvth_it, *this, comp);
        return expand_partition(begin, three_twelvth_it, new_mid, four_twelvth_it, end, comp);
    }
    template<typename It, typename Comp>
    __attribute__((noinline)) It partition_far_right(It begin, It end, std::ptrdiff_t num_elements, std::ptrdiff_t num_nth, Comp && comp) const
    {
        std::ptrdiff_t three_twelvth = num_elements / 4;
        It three_twelvth_it = begin + three_twelvth;
        for (It it0 = begin, it1 = three_twelvth_it, it2 = three_twelvth_it + three_twelvth, it3 = it2 + three_twelvth; it0 != three_twelvth_it; ++it0, ++it1, ++it2, ++it3)
        {
            upper_median4(it0, it1, it2, it3, comp);
        }
        std::ptrdiff_t one_twelvth = three_twelvth / 3;
        It six_twelvth_it = three_twelvth_it + three_twelvth;
        It seven_twelvth_it = six_twelvth_it + one_twelvth;
        It eight_twelvth_it = seven_twelvth_it + one_twelvth;
        It nine_twelvth_it = six_twelvth_it + three_twelvth;
        for (It it0 = six_twelvth_it, it1 = seven_twelvth_it, it2 = eight_twelvth_it; it0 != seven_twelvth_it; ++it0, ++it1, ++it2)
        {
            if (comp(*it2, *it0))
                std::iter_swap(it0, it2);
            if (comp(*it2, *it1))
                std::iter_swap(it1, it2);
        }
        It new_mid = eight_twelvth_it + ((num_nth * one_twelvth) / num_elements);
        quickselect(eight_twelvth_it, new_mid, nine_twelvth_it, *this, comp);
        return expand_partition(begin, eight_twelvth_it, new_mid, nine_twelvth_it, end, comp);
    }
};

template<typename It>
void quickselect(It begin, It nth, It end)
{
    return quickselect(begin, nth, end, repeated_step_adaptive(), std::less<>());
}

template<typename It>
void faster_partial_sort(It begin, It middle, It end)
{
    if (begin == middle)
        return;
    --middle;
    std::nth_element(begin, middle, end);
    std::sort(begin, middle);
}


std::vector<int64_t> build_test_data(int size)
{
    std::mt19937_64 randomness(1232123432424);
    std::uniform_int_distribution<int64_t> ints;
    std::vector<int64_t> result(size);
    for (int64_t & i : result)
    {
        i = ints(randomness);
    }
    return result;
}

template<typename It>
void d_partial_sort(It begin, It middle, It end)
{
    if (begin == middle)
        return;
    --middle;
    quickselect(begin, middle, end);
    std::sort(begin, middle);
}

static constexpr int benchmark_multiplier = 256;
static constexpr int max_benchmark_range = 1 << 27;

void benchmark_d_partial_sort(benchmark::State & state)
{
    while (state.KeepRunning())
    {
        auto test_data = build_test_data(state.range(0));
        d_partial_sort(test_data.begin(), test_data.begin() + test_data.size() / 10, test_data.end());
        benchmark::DoNotOptimize(*test_data.data());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(benchmark_d_partial_sort)->RangeMultiplier(benchmark_multiplier)->Range(benchmark_multiplier, max_benchmark_range);

void benchmark_faster_partial_sort(benchmark::State & state)
{
    while (state.KeepRunning())
    {
        auto test_data = build_test_data(state.range(0));
        faster_partial_sort(test_data.begin(), test_data.begin() + test_data.size() / 100, test_data.end());
        benchmark::DoNotOptimize(*test_data.data());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(benchmark_faster_partial_sort)->RangeMultiplier(benchmark_multiplier)->Range(benchmark_multiplier, max_benchmark_range);


void benchmark_std_sort(benchmark::State & state)
{
    while (state.KeepRunning())
    {
        auto test_data = build_test_data(state.range(0));
        std::sort(test_data.begin(), test_data.end());
        benchmark::DoNotOptimize(*test_data.data());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(benchmark_std_sort)->RangeMultiplier(benchmark_multiplier)->Range(benchmark_multiplier, max_benchmark_range);

void benchmark_baseline(benchmark::State & state)
{
    while (state.KeepRunning())
    {
        auto test_data = build_test_data(state.range(0));
        benchmark::DoNotOptimize(*test_data.data());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(benchmark_baseline)->RangeMultiplier(benchmark_multiplier)->Range(benchmark_multiplier, max_benchmark_range);

void benchmark_partial_sort(benchmark::State & state)
{
    while (state.KeepRunning())
    {
        auto test_data = build_test_data(state.range(0));
        std::partial_sort(test_data.begin(), test_data.begin() + test_data.size() / 100, test_data.end());
        benchmark::DoNotOptimize(*test_data.data());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(benchmark_partial_sort)->RangeMultiplier(benchmark_multiplier)->Range(benchmark_multiplier, max_benchmark_range);


#include <gtest/gtest.h>

template<typename It>
::testing::AssertionResult TestPartition(It begin, It to_partition, It end)
{
    auto original = *to_partition;
    It new_it = partition_on(begin, to_partition, end, std::less<>());
    if (new_it == end)
        return ::testing::AssertionFailure() << "partition_on return end";
    if (*new_it != original)
        return ::testing::AssertionFailure() << "partition returned different value";
    for (It it = begin; it != new_it; ++it)
    {
        if (*it > original)
            return ::testing::AssertionFailure() << "partition put a larger element first";
    }
    for (It it = new_it + 1; it != end; ++it)
    {
        if (*it < original)
            return ::testing::AssertionFailure() << "partition put a smaller element later";
    }
    return ::testing::AssertionSuccess();
}

TEST(partition_on, many)
{
    std::mt19937_64 randomness(179);
    std::uniform_int_distribution<int> distribution;
    float percentiles[] = { 0.02f, 0.1f, 0.25f, 0.5f, 0.75f, 0.9f, 0.98f };
    float * current_percentile = percentiles;
    for (int i = 1; i < 1000; ++i)
    {
        std::vector<int> ints(i);
        std::generate(ints.begin(), ints.end(), [&]{ return distribution(randomness); });
        int index = static_cast<int>(ints.size() * *current_percentile);
        ++current_percentile;
        if (current_percentile == std::end(percentiles))
            current_percentile = percentiles;
        auto mid = ints.begin() + index;
        ASSERT_TRUE(TestPartition(ints.begin(), mid, ints.end()));
    }
}

TEST(expand_partition, simple)
{
    std::vector<int> input = { 3, 2, 1, 1, 2, 3, 3, 2, 1, 1 };
    auto mid = expand_partition(input.begin(), input.begin() + 3, input.begin() + 4, input.begin() + 6, input.end(), std::less<>());
    ASSERT_EQ(2, *mid);
    for (auto it = input.begin(); it != mid; ++it)
    {
        ASSERT_LE(*it, *mid);
    }
    for (auto it = mid + 1; it != input.end(); ++it)
    {
        ASSERT_LE(*mid, *it);
    }
}


TEST(quickselect, simple)
{
    std::vector<int> input = { 1, 5, 3, 2, 5, 7, 8, 2, 3, 4, 5 };
    auto mid = input.begin() + 4;
    quickselect(input.begin(), mid, input.end());
    for (auto it = input.begin(); it != mid; ++it)
    {
        EXPECT_LE(*it, *mid);
    }
    for (auto it = mid + 1, end = input.end(); it != end; ++it)
    {
        EXPECT_LE(*mid, *it);
    }
}

TEST(quickselect, many)
{
    std::mt19937_64 randomness(179);
    std::uniform_int_distribution<int> distribution;
    float percentiles[] = { 0.02f, 0.1f, 0.25f, 0.5f, 0.75f, 0.9f, 0.98f };
    float * current_percentile = percentiles;
    for (int i = 1; i < 1000; ++i)
    {
        std::vector<int> ints(i);
        std::generate(ints.begin(), ints.end(), [&]{ return distribution(randomness); });
        int index = static_cast<int>(ints.size() * *current_percentile);
        ++current_percentile;
        if (current_percentile == std::end(percentiles))
            current_percentile = percentiles;
        auto mid = ints.begin() + index;
        quickselect(ints.begin(), mid, ints.end());
        for (auto it = ints.begin(); it != mid; ++it)
        {
            EXPECT_LE(*it, *mid);
        }
        for (auto it = mid + 1, end = ints.end(); it != end; ++it)
        {
            EXPECT_LE(*mid, *it);
        }
    }
}
