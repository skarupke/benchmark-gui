#include "util/binary_search.hpp"
#include "hashtable_benchmarks/benchmark_shared.hpp"
#include "util/binary_search_erthink.hpp"

namespace
{
struct Standard
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return std::lower_bound(begin, end, val, cmp);
    }
};
struct Linear
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return std::find_if(begin, end, [&](const auto & x){ return !cmp(x, val); });
    }
};
struct Custom
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return custom_lower_bound(begin, end, val, cmp);
    }
};
struct PlusLinear
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return lower_bound_plus_linear(begin, end, val, cmp);
    }
};
struct Branchless
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return branchless_lower_bound(begin, end, val, cmp);
    }
};
struct BranchlessOdd
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return branchless_lower_bound_odd(begin, end, val, cmp);
    }
};
struct BrokenBranchlessNonPow
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return broken_branchless_lower_bound_non_pow(begin, end, val, cmp);
    }
};
struct BranchlessIdx
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return branchless_lower_bound_idx(begin, end, val, cmp);
    }
};
struct BranchlessUnroll2
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return branchless_lower_bound_unroll2(begin, end, val, cmp);
    }
};
struct BranchlessUnroll4
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return branchless_lower_bound_unroll4(begin, end, val, cmp);
    }
};
struct BranchlessUnrollAll
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return branchless_lower_bound_unroll_all(begin, end, val, cmp);
    }
};
struct Branchless2
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return branchless_lower_bound2(begin, end, val, cmp);
    }
};
struct Branchless3
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return branchless_lower_bound3(begin, end, val, cmp);
    }
};
struct Branchless4
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return branchless_lower_bound4(begin, end, val, cmp);
    }
};
struct Algorithmica
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return algorithmica_lower_bound(begin, end, val, cmp);
    }
};
struct AlgorithmicaPrefetch
{
    template<typename It, typename T, typename C>
    auto operator()(It begin, It end, const T & val, C && cmp)
    {
        return algorithmica_lower_bound_prefetch(begin, end, val, cmp);
    }
};
struct ErthinkLowerBound
{
    template<typename It, typename T, typename C>
    It operator()(It begin, It end, const T & val, C && cmp)
    {
        return erthink_lower_bound(begin, end, val, cmp);
    }
};
struct ErthinkLowerBoundTight
{
    template<typename It, typename T, typename C>
    It operator()(It begin, It end, const T & val, C && cmp)
    {
        return erthink_lower_bound_tight(begin, end, val, cmp);
    }
};
struct ErthinkMini0
{
    template<typename It, typename T, typename C>
    It operator()(It begin, It end, const T & val, C && cmp)
    {
        auto ptr_begin = std::addressof(*begin);
        return begin + (ly_bl_mini0(ptr_begin, end - begin, val, cmp) - ptr_begin);
    }
};
struct ErthinkMini1
{
    template<typename It, typename T, typename C>
    It operator()(It begin, It end, const T & val, C &&)
    {
        if (begin == end)
            return end;
        auto ptr_begin = std::addressof(*begin);
        return begin + (ly_bl_mini1(ptr_begin, end - begin, val) - ptr_begin);
    }
};
struct ErthinkMini2
{
    template<typename It, typename T, typename C>
    It operator()(It begin, It end, const T & val, C &&)
    {
        if (begin == end)
            return end;
        auto ptr_begin = std::addressof(*begin);
        return begin + (ly_bl_mini2(ptr_begin, end - begin, val) - ptr_begin);
    }
};
struct ErthinkMini3
{
    template<typename It, typename T, typename C>
    It operator()(It begin, It end, const T & val, C &&)
    {
        if (begin == end)
            return end;
        auto ptr_begin = std::addressof(*begin);
        return begin + (ly_bl_mini3(ptr_begin, end - begin, val) - ptr_begin);
    }
};
struct ErthinkGoto
{
    template<typename It, typename T, typename C>
    It operator()(It begin, It end, const T & val, C &&)
    {
        auto ptr_begin = std::addressof(*begin);
        return begin + (ly_bl_clz_goto(ptr_begin, end - begin, val) - ptr_begin);
    }
};
struct ErthinkSwitch
{
    template<typename It, typename T, typename C>
    It operator()(It begin, It end, const T & val, C &&)
    {
        auto ptr_begin = std::addressof(*begin);
        return begin + (ly_bl_clz_switch(ptr_begin, end - begin, val) - ptr_begin);
    }
};
struct CmpFirst
{
    template<typename It, typename T, typename C>
    It operator()(It begin, It end, const T & val, C && cmp)
    {
        return lower_bound_cmp_first(begin, end, val, cmp);
    }
};
struct SkipLast
{
    template<typename It, typename T, typename C>
    It operator()(It begin, It end, const T & val, C && cmp)
    {
        return lower_bound_skip_last(begin, end, val, cmp);
    }
};

