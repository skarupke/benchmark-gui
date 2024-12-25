#include "container/knuth_multiset.hpp"

#include "custom_benchmark/custom_benchmark.h"
#include "hashtable_benchmarks/benchmark_shared.hpp"

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "container/flat_hash_map.hpp"
#include "debug/assert.hpp"

template<typename T>
struct knuth_multiset_based
{
    void reserve(size_t)
    {
    }
    void clear()
    {
        elements.clear();
    }
    int insert(const T & x)
    {
        return *elements.insert(x);
    }

private:
    std::multiset<T> elements;
};
template<typename T>
struct knuth_unordered_multiset_based
{
    void reserve(size_t)
    {
    }
    void clear()
    {
        elements.clear();
    }
    int insert(const T & x)
    {
        return *elements.insert(x);
    }

private:
    std::unordered_multiset<T> elements;
};
template<typename T>
struct knuth_map_based
{
    void reserve(size_t)
    {
    }
    void clear()
    {
        counts.clear();
    }
    int insert(const T & x)
    {
        return ++counts[x];
    }

private:
    std::map<T, int> counts;
};
template<typename T>
struct knuth_unordered_map_based
{
    void reserve(size_t size)
    {
        counts.reserve(size);
    }
    void clear()
    {
        counts.clear();
    }

    int insert(const T & x)
    {
        return ++counts[x];
    }

private:
    std::unordered_map<T, int> counts;
};
template<typename T>
struct knuth_flat_hash_map_based
{
    void reserve(size_t size)
    {
        counts.reserve(size);
    }
    void clear()
    {
        counts.clear();
    }

    int insert(const T & x)
    {
        return ++counts[x];
    }

private:
    ska::flat_hash_map<T, int> counts;
};


template<typename T>
struct faster_distribution
{
    explicit faster_distribution(T limit)
        : limit(static_cast<uint64_t>(limit) + 1)
    {
        CHECK_FOR_PROGRAMMER_ERROR(limit > 0 && static_cast<uint64_t>(limit) < std::numeric_limits<uint64_t>::max());
    }

    uint64_t limit;
    // from https://lemire.me/blog/2019/06/06/nearly-divisionless-random-integer-generation-on-various-systems/
    template<typename Randomness>
    T operator()(Randomness & random) const
    {
          uint64_t x = random() ;
          __uint128_t m = ( __uint128_t ) x * ( __uint128_t ) limit;
          uint64_t l = ( uint64_t ) m;
          if (l < limit) {
            uint64_t t = -limit % limit;
            while (l < t) {
              x = random() ;
              m = ( __uint128_t ) x * ( __uint128_t ) limit;
              l = ( uint64_t ) m;
            }
          }
          return static_cast<T>(m >> 64);
    }
};

void benchmark_knuth_baseline(skb::State & state)
{
    int num_items = state.range(0);
    faster_distribution<int> distribution(num_items / 3);
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        for (int i = 0; i < num_items; ++i)
        {
            skb::DoNotOptimize(no_inline_random_number(distribution, randomness));
        }
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_knuth_baseline);

