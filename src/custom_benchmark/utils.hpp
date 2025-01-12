#pragma once

#include <random>
#include "custom_benchmark/custom_benchmark.h"

using profiling_randomness = std::mt19937_64;

extern thread_local profiling_randomness global_randomness;

template<typename Distribution, typename Randomness>
SKA_NOINLINE(decltype(auto)) no_inline_random_number(Distribution & distribution, Randomness & randomness)
{
    return distribution(randomness);
}

template<typename T>
struct BenchmarkRandomDistribution;

template<>
struct BenchmarkRandomDistribution<int>
{
    int operator()(profiling_randomness & randomness)
    {
        return distribution(randomness);
    }

private:
    std::uniform_int_distribution<int> distribution;
};
const std::vector<const char *> & get_word_list();
template<>
struct BenchmarkRandomDistribution<std::string>
{
    std::string operator()(profiling_randomness & randomness)
    {
        int num_words = random_num_words(randomness);
        std::string result;
        const std::vector<const char *> & word_list = get_word_list();
        for (int i = 0; i < num_words; ++i)
        {
            result += word_list[random_word(randomness)];
        }
        return result;
    }

private:
    std::uniform_int_distribution<int> random_num_words{1, 10};
    std::uniform_int_distribution<size_t> random_word{0, get_word_list().size() - 1};
};

struct KeyPlaceHolder
{
};
struct ValuePlaceHolder
{
};

struct hashtable_benchmark_base
{
    float max_load_factor = 0.0f;
};

struct TrackAllocations
{
    TrackAllocations(skb::State & state);
    ~TrackAllocations();

    skb::State & state;
    size_t num_bytes_allocated_before;
    size_t num_bytes_freed_before;
};

template<typename T>
struct TableCreator
{
    T operator()(float max_load_factor) const
    {
        T result;
        if (max_load_factor > 0.0f)
            result.max_load_factor(max_load_factor);
        return result;
    }
    T operator()(float max_load_factor, int num_items) const
    {
        T result;
        if (max_load_factor > 0.0f)
            result.max_load_factor(max_load_factor);
        result.reserve(num_items);
        return result;
    }
};

template<typename T>
struct const_cast_pair;
template<typename F, typename S>
struct const_cast_pair<std::pair<F, S>>
{
    using type = std::pair<std::remove_const_t<F>, std::remove_const_t<S>>;
};

