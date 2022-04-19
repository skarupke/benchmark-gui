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
#include <emmintrin.h> // for SSE and SSE2
#include <x86intrin.h> // for _bit_scan_forward
#include "flat_hash_map.hpp"

#ifdef _MSC_VER
#define SKA_NOINLINE(...) __declspec(noinline) __VA_ARGS__
#else
#define SKA_NOINLINE(...) __VA_ARGS__ __attribute__((noinline))
#endif

namespace ska
{

namespace detailv6
{
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

inline uint8_t log2(size_t value)
{
    static constexpr uint8_t table[64] =
    {
        63,  0, 58,  1, 59, 47, 53,  2,
        60, 39, 48, 27, 54, 33, 42,  3,
        61, 51, 37, 40, 49, 18, 28, 20,
        55, 30, 34, 11, 43, 14, 22,  4,
        62, 57, 46, 52, 38, 26, 32, 41,
        50, 36, 17, 19, 29, 10, 13, 21,
        56, 45, 25, 31, 35, 16,  9, 12,
        44, 24, 15,  8, 23,  7,  6,  5
    };
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return table[((value - (value >> 1)) * 0x07EDD5E59A4E28C2) >> 58];
}

inline int equal_bits(__m128i a, __m128i b)
{
    __m128i equal = _mm_cmpeq_epi8(a, b);
    return _mm_movemask_epi8(equal);
}

using ska::detailv3::HashPolicySelector;

enum LookupType
{
    Default,
    ConstantArray,
    ZeroStart,
};

template<LookupType>
struct sherwood_v6_constants
{
    static __m128i sixteen_ones;
    static __m128i sixteen_distance_starts;
    static constexpr int8_t magic_for_end_misaligned    = 0b10000010;
    static constexpr int8_t magic_for_end_aligned       = 0b10000001;
    static constexpr int8_t magic_for_empty             = 0b10000000;
    static constexpr int8_t distance_start_pattern      = 0b10100000;
    static constexpr int8_t bit_for_distance            = 0b00100000;
    static constexpr int8_t extra_bits_mask             = 0b00011111;
    static constexpr int num_extra_bits                 = 5;
    inline static int distance_from_metadata(int8_t metadata)
    {
        return (metadata - distance_start_pattern) >> 5;
    }
};
template<>
struct sherwood_v6_constants<ConstantArray>
{
    static __m128i sixteen_ones;
    static __m128i sixteen_distance_starts;
    static __m128i lookup_starts[32];
    static constexpr int8_t magic_for_end_misaligned    = 0b10000010;
    static constexpr int8_t magic_for_end_aligned       = 0b10000001;
    static constexpr int8_t magic_for_empty             = 0b10000000;
    static constexpr int8_t distance_start_pattern      = 0b10100000;
    static constexpr int8_t bit_for_distance            = 0b00100000;
    static constexpr int8_t extra_bits_mask             = 0b00011111;
    static constexpr int num_extra_bits                 = 5;
    inline static int distance_from_metadata(int8_t metadata)
    {
        return (metadata - distance_start_pattern) >> 5;
    }
};
template<>
struct sherwood_v6_constants<ZeroStart>
{
    static __m128i sixteen_ones;
    static __m128i sixteen_distance_starts;
    static constexpr int8_t magic_for_end_misaligned    = 0b10000010;
    static constexpr int8_t magic_for_end_aligned       = 0b10000001;
    static constexpr int8_t magic_for_empty             = 0b10000000;
    static constexpr int8_t distance_start_pattern      = 0b00000000;
    static constexpr int8_t bit_for_distance            = 0b00100000;
    static constexpr int8_t extra_bits_mask             = 0b00011111;
    static constexpr int num_extra_bits                 = 5;
    inline static int distance_from_metadata(int8_t metadata)
    {
        return metadata >> 5;
    }
};
template<LookupType L>
__m128i sherwood_v6_constants<L>::sixteen_ones;
template<LookupType L>
__m128i sherwood_v6_constants<L>::sixteen_distance_starts;
//__m128i sherwood_v6_constants<ConstantArray>::lookup_starts[32];

template<typename T, LookupType lookup_type>
struct sherwood_v6_block
{
    sherwood_v6_block()
    {
    }
    ~sherwood_v6_block()
    {
    }

