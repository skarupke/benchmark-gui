#include "custom_benchmark/custom_benchmark.h"
#include "hashtable_benchmarks/benchmark_shared.hpp"
#include <x86intrin.h> // for _bit_scan_forward

struct BitIterStartAtZero
{
    template<typename Func>
    int operator()(int bits, Func && func)
    {
        for (int offset = 0; bits; )
        {
            int bit = _bit_scan_forward(bits);
            offset += bit;
            if (func(offset))
                return offset;
            bits >>= bit + 1;
            ++offset;
        }
        return -1;
    }
};
struct BitIterPlusOne
{
    template<typename Func>
    int operator()(int bits, Func && func)
    {
        for (int offset = -1; bits; )
        {
            int bit = _bit_scan_forward(bits) + 1;
            offset += bit;
            if (func(offset))
                return offset;
            bits >>= bit;
        }
        return -1;
    }
};
struct BitIterNoIntrinsic
{
    template<typename Func>
    int operator()(int bits, Func && func)
    {
        for (int i = 0; i < 32; ++i)
        {
            if ((bits & 1) && func(i))
                return i;
            bits >>= 1;
        }
        return -1;
    }
};
struct BitIterMask
{
    template<typename Func>
    int operator()(int bits, Func && func)
    {
        while (bits)
        {
            int bit = _bit_scan_forward(bits);
            if (func(bit))
                return bit;
            bits &= 0xfffffffe << bit;
        }
        return -1;
    }
};


template<typename Func>
struct BenchmarkBitIterRandom
{
    void operator()(skb::State & state)
    {
        int success[32];
        std::uniform_int_distribution<int> distribution(0, state.range(0));
        for (int & s : success)
        {
            s = distribution(global_randomness);
        }
        int desired = success[0];
        std::shuffle(std::begin(success), std::end(success), global_randomness);
        Func func;
        while (state.KeepRunning())
        {
            int result = func(no_inline_random_number(distribution, global_randomness), [&](int index)
            {
                return success[index] == desired;
            });
            skb::DoNotOptimize(result);
        }
    }
};
template<typename Func>
struct BenchmarkBitIterFirst
{
    void operator()(skb::State & state)
    {
        int success[32];
        std::uniform_int_distribution<int> distribution(0, state.range(0));
        std::fill(std::begin(success), std::end(success), distribution(global_randomness));
        int desired = success[0];
        Func func;
        while (state.KeepRunning())
        {
            int result = func(no_inline_random_number(distribution, global_randomness), [&](int index)
            {
                return success[index] == desired;
            });
            skb::DoNotOptimize(result);
        }
    }
};
template<typename Func>
struct BenchmarkBitIterSecond
{
    void operator()(skb::State & state)
    {
        int success[32];
        std::uniform_int_distribution<int> distribution(0, state.range(0));
        std::fill(std::begin(success), std::end(success), distribution(global_randomness));
        int desired = success[0];
        Func func;
        while (state.KeepRunning())
        {
            bool found_one = false;
            int result = func(no_inline_random_number(distribution, global_randomness), [&](int index)
            {
                return success[index] == desired && std::exchange(found_one, true);
            });
            skb::DoNotOptimize(result);
        }
    }
};
void benchmark_bit_scan_forward(skb::State & state)
{
    std::uniform_int_distribution<int> distribution(0, state.range(0));
    while (state.KeepRunning())
    {
        skb::DoNotOptimize(_bit_scan_forward(no_inline_random_number(distribution, global_randomness)));
    }
}
void bit_iter_random_baseline(skb::State & state)
{
    std::uniform_int_distribution<int> distribution(0, state.range(0));
    while (state.KeepRunning())
    {
        skb::DoNotOptimize(no_inline_random_number(distribution, global_randomness));
    }
}
SKA_BENCHMARK("baseline", bit_iter_random_baseline);

