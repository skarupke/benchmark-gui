#include "hashtable_benchmarks/lookups.hpp"
#include "container/flat_hash_map.hpp"
#include <x86intrin.h>
#include "container/flat_hash_map2.hpp"

template<typename K, typename V, typename H, typename E, typename A, auto S>
struct HasNumLookups<ska::flat_hash_map<K, V, H, E, A, S>>
{
    static constexpr bool value = true;
};


struct crc32_hash_policy
{
    template<int /*num_extra_bits*/>
    size_t index_for_hash(size_t hash, size_t num_slots_minus_one) const
    {
        return _mm_crc32_u64(0, hash) & num_slots_minus_one;
    }
    size_t keep_in_range(size_t index, size_t num_slots_minus_one) const
    {
        return index & num_slots_minus_one;
    }

    int next_size_over(size_t & size) const
    {
        size = ska::detailv3::next_power_of_two(size);
        return 0;
    }
    void commit(int)
    {
    }
    void reset()
    {
    }
};

template<typename T>
struct crc32_std_hash : std::hash<T>
{
    typedef crc32_hash_policy hash_policy;
};


void RegisterFlatHashMap()
{
    skb::CategoryBuilder categories_so_far;
    skb::CategoryBuilder prime = categories_so_far.AddCategory("bucket count", "prime number");
    //skb::CategoryBuilder libdivide = categories_so_far.AddCategory("bucket count", "prime number libdivide");
    skb::CategoryBuilder power_of_two = categories_so_far.AddCategory("bucket count", "power of two");
    skb::CategoryBuilder fibonacci = categories_so_far.AddCategory("bucket count", "fibonacci");
    skb::CategoryBuilder crc32 = categories_so_far.AddCategory("bucket count", "crc32");
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator>>()("flat_hash_map", prime);
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator>>()("flat_hash_map", power_of_two);
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv3::EqualityCheck>>()("flat_hash_map", power_of_two.AddCategory("flat_hash_map variation", "equality check"));
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv3::SimpleLookup, int>>()("flat_hash_map", power_of_two.AddCategory("flat_hash_map variation", "int distance"));
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv3::Indexing>>()("flat_hash_map", power_of_two.AddCategory("flat_hash_map variation", "indexing"));
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv3::Indexing, int>>()("flat_hash_map", power_of_two.AddCategory("flat_hash_map variation", "indexing int"));
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv3::Indexing, std::ptrdiff_t>>()("flat_hash_map", power_of_two.AddCategory("flat_hash_map variation", "indexing ptrdiff_t"));
    RegisterLookups<ska2::flat_hash_map2<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska2::detail_direct::SimpleLookup, int16_t>>()("flat_hash_map", power_of_two.AddCategory("flat_hash_map variation", "special direct"));
    //RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, power_of_two_hash, std::equal_to<>, Allocator, ska::detailv3::ExtraZeroCheck>>()("flat_hash_map", power_of_two.AddCategory("flat_hash_map variation", "zero check"));
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, ska::fibonacci_std_hash<KeyPlaceHolder>, std::equal_to<>, Allocator>>()("flat_hash_map", fibonacci);
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, crc32_std_hash<KeyPlaceHolder>, std::equal_to<>, Allocator>>()("flat_hash_map", crc32);
    RegisterLookups<ska::flat_hash_map<KeyPlaceHolder, ValuePlaceHolder, ska::fibonacci_std_hash<KeyPlaceHolder>, std::equal_to<>, Allocator, ska::detailv3::EqualityCheck>>()("flat_hash_map", fibonacci.AddCategory("flat_hash_map variation", "equality check"));
}
