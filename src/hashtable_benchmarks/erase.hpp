#pragma once

#include "hashtable_benchmarks/inserts.hpp"
#include "hashtable_benchmarks/benchmark_shared.hpp"


template<typename T>
struct benchmark_erase_whole_table : hashtable_benchmark_base
{
    void operator()(skb::State & state) const
    {
        int num_items = state.range(0);
        std::vector<typename const_cast_pair<typename T::value_type>::type> data;
        using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
        distribution_type distribution;
        for (int i = 0; i < num_items; ++i)
            data.emplace_back(distribution(global_randomness), typename T::value_type::second_type());
        T map = TableCreator<T>()(max_load_factor);
        while (state.KeepRunning())
        {
            state.PauseTiming();
            map.insert(data.begin(), data.end());
            std::shuffle(data.begin(), data.end(), global_randomness);
            state.ResumeTiming();
            for (const auto & elem : data)
            {
                map.erase(elem.first);
            }
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }
};


