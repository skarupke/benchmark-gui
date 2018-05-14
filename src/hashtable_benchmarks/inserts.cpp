#include "hashtable_benchmarks/inserts.hpp"

void benchmark_insert_baseline_int(skb::State & state)
{
    int num_items = state.range(0);
    while (state.KeepRunning())
    {
        BenchmarkRandomDistribution<int> distribution;
        for (int i = 0; i < num_items; ++i)
        {
            skb::DoNotOptimize(no_inline_random_number(distribution, global_randomness));
            skb::DoNotOptimize(i);
        }
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
void benchmark_insert_baseline_string(skb::State & state)
{
    int num_items = state.range(0);
    while (state.KeepRunning())
    {
        BenchmarkRandomDistribution<std::string> distribution;
        for (int i = 0; i < num_items; ++i)
        {
            skb::DoNotOptimize(no_inline_random_number(distribution, global_randomness));
            skb::DoNotOptimize(i);
        }
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}

SKA_BENCHMARK("baseline", benchmark_insert_baseline_int);
SKA_BENCHMARK("baseline", benchmark_insert_baseline_string);

#if 0

std::vector<size_t> GetClusterSizes(const std::vector<bool> & table)
{
    std::vector<size_t> cluster_sizes;
    auto it = table.begin();
    auto end = table.end();
    if (*it)
    {
        auto end_it = std::find(it + 1, end, false);
        cluster_sizes.push_back(end_it - it);
        it = end_it + 1;
    }
    if (table.back())
    {
        auto last_block_end = std::find(table.rbegin(), table.rend(), false);
        if (cluster_sizes.empty())
            cluster_sizes.emplace_back();
        cluster_sizes[0] += last_block_end - table.rbegin();
    }
    for (;;)
    {
        it = std::find(it, end, true);
        if (it == end)
            return cluster_sizes;
        auto end_it = std::find(it + 1, end, false);
        if (end_it == end)
            return cluster_sizes;
        cluster_sizes.push_back(end_it - it);
        it = end_it + 1;
    }
}

std::vector<size_t> SimulateClusters(size_t size, float load_factor, std::mt19937_64 & randomness)
{
    std::vector<bool> table(size);
    size_t limit = size_t(size * load_factor);
    std::uniform_int_distribution<size_t> distribution(0, table.size() - 1);
    for (size_t i = 0; i < limit; ++i)
    {
        size_t position = distribution(randomness);
        while (table[position])
        {
            ++position;
            if (position == size)
                position = 0;
        }
        table[position] = true;
    }
    return GetClusterSizes(table);
}

#include "test/include_test.hpp"

TEST(clusters, get_cluster_sizes_simple)
{
    std::vector<std::pair<std::vector<bool>, std::vector<size_t>>> expected =
    {
        { { true, false, false }, { 1 } },
        { { false, false, false }, { } },
        { { true, true, true, false }, { 3 } },
        { { true, false, true }, { 2 } },
        { { true, false, true, false }, { 1, 1 } },
        { { true, false, true, true, false }, { 1, 2 } },
        { { true, false, true, true, false, true }, { 2, 2 } },
        { { true, false, true, true, false, true, false, true }, { 2, 2, 1 } },
        { { true, false, true, true, false, true, false, true, false }, { 1, 2, 1, 1 } },
        { { false, false, true, true, false, true, false, true, false }, { 2, 1, 1 } },
        { { false, false, true, true, false, true, false, true, true }, { 2, 2, 1 } },
    };
    for (const auto & pair : expected)
    {
        ASSERT_EQ(pair.second, GetClusterSizes(pair.first));
    }
}

void PrintClusterSizes(size_t size, float load_factor, std::mt19937_64 & randomness)
{
    std::vector<size_t> cluster_sizes = SimulateClusters(size, load_factor, randomness);
    std::sort(cluster_sizes.begin(), cluster_sizes.end(), std::greater<>());
    bool first = true;
    std::cout << "Clusters for size " << size << ", load factor " << load_factor << ":\n";
    for (size_t i : cluster_sizes)
    {
        if (first)
            first = false;
        else
            std::cout << ", ";
        std::cout << i;
    }
    std::cout << '\n' << std::endl;
}

TEST(clusters, simulate_clusters)
{
    std::mt19937_64 randomness(6);
    PrintClusterSizes(10, 0.5f, randomness);
    for (float load_factor : { 0.5f, 0.75f, 0.875f, 0.9375f })
        PrintClusterSizes(100, load_factor, randomness);
    for (float load_factor : { 0.5f, 0.75f, 0.875f, 0.9375f })
        PrintClusterSizes(1000, load_factor, randomness);
    for (float load_factor : { 0.5f, 0.75f, 0.875f, 0.9375f })
        PrintClusterSizes(10000, load_factor, randomness);
    for (float load_factor : { 0.5f, 0.75f, 0.875f, 0.9375f })
        PrintClusterSizes(100000, load_factor, randomness);
    for (float load_factor : { 0.5f, 0.75f, 0.875f, 0.9375f })
        PrintClusterSizes(1000000, load_factor, randomness);
}

#endif
