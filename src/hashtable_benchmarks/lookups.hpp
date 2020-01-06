#pragma once

#include "custom_benchmark/custom_benchmark.h"
#include "hashtable_benchmarks/benchmark_shared.hpp"
#include "container/flat_hash_map.hpp"
#include "hashtable_benchmarks/counting_allocator.hpp"
#include "hashtable_benchmarks/inserts.hpp"
#include "hashtable_benchmarks/erase.hpp"

void RegisterHashtableBenchmarks();

static constexpr const size_t num_lookup_loops = 10000;

template<typename T>
struct SpecialKeys;

template<>
struct SpecialKeys<int>
{
    static int empty_key()
    {
        return std::numeric_limits<int>::lowest();
    }
    static int deleted_key()
    {
        return std::numeric_limits<int>::lowest() + 1;
    }
};
template<>
struct SpecialKeys<std::string>
{
    static std::string empty_key()
    {
        return std::string(1, '\0');
    }
    static std::string deleted_key()
    {
        return std::string(1, char(255));
    }
};

template<typename T, typename Distribution>
std::vector<typename const_cast_pair<T>::type> create_random_values(profiling_randomness & randomness, int num_items)
{
    std::vector<typename const_cast_pair<T>::type> values;
    values.reserve(num_items);
    Distribution distribution;
    for (int i = 0; i < num_items; ++i)
    {
        if constexpr (std::is_same<std::string, typename T::second_type>::value)
        {
            values.emplace_back(distribution(randomness), std::to_string(i));
        }
        else
        {
            values.emplace_back(distribution(randomness), i);
        }
    }
    std::shuffle(values.begin(), values.end(), randomness);
    return values;
}

template<typename T, typename F, typename S>
void insert_into_map(T & map, std::vector<std::pair<F, S>> values, skb::State & state)
{
    TrackAllocations track_allocations(state);
    for (auto & value : values)
        map.insert(std::move(value));
}

static constexpr size_t l3_cache_size = 6 * 1024 * 1024;

template<typename T>
std::vector<T> copy_tables_to_overflow_cache(const T & table)
{
    size_t size = sizeof(T) + sizeof(typename T::value_type) * table.size();
    size_t num_tables = std::max(size_t(2), (100 * l3_cache_size) / size);
    return std::vector<T>(num_tables, table);
}

template<typename T>
struct benchmark_successful_lookup : hashtable_benchmark_base
{
    void operator()(skb::State & state) const
    {
        T map = TableCreator<T>()(max_load_factor);
        const profiling_randomness random_state = global_randomness;
        int num_items = state.range(0);

        using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
        insert_into_map(map, create_random_values<typename T::value_type, distribution_type>(global_randomness, num_items), state);

        while (state.KeepRunning())
        {
            distribution_type distribution;
            global_randomness = random_state;
            for (int i = 0; i < num_items; ++i)
                skb::DoNotOptimize(map.find(no_inline_random_number(distribution, global_randomness))->second);
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }
};

template<typename T>
struct benchmark_successful_lookup_cache_miss : hashtable_benchmark_base
{
    void operator()(skb::State & state) const
    {
        T map = TableCreator<T>()(max_load_factor);
        const profiling_randomness random_state = global_randomness;
        int num_items = state.range(0);

        using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
        insert_into_map(map, create_random_values<typename T::value_type, distribution_type>(global_randomness, num_items), state);

        std::vector<T> maps = copy_tables_to_overflow_cache(map);
        std::uniform_int_distribution<size_t> map_distribution(0, maps.size() - 1);
        while (state.KeepRunning())
        {
            distribution_type distribution;
            std::mt19937_64 key_randomness = random_state;
            for (int i = 0; i < num_items; ++i)
            {
                T & current_map = maps[no_inline_random_number(map_distribution, global_randomness)];
                skb::DoNotOptimize(current_map.find(no_inline_random_number(distribution, key_randomness))->second);
            }
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }
};

template<typename T>
struct benchmark_successful_lookup_predictable : hashtable_benchmark_base
{
    using generator_type = std::function<std::vector<typename const_cast_pair<typename T::value_type>::type> (profiling_randomness &, int)>;
    benchmark_successful_lookup_predictable(generator_type generator)
        : generator(std::move(generator))
    {
    }

