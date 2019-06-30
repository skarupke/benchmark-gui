#include "hashtable_benchmarks/lookups.hpp"
#include "container/bytell_hash_map.hpp"

template<typename K, typename V, typename H, typename E, typename A, auto L, auto S>
struct HasNumLookups<ska::bytell_hash_map<K, V, H, E, A, L, S>>
{
    static constexpr bool value = true;
};

void RegisterBytellHashMap()
{
    skb::CategoryBuilder categories_so_far;
    skb::CategoryBuilder prime = categories_so_far.AddCategory("bucket count", "prime number");
    //skb::CategoryBuilder libdivide = categories_so_far.AddCategory("bucket count", "prime number libdivide");
    //skb::CategoryBuilder switch_prime = categories_so_far.AddCategory("bucket count", "switch prime number");
    skb::CategoryBuilder power_of_two = categories_so_far.AddCategory("bucket count", "power of two");
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<ska::bytell_hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("bytell_hash_map", prime);
    RegisterLookups<ska::bytell_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator>>()("bytell_hash_map", power_of_two);
    //RegisterLookups<ska::bytell_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv8::SomeOutside>>()("bytell_hash_map", power_of_two.AddCategory("bytell variant", "some outside"));
    //RegisterLookups<ska::bytell_hash_map<KeyPlaceHolder, ValuePlaceHolder, libdivide_hash, std::equal_to<>, Allocator>>()("bytell_hash_map", libdivide);
    //RegisterLookups<ska::bytell_hash_map<KeyPlaceHolder, ValuePlaceHolder, switch_prime_number_hash, std::equal_to<>, Allocator>>()("bytell_hash_map", switch_prime);
    RegisterLookups<ska::bytell_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv8::Default, 16>>()("bytell_hash_map", power_of_two.AddCategory("bytell block size", "16"));
    RegisterLookups<ska::bytell_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv8::Default, 4>>()("bytell_hash_map", power_of_two.AddCategory("bytell block size", "4"));
    RegisterLookups<ska::bytell_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv8::Default, 1>>()("bytell_hash_map", power_of_two.AddCategory("bytell block size", "1"));
}
