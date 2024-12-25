#include "hashtable_benchmarks/lookups.hpp"
#include "container/agl_table.hpp"
#include "container/agl_fib_table.hpp"

void RegisterAglHashMap()
{
    skb::CategoryBuilder categories_so_far;
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<ska::agl_hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator, ska::detailv11::Hash>>()("agl_hash_map", categories_so_far.AddCategory("lookup_type", "hash"));
    RegisterLookups<ska::agl_hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator, ska::detailv11::Bool>>()("agl_hash_map", categories_so_far.AddCategory("lookup_type", "bool"));
    //RegisterLookups<ska::agl_fib_hash_map<KeyPlaceHolder, ValuePlaceHolder, std_hash, std::equal_to<>, Allocator, ska::detailv12::SimpleLookup>>()("agl_fib_hash_map", categories_so_far);
}
