#include "hashtable_benchmarks/lookups.hpp"
#include "container/flat_hash_map16.hpp"

void RegisterFlat16HashMap()
{
    skb::CategoryBuilder categories_so_far;
    skb::CategoryBuilder prime = categories_so_far.AddCategory("bucket count", "prime number");
    skb::CategoryBuilder power_of_two = categories_so_far.AddCategory("bucket count", "power of two");
    skb::CategoryBuilder other_power_of_two = categories_so_far.AddCategory("bucket count", "other two");
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<ska::flat16_hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("flat16_hash_map", prime);
    RegisterLookups<ska::flat16_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator>>()("flat16_hash_map", power_of_two);
    RegisterLookups<ska::flat16_hash_map<KeyPlaceHolder, ValuePlaceHolder, other_bits_power_of_two_hash, std::equal_to<>, Allocator>>()("flat16_hash_map", other_power_of_two);
    RegisterLookups<ska::flat16_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv5::ZeroCheck>>()("flat16_hash_map", power_of_two.AddCategory("flat_hash_map variation", "zero check"));
    RegisterLookups<ska::flat16_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv5::AlwaysTwoChecks>>()("flat16_hash_map", power_of_two.AddCategory("flat_hash_map variation", "two checks"));
}
