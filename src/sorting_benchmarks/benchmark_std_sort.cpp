#include "sorting_benchmarks/benchmark_sort.hpp"

template<typename T>
struct BenchmarkStdSort
{
    void operator()(skb::State & state) const
    {
        size_t num_items = state.range(0);
        std::vector<T> input = sort_benchmark::SortInput<T>::sort_input(num_items);
        while (state.KeepRunning())
        {
            std::sort(input.begin(), input.end());
            sort_benchmark::noinline_shuffle(input.begin(), input.end());
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }
    static constexpr bool DoesSupportType = true;
};

void RegisterStdSort()
{
    sort_benchmark::RegisterSort<BenchmarkStdSort>("std::sort");
}

