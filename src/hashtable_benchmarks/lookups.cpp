#include "hashtable_benchmarks/lookups.hpp"
#include "custom_benchmark/custom_benchmark.h"
#include "hashtable_benchmarks/benchmark_shared.hpp"

#include "container/perfect_hash_map.hpp"
#include <cstdio>


TrackAllocations::TrackAllocations(skb::State & state)
    : state(state)
{
    num_bytes_allocated_before = counting_allocator_stats::num_bytes_allocated;
    num_bytes_freed_before = counting_allocator_stats::num_freed;
}
TrackAllocations::~TrackAllocations()
{
    size_t num_allocated_bytes_after = counting_allocator_stats::num_bytes_allocated;
    size_t num_freed_bytes_after = counting_allocator_stats::num_freed;
    state.SetNumBytes(num_allocated_bytes_after - num_bytes_allocated_before, num_freed_bytes_after - num_bytes_freed_before);
}

CategoryBuilder add_max_load_factor_category(CategoryBuilder builder, float max_load_factor)
{
    if (max_load_factor > 0.0f)
    {
        char buffer_for_printf[128];
        snprintf(buffer_for_printf, sizeof(buffer_for_printf), "%g", max_load_factor);
        return builder.AddCategory("max_load_factor", buffer_for_printf);
    }
    else
        return builder.AddCategory("max_load_factor", "default");
}

#define BASELINE_CONCAT(a, b) a ## b
#define IMPL_BASELINE(name) void BASELINE_CONCAT(name,_int)(skb::State & state)\
{\
    return name<int>(state);\
}\
void BASELINE_CONCAT(name,_string)(skb::State & state)\
{\
    return name<std::string>(state);\
}\