static int bit_iter_range_min = 4;
static int bit_iter_range_max = std::numeric_limits<int>::max();
static double bit_iter_range_multiplier = 2.0f;
template<typename T>
void RegisterBitIterBenchmarks(std::string name)
{
    skb::BenchmarkCategories categories("instructions", name);
    categories.AddCategory("instruction", "bit_iter");
    skb::BenchmarkCategories first_categories = categories;
    first_categories.AddCategory("bit_iter desired", "first");
    skb::BenchmarkCategories second_categories = categories;
    second_categories.AddCategory("bit_iter desired", "second");
    categories.AddCategory("bit_iter desired", "random");
    BenchmarkBitIterRandom<T> random;
    BenchmarkBitIterFirst<T> first;
    BenchmarkBitIterSecond<T> second;
    SKA_BENCHMARK_CATEGORIES(random, categories)->SetBaseline("bit_iter_random_baseline")->SetRange(bit_iter_range_min, bit_iter_range_max)->SetRangeMultiplier(bit_iter_range_multiplier);
    SKA_BENCHMARK_CATEGORIES(first, first_categories)->SetBaseline("bit_iter_random_baseline")->SetRange(bit_iter_range_min, bit_iter_range_max)->SetRangeMultiplier(bit_iter_range_multiplier);
    SKA_BENCHMARK_CATEGORIES(second, second_categories)->SetBaseline("bit_iter_random_baseline")->SetRange(bit_iter_range_min, bit_iter_range_max)->SetRangeMultiplier(bit_iter_range_multiplier);
}

void RegisterBitIterBenchmarks()
{
    RegisterBitIterBenchmarks<BitIterPlusOne>("plus_one");
    RegisterBitIterBenchmarks<BitIterStartAtZero>("start_at_zero");
    RegisterBitIterBenchmarks<BitIterNoIntrinsic>("no_intrinsic");
    RegisterBitIterBenchmarks<BitIterMask>("mask");
    skb::BenchmarkCategories bit_scan_forward("instructions", "bit_scan_forward");
    bit_scan_forward.AddCategory("instruction", "bit_iter");
    SKA_BENCHMARK_CATEGORIES(&benchmark_bit_scan_forward, bit_scan_forward)->SetBaseline("bit_iter_random_baseline")->SetRange(bit_iter_range_min, bit_iter_range_max)->SetRangeMultiplier(bit_iter_range_multiplier);
}

#ifndef DISABLE_GTEST
#include <test/include_test.hpp>
struct BitIterTestData
{
    int to_test;
    std::vector<int> expected;
};
/*value, bit
1, 0
2, 1
4, 2
8, 3
16, 4
32, 5
64, 6
128, 7
256, 8
512, 9
1024, 10*/
static std::vector<BitIterTestData> test_data =
{
    { 0, {} },
    { 5, { 0, 2 } },
    { 1, { 0 } },
    { 2, { 1 } },
    { 513, { 0, 9 } },
    { 768, { 8, 9 } },
    { 1023, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } },
};

TEST(bit_iter, start_at_zero)
{
    BitIterStartAtZero bit_iter;
    for (BitIterTestData & test : test_data)
    {
        bit_iter(test.to_test, [&, current = 0](int index) mutable -> bool
        {
            EXPECT_LT(current, test.expected.size());
            EXPECT_EQ(test.expected[current], index);
            ++current;
            return false;
        });
    }
}
TEST(bit_iter, plus_one)
{
    BitIterPlusOne bit_iter;
    for (BitIterTestData & test : test_data)
    {
        bit_iter(test.to_test, [&, current = 0](int index) mutable -> bool
        {
            EXPECT_LT(current, test.expected.size());
            EXPECT_EQ(test.expected[current], index);
            ++current;
            return false;
        });
    }
}
TEST(bit_iter, no_intrinsic)
{
    BitIterNoIntrinsic bit_iter;
    for (BitIterTestData & test : test_data)
    {
        bit_iter(test.to_test, [&, current = 0](int index) mutable -> bool
        {
            EXPECT_LT(current, test.expected.size());
            EXPECT_EQ(test.expected[current], index);
            ++current;
            return false;
        });
    }
}
TEST(bit_iter, mask)
{
    BitIterMask bit_iter;
    for (BitIterTestData & test : test_data)
    {
        bit_iter(test.to_test, [&, current = 0](int index) mutable -> bool
        {
            EXPECT_LT(current, test.expected.size());
            EXPECT_EQ(test.expected[current], index);
            ++current;
            return false;
        });
    }
}
#endif