    generator_type generator;
    void operator()(skb::State & state) const
    {
        T map = TableCreator<T>()(max_load_factor);

        insert_into_map(map, generator(global_randomness, state.range(0)), state);
        std::vector<typename T::key_type> random_keys;
        random_keys.reserve(map.size());
        for (auto & entry : map)
            random_keys.push_back(entry.first);

        std::shuffle(random_keys.begin(), random_keys.end(), global_randomness);
        while (state.KeepRunning())
        {
            for (const auto & i : random_keys)
                skb::DoNotOptimize(map.find(i)->second);
        }
        state.SetItemsProcessed(state.iterations() * random_keys.size());
    }
};

template<typename T>
struct benchmark_successful_lookup_predictable_cache_miss : hashtable_benchmark_base
{
    void operator()(skb::State & state) const
    {
        T map = TableCreator<T>()(max_load_factor);

        using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
        insert_into_map(map, create_random_values<typename T::value_type, distribution_type>(global_randomness, state.range(0)), state);
        std::vector<typename T::key_type> random_keys;
        random_keys.reserve(map.size());
        for (auto & entry : map)
            random_keys.push_back(entry.first);

        std::shuffle(random_keys.begin(), random_keys.end(), global_randomness);
        std::vector<T> maps = copy_tables_to_overflow_cache(map);
        size_t map_index = 0;
        size_t num_maps = maps.size();
        while (state.KeepRunning())
        {
            for (const auto & i : random_keys)
            {
                T & current_map = maps[map_index++];
                skb::DoNotOptimize(current_map.find(i)->second);
                if (map_index == num_maps)
                    map_index = 0;
            }
        }
        state.SetItemsProcessed(state.iterations() * random_keys.size());
    }
};

template<typename T>
struct benchmark_successful_lookup_permutation : hashtable_benchmark_base
{
    void operator()(skb::State & state) const
    {
        T map = TableCreator<T>()(max_load_factor);
        int num_items = state.range(0);

        using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
        insert_into_map(map, create_random_values<typename T::value_type, distribution_type>(global_randomness, num_items), state);
        std::vector<typename const_cast_pair<typename T::value_type>::type> all_values(map.begin(), map.end());
        std::shuffle(all_values.begin(), all_values.end(), global_randomness);
        for (size_t i = 0, end = all_values.size(); i < end; ++i)
        {
            size_t other_index = i == 0 ? all_values.size() - 1 : i - 1;
            typename T::iterator found = map.find(all_values[i].first);
            CHECK_FOR_PROGRAMMER_ERROR(found != map.end());
            found->second = all_values[other_index].first;
        }

        while (state.KeepRunning())
        {
            typename T::key_type to_find = all_values.front().first;
            for (int i = 0; i < num_items; ++i)
            {
                auto found = map.find(to_find);
                skb::DoNotOptimize(found->first);
                to_find = found->second;
            }
        }
        state.SetItemsProcessed(state.iterations() * num_items);
    }
};

template<typename T>
struct benchmark_successful_lookup_predictable_best_worst_n : hashtable_benchmark_base
{
    benchmark_successful_lookup_predictable_best_worst_n(float percent, bool best)
        : percent(percent)
        , best(best)
    {
    }

    float percent;
    bool best;
    void operator()(skb::State & state) const
    {
        T map = TableCreator<T>()(max_load_factor);

        using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
        insert_into_map(map, create_random_values<typename T::value_type, distribution_type>(global_randomness, state.range(0)), state);
        std::vector<std::pair<typename T::key_type, int>> keys_with_distance;
        keys_with_distance.reserve(map.size());
        for (const auto & element : map)
            keys_with_distance.emplace_back(element.first, map.num_lookups(element.first));
        auto cutoff = keys_with_distance.begin() + static_cast<std::ptrdiff_t>(percent * keys_with_distance.size());
        if (best)
        {
            std::nth_element(keys_with_distance.begin(), cutoff, keys_with_distance.end(), [](const auto & l, const auto & r)
            {
                return l.second < r.second;
            });
        }
        else
        {
            std::nth_element(keys_with_distance.begin(), cutoff, keys_with_distance.end(), [](const auto & l, const auto & r)
            {
                return l.second > r.second;
            });
        }
        if (cutoff != keys_with_distance.end())
            keys_with_distance.erase(cutoff + 1, keys_with_distance.end());

        std::shuffle(keys_with_distance.begin(), keys_with_distance.end(), global_randomness);
        while (state.KeepRunning())
        {
            for (const auto & i : keys_with_distance)
                skb::DoNotOptimize(map.find(i.first)->second);
        }
        state.SetItemsProcessed(state.iterations() * keys_with_distance.size());
    }
};

struct MultipleOf16Distribution
{
    MultipleOf16Distribution()
        : distribution(0, std::numeric_limits<int>::max() / 16)
    {
    }

    template<typename R>
    int operator()(R & randomness)
    {
        return 16 * distribution(randomness);
    }

    std::uniform_int_distribution<int> distribution;
};

struct MostlySequentialDistribution
{
    MostlySequentialDistribution()
    {
    }

    template<typename R>
    int operator()(R & randomness)
    {
        if (random_choice(randomness) == 0)
            return distribution(randomness);
        else
            return sequential++;
    }

