#pragma once

#include "test/include_test.hpp"
#include <cstdint>
#include <utility>

struct IdentityHasher
{
    template<typename T>
    std::size_t operator()(T value) const
    {
        return value;
    }
};
struct CtorDtorCounter
{
    CtorDtorCounter(int & ctor_counter, int & dtor_counter)
        : ctor_counter(ctor_counter), dtor_counter(dtor_counter)
    {
        ++ctor_counter;
    }
    CtorDtorCounter(const CtorDtorCounter & other)
        : ctor_counter(other.ctor_counter), dtor_counter(other.dtor_counter)
    {
        ++ctor_counter;
    }
    CtorDtorCounter & operator=(const CtorDtorCounter &)
    {
        return *this;
    }
    ~CtorDtorCounter()
    {
        ++dtor_counter;
    }

private:
    int & ctor_counter;
    int & dtor_counter;
};
struct CountingAllocatorBase
{
    static std::size_t construction_counter;
    static std::size_t destruction_counter;
    static std::size_t num_allocations;
    static std::size_t num_frees;
};
template<typename T>
struct CountingAllocator : CountingAllocatorBase
{
    CountingAllocator() = default;
    template<typename U>
    CountingAllocator(CountingAllocator<U>)
    {
    }

    typedef T value_type;
    template<typename U>
    struct rebind
    {
        typedef CountingAllocator<U> other;
    };

    T * allocate(std::size_t n, const void * = nullptr)
    {
        ++num_allocations;
        return static_cast<T *>(malloc(sizeof(T) * n));
    }
    void deallocate(T * ptr, std::size_t)
    {
        ++num_frees;
        return free(ptr);
    }
    template<typename U, typename... Args>
    void construct(U * ptr, Args &&... args)
    {
        new (ptr) U(std::forward<Args>(args)...);
        ++construction_counter;
    }
    template<typename U>
    void destroy(U * ptr)
    {
        ++destruction_counter;
        ptr->~U();
    }
};
struct ScopedAssertNoLeaks
{
    ScopedAssertNoLeaks(bool expect_allocations, bool expect_constructions)
        : expect_allocations(expect_allocations)
        , expect_constructions(expect_constructions)
        , constructions_before(CountingAllocatorBase::construction_counter)
        , destructions_before(CountingAllocatorBase::destruction_counter)
        , allocations_before(CountingAllocatorBase::num_allocations)
        , frees_before(CountingAllocatorBase::num_frees)
    {
    }
    ~ScopedAssertNoLeaks()
    {
        run_asserts();
    }

private:
    bool expect_allocations;
    bool expect_constructions;
    std::size_t constructions_before;
    std::size_t destructions_before;
    std::size_t allocations_before;
    std::size_t frees_before;

    void run_asserts()
    {
        if (expect_allocations) ASSERT_NE(CountingAllocatorBase::num_allocations, allocations_before);
        else ASSERT_EQ(CountingAllocatorBase::num_allocations, allocations_before);
        if (expect_constructions) ASSERT_NE(CountingAllocatorBase::construction_counter, constructions_before);
        else ASSERT_EQ(CountingAllocatorBase::construction_counter, constructions_before);
        ASSERT_EQ(CountingAllocatorBase::construction_counter - constructions_before, CountingAllocatorBase::destruction_counter - destructions_before);
        ASSERT_EQ(CountingAllocatorBase::num_allocations - allocations_before, CountingAllocatorBase::num_frees - frees_before);
    }
};

struct stateful_hasher
{
    stateful_hasher() = default;
    stateful_hasher(const stateful_hasher & other)
        : to_add(other.to_add + 1)
    {
    }
    stateful_hasher(stateful_hasher &&) = default;
    stateful_hasher & operator=(const stateful_hasher & other)
    {
        to_add = other.to_add + 1;
        return *this;
    }
    stateful_hasher & operator=(stateful_hasher &&) = default;

    template<typename T>
    size_t operator()(const T & value) const
    {
        return std::hash<T>()(value) + to_add;
    }

    size_t to_add = 0;
};
