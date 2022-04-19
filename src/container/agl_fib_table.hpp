//          Copyright Malte Skarupke 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <utility>
#include <type_traits>
#include <emmintrin.h> // for SSE and SSE2
#include <x86intrin.h> // for _bit_scan_forward
#include "flat_hash_map.hpp"
#include <vector>

#ifdef _MSC_VER
#define SKA_NOINLINE(...) __declspec(noinline) __VA_ARGS__
#else
#define SKA_NOINLINE(...) __VA_ARGS__ __attribute__((noinline))
#endif

namespace ska
{

namespace detailv12
{
using ska::detailv3::functor_storage;
using ska::detailv3::KeyOrValueEquality;
using ska::detailv3::KeyOrValueHasher;
using ska::detailv3::HashPolicySelector;
using ska::detailv3::AssignIfTrue;

using ska::detailv3::sherwood_v3_entry;

static constexpr int8_t max_lookups = 8;

template<typename Entry>
static Entry * empty_default_table()
{
    static Entry result[max_lookups] = { {}, {}, {}, {}, {}, {}, {}, {Entry::special_end_value} };
    return result;
}

enum LookupType
{
    SimpleLookup
};

template<typename T, typename FindKey, typename ArgumentHash, typename Hasher, typename ArgumentEqual, typename Equal, typename ArgumentAlloc, typename EntryAlloc, LookupType lookup_type>
class sherwood_v12_table : private EntryAlloc, private Hasher, private Equal
{
    using Entry = sherwood_v3_entry<T>;
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