template<typename T>
void benchmark_successful_lookup_baseline(skb::State & state)
{
    const profiling_randomness random_state = global_randomness;
    int num_items = state.range(0);
    while (state.KeepRunning())
    {
        BenchmarkRandomDistribution<T> distribution;
        global_randomness = random_state;
        for (int i = 0; i < num_items; ++i)
            skb::DoNotOptimize(no_inline_random_number(distribution, global_randomness));
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
IMPL_BASELINE(benchmark_successful_lookup_baseline)

template<typename T>
void benchmark_successful_lookup_permutation_baseline(skb::State & state)
{
    int num_items = state.range(0);
    using distribution_type = BenchmarkRandomDistribution<T>;
    std::vector<std::pair<T, T>> values = create_random_values<std::pair<T, T>, distribution_type>(global_randomness, num_items);

    while (state.KeepRunning())
    {
        T to_find = values.back().first;
        for (int i = 0; i < num_items; ++i)
        {
            to_find = values[i].first;
            skb::DoNotOptimize(to_find);
        }
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
IMPL_BASELINE(benchmark_successful_lookup_permutation_baseline)

template<typename T>
void benchmark_successful_lookup_baseline_cache_miss(skb::State & state)
{
    const profiling_randomness random_state = global_randomness;
    int num_items = state.range(0);

    std::uniform_int_distribution<size_t> map_distribution(0, 1000000000 / num_items);
    while (state.KeepRunning())
    {
        BenchmarkRandomDistribution<T> distribution;
        std::mt19937_64 key_randomness = random_state;
        for (int i = 0; i < num_items; ++i)
        {
            skb::DoNotOptimize(no_inline_random_number(map_distribution, global_randomness));
            skb::DoNotOptimize(no_inline_random_number(distribution, key_randomness));
        }
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
IMPL_BASELINE(benchmark_successful_lookup_baseline_cache_miss)

template<typename T>
void benchmark_successful_lookup_predictable_baseline(skb::State & state)
{
    std::vector<T> random_keys(state.range(0));
    BenchmarkRandomDistribution<T> distribution;
    for (T & i : random_keys)
    {
        i = distribution(global_randomness);
    }

    std::shuffle(random_keys.begin(), random_keys.end(), global_randomness);
    while (state.KeepRunning())
    {
        for (const T & i : random_keys)
            skb::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations() * random_keys.size());
}
IMPL_BASELINE(benchmark_successful_lookup_predictable_baseline)

template<typename T>
void benchmark_successful_lookup_predictable_best_worst_baseline_1(skb::State & state)
{
    std::vector<T> random_keys(std::max(1, state.range(0) / 100));
    BenchmarkRandomDistribution<T> distribution;
    for (T & i : random_keys)
    {
        i = distribution(global_randomness);
    }

    std::shuffle(random_keys.begin(), random_keys.end(), global_randomness);

    while (state.KeepRunning())
    {
        for (const T & i : random_keys)
            skb::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations() * random_keys.size());
}
IMPL_BASELINE(benchmark_successful_lookup_predictable_best_worst_baseline_1)

template<typename T>
void benchmark_successful_lookup_predictable_best_worst_baseline_10(skb::State & state)
{
    std::vector<T> random_keys(std::max(1, state.range(0) / 10));
    BenchmarkRandomDistribution<T> distribution;
    for (T & i : random_keys)
    {
        i = distribution(global_randomness);
    }

    std::shuffle(random_keys.begin(), random_keys.end(), global_randomness);

    while (state.KeepRunning())
    {
        for (const T & i : random_keys)
            skb::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations() * random_keys.size());
}
IMPL_BASELINE(benchmark_successful_lookup_predictable_best_worst_baseline_10)

template<typename T>
void benchmark_successful_lookup_predictable_best_worst_baseline_25(skb::State & state)
{
    std::vector<T> random_keys(std::max(1, state.range(0) / 4));
    BenchmarkRandomDistribution<T> distribution;
    for (T & i : random_keys)
    {
        i = distribution(global_randomness);
    }

    std::shuffle(random_keys.begin(), random_keys.end(), global_randomness);

    while (state.KeepRunning())
    {
        for (const T & i : random_keys)
            skb::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations() * random_keys.size());
}
IMPL_BASELINE(benchmark_successful_lookup_predictable_best_worst_baseline_25)

template<typename T>
void benchmark_successful_lookup_predictable_baseline_cache_miss(skb::State & state)
{
    std::vector<T> random_keys(state.range(0));
    BenchmarkRandomDistribution<T> distribution;
    for (T & i : random_keys)
    {
        i = distribution(global_randomness);
    }

    std::shuffle(random_keys.begin(), random_keys.end(), global_randomness);
    size_t map_counter = 0;
    size_t reset_counter_at = random_keys.size();
    while (state.KeepRunning())
    {
        for (const T & i : random_keys)
        {
            skb::DoNotOptimize(map_counter++);
            skb::DoNotOptimize(i);
            if (map_counter == reset_counter_at)
                map_counter = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * random_keys.size());
}
IMPL_BASELINE(benchmark_successful_lookup_predictable_baseline_cache_miss)

template<typename T>
void benchmark_unsuccessful_lookup_baseline(skb::State & state)
{
    BenchmarkRandomDistribution<T> random_index;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_lookup_loops; ++i)
            skb::DoNotOptimize(no_inline_random_number(random_index, global_randomness));
    }
    state.SetItemsProcessed(state.iterations() * num_lookup_loops);
}
IMPL_BASELINE(benchmark_unsuccessful_lookup_baseline)

template<typename T>
void benchmark_unsuccessful_lookup_baseline_cache_miss(skb::State & state)
{
    BenchmarkRandomDistribution<T> random_index;
    std::uniform_int_distribution<size_t> random_map(0, 1000000000 / state.range(0));
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_lookup_loops; ++i)
        {
            skb::DoNotOptimize(no_inline_random_number(random_map, global_randomness));
            skb::DoNotOptimize(no_inline_random_number(random_index, global_randomness));
        }
    }
    state.SetItemsProcessed(state.iterations() * num_lookup_loops);
}
IMPL_BASELINE(benchmark_unsuccessful_lookup_baseline_cache_miss)

template<typename T>
void benchmark_unsuccessful_lookup_predictable_baseline(skb::State & state)
{
    BenchmarkRandomDistribution<T> random_index;
    std::vector<T> random_keys(state.range(0));
    for (T & random : random_keys)
        random = random_index(global_randomness);

    while (state.KeepRunning())
    {
        for (const T & i : random_keys)
            skb::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations() * random_keys.size());
}
IMPL_BASELINE(benchmark_unsuccessful_lookup_predictable_baseline)

template<typename T>
void benchmark_unsuccessful_lookup_predictable_baseline_cache_miss(skb::State & state)
{
    BenchmarkRandomDistribution<T> random_index;
    std::vector<T> random_keys(state.range(0));
    for (T & random : random_keys)
        random = random_index(global_randomness);

    size_t map_index = 0;
    size_t reset_map_index_at = random_keys.size();
    while (state.KeepRunning())
    {
        for (const T & i : random_keys)
        {
            skb::DoNotOptimize(map_index++);
            skb::DoNotOptimize(i);
            if (map_index == reset_map_index_at)
                map_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * random_keys.size());
}
IMPL_BASELINE(benchmark_unsuccessful_lookup_predictable_baseline_cache_miss)

SKA_BENCHMARK("baseline", benchmark_successful_lookup_baseline_int);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_baseline_cache_miss_int);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_permutation_baseline_int);
SKA_BENCHMARK("baseline", benchmark_unsuccessful_lookup_baseline_int);
SKA_BENCHMARK("baseline", benchmark_unsuccessful_lookup_baseline_cache_miss_int);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_predictable_baseline_int);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_predictable_best_worst_baseline_1_int);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_predictable_best_worst_baseline_10_int);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_predictable_best_worst_baseline_25_int);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_predictable_baseline_cache_miss_int);
SKA_BENCHMARK("baseline", benchmark_unsuccessful_lookup_predictable_baseline_int);
SKA_BENCHMARK("baseline", benchmark_unsuccessful_lookup_predictable_baseline_cache_miss_int);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_baseline_string);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_permutation_baseline_string);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_baseline_cache_miss_string);
SKA_BENCHMARK("baseline", benchmark_unsuccessful_lookup_baseline_string);
SKA_BENCHMARK("baseline", benchmark_unsuccessful_lookup_baseline_cache_miss_string);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_predictable_baseline_string);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_predictable_best_worst_baseline_1_string);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_predictable_best_worst_baseline_10_string);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_predictable_best_worst_baseline_25_string);
SKA_BENCHMARK("baseline", benchmark_successful_lookup_predictable_baseline_cache_miss_string);
SKA_BENCHMARK("baseline", benchmark_unsuccessful_lookup_predictable_baseline_string);
SKA_BENCHMARK("baseline", benchmark_unsuccessful_lookup_predictable_baseline_cache_miss_string);

