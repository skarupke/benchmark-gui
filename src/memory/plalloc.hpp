#pragma once

#include <memory>
#include <vector>

template<typename T>
struct plalloc
{
    typedef T value_type;

    plalloc() = default;
    template<typename U>
    plalloc(const plalloc<U> &) {}
    plalloc(const plalloc &) {}
    plalloc & operator=(const plalloc &) { return *this; }
    plalloc(plalloc &&) = default;
    plalloc & operator=(plalloc &&) = default;

    typedef std::true_type propagate_on_container_copy_assignment;
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::true_type propagate_on_container_swap;

    bool operator==(const plalloc & other) const
    {
        return this == &other;
    }
    bool operator!=(const plalloc & other) const
    {
        return !(*this == other);
    }

    T * allocate(size_t num_to_allocate)
    {
        if (num_to_allocate != 1)
        {
            return static_cast<T *>(::operator new(sizeof(T) * num_to_allocate));
        }
        else if (first_free)
        {
            T * result = std::addressof(first_free->value);
            first_free = first_free->next;
            return result;
        }
        else
        {
            // first allocate 8, then double whenever
            // we run out of memory
            size_t to_allocate = 8 << memory.size();
            memory.emplace_back(new value_holder[to_allocate]);
            for (auto it = memory.back().get(), end = it + to_allocate - 1;; ++it)
            {
                if (it == end)
                    return std::addressof(it->value);
                it->next = first_free;
                first_free = it;
            }
        }
    }
    void deallocate(T * ptr, size_t num_to_free)
    {
        if (num_to_free == 1)
        {
            value_holder * holder = reinterpret_cast<value_holder *>(ptr);
            holder->next = first_free;
            first_free = holder;
        }
        else
        {
            ::operator delete(ptr);
        }
    }

    // boilerplate that shouldn't be needed, except
    // libstdc++ doesn't use allocator_traits yet
    template<typename U>
    struct rebind
    {
        typedef plalloc<U> other;
    };
    typedef T * pointer;
    typedef const T * const_pointer;
    typedef T & reference;
    typedef const T & const_reference;
    template<typename U, typename... Args>
    void construct(U * object, Args &&... args)
    {
        new (object) U(std::forward<Args>(args)...);
    }
    template<typename U, typename... Args>
    void construct(const U * object, Args &&... args) = delete;
    template<typename U>
    void destroy(U * object)
    {
        object->~U();
    }

private:
    union value_holder
    {
        value_holder() {}
        ~value_holder() {}
        T value;
        value_holder * next;
    };

    std::vector<std::unique_ptr<value_holder[]>> memory;
    value_holder * first_free = nullptr;
};
