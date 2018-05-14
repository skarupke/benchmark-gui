#include "hashtable_benchmarks/lookups.hpp"
#include "container/unordered_map.hpp"

template<typename K, typename V, typename H, typename E, typename A>
struct HasNumLookups<ska::unordered_map<K, V, H, E, A>>
{
    static constexpr bool value = true;
};


void RegisterUnorderedMap()
{
    CategoryBuilder categories_so_far;
    CategoryBuilder prime = categories_so_far.AddCategory("bucket count", "prime number");
    CategoryBuilder libdivide = categories_so_far.AddCategory("bucket count", "prime number libdivide");
    CategoryBuilder switch_prime = categories_so_far.AddCategory("bucket count", "switch prime number");
    CategoryBuilder power_of_two = categories_so_far.AddCategory("bucket count", "power of two");
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    CategoryBuilder ska_unordered_map = prime.AddCategory("unordered_map variant", "ska::unordered_map");
    CategoryBuilder ska_unordered_map_power_of_two = power_of_two.AddCategory("unordered_map variant", "ska::unordered_map");
    CategoryBuilder ska_unordered_map_libdivide = libdivide.AddCategory("unordered_map variant", "ska::unordered_map");
    CategoryBuilder ska_unordered_map_switch_prime = switch_prime.AddCategory("unordered_map variant", "ska::unordered_map");
    RegisterLookups<ska::unordered_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("unordered_map", ska_unordered_map);
    RegisterLookups<ska::unordered_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator>>()("unordered_map", ska_unordered_map_power_of_two);
    RegisterLookups<ska::unordered_map<KeyPlaceHolder, ValuePlaceHolder, libdivide_hash, std::equal_to<>, Allocator>>()("unordered_map", ska_unordered_map_libdivide);
    RegisterLookups<ska::unordered_map<KeyPlaceHolder, ValuePlaceHolder, switch_prime_number_hash, std::equal_to<>, Allocator>>()("unordered_map", ska_unordered_map_switch_prime);
}
