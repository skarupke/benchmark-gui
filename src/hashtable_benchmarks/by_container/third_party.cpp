#include "hashtable_benchmarks/lookups.hpp"

#include <google/dense_hash_map>
#include <map>
#include <boost/container/flat_map.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <unordered_map>
#include <boost/unordered_map.hpp>
// hash_map is apparently deprecated...
#ifdef __DEPRECATED
#undef __DEPRECATED
#endif
#include <hash_map>


template<typename K, typename V, typename H, typename E, typename A, typename... Args>
void hashtable_emplace(google::dense_hash_map<K, V, H, E, A> & table, Args &&... args)
{
    table.insert({std::forward<Args>(args)...});
}
template<typename K, typename V, typename H, typename E, typename A, typename... Args>
void hashtable_emplace(__gnu_cxx::hash_map<K, V, H, E, A> & table, Args &&... args)
{
    table.insert({std::forward<Args>(args)...});
}

template<typename K, typename V, typename H, typename E, typename A>
struct TableCreator<google::dense_hash_map<K, V, H, E, A>>
{
    google::dense_hash_map<K, V, H, E, A> operator()(float max_load_factor) const
    {
        google::dense_hash_map<K, V, H, E, A> result;
        result.set_empty_key(SpecialKeys<K>::empty_key());
        result.set_deleted_key(SpecialKeys<K>::deleted_key());
        if (max_load_factor > 0.0f)
            result.max_load_factor(max_load_factor);
        return result;
    }
    google::dense_hash_map<K, V, H, E, A> operator()(float max_load_factor, int num_items) const
    {
        google::dense_hash_map<K, V, H, E, A> result(num_items);
        result.set_empty_key(SpecialKeys<K>::empty_key());
        result.set_deleted_key(SpecialKeys<K>::deleted_key());
        if (max_load_factor > 0.0f)
            result.max_load_factor(max_load_factor);
        return result;
    }
};
template<typename K, typename V, typename L, typename A>
struct TableCreator<std::map<K, V, L, A>>
{
    std::map<K, V, L, A> operator()(float /*max_load_factor*/) const
    {
        return std::map<K, V, L, A>();
    }
    std::map<K, V, L, A> operator()(float /*max_load_factor*/, int /*num_items*/) const
    {
        return std::map<K, V, L, A>();
    }
};
template<typename K, typename V, typename L, typename A>
struct TableCreator<boost::container::flat_map<K, V, L, A>>
{
    boost::container::flat_map<K, V, L, A> operator()(float /*max_load_factor*/) const
    {
        return boost::container::flat_map<K, V, L, A>();
    }
    boost::container::flat_map<K, V, L, A> operator()(float /*max_load_factor*/, int /*num_items*/) const
    {
        return boost::container::flat_map<K, V, L, A>();
    }
};
template<typename K, typename V, typename L, typename H, typename A>
struct TableCreator<__gnu_cxx::hash_map<K, V, L, H, A>>
{
    __gnu_cxx::hash_map<K, V, L, H, A> operator()(float /*max_load_factor*/) const
    {
        return __gnu_cxx::hash_map<K, V, L, H, A>();
    }
    __gnu_cxx::hash_map<K, V, L, H, A> operator()(float /*max_load_factor*/, int num_items) const
    {
        return __gnu_cxx::hash_map<K, V, L, H, A>(num_items);
    }
};
template<typename K, typename V, typename L, typename A>
struct UpperLimitOverride<std::map<K, V, L, A>>
{
    static constexpr int value = 4 * 1024 * 1024;
};
template<typename K, typename V, typename L, typename A>
struct UpperLimitOverride<boost::container::flat_map<K, V, L, A>>
{
    static constexpr int value = 32 * 1024 * 1024;
};
template<typename K, typename V, typename L, typename A>
struct IsHashMap<std::map<K, V, L, A>>
{
    static constexpr bool value = false;
};
template<typename K, typename V, typename L, typename A>
struct IsHashMap<boost::container::flat_map<K, V, L, A>>
{
    static constexpr bool value = false;
};
template<typename K, typename V, typename L, typename A>
struct MaxSupportedLoadFactor<boost::container::flat_map<K, V, L, A>>
{
    static constexpr float value = 0.0f;
};
template<typename K, typename V, typename L, typename A>
struct MaxSupportedLoadFactor<std::map<K, V, L, A>>
{
    static constexpr float value = 0.0f;
};
template<typename K, typename V, typename H, typename E, typename A>
struct MaxSupportedLoadFactor<google::dense_hash_map<K, V, H, E, A>>
{
    static constexpr float value = 0.9375f;
};
template<typename K, typename V, typename H, typename E, typename A>
struct MaxSupportedLoadFactor<__gnu_cxx::hash_map<K, V, H, E, A>>
{
    static constexpr float value = 0.0f;
};
template<typename K, typename V, typename L, typename A, typename F, typename S>
void insert_into_map(boost::container::flat_map<K, V, L, A> & map, std::vector<std::pair<F, S>> values, skb::State & state)
{
    TrackAllocations track_allocations(state);
    std::sort(values.begin(), values.end());
    map.insert(std::make_move_iterator(values.begin()), std::make_move_iterator(values.end()));
}

template<typename K, typename V, typename A>
struct multi_index : boost::multi_index_container<std::pair<K, V>, boost::multi_index::indexed_by<boost::multi_index::hashed_unique<boost::multi_index::member<std::pair<K, V>, K, &std::pair<K, V>::first > > >, A>
{
    using boost::multi_index_container<std::pair<K, V>, boost::multi_index::indexed_by<boost::multi_index::hashed_unique<boost::multi_index::member<std::pair<K, V>, K, &std::pair<K, V>::first > > >, A>::multi_index_container;
};
template<typename K, typename V, typename A>
struct HasMutableMapped<multi_index<K, V, A>>
{
    static constexpr bool value = false;
};

void RegisterThirdPartyHashtables()
{
    CategoryBuilder categories_so_far;
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<std::map<KeyPlaceHolder, ValuePlaceHolder, std::less<>, Allocator>>()("map", categories_so_far);
    RegisterLookups<boost::container::flat_map<KeyPlaceHolder, ValuePlaceHolder, std::less<>, Allocator>>()("boost::flat_map", categories_so_far);
    RegisterLookups<multi_index<KeyPlaceHolder, ValuePlaceHolder, Allocator>>()("boost::multi_index", categories_so_far);
    RegisterLookups<google::dense_hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("dense_hash_map", categories_so_far);
    RegisterLookups<__gnu_cxx::hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("gnu_cxx::hash_map", categories_so_far);
    CategoryBuilder std_unordered_map = categories_so_far.AddCategory("unordered_map variant", "std::unordered_map");
    CategoryBuilder boost_unordered_map = categories_so_far.AddCategory("unordered_map variant", "boost::unordered_map");
    RegisterLookups<std::unordered_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("unordered_map", std_unordered_map);
    RegisterLookups<boost::unordered_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("unordered_map", boost_unordered_map);
}