template<typename Set, int (Set::*Func)(const int &)>
void benchmark_insert_knuth(skb::State & state)
{
    Set set;
    int num_items = state.range(0);
    set.reserve(num_items);
    faster_distribution<int> distribution(num_items / 3);
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        set.clear();
        for (int i = 0; i < num_items; ++i)
        {
            skb::DoNotOptimize((set.*Func)(no_inline_random_number(distribution, randomness)));
        }
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

skb::Benchmark * SetRange(skb::Benchmark * bm)
{
    return bm->SetRange(4, 16*1024)->SetRangeMultiplier(2.0);
}

void RegisterKnuthBenchmarks()
{
    skb::CategoryBuilder builder;
    {
        skb::CategoryBuilder with_pushback = builder.AddCategory("pushback", "yes");
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set<int>, &knuth_multi_set<int>::insert_1>, with_pushback.BuildCategories("knuth_insert", "insert_1"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set<int>, &knuth_multi_set<int>::insert_1a>, with_pushback.BuildCategories("knuth_insert", "insert_1a"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set<int>, &knuth_multi_set<int>::insert_0>, with_pushback.BuildCategories("knuth_insert", "insert_0"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set<int>, &knuth_multi_set<int>::insert_1_no_goto>, with_pushback.BuildCategories("knuth_insert", "insert_1_no_goto"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set<int>, &knuth_multi_set<int>::insert_2>, with_pushback.BuildCategories("knuth_insert", "insert_2"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set<int>, &knuth_multi_set<int>::insert_2a>, with_pushback.BuildCategories("knuth_insert", "insert_2a"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set<int>, &knuth_multi_set<int>::insert_2a_loop>, with_pushback.BuildCategories("knuth_insert", "insert_2a_loop"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set<int>, &knuth_multi_set<int>::insert_stl>, with_pushback.BuildCategories("knuth_insert", "insert_stl"))->SetBaseline("benchmark_knuth_baseline"));
        skb::CategoryBuilder without_pushback = builder.AddCategory("pushback", "no");
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set_no_pushback<int>, &knuth_multi_set_no_pushback<int>::insert_1>, without_pushback.BuildCategories("knuth_insert", "insert_1"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set_no_pushback<int>, &knuth_multi_set_no_pushback<int>::insert_1a>, without_pushback.BuildCategories("knuth_insert", "insert_1a"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set_no_pushback<int>, &knuth_multi_set_no_pushback<int>::insert_0>, without_pushback.BuildCategories("knuth_insert", "insert_0"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set_no_pushback<int>, &knuth_multi_set_no_pushback<int>::insert_0_asm>, without_pushback.BuildCategories("knuth_insert", "insert_0_asm"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set_no_pushback<int>, &knuth_multi_set_no_pushback<int>::insert_1_no_goto>, without_pushback.BuildCategories("knuth_insert", "insert_1_no_goto"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set_no_pushback<int>, &knuth_multi_set_no_pushback<int>::insert_2>, without_pushback.BuildCategories("knuth_insert", "insert_2"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set_no_pushback<int>, &knuth_multi_set_no_pushback<int>::insert_2a>, without_pushback.BuildCategories("knuth_insert", "insert_2a"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set_no_pushback<int>, &knuth_multi_set_no_pushback<int>::insert_2a_loop>, without_pushback.BuildCategories("knuth_insert", "insert_2a_loop"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multi_set_no_pushback<int>, &knuth_multi_set_no_pushback<int>::insert_stl>, without_pushback.BuildCategories("knuth_insert", "insert_stl"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_multiset_based<int>, &knuth_multiset_based<int>::insert>, builder.BuildCategories("knuth_insert", "multiset_based"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_unordered_multiset_based<int>, &knuth_unordered_multiset_based<int>::insert>, builder.BuildCategories("knuth_insert", "unordered_multiset_based"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_map_based<int>, &knuth_map_based<int>::insert>, builder.BuildCategories("knuth_insert", "map_based"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_unordered_map_based<int>, &knuth_unordered_map_based<int>::insert>, builder.BuildCategories("knuth_insert", "unordered_map_based"))->SetBaseline("benchmark_knuth_baseline"));
        SetRange(SKA_BENCHMARK_CATEGORIES(&benchmark_insert_knuth<knuth_flat_hash_map_based<int>, &knuth_flat_hash_map_based<int>::insert>, builder.BuildCategories("knuth_insert", "flat_hash_map_based"))->SetBaseline("benchmark_knuth_baseline"));
    }
}


#ifndef DISABLE_GTEST
#include <test/include_test.hpp>

TEST(knuth_multiset, example_1)
{
    knuth_multi_set<int> set;
    ASSERT_EQ(1, set.insert_1(5));
    ASSERT_EQ(1, set.insert_1(10));
    ASSERT_EQ(2, set.insert_1(5));
    ASSERT_EQ(2, set.insert_1(10));
    ASSERT_EQ(1, set.insert_1(3));
    ASSERT_EQ(3, set.insert_1(5));
}
TEST(knuth_multiset, example_1_no_goto)
{
    knuth_multi_set<int> set;
    ASSERT_EQ(1, set.insert_1_no_goto(5));
    ASSERT_EQ(1, set.insert_1_no_goto(10));
    ASSERT_EQ(2, set.insert_1_no_goto(5));
    ASSERT_EQ(2, set.insert_1_no_goto(10));
    ASSERT_EQ(1, set.insert_1_no_goto(3));
    ASSERT_EQ(3, set.insert_1_no_goto(5));
}
TEST(knuth_multiset, example_1a)
{
    knuth_multi_set<int> set;
    ASSERT_EQ(1, set.insert_1a(5));
    ASSERT_EQ(1, set.insert_1a(10));
    ASSERT_EQ(2, set.insert_1a(5));
    ASSERT_EQ(2, set.insert_1a(10));
    ASSERT_EQ(1, set.insert_1a(3));
    ASSERT_EQ(3, set.insert_1a(5));
}
TEST(knuth_multiset, example_0)
{
    knuth_multi_set<int> set;
    ASSERT_EQ(1, set.insert_0(5));
    ASSERT_EQ(1, set.insert_0(10));
    ASSERT_EQ(2, set.insert_0(5));
    ASSERT_EQ(2, set.insert_0(10));
    ASSERT_EQ(1, set.insert_0(3));
    ASSERT_EQ(3, set.insert_0(5));
}
TEST(knuth_multiset, example_2)
{
    knuth_multi_set<int> set;
    ASSERT_EQ(1, set.insert_2(5));
    ASSERT_EQ(1, set.insert_2(10));
    ASSERT_EQ(2, set.insert_2(5));
    ASSERT_EQ(2, set.insert_2(10));
    ASSERT_EQ(1, set.insert_2(3));
    ASSERT_EQ(3, set.insert_2(5));
}
TEST(knuth_multiset, example_2a)
{
    knuth_multi_set<int> set;
    ASSERT_EQ(1, set.insert_2a(5));
    ASSERT_EQ(1, set.insert_2a(10));
    ASSERT_EQ(2, set.insert_2a(5));
    ASSERT_EQ(2, set.insert_2a(10));
    ASSERT_EQ(1, set.insert_2a(3));
    ASSERT_EQ(3, set.insert_2a(5));
}
TEST(knuth_multiset, example_2a_loop)
{
    knuth_multi_set<int> set;
    ASSERT_EQ(1, set.insert_2a_loop(5));
    ASSERT_EQ(1, set.insert_2a_loop(10));
    ASSERT_EQ(2, set.insert_2a_loop(5));
    ASSERT_EQ(2, set.insert_2a_loop(10));
    ASSERT_EQ(1, set.insert_2a_loop(3));
    ASSERT_EQ(3, set.insert_2a_loop(5));
}
TEST(knuth_multiset, stl_based)
{
    knuth_multi_set<int> set;
    ASSERT_EQ(1, set.insert_stl(5));
    ASSERT_EQ(1, set.insert_stl(10));
    ASSERT_EQ(2, set.insert_stl(5));
    ASSERT_EQ(2, set.insert_stl(10));
    ASSERT_EQ(1, set.insert_stl(3));
    ASSERT_EQ(3, set.insert_stl(5));
}

TEST(knuth_multiset, map_based)
{
    knuth_map_based<int> set;
    ASSERT_EQ(1, set.insert(5));
    ASSERT_EQ(1, set.insert(10));
    ASSERT_EQ(2, set.insert(5));
    ASSERT_EQ(2, set.insert(10));
    ASSERT_EQ(1, set.insert(3));
    ASSERT_EQ(3, set.insert(5));
}
TEST(knuth_multiset, unordered_map_based)
{
    knuth_unordered_map_based<int> set;
    ASSERT_EQ(1, set.insert(5));
    ASSERT_EQ(1, set.insert(10));
    ASSERT_EQ(2, set.insert(5));
    ASSERT_EQ(2, set.insert(10));
    ASSERT_EQ(1, set.insert(3));
    ASSERT_EQ(3, set.insert(5));
}
TEST(knuth_multiset, flat_hash_map_based)
{
    knuth_flat_hash_map_based<int> set;
    ASSERT_EQ(1, set.insert(5));
    ASSERT_EQ(1, set.insert(10));
    ASSERT_EQ(2, set.insert(5));
    ASSERT_EQ(2, set.insert(10));
    ASSERT_EQ(1, set.insert(3));
    ASSERT_EQ(3, set.insert(5));
}

TEST(knuth_multiset_no_pushback, example_1)
{
    knuth_multi_set_no_pushback<int> set;
    set.reserve(3);
    ASSERT_EQ(1, set.insert_1(5));
    ASSERT_EQ(1, set.insert_1(10));
    ASSERT_EQ(2, set.insert_1(5));
    ASSERT_EQ(2, set.insert_1(10));
    ASSERT_EQ(1, set.insert_1(3));
    ASSERT_EQ(3, set.insert_1(5));
}
TEST(knuth_multiset_no_pushback, example_1_no_goto)
{
    knuth_multi_set_no_pushback<int> set;
    set.reserve(3);
    ASSERT_EQ(1, set.insert_1_no_goto(5));
    ASSERT_EQ(1, set.insert_1_no_goto(10));
    ASSERT_EQ(2, set.insert_1_no_goto(5));
    ASSERT_EQ(2, set.insert_1_no_goto(10));
    ASSERT_EQ(1, set.insert_1_no_goto(3));
    ASSERT_EQ(3, set.insert_1_no_goto(5));
}
TEST(knuth_multiset_no_pushback, example_1a)
{
    knuth_multi_set_no_pushback<int> set;
    set.reserve(3);
    ASSERT_EQ(1, set.insert_1a(5));
    ASSERT_EQ(1, set.insert_1a(10));
    ASSERT_EQ(2, set.insert_1a(5));
    ASSERT_EQ(2, set.insert_1a(10));
    ASSERT_EQ(1, set.insert_1a(3));
    ASSERT_EQ(3, set.insert_1a(5));
}
TEST(knuth_multiset_no_pushback, example_0)
{
    knuth_multi_set_no_pushback<int> set;
    set.reserve(3);
    ASSERT_EQ(1, set.insert_0(5));
    ASSERT_EQ(1, set.insert_0(10));
    ASSERT_EQ(2, set.insert_0(5));
    ASSERT_EQ(2, set.insert_0(10));
    ASSERT_EQ(1, set.insert_0(3));
    ASSERT_EQ(3, set.insert_0(5));
}
TEST(knuth_multiset_no_pushback, example_0_asm)
{
    knuth_multi_set_no_pushback<int> set;
    set.reserve(3);
    ASSERT_EQ(1, set.insert_0_asm(5));
    ASSERT_EQ(1, set.insert_0_asm(10));
    ASSERT_EQ(2, set.insert_0_asm(5));
    ASSERT_EQ(2, set.insert_0_asm(10));
    ASSERT_EQ(1, set.insert_0_asm(3));
    ASSERT_EQ(3, set.insert_0_asm(5));
}
TEST(knuth_multiset_no_pushback, example_2)
{
    knuth_multi_set_no_pushback<int> set;
    set.reserve(3);
    ASSERT_EQ(1, set.insert_2(5));
    ASSERT_EQ(1, set.insert_2(10));
    ASSERT_EQ(2, set.insert_2(5));
    ASSERT_EQ(2, set.insert_2(10));
    ASSERT_EQ(1, set.insert_2(3));
    ASSERT_EQ(3, set.insert_2(5));
}
TEST(knuth_multiset_no_pushback, example_2a)
{
    knuth_multi_set_no_pushback<int> set;
    set.reserve(3);
    ASSERT_EQ(1, set.insert_2a(5));
    ASSERT_EQ(1, set.insert_2a(10));
    ASSERT_EQ(2, set.insert_2a(5));
    ASSERT_EQ(2, set.insert_2a(10));
    ASSERT_EQ(1, set.insert_2a(3));
    ASSERT_EQ(3, set.insert_2a(5));
}
TEST(knuth_multiset_no_pushback, example_2a_loop)
{
    knuth_multi_set_no_pushback<int> set;
    set.reserve(3);
    ASSERT_EQ(1, set.insert_2a_loop(5));
    ASSERT_EQ(1, set.insert_2a_loop(10));
    ASSERT_EQ(2, set.insert_2a_loop(5));
    ASSERT_EQ(2, set.insert_2a_loop(10));
    ASSERT_EQ(1, set.insert_2a_loop(3));
    ASSERT_EQ(3, set.insert_2a_loop(5));
}
TEST(knuth_multiset_no_pushback, example_stl)
{
    knuth_multi_set_no_pushback<int> set;
    set.reserve(3);
    ASSERT_EQ(1, set.insert_stl(5));
    ASSERT_EQ(1, set.insert_stl(10));
    ASSERT_EQ(2, set.insert_stl(5));
    ASSERT_EQ(2, set.insert_stl(10));
    ASSERT_EQ(1, set.insert_stl(3));
    ASSERT_EQ(3, set.insert_stl(5));
}


#endif
