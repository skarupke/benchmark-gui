#pragma once
#include "custom_benchmark/custom_benchmark.h"
#include "hashtable_benchmarks/benchmark_shared.hpp"


template<typename T, typename... Args>
void hashtable_emplace(T & table, Args &&... args)
{
    table.emplace(std::forward<Args>(args)...);
}

template<typename T>
struct benchmark_insert : hashtable_benchmark_base
{
    void operator()(skb::State & state) const
    {
        int num_items = state.range(0);

        while (state.KeepRunning())
        {
            using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
            distribution_type distribution;
            T map = reserve ? TableCreator<T>()(max_load_factor, num_items) : TableCreator<T>()(max_load_factor);
            for (int i = 0; i < num_items; ++i)
            {
                hashtable_emplace(map, no_inline_random_number(distribution, global_randomness), typename T::value_type::second_type());
            }
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }

    bool reserve = false;
};