    sherwood_v12_table()
    {
    }
    explicit sherwood_v12_table(size_type bucket_count, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : EntryAlloc(alloc), Hasher(hash), Equal(equal)
    {
        if (bucket_count)
            rehash(bucket_count);
    }
    sherwood_v12_table(size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v12_table(bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v12_table(size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v12_table(bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    explicit sherwood_v12_table(const ArgumentAlloc & alloc)
        : EntryAlloc(alloc)
    {
    }
    template<typename It>
    sherwood_v12_table(It first, It last, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v12_table(bucket_count, hash, equal, alloc)
    {
        insert(first, last);
    }
    template<typename It>
    sherwood_v12_table(It first, It last, size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v12_table(first, last, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    template<typename It>
    sherwood_v12_table(It first, It last, size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v12_table(first, last, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v12_table(std::initializer_list<T> il, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v12_table(bucket_count, hash, equal, alloc)
    {
        if (bucket_count == 0)
            rehash(il.size());
        insert(il.begin(), il.end());
    }
    sherwood_v12_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v12_table(il, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v12_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v12_table(il, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v12_table(const sherwood_v12_table & other)
        : sherwood_v12_table(other, AllocatorTraits::select_on_container_copy_construction(other.get_allocator()))
    {
    }
    sherwood_v12_table(const sherwood_v12_table & other, const ArgumentAlloc & alloc)
        : EntryAlloc(alloc), Hasher(other), Equal(other), _max_load_factor(other._max_load_factor)
    {
        rehash_for_other_container(other);
        try
        {
            insert(other.begin(), other.end());
        }
        catch(...)
        {
            clear();
            deallocate_data(entries0, num_slots_minus_one);
            throw;
        }
    }
    sherwood_v12_table(sherwood_v12_table && other) noexcept
        : EntryAlloc(std::move(other)), Hasher(std::move(other)), Equal(std::move(other))
        , _max_load_factor(other._max_load_factor)
    {
        swap_pointers(other);
    }
    sherwood_v12_table(sherwood_v12_table && other, const ArgumentAlloc & alloc) noexcept
        : EntryAlloc(alloc), Hasher(std::move(other)), Equal(std::move(other))
        , _max_load_factor(other._max_load_factor)
    {
        swap_pointers(other);
    }
    sherwood_v12_table & operator=(const sherwood_v12_table & other)
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
    sherwood_v12_table & operator=(sherwood_v12_table && other) noexcept
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
    ~sherwood_v12_table()
    {
        clear();
        deallocate_data(entries0, num_slots_minus_one);
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
    private:
        friend class sherwood_v12_table;
        EntryPointer current = EntryPointer();

    public:
        templated_iterator()
        {
        }
        templated_iterator(EntryPointer entries)
            : current(entries)
        {
        }

        using iterator_category = std::forward_iterator_tag;
        using value_type = ValueType;
        using difference_type = ptrdiff_t;
        using pointer = ValueType *;
        using reference = ValueType &;

        friend bool operator==(const templated_iterator & lhs, const templated_iterator & rhs)
        {
            return lhs.current == rhs.current;
        }
        friend bool operator!=(const templated_iterator & lhs, const templated_iterator & rhs)
        {
            return !(lhs == rhs);
        }

        templated_iterator & operator++()
        {
            do
            {
                ++current;
            }
            while(current->is_empty());
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
            return current->value;
        }
        ValueType * operator->() const
        {
            return std::addressof(current->value);
        }

        operator templated_iterator<const value_type>() const
        {
            return { current };
        }
    };
    using iterator = templated_iterator<value_type>;
    using const_iterator = templated_iterator<const value_type>;

    iterator begin()
    {
        for (EntryPointer it = entries0;; ++it)
        {
            if (it->has_value())
                return { it };
        }
    }
    const_iterator begin() const
    {
        for (EntryPointer it = entries0;; ++it)
        {
            if (it->has_value())
                return { it };
        }
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    iterator end()
    {
        return { entries0 + static_cast<ptrdiff_t>(num_slots_minus_one + max_lookups) };
    }
    const_iterator end() const
    {
        return { entries0 + static_cast<ptrdiff_t>(num_slots_minus_one + max_lookups) };
    }
    const_iterator cend() const
    {
        return end();
    }

    iterator find(const FindKey & key)
    {
        auto [index0, index1] = indices_from_hash(hash_object(key));
        EntryPointer it0 = entries0 + ptrdiff_t(index0);
        EntryPointer it1 = entries1 + ptrdiff_t(index1);
        for (int8_t distance = 0;;)
        {
            bool valid1 = it1->distance_from_desired >= distance;
            if (valid1)
            {
                if (compares_equal(key, it1->value))
                    return { it1 };
                ++it1;
            }
            for (;;)
            {
                if (it0->distance_from_desired >= distance++)
                {
                    if (compares_equal(key, it0->value))
                        return { it0 };
                    ++it0;
                    if (!valid1)
                        continue;
                }
                else if (!valid1)
                    return end();
                break;
            }
        }
    }
    inline const_iterator find(const FindKey & key) const
    {
        return const_cast<sherwood_v12_table *>(this)->find(key);
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

    template<typename Key, typename... Args>
    inline std::pair<iterator, bool> emplace(Key && key, Args &&... args)
    {
        size_t hash = hash_object(key);
        auto [index0, index1] = indices_from_hash(hash);
        EntryPointer it0 = entries0 + ptrdiff_t(index0);
        EntryPointer it1 = entries1 + ptrdiff_t(index1);
        for (int8_t distance0 = 0, distance1 = 0;;)
        {
            bool valid0 = it0[distance0].distance_from_desired >= distance0;
            if (valid0)
            {
                if (compares_equal(key, it0[distance0].value))
                    return { { it0 + distance0 }, false };
                ++distance0;
            }
            if (it1[distance1].distance_from_desired >= distance1)
            {
                if (compares_equal(key, it1[distance1].value))
                    return { { it1 + distance1 }, false };
                ++distance1;
            }
            else if (!valid0)
            {
                // prefer going right because the left one would overflow
                // into the right one, but the right one has a separate buffer
                // at the end to overflow into
                if (distance0 < distance1)
                    return emplace_new_key(distance0, it0 + distance0, std::forward<Key>(key), std::forward<Args>(args)...);
                else
                    return emplace_new_key(distance1, it1 + distance1, std::forward<Key>(key), std::forward<Args>(args)...);
            }
        }
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
            // todo: does this call destructors?
            reset_to_empty_state();
            return;
        }
        num_buckets = std::max(detailv3::next_power_of_two(num_buckets), size_t(4));
        if (num_buckets == bucket_count())
            return;
        // if this is ever not a constant again, need to change the loop for the
        // old buckets below
        int8_t new_max_lookups = max_lookups;//compute_max_lookups(num_buckets);
        EntryPointer new_buckets(AllocatorTraits::allocate(*this, num_buckets + new_max_lookups));
        EntryPointer special_end_item = new_buckets + static_cast<ptrdiff_t>(num_buckets + new_max_lookups - 1);
        for (EntryPointer it = new_buckets; it != special_end_item; ++it)
            it->distance_from_desired = -1;
        special_end_item->distance_from_desired = Entry::special_end_value;
        std::swap(entries0, new_buckets);
        entries1 = entries0 + num_buckets / 2;
        fib_shift_amount = 65 - detailv3::log2(num_buckets);
        std::swap(num_slots_minus_one, num_buckets);
        --num_slots_minus_one;
        num_elements = 0;
        if (new_buckets != empty_default_table<Entry>())
        {
            for (EntryPointer it = new_buckets, end = it + static_cast<ptrdiff_t>(num_buckets + new_max_lookups); it != end; ++it)
            {
                if (it->has_value())
                {
                    emplace(std::move(it->value));
                    it->destroy_value();
                }
            }
            deallocate_data(new_buckets, num_buckets);
        }
    }

    void reserve(size_t num_elements)
    {
        size_t required_buckets = num_buckets_for_reserve(num_elements);
        if (required_buckets > bucket_count())
            rehash(required_buckets);
    }

    // the return value is a type that can be converted to an iterator
    // the reason for doing this is that it's not free to find the
    // iterator pointing at the next element. if you care about the
    // next iterator, turn the return value into an iterator
    convertible_to_iterator erase(const_iterator to_erase)
    {
        // todo: implement
        return { to_erase.current };
    }

    iterator erase(const_iterator begin_it, const_iterator end_it)
    {
        // todo: implement
        if (begin_it == end_it)
            return { begin_it.current, begin_it.index };
        return { begin_it.current, begin_it.index };
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
        for (EntryPointer it = entries0, end = it + num_slots_minus_one + 1; it != end; ++it)
        {
            if (it->is_empty())
                continue;
            AllocatorTraits::destroy(*this, std::addressof(it->value));
            it->set_empty();
        }
        num_elements = 0;
    }

    void shrink_to_fit()
    {
        rehash_for_other_container(*this);
    }

    void swap(sherwood_v12_table & other)
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
        return (AllocatorTraits::max_size(*this)) / sizeof(T);
    }
    size_t bucket_count() const
    {
        return num_slots_minus_one ? num_slots_minus_one + 1 : 0;
    }
    size_type max_bucket_count() const
    {
        return (AllocatorTraits::max_size(*this)) / sizeof(T);
    }
    float load_factor() const
    {
        return static_cast<double>(num_elements) / (num_slots_minus_one + 1);
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
    EntryPointer entries0 = empty_default_table<Entry>();
    EntryPointer entries1 = empty_default_table<Entry>();
    size_t num_slots_minus_one = 0;
    int fib_shift_amount = 63;
    float _max_load_factor = 0.9375f;
    size_t num_elements = 0;

    size_t num_buckets_for_reserve(size_t num_elements) const
    {
        return static_cast<size_t>(std::ceil(num_elements / static_cast<double>(_max_load_factor)));
    }
    void rehash_for_other_container(const sherwood_v12_table & other)
    {
        rehash(std::min(num_buckets_for_reserve(other.size()), other.bucket_count()));
    }
    bool is_full() const
    {
        if (!num_slots_minus_one)
            return true;
        else
            return num_elements + 1 > (num_slots_minus_one + 1) * static_cast<double>(_max_load_factor);
    }

    void swap_pointers(sherwood_v12_table & other)
    {
        using std::swap;
        swap(fib_shift_amount, other.fib_shift_amount);
        swap(entries0, other.entries0);
        swap(entries1, other.entries1);
        swap(num_slots_minus_one, other.num_slots_minus_one);
        swap(num_elements, other.num_elements);
        swap(_max_load_factor, other._max_load_factor);
    }

    template<typename Key, typename... Args>
    SKA_NOINLINE(std::pair<iterator, bool>) emplace_new_key(int8_t distance_from_desired, EntryPointer current_entry, Key && key, Args &&... args)
    {
        using std::swap;
        if (is_full() || distance_from_desired == max_lookups)
        {
            grow();
            return emplace(std::forward<Key>(key), std::forward<Args>(args)...);
        }
        else if (current_entry->is_empty())
        {
            current_entry->emplace(distance_from_desired, std::forward<Key>(key), std::forward<Args>(args)...);
            ++num_elements;
            return { { current_entry }, true };
        }
        value_type to_insert(std::forward<Key>(key), std::forward<Args>(args)...);
        swap(distance_from_desired, current_entry->distance_from_desired);
        swap(to_insert, current_entry->value);
        iterator result = { current_entry };
        for (++distance_from_desired, ++current_entry;; ++current_entry)
        {
            if (current_entry->is_empty())
            {
                current_entry->emplace(distance_from_desired, std::move(to_insert));
                ++num_elements;
                return { result, true };
            }
            else if (current_entry->distance_from_desired < distance_from_desired)
            {
                swap(distance_from_desired, current_entry->distance_from_desired);
                swap(to_insert, current_entry->value);
                ++distance_from_desired;
            }
            else
            {
                ++distance_from_desired;
                if (distance_from_desired == max_lookups)
                {
                    swap(to_insert, result.current->value);
                    grow();
                    return emplace(std::move(to_insert));
                }
            }
        }
    }

    void grow()
    {
        rehash(std::max(size_t(1), 2 * bucket_count()));
    }

    void deallocate_data(EntryPointer begin, size_t num_slots_minus_one)
    {
        if (begin != empty_default_table<Entry>())
        {
            AllocatorTraits::deallocate(*this, begin, num_slots_minus_one + max_lookups + 1);
        }
    }

    void reset_to_empty_state()
    {
        deallocate_data(entries0, num_slots_minus_one);
        entries0 = empty_default_table<Entry>();
        entries1 = empty_default_table<Entry>();
        num_slots_minus_one = 0;
        fib_shift_amount = 63;
    }

    template<typename U>
    size_t hash_object(const U & key)
    {
        return const_cast<const sherwood_v12_table &>(*this).hash_object(key);
    }
    template<typename U>
    size_t hash_object(const U & key) const
    {
        return static_cast<const Hasher &>(*this)(key);
    }
    std::pair<size_t, size_t> indices_from_hash(size_t hash) const
    {
        size_t fib0 = hash * 11400714819323198485llu;
        size_t fib1 = hash * 7640891576956012809llu;
        return { fib0 >> fib_shift_amount, fib1 >> fib_shift_amount };
    }
    template<typename L, typename R>
    bool compares_equal(const L & lhs, const R & rhs)
    {
        return static_cast<Equal &>(*this)(lhs, rhs);
    }

    struct convertible_to_iterator
    {
        EntryPointer it;

        operator iterator() const
        {
            if (it->is_empty())
                return ++iterator{it};
            else
                return { it };
        }
        operator const_iterator() const
        {
            if (it->is_empty())
                return ++iterator{it};
            else
                return { it };
        }
    };
};
template<typename T, typename Enable = void>
struct AlignmentOr8Bytes
{
    static constexpr size_t value = 8;
};
template<typename T>
struct AlignmentOr8Bytes<T, typename std::enable_if<alignof(T) >= 1>::type>
{
    static constexpr size_t value = alignof(T);
};
template<typename... Args>
struct CalculateBytellBlockSize;
template<typename First, typename... More>
struct CalculateBytellBlockSize<First, More...>
{
    static constexpr size_t this_value = AlignmentOr8Bytes<First>::value;
    static constexpr size_t base_value = CalculateBytellBlockSize<More...>::value;
    static constexpr size_t value = this_value > base_value ? this_value : base_value;
};
template<>
struct CalculateBytellBlockSize<>
{
    static constexpr size_t value = 8;
};
}

template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<K, V> >, detailv12::LookupType lookup_type = detailv12::SimpleLookup >
class agl_fib_hash_map
        : public detailv12::sherwood_v12_table
        <
            std::pair<K, V>,
            K,
            H,
            detailv12::KeyOrValueHasher<K, std::pair<K, V>, H>,
            E,
            detailv12::KeyOrValueEquality<K, std::pair<K, V>, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<detailv12::sherwood_v3_entry<std::pair<K, V>>>,
            lookup_type
        >
{
    using Table = detailv12::sherwood_v12_table
    <
        std::pair<K, V>,
        K,
        H,
        detailv12::KeyOrValueHasher<K, std::pair<K, V>, H>,
        E,
        detailv12::KeyOrValueEquality<K, std::pair<K, V>, E>,
        A,
        typename std::allocator_traits<A>::template rebind_alloc<detailv12::sherwood_v3_entry<std::pair<K, V>>>,
        lookup_type
    >;
public:

    using key_type = K;
    using mapped_type = V;

    using Table::Table;
    agl_fib_hash_map()
    {
    }

    inline V & operator[](const K & key)
    {
        return emplace(key, convertible_to_value()).first->second;
    }
    inline V & operator[](K && key)
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
    template<typename M>
    std::pair<typename Table::iterator, bool> insert_or_assign(const key_type & key, M && m)
    {
        auto emplace_result = emplace(key, std::forward<M>(m));
        if (!emplace_result.second)
            emplace_result.first->second = std::forward<M>(m);
        return emplace_result;
    }
    template<typename M>
    std::pair<typename Table::iterator, bool> insert_or_assign(key_type && key, M && m)
    {
        auto emplace_result = emplace(std::move(key), std::forward<M>(m));
        if (!emplace_result.second)
            emplace_result.first->second = std::forward<M>(m);
        return emplace_result;
    }
    template<typename M>
    typename Table::iterator insert_or_assign(typename Table::const_iterator, const key_type & key, M && m)
    {
        return insert_or_assign(key, std::forward<M>(m)).first;
    }
    template<typename M>
    typename Table::iterator insert_or_assign(typename Table::const_iterator, key_type && key, M && m)
    {
        return insert_or_assign(std::move(key), std::forward<M>(m)).first;
    }

    friend bool operator==(const agl_fib_hash_map & lhs, const agl_fib_hash_map & rhs)
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
    friend bool operator!=(const agl_fib_hash_map & lhs, const agl_fib_hash_map & rhs)
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

template<typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename A = std::allocator<T>, detailv12::LookupType lookup_type = detailv12::SimpleLookup >
class agl_fib_hash_set
        : public detailv12::sherwood_v12_table
        <
            T,
            T,
            H,
            detailv12::functor_storage<size_t, H>,
            E,
            detailv12::functor_storage<bool, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<detailv12::sherwood_v3_entry<T>>,
            lookup_type
        >
{
    using Table = detailv12::sherwood_v12_table
    <
        T,
        T,
        H,
        detailv12::functor_storage<size_t, H>,
        E,
        detailv12::functor_storage<bool, E>,
        A,
        typename std::allocator_traits<A>::template rebind_alloc<detailv12::sherwood_v3_entry<T>>,
        lookup_type
    >;
public:

    using key_type = T;

    using Table::Table;
    agl_fib_hash_set()
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

    friend bool operator==(const agl_fib_hash_set & lhs, const agl_fib_hash_set & rhs)
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
    friend bool operator!=(const agl_fib_hash_set & lhs, const agl_fib_hash_set & rhs)
    {
        return !(lhs == rhs);
    }
};

} // end namespace ska
