#include "hashtable_benchmarks/lookups.hpp"
#include "container/microsoft_unordered_map"

void RegisterMicrosoftUnorderedMap()
{
    skb::CategoryBuilder categories;
    categories = categories.AddCategory("unordered_map variant", "dinkumware");
    using Allocator = counting_allocator<std::pair<const KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<ms_std::unordered_map<KeyPlaceHolder, ValuePlaceHolder, ms_std::hash<KeyPlaceHolder>, std::equal_to<>, Allocator>>()("unordered_map", categories);
}
