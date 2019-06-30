#include "hashtable_benchmarks/lookups.hpp"
#include "container/libc++_unordered_map.hpp"

void RegisterLLVMUnorderedMap()
{
    skb::CategoryBuilder categories;
    categories = categories.AddCategory("unordered_map variant", "libc++ (llvm)");
    using Allocator = counting_allocator<std::pair<KeyPlaceHolder, ValuePlaceHolder>>;
    RegisterLookups<llvm_std::unordered_map<KeyPlaceHolder, ValuePlaceHolder, std::hash<KeyPlaceHolder>, std::equal_to<>, Allocator>>()("unordered_map", categories);
}
