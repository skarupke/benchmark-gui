#include "util/heap.hpp"
#include "custom_benchmark/custom_benchmark.h"
#include "hashtable_benchmarks/benchmark_shared.hpp"
#include <algorithm>
#include <set>
#include "test/include_test.hpp"

void benchmark_heap_push(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            std::push_heap(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_std_multiset_push(skb::State & state)
{
    std::multiset<int> heap;
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness));
        skb::DoNotOptimize(*heap.begin());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_interval_heap_push(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            interval_heap_push(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_minmax_heap_push(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_minmax_heap(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<int D>
void benchmark_push_dary_heap(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_dary_heap<D>(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    //CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<D>(heap.begin(), heap.end()));
}

template<int D>
void benchmark_push_dary_heap_grandparent(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_dary_heap_grandparent<D>(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    //CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<D>(heap.begin(), heap.end()));
}
template<int D>
void benchmark_push_dary_heap_great_grandparent(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_dary_heap_great_grandparent<D>(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<D>(heap.begin(), heap.end()));
}
template<int D>
void benchmark_push_dary_heap_great_great_grandparent(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_dary_heap_great_great_grandparent<D>(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<D>(heap.begin(), heap.end()));
}

void benchmark_push_binary_heap_binary_search(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_binary_heap_binary_search(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<2>(heap.begin(), heap.end()));
}

void benchmark_push_binary_heap_binary_search_plus_one(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_binary_heap_binary_search_plus_one(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<2>(heap.begin(), heap.end()));
}

void benchmark_pairing_heap_push(skb::State & state)
{
    PairingHeap<int>::MemoryPool pool;
    PairingHeap<int> heap;
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
        {
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        }
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pairing_pair_heap_push(skb::State & state)
{
    PairingHeapPair<int>::MemoryPool pool;
    PairingHeapPair<int> heap;
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
        {
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        }
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<size_t MergeInterval>
void benchmark_pairing_push_heap_push(skb::State & state)
{
    typename PairingHeapMorePushWork<int, MergeInterval>::MemoryPool pool;
    PairingHeapMorePushWork<int, MergeInterval> heap;
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
        {
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        }
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_make_heap(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        std::make_heap(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_make_interval_heap(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        interval_heap_make(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_make_minmax_heap(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_minmax_heap(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<int D>
void benchmark_make_dary_heap(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<D>(heap.begin(), heap.end()));
}

void benchmark_pop_minmax_heap_min(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_minmax_heap(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_minmax_heap_min(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_minmax_heap_max(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_minmax_heap(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_minmax_heap_max(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        std::make_heap(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            std::pop_heap(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_std_multiset(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<std::multiset<int>> heaps;
    static constexpr size_t max_memory_usage = 2 * 1024 * 1024 * size_t(1024);
    static constexpr size_t memory_per_item = 4 * 8;
    static constexpr size_t max_cached_items = max_memory_usage / memory_per_item;
    int num_heaps = std::max(2, std::min(128, static_cast<int>(max_cached_items / num_items)));
    heaps.reserve(num_heaps);
    while (heaps.size() != heaps.capacity())
    {
        std::multiset<int> heap;
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness));
        heaps.push_back(std::move(heap));
    }
    std::uniform_int_distribution<int> random_heap(0, num_heaps - 1);
    while (state.KeepRunning())
    {
        std::multiset<int> heap = heaps[no_inline_random_number(random_heap, randomness)];
        skb::DoNotOptimize(*heap.begin());
        for (int i = 0; i < num_items; ++i)
            heap.erase(heap.begin());
        skb::DoNotOptimize(heap.begin() == heap.end());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_interval_heap_min(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        interval_heap_make(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            interval_heap_pop_min(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_interval_heap_max(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        interval_heap_make(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            interval_heap_pop_max(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<int D>
void benchmark_pop_dary_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_dary_heap<D>(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

template<int D>
void benchmark_pop_dary_heap_linear(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_dary_heap_linear<D>(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

void benchmark_pop_binary_heap_grandparent(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_grandparent(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

void benchmark_pop_quaternary_heap_grandparent(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<4>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_quaternary_heap_grandparent(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

void benchmark_pop_binary_heap_unrolled(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_unrolled(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}
void benchmark_pop_binary_heap_unrolled_grandparent(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_unrolled_grandparent(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}
void benchmark_pop_binary_heap_unrolled_8(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_unrolled_8(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}
void benchmark_pop_binary_heap_unrolled_2(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_unrolled_2(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

void benchmark_pop_binary_heap_unrolled_no_early_out(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_unrolled_no_early_out(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}
void benchmark_pop_quaternary_heap_unrolled(skb::State & state)
 {
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<4>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_quaternary_heap_unrolled(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}
void benchmark_pop_quaternary_heap_unrolled_2(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<4>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_quaternary_heap_unrolled_2(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

template<int D>
void benchmark_pop_dary_heap_half(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        for (int i = 0; i < (num_items / 2); ++i)
            pop_dary_heap<D>(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_dary_heap_make_std_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        std::make_heap(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_dary_heap<2>(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

void benchmark_pop_pairing_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeap<int> heap;
    PairingHeap<int>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        for (int i = 0; i < num_items; ++i)
            heap.delete_min(pool);
        skb::DoNotOptimize(heap.empty());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_pairing_pair_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeapPair<int> heap;
    PairingHeapPair<int>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        for (int i = 0; i < num_items; ++i)
            heap.delete_min(pool);
        skb::DoNotOptimize(heap.empty());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<size_t MergeInterval>
void benchmark_pop_pairing_push_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeapMorePushWork<int, MergeInterval> heap;
    typename PairingHeapMorePushWork<int, MergeInterval>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        for (int i = 0; i < num_items; ++i)
            heap.delete_min(pool);
        skb::DoNotOptimize(heap.empty());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_pairing_heap_half(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeap<int> heap;
    PairingHeap<int>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        for (int i = 0; i < (num_items / 2); ++i)
            heap.delete_min(pool);
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        for (int i = 0; i < num_items; ++i)
            skb::DoNotOptimize(no_inline_random_number(distribution, randomness));
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_heap_baseline);

void benchmark_make_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        std::make_heap(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_make_heap_baseline);

void benchmark_copy_std_multiset_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<std::multiset<int>> heaps;
    static constexpr size_t max_memory_usage = 2 * 1024 * 1024 * size_t(1024);
    static constexpr size_t memory_per_item = 4 * 8;
    static constexpr size_t max_cached_items = max_memory_usage / memory_per_item;
    int num_heaps = std::max(2, std::min(128, static_cast<int>(max_cached_items / num_items)));
    heaps.reserve(num_heaps);
    while (heaps.size() != heaps.capacity())
    {
        std::multiset<int> heap;
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness));
        heaps.push_back(std::move(heap));
    }
    std::uniform_int_distribution<int> random_heap(0, num_heaps - 1);
    while (state.KeepRunning())
    {
        std::multiset<int> heap = heaps[no_inline_random_number(random_heap, randomness)];
        skb::DoNotOptimize(*heap.begin());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_copy_std_multiset_baseline);

void benchmark_make_minmax_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_minmax_heap(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_make_minmax_heap_baseline);

void benchmark_make_interval_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        interval_heap_make(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_make_interval_heap_baseline);

void benchmark_make_pairing_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeap<int> heap;
    PairingHeap<int>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_make_pairing_heap_baseline);

void benchmark_make_pairing_pair_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeapPair<int> heap;
    PairingHeapPair<int>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_make_pairing_pair_heap_baseline);

template<size_t MergeInterval>
void benchmark_make_pairing_push_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeapMorePushWork<int, MergeInterval> heap;
    typename PairingHeapMorePushWork<int, MergeInterval>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<int D>
void benchmark_make_dary_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<typename It, typename Compare>
It min_element_four(It begin, It end, Compare && compare)
{
    size_t size = end - begin;
    if (!size)
        return end;
    size_t num_loops = (size - 1) / 4;
    It smallest = begin;
    ++begin;
    for (; num_loops > 0; --num_loops)
    {
        It min_first_half = begin + !!compare(begin[1], begin[0]);
        It min_second_half = begin + 2 + !!compare(begin[3], begin[2]);
        It min_four = compare(*min_second_half, *min_first_half) ? min_second_half : min_first_half;
        if (compare(*min_four, *smallest))
            smallest = min_four;
        begin += 4;
    }
    for (; begin != end; ++begin)
    {
        if (compare(*begin, *smallest))
            smallest = begin;
    }
    return smallest;
}

template<typename It>
It min_element_four(It begin, It end)
{
    return min_element_four(begin, end, std::less<>{});
}
template<typename It, typename Compare>
It min_element_four_branchy(It begin, It end, Compare && compare)
{
    size_t size = end - begin;
    if (!size)
        return end;
    size_t num_loops = (size - 1) / 4;
    It smallest = begin;
    ++begin;
    for (; num_loops > 0; --num_loops)
    {
        It min_first_half = compare(begin[1], begin[0]) ? begin + 1 : begin;
        It min_second_half = compare(begin[3], begin[2]) ? begin + 3 : begin + 2;
        It min_four = compare(*min_second_half, *min_first_half) ? min_second_half : min_first_half;
        if (compare(*min_four, *smallest))
            smallest = min_four;
        begin += 4;
    }
    for (; begin != end; ++begin)
    {
        if (compare(*begin, *smallest))
            smallest = begin;
    }
    return smallest;
}

template<typename It>
It min_element_four_branchy(It begin, It end)
{
    return min_element_four_branchy(begin, end, std::less<>{});
}
template<typename It, typename Compare>
It min_element_four_parallel(It begin, It end, Compare && compare)
{
    size_t size = end - begin;
    if (!size)
        return end;
    size_t num_loops = size / 4;
    It smallest0 = begin;
    ++begin;
    if (num_loops)
    {
        It smallest1 = begin;
        It smallest2 = begin + 1;
        It smallest3 = begin + 2;
        begin += 3;
        for (--num_loops; num_loops > 0; --num_loops)
        {
            if (compare(begin[0], *smallest0))
                smallest0 = begin;
            if (compare(begin[1], *smallest1))
                smallest1 = begin + 1;
            if (compare(begin[2], *smallest2))
                smallest2 = begin + 2;
            if (compare(begin[3], *smallest3))
                smallest3 = begin + 3;
            begin += 4;
        }
        if (compare(*smallest3, *smallest2))
            smallest2 = smallest3;
        if (compare(*smallest1, *smallest0))
            smallest0 = smallest1;
        if (compare(*smallest2, *smallest0))
            smallest0 = smallest2;
    }
    for (; begin != end; ++begin)
    {
        if (compare(*begin, *smallest0))
            smallest0 = begin;
    }
    return smallest0;
}

template<typename It>
It min_element_four_parallel(It begin, It end)
{
    return min_element_four_parallel(begin, end, std::less<>{});
}

TEST(min_element_four_heap, cases)
{
    std::vector<int> input = { 1, 2, 3, 4, 5, 6 };
    ASSERT_EQ(1, *min_element_four(input.begin(), input.end()));
    input = { 1, 2, 3, 4, 5, 6, 0 };
    ASSERT_EQ(0, *min_element_four(input.begin(), input.end()));
    input = { 1, 2, 3, 4, -1, 5, 6, 0 };
    ASSERT_EQ(-1, *min_element_four(input.begin(), input.end()));
    input = { 1, 2, 3, 4, -1, 5, 6, -2, 0 };
    ASSERT_EQ(-2, *min_element_four(input.begin(), input.end()));
}
TEST(min_element_four_heap, parallel_cases)
{
    std::vector<int> input = { 1, 2, 3 };
    ASSERT_EQ(1, *min_element_four_parallel(input.begin(), input.end()));
    input = { 1, 2, 3, 0 };
    ASSERT_EQ(0, *min_element_four_parallel(input.begin(), input.end()));
    input = { 1, 2, 3, 4, -1, 5, 6, 0 };
    ASSERT_EQ(-1, *min_element_four_parallel(input.begin(), input.end()));
    input = { 1, 2, 3, 4, -1, 5, 6, -2, 0 };
    ASSERT_EQ(-2, *min_element_four_parallel(input.begin(), input.end()));
}

void benchmark_min_element(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> data;
    data.reserve(num_items);
    while (state.KeepRunning())
    {
        data.clear();
        for (int i = 0; i < num_items; ++i)
            data.push_back(no_inline_random_number(distribution, randomness));
        skb::DoNotOptimize(*std::min_element(data.begin(), data.end()));
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}

void benchmark_min_element_four(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> data;
    data.reserve(num_items);
    while (state.KeepRunning())
    {
        data.clear();
        for (int i = 0; i < num_items; ++i)
            data.push_back(no_inline_random_number(distribution, randomness));
        skb::DoNotOptimize(*min_element_four(data.begin(), data.end()));
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}

void benchmark_min_element_branchy(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> data;
    data.reserve(num_items);
    while (state.KeepRunning())
    {
        data.clear();
        for (int i = 0; i < num_items; ++i)
            data.push_back(no_inline_random_number(distribution, randomness));
        skb::DoNotOptimize(*min_element_four_branchy(data.begin(), data.end()));
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
void benchmark_min_element_parallel(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> data;
    data.reserve(num_items);
    while (state.KeepRunning())
    {
        data.clear();
        for (int i = 0; i < num_items; ++i)
            data.push_back(no_inline_random_number(distribution, randomness));
        skb::DoNotOptimize(*min_element_four_parallel(data.begin(), data.end()));
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}

skb::Benchmark * SetHeapRange(skb::Benchmark * bm)
{
    return bm->SetRange(4, 1024*1024*1024)->SetRangeMultiplier(std::pow(2.0, 0.25));
}

void RegisterHeapBenchmarks()
{
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<2>, "baseline", "benchmark_make_dary_heap_baseline_2");
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<3>, "baseline", "benchmark_make_dary_heap_baseline_3");
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<4>, "baseline", "benchmark_make_dary_heap_baseline_4");
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<5>, "baseline", "benchmark_make_dary_heap_baseline_5");
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<6>, "baseline", "benchmark_make_dary_heap_baseline_6");
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<8>, "baseline", "benchmark_make_dary_heap_baseline_8");
    skb::CategoryBuilder builder;
    {
        skb::CategoryBuilder push = builder.AddCategory("operation", "push");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_heap_push, push.BuildCategories("heap", "std::heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_std_multiset_push, push.BuildCategories("heap", "std::multiset"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_minmax_heap_push, push.BuildCategories("heap", "minmax_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_interval_heap_push, push.BuildCategories("heap", "interval_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<2>, push.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<3>, push.AddCategory("dary_heap d", "3").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<4>, push.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<5>, push.AddCategory("dary_heap d", "5").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<6>, push.AddCategory("dary_heap d", "6").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<8>, push.AddCategory("dary_heap d", "8").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<2>, push.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<3>, push.AddCategory("dary_heap d", "3").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<4>, push.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<5>, push.AddCategory("dary_heap d", "5").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<6>, push.AddCategory("dary_heap d", "6").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<8>, push.AddCategory("dary_heap d", "8").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_great_grandparent<2>, push.AddCategory("dary_heap d", "2").AddCategory("grandparent", "great").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_great_grandparent<4>, push.AddCategory("dary_heap d", "4").AddCategory("grandparent", "great").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_great_great_grandparent<2>, push.AddCategory("dary_heap d", "2").AddCategory("grandparent", "great_great").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_great_great_grandparent<4>, push.AddCategory("dary_heap d", "4").AddCategory("grandparent", "great_great").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_binary_heap_binary_search, push.AddCategory("dary_heap d", "2").AddCategory("grandparent", "binary").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_binary_heap_binary_search_plus_one, push.AddCategory("dary_heap d", "2").AddCategory("grandparent", "binary+1").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pairing_heap_push, push.BuildCategories("heap", "pairing_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pairing_pair_heap_push, push.BuildCategories("heap", "pairing_heap_pair"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pairing_push_heap_push<4>, push.AddCategory("merge interval", "4").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pairing_push_heap_push<8>, push.AddCategory("merge interval", "8").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pairing_push_heap_push<16>, push.AddCategory("merge interval", "16").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_heap_baseline"));
    }
    {
        skb::CategoryBuilder make = builder.AddCategory("operation", "make");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_heap, make.BuildCategories("heap", "std::heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_interval_heap, make.BuildCategories("heap", "interval_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_minmax_heap, make.BuildCategories("heap", "minmax_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<2>, make.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<3>, make.AddCategory("dary_heap d", "3").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<4>, make.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<5>, make.AddCategory("dary_heap d", "5").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<6>, make.AddCategory("dary_heap d", "6").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<8>, make.AddCategory("dary_heap d", "8").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
    }
    {
        skb::CategoryBuilder pop = builder.AddCategory("operation", "pop");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_minmax_heap_min, pop.AddCategory("minmax pop", "min").BuildCategories("heap", "minmax_heap"))->SetBaseline("benchmark_make_minmax_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_minmax_heap_max, pop.AddCategory("minmax pop", "max").BuildCategories("heap", "minmax_heap"))->SetBaseline("benchmark_make_minmax_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_interval_heap_min, pop.AddCategory("minmax pop", "min").BuildCategories("heap", "interval_heap"))->SetBaseline("benchmark_make_interval_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_interval_heap_max, pop.AddCategory("minmax pop", "max").BuildCategories("heap", "interval_heap"))->SetBaseline("benchmark_make_interval_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_heap, pop.BuildCategories("heap", "std::heap"))->SetBaseline("benchmark_make_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_std_multiset, pop.BuildCategories("heap", "std::multiset"))->SetBaseline("benchmark_copy_std_multiset_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<2>, pop.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<3>, pop.AddCategory("dary_heap d", "3").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_3"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<4>, pop.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_4"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap_linear<4>, pop.AddCategory("dary_heap d", "4").AddCategory("compare", "linear").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_4"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<5>, pop.AddCategory("dary_heap d", "5").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_5"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<6>, pop.AddCategory("dary_heap d", "6").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_6"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<8>, pop.AddCategory("dary_heap d", "8").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_8"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap_linear<8>, pop.AddCategory("dary_heap d", "8").AddCategory("compare", "linear").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_8"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_grandparent, pop.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_quaternary_heap_grandparent, pop.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_make_dary_heap_baseline_4"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_unrolled, pop.AddCategory("dary_heap d", "2").AddCategory("unrolled", "with_early_out").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_unrolled_grandparent, pop.AddCategory("dary_heap d", "2").AddCategory("unrolled", "with_early_out").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_unrolled_8, pop.AddCategory("dary_heap d", "2").AddCategory("unrolled", "with_early_out_8").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_unrolled_2, pop.AddCategory("dary_heap d", "2").AddCategory("unrolled", "with_early_out_2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_unrolled_no_early_out, pop.AddCategory("dary_heap d", "2").AddCategory("unrolled", "no_early_out").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_quaternary_heap_unrolled, pop.AddCategory("dary_heap d", "4").AddCategory("unrolled", "with_early_out").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_4"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_quaternary_heap_unrolled_2, pop.AddCategory("dary_heap d", "4").AddCategory("unrolled", "with_early_out_2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_4"));
        //SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap_make_std_heap, pop.AddCategory("dary_heap d", "2").BuildCategories("heap", "pop_dary_heap_make_std_heap"))->SetBaseline("benchmark_make_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_heap, pop.BuildCategories("heap", "pairing_heap"))->SetBaseline("benchmark_make_pairing_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_pair_heap, pop.BuildCategories("heap", "pairing_heap_pair"))->SetBaseline("benchmark_make_pairing_pair_heap_baseline"));
        SKA_BENCHMARK_NAME(&benchmark_make_pairing_push_heap_baseline<4>, "baseline", "benchmark_make_pairing_push_heap_baseline_4");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_push_heap<4>, pop.AddCategory("merge interval", "4").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_make_pairing_push_heap_baseline_4"));
        SKA_BENCHMARK_NAME(&benchmark_make_pairing_push_heap_baseline<8>, "baseline", "benchmark_make_pairing_push_heap_baseline_8");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_push_heap<8>, pop.AddCategory("merge interval", "8").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_make_pairing_push_heap_baseline_8"));
        SKA_BENCHMARK_NAME(&benchmark_make_pairing_push_heap_baseline<16>, "baseline", "benchmark_make_pairing_push_heap_baseline_16");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_push_heap<16>, pop.AddCategory("merge interval", "16").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_make_pairing_push_heap_baseline_16"));
        skb::CategoryBuilder pop_half = builder.AddCategory("operation", "pop_half");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap_half<2>, pop_half.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_heap_half, pop_half.BuildCategories("heap", "pairing_heap"))->SetBaseline("benchmark_heap_baseline"));
    }
    {
        skb::CategoryBuilder min_element = builder.AddCategory("instruction", "min_element");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_min_element, min_element.BuildCategories("instructions", "min_element"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_min_element_four, min_element.BuildCategories("instructions", "min_element_four"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_min_element_branchy, min_element.BuildCategories("instructions", "min_element_branchy"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_min_element_parallel, min_element.BuildCategories("instructions", "min_element_parallel"))->SetBaseline("benchmark_heap_baseline"));
    }
}


int main(int argc, char * argv[])
{
    RegisterHeapBenchmarks();
    if (skb::RunSingleBenchmarkFromCommandLine(argc, argv))
        return 0;

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    if (result)
        return result;
}
