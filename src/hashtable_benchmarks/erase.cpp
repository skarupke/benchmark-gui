#include "hashtable_benchmarks/erase.hpp"


template<typename T>
struct benchmark_erase_whole_table_baseline : hashtable_benchmark_base
{
    void operator()(skb::State & state) const
    {
        int num_items = state.range(0);
        std::vector<T> data;
        BenchmarkRandomDistribution<T> distribution;
        for (int i = 0; i < num_items; ++i)
            data.emplace_back(distribution(global_randomness));
        while (state.KeepRunning())
        {
            state.PauseTiming();
            std::shuffle(data.begin(), data.end(), global_randomness);
            state.ResumeTiming();
            for (const auto & elem : data)
            {
                skb::DoNotOptimize(elem);
            }
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }
};

void benchmark_erase_whole_table_baseline_int(skb::State & state)
{
    return benchmark_erase_whole_table_baseline<int>()(state);
}
void benchmark_erase_whole_table_baseline_string(skb::State & state)
{
    return benchmark_erase_whole_table_baseline<std::string>()(state);
}

SKA_BENCHMARK("baseline", benchmark_erase_whole_table_baseline_int);
SKA_BENCHMARK("baseline", benchmark_erase_whole_table_baseline_string);
