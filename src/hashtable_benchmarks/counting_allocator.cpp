#include "hashtable_benchmarks/counting_allocator.hpp"

thread_local size_t counting_allocator_stats::num_allocated = 0;
thread_local size_t counting_allocator_stats::num_freed = 0;
thread_local size_t counting_allocator_stats::num_bytes_allocated = 0;
thread_local size_t counting_allocator_stats::num_bytes_freed = 0;




#ifndef DISABLE_GTEST
#include "test/include_test.hpp"

#include <vector>
#include <set>

TEST(counting_allocator, vector)
{
    std::vector<int, counting_allocator<int>> a;
    size_t num_allocated_before = counting_allocator_stats::num_allocated;
    size_t num_bytes_before = counting_allocator_stats::num_bytes_allocated;
    a.push_back(5);
    ASSERT_EQ(1u, counting_allocator_stats::num_allocated - num_allocated_before);
    ASSERT_LT(0u, counting_allocator_stats::num_bytes_allocated - num_bytes_before);
}

TEST(counting_allocator, set)
{
    std::set<int, std::less<int>, counting_allocator<int>> a;
    size_t num_allocated_before = counting_allocator_stats::num_allocated;
    size_t num_bytes_before = counting_allocator_stats::num_bytes_allocated;
    a.insert(5);
    ASSERT_EQ(1u, counting_allocator_stats::num_allocated - num_allocated_before);
    ASSERT_LT(0u, counting_allocator_stats::num_bytes_allocated - num_bytes_before);
}

#endif