template<typename T>
struct FindDistribution : BenchmarkRandomDistribution<T>
{
    using BenchmarkRandomDistribution<T>::BenchmarkRandomDistribution;
};

template<>
struct FindDistribution<std::string>
{
    const char * operator()(profiling_randomness & randomness)
    {
        return word_list[random_word(randomness)];
    }

private:
    const std::vector<const char *> & word_list = get_word_list();
    std::uniform_int_distribution<size_t> random_word{0, word_list.size() - 1};
};


template<typename T, typename Func>
struct BenchmarkBinarySearchRandom
{
    void operator()(skb::State & state)
    {
        BenchmarkRandomDistribution<T> generate_distribution;
        FindDistribution<T> find_distribution;
        std::vector<T> data;
        data.reserve(state.range(0));
        while (data.size() != data.capacity())
        {
            data.push_back(no_inline_random_number(generate_distribution, global_randomness));
        }
        auto begin = data.begin();
        auto end = data.end();
        std::sort(begin, end);
        Func func;
        while (state.KeepRunning())
        {
            auto result = func(begin, end, no_inline_random_number(find_distribution, global_randomness), std::less<>{});
            skb::DoNotOptimize(result - begin);
        }
    }
};

void binary_search_random_baseline_int(skb::State & state)
{
    BenchmarkRandomDistribution<int> random_index;
    while (state.KeepRunning())
    {
        skb::DoNotOptimize(no_inline_random_number(random_index, global_randomness));
    }
}
void binary_search_random_baseline_string(skb::State & state)
{
    FindDistribution<std::string> distribution;
    while (state.KeepRunning())
    {
        skb::DoNotOptimize(reinterpret_cast<size_t>(no_inline_random_number(distribution, global_randomness)));
    }
}


template<typename T, typename Func>
void RegisterBinarySearchBenchmarks(interned_string type_name, skb::BenchmarkCategories categories)
{
    static int range_min = 4;
    static int range_max = 8*1024*1024;
    static double range_multiplier = std::pow(2.0, 0.125);
    categories.AddCategory("key", type_name);
    BenchmarkBinarySearchRandom<T, Func> random;
    SKA_BENCHMARK_CATEGORIES(random, categories)->SetBaseline("binary_search_random_baseline_" + type_name)->SetRange(range_min, range_max)->SetRangeMultiplier(range_multiplier);
}

}

template<typename T>
void RegisterForType(interned_string type_name)
{
    auto categories = [](interned_string name){ return skb::BenchmarkCategories("binary_search", name); };
    RegisterBinarySearchBenchmarks<T, Standard>(type_name, categories("std::lower_bound"));
    RegisterBinarySearchBenchmarks<T, Linear>(type_name, categories("std::find"));
    RegisterBinarySearchBenchmarks<T, Branchless>(type_name, categories("shar"));
    RegisterBinarySearchBenchmarks<T, BranchlessOdd>(type_name, categories("shar_odd"));
    //RegisterBinarySearchBenchmarks<T, BrokenBranchlessNonPow>(type_name, categories("shar_non_pow"));
    RegisterBinarySearchBenchmarks<T, BranchlessIdx>(type_name, categories("shar_idx"));
    RegisterBinarySearchBenchmarks<T, BranchlessUnroll2>(type_name, categories("shar_unroll2"));
    RegisterBinarySearchBenchmarks<T, BranchlessUnroll4>(type_name, categories("shar_unroll4"));
    RegisterBinarySearchBenchmarks<T, BranchlessUnrollAll>(type_name, categories("shar_unroll_all"));
    RegisterBinarySearchBenchmarks<T, Branchless2>(type_name, categories("branchless2"));
    RegisterBinarySearchBenchmarks<T, Branchless3>(type_name, categories("branchless3"));
    RegisterBinarySearchBenchmarks<T, Branchless4>(type_name, categories("branchless4"));
    RegisterBinarySearchBenchmarks<T, Custom>(type_name, categories("custom"));
    RegisterBinarySearchBenchmarks<T, PlusLinear>(type_name, categories("plus_linear"));
    RegisterBinarySearchBenchmarks<T, Algorithmica>(type_name, categories("algorithmica"));
    RegisterBinarySearchBenchmarks<T, AlgorithmicaPrefetch>(type_name, categories("algorithmica_prefetch"));
    skb::BenchmarkCategories erthink_mini = categories("erthink_mini");
    RegisterBinarySearchBenchmarks<T, ErthinkMini0>(type_name, erthink_mini.AddCategoryCopy("unroll", "0"));
    RegisterBinarySearchBenchmarks<T, ErthinkMini1>(type_name, erthink_mini.AddCategoryCopy("unroll", "1"));
    RegisterBinarySearchBenchmarks<T, ErthinkMini2>(type_name, erthink_mini.AddCategoryCopy("unroll", "2"));
    RegisterBinarySearchBenchmarks<T, ErthinkMini3>(type_name, erthink_mini.AddCategoryCopy("unroll", "3"));
    RegisterBinarySearchBenchmarks<T, ErthinkGoto>(type_name, categories("erthink_goto"));
    RegisterBinarySearchBenchmarks<T, ErthinkSwitch>(type_name, categories("erthink_switch"));
    RegisterBinarySearchBenchmarks<T, ErthinkLowerBound>(type_name, categories("erthink_lower_bound"));
    RegisterBinarySearchBenchmarks<T, ErthinkLowerBoundTight>(type_name, categories("erthink_lower_bound_tight"));
    RegisterBinarySearchBenchmarks<T, CmpFirst>(type_name, categories("cmp_first"));
    RegisterBinarySearchBenchmarks<T, SkipLast>(type_name, categories("skip_last"));
}

