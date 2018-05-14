//          Copyright Malte Skarupke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <array>
#include <algorithm>
#include <iterator>
#include <utility>
#include <type_traits>
#include "flat_hash_map.hpp"

#ifdef _MSC_VER
#define SKA_NOINLINE(...) __declspec(noinline) __VA_ARGS__
#else
#define SKA_NOINLINE(...) __VA_ARGS__ __attribute__((noinline))
#endif

namespace ska
{

namespace detailv4
{
template<typename T, typename Allocator>
struct sherwood_v4_entry
{
    sherwood_v4_entry()
    {
    }
    ~sherwood_v4_entry()
    {
    }

    bool has_value() const
    {
        return next;
    }
    bool is_empty() const
    {
        return !next;
    }
    template<typename... Args>
    void emplace(Args &&... args)
    {
        new (std::addressof(value)) T(std::forward<Args>(args)...);
    }

    void destroy_value()
    {
        value.~T();
    }

    using EntryPointer = typename std::allocator_traits<typename std::allocator_traits<Allocator>::template rebind_alloc<sherwood_v4_entry>>::pointer;

    EntryPointer next = nullptr;
    union
    {
        T value;
        size_t num_elements_in_this_chunk;
    };

    static sherwood_v4_entry empty_entry[3];

    static constexpr sherwood_v4_entry * special_end_entry = empty_entry;
};
template<typename T, typename A>
sherwood_v4_entry<T, A> sherwood_v4_entry<T, A>::empty_entry[3];

template<typename Result, typename Functor>
struct functor_storage : Functor
{
    functor_storage() = default;
    functor_storage(const Functor & functor)
        : Functor(functor)
    {
    }
    template<typename... Args>
    Result operator()(Args &&... args)
    {
        return static_cast<Functor &>(*this)(std::forward<Args>(args)...);
    }
    template<typename... Args>
    Result operator()(Args &&... args) const
    {
        return static_cast<const Functor &>(*this)(std::forward<Args>(args)...);
    }
};
template<typename Result, typename... Args>
struct functor_storage<Result, Result (*)(Args...)>
{
    typedef Result (*function_ptr)(Args...);
    function_ptr function;
    functor_storage(function_ptr function)
        : function(function)
    {
    }
    Result operator()(Args... args) const
    {
        return function(std::forward<Args>(args)...);
    }
    operator function_ptr &()
    {
        return function;
    }
    operator const function_ptr &()
    {
        return function;
    }
};
template<typename key_type, typename value_type, typename hasher>
struct KeyOrValueHasher : functor_storage<size_t, hasher>
{
    typedef functor_storage<size_t, hasher> hasher_storage;
    KeyOrValueHasher() = default;
    KeyOrValueHasher(const hasher & hash)
        : hasher_storage(hash)
    {
    }
    size_t operator()(const key_type & key)
    {
        return static_cast<hasher_storage &>(*this)(key);
    }
    size_t operator()(const key_type & key) const
    {
        return static_cast<const hasher_storage &>(*this)(key);
    }
    size_t operator()(const value_type & value)
    {
        return static_cast<hasher_storage &>(*this)(value.first);
    }
    size_t operator()(const value_type & value) const
    {
        return static_cast<const hasher_storage &>(*this)(value.first);
    }
    template<typename F, typename S>
    size_t operator()(const std::pair<F, S> & value)
    {
        return static_cast<hasher_storage &>(*this)(value.first);
    }
    template<typename F, typename S>
    size_t operator()(const std::pair<F, S> & value) const
    {
        return static_cast<const hasher_storage &>(*this)(value.first);
    }
};
template<typename key_type, typename value_type, typename key_equal>
struct KeyOrValueEquality : functor_storage<bool, key_equal>
{
    typedef functor_storage<bool, key_equal> equality_storage;
    KeyOrValueEquality() = default;
    KeyOrValueEquality(const key_equal & equality)
        : equality_storage(equality)
    {
    }
    bool operator()(const key_type & lhs, const key_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs, rhs);
    }
    bool operator()(const key_type & lhs, const value_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs, rhs.first);
    }
    bool operator()(const value_type & lhs, const key_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs);
    }
    bool operator()(const value_type & lhs, const value_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
    template<typename F, typename S>
    bool operator()(const key_type & lhs, const std::pair<F, S> & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs, rhs.first);
    }
    template<typename F, typename S>
    bool operator()(const std::pair<F, S> & lhs, const key_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs);
    }
    template<typename F, typename S>
    bool operator()(const value_type & lhs, const std::pair<F, S> & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
    template<typename F, typename S>
    bool operator()(const std::pair<F, S> & lhs, const value_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
    template<typename FL, typename SL, typename FR, typename SR>
    bool operator()(const std::pair<FL, SL> & lhs, const std::pair<FR, SR> & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
};

template<typename T, bool>
struct AssignIfTrue
{
    void operator()(T & lhs, const T & rhs)
    {
        lhs = rhs;
    }
    void operator()(T & lhs, T && rhs)
    {
        lhs = std::move(rhs);
    }
};
template<typename T>
struct AssignIfTrue<T, false>
{
    void operator()(T &, const T &)
    {
    }
    void operator()(T &, T &&)
    {
    }
};

using ska::detailv3::HashPolicySelector;

template<typename T, typename FindKey, typename ArgumentHash, typename Hasher, typename ArgumentEqual, typename Equal, typename ArgumentAlloc, typename EntryAlloc>
class sherwood_v4_table : private EntryAlloc, private Hasher, private Equal
{
    using Entry = detailv4::sherwood_v4_entry<T, ArgumentAlloc>;
    using AllocatorTraits = std::allocator_traits<EntryAlloc>;
    using EntryPointer = typename AllocatorTraits::pointer;
    struct convertible_to_iterator;

public:

    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = ArgumentHash;
    using key_equal = ArgumentEqual;
    using allocator_type = EntryAlloc;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    sherwood_v4_table()
    {
    }
    explicit sherwood_v4_table(size_type bucket_count, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : EntryAlloc(alloc), Hasher(hash), Equal(equal)
    {
        rehash(bucket_count);
    }
    sherwood_v4_table(size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v4_table(bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v4_table(size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v4_table(bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    explicit sherwood_v4_table(const ArgumentAlloc & alloc)
        : EntryAlloc(alloc)
    {
    }
    template<typename It>
    sherwood_v4_table(It first, It last, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v4_table(bucket_count, hash, equal, alloc)
    {
        insert(first, last);
    }
    template<typename It>
    sherwood_v4_table(It first, It last, size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v4_table(first, last, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    template<typename It>
    sherwood_v4_table(It first, It last, size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v4_table(first, last, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v4_table(std::initializer_list<T> il, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v4_table(bucket_count, hash, equal, alloc)
    {
        if (bucket_count == 0)
            reserve(il.size());
        insert(il.begin(), il.end());
    }
    sherwood_v4_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v4_table(il, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v4_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v4_table(il, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v4_table(const sherwood_v4_table & other)
        : sherwood_v4_table(other, AllocatorTraits::select_on_container_copy_construction(other.get_allocator()))
    {
    }
    sherwood_v4_table(const sherwood_v4_table & other, const ArgumentAlloc & alloc)
        : EntryAlloc(alloc), Hasher(other), Equal(other), _max_load_factor(other._max_load_factor)
    {
        try
        {
            rehash_for_other_container(other);
            insert(other.begin(), other.end());
        }
        catch(...)
        {
            clear();
            deallocate_data();
            throw;
        }
    }
    sherwood_v4_table(sherwood_v4_table && other) noexcept
        : EntryAlloc(std::move(other)), Hasher(std::move(other)), Equal(std::move(other)), _max_load_factor(other._max_load_factor)
    {
        swap_pointers(other);
    }
    sherwood_v4_table(sherwood_v4_table && other, const ArgumentAlloc & alloc) noexcept
        : EntryAlloc(alloc), Hasher(std::move(other)), Equal(std::move(other)), _max_load_factor(other._max_load_factor)
    {
        swap_pointers(other);
    }
    sherwood_v4_table & operator=(const sherwood_v4_table & other)
    {
        if (this == std::addressof(other))
            return *this;

        clear();
        if (AllocatorTraits::propagate_on_container_copy_assignment::value)
        {
            if (static_cast<EntryAlloc &>(*this) != static_cast<const EntryAlloc &>(other))
            {
                reset_to_empty_state();
            }
            AssignIfTrue<EntryAlloc, AllocatorTraits::propagate_on_container_copy_assignment::value>()(*this, other);
        }
        _max_load_factor = other._max_load_factor;
        static_cast<Hasher &>(*this) = other;
        static_cast<Equal &>(*this) = other;
        rehash_for_other_container(other);
        insert(other.begin(), other.end());
        return *this;
    }
    sherwood_v4_table & operator=(sherwood_v4_table && other) noexcept
    {
        if (this == std::addressof(other))
            return *this;
        else if (AllocatorTraits::propagate_on_container_move_assignment::value)
        {
            clear();
            reset_to_empty_state();
            AssignIfTrue<EntryAlloc, AllocatorTraits::propagate_on_container_move_assignment::value>()(*this, std::move(other));
            swap_pointers(other);
        }
        else if (static_cast<EntryAlloc &>(*this) == static_cast<EntryAlloc &>(other))
        {
            swap_pointers(other);
        }
        else
        {
            clear();
            _max_load_factor = other._max_load_factor;
            rehash_for_other_container(other);
            for (T & elem : other)
                emplace(std::move(elem));
            other.clear();
        }
        static_cast<Hasher &>(*this) = std::move(other);
        static_cast<Equal &>(*this) = std::move(other);
        return *this;
    }
    ~sherwood_v4_table()
    {
        clear();
        deallocate_data();
    }

    const allocator_type & get_allocator() const
    {
        return static_cast<const allocator_type &>(*this);
    }
    const ArgumentEqual & key_eq() const
    {
        return static_cast<const ArgumentEqual &>(*this);
    }
    const ArgumentHash & hash_function() const
    {
        return static_cast<const ArgumentHash &>(*this);
    }

    template<typename ValueType>
    struct templated_iterator
    {
        templated_iterator()
        {
        }
        templated_iterator(EntryPointer element, EntryPointer bucket)
            : current_element(element), current_bucket(bucket)
        {
        }

        EntryPointer current_element = EntryPointer();
        EntryPointer current_bucket = EntryPointer();

        using iterator_category = std::forward_iterator_tag;
        using value_type = ValueType;
        using difference_type = ptrdiff_t;
        using pointer = ValueType *;
        using reference = ValueType &;

        friend bool operator==(const templated_iterator & lhs, const templated_iterator & rhs)
        {
            return lhs.current_element == rhs.current_element;
        }
        friend bool operator!=(const templated_iterator & lhs, const templated_iterator & rhs)
        {
            return !(lhs == rhs);
        }

        templated_iterator & operator++()
        {
            if (current_element->next == Entry::special_end_entry)
            {
                do
                {
                    --current_bucket;
                }
                while (current_bucket->is_empty());
                current_element = current_bucket;
            }
            else
                current_element = current_element->next;
            return *this;
        }
        templated_iterator operator++(int)
        {
            templated_iterator copy(*this);
            ++*this;
            return copy;
        }

        ValueType & operator*() const
        {
            return current_element->value;
        }
        ValueType * operator->() const
        {
            return std::addressof(current_element->value);
        }

        operator templated_iterator<const value_type>() const
        {
            return { current_element, current_bucket };
        }
    };
    using iterator = templated_iterator<value_type>;
    using const_iterator = templated_iterator<const value_type>;

    iterator begin()
    {
        if (num_slots_minus_one)
        {
            for (EntryPointer it = entries + ptrdiff_t(num_slots_minus_one);; --it)
            {
                if (it->has_value())
                    return { it, it };
            }
        }
        else
            return end();
    }
    const_iterator begin() const
    {
        return const_cast<sherwood_v4_table *>(this)->begin();
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    iterator end()
    {
        return { entries - ptrdiff_t(1), entries - ptrdiff_t(1) };
    }
    const_iterator end() const
    {
        return { entries - ptrdiff_t(1), entries - ptrdiff_t(1) };
    }
    const_iterator cend() const
    {
        return end();
    }

    iterator find(const FindKey & key)
    {
        size_t index = hash_policy.template index_for_hash<0>(hash_object(key), num_slots_minus_one);
        EntryPointer bucket = entries + ptrdiff_t(index);
        if (bucket->has_value())
        {
            for (EntryPointer it = bucket;; it = it->next)
            {
                if (compares_equal(key, it->value))
                    return { it, bucket };
                if (it->next == Entry::special_end_entry)
                    break;
            }
        }
        return end();
    }
    const_iterator find(const FindKey & key) const
    {
        return const_cast<sherwood_v4_table *>(this)->find(key);
    }
    size_t count(const FindKey & key) const
    {
        return find(key) == end() ? 0 : 1;
    }
    std::pair<iterator, iterator> equal_range(const FindKey & key)
    {
        iterator found = find(key);
        if (found == end())
            return { found, found };
        else
            return { found, std::next(found) };
    }
    std::pair<const_iterator, const_iterator> equal_range(const FindKey & key) const
    {
        const_iterator found = find(key);
        if (found == end())
            return { found, found };
        else
            return { found, std::next(found) };
    }
    // debug function. same as find(), but returns how many items were
    // looked at
    int num_lookups(const FindKey & key)
    {
        int result = 0;
        size_t index = hash_policy.template index_for_hash<0>(hash_object(key), num_slots_minus_one);
        EntryPointer bucket = entries + ptrdiff_t(index);
        if (bucket->has_value())
        {
            for (EntryPointer it = bucket;; it = it->next)
            {
                ++result;
                if (compares_equal(key, it->value))
                    break;
                if (it->next == Entry::special_end_entry)
                    break;
            }
        }
        return result;
    }

    template<typename Key, typename... Args>
    std::pair<iterator, bool> emplace(Key && key, Args &&... args)
    {
        size_t index = hash_policy.template index_for_hash<0>(hash_object(key), num_slots_minus_one);
        EntryPointer bucket = entries + ptrdiff_t(index);
        EntryPointer it = bucket;
        if (bucket->has_value())
        {
            for (;; it = it->next)
            {
                if (compares_equal(key, it->value))
                    return { { it, bucket }, false };
                if (it->next == Entry::special_end_entry)
                    break;
            }
        }
        return emplace_new_key(it, bucket, std::forward<Key>(key), std::forward<Args>(args)...);
    }

    std::pair<iterator, bool> insert(const value_type & value)
    {
        return emplace(value);
    }
    std::pair<iterator, bool> insert(value_type && value)
    {
        return emplace(std::move(value));
    }
    template<typename... Args>
    iterator emplace_hint(const_iterator, Args &&... args)
    {
        return emplace(std::forward<Args>(args)...).first;
    }
    iterator insert(const_iterator, const value_type & value)
    {
        return emplace(value).first;
    }
    iterator insert(const_iterator, value_type && value)
    {
        return emplace(std::move(value)).first;
    }

    template<typename It>
    void insert(It begin, It end)
    {
        for (; begin != end; ++begin)
        {
            emplace(*begin);
        }
    }
    void insert(std::initializer_list<value_type> il)
    {
        insert(il.begin(), il.end());
    }

    void rehash(size_t num_buckets)
    {
        num_buckets = std::max(num_buckets, static_cast<size_t>(std::ceil(num_elements / static_cast<double>(_max_load_factor))));
        if (num_buckets == 0)
        {
            reset_to_empty_state();
            return;
        }
        auto new_prime_index = hash_policy.next_size_over(num_buckets);
        if (num_buckets == bucket_count())
            return;
        EntryPointer new_buckets(AllocatorTraits::allocate(*this, num_buckets + 1));
        EntryPointer end_it = new_buckets + static_cast<ptrdiff_t>(num_buckets + 1);
        for (EntryPointer it = new_buckets + ptrdiff_t(1); it != end_it; ++it)
        {
            it->next = nullptr;
        }
        new_buckets->num_elements_in_this_chunk = num_buckets + 1;
        new_buckets->next = entries - ptrdiff_t(1);
        std::swap(entries, new_buckets);
        ++entries;
        std::swap(num_slots_minus_one, num_buckets);
        --num_slots_minus_one;
        hash_policy.commit(new_prime_index);
        num_elements = 0;
        if (!num_buckets)
            return;

        for (EntryPointer it = new_buckets, end = it + static_cast<ptrdiff_t>(num_buckets + 1); it != end; ++it)
        {
            if (it->has_value())
            {
                for (EntryPointer e = it;;)
                {
                    EntryPointer next = e->next;
                    emplace(std::move(e->value));
                    e->destroy_value();
                    e->next = std::exchange(next_free_entry, e);
                    if (next == Entry::special_end_entry)
                        break;
                    e = next;
                }
            }
            else
                it->next = std::exchange(next_free_entry, it);
        }
    }

    void reserve(size_t num_elements)
    {
        if (!num_elements)
            return;
        num_elements = static_cast<size_t>(std::ceil(num_elements / static_cast<double>(_max_load_factor)));
        if (num_elements > bucket_count())
            rehash(num_elements);
        // if I have no free list, rehash a second time to populate the free
        // list. otherwise we will re-allocate the first time that there is a
        // hash collision. you don't want to re-allocate after a reserve
        if (!next_free_entry)
            rehash(bucket_count() + 1);
    }

    // the return value is a type that can be converted to an iterator
    // the reason for doing this is that it's not free to find the
    // iterator pointing at the next element. if you care about the
    // next iterator, turn the return value into an iterator
    convertible_to_iterator erase(const_iterator to_erase)
    {
        --num_elements;
        EntryPointer element = to_erase.current_element;
        EntryPointer bucket = to_erase.current_bucket;
        if (bucket == element)
        {
            if (element->next == Entry::special_end_entry)
            {
                element->destroy_value();
                element->next = nullptr;
                return { Entry::special_end_entry, bucket };
            }
            else
            {
                EntryPointer next = element->next;
                element->destroy_value();
                element->emplace(std::move(next->value));
                next->destroy_value();
                element->next = next->next;
                next->next = next_free_entry;
                next_free_entry = next;
                return { element, bucket };
            }
        }
        else
        {
            for (EntryPointer * next = std::addressof(bucket->next);; next = std::addressof((*next)->next))
            {
                if (*next != element)
                    continue;
                element->destroy_value();
                *next = element->next;
                element->next = next_free_entry;
                next_free_entry = element;
                return { *next, bucket };
            }
        }
    }

    iterator erase(const_iterator begin_it, const_iterator end_it)
    {
        while (begin_it.current_bucket != end_it.current_bucket)
        {
            begin_it = erase(begin_it);
        }
        if (begin_it.current_element != end_it.current_element)
        {
            --num_elements;
            begin_it.current_element->destroy_value();
            for (EntryPointer it = begin_it.current_element->next; it != end_it.current_element;)
            {
                EntryPointer next = it->next;
                --num_elements;
                it->destroy_value();
                it->next = next_free_entry;
                next_free_entry = it;
                it = next;
            }
            begin_it.current_element->next = end_it.current_element->next;
            begin_it.current_element->emplace(std::move(end_it.current_element->value));
            end_it.current_element->destroy_value();
            end_it.current_element->next = next_free_entry;
            next_free_entry = end_it.current_element;
        }
        return { begin_it.current_element, begin_it.current_bucket };
    }

    size_t erase(const FindKey & key)
    {
        auto found = find(key);
        if (found == end())
            return 0;
        else
        {
            erase(found);
            return 1;
        }
    }

    void clear()
    {
        if (!num_slots_minus_one)
            return;
        for (EntryPointer it = entries, end = it + static_cast<ptrdiff_t>(num_slots_minus_one + 1); it != end; ++it)
        {
            if (!it->has_value())
                continue;

            it->destroy_value();
            for (EntryPointer e = it->next; e != Entry::special_end_entry;)
            {
                EntryPointer next = e->next;
                e->destroy_value();
                e->next = next_free_entry;
                next_free_entry = e;
                e = next;
            }
            it->next = nullptr;
        }
        num_elements = 0;
    }

    void swap(sherwood_v4_table & other)
    {
        using std::swap;
        swap_pointers(other);
        swap(static_cast<ArgumentHash &>(*this), static_cast<ArgumentHash &>(other));
        swap(static_cast<ArgumentEqual &>(*this), static_cast<ArgumentEqual &>(other));
        if (AllocatorTraits::propagate_on_container_swap::value)
            swap(static_cast<EntryAlloc &>(*this), static_cast<EntryAlloc &>(other));
    }

    size_t size() const
    {
        return num_elements;
    }
    size_t max_size() const
    {
        return (AllocatorTraits::max_size(*this)) / sizeof(Entry);
    }
    size_t bucket_count() const
    {
        return num_slots_minus_one + 1;
    }
    size_type max_bucket_count() const
    {
        return (AllocatorTraits::max_size(*this) - 1) / sizeof(Entry);
    }
    size_t bucket(const FindKey & key) const
    {
        return hash_policy.template index_for_hash<0>(hash_object(key), num_slots_minus_one);
    }
    float load_factor() const
    {
        size_t buckets = bucket_count();
        if (buckets)
            return static_cast<float>(num_elements) / bucket_count();
        else
            return 0;
    }
    void max_load_factor(float value)
    {
        _max_load_factor = value;
    }
    float max_load_factor() const
    {
        return _max_load_factor;
    }

    bool empty() const
    {
        return num_elements == 0;
    }

private:
    EntryPointer entries = Entry::empty_entry + 1;
    size_t num_slots_minus_one = 0;
    typename HashPolicySelector<ArgumentHash>::type hash_policy;
    float _max_load_factor = 1.0f;
    size_t num_elements = 0;
    EntryPointer next_free_entry = nullptr;

    void rehash_for_other_container(const sherwood_v4_table & other)
    {
        reserve(other.size());
    }

    void swap_pointers(sherwood_v4_table & other)
    {
        using std::swap;
        swap(hash_policy, other.hash_policy);
        swap(entries, other.entries);
        swap(num_slots_minus_one, other.num_slots_minus_one);
        swap(num_elements, other.num_elements);
        swap(_max_load_factor, other._max_load_factor);
        // todo: swap next_free_entry if the allocator is swapped
    }

    template<typename Key, typename... Args>
    SKA_NOINLINE(std::pair<iterator, bool>) emplace_new_key(EntryPointer current_entry, EntryPointer current_bucket, Key && key, Args &&... args)
    {
        using std::swap;
        if (!num_slots_minus_one || static_cast<double>(num_elements + 1) / static_cast<double>(num_slots_minus_one + 1) > _max_load_factor)
        {
            grow();
            return emplace(std::forward<Key>(key), std::forward<Args>(args)...);
        }
        else if (current_entry->next == nullptr)
        {
            current_entry->emplace(std::forward<Key>(key), std::forward<Args>(args)...);
            current_entry->next = Entry::special_end_entry;
            ++num_elements;
            return { { current_entry, current_bucket }, true };
        }
        else if (next_free_entry == nullptr)
        {
            if (num_slots_minus_one < 16)
            {
                grow();
                return emplace(std::forward<Key>(key), std::forward<Args>(args)...);
            }
            grow_free_list();
        }
        EntryPointer to_insert = next_free_entry;
        to_insert->emplace(std::forward<Key>(key), std::forward<Args>(args)...);
        next_free_entry = next_free_entry->next;
        to_insert->next = std::exchange(current_entry->next, to_insert);
        ++num_elements;
        return { { to_insert, current_bucket }, true };
    }

    void grow()
    {
        rehash(std::max(size_t(4), 2 * bucket_count()));
    }

    void grow_free_list()
    {
        static constexpr ptrdiff_t to_allocate = 16;
        EntryPointer new_buckets(AllocatorTraits::allocate(*this, to_allocate));
        new_buckets->num_elements_in_this_chunk = to_allocate;
        new_buckets->next = std::exchange(entries[-1].next, new_buckets);
        for (EntryPointer it = new_buckets + ptrdiff_t(1), end_it = new_buckets + to_allocate; it != end_it; ++it)
            it->next = std::exchange(next_free_entry, it);
    }

    void deallocate_data()
    {
        for (EntryPointer storage = entries - ptrdiff_t(1); storage != Entry::empty_entry;)
        {
            EntryPointer next = storage->next;
            AllocatorTraits::deallocate(*this, storage, storage->num_elements_in_this_chunk);
            storage = next;
        }
    }

    void reset_to_empty_state()
    {
        deallocate_data();
        entries = Entry::empty_entry + 1;
        num_slots_minus_one = 0;
        hash_policy.reset();
        next_free_entry = nullptr;
    }

    template<typename U>
    size_t hash_object(const U & key)
    {
        return static_cast<Hasher &>(*this)(key);
    }
    template<typename U>
    size_t hash_object(const U & key) const
    {
        return static_cast<const Hasher &>(*this)(key);
    }
    template<typename L, typename R>
    bool compares_equal(const L & lhs, const R & rhs)
    {
        return static_cast<Equal &>(*this)(lhs, rhs);
    }

    struct convertible_to_iterator
    {
        EntryPointer element;
        EntryPointer bucket;

        operator iterator()
        {
            if (element == Entry::special_end_entry)
            {
                do
                {
                    --bucket;
                }
                while (bucket->is_empty());
                return { bucket, bucket };
            }
            else
                return { element, bucket };
        }
        operator const_iterator()
        {
            if (element == Entry::special_end_entry || element->is_empty())
            {
                do
                {
                    --bucket;
                }
                while (bucket->is_empty());
                return { bucket, bucket };
            }
            else
                return { element, bucket };
        }
    };

};
}


template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<K, V> > >
class ptr_hash_map
        : public detailv4::sherwood_v4_table
        <
            std::pair<K, V>,
            K,
            H,
            detailv4::KeyOrValueHasher<K, std::pair<K, V>, H>,
            E,
            detailv4::KeyOrValueEquality<K, std::pair<K, V>, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<detailv4::sherwood_v4_entry<std::pair<K, V>, A>>
        >
{
    using Table = detailv4::sherwood_v4_table
    <
        std::pair<K, V>,
        K,
        H,
        detailv4::KeyOrValueHasher<K, std::pair<K, V>, H>,
        E,
        detailv4::KeyOrValueEquality<K, std::pair<K, V>, E>,
        A,
        typename std::allocator_traits<A>::template rebind_alloc<detailv4::sherwood_v4_entry<std::pair<K, V>, A>>
    >;
public:

    using key_type = K;
    using mapped_type = V;

    using Table::Table;
    ptr_hash_map()
    {
    }

    V & operator[](const K & key)
    {
        return emplace(key, convertible_to_value()).first->second;
    }
    V & operator[](K && key)
    {
        return emplace(std::move(key), convertible_to_value()).first->second;
    }
    V & at(const K & key)
    {
        auto found = this->find(key);
        if (found == this->end())
            throw std::out_of_range("Argument passed to at() was not in the map.");
        return found->second;
    }
    const V & at(const K & key) const
    {
        auto found = this->find(key);
        if (found == this->end())
            throw std::out_of_range("Argument passed to at() was not in the map.");
        return found->second;
    }

    using Table::emplace;
    std::pair<typename Table::iterator, bool> emplace()
    {
        return emplace(key_type(), convertible_to_value());
    }

    friend bool operator==(const ptr_hash_map & lhs, const ptr_hash_map & rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        for (const typename Table::value_type & value : lhs)
        {
            auto found = rhs.find(value.first);
            if (found == rhs.end())
                return false;
            else if (value.second != found->second)
                return false;
        }
        return true;
    }
    friend bool operator!=(const ptr_hash_map & lhs, const ptr_hash_map & rhs)
    {
        return !(lhs == rhs);
    }

private:
    struct convertible_to_value
    {
        operator V() const
        {
            return V();
        }
    };
};

template<typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename A = std::allocator<T> >
class ptr_hash_set
        : public detailv4::sherwood_v4_table
        <
            T,
            T,
            H,
            detailv4::functor_storage<size_t, H>,
            E,
            detailv4::functor_storage<bool, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<detailv4::sherwood_v4_entry<T, A>>
        >
{
    using Table = detailv4::sherwood_v4_table
    <
        T,
        T,
        H,
        detailv4::functor_storage<size_t, H>,
        E,
        detailv4::functor_storage<bool, E>,
        A,
        typename std::allocator_traits<A>::template rebind_alloc<detailv4::sherwood_v4_entry<T, A>>
    >;
public:

    using key_type = T;

    using Table::Table;
    ptr_hash_set()
    {
    }

    template<typename... Args>
    std::pair<typename Table::iterator, bool> emplace(Args &&... args)
    {
        return Table::emplace(T(std::forward<Args>(args)...));
    }
    std::pair<typename Table::iterator, bool> emplace(const key_type & arg)
    {
        return Table::emplace(arg);
    }
    std::pair<typename Table::iterator, bool> emplace(key_type & arg)
    {
        return Table::emplace(arg);
    }
    std::pair<typename Table::iterator, bool> emplace(const key_type && arg)
    {
        return Table::emplace(std::move(arg));
    }
    std::pair<typename Table::iterator, bool> emplace(key_type && arg)
    {
        return Table::emplace(std::move(arg));
    }

    friend bool operator==(const ptr_hash_set & lhs, const ptr_hash_set & rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        for (const T & value : lhs)
        {
            if (rhs.find(value) == rhs.end())
                return false;
        }
        return true;
    }
    friend bool operator!=(const ptr_hash_set & lhs, const ptr_hash_set & rhs)
    {
        return !(lhs == rhs);
    }
};

} // end namespace ska
