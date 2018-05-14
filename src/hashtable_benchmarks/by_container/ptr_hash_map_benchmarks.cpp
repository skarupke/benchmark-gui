#include "hashtable_benchmarks/lookups.hpp"
#include "container/ptr_hash_map.hpp"

void RegisterPtrHashMap()
{
    CategoryBuilder categories_so_far;
    CategoryBuilder prime = categories_so_far.AddCategory("bucket count", "prime number");
    CategoryBuilder power_of_two = categories_so_far.AddCategory("bucket count", "power of two");
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<ska::ptr_hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("ptr_hash_map", prime);
    RegisterLookups<ska::ptr_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator>>()("ptr_hash_map", power_of_two);
}
