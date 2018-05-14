#pragma once

#include <new>
#include <cstdint>
#include <limits>

struct counting_allocator_stats
{
    static thread_local std::size_t num_allocated;
    static thread_local std::size_t num_freed;
    static thread_local std::size_t num_bytes_allocated;
    static thread_local std::size_t num_bytes_freed;
};

template<typename T>
struct counting_allocator
{
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    counting_allocator()
    {
    }

    template<typename U>
    counting_allocator(counting_allocator<U>)
    {
    }

    T * allocate(std::size_t n)
    {
        ++counting_allocator_stats::num_allocated;
        std::size_t bytes = n * sizeof(T);
        counting_allocator_stats::num_bytes_allocated += bytes;
        return static_cast<T *>(::operator new(bytes));
    }
    void deallocate(T * ptr, std::size_t n)
    {
        std::size_t bytes = n * sizeof(T);
        ++counting_allocator_stats::num_freed;
        counting_allocator_stats::num_bytes_freed += bytes;
        return ::operator delete(ptr);
    }
    template<typename U, typename... Args>
    void construct(U * ptr, Args &&... args)
    {
        new (ptr) U(std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U * ptr)
    {
        ptr->~U();
    }

    std::size_t max_size() const
    {
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    template<typename U>
    struct rebind
    {
        using other = counting_allocator<U>;
    };

    bool operator==(const counting_allocator &) const
    {
        return true;
    }
    bool operator!=(const counting_allocator &) const
    {
        return false;
    }
};

