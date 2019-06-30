#include "hashtable_benchmarks/lookups.hpp"
#include "container/block_hash_map.hpp"

void RegisterBlockHashMap()
{
    skb::CategoryBuilder categories_so_far;
    skb::CategoryBuilder prime = categories_so_far.AddCategory("bucket count", "prime number");
    skb::CategoryBuilder power_of_two = categories_so_far.AddCategory("bucket count", "power of two");
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<ska::block_hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("block_hash_map", prime);
    RegisterLookups<ska::block_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv6::Default>>()("block_hash_map", power_of_two.AddCategory("block_hash_map variant", "default"));
    RegisterLookups<ska::block_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv6::ConstantArray>>()("block_hash_map", power_of_two.AddCategory("block_hash_map variant", "constant array"));
    RegisterLookups<ska::block_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv6::ZeroStart>>()("block_hash_map", power_of_two.AddCategory("block_hash_map variant", "zero start"));
}