    union
    {
        __m128i control_bytes;
        int8_t control_bytes_as_ints[16];
    };
    union
    {
        T data[16];
    };

    static sherwood_v6_block * empty_block()
    {
        static sherwood_v6_block * empty_block = []
        {
            sherwood_v6_constants<lookup_type>::sixteen_ones = _mm_set1_epi8(sherwood_v6_constants<lookup_type>::bit_for_distance);
            sherwood_v6_constants<lookup_type>::sixteen_distance_starts = _mm_set1_epi8(sherwood_v6_constants<lookup_type>::distance_start_pattern);
            if constexpr (lookup_type == ConstantArray)
            {
                int count = 0;
                for (__m128i & m : sherwood_v6_constants<ConstantArray>::lookup_starts)
                {
                    m = _mm_or_si128(sherwood_v6_constants<ConstantArray>::sixteen_distance_starts, _mm_set1_epi8(count++));
                }
            }
            static sherwood_v6_block result[2];
            result[0].control_bytes = result[1].control_bytes = _mm_set1_epi8(sherwood_v6_constants<lookup_type>::magic_for_end_aligned);
            return result;
        }();
        return empty_block;
    }

    int first_empty_index() const
    {
        int empty_indices = _mm_movemask_epi8(_mm_cmpeq_epi8(control_bytes, _mm_set1_epi8(sherwood_v6_constants<lookup_type>::magic_for_empty)));
        if (empty_indices)
            return _bit_scan_forward(empty_indices);
        else
            return -1;
    }
    int index_with_smallest_distance(int8_t to_compare) const
    {
        int lowest_index = -1;
        int smallest_distance = sherwood_v6_constants<lookup_type>::distance_from_metadata(to_compare);
        for (int i = 0; i < 16; ++i)
        {
            int distance = sherwood_v6_constants<lookup_type>::distance_from_metadata(control_bytes_as_ints[i]);
            if (distance < smallest_distance)
            {
                smallest_distance = distance;
                lowest_index = i;
            }
        }
        return lowest_index;
    }
    int index_with_largest_distance() const
    {
        int index = -1;
        int largest_distance = 0;
        for (int i = 0; i < 16; ++i)
        {
            int distance = sherwood_v6_constants<lookup_type>::distance_from_metadata(control_bytes_as_ints[i]);
            if (distance > largest_distance)
            {
                largest_distance = distance;
                index = i;
            }
        }
        return index;
    }
};

template<typename T, typename FindKey, typename ArgumentHash, typename Hasher, typename ArgumentEqual, typename Equal, typename ArgumentAlloc, typename ByteAlloc, LookupType lookup_type>
class sherwood_v6_table : private ByteAlloc, private Hasher, private Equal
{
    using AllocatorTraits = std::allocator_traits<ByteAlloc>;
    using BlockPointer = sherwood_v6_block<T, lookup_type> *;
    using BytePointer = typename AllocatorTraits::pointer;
    struct convertible_to_iterator;

public:

    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = ArgumentHash;
    using key_equal = ArgumentEqual;
    using allocator_type = ByteAlloc;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    sherwood_v6_table()
    {
    }
    explicit sherwood_v6_table(size_type bucket_count, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : ByteAlloc(alloc), Hasher(hash), Equal(equal)
    {
        if (bucket_count)
            rehash(bucket_count);
    }
    sherwood_v6_table(size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v6_table(bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v6_table(size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v6_table(bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    explicit sherwood_v6_table(const ArgumentAlloc & alloc)
        : ByteAlloc(alloc)
    {
    }
    template<typename It>
    sherwood_v6_table(It first, It last, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v6_table(bucket_count, hash, equal, alloc)
    {
        insert(first, last);
    }
    template<typename It>
    sherwood_v6_table(It first, It last, size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v6_table(first, last, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    template<typename It>
    sherwood_v6_table(It first, It last, size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v6_table(first, last, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v6_table(std::initializer_list<T> il, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v6_table(bucket_count, hash, equal, alloc)
    {
        if (bucket_count == 0)
            rehash(il.size());
        insert(il.begin(), il.end());
    }
    sherwood_v6_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v6_table(il, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v6_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v6_table(il, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v6_table(const sherwood_v6_table & other)
        : sherwood_v6_table(other, AllocatorTraits::select_on_container_copy_construction(other.get_allocator()))
    {
    }
    sherwood_v6_table(const sherwood_v6_table & other, const ArgumentAlloc & alloc)
        : ByteAlloc(alloc), Hasher(other), Equal(other), _max_load_factor(other._max_load_factor)
    {
        rehash_for_other_container(other);
        try
        {
            insert(other.begin(), other.end());
        }
        catch(...)
        {
            clear();
            deallocate_data(entries, num_blocks_minus_one, max_lookups);
            throw;
        }
    }
    sherwood_v6_table(sherwood_v6_table && other) noexcept
        : ByteAlloc(std::move(other)), Hasher(std::move(other)), Equal(std::move(other))
        , _max_load_factor(other._max_load_factor)
    {
        swap_pointers(other);
    }
    sherwood_v6_table(sherwood_v6_table && other, const ArgumentAlloc & alloc) noexcept
        : ByteAlloc(alloc), Hasher(std::move(other)), Equal(std::move(other))
        , _max_load_factor(other._max_load_factor)
    {
        swap_pointers(other);
    }
    sherwood_v6_table & operator=(const sherwood_v6_table & other)
    {
        if (this == std::addressof(other))
            return *this;

        clear();
        if (AllocatorTraits::propagate_on_container_copy_assignment::value)
        {
            if (static_cast<ByteAlloc &>(*this) != static_cast<const ByteAlloc &>(other))
            {
                reset_to_empty_state();
            }
            AssignIfTrue<ByteAlloc, AllocatorTraits::propagate_on_container_copy_assignment::value>()(*this, other);
        }
        _max_load_factor = other._max_load_factor;
        static_cast<Hasher &>(*this) = other;
        static_cast<Equal &>(*this) = other;
        rehash_for_other_container(other);
        insert(other.begin(), other.end());
        return *this;
    }
    sherwood_v6_table & operator=(sherwood_v6_table && other) noexcept
    {
        if (this == std::addressof(other))
            return *this;
        else if (AllocatorTraits::propagate_on_container_move_assignment::value)
        {
            clear();
            reset_to_empty_state();
            AssignIfTrue<ByteAlloc, AllocatorTraits::propagate_on_container_move_assignment::value>()(*this, std::move(other));
            swap_pointers(other);
        }
        else if (static_cast<ByteAlloc &>(*this) == static_cast<ByteAlloc &>(other))
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
    ~sherwood_v6_table()
    {
        clear();
        deallocate_data(entries, num_blocks_minus_one, max_lookups);
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
        BlockPointer current = BlockPointer();
        int position_in_block = 0;

        templated_iterator()
        {
        }
        templated_iterator(BlockPointer entries, int position_in_block)
            : current(entries)
            , position_in_block(position_in_block)
        {
        }

        using iterator_category = std::forward_iterator_tag;
        using value_type = ValueType;
        using difference_type = ptrdiff_t;
        using pointer = ValueType *;
        using reference = ValueType &;

        friend bool operator==(const templated_iterator & lhs, const templated_iterator & rhs)
        {
            return lhs.current == rhs.current && lhs.position_in_block == rhs.position_in_block;
        }
        friend bool operator!=(const templated_iterator & lhs, const templated_iterator & rhs)
        {
            return !(lhs == rhs);
        }

        templated_iterator & operator++()
        {
            do
            {
                ++position_in_block;
                if (position_in_block == 16)
                {
                    position_in_block = 0;
                    ++current;
                }
            }
            while(current->control_bytes_as_ints[position_in_block] == sherwood_v6_constants<lookup_type>::magic_for_empty);
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
            return current->data[position_in_block];
        }
        ValueType * operator->() const
        {
            return current->data + position_in_block;
        }

        operator templated_iterator<const value_type>() const
        {
            return { current, position_in_block };
        }
    };
    using iterator = templated_iterator<value_type>;
    using const_iterator = templated_iterator<const value_type>;

    iterator begin()
    {
        if (entries[0].control_bytes_as_ints[0] == sherwood_v6_constants<lookup_type>::magic_for_empty)
            return ++iterator{entries, 0};
        else
            return iterator{entries, 0};
    }
    const_iterator begin() const
    {
        if (entries[0].control_bytes_as_ints[0] == sherwood_v6_constants<lookup_type>::magic_for_empty)
            return ++const_iterator{entries, 0};
        else
            return const_iterator{entries, 0};
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    iterator end()
    {
        return { entries + num_blocks_minus_one + max_lookups, 0 };
    }
    const_iterator end() const
    {
        return { entries + num_blocks_minus_one + max_lookups, 0 };
    }
    const_iterator cend() const
    {
        return end();
    }

    inline iterator find(const FindKey & key)
    {
        size_t hash = hash_object(key);
        static constexpr int num_extra_bits = sherwood_v6_constants<lookup_type>::num_extra_bits;
        size_t index = hash_policy.template index_for_hash<num_extra_bits>(hash, num_blocks_minus_one);
        __m128i compare;
        if constexpr (lookup_type == Default)
        {
            compare = _mm_or_si128(sherwood_v6_constants<lookup_type>::sixteen_distance_starts, _mm_set1_epi8(hash_policy.template extra_bits_for_hash<num_extra_bits>(hash)));
        }
        else if constexpr (lookup_type == ConstantArray)
        {
            compare = sherwood_v6_constants<lookup_type>::lookup_starts[hash_policy.template extra_bits_for_hash<num_extra_bits>(hash)];
        }
        else if constexpr (lookup_type == ZeroStart)
        {
            compare = _mm_set1_epi8(hash_policy.template extra_bits_for_hash<num_extra_bits>(hash));
        }
        __m128i compare_distance = sherwood_v6_constants<lookup_type>::sixteen_distance_starts;
        __m128i sixteen_ones = sherwood_v6_constants<lookup_type>::sixteen_ones;
        for (BlockPointer lookup = entries + index;; ++lookup)
        {
            int which_are_equal = equal_bits(compare, lookup->control_bytes);
            while (which_are_equal)
            {
                int offset = _bit_scan_forward(which_are_equal);
                if (compares_equal(key, lookup->data[offset]))
                    return { lookup, offset };
                which_are_equal &= 0xfffffffe << offset;
            }
            if (_mm_movemask_epi8(_mm_cmplt_epi8(lookup->control_bytes, compare_distance)))
                return end();
            compare = _mm_add_epi8(compare, sixteen_ones);
            compare_distance = _mm_add_epi8(compare_distance, sixteen_ones);
        }
    }
    const_iterator find(const FindKey & key) const
    {
        return const_cast<sherwood_v6_table *>(this)->find(key);
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
        size_t hash = hash_object(key);
        static constexpr int num_extra_bits = sherwood_v6_constants<lookup_type>::num_extra_bits;
        size_t index = hash_policy.template index_for_hash<num_extra_bits>(hash, num_blocks_minus_one);
        __m128i compare = _mm_or_si128(sherwood_v6_constants<lookup_type>::sixteen_distance_starts, _mm_set1_epi8(hash_policy.extra_bits_for_hash<num_extra_bits>(hash)));
        __m128i compare_distance = sherwood_v6_constants<lookup_type>::sixteen_distance_starts;
        for (BlockPointer lookup = entries + index;; ++lookup)
        {
            int which_are_equal = equal_bits(compare, lookup->control_bytes);
            while (which_are_equal)
            {
                int offset = _bit_scan_forward(which_are_equal);
                ++result;
                if (compares_equal(key, lookup->data[offset]))
                    return result;
                which_are_equal &= 0xfffffffe << offset;
            }
            if (_mm_movemask_epi8(_mm_cmplt_epi8(lookup->control_bytes, compare_distance)))
                return result;
            compare = _mm_add_epi8(compare, sherwood_v6_constants<lookup_type>::sixteen_ones);
            compare_distance = _mm_add_epi8(compare_distance, sherwood_v6_constants<lookup_type>::sixteen_ones);
        }
    }


    template<typename Key, typename... Args>
    inline std::pair<iterator, bool> emplace(Key && key, Args &&... args)
    {
        size_t hash = hash_object(key);
        static constexpr int num_extra_bits = sherwood_v6_constants<lookup_type>::num_extra_bits;
        size_t index = hash_policy.template index_for_hash<num_extra_bits>(hash, num_blocks_minus_one);
        uint8_t extra_bits = hash_policy.template extra_bits_for_hash<num_extra_bits>(hash);
        __m128i compare = _mm_or_si128(sherwood_v6_constants<lookup_type>::sixteen_distance_starts, _mm_set1_epi8(extra_bits));
        __m128i compare_distance = sherwood_v6_constants<lookup_type>::sixteen_distance_starts;
        BlockPointer lookup = entries + index;
        BlockPointer initial = lookup;
        for (;; ++lookup)
        {
            int which_are_equal = equal_bits(compare, lookup->control_bytes);
            while (which_are_equal)
            {
                int offset = _bit_scan_forward(which_are_equal);
                if (compares_equal(key, lookup->data[offset]))
                    return { { lookup, offset }, false };
                which_are_equal &= 0xfffffffe << offset;
            }
            if (_mm_movemask_epi8(_mm_cmplt_epi8(lookup->control_bytes, compare_distance)))
                break;
            compare = _mm_add_epi8(compare, sherwood_v6_constants<lookup_type>::sixteen_ones);
            compare_distance = _mm_add_epi8(compare_distance, sherwood_v6_constants<lookup_type>::sixteen_ones);
        }
        return emplace_new_key(initial, lookup, extra_bits, std::forward<Key>(key), std::forward<Args>(args)...);
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

    void rehash(size_t num_items)
    {
        num_items = std::max(num_items, static_cast<size_t>(std::ceil(num_elements / static_cast<double>(_max_load_factor))));
        if (num_items == 0)
        {
            reset_to_empty_state();
            return;
        }
        size_t num_blocks = num_items / 16;
        if (num_items % 16)
            ++num_blocks;
        auto new_prime_index = hash_policy.next_size_over(num_blocks);
        if (max_lookups && num_blocks == num_blocks_minus_one + 1)
            return;
        uint8_t new_max_lookups = calculate_max_lookups(num_blocks);
        size_t memory_requirement = calculate_memory_requirement(num_blocks, new_max_lookups);
        unsigned char * new_memory = &*AllocatorTraits::allocate(*this, memory_requirement);
        bool is_misaligned = (reinterpret_cast<size_t>(new_memory) % 16) != 0;
        if (is_misaligned)
            new_memory += 8;

        BlockPointer new_buckets = reinterpret_cast<BlockPointer>(new_memory);

        BlockPointer special_end_item = new_buckets + num_blocks + new_max_lookups - 1;
        for (BlockPointer it = new_buckets; it != special_end_item; ++it)
            it->control_bytes = _mm_set1_epi8(sherwood_v6_constants<lookup_type>::magic_for_empty);
        if (is_misaligned)
            special_end_item->control_bytes = _mm_set1_epi8(sherwood_v6_constants<lookup_type>::magic_for_end_misaligned);
        else
            special_end_item->control_bytes = _mm_set1_epi8(sherwood_v6_constants<lookup_type>::magic_for_end_aligned);
        using std::swap;
        swap(entries, new_buckets);
        swap(num_blocks_minus_one, num_blocks);
        --num_blocks_minus_one;
        hash_policy.commit(new_prime_index);
        swap(max_lookups, new_max_lookups);
        num_elements = 0;
        for (BlockPointer it = new_buckets, end = new_buckets + num_blocks + new_max_lookups; it != end; ++it)
        {
            for (int i = 0; i < 16; ++i)
            {
                if (it->control_bytes_as_ints[i] != sherwood_v6_constants<lookup_type>::magic_for_empty)
                {
                    emplace(std::move(it->data[i]));
                    AllocatorTraits::destroy(*this, it->data + i);
                }
            }
        }
        deallocate_data(new_buckets, num_blocks, new_max_lookups);
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
        BlockPointer current = to_erase.current;
        AllocatorTraits::destroy(*this, current->data + to_erase.position_in_block);
        current->control_bytes_as_ints[to_erase.position_in_block] = sherwood_v6_constants<lookup_type>::magic_for_empty;
        --num_elements;
        BlockPointer next = current + 1;
        int old_index = to_erase.position_in_block;
        for (int mover_index = next->index_with_largest_distance(); mover_index != -1; mover_index = next->index_with_largest_distance())
        {
            AllocatorTraits::construct(*this, current->data + old_index, std::move(next->data[mover_index]));
            current->control_bytes_as_ints[old_index] = next->control_bytes_as_ints[mover_index] - sherwood_v6_constants<lookup_type>::bit_for_distance;
            AllocatorTraits::destroy(*this, next->data + mover_index);
            next->control_bytes_as_ints[mover_index] = sherwood_v6_constants<lookup_type>::magic_for_empty;
            current = next;
            old_index = mover_index;
            ++next;
        }
        return { to_erase.current, to_erase.position_in_block };
    }

    convertible_to_iterator erase(const_iterator begin_it, const_iterator end_it)
    {
        if (begin_it == end_it)
            return { begin_it.current, begin_it.position_in_block };
        for (const_iterator it = begin_it; it != end_it; ++it)
        {
            AllocatorTraits::destroy(*this, std::addressof(*it));
            it.current->control_bytes_as_ints[it.position_in_block] = sherwood_v6_constants<lookup_type>::magic_for_empty;
            --num_elements;
        }
        auto last_end = end();
        if (end_it == last_end)
            return { end_it.current, end_it.position_in_block };
        bool moved_any = end_it.position_in_block != 0;
        end_it.position_in_block = 0;
        for (;;)
        {
            for (int mover_index = end_it.current->index_with_largest_distance(); mover_index != -1; mover_index = end_it.current->index_with_largest_distance())
            {
                if (!emplace(std::move(end_it.current->data[mover_index])).second)
                    break;
                moved_any = true;
                --num_elements;
                AllocatorTraits::destroy(*this, end_it.current->data + mover_index);
                end_it.current->control_bytes_as_ints[mover_index] = sherwood_v6_constants<lookup_type>::magic_for_empty;
            }

            ++end_it.current;
            if (end_it == last_end || !moved_any)
                break;
            moved_any = false;
        }
        return { begin_it.current, begin_it.position_in_block };
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
        for (BlockPointer it = entries, end = it + num_blocks_minus_one + max_lookups; it != end; ++it)
        {
            for (int i = 0; i < 16; ++i)
            {
                if (it->control_bytes_as_ints[i] != sherwood_v6_constants<lookup_type>::magic_for_empty)
                {
                    AllocatorTraits::destroy(*this, std::addressof(it->data[i]));
                    it->control_bytes_as_ints[i] = sherwood_v6_constants<lookup_type>::magic_for_empty;
                }
            }
        }
        num_elements = 0;
    }

    void shrink_to_fit()
    {
        rehash_for_other_container(*this);
    }

    void swap(sherwood_v6_table & other)
    {
        using std::swap;
        swap_pointers(other);
        swap(static_cast<ArgumentHash &>(*this), static_cast<ArgumentHash &>(other));
        swap(static_cast<ArgumentEqual &>(*this), static_cast<ArgumentEqual &>(other));
        if (AllocatorTraits::propagate_on_container_swap::value)
            swap(static_cast<ByteAlloc &>(*this), static_cast<ByteAlloc &>(other));
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
        if (max_lookups)
            return 16 * (num_blocks_minus_one + 1);
        else
            return 0;
    }
    size_type max_bucket_count() const
    {
        return (AllocatorTraits::max_size(*this)) / sizeof(T);
    }
    size_t bucket(const FindKey & key) const
    {
        return hash_policy.template index_for_hash<sherwood_v6_constants<lookup_type>::num_extra_bits>(hash_object(key), num_blocks_minus_one);
    }
    float load_factor() const
    {
        return load_factor_for_num_items(num_elements);
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
    BlockPointer entries = sherwood_v6_block<T, lookup_type>::empty_block();
    size_t num_blocks_minus_one = 0;
    typename HashPolicySelector<ArgumentHash>::type hash_policy;
    uint8_t max_lookups = 0;
    float _max_load_factor = 0.9375f;
    size_t num_elements = 0;

    size_t num_buckets_for_reserve(size_t num_elements) const
    {
        return static_cast<size_t>(std::ceil(num_elements / static_cast<double>(_max_load_factor)));
    }
    void rehash_for_other_container(const sherwood_v6_table & other)
    {
        rehash(std::min(num_buckets_for_reserve(other.size()), other.bucket_count()));
    }

    static uint8_t calculate_max_lookups(size_t num_blocks)
    {
        if constexpr (lookup_type == ZeroStart)
        {
            return std::max(1, std::min(3, 1 + log2(num_blocks) / 4));
        }
        else
        {
            // todo: it should be possible to raise the max to 6
            return std::max(1, std::min(4, 1 + log2(num_blocks) / 4));
        }
    }
    float load_factor_for_num_items(size_t num_items) const
    {
        if (!max_lookups)
            return num_items ? std::numeric_limits<float>::infinity() : 0.0f;
        size_t buckets = num_blocks_minus_one + 1;
        buckets *= 16;
        return static_cast<double>(num_items) / buckets;
    }

    void swap_pointers(sherwood_v6_table & other)
    {
        using std::swap;
        swap(hash_policy, other.hash_policy);
        swap(entries, other.entries);
        swap(max_lookups, other.max_lookups);
        swap(num_blocks_minus_one, other.num_blocks_minus_one);
        swap(num_elements, other.num_elements);
        swap(_max_load_factor, other._max_load_factor);
    }

    template<typename Key, typename... Args>
    SKA_NOINLINE(std::pair<iterator, bool>) emplace_new_key(BlockPointer initial, BlockPointer current, uint8_t extra_bits, Key && key, Args &&... args)
    {
        using std::swap;
        if (load_factor_for_num_items(num_elements + 1) > _max_load_factor)
        {
            grow();
            return emplace(std::forward<Key>(key), std::forward<Args>(args)...);
        }
        int distance_from_desired = current - initial;
        int8_t new_metadata = sherwood_v6_constants<lookup_type>::distance_start_pattern;
        new_metadata |= extra_bits;
        new_metadata += distance_from_desired * sherwood_v6_constants<lookup_type>::bit_for_distance;
        int lowest_index = -1;
        for (;; ++current, ++distance_from_desired, new_metadata += sherwood_v6_constants<lookup_type>::bit_for_distance)
        {
            if (distance_from_desired == max_lookups)
            {
                grow();
                return emplace(std::forward<Key>(key), std::forward<Args>(args)...);
            }
            int first_empty_index = current->first_empty_index();
            if (first_empty_index != -1)
            {
                AllocatorTraits::construct(*this, std::addressof(current->data[first_empty_index]), std::forward<Key>(key), std::forward<Args>(args)...);
                current->control_bytes_as_ints[first_empty_index] = new_metadata;
                ++num_elements;
                return { { current, first_empty_index }, true };
            }
            lowest_index = current->index_with_smallest_distance(new_metadata);
            if (lowest_index != -1)
                break;
        }
        value_type to_insert(std::forward<Key>(key), std::forward<Args>(args)...);
        swap(to_insert, current->data[lowest_index]);
        swap(new_metadata, current->control_bytes_as_ints[lowest_index]);
        new_metadata += sherwood_v6_constants<lookup_type>::bit_for_distance;
        distance_from_desired = sherwood_v6_constants<lookup_type>::distance_from_metadata(new_metadata);
        iterator result = { current, lowest_index };
        for (++current;; ++current)
        {
            int first_empty_index = current->first_empty_index();
            if (first_empty_index != -1)
            {
                AllocatorTraits::construct(*this, current->data + first_empty_index, std::move(to_insert));
                current->control_bytes_as_ints[first_empty_index] = new_metadata;
                ++num_elements;
                return { result, true };
            }
            lowest_index = current->index_with_smallest_distance(new_metadata);
            if (lowest_index != -1)
            {
                swap(to_insert, current->data[lowest_index]);
                swap(new_metadata, current->control_bytes_as_ints[lowest_index]);
                new_metadata += sherwood_v6_constants<lookup_type>::bit_for_distance;
                distance_from_desired = sherwood_v6_constants<lookup_type>::distance_from_metadata(new_metadata);
            }
            else
            {
                ++distance_from_desired;
                if (distance_from_desired == max_lookups)
                {
                    swap(to_insert, *result);
                    grow();
                    return emplace(std::move(to_insert));
                }
                new_metadata += sherwood_v6_constants<lookup_type>::bit_for_distance;
            }
        }
    }

    void grow()
    {
        rehash(std::max(size_t(4), 2 * bucket_count()));
    }

    size_t calculate_memory_requirement(size_t num_blocks, uint8_t max_lookups)
    {
        size_t memory_required = sizeof(sherwood_v6_block<T, lookup_type>) * (num_blocks + max_lookups - 1);
        memory_required += 16; // for metadata of past-the-end pointer
        memory_required += 8; // to fix alignment should the allocator return misaligned memory
        return memory_required;

    }

    void deallocate_data(BlockPointer begin, size_t num_blocks_minus_one, uint8_t max_lookups)
    {
        if (begin == sherwood_v6_block<T, lookup_type>::empty_block())
            return;

        size_t memory = calculate_memory_requirement(num_blocks_minus_one + 1, max_lookups);
        BlockPointer end = begin + num_blocks_minus_one + max_lookups;
        unsigned char * as_byte_pointer = reinterpret_cast<unsigned char *>(begin);
        if (end->control_bytes_as_ints[0] == sherwood_v6_constants<lookup_type>::magic_for_end_misaligned)
            as_byte_pointer -= 8;
        AllocatorTraits::deallocate(*this, typename AllocatorTraits::pointer(as_byte_pointer), memory);
    }

    void reset_to_empty_state()
    {
        deallocate_data(entries, num_blocks_minus_one, max_lookups);
        entries = sherwood_v6_block<T, lookup_type>::empty_block();
        num_blocks_minus_one = 0;
        hash_policy.reset();
        max_lookups = 0;
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
        BlockPointer it;
        int position_in_block;

        operator iterator()
        {
            if (it->control_bytes_as_ints[position_in_block] == sherwood_v6_constants<lookup_type>::magic_for_empty)
                return ++iterator{it, position_in_block};
            else
                return { it, position_in_block };
        }
        operator const_iterator()
        {
            if (it->control_bytes_as_ints[position_in_block] == sherwood_v6_constants<lookup_type>::magic_for_empty)
                return ++iterator{it, position_in_block};
            else
                return { it, position_in_block };
        }
    };
};
}

template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<K, V> >, detailv6::LookupType lookup_type = detailv6::Default>
class block_hash_map
        : public detailv6::sherwood_v6_table
        <
            std::pair<K, V>,
            K,
            H,
            detailv6::KeyOrValueHasher<K, std::pair<K, V>, H>,
            E,
            detailv6::KeyOrValueEquality<K, std::pair<K, V>, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
            lookup_type
        >
{
    using Table = detailv6::sherwood_v6_table
    <
        std::pair<K, V>,
        K,
        H,
        detailv6::KeyOrValueHasher<K, std::pair<K, V>, H>,
        E,
        detailv6::KeyOrValueEquality<K, std::pair<K, V>, E>,
        A,
        typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
        lookup_type
    >;
public:

    using key_type = K;
    using mapped_type = V;

    using Table::Table;
    block_hash_map()
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

    friend bool operator==(const block_hash_map & lhs, const block_hash_map & rhs)
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
    friend bool operator!=(const block_hash_map & lhs, const block_hash_map & rhs)
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

template<typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename A = std::allocator<T>, detailv6::LookupType lookup_type = detailv6::Default>
class block_hash_set
        : public detailv6::sherwood_v6_table
        <
            T,
            T,
            H,
            detailv6::functor_storage<size_t, H>,
            E,
            detailv6::functor_storage<bool, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
            lookup_type
        >
{
    using Table = detailv6::sherwood_v6_table
    <
        T,
        T,
        H,
        detailv6::functor_storage<size_t, H>,
        E,
        detailv6::functor_storage<bool, E>,
        A,
        typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
        lookup_type
    >;
public:

    using key_type = T;

    using Table::Table;
    block_hash_set()
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

    friend bool operator==(const block_hash_set & lhs, const block_hash_set & rhs)
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
    friend bool operator!=(const block_hash_set & lhs, const block_hash_set & rhs)
    {
        return !(lhs == rhs);
    }
};

} // end namespace ska
