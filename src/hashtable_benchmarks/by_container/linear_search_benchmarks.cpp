#include "hashtable_benchmarks/lookups.hpp"
#include "container/linear_search_map.hpp"
#include "container/inplace_linear_search_map.hpp"

template<typename K, typename V, typename A>
struct UpperLimitOverride<ska::linear_search_map<K, V, A>>
{
    static constexpr int value = 4096;
};
template<typename K, typename V, typename A>
struct UpperLimitOverride<ska::inplace_linear_search_map<K, V, A>>
{
    static constexpr int value = 4096;
};

template<typename K, typename V, typename A>
struct TableCreator<ska::inplace_linear_search_map<K, V, A>>
{
    ska::inplace_linear_search_map<K, V, A> operator()(float /*max_load_factor*/) const
    {
        return ska::inplace_linear_search_map<K, V, A>();
    }
    ska::inplace_linear_search_map<K, V, A> operator()(float /*max_load_factor*/, int num_items) const
    {
        ska::inplace_linear_search_map<K, V, A> result;
        result.reserve(num_items);
        return result;
    }
};
template<typename K, typename V, typename A>
struct TableCreator<ska::linear_search_map<K, V, A>>
{
    ska::linear_search_map<K, V, A> operator()(float /*max_load_factor*/) const
    {
        return ska::linear_search_map<K, V, A>();
    }
    ska::linear_search_map<K, V, A> operator()(float /*max_load_factor*/, int num_items) const
    {
        ska::linear_search_map<K, V, A> result;
        result.reserve(num_items);
        return result;
    }
};
template<typename K, typename V, typename A>
struct IsHashMap<ska::linear_search_map<K, V, A>>
{
    static constexpr bool value = false;
};
template<typename K, typename V, typename A>
struct IsHashMap<ska::inplace_linear_search_map<K, V, A>>
{
    static constexpr bool value = false;
};
template<typename K, typename V, typename A>
struct MaxSupportedLoadFactor<ska::linear_search_map<K, V, A>>
{
    static constexpr float value = 0.0f;
};
template<typename K, typename V, typename A>
struct MaxSupportedLoadFactor<ska::inplace_linear_search_map<K, V, A>>
{
    static constexpr float value = 0.0f;
};

void RegisterLinearSearch()
{
    skb::CategoryBuilder categories_so_far;
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<ska::linear_search_map<KeyPlaceHolder, ValuePlaceHolder, Allocator>>()("linear search", categories_so_far);
    RegisterLookups<ska::inplace_linear_search_map<KeyPlaceHolder, ValuePlaceHolder, Allocator>>()("smallvector linear search", categories_so_far);
}


