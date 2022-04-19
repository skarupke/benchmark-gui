#include "sorting_benchmarks/benchmark_sort.hpp"
#include "util/learned_sort.h"

template<typename T>
struct BenchmarkLearnedSort
{
    void operator()(skb::State & state) const
    {
        size_t num_items = state.range(0);
        std::vector<T> input = sort_benchmark::SortInput<T>::sort_input(num_items);
        while (state.KeepRunning())
        {
            learned_sort::sort(input.begin(), input.end());
            sort_benchmark::noinline_shuffle(input.begin(), input.end());
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }
    static constexpr bool DoesSupportType = !sort_benchmark::IsIntWithPadding<T>::value && !sort_benchmark::IsStdList<T>::value && !std::is_same_v<std::string, T>;
};
template<typename T>
struct BenchmarkLearnedSort2
{
    void operator()(skb::State & state) const
    {
        size_t num_items = state.range(0);
        std::vector<T> input = sort_benchmark::SortInput<T>::sort_input(num_items);
        while (state.KeepRunning())
        {
            learned_sort2::learned_sort::sort(input.begin(), input.end());
            sort_benchmark::noinline_shuffle(input.begin(), input.end());
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }
    static constexpr bool DoesSupportType = !sort_benchmark::IsIntWithPadding<T>::value && !sort_benchmark::IsStdList<T>::value && !std::is_same_v<std::string, T>;
};
void RegisterLearnedSort()
{
    sort_benchmark::RegisterSort<BenchmarkLearnedSort>("learned_sort");
    sort_benchmark::RegisterSort<BenchmarkLearnedSort2>("learned_sort2");
}
