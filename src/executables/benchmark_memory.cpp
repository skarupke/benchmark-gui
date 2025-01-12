#include <random>
#include <numeric>
#include <vector>

#include "custom_benchmark/custom_benchmark.h"
#include "custom_benchmark/utils.hpp"
#include "test/include_test.hpp"


static constexpr const size_t num_loops = 10000;
static constexpr size_t memory_benchmark_multiplier = 16;

void benchmark_memory_access(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::vector<size_t> bytes(num_bytes);
    std::iota(bytes.begin(), bytes.end(), size_t(100));
    std::uniform_int_distribution<size_t> random_index(0, num_bytes - 1);
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
            skb::DoNotOptimize(bytes[random_index(global_randomness)]);
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}
void benchmark_memory_access_permutation(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::vector<size_t> bytes(num_bytes);
    {
        std::vector<size_t> offsets(num_bytes);
        std::iota(offsets.begin(), offsets.end(), size_t());
        std::shuffle(offsets.begin(), offsets.end(), global_randomness);
        for (size_t i = 0; i < num_bytes; ++i)
        {
            size_t other_index = i == 0 ? num_bytes - 1 : i - 1;
            size_t & found = bytes[offsets[i]];
            CHECK_FOR_PROGRAMMER_ERROR(found == 0);
            found = offsets[other_index];
        }
    }
    size_t index = bytes[0];
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
        {
            index = bytes[index];
            skb::DoNotOptimize(index);
        }
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}

#if 1
void benchmark_predictable_memory_access(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::vector<size_t> bytes(num_bytes);
    std::iota(bytes.begin(), bytes.end(), size_t(100));
    std::uniform_int_distribution<size_t> random_index(0, num_bytes - 1);
    constexpr size_t num_indices = 16 * 1024 * 1024;
    std::vector<size_t> indices(num_indices);
    for (size_t & index : indices)
        index = random_index(global_randomness);
    size_t current_index = 0;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
        {
            skb::DoNotOptimize(bytes[indices[current_index++]]);
            if (current_index == num_indices)
                current_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}
#else
std::vector<size_t> random_bytes(size_t num)
{
    std::vector<size_t> result(num / sizeof(size_t));
    std::iota(result.begin(), result.end(), size_t(100));
    return result;
}
// version for the slides
void benchmark_cache_miss(benchmark::State & state)
{
    std::vector<size_t> bytes = random_bytes(state.range(0));
    std::vector<size_t> indices = random_indices(bytes.size());
    size_t current_index = 0;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < 10000; ++i)
        {
            size_t random_index = indices[current_index++];
            benchmark::DoNotOptimize(bytes[random_index]);
            if (current_index == indices.size())
                current_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
void benchmark_cache_miss(benchmark::State & state)
{
    std::vector<size_t> bytes = random_bytes(state.range(0));


    while (state.KeepRunning())
    {
        for (size_t i = 0; i < 10000; ++i)
        {
            size_t random_index = indices[current_index++];
            benchmark::DoNotOptimize(bytes[random_index]);
            if (current_index == indices.size())
                current_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
void benchmark_cache_miss(benchmark::State & state)
{
    std::vector<size_t> bytes = random_bytes(state.range(0));
    std::mt19937_64 randomness;
    std::uniform_int_distribution<size_t> distribution(0, bytes.size() - 1);
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < 10000; ++i)
        {
            size_t random_index = indices[current_index++];
            benchmark::DoNotOptimize(bytes[random_index]);
            if (current_index == indices.size())
                current_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
void benchmark_cache_miss(benchmark::State & state)
{
    std::vector<size_t> bytes = random_bytes(state.range(0));
    std::mt19937_64 randomness;
    std::uniform_int_distribution<size_t> distribution(0, bytes.size() - 1);
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < 10000; ++i)
        {

            benchmark::DoNotOptimize(bytes[random_index]);


        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
void benchmark_cache_miss(benchmark::State & state)
{
    std::vector<size_t> bytes = random_bytes(state.range(0));
    std::mt19937_64 randomness;
    std::uniform_int_distribution<size_t> distribution(0, bytes.size() - 1);
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < 10000; ++i)
        {
            size_t random_index = distribution(randomness);
            benchmark::DoNotOptimize(bytes[random_index]);


        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
#endif
void benchmark_sequential_memory_access(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::vector<size_t> bytes(num_bytes);
    std::iota(bytes.begin(), bytes.end(), size_t(100));
    while (state.KeepRunning())
    {
        for (size_t i : bytes)
            skb::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations() * num_bytes);
}
void benchmark_sequential_memory_access_baseline(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_bytes; ++i)
            skb::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations() * num_bytes);
}

void benchmark_memory_access_baseline(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::uniform_int_distribution<size_t> random_index(0, num_bytes - 1);

    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
            skb::DoNotOptimize(random_index(global_randomness));
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}
void benchmark_predictable_memory_access_baseline(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::uniform_int_distribution<size_t> random_index(0, num_bytes - 1);
    constexpr size_t num_indices = 16 * 1024 * 1024;
    std::vector<size_t> indices(num_indices);
    for (size_t & index : indices)
        index = random_index(global_randomness);
    size_t current_index = 0;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
        {
            skb::DoNotOptimize(indices[current_index++]);
            if (current_index == num_indices)
                current_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}
void benchmark_memory_access_permutation_baseline(skb::State & state)
{
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
        {
            skb::DoNotOptimize(i);
        }
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}

static constexpr int memory_access_min = 2048 / memory_benchmark_multiplier;
static constexpr int memory_access_max = 4 * 1024 * 1024 * (1024 / memory_benchmark_multiplier);
SKA_BENCHMARK("baseline", benchmark_memory_access_baseline);
SKA_BENCHMARK("baseline", benchmark_predictable_memory_access_baseline);
SKA_BENCHMARK("baseline", benchmark_sequential_memory_access_baseline);
SKA_BENCHMARK("baseline", benchmark_memory_access_permutation_baseline);
SKA_BENCHMARK("memory access", benchmark_memory_access)->SetBaseline("benchmark_memory_access_baseline")->SetRange(memory_access_min, memory_access_max)->SetRangeMultiplier(2.0);
SKA_BENCHMARK("memory access", benchmark_memory_access_permutation)->SetBaseline("benchmark_memory_access_permutation_baseline")->SetRange(memory_access_min, memory_access_max)->SetRangeMultiplier(2.0);
SKA_BENCHMARK("memory access", benchmark_predictable_memory_access)->SetBaseline("benchmark_predictable_memory_access_baseline")->SetRange(memory_access_min, memory_access_max)->SetRangeMultiplier(2.0);
SKA_BENCHMARK("memory access", benchmark_sequential_memory_access)->SetBaseline("benchmark_sequential_memory_access_baseline")->SetRange(memory_access_min, memory_access_max)->SetRangeMultiplier(2.0);


int main(int argc, char * argv[])
{
    if (skb::RunSingleBenchmarkFromCommandLine(argc, argv))
        return 0;

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    if (result)
        return result;
}