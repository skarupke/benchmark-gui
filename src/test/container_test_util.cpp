#include "test/container_test_util.hpp"

std::size_t CountingAllocatorBase::construction_counter = 0;
std::size_t CountingAllocatorBase::destruction_counter = 0;
std::size_t CountingAllocatorBase::num_allocations = 0;
std::size_t CountingAllocatorBase::num_frees = 0;
