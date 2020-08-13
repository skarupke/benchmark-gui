
#include "util/regions_sort_copy.hpp"
#include "sorting_benchmarks/benchmark_sort.hpp"

template<int sub_sorts>
struct BenchmarkWrapper
{
    template<typename T>
    struct BenchmarkRegionsSort
    {
        struct SortSettings : boost_ska_sort::sort::detail_ska_sort::default_sort_settings
        {
            static constexpr int num_sub_sorts = sub_sorts;
        };

        void operator()(skb::State & state) const
        {
            size_t num_items = state.range(0);
            std::vector<T> input = sort_benchmark::SortInput<T>::sort_input(num_items);
            while (state.KeepRunning())
            {
                boost_ska_sort::sort::regions_sort_with_settings<SortSettings>(input.begin(), input.end());
                sort_benchmark::noinline_shuffle(input.begin(), input.end());
            }
            state.SetItemsProcessed(state.iterations() * num_items);
            boost_ska_sort::sort::regions_sort(input.begin(), input.end());
            CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(input.begin(), input.end()));
        }

        static constexpr bool DoesSupportType = true;
    };
};

void RegisterRegionsSort()
{
    skb::CategoryBuilder categories_so_far;
    sort_benchmark::RegisterSort<BenchmarkWrapper<1>::BenchmarkRegionsSort>("regions sort", categories_so_far.AddCategory("num_sub_sorts", "1"));
    sort_benchmark::RegisterSort<BenchmarkWrapper<2>::BenchmarkRegionsSort>("regions sort", categories_so_far.AddCategory("num_sub_sorts", "2"));
    sort_benchmark::RegisterSort<BenchmarkWrapper<4>::BenchmarkRegionsSort>("regions sort", categories_so_far.AddCategory("num_sub_sorts", "4"));
    sort_benchmark::RegisterSort<BenchmarkWrapper<8>::BenchmarkRegionsSort>("regions sort", categories_so_far.AddCategory("num_sub_sorts", "8"));
    sort_benchmark::RegisterSort<BenchmarkWrapper<16>::BenchmarkRegionsSort>("regions sort", categories_so_far.AddCategory("num_sub_sorts", "16"));
}
