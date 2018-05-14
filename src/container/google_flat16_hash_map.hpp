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

namespace detailv12
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

inline int equal_bits(__m128i a, __m128i b)
{
    __m128i equal = _mm_cmpeq_epi8(a, b);
    return _mm_movemask_epi8(equal);
}

using ska::detailv3::HashPolicySelector;

template<typename = void>
struct sherwood_v12_constants
{
    static constexpr int8_t magic_for_end_misaligned    = 0b10000010;
    static constexpr int8_t magic_for_end_aligned       = 0b10000001;
    static constexpr int8_t magic_for_empty             = 0b10000000;
    static constexpr int8_t magic_for_tombstone         = 0b10000011;
    static constexpr int8_t extra_bits_mask             = 0b01111111;
    static constexpr int num_extra_bits                 = 7;

    static bool skip_while_iterating(int8_t metadata)
    {
        return metadata == magic_for_empty || metadata == magic_for_tombstone;
    }
};

union sherwood_v12_metadata_block
{
    __m128i control_bytes;
    int8_t control_bytes_as_ints[16];

    static sherwood_v12_metadata_block * empty_block()
    {
        static sherwood_v12_metadata_block * empty_block = []
        {
            static sherwood_v12_metadata_block blocks[2];
            blocks[0].control_bytes = blocks[1].control_bytes = _mm_set_epi8(
                        sherwood_v12_constants<>::magic_for_end_aligned,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_empty,
                        sherwood_v12_constants<>::magic_for_end_aligned);
            return blocks;
        }();
        return empty_block;
    }

    int first_empty_index() const
    {
        int empty_indices = _mm_movemask_epi8(_mm_and_si128(control_bytes, _mm_set1_epi8(sherwood_v12_constants<>::magic_for_empty)));
        if (empty_indices)
            return _bit_scan_forward(empty_indices);
        else
            return -1;
    }
    bool any_empty() const
    {
        return _mm_movemask_epi8(_mm_cmpeq_epi8(control_bytes, _mm_set1_epi8(sherwood_v12_constants<>::magic_for_empty))) != 0;
    }
};

enum SherwoodV12LookupType
{
    Default,
    Prefetch
};

template<typename T, typename FindKey, typename ArgumentHash, typename Hasher, typename ArgumentEqual, typename Equal, typename ArgumentAlloc, typename ByteAlloc, SherwoodV12LookupType lookup_type>
class sherwood_v12_table : private ByteAlloc, private Hasher, private Equal
{
    using AllocatorTraits = std::allocator_traits<ByteAlloc>;
    using MetadataPointer = sherwood_v12_metadata_block *;
    using DataPointer = T *;
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