void RegisterBinarySearchBenchmarks()
{
    SKA_BENCHMARK_REGISTER("baseline", binary_search_random_baseline_int);
    SKA_BENCHMARK_REGISTER("baseline", binary_search_random_baseline_string);
    RegisterForType<int>("int");
    RegisterForType<std::string>("string");
}

#ifndef DISABLE_GTEST
#include <test/include_test.hpp>

template<typename BinarySearch, typename Cmp>
void test_all(Cmp && cmp, int limit = 64)
{
    std::vector<int> sorted;
    auto found = BinarySearch()(sorted.begin(), sorted.end(), 0, cmp);
    ASSERT_EQ(sorted.end(), found);
    for (int i = 0; i < limit; ++i)
    {
        sorted.push_back(i);
        found = BinarySearch()(sorted.begin(), sorted.end(), -1, cmp);
        ASSERT_EQ(0, *found) << "i: " << i;
        for (int j = 0; j <= i; ++j)
        {
            found = BinarySearch()(sorted.begin(), sorted.end(), j, cmp);
            ASSERT_EQ(j, *found) << "i: " << i << ", j: " << j;
        }
        found = BinarySearch()(sorted.begin(), sorted.end(), i + 1, cmp);
        ASSERT_EQ(sorted.end(), found) << "i: " << i;
    }
}
template<typename BinarySearch>
void test_all(int limit = 64)
{
    test_all<BinarySearch>(std::less<>{}, limit);
}

struct CountingCompare
{
    template<typename T, typename U>
    bool operator()(T && l, U && r)
    {
        ++count;
        return std::less<>{}(l, r);
    }

    int & count;
};

template<typename BinarySearch>
void test_counts(int limit = 64)
{
    int count = 0;
    CountingCompare cmp{count};
    std::vector<int> sorted;
    auto found = BinarySearch()(sorted.begin(), sorted.end(), 0, cmp);
    ASSERT_EQ(sorted.end(), found);
    ASSERT_EQ(0, count);
    for (int i = 0; i < limit; ++i)
    {
        sorted.push_back(i);
        int compare_limit = 65 - std::countl_zero(bit_ceil(i));
        count = 0;
        found = BinarySearch()(sorted.begin(), sorted.end(), -1, cmp);
        ASSERT_EQ(0, *found) << "i: " << i;
        int actual_count = count;
        count = 0;
        Standard()(sorted.begin(), sorted.end(), -1, cmp);
        ASSERT_LE(actual_count, compare_limit) << "count: " << count << ", actual_count: " << actual_count << ", i: " << i;
        for (int j = 0; j <= i; ++j)
        {
            count = 0;
            found = BinarySearch()(sorted.begin(), sorted.end(), j, cmp);
            ASSERT_EQ(j, *found) << "i: " << i << ", j: " << j;
            actual_count = count;
            count = 0;
            Standard()(sorted.begin(), sorted.end(), j, cmp);
            ASSERT_LE(actual_count, compare_limit) << "count: " << count << ", actual_count: " << actual_count << ", i: " << i << ", j: " << j;
        }
        count = 0;
        found = BinarySearch()(sorted.begin(), sorted.end(), i + 1, cmp);
        ASSERT_EQ(sorted.end(), found) << "i: " << i;
        actual_count = count;
        count = 0;
        Standard()(sorted.begin(), sorted.end(), i + 1, cmp);
        ASSERT_LE(actual_count, compare_limit) << "count: " << count << ", actual_count: " << actual_count << ", i: " << i;
    }
}

