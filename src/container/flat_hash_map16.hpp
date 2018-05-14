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

namespace detailv5
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

template<typename = void>
struct global_lookup_compare
{
    static __m128i sixteen_lookup[8];
    static __m128i thirty_two_lookup[8];
};
template<typename T>
__m128i global_lookup_compare<T>::sixteen_lookup[8];
template<typename T>
__m128i global_lookup_compare<T>::thirty_two_lookup[8];

struct sherwood_v5_metadata
{
    uint8_t as_byte = 255;

    static constexpr int num_extra_bits = 3;

    sherwood_v5_metadata()
    {
    }
    sherwood_v5_metadata(uint8_t extra_bits, uint8_t distance_from_desired)
        : as_byte((extra_bits << 5) | distance_from_desired)
    {
    }

    bool has_value() const
    {
        return as_byte != 255;
    }
    bool is_empty() const
    {
        return as_byte == 255;
    }
    uint8_t get_distance_from_desired() const
    {
        return as_byte & 0b11111;
    }
    bool is_at_desired_position() const
    {
        return get_distance_from_desired() == 0;
    }

    void clear()
    {
        as_byte = 255;
    }

    static sherwood_v5_metadata * empty_array_ptr()
    {
        static sherwood_v5_metadata * result = [&]
        {
            global_lookup_compare<>::sixteen_lookup[0] = _mm_set_epi8(char(15), char(14), char(13), char(12), char(11), char(10), char(9), char(8), char(7), char(6), char(5), char(4), char(3), char(2), char(1), char(0));
            global_lookup_compare<>::sixteen_lookup[1] = _mm_set_epi8(char(47), char(46), char(45), char(44), char(43), char(42), char(41), char(40), char(39), char(38), char(37), char(36), char(35), char(34), char(33), char(32));
            global_lookup_compare<>::sixteen_lookup[2] = _mm_set_epi8(char(79), char(78), char(77), char(76), char(75), char(74), char(73), char(72), char(71), char(70), char(69), char(68), char(67), char(66), char(65), char(64));
            global_lookup_compare<>::sixteen_lookup[3] = _mm_set_epi8(char(111), char(110), char(109), char(108), char(107), char(106), char(105), char(104), char(103), char(102), char(101), char(100), char(99), char(98), char(97), char(96));
            global_lookup_compare<>::sixteen_lookup[4] = _mm_set_epi8(char(143), char(142), char(141), char(140), char(139), char(138), char(137), char(136), char(135), char(134), char(133), char(132), char(131), char(130), char(129), char(128));
            global_lookup_compare<>::sixteen_lookup[5] = _mm_set_epi8(char(175), char(174), char(173), char(172), char(171), char(170), char(169), char(168), char(167), char(166), char(165), char(164), char(163), char(162), char(161), char(160));
            global_lookup_compare<>::sixteen_lookup[6] = _mm_set_epi8(char(207), char(206), char(205), char(204), char(203), char(202), char(201), char(200), char(199), char(198), char(197), char(196), char(195), char(194), char(193), char(192));
            global_lookup_compare<>::sixteen_lookup[7] = _mm_set_epi8(char(239), char(238), char(237), char(236), char(235), char(234), char(233), char(232), char(231), char(230), char(229), char(228), char(227), char(226), char(225), char(224));
            global_lookup_compare<>::thirty_two_lookup[0] = _mm_set_epi8(char(30), char(30), char(29), char(28), char(27), char(26), char(25), char(24), char(23), char(22), char(21), char(20), char(19), char(18), char(17), char(16));
            global_lookup_compare<>::thirty_two_lookup[1] = _mm_set_epi8(char(62), char(62), char(61), char(60), char(59), char(58), char(57), char(56), char(55), char(54), char(53), char(52), char(51), char(50), char(49), char(48));
            global_lookup_compare<>::thirty_two_lookup[2] = _mm_set_epi8(char(94), char(94), char(93), char(92), char(91), char(90), char(89), char(88), char(87), char(86), char(85), char(84), char(83), char(82), char(81), char(80));
            global_lookup_compare<>::thirty_two_lookup[3] = _mm_set_epi8(char(126), char(126), char(125), char(124), char(123), char(122), char(121), char(120), char(119), char(118), char(117), char(116), char(115), char(114), char(113), char(112));
            global_lookup_compare<>::thirty_two_lookup[4] = _mm_set_epi8(char(158), char(158), char(157), char(156), char(155), char(154), char(153), char(152), char(151), char(150), char(149), char(148), char(147), char(146), char(145), char(144));
            global_lookup_compare<>::thirty_two_lookup[5] = _mm_set_epi8(char(190), char(190), char(189), char(188), char(187), char(186), char(185), char(184), char(183), char(182), char(181), char(180), char(179), char(178), char(177), char(176));
            global_lookup_compare<>::thirty_two_lookup[6] = _mm_set_epi8(char(222), char(222), char(221), char(220), char(219), char(218), char(217), char(216), char(215), char(214), char(213), char(212), char(211), char(210), char(209), char(208));
            global_lookup_compare<>::thirty_two_lookup[7] = _mm_set_epi8(char(254), char(254), char(253), char(252), char(251), char(250), char(249), char(248), char(247), char(246), char(245), char(244), char(243), char(242), char(241), char(240));
            static constexpr uint8_t empty_array[17] =
            {
                255, 255, 255, 255,
                255, 255, 255, 255,
                255, 255, 255, 255,
                255, 255, 255, 255, 0
            };
            return reinterpret_cast<sherwood_v5_metadata *>(const_cast<uint8_t *>(empty_array));
        }();
        return result;
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

inline int equal_bits(__m128i a, __m128i b)
{
    __m128i equal = _mm_cmpeq_epi8(a, b);
    return _mm_movemask_epi8(equal);
}

using ska::detailv3::HashPolicySelector;

enum Flat16HashMapLookupType
{
    Default,
    ZeroCheck,
    AlwaysTwoChecks
};

template<typename T, typename FindKey, typename ArgumentHash, typename Hasher, typename ArgumentEqual, typename Equal, typename ArgumentAlloc, typename ByteAlloc, Flat16HashMapLookupType lookup_type>
class sherwood_v5_table : private ByteAlloc, private Hasher, private Equal
{
    using AllocatorTraits = std::allocator_traits<ByteAlloc>;
    using EntryPointer = T *;
    using MetadataPointer = sherwood_v5_metadata *;
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

    sherwood_v5_table()
    {
    }
    explicit sherwood_v5_table(size_type bucket_count, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : ByteAlloc(alloc), Hasher(hash), Equal(equal)
    {
        if (bucket_count)
            rehash(bucket_count);
    }
    sherwood_v5_table(size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v5_table(bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v5_table(size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v5_table(bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    explicit sherwood_v5_table(const ArgumentAlloc & alloc)
        : ByteAlloc(alloc)
    {
    }
    template<typename It>
    sherwood_v5_table(It first, It last, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v5_table(bucket_count, hash, equal, alloc)
    {
        insert(first, last);
    }
    template<typename It>
    sherwood_v5_table(It first, It last, size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v5_table(first, last, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    template<typename It>
    sherwood_v5_table(It first, It last, size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v5_table(first, last, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v5_table(std::initializer_list<T> il, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v5_table(bucket_count, hash, equal, alloc)
    {
        if (bucket_count == 0)
            rehash(il.size());
        insert(il.begin(), il.end());
    }
    sherwood_v5_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v5_table(il, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v5_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v5_table(il, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v5_table(const sherwood_v5_table & other)
        : sherwood_v5_table(other, AllocatorTraits::select_on_container_copy_construction(other.get_allocator()))
    {
    }
    sherwood_v5_table(const sherwood_v5_table & other, const ArgumentAlloc & alloc)
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
            deallocate_data(entries, num_slots_minus_one, do_31_lookups);
            throw;
        }
    }
    sherwood_v5_table(sherwood_v5_table && other) noexcept
        : ByteAlloc(std::move(other)), Hasher(std::move(other)), Equal(std::move(other))
        , _max_load_factor(other._max_load_factor)
    {
        swap_pointers(other);
    }
    sherwood_v5_table(sherwood_v5_table && other, const ArgumentAlloc & alloc) noexcept
        : ByteAlloc(alloc), Hasher(std::move(other)), Equal(std::move(other))
        , _max_load_factor(other._max_load_factor)
    {
        swap_pointers(other);
    }
    sherwood_v5_table & operator=(const sherwood_v5_table & other)
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
    sherwood_v5_table & operator=(sherwood_v5_table && other) noexcept
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
    ~sherwood_v5_table()
    {
        clear();
        deallocate_data(entries, num_slots_minus_one, do_31_lookups);
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
        EntryPointer current = EntryPointer();
        MetadataPointer metadata = MetadataPointer();

        templated_iterator()
        {
        }
        templated_iterator(EntryPointer entries, MetadataPointer meta)
            : current(entries)
            , metadata(meta)
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
                ++metadata;
            }
            while(metadata->is_empty());
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
            return *current;
        }
        EntryPointer operator->() const
        {
            return current;
        }

        operator templated_iterator<const value_type>() const
        {
            return { current, metadata };
        }
    };
    using iterator = templated_iterator<value_type>;
    using const_iterator = templated_iterator<const value_type>;

    iterator begin()
    {
        EntryPointer it = entries;
        for (MetadataPointer metadata_it = metadata;; ++it, ++metadata_it)
        {
            if (metadata_it->has_value())
                return { it, metadata_it };
        }
    }
    const_iterator begin() const
    {
        EntryPointer it = entries;
        for (MetadataPointer metadata_it = metadata;; ++it, ++metadata_it)
        {
            if (metadata_it->has_value())
                return { it, metadata_it };
        }
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    iterator end()
    {
        std::ptrdiff_t to_add = static_cast<ptrdiff_t>(num_slots_minus_one + 1 + get_max_lookups(do_31_lookups));
        return { entries + to_add, metadata + to_add };
    }
    const_iterator end() const
    {
        std::ptrdiff_t to_add = static_cast<ptrdiff_t>(num_slots_minus_one + 1 + get_max_lookups(do_31_lookups));
        return { entries + to_add, metadata + to_add };
    }
    const_iterator cend() const
    {
        return end();
    }

    inline iterator find(const FindKey & key)
    {
        size_t hash = hash_object(key);
        static constexpr int num_extra_bits = sherwood_v5_metadata::num_extra_bits;
        size_t index = hash_policy.template index_for_hash<num_extra_bits>(hash, num_slots_minus_one);
        uint8_t extra_bits = hash_policy.template extra_bits_for_hash<num_extra_bits>(hash);
        MetadataPointer metadata_lookup = metadata + index;
        EntryPointer value_lookup = entries + index;
        if constexpr (lookup_type == ZeroCheck)
        {
            if (metadata_lookup->get_distance_from_desired() == 0 && compares_equal(key, *value_lookup))
                return { value_lookup, metadata_lookup };
        }
        __m128i compare_metadata = global_lookup_compare<>::sixteen_lookup[extra_bits];
        for (bool loop_once = do_31_lookups;; loop_once = false)
        {
            __m128i loaded_metadata = load_sixteen_metadatas(metadata_lookup);
            int which_are_equal = equal_bits(loaded_metadata, compare_metadata);
            while (which_are_equal)
            {
                int offset = _bit_scan_forward(which_are_equal);
                EntryPointer value_it = value_lookup + offset;
                if (compares_equal(key, *value_it))
                    return { value_it, metadata_lookup + offset };
                which_are_equal &= 0xfffffffe << offset;
            }
            if (!loop_once || some_are_empty(metadata_lookup))
                return end();

            metadata_lookup += 16;
            value_lookup += 16;
            compare_metadata = global_lookup_compare<>::thirty_two_lookup[extra_bits];
        }
    }
    const_iterator find(const FindKey & key) const
    {
        return const_cast<sherwood_v5_table *>(this)->find(key);
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
        static constexpr int num_extra_bits = sherwood_v5_metadata::num_extra_bits;
        size_t index = hash_policy.template index_for_hash<num_extra_bits>(hash, num_slots_minus_one);
        uint8_t extra_bits = hash_policy.template extra_bits_for_hash<num_extra_bits>(hash);
        MetadataPointer metadata_lookup = metadata + index;
        EntryPointer value_lookup = entries + index;
        __m128i compare_metadata = global_lookup_compare<>::sixteen_lookup[extra_bits];
        for (bool loop_once = do_31_lookups;; loop_once = false)
        {
            __m128i loaded_metadata = load_sixteen_metadatas(metadata_lookup);
            int which_are_equal = equal_bits(loaded_metadata, compare_metadata);
            while (which_are_equal)
            {
                int offset = _bit_scan_forward(which_are_equal);
                EntryPointer value_it = value_lookup + offset;
                ++result;
                if (compares_equal(key, *value_it))
                    return result;
                which_are_equal &= 0xfffffffe << offset;
            }
            if (!loop_once || some_are_empty(metadata_lookup))
                return result;

            metadata_lookup += 16;
            value_lookup += 16;
            compare_metadata = global_lookup_compare<>::thirty_two_lookup[extra_bits];
        }
    }

    template<typename Key, typename... Args>
    inline std::pair<iterator, bool> emplace(Key && key, Args &&... args)
    {
        size_t hash = hash_object(key);
        static constexpr int num_extra_bits = sherwood_v5_metadata::num_extra_bits;
        size_t index = hash_policy.template index_for_hash<num_extra_bits>(hash, num_slots_minus_one);
        uint8_t extra_bits = hash_policy.template extra_bits_for_hash<num_extra_bits>(hash);
        MetadataPointer metadata_lookup = metadata + index;
        MetadataPointer metadata_it = metadata_lookup;
        EntryPointer value_lookup = entries + index;
        EntryPointer value_it = value_lookup;
        __m128i compare_metadata = global_lookup_compare<>::sixteen_lookup[extra_bits];
        for (bool loop_once = do_31_lookups;; loop_once = false)
        {
            __m128i loaded_metadata = load_sixteen_metadatas(metadata_it);
            int which_are_equal = equal_bits(loaded_metadata, compare_metadata);
            // todo: change this loop to be more similar to the same loop in
            // the find() function
            for (; which_are_equal; ++value_it, ++metadata_it)
            {
                int bit = _bit_scan_forward(which_are_equal);
                value_it += bit;
                metadata_it += bit;
                if (compares_equal(key, *value_it))
                    return { { value_it, metadata_it }, false };
                which_are_equal >>= bit + 1;
            }
            if (!loop_once || some_are_empty(metadata_lookup))
                return emplace_new_key(metadata_lookup, metadata_it, value_it, extra_bits, std::forward<Key>(key), std::forward<Args>(args)...);

            metadata_it = metadata_lookup + 16;
            value_it = value_lookup + 16;
            compare_metadata = global_lookup_compare<>::thirty_two_lookup[extra_bits];
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
            reset_to_empty_state();
            return;
        }
        auto new_prime_index = hash_policy.next_size_over(num_buckets);
        if (num_buckets == bucket_count())
            return;
        bool new_31_lookups = should_do_31_lookups(num_buckets);
        uint8_t new_max_lookups = get_max_lookups(new_31_lookups);
        std::pair<size_t, size_t> memory_requirement = calculate_memory_requirement(num_buckets, new_max_lookups);
        unsigned char * new_memory = &*AllocatorTraits::allocate(*this, memory_requirement.first);

        EntryPointer new_buckets = reinterpret_cast<EntryPointer>(new_memory);
        MetadataPointer new_metadata = reinterpret_cast<MetadataPointer>(new_memory + memory_requirement.second);

        MetadataPointer special_end_item = new_metadata + num_buckets + new_max_lookups;
        for (MetadataPointer it = new_metadata; it != special_end_item; ++it)
        {
            it->clear();
        }
        special_end_item->as_byte = 0;
        using std::swap;
        swap(entries, new_buckets);
        swap(metadata, new_metadata);
        swap(num_slots_minus_one, num_buckets);
        --num_slots_minus_one;
        hash_policy.commit(new_prime_index);
        swap(do_31_lookups, new_31_lookups);
        num_elements = 0;
        uint8_t old_num_lookups = get_max_lookups(new_31_lookups);
        EntryPointer it = new_buckets;
        for (MetadataPointer metadata_it = new_metadata, end = metadata_it + num_buckets + old_num_lookups + 1; metadata_it != end; ++it, ++metadata_it)
        {
            if (metadata_it->has_value())
            {
                emplace(std::move(*it));
                AllocatorTraits::destroy(*this, std::addressof(*it));
            }
        }
        deallocate_data(new_buckets, num_buckets, new_31_lookups);
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
        EntryPointer current = to_erase.current;
        MetadataPointer current_metadata = to_erase.metadata;
        AllocatorTraits::destroy(*this, std::addressof(*current));
        current_metadata->clear();
        --num_elements;
        EntryPointer next = current + 1;
        MetadataPointer next_metadata = current_metadata + 1;
        for (; !next_metadata->is_empty() && !next_metadata->is_at_desired_position(); ++current, ++next, ++current_metadata, ++next_metadata)
        {
            *current_metadata = *next_metadata;
            --current_metadata->as_byte;
            AllocatorTraits::construct(*this, std::addressof(*current), std::move(*next));
            AllocatorTraits::destroy(*this, std::addressof(*next));
            next_metadata->clear();
        }
        return { to_erase.current, to_erase.metadata };
    }

    iterator erase(const_iterator begin_it, const_iterator end_it)
    {
        if (begin_it == end_it)
            return { begin_it.current, begin_it.metadata };
        EntryPointer entry_it = begin_it.current;
        for (MetadataPointer it = begin_it.metadata, end = end_it.metadata; it != end; ++it, ++entry_it)
        {
            if (it->has_value())
            {
                AllocatorTraits::destroy(*this, std::addressof(*entry_it));
                it->clear();
                --num_elements;
            }
        }
        if (end_it == this->end())
            return this->end();
        ptrdiff_t num_to_move = std::min(static_cast<ptrdiff_t>(end_it.metadata->get_distance_from_desired()), end_it.metadata - begin_it.metadata);
        EntryPointer to_return_value = end_it.current - num_to_move;
        MetadataPointer to_return_meta = end_it.metadata - num_to_move;
        EntryPointer value_it = end_it.current;
        for (MetadataPointer it = end_it.metadata; !it->is_empty() && !it->is_at_desired_position(); ++value_it)
        {
            EntryPointer target = value_it - num_to_move;
            MetadataPointer target_meta = it - num_to_move;
            *target_meta = *it;
            target_meta->as_byte -= num_to_move;
            AllocatorTraits::construct(*this, std::addressof(*target), std::move(*value_it));
            AllocatorTraits::destroy(*this, std::addressof(*value_it));
            it->clear();
            ++it;
            num_to_move = std::min(static_cast<ptrdiff_t>(it->get_distance_from_desired()), num_to_move);
        }
        return { to_return_value, to_return_meta };
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

        EntryPointer it = entries;
        for (MetadataPointer metadata_it = metadata, end_it = end().metadata; metadata_it != end_it; ++it, ++metadata_it)
        {
            if (metadata_it->has_value())
            {
                AllocatorTraits::destroy(*this, std::addressof(*it));
                metadata_it->clear();
            }
        }
        num_elements = 0;
    }

    void shrink_to_fit()
    {
        rehash_for_other_container(*this);
    }

    void swap(sherwood_v5_table & other)
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
        return num_slots_minus_one + 1;
    }
    size_type max_bucket_count() const
    {
        return (AllocatorTraits::max_size(*this)) / sizeof(T);
    }
    size_t bucket(const FindKey & key) const
    {
        return hash_policy.template index_for_hash<sherwood_v5_metadata::num_extra_bits>(hash_object(key), num_slots_minus_one);
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
    EntryPointer entries = nullptr;
    MetadataPointer metadata = sherwood_v5_metadata::empty_array_ptr();
    size_t num_slots_minus_one = 0;
    typename HashPolicySelector<ArgumentHash>::type hash_policy;
    bool do_31_lookups = false;
    float _max_load_factor = 0.875f;
    size_t num_elements = 0;

    inline static __m128i load_sixteen_metadatas(MetadataPointer meta)
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i *>(meta));
    }
    inline static bool some_are_empty(MetadataPointer meta)
    {
        sherwood_v5_metadata last_metadata = meta[15];
        return last_metadata.is_empty() || last_metadata.get_distance_from_desired() < 15;
    }

    static bool should_do_31_lookups(size_t num_buckets)
    {
        return lookup_type == AlwaysTwoChecks || num_buckets > 16384;
    }
    static uint8_t get_max_lookups(bool do_31_lookups)
    {
        return do_31_lookups ? 31 : 15;
    }

    size_t num_buckets_for_reserve(size_t num_elements) const
    {
        return static_cast<size_t>(std::ceil(num_elements / static_cast<double>(_max_load_factor)));
    }
    void rehash_for_other_container(const sherwood_v5_table & other)
    {
        rehash(std::min(num_buckets_for_reserve(other.size()), other.bucket_count()));
    }

    void swap_pointers(sherwood_v5_table & other)
    {
        using std::swap;
        swap(hash_policy, other.hash_policy);
        swap(entries, other.entries);
        swap(metadata, other.metadata);
        swap(num_slots_minus_one, other.num_slots_minus_one);
        swap(num_elements, other.num_elements);
        swap(do_31_lookups, other.do_31_lookups);
        swap(_max_load_factor, other._max_load_factor);
    }

    template<typename Key, typename... Args>
    SKA_NOINLINE(std::pair<iterator, bool>) emplace_new_key(MetadataPointer initial, MetadataPointer current, EntryPointer slot, uint8_t extra_bits, Key && key, Args &&... args)
    {
        using std::swap;
        if (num_slots_minus_one == 0 || static_cast<double>(num_elements + 1) / static_cast<double>(bucket_count()) > _max_load_factor)
        {
            grow();
            return emplace(std::forward<Key>(key), std::forward<Args>(args)...);
        }
        sherwood_v5_metadata to_insert_meta(extra_bits, static_cast<uint8_t>(current - initial));
        uint8_t max_lookups = get_max_lookups(do_31_lookups);
        for (;; ++current, ++slot, ++to_insert_meta.as_byte)
        {
            if (to_insert_meta.get_distance_from_desired() == max_lookups)
            {
                grow();
                return emplace(std::forward<Key>(key), std::forward<Args>(args)...);
            }
            else if (to_insert_meta.get_distance_from_desired() > current->get_distance_from_desired())
                break;
            else if (current->is_empty())
            {
                AllocatorTraits::construct(*this, std::addressof(*slot), std::forward<Key>(key), std::forward<Args>(args)...);
                *current = to_insert_meta;
                ++num_elements;
                return { { slot, current }, true };
            }
        }
        value_type to_insert(std::forward<Key>(key), std::forward<Args>(args)...);
        swap(to_insert, *slot);
        swap(to_insert_meta, *current);
        ++to_insert_meta.as_byte;
        iterator result = { slot, current };
        for (++current, ++slot;; ++current, ++slot)
        {
            if (current->is_empty())
            {
                AllocatorTraits::construct(*this, std::addressof(*slot), std::move(to_insert));
                *current = to_insert_meta;
                ++num_elements;
                return { result, true };
            }
            else if (current->get_distance_from_desired() < to_insert_meta.get_distance_from_desired())
            {
                swap(to_insert, *slot);
                swap(to_insert_meta, *current);
                ++to_insert_meta.as_byte;
            }
            else
            {
                ++to_insert_meta.as_byte;
                if (to_insert_meta.get_distance_from_desired() == max_lookups)
                {
                    swap(to_insert, *result.current);
                    grow();
                    return emplace(std::move(to_insert));
                }
            }
        }
    }

    void grow()
    {
        rehash(std::max(size_t(4), 2 * bucket_count()));
    }

    std::pair<size_t, size_t> calculate_memory_requirement(size_t num_buckets, uint8_t max_lookups)
    {
        size_t memory_required = sizeof(T) * (num_buckets + max_lookups);
        size_t metadata_offset = memory_required;
        memory_required += sizeof(sherwood_v5_metadata) * (num_buckets + max_lookups + 1);
        return { memory_required, metadata_offset };

    }

    void deallocate_data(EntryPointer begin, size_t num_slots_minus_one, bool do_31_lookups)
    {
        if (!begin)
            return;

        size_t memory = calculate_memory_requirement(num_slots_minus_one + 1, get_max_lookups(do_31_lookups)).first;
        AllocatorTraits::deallocate(*this, typename AllocatorTraits::pointer(reinterpret_cast<unsigned char *>(begin)), memory);
    }

    void reset_to_empty_state()
    {
        deallocate_data(entries, num_slots_minus_one, do_31_lookups);
        entries = nullptr;
        metadata = sherwood_v5_metadata::empty_array_ptr();
        num_slots_minus_one = 0;
        hash_policy.reset();
        do_31_lookups = false;
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
        EntryPointer it;
        MetadataPointer meta;

        operator iterator()
        {
            if (meta->has_value())
                return { it, meta };
            else
                return ++iterator{it, meta};
        }
        operator const_iterator()
        {
            if (meta->has_value())
                return { it, meta };
            else
                return ++iterator{it, meta};
        }
    };
};
}

template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<K, V> >, ska::detailv5::Flat16HashMapLookupType lookup_type = ska::detailv5::Default >
class flat16_hash_map
        : public detailv5::sherwood_v5_table
        <
            std::pair<K, V>,
            K,
            H,
            detailv5::KeyOrValueHasher<K, std::pair<K, V>, H>,
            E,
            detailv5::KeyOrValueEquality<K, std::pair<K, V>, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
            lookup_type
        >
{
    using Table = detailv5::sherwood_v5_table
    <
        std::pair<K, V>,
        K,
        H,
        detailv5::KeyOrValueHasher<K, std::pair<K, V>, H>,
        E,
        detailv5::KeyOrValueEquality<K, std::pair<K, V>, E>,
        A,
        typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
        lookup_type
    >;
public:

    using key_type = K;
    using mapped_type = V;

    using Table::Table;
    flat16_hash_map()
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

    friend bool operator==(const flat16_hash_map & lhs, const flat16_hash_map & rhs)
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
    friend bool operator!=(const flat16_hash_map & lhs, const flat16_hash_map & rhs)
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

template<typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename A = std::allocator<T>, ska::detailv5::Flat16HashMapLookupType lookup_type = ska::detailv5::Default >
class flat16_hash_set
        : public detailv5::sherwood_v5_table
        <
            T,
            T,
            H,
            detailv5::functor_storage<size_t, H>,
            E,
            detailv5::functor_storage<bool, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
            lookup_type
        >
{
    using Table = detailv5::sherwood_v5_table
    <
        T,
        T,
        H,
        detailv5::functor_storage<size_t, H>,
        E,
        detailv5::functor_storage<bool, E>,
        A,
        typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
        lookup_type
    >;
public:

    using key_type = T;

    using Table::Table;
    flat16_hash_set()
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

    friend bool operator==(const flat16_hash_set & lhs, const flat16_hash_set & rhs)
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
    friend bool operator!=(const flat16_hash_set & lhs, const flat16_hash_set & rhs)
    {
        return !(lhs == rhs);
    }
};

} // end namespace ska