    int sequential = 0;
    std::uniform_int_distribution<int> distribution;
    std::uniform_int_distribution<int> random_choice{0, 99};
};

struct ManySequentialDistribution
{
    template<typename R>
    unsigned operator()(R & /*randomness*/)
    {
        if (is_incrementing)
        {
            ++*next_to_increment;
            if (*next_to_increment == limit)
            {
                if (next_to_increment == &x)
                {
                    next_to_increment = &y;
                }
                else
                {
                    is_incrementing = false;
                    next_to_increment = &x;
                }
            }
        }
        else
        {
            --*next_to_increment;
            if (*next_to_increment == -limit)
            {
                if (next_to_increment == &x)
                {
                    next_to_increment = &y;
                }
                else
                {
                    is_incrementing = true;
                    next_to_increment = &x;
                    ++limit;
                }
            }
        }
        return static_cast<unsigned>(static_cast<unsigned short>(y)) << 16 | static_cast<unsigned>(static_cast<unsigned short>(x));
    }

    short x = 0;
    short y = 0;
    short limit = 1;
    bool is_incrementing = true;
    short * next_to_increment = &x;
};

template<typename T>
struct benchmark_unsuccessful_lookup : hashtable_benchmark_base
{
    void operator()(skb::State & state) const
    {
        T map = TableCreator<T>()(max_load_factor);

        using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
        insert_into_map(map, create_random_values<typename T::value_type, distribution_type>(global_randomness, state.range(0)), state);

        distribution_type random_index;
        while (state.KeepRunning())
        {
            for (size_t i = 0; i < num_lookup_loops; ++i)
                skb::DoNotOptimize(map.find(no_inline_random_number(random_index, global_randomness)) == map.end());
        }
        state.SetItemsProcessed(state.iterations() * num_lookup_loops);
    }
};

template<typename T>
struct benchmark_unsuccessful_lookup_cache_miss : hashtable_benchmark_base
{
    void operator()(skb::State & state) const
    {
        T map = TableCreator<T>()(max_load_factor);

        using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
        insert_into_map(map, create_random_values<typename T::value_type, distribution_type>(global_randomness, state.range(0)), state);

        std::vector<T> maps = copy_tables_to_overflow_cache(map);
        distribution_type random_index;
        std::uniform_int_distribution<size_t> random_map(0, maps.size() - 1);
        while (state.KeepRunning())
        {
            for (size_t i = 0; i < num_lookup_loops; ++i)
            {
                T & current_map = maps[no_inline_random_number(random_map, global_randomness)];
                skb::DoNotOptimize(current_map.find(no_inline_random_number(random_index, global_randomness)) == current_map.end());
            }
        }
        state.SetItemsProcessed(state.iterations() * num_lookup_loops);
    }
};

template<typename T>
struct benchmark_unsuccessful_lookup_predictable : hashtable_benchmark_base
{
    using generator_type = std::function<std::vector<typename const_cast_pair<typename T::value_type>::type> (profiling_randomness &, int)>;
    benchmark_unsuccessful_lookup_predictable(generator_type generator)
        : generator(std::move(generator))
    {
    }

    generator_type generator;

    void operator()(skb::State & state) const
    {
        T map = TableCreator<T>()(max_load_factor);

        insert_into_map(map, generator(global_randomness, state.range(0)), state);

        using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
        distribution_type random_index;
        std::vector<typename T::key_type> random_keys(state.range(0));
        for (auto & random : random_keys)
            random = random_index(global_randomness);

        while (state.KeepRunning())
        {
            for (const auto & i : random_keys)
                skb::DoNotOptimize(map.find(i) == map.end());
        }
        state.SetItemsProcessed(state.iterations() * random_keys.size());
    }
};

template<typename T>
struct benchmark_unsuccessful_lookup_predictable_cache_miss : hashtable_benchmark_base
{
    void operator()(skb::State & state) const
    {
        T map = TableCreator<T>()(max_load_factor);

        using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
        insert_into_map(map, create_random_values<typename T::value_type, distribution_type>(global_randomness, state.range(0)), state);

        using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
        distribution_type random_index;
        std::vector<typename T::key_type> random_keys(state.range(0));
        for (auto & random : random_keys)
            random = random_index(global_randomness);
        std::vector<T> maps = copy_tables_to_overflow_cache(map);
        size_t map_index = 0;
        size_t num_maps = maps.size();
        while (state.KeepRunning())
        {
            for (const auto & i : random_keys)
            {
                T & current_map = maps[map_index++];
                skb::DoNotOptimize(current_map.find(i) == current_map.end());
                if (map_index == num_maps)
                    map_index = 0;
            }
        }
        state.SetItemsProcessed(state.iterations() * random_keys.size());
    }
};

struct std_hash
{
    template<typename T>
    size_t operator()(const T & v) const
    {
        return std::hash<T>()(v);
    }
};

struct power_of_two_hash
{
    template<typename T>
    size_t operator()(const T & v) const
    {
        return std::hash<T>()(v);
    }
    using hash_policy = ska::power_of_two_hash_policy;
};

struct other_bits_power_of_two_hash
{
    template<typename T>
    size_t operator()(const T & t) const
    {
        return std::hash<T>()(t);
    }

