#include "hashtable_benchmarks/lookups.hpp"
#include "container/tessil/hopscotch_map.h"
#include "container/tessil/hopscotch_sc_map.h"
#include "container/tessil/robin_map.h"

void RegisterTessilHashtables()
{
    CategoryBuilder categories_so_far;
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<tsl::hopscotch_map<KeyPlaceHolder, ValuePlaceHolder, std::hash<KeyPlaceHolder>, std::equal_to<KeyPlaceHolder>, Allocator>>()("tsl::hopscotch_map", categories_so_far);
    RegisterLookups<tsl::hopscotch_sc_map<KeyPlaceHolder, ValuePlaceHolder, std::hash<KeyPlaceHolder>, std::equal_to<KeyPlaceHolder>, std::less<KeyPlaceHolder>, Allocator>>()("tsl::hopscotch_sc_map", categories_so_far);
    RegisterLookups<tsl::robin_map<KeyPlaceHolder, ValuePlaceHolder, std::hash<KeyPlaceHolder>, std::equal_to<KeyPlaceHolder>, Allocator>>()("tsl::robin_map", categories_so_far);
}
