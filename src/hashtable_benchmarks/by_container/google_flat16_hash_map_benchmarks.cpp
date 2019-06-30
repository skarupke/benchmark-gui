#include "hashtable_benchmarks/lookups.hpp"
#include "container/google_flat16_hash_map.hpp"

template<typename K, typename V, typename H, typename E, typename A, auto S>
struct HasNumLookups<ska::google_flat16_hash_map<K, V, H, E, A, S>>
{
    static constexpr bool value = true;
};


void RegisterGoogleFlat16HashMap()
{
    skb::CategoryBuilder categories_so_far;
    skb::CategoryBuilder prime = categories_so_far.AddCategory("bucket count", "prime number");
    skb::CategoryBuilder power_of_two = categories_so_far.AddCategory("bucket count", "power of two");
    skb::CategoryBuilder power_of_two_other = categories_so_far.AddCategory("bucket count", "other two");
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<ska::google_flat16_hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("google_flat16_hash_map", prime);
    RegisterLookups<ska::google_flat16_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator>>()("google_flat16_hash_map", power_of_two);
    //RegisterLookups<ska::google_flat16_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv12::Prefetch>>()("google_flat16_hash_map", power_of_two.AddCategory("flat_hash_map variation", "prefetch"));
    RegisterLookups<ska::google_flat16_hash_map<KeyPlaceHolder, ValuePlaceHolder, other_bits_power_of_two_hash, std::equal_to<>, Allocator>>()("google_flat16_hash_map", power_of_two_other);
}