TEST(binary_search, all)
{
    test_all<Branchless2>();
}
TEST(binary_search, all3)
{
    test_all<Branchless3>();
}
TEST(binary_search, all4)
{
    test_all<Branchless4>();
}

TEST(binary_search, all_shar)
{
    int count = 0;
    CountingCompare cmp{count};
    test_all<Branchless>(cmp);
    ASSERT_EQ(14616, count);
}

TEST(binary_search, all_shar_odd)
{
    int count = 0;
    CountingCompare cmp{count};
    test_all<BranchlessOdd>(cmp);
    ASSERT_EQ(14286, count);
}
TEST(binary_search, all_cmp_first)
{
    int count = 0;
    CountingCompare cmp{count};
    test_all<CmpFirst>(cmp);
    ASSERT_EQ(13974, count);
}
TEST(binary_search, all_skip_last)
{
    int count = 0;
    CountingCompare cmp{count};
    test_all<SkipLast>(cmp);
    ASSERT_EQ(12600, count);
}

TEST(binary_search, counts)
{
    test_counts<BranchlessOdd>();
}

TEST(binary_search, half_shar_odd)
{
    int count = 0;
    CountingCompare cmp{count};
    test_all<BranchlessOdd>(cmp, 1);
    EXPECT_EQ(3, count);
    count = 0;
    test_all<BranchlessOdd>(cmp, 2);
    EXPECT_EQ(11, count);
    count = 0;
    test_all<BranchlessOdd>(cmp, 3);
    EXPECT_EQ(24, count);
    count = 0;
    test_all<BranchlessOdd>(cmp, 4);
    EXPECT_EQ(42, count);
}

TEST(binary_search, all_lower_bound)
{
    int count = 0;
    CountingCompare cmp{count};
    test_all<Standard>(cmp);
    ASSERT_EQ(11835, count);
}

TEST(binary_search, half_lower_bound)
{
    int count = 0;
    CountingCompare cmp{count};
    test_all<Standard>(cmp, 1);
    EXPECT_EQ(3, count);
    count = 0;
    test_all<Standard>(cmp, 2);
    EXPECT_EQ(10, count);
    count = 0;
    test_all<Standard>(cmp, 3);
    EXPECT_EQ(20, count);
    count = 0;
    test_all<Standard>(cmp, 4);
    EXPECT_EQ(35, count);
}

TEST(binary_search, DISABLED_all_non_pow)
{
    int count = 0;
    CountingCompare cmp{count};
    test_all<BrokenBranchlessNonPow>(cmp);
    ASSERT_EQ(11835, count);
}

TEST(binary_search, all_shar_idx)
{
    test_all<BranchlessIdx>();
}

TEST(binary_search, all_shar_unroll2)
{
    test_all<BranchlessUnroll2>();
}

TEST(binary_search, all_shar_unroll4)
{
    test_all<BranchlessUnroll4>();
}

TEST(binary_search, all_shar_unroll_all)
{
    test_all<BranchlessUnrollAll>();
}

TEST(binary_search, plus_linear)
{
    test_all<PlusLinear>();
}

TEST(binary_search, custom)
{
    test_all<Custom>();
}
TEST(binary_search, all_algorithmica)
{
    test_all<Algorithmica>();
}
TEST(binary_search, all_erthink_mini0)
{
    int count = 0;
    CountingCompare cmp{count};
    test_all<ErthinkMini0>(cmp);
    ASSERT_EQ(14616, count);
}
TEST(binary_search, all_erthink_mini1)
{
    test_all<ErthinkMini1>();
}
TEST(binary_search, all_erthink_goto)
{
    test_all<ErthinkGoto>();
}
TEST(binary_search, all_erthink_switch)
{
    test_all<ErthinkSwitch>();
}
TEST(binary_search, all_erthink_lower_bound)
{
    test_all<ErthinkLowerBound>();
}
TEST(binary_search, all_erthink_lower_bound_tight)
{
    test_all<ErthinkLowerBoundTight>();
}

#endif
