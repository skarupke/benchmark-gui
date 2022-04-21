#include "sorting_benchmarks/benchmark_sort.hpp"
#include <boost/sort/sort.hpp>


template<typename T>
struct BenchmarkSpreadSort
{
    void operator()(skb::State & state) const
    {
        size_t num_items = state.range(0);
        std::vector<T> input = sort_benchmark::SortInput<T>::sort_input(num_items);
        while (state.KeepRunning())
        {
            boost::sort::spreadsort::spreadsort(input.begin(), input.end());
            sort_benchmark::noinline_shuffle(input.begin(), input.end());
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }

    static constexpr bool DoesSupportType = !sort_benchmark::IsIntWithPadding<T>::value && !sort_benchmark::IsStdList<T>::value;
};
template<typename T>
struct BenchmarkPdqSort
{
    void operator()(skb::State & state) const
    {
        size_t num_items = state.range(0);
        std::vector<T> input = sort_benchmark::SortInput<T>::sort_input(num_items);
        while (state.KeepRunning())
        {
            boost::sort::pdqsort(input.begin(), input.end());
            sort_benchmark::noinline_shuffle(input.begin(), input.end());
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }

    static constexpr bool DoesSupportType = true;
};
template<typename T>
struct BenchmarkPdqSortBranchless
{
    void operator()(skb::State & state) const
    {
        size_t num_items = state.range(0);
        std::vector<T> input = sort_benchmark::SortInput<T>::sort_input(num_items);
        while (state.KeepRunning())
        {
            boost::sort::pdqsort_branchless(input.begin(), input.end());
            sort_benchmark::noinline_shuffle(input.begin(), input.end());
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }

    static constexpr bool DoesSupportType = true;
};

void RegisterBoostSorts()
{
    sort_benchmark::RegisterSort<BenchmarkSpreadSort>("boost::spreadsort");
    sort_benchmark::RegisterSort<BenchmarkPdqSort>("boost::pdqsort");
    sort_benchmark::RegisterSort<BenchmarkPdqSortBranchless>("boost::pdqsort_branchless");
}