    using hash_policy = ska::power_of_two_hash_policy_other_bits;
};


struct libdivide_hash
{
    template<typename T>
    size_t operator()(const T & v) const
    {
        return std::hash<T>()(v);
    }
    using hash_policy = ska::libdivide_prime_hash_policy;
};

struct switch_prime_number_hash
{
    template<typename T>
    size_t operator()(const T & v) const
    {
        return std::hash<T>()(v);
    }
    using hash_policy = ska::switch_prime_number_hash_policy;
};

template<typename T>
struct UpperLimitOverride
{
    static constexpr int value = std::numeric_limits<int>::max();
};

template<typename T>
struct BenchmarkMemorySize
{
    static constexpr size_t value = sizeof(T);
};
template<typename T>
struct BenchmarkMemorySize<const T>
        : BenchmarkMemorySize<T>
{
};
template<>
struct BenchmarkMemorySize<std::string>
{
    static constexpr size_t value = sizeof(std::string) + 40;
};
template<typename A, typename B>
struct BenchmarkMemorySize<std::pair<A, B>>
{
    static constexpr size_t value = BenchmarkMemorySize<A>::value + BenchmarkMemorySize<B>::value;
};

template<typename T>
skb::Benchmark * LookupRange(skb::Benchmark * benchmark)
{
    int upper_limit = 16 * 1024 * 1024;
    upper_limit = std::min(upper_limit, std::numeric_limits<int>::max() / int(BenchmarkMemorySize<typename T::value_type>::value));

    if (UpperLimitOverride<T>::value < upper_limit)
        upper_limit = UpperLimitOverride<T>::value;
    return benchmark->SetRange(4, upper_limit)->SetRangeMultiplier(1.1);
}

template<typename T>
skb::Benchmark * LookupReducedRange(skb::Benchmark * benchmark)
{
    int upper_limit = 256 * 1024;
    if (UpperLimitOverride<T>::value < upper_limit)
        upper_limit = UpperLimitOverride<T>::value;
    return benchmark->SetRange(4, upper_limit)->SetRangeMultiplier(1.1);
}

skb::CategoryBuilder add_max_load_factor_category(skb::CategoryBuilder builder, float max_load_factor);

template<typename Container>
struct IsHashMap
{
    static constexpr bool value = true;
};
template<typename Container>
struct HasMutableMapped
{
    static constexpr bool value = true;
};
template<typename Container>
struct HasNumLookups
{
    static constexpr bool value = false;
};

template<typename Container>
struct MaxSupportedLoadFactor
{
    static constexpr float value = 1.0f;
};

inline interned_string KeyCategoryString(const skb::CategoryBuilder & categories)
{
    static const interned_string key_str("key");
    auto found = categories.categories.find(key_str);
    CHECK_FOR_PROGRAMMER_ERROR(found != categories.categories.end());
    return found->second;
}

template<typename T>
void RegisterLookup(const interned_string & name, skb::CategoryBuilder categories)
{
    skb::CategoryBuilder successful = categories.AddCategory("hashtable type", "lookup").AddCategory("lookup type", "successful");
    skb::CategoryBuilder unsuccessful = categories.AddCategory("hashtable type", "lookup").AddCategory("lookup type", "unsuccessful");
    using distribution_type = BenchmarkRandomDistribution<typename T::key_type>;
    interned_string key_str = KeyCategoryString(categories);
    for (float max_load_factor : { 0.0f, 0.5f, 0.875f, 0.9375f, 1.0f })
    {
        if (max_load_factor > MaxSupportedLoadFactor<T>::value)
            continue;
        benchmark_successful_lookup<T> successful_lookup;
        successful_lookup.max_load_factor = max_load_factor;
        LookupRange<T>(SKA_BENCHMARK_CATEGORIES(successful_lookup, add_max_load_factor_category(successful.AddCategory("argument type", "random"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_successful_lookup_baseline_" + key_str));
        benchmark_successful_lookup_cache_miss<T> successful_lookup_cache_miss;
        successful_lookup_cache_miss.max_load_factor = max_load_factor;
        LookupReducedRange<T>(SKA_BENCHMARK_CATEGORIES(successful_lookup_cache_miss, add_max_load_factor_category(successful.AddCategory("cache miss", "cache miss").AddCategory("argument type", "random"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_successful_lookup_baseline_cache_miss_" + key_str));
        benchmark_successful_lookup_predictable<T> successful_predictable(&create_random_values<typename T::value_type, distribution_type>);
        successful_predictable.max_load_factor = max_load_factor;
        LookupRange<T>(SKA_BENCHMARK_CATEGORIES(successful_predictable, add_max_load_factor_category(successful.AddCategory("argument type", "predictable random"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_successful_lookup_predictable_baseline_" + key_str));
        benchmark_unsuccessful_lookup<T> unsuccessful_lookup;
        unsuccessful_lookup.max_load_factor = max_load_factor;
        LookupRange<T>(SKA_BENCHMARK_CATEGORIES(unsuccessful_lookup, add_max_load_factor_category(unsuccessful.AddCategory("argument type", "random"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_unsuccessful_lookup_baseline_" + key_str));
        benchmark_unsuccessful_lookup_cache_miss<T> unsuccessful_lookup_cache_miss;
        unsuccessful_lookup_cache_miss.max_load_factor = max_load_factor;
        LookupReducedRange<T>(SKA_BENCHMARK_CATEGORIES(unsuccessful_lookup_cache_miss, add_max_load_factor_category(unsuccessful.AddCategory("cache miss", "cache miss").AddCategory("argument type", "random"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_unsuccessful_lookup_baseline_cache_miss_" + key_str));
        benchmark_unsuccessful_lookup_predictable<T> unsuccessful_predictable(&create_random_values<typename T::value_type, distribution_type>);
        unsuccessful_predictable.max_load_factor = max_load_factor;
        LookupRange<T>(SKA_BENCHMARK_CATEGORIES(unsuccessful_predictable, add_max_load_factor_category(unsuccessful.AddCategory("argument type", "predictable random"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_unsuccessful_lookup_predictable_baseline_" + key_str));
        benchmark_successful_lookup_predictable_cache_miss<T> successful_predictable_cache_miss;
        successful_predictable_cache_miss.max_load_factor = max_load_factor;
        LookupReducedRange<T>(SKA_BENCHMARK_CATEGORIES(successful_predictable_cache_miss, add_max_load_factor_category(successful.AddCategory("cache miss", "cache miss").AddCategory("argument type", "predictable random"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_successful_lookup_predictable_baseline_cache_miss_" + key_str));
        benchmark_unsuccessful_lookup_predictable_cache_miss<T> unsuccessful_predictable_cache_miss;
        unsuccessful_predictable_cache_miss.max_load_factor = max_load_factor;
        LookupReducedRange<T>(SKA_BENCHMARK_CATEGORIES(unsuccessful_predictable_cache_miss, add_max_load_factor_category(unsuccessful.AddCategory("cache miss", "cache miss").AddCategory("argument type", "predictable random"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_unsuccessful_lookup_predictable_baseline_cache_miss_" + key_str));
        if constexpr (HasMutableMapped<T>::value && std::is_convertible<typename T::key_type, typename T::value_type::second_type>::value)
        {
            benchmark_successful_lookup_permutation<T> successful_lookup_permutation;
            successful_lookup_permutation.max_load_factor = max_load_factor;
            LookupRange<T>(SKA_BENCHMARK_CATEGORIES(successful_lookup_permutation, add_max_load_factor_category(successful.AddCategory("argument type", "permutation"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_successful_lookup_permutation_baseline_" + key_str));
        }
        if constexpr (HasNumLookups<T>::value)
        {
            skb::CategoryBuilder num_lookups_categories = successful.AddCategory("argument type", "predictable random");
            for (std::pair<float, std::string> worst : std::vector<std::pair<float, std::string>>{ { 0.25f, "25" }, { 0.1f, "10" }, { 0.01f, "1" } })
            {
                benchmark_successful_lookup_predictable_best_worst_n<T> worst_n_lookup(worst.first, false);
                LookupRange<T>(SKA_BENCHMARK_CATEGORIES(worst_n_lookup, add_max_load_factor_category(num_lookups_categories.AddCategory("best or worst", interned_string("worst " + worst.second + "%")), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_successful_lookup_predictable_best_worst_baseline_" + worst.second + "_" + key_str));
            }
            for (std::pair<float, std::string> best : std::vector<std::pair<float, std::string>>{ { 0.25f, "25" }, { 0.1f, "10" } })
            {
                benchmark_successful_lookup_predictable_best_worst_n<T> best_n_lookup(best.first, true);
                LookupRange<T>(SKA_BENCHMARK_CATEGORIES(best_n_lookup, add_max_load_factor_category(num_lookups_categories.AddCategory("best or worst", interned_string("best " + best.second + "%")), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_successful_lookup_predictable_best_worst_baseline_" + best.second + "_" + key_str));
            }
        }

        if constexpr (IsHashMap<T>::value && sizeof(typename T::value_type) < 512 && std::is_same<typename T::key_type, int>::value)
        {
            benchmark_successful_lookup_predictable<T> successful_predictable_16(&create_random_values<typename T::value_type, MultipleOf16Distribution>);
            successful_predictable_16.max_load_factor = max_load_factor;
            LookupReducedRange<T>(SKA_BENCHMARK_CATEGORIES(successful_predictable_16, add_max_load_factor_category(successful.AddCategory("argument type", "predictable random"), max_load_factor).AddCategory("collisions", "multiple of 16").BuildCategories("hashtable", name))->SetBaseline("benchmark_successful_lookup_predictable_baseline_" + key_str));
            benchmark_unsuccessful_lookup_predictable<T> unsuccessful_predictable_16(&create_random_values<typename T::value_type, MultipleOf16Distribution>);
            unsuccessful_predictable_16.max_load_factor = max_load_factor;
            LookupReducedRange<T>(SKA_BENCHMARK_CATEGORIES(unsuccessful_predictable_16, add_max_load_factor_category(unsuccessful.AddCategory("argument type", "predictable random"), max_load_factor).AddCategory("collisions", "multiple of 16").BuildCategories("hashtable", name))->SetBaseline("benchmark_unsuccessful_lookup_predictable_baseline_" + key_str));
            benchmark_successful_lookup_predictable<T> successful_predictable_sequential(&create_random_values<typename T::value_type, ManySequentialDistribution>);
            successful_predictable_sequential.max_load_factor = max_load_factor;
            LookupReducedRange<T>(SKA_BENCHMARK_CATEGORIES(successful_predictable_sequential, add_max_load_factor_category(successful.AddCategory("argument type", "predictable random"), max_load_factor).AddCategory("collisions", "many sequential").BuildCategories("hashtable", name))->SetBaseline("benchmark_successful_lookup_predictable_baseline_" + key_str));
            benchmark_unsuccessful_lookup_predictable<T> unsuccessful_predictable_sequential(&create_random_values<typename T::value_type, ManySequentialDistribution>);
            unsuccessful_predictable_sequential.max_load_factor = max_load_factor;
            LookupReducedRange<T>(SKA_BENCHMARK_CATEGORIES(unsuccessful_predictable_sequential, add_max_load_factor_category(unsuccessful.AddCategory("argument type", "predictable random"), max_load_factor).AddCategory("collisions", "many sequential").BuildCategories("hashtable", name))->SetBaseline("benchmark_unsuccessful_lookup_predictable_baseline_" + key_str));
            benchmark_successful_lookup_predictable<T> successful_predictable_mostly_sequential(&create_random_values<typename T::value_type, MostlySequentialDistribution>);
            successful_predictable_mostly_sequential.max_load_factor = max_load_factor;
            LookupReducedRange<T>(SKA_BENCHMARK_CATEGORIES(successful_predictable_mostly_sequential, add_max_load_factor_category(successful.AddCategory("argument type", "predictable random"), max_load_factor).AddCategory("collisions", "mostly sequential").BuildCategories("hashtable", name))->SetBaseline("benchmark_successful_lookup_predictable_baseline_" + key_str));
            benchmark_unsuccessful_lookup_predictable<T> unsuccessful_predictable_mostly_sequential(&create_random_values<typename T::value_type, MostlySequentialDistribution>);
            unsuccessful_predictable_mostly_sequential.max_load_factor = max_load_factor;
            LookupReducedRange<T>(SKA_BENCHMARK_CATEGORIES(unsuccessful_predictable_mostly_sequential, add_max_load_factor_category(unsuccessful.AddCategory("argument type", "predictable random"), max_load_factor).AddCategory("collisions", "mostly sequential").BuildCategories("hashtable", name))->SetBaseline("benchmark_unsuccessful_lookup_predictable_baseline_" + key_str));
        }

        benchmark_insert<T> insert;
        insert.max_load_factor = max_load_factor;
        LookupRange<T>(SKA_BENCHMARK_CATEGORIES(insert, add_max_load_factor_category(categories.AddCategory("hashtable type", "insert").AddCategory("reserve", "no reserve"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_insert_baseline_" + key_str));
        benchmark_insert<T> reserve_insert;
        reserve_insert.reserve = true;
        reserve_insert.max_load_factor = max_load_factor;
        LookupRange<T>(SKA_BENCHMARK_CATEGORIES(reserve_insert, add_max_load_factor_category(categories.AddCategory("hashtable type", "insert").AddCategory("reserve", "reserve"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_insert_baseline_" + key_str));
        benchmark_erase_whole_table<T> erase;
        erase.max_load_factor = max_load_factor;
        LookupRange<T>(SKA_BENCHMARK_CATEGORIES(erase, add_max_load_factor_category(categories.AddCategory("hashtable type", "erase"), max_load_factor).BuildCategories("hashtable", name))->SetBaseline("benchmark_erase_whole_table_baseline_" + key_str));
    }
}

template<size_t N>
struct NValueBytes
{
    int bytes[N / sizeof(int)];
    NValueBytes(int i = 0)
    {
        std::fill(std::begin(bytes), std::end(bytes), i);
    }
    operator int() const
    {
        return bytes[0];
    }
};

template<typename T, typename ToReplace, typename Replacement>
struct ReplaceTemplateArgument
{
    using type = T;
};
template<typename ToReplace, typename Replacement>
struct ReplaceTemplateArgument<ToReplace, ToReplace, Replacement>
{
    using type = Replacement;
};
template<template <typename, auto...> typename Template, typename ToReplace, typename Replacement, typename A, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<A, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<typename ReplaceTemplateArgument<A, ToReplace, Replacement>::type, OtherArgs...>;
};
template<template <typename, auto...> typename Template, typename ToReplace, typename Replacement, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<ToReplace, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<Replacement, OtherArgs...>;
};
template<template <typename, auto...> typename Template, typename ToReplace, typename Replacement, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<const ToReplace, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<const Replacement, OtherArgs...>;
};
template<template <typename, typename, auto...> typename Template, typename ToReplace, typename Replacement, typename A, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<A, ToReplace, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<typename ReplaceTemplateArgument<A, ToReplace, Replacement>::type, Replacement, OtherArgs...>;
};
template<template <typename, typename, auto...> typename Template, typename ToReplace, typename Replacement, typename A, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<A, const ToReplace, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<typename ReplaceTemplateArgument<A, ToReplace, Replacement>::type, const Replacement, OtherArgs...>;
};
template<template <typename, typename, auto...> typename Template, typename ToReplace, typename Replacement, typename B, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<ToReplace, B, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<Replacement, typename ReplaceTemplateArgument<B, ToReplace, Replacement>::type, OtherArgs...>;
};
template<template <typename, typename, auto...> typename Template, typename ToReplace, typename Replacement, typename B, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<const ToReplace, B, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<const Replacement, typename ReplaceTemplateArgument<B, ToReplace, Replacement>::type, OtherArgs...>;
};
template<template <typename, typename, typename, auto...> typename Template, typename ToReplace, typename Replacement, typename A, typename C, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<A, ToReplace, C, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<typename ReplaceTemplateArgument<A, ToReplace, Replacement>::type, Replacement, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, OtherArgs...>;
};
template<template <typename, typename, typename, auto...> typename Template, typename ToReplace, typename Replacement, typename B, typename C, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<ToReplace, B, C, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<Replacement, typename ReplaceTemplateArgument<B, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, OtherArgs...>;
};
template<template <typename, typename, typename, typename, auto...> typename Template, typename ToReplace, typename Replacement, typename A, typename C, typename D, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<A, ToReplace, C, D, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<typename ReplaceTemplateArgument<A, ToReplace, Replacement>::type, Replacement, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<D, ToReplace, Replacement>::type, OtherArgs...>;
};
template<template <typename, typename, typename, typename, auto...> typename Template, typename ToReplace, typename Replacement, typename B, typename C, typename D, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<ToReplace, B, C, D, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<Replacement, typename ReplaceTemplateArgument<B, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<D, ToReplace, Replacement>::type, OtherArgs...>;
};
template<template <typename, typename, typename, typename, typename, auto...> typename Template, typename ToReplace, typename Replacement, typename A, typename C, typename D, typename E, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<A, ToReplace, C, D, E, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<typename ReplaceTemplateArgument<A, ToReplace, Replacement>::type, Replacement, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<D, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<E, ToReplace, Replacement>::type, OtherArgs...>;
};
template<template <typename, typename, typename, typename, typename, auto...> typename Template, typename ToReplace, typename Replacement, typename B, typename C, typename D, typename E, auto... OtherArgs>
struct ReplaceTemplateArgument<Template<ToReplace, B, C, D, E, OtherArgs...>, ToReplace, Replacement>
{
    using type = Template<Replacement, typename ReplaceTemplateArgument<B, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<D, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<E, ToReplace, Replacement>::type, OtherArgs...>;
};
template<template <typename, typename, typename, typename, typename, auto, auto, typename> typename Template, typename ToReplace, typename Replacement, typename B, typename C, typename D, typename E, auto F, auto G, typename H>
struct ReplaceTemplateArgument<Template<ToReplace, B, C, D, E, F, G, H>, ToReplace, Replacement>
{
    using type = Template<Replacement, typename ReplaceTemplateArgument<B, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<D, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<E, ToReplace, Replacement>::type, F, G, typename ReplaceTemplateArgument<H, ToReplace, Replacement>::type>;
};
template<template <typename, typename, typename, typename, typename, auto, auto, typename> typename Template, typename ToReplace, typename Replacement, typename A, typename C, typename D, typename E, auto F, auto G, typename H>
struct ReplaceTemplateArgument<Template<A, ToReplace, C, D, E, F, G, H>, ToReplace, Replacement>
{
    using type = Template<typename ReplaceTemplateArgument<A, ToReplace, Replacement>::type, Replacement, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<D, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<E, ToReplace, Replacement>::type, F, G, typename ReplaceTemplateArgument<H, ToReplace, Replacement>::type>;
};
template<template <typename, typename, typename, typename, typename, auto, typename> typename Template, typename ToReplace, typename Replacement, typename B, typename C, typename D, typename E, auto F, typename G>
struct ReplaceTemplateArgument<Template<ToReplace, B, C, D, E, F, G>, ToReplace, Replacement>
{
    using type = Template<Replacement, typename ReplaceTemplateArgument<B, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<D, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<E, ToReplace, Replacement>::type, F, typename ReplaceTemplateArgument<G, ToReplace, Replacement>::type>;
};
template<template <typename, typename, typename, typename, typename, auto, typename> typename Template, typename ToReplace, typename Replacement, typename A, typename C, typename D, typename E, auto F, typename G>
struct ReplaceTemplateArgument<Template<A, ToReplace, C, D, E, F, G>, ToReplace, Replacement>
{
    using type = Template<typename ReplaceTemplateArgument<A, ToReplace, Replacement>::type, Replacement, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<D, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<E, ToReplace, Replacement>::type, F, typename ReplaceTemplateArgument<G, ToReplace, Replacement>::type>;
};
template<template <typename, typename, typename, typename, typename, typename, auto, auto, typename> typename Template, typename ToReplace, typename Replacement, typename B, typename C, typename D, typename E, typename F, auto G, auto H, typename I>
struct ReplaceTemplateArgument<Template<ToReplace, B, C, D, E, F, G, H, I>, ToReplace, Replacement>
{
    using type = Template<Replacement, typename ReplaceTemplateArgument<B, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<D, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<E, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<F, ToReplace, Replacement>::type, G, H, typename ReplaceTemplateArgument<I, ToReplace, Replacement>::type>;
};
template<template <typename, typename, typename, typename, typename, typename, auto, auto, typename> typename Template, typename ToReplace, typename Replacement, typename A, typename C, typename D, typename E, typename F, auto G, auto H, typename I>
struct ReplaceTemplateArgument<Template<A, ToReplace, C, D, E, F, G, H, I>, ToReplace, Replacement>
{
    using type = Template<typename ReplaceTemplateArgument<A, ToReplace, Replacement>::type, Replacement, typename ReplaceTemplateArgument<C, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<D, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<E, ToReplace, Replacement>::type, typename ReplaceTemplateArgument<F, ToReplace, Replacement>::type, G, H, typename ReplaceTemplateArgument<I, ToReplace, Replacement>::type>;
};

template<typename T>
struct RegisterValueCombinations
{
    void operator()(const interned_string & name, skb::CategoryBuilder categories_so_far)
    {
        using int_type = typename ReplaceTemplateArgument<T, ValuePlaceHolder, int>::type;
        RegisterLookup<int_type>(name, categories_so_far.AddCategory("value", "int"));
        using sixty_four_type = typename ReplaceTemplateArgument<T, ValuePlaceHolder, NValueBytes<64>>::type;
        RegisterLookup<sixty_four_type>(name, categories_so_far.AddCategory("value", "64 bytes"));
        using ten_twenty_four_four_type = typename ReplaceTemplateArgument<T, ValuePlaceHolder, NValueBytes<1024>>::type;
        RegisterLookup<ten_twenty_four_four_type>(name, categories_so_far.AddCategory("value", "1024 bytes"));
        using string_type = typename ReplaceTemplateArgument<T, ValuePlaceHolder, std::string>::type;
        RegisterLookup<string_type>(name, categories_so_far.AddCategory("value", "string"));
    }
};

template<typename T>
struct RegisterLookups
{
    void operator()(const interned_string & name, skb::CategoryBuilder categories_so_far)
    {
        using int_type = typename ReplaceTemplateArgument<T, KeyPlaceHolder, int>::type;
        RegisterValueCombinations<int_type>()(name, categories_so_far.AddCategory("key", "int"));
        using string_type = typename ReplaceTemplateArgument<T, KeyPlaceHolder, std::string>::type;
        RegisterValueCombinations<string_type>()(name, categories_so_far.AddCategory("key", "string"));
    }
};