    sherwood_v12_table()
    {
    }
    explicit sherwood_v12_table(size_type bucket_count, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : ByteAlloc(alloc), Hasher(hash), Equal(equal)
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
        : ByteAlloc(alloc)
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
            deallocate_data(data, num_blocks_minus_one);
            throw;
        }
    }
    sherwood_v12_table(sherwood_v12_table && other) noexcept
        : ByteAlloc(std::move(other)), Hasher(std::move(other)), Equal(std::move(other))
        , _max_load_factor(other._max_load_factor)
    {
        swap_pointers(other);
    }
    sherwood_v12_table(sherwood_v12_table && other, const ArgumentAlloc & alloc) noexcept
        : ByteAlloc(alloc), Hasher(std::move(other)), Equal(std::move(other))
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
    sherwood_v12_table & operator=(sherwood_v12_table && other) noexcept
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
    ~sherwood_v12_table()
    {
        clear();
        deallocate_data(data, num_blocks_minus_one);
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
        MetadataPointer current_metadata{};
        DataPointer current_data{};
        int position_in_block = 0;

        templated_iterator()
        {
        }
        templated_iterator(MetadataPointer metadata, DataPointer data, int position_in_block)
            : current_metadata(metadata)
            , current_data(data)
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
            return lhs.current_metadata == rhs.current_metadata && lhs.position_in_block == rhs.position_in_block;
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
                    ++current_metadata;
                    current_data += 16;
                }
            }
            while(sherwood_v12_constants<>::skip_while_iterating(current_metadata->control_bytes_as_ints[position_in_block]));
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
            return current_data[position_in_block];
        }
        ValueType * operator->() const
        {
            return current_data + position_in_block;
        }

        operator templated_iterator<const value_type>() const
        {
            return { current_metadata, current_data, position_in_block };
        }
    };
    using iterator = templated_iterator<value_type>;
    using const_iterator = templated_iterator<const value_type>;

    iterator begin()
    {
        if (!data)
            return {metadata + 1, nullptr, 0};
        else if (sherwood_v12_constants<>::skip_while_iterating(metadata[0].control_bytes_as_ints[0]))
            return ++iterator{metadata, data, 0};
        else
            return {metadata, data, 0};
    }
    const_iterator begin() const
    {
        if (!data)
            return {metadata + 1, nullptr, 0};
        else if (sherwood_v12_constants<>::skip_while_iterating(metadata[0].control_bytes_as_ints[0]))
            return ++iterator{metadata, data, 0};
        else
            return {metadata, data, 0};
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    iterator end()
    {
        return { metadata + num_blocks_minus_one + 1, nullptr, 0 };
    }
    const_iterator end() const
    {
        return { metadata + num_blocks_minus_one + 1, nullptr, 0 };
    }
    const_iterator cend() const
    {
        return end();
    }

    inline iterator find(const FindKey & key)
    {
        size_t hash = hash_object(key);
        size_t num_blocks_minus_one = this->num_blocks_minus_one;
        MetadataPointer metadata = this->metadata;
        DataPointer data = this->data;
        static constexpr int num_extra_bits = sherwood_v12_constants<>::num_extra_bits;
        size_t index = hash_policy.template index_for_hash<num_extra_bits>(hash, num_blocks_minus_one);
        __m128i compare = _mm_set1_epi8(hash_policy.template extra_bits_for_hash<num_extra_bits>(hash));
        if constexpr (lookup_type == Prefetch)
        {
            _mm_prefetch(data + index * 16, _MM_HINT_T1);
        }

        for (;;)
        {
            MetadataPointer lookup = metadata + index;
            int which_are_equal = equal_bits(compare, lookup->control_bytes);
            while (which_are_equal)
            {
                int offset = _bit_scan_forward(which_are_equal);
                if (compares_equal(key, data[index * 16 + offset]))
                    return { lookup, data + index * 16, offset };
                which_are_equal &= 0xfffffffe << offset;
            }
            if (lookup->any_empty())
                return end();
            if (index == num_blocks_minus_one)
                index = 0;
            else
                ++index;
        }
    }
    const_iterator find(const FindKey & key) const
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
    // debug function. same as find(), but returns how many items were
    // looked at
    inline int num_lookups(const FindKey & key)
    {
        int result = 0;
        size_t hash = hash_object(key);
        size_t num_blocks_minus_one = this->num_blocks_minus_one;
        MetadataPointer metadata = this->metadata;
        DataPointer data = this->data;
        static constexpr int num_extra_bits = sherwood_v12_constants<>::num_extra_bits;
        size_t index = hash_policy.template index_for_hash<num_extra_bits>(hash, num_blocks_minus_one);
        __m128i compare = _mm_set1_epi8(hash_policy.template extra_bits_for_hash<num_extra_bits>(hash));

        for (;;)
        {
            MetadataPointer lookup = metadata + index;
            int which_are_equal = equal_bits(compare, lookup->control_bytes);
            while (which_are_equal)
            {
                int offset = _bit_scan_forward(which_are_equal);
                ++result;
                if (compares_equal(key, data[index * 16 + offset]))
                    return result;
                which_are_equal &= 0xfffffffe << offset;
            }
            if (lookup->any_empty())
                return result;
            if (index == num_blocks_minus_one)
                index = 0;
            else
                ++index;
        }
    }


    template<typename Key, typename... Args>
    inline std::pair<iterator, bool> emplace(Key && key, Args &&... args)
    {
#if 1
        size_t hash = hash_object(key);
        size_t num_blocks_minus_one = this->num_blocks_minus_one;
        MetadataPointer metadata = this->metadata;
        DataPointer data = this->data;
        static constexpr int num_extra_bits = sherwood_v12_constants<>::num_extra_bits;
        size_t index = hash_policy.template index_for_hash<num_extra_bits>(hash, num_blocks_minus_one);
        size_t initial = index;
        uint8_t extra_bits = hash_policy.template extra_bits_for_hash<num_extra_bits>(hash);
        __m128i compare = _mm_set1_epi8(extra_bits);
        for (;;)
        {
            MetadataPointer lookup = metadata + index;
            int which_are_equal = equal_bits(compare, lookup->control_bytes);
            while (which_are_equal)
            {
                int offset = _bit_scan_forward(which_are_equal);
                if (compares_equal(key, data[index * 16 + offset]))
                    return { { lookup, data + index * 16, offset }, false };
                which_are_equal &= 0xfffffffe << offset;
            }
            if (lookup->any_empty())
                break;
            if (index == num_blocks_minus_one)
                index = 0;
            else
                ++index;
        }
        return emplace_new_key(initial, extra_bits, std::forward<Key>(key), std::forward<Args>(args)...);
#else
        size_t hash = hash_object(key);
        size_t num_blocks_minus_one = this->num_blocks_minus_one;
        MetadataPointer metadata = this->metadata;
        DataPointer data = this->data;
        static constexpr int num_extra_bits = sherwood_v12_constants<>::num_extra_bits;
        size_t index = hash_policy.template index_for_hash<num_extra_bits>(hash, num_blocks_minus_one);
        size_t initial = index;
        uint8_t extra_bits = hash_policy.template extra_bits_for_hash<num_extra_bits>(hash);
        __m128i compare = _mm_set1_epi8(extra_bits);
        for (;;)
        {
            MetadataPointer lookup = metadata + index;
            int which_are_equal = equal_bits(compare, lookup->control_bytes);
            while (which_are_equal)
            {
                int offset = _bit_scan_forward(which_are_equal);
                if (compares_equal(key, data[index * 16 + offset]))
                    return { { lookup, data + index * 16, offset }, false };
                which_are_equal &= 0xfffffffe << offset;
            }
            if (lookup->any_empty())
                break;
            if (index == num_blocks_minus_one)
                index = 0;
            else
                ++index;
        }
        return emplace_new_key(initial, extra_bits, std::forward<Key>(key), std::forward<Args>(args)...);
#endif
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
        if (data && num_blocks == num_blocks_minus_one + 1)
            return;
        size_t memory_requirement = calculate_memory_requirement(num_blocks);
        unsigned char * new_memory = &*AllocatorTraits::allocate(*this, memory_requirement);
        bool is_misaligned = (reinterpret_cast<size_t>(new_memory) % 16) != 0;
        if (is_misaligned)
            new_memory += 8;

        DataPointer new_data = reinterpret_cast<DataPointer>(new_memory);
        MetadataPointer new_metadata = reinterpret_cast<MetadataPointer>(new_data + num_blocks * 16);

        MetadataPointer special_end_item = new_metadata + num_blocks;
        for (MetadataPointer it = new_metadata; it != special_end_item; ++it)
            it->control_bytes = _mm_set1_epi8(sherwood_v12_constants<>::magic_for_empty);
        if (is_misaligned)
            special_end_item->control_bytes = _mm_set1_epi8(sherwood_v12_constants<>::magic_for_end_misaligned);
        else
            special_end_item->control_bytes = _mm_set1_epi8(sherwood_v12_constants<>::magic_for_end_aligned);
        using std::swap;
        swap(data, new_data);
        swap(metadata, new_metadata);
        swap(num_blocks_minus_one, num_blocks);
        --num_blocks_minus_one;
        hash_policy.commit(new_prime_index);
        num_elements = 0;
        num_tombstones = 0;
        if (new_data)
        {
            DataPointer data_it = new_data;
            for (MetadataPointer it = new_metadata, end = it + num_blocks + 1; it != end; ++it, data_it += 16)
            {
                for (int i = 0; i < 16; ++i)
                {
                    if (!sherwood_v12_constants<>::skip_while_iterating(it->control_bytes_as_ints[i]))
                    {
                        emplace(std::move(data_it[i]));
                        AllocatorTraits::destroy(*this, data_it + i);
                    }
                }
            }
            deallocate_data(new_data, num_blocks);
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
        AllocatorTraits::destroy(*this, to_erase.current_data + to_erase.position_in_block);
        --num_elements;
        uint8_t new_metadata = sherwood_v12_constants<>::magic_for_empty;
        if (!to_erase.current_metadata->any_empty())
        {
            new_metadata = sherwood_v12_constants<>::magic_for_tombstone;
            ++num_tombstones;
        }
        to_erase.current_metadata->control_bytes_as_ints[to_erase.position_in_block] = new_metadata;
        return { to_erase.current_metadata, to_erase.current_data, to_erase.position_in_block };
    }

    iterator erase(const_iterator begin_it, const_iterator end_it)
    {
        while (begin_it != end_it)
        {
            begin_it = erase(begin_it);
        }
        return { end_it.current_metadata, end_it.current_data, end_it.position_in_block };
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
        if (!data)
            return;
        DataPointer data_it = data;
        for (MetadataPointer it = metadata, end = it + num_blocks_minus_one + 1; it != end; ++it, data_it += 16)
        {
            for (int i = 0; i < 16; ++i)
            {
                if (it->control_bytes_as_ints[i] == sherwood_v12_constants<>::magic_for_tombstone)
                    it->control_bytes_as_ints[i] = sherwood_v12_constants<>::magic_for_empty;
                else if (it->control_bytes_as_ints[i] != sherwood_v12_constants<>::magic_for_empty)
                {
                    AllocatorTraits::destroy(*this, data_it + i);
                    it->control_bytes_as_ints[i] = sherwood_v12_constants<>::magic_for_empty;
                }
            }
        }
        num_elements = 0;
        num_tombstones = 0;
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
        if (!data)
            return 0;
        else
            return 16 * (num_blocks_minus_one + 1);
    }
    size_type max_bucket_count() const
    {
        return (AllocatorTraits::max_size(*this)) / sizeof(T);
    }
    size_t bucket(const FindKey & key) const
    {
        return hash_policy.template index_for_hash<sherwood_v12_constants<>::num_extra_bits>(hash_object(key), num_blocks_minus_one);
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
    MetadataPointer metadata = sherwood_v12_metadata_block::empty_block();
    DataPointer data = nullptr;
    size_t num_blocks_minus_one = 0;
    typename HashPolicySelector<ArgumentHash>::type hash_policy;
    float _max_load_factor = 0.875f;
    size_t num_elements = 0;
    size_t num_tombstones = 0;

    size_t num_buckets_for_reserve(size_t num_elements) const
    {
        return static_cast<size_t>(std::ceil(num_elements / static_cast<double>(_max_load_factor)));
    }
    void rehash_for_other_container(const sherwood_v12_table & other)
    {
        rehash(std::min(num_buckets_for_reserve(other.size()), other.bucket_count()));
    }

    float load_factor_for_num_items(size_t num_items) const
    {
        if (!data)
            return num_items ? std::numeric_limits<float>::infinity() : 0.0f;
        size_t buckets = num_blocks_minus_one + 1;
        buckets *= 16;
        return static_cast<double>(num_items) / buckets;
    }

    void swap_pointers(sherwood_v12_table & other)
    {
        using std::swap;
        swap(hash_policy, other.hash_policy);
        swap(data, other.data);
        swap(metadata, other.metadata);
        swap(num_blocks_minus_one, other.num_blocks_minus_one);
        swap(num_elements, other.num_elements);
        swap(num_tombstones, other.num_tombstones);
        swap(_max_load_factor, other._max_load_factor);
    }

    template<typename... Args>
    SKA_NOINLINE(std::pair<iterator, bool>) emplace_new_key(size_t index, uint8_t extra_bits, Args &&... args)
    {
        using std::swap;
        float new_load_factor = load_factor_for_num_items(num_elements + num_tombstones + 1);
        if (new_load_factor == 1.0f || new_load_factor > _max_load_factor)
        {
            grow();
            return emplace(std::forward<Args>(args)...);
        }
        for (;; index = hash_policy.keep_in_range(index + 1, num_blocks_minus_one))
        {
            MetadataPointer block = metadata + index;
            int first_empty_index = block->first_empty_index();
            if (first_empty_index != -1)
            {
                AllocatorTraits::construct(*this, data + index * 16 + first_empty_index, std::forward<Args>(args)...);
                if (block->control_bytes_as_ints[first_empty_index] == sherwood_v12_constants<>::magic_for_tombstone)
                    --num_tombstones;
                block->control_bytes_as_ints[first_empty_index] = extra_bits;
                ++num_elements;
                return { { block, data + index * 16, first_empty_index }, true };
            }
        }
    }

    void grow()
    {
        rehash(std::max(size_t(4), 2 * bucket_count()));
    }

    size_t calculate_memory_requirement(size_t num_blocks)
    {
        size_t memory_required = (sizeof(sherwood_v12_metadata_block) + sizeof(T) * 16) * num_blocks;
        memory_required += 16; // for metadata of past-the-end pointer
        memory_required += 8; // to fix alignment should the allocator return misaligned memory
        return memory_required;

    }

    void deallocate_data(DataPointer begin, size_t num_blocks_minus_one)
    {
        if (!begin)
            return;

        size_t memory = calculate_memory_requirement(num_blocks_minus_one + 1);
        MetadataPointer end = reinterpret_cast<MetadataPointer>(begin + 16 * (num_blocks_minus_one + 1)) + num_blocks_minus_one + 1;
        unsigned char * as_byte_pointer = reinterpret_cast<unsigned char *>(begin);
        if (end->control_bytes_as_ints[0] == sherwood_v12_constants<>::magic_for_end_misaligned)
            as_byte_pointer -= 8;
        AllocatorTraits::deallocate(*this, typename AllocatorTraits::pointer(as_byte_pointer), memory);
    }

    void reset_to_empty_state()
    {
        deallocate_data(data, num_blocks_minus_one);
        metadata = sherwood_v12_metadata_block::empty_block();
        data = nullptr;
        num_blocks_minus_one = 0;
        hash_policy.reset();
        num_tombstones = 0;
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
        MetadataPointer metadata;
        DataPointer data;
        int position_in_block;

        operator iterator()
        {
            if (sherwood_v12_constants<>::skip_while_iterating(metadata->control_bytes_as_ints[position_in_block]))
                return ++iterator{metadata, data, position_in_block};
            else
                return { metadata, data, position_in_block };
        }
        operator const_iterator()
        {
            if (sherwood_v12_constants<>::skip_while_iterating(metadata->control_bytes_as_ints[position_in_block]))
                return ++iterator{metadata, data, position_in_block};
            else
                return { metadata, data, position_in_block };
        }
    };
};
}

template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<K, V> >, detailv12::SherwoodV12LookupType lookup_type = detailv12::Default>
class google_flat16_hash_map
        : public detailv12::sherwood_v12_table
        <
            std::pair<K, V>,
            K,
            H,
            detailv12::KeyOrValueHasher<K, std::pair<K, V>, H>,
            E,
            detailv12::KeyOrValueEquality<K, std::pair<K, V>, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
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
        typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
        lookup_type
    >;
public:

    using key_type = K;
    using mapped_type = V;

    using Table::Table;
    google_flat16_hash_map()
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

    friend bool operator==(const google_flat16_hash_map & lhs, const google_flat16_hash_map & rhs)
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
    friend bool operator!=(const google_flat16_hash_map & lhs, const google_flat16_hash_map & rhs)
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

template<typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename A = std::allocator<T>, detailv12::SherwoodV12LookupType lookup_type = detailv12::Default >
class google_flat16_hash_set
        : public detailv12::sherwood_v12_table
        <
            T,
            T,
            H,
            detailv12::functor_storage<size_t, H>,
            E,
            detailv12::functor_storage<bool, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
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
        typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
        lookup_type
    >;
public:

    using key_type = T;

    using Table::Table;
    google_flat16_hash_set()
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

    friend bool operator==(const google_flat16_hash_set & lhs, const google_flat16_hash_set & rhs)
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
    friend bool operator!=(const google_flat16_hash_set & lhs, const google_flat16_hash_set & rhs)
    {
        return !(lhs == rhs);
    }
};

} // end namespace ska
