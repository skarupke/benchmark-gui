#pragma once

#include <atomic>
#include <cstddef>

namespace memory_metrics
{
extern std::atomic<size_t> allocations;
extern std::atomic<size_t> frees;
extern std::atomic<size_t> total_allocated;
}

#ifndef DISABLE_TESTS
#include "test/include_test.hpp"
struct RequireNoLeaks
{
    ~RequireNoLeaks()
    {
        AssertNoLeaks();
    }
private:
    size_t allocations_before = memory_metrics::allocations;
    size_t frees_before = memory_metrics::frees;

    void AssertNoLeaks()
    {
        size_t allocations = memory_metrics::allocations - allocations_before;
        size_t frees = memory_metrics::frees - frees_before;
        ASSERT_EQ(allocations, frees);
    }
};

#endif

