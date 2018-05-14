#include "memory/metrics.hpp"
#include <cstdlib>


namespace memory_metrics
{
std::atomic<size_t> allocations(0);
std::atomic<size_t> frees(0);
std::atomic<size_t> total_allocated(0);
}

#if !defined(ADDRESS_SANITIZER_BUILD)
void * operator new(size_t size)
{
    memory_metrics::allocations.fetch_add(1, std::memory_order_relaxed);
    memory_metrics::total_allocated.fetch_add(size, std::memory_order_relaxed);
    return malloc(size);
}
void operator delete(void * ptr) noexcept
{
    if (!ptr)
        return;
    memory_metrics::frees.fetch_add(1, std::memory_order_relaxed);
    free(ptr);
}
void operator delete(void * ptr, size_t) noexcept
{
    ::operator delete(ptr);
}
#endif