void benchmark_successful_lookup_perfect(skb::State & state)
{
    const profiling_randomness random_state = global_randomness;
    int num_items = state.range(0);
    std::vector<std::pair<int, int>> random_keys;
    random_keys.reserve(num_items);
    std::uniform_int_distribution<int> setup_distribution;
    while (random_keys.size() != random_keys.capacity())
    {
        random_keys.emplace_back(setup_distribution(global_randomness), static_cast<int>(random_keys.size()));
    }
    ska::perfect_hash_table<int, int> map(random_keys.begin(), random_keys.end());

    while (state.KeepRunning())
    {
        std::uniform_int_distribution<int> distribution;
        global_randomness = random_state;
        for (int i = 0; i < num_items; ++i)
            skb::DoNotOptimize(map[distribution(global_randomness)]);
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}

void RegisterBlockHashMap();
void RegisterBytellHashMap();
void RegisterFlat16HashMap();
void RegisterFlatHashMap();
void RegisterGoogleBlockHashMap();
void RegisterGoogleFlat16HashMap();
void RegisterLinearSearch();
void RegisterLLVMUnorderedMap();
void RegisterMicrosoftUnorderedMap();
void RegisterPtrHashMap();
void RegisterTessilHashtables();
void RegisterThirdPartyHashtables();
void RegisterUnorderedMap();

void RegisterHashtableBenchmarks()
{
    RegisterBlockHashMap();
    RegisterBytellHashMap();
    RegisterFlat16HashMap();
    RegisterFlatHashMap();
    RegisterGoogleBlockHashMap();
    RegisterGoogleFlat16HashMap();
    RegisterLinearSearch();
    RegisterLLVMUnorderedMap();
    RegisterMicrosoftUnorderedMap();
    RegisterPtrHashMap();
    RegisterTessilHashtables();
    RegisterThirdPartyHashtables();
    RegisterUnorderedMap();
}

#ifndef DISABLE_GTEST
#include "test/include_test.hpp"
TEST(sequential_distribution, cases)
{
    ManySequentialDistribution a;
    profiling_randomness randomness;
    ASSERT_EQ(0x00000001u, a(randomness));
    ASSERT_EQ(0x00010001u, a(randomness));
    ASSERT_EQ(0x00010000u, a(randomness));
    ASSERT_EQ(0x0001ffffu, a(randomness));
    ASSERT_EQ(0x0000ffffu, a(randomness));
    ASSERT_EQ(0xffffffffu, a(randomness));
    ASSERT_EQ(0xffff0000u, a(randomness));
    ASSERT_EQ(0xffff0001u, a(randomness));
    ASSERT_EQ(0xffff0002u, a(randomness));
    ASSERT_EQ(0x00000002u, a(randomness));
    ASSERT_EQ(0x00010002u, a(randomness));
    ASSERT_EQ(0x00020002u, a(randomness));
    ASSERT_EQ(0x00020001u, a(randomness));
    ASSERT_EQ(0x00020000u, a(randomness));
    ASSERT_EQ(0x0002ffffu, a(randomness));
    ASSERT_EQ(0x0002fffeu, a(randomness));
    ASSERT_EQ(0x0001fffeu, a(randomness));
    ASSERT_EQ(0x0000fffeu, a(randomness));
    ASSERT_EQ(0xfffffffeu, a(randomness));
    ASSERT_EQ(0xfffefffeu, a(randomness));
    ASSERT_EQ(0xfffeffffu, a(randomness));
    ASSERT_EQ(0xfffe0000u, a(randomness));
    ASSERT_EQ(0xfffe0001u, a(randomness));
    ASSERT_EQ(0xfffe0002u, a(randomness));
    ASSERT_EQ(0xfffe0003u, a(randomness));
    ASSERT_EQ(0xffff0003u, a(randomness));
    ASSERT_EQ(0x00000003u, a(randomness));
    ASSERT_EQ(0x00010003u, a(randomness));
    ASSERT_EQ(0x00020003u, a(randomness));
    ASSERT_EQ(0x00030003u, a(randomness));
    ASSERT_EQ(0x00030002u, a(randomness));
    ASSERT_EQ(0x00030001u, a(randomness));
    ASSERT_EQ(0x00030000u, a(randomness));
    ASSERT_EQ(0x0003ffffu, a(randomness));
    ASSERT_EQ(0x0003fffeu, a(randomness));
    ASSERT_EQ(0x0003fffdu, a(randomness));
    ASSERT_EQ(0x0002fffdu, a(randomness));
}

#endif

