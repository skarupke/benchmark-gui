#include "hashtable_benchmarks/lookups.hpp"
#include "container/flat_hash_map.hpp"

template<typename K, typename V, typename H, typename E, typename A, auto S>
struct HasNumLookups<ska::flat_hash_map<K, V, H, E, A, S>>
{
    static constexpr bool value = true;
};


void RegisterFlatHashMap()
{
    skb::CategoryBuilder categories_so_far;
    skb::CategoryBuilder prime = categories_so_far.AddCategory("bucket count", "prime number");
    //skb::CategoryBuilder libdivide = categories_so_far.AddCategory("bucket count", "prime number libdivide");
    skb::CategoryBuilder power_of_two = categories_so_far.AddCategory("bucket count", "power of two");
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("flat_hash_map", prime);
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator>>()("flat_hash_map", power_of_two);
    //RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv3::ExtraZeroCheck>>()("flat_hash_map", power_of_two.AddCategory("flat_hash_map variation", "zero check"));
}
