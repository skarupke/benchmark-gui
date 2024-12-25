#pragma once


#include <custom_benchmark/custom_benchmark.h>
#include "hashtable_benchmarks/benchmark_shared.hpp"
#include <list>
#include <vector>

extern const std::vector<const char *> & get_word_list();
extern const std::vector<const char *> & get_kern_log();
namespace sort_benchmark
{

template<size_t Size>
struct IntWithPadding
{
    int data[Size / 4];

    IntWithPadding(int value)
    {
        std::fill(std::begin(data), std::end(data), value);
    }

    int to_sort_on() const
    {
        return data[Size / 8 - 1];
    }
    int & padding_byte()
    {
        return data[Size / 8];
    }

    bool operator<(const IntWithPadding & other) const
    {
        return to_sort_on() < other.to_sort_on();
    }
};
template<size_t Size>
int to_radix_sort_key(const IntWithPadding<Size> & int_with_padding)
{
    return int_with_padding.to_sort_on();
}
template<typename>
struct IsIntWithPadding
{
    static constexpr bool value = false;
};
template<size_t Size>
struct IsIntWithPadding<IntWithPadding<Size>>
{
    static constexpr bool value = true;
};

template<typename T>
struct IsStdList
{
    static constexpr bool value = false;
};
template<typename T, typename A>
struct IsStdList<std::list<T, A>>
{
    static constexpr bool value = true;
};

template<typename T>
struct SortInput
{
    static SKA_NOINLINE(std::vector<T>) sort_input(size_t num_items)
    {
        std::uniform_int_distribution<T> distribution;
        std::vector<T> result(num_items);
        for (T & item : result)
            item = distribution(global_randomness);
        return result;
    }
};
template<size_t Size>
struct SortInput<IntWithPadding<Size>>
{
    static SKA_NOINLINE(std::vector<IntWithPadding<Size>>) sort_input(size_t num_items)
    {
        std::uniform_int_distribution<int> distribution;
        std::vector<IntWithPadding<Size>> result;
        result.reserve(num_items);
        for (size_t i = 0; i < num_items; ++i)
            result.emplace_back(distribution(global_randomness));
        return result;
    }
};
template<>
struct SortInput<float>
{
    static SKA_NOINLINE(std::vector<float>) sort_input(size_t num_items)
    {
        std::normal_distribution<float> distribution(0.0f, 1000.0f);
        std::vector<float> result(num_items);
        for (float & item : result)
            item = distribution(global_randomness);
        return result;
    }
};
template<>
struct SortInput<double>
{
    static SKA_NOINLINE(std::vector<double>) sort_input(size_t num_items)
    {
        std::normal_distribution<double> distribution(0.0, 1000000.0);
        std::vector<double> result(num_items);
        for (double & item : result)
            item = distribution(global_randomness);
        return result;
    }
};
template<>
struct SortInput<std::string>
{
    static SKA_NOINLINE(std::vector<std::string>) sort_input(size_t num_items)
    {
        std::uniform_int_distribution<int> list_choice(0, 10);
        std::uniform_int_distribution<int> num_words(1, 10);
        const std::vector<const char *> & word_list = get_word_list();
        const std::vector<const char *> & kern_log = get_kern_log();
        std::uniform_int_distribution<size_t> random_word(0, word_list.size() - 1);
        std::uniform_int_distribution<size_t> random_log(0, kern_log.size() - 1);
        std::vector<std::string> result(num_items);
        for (std::string & item : result)
        {
            if (list_choice(global_randomness) != 0)
            {
                for (int i = 0, end = num_words(global_randomness); i < end; ++i)
                {
                    item += word_list[random_word(global_randomness)];
                }
            }
            else
            {
                item = kern_log[random_log(global_randomness)];
            }
        }
        return result;
    }
};
template<typename T>
struct SortInput<std::list<T>>
{
    static SKA_NOINLINE(std::vector<std::list<T>>) sort_input(size_t num_items)
    {
        std::vector<std::string> string_input = SortInput<std::string>::sort_input(num_items);
        std::vector<std::list<T>> result;
        result.reserve(string_input.size());
        for (const std::string & str : string_input)
        {
            result.emplace_back();
            std::list<T> & to_build = result.back();
            for (char c : str)
                to_build.emplace_back(c);
        }
        return result;
    }
};

template<typename It>
SKA_NOINLINE(void) noinline_shuffle(It begin, It end)
{
    std::shuffle(begin, end, global_randomness);
}


template<typename T>
void benchmark_sort_baseline(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}

template<typename T>
inline skb::Benchmark * SortRange(skb::Benchmark * benchmark, double range_multiplier = std::sqrt(2.0))
{
    size_t max = 256 * 1024 * 1024;
    size_t max_memory = size_t(8) * size_t(1024) * size_t(1024) * size_t(1024);
    max = std::min(std::min(max, max_memory / sizeof(T)), static_cast<size_t>(std::numeric_limits<int>::max()));
    //if (sizeof(T) < sizeof(int))
    //    max = std::min(max, static_cast<int>(std::numeric_limits<T>::max()));
    return benchmark->SetRange(4, static_cast<int>(max))->SetRangeMultiplier(range_multiplier);
}

template<>
inline skb::Benchmark * SortRange<std::string>(skb::Benchmark * benchmark, double range_multiplier)
{
    int max = 4 * 1024 * 1024;
    return benchmark->SetRange(4, max)->SetRangeMultiplier(range_multiplier);
}
template<>
inline skb::Benchmark * SortRange<std::list<int>>(skb::Benchmark * benchmark, double range_multiplier)
{
    int max = 4 * 1024 * 1024;
    return benchmark->SetRange(4, max)->SetRangeMultiplier(range_multiplier);
}

inline interned_string get_baseline_sort_name_for_type(std::string_view baseline_type)
{
    return interned_string{ "baseline_sorting_" + std::string(baseline_type.data(), baseline_type.size()) };
}

template<template<typename> typename SortBenchmark, typename T>
void RegisterSortForType(skb::CategoryBuilder categories_so_far, interned_string name, interned_string sorted_type, std::string_view baseline_type)
{
    categories_so_far = categories_so_far.AddCategory("sorted_type", sorted_type);
    interned_string baseline_name = get_baseline_sort_name_for_type(baseline_type);

    if constexpr (SortBenchmark<T>::DoesSupportType)
        SortRange<T>(SKA_BENCHMARK_CATEGORIES(SortBenchmark<T>{}, categories_so_far.BuildCategories("sorting", name))->SetBaseline(baseline_name));
    else
    {
        static_cast<void>(name);
        static_cast<void>(baseline_name);
    }
}

template<template<typename> typename SortBenchmark>
void RegisterSort(interned_string name, skb::CategoryBuilder categories_so_far = {})
{
    #ifdef DONE_PULLING_OUT_BINARY
    using namespace std::string_view_literals;
    RegisterSortForType<SortBenchmark, int>(categories_so_far, name, "int", "int"sv);
    RegisterSortForType<SortBenchmark, uint8_t>(categories_so_far, name, "uint8_t", "uint8_t"sv);
    RegisterSortForType<SortBenchmark, float>(categories_so_far, name, "float", "float"sv);
    RegisterSortForType<SortBenchmark, double>(categories_so_far, name, "double", "double"sv);
    RegisterSortForType<SortBenchmark, std::string>(categories_so_far, name, "string", "string"sv);
    RegisterSortForType<SortBenchmark, std::list<int>>(categories_so_far, name, "std::list", "std::list"sv);

    RegisterSortForType<SortBenchmark, IntWithPadding<8>>(categories_so_far.AddCategory("struct size", "8"), name, "int", "int_size_8"sv);
    //RegisterSortForType<SortBenchmark, IntWithPadding<16>>(categories_so_far.AddCategory("struct size", "16"), name, "int", "int_size_16"sv);
    //RegisterSortForType<SortBenchmark, IntWithPadding<32>>(categories_so_far.AddCategory("struct size", "32"), name, "int", "int_size_32"sv);
    RegisterSortForType<SortBenchmark, IntWithPadding<64>>(categories_so_far.AddCategory("struct size", "64"), name, "int", "int_size_64"sv);
    RegisterSortForType<SortBenchmark, IntWithPadding<256>>(categories_so_far.AddCategory("struct size", "256"), name, "int", "int_size_256"sv);
    RegisterSortForType<SortBenchmark, IntWithPadding<1024>>(categories_so_far.AddCategory("struct size", "1024"), name, "int", "int_size_1024"sv);
    #else
    static_cast<void>(name);
    static_cast<void>(categories_so_far);
    #endif
}


}
