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

namespace detailv11
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

using ska::detailv3::HashPolicySelector;
using ska::detailv3::AssignIfTrue;

enum LookupType
{
    SIMD_Hash,
    Hash,
    Bool
};

template<typename T, LookupType>
struct sherwood_v11_block
{
    static constexpr size_t BlockSize = 4;
    union
    {
        uint32_t hashes[BlockSize];
        __m128i hashes_simd;
    };
    union
    {
        T data[BlockSize];
    };

    static sherwood_v11_block * empty_block()
    {
        static __m128i empty_bytes = _mm_setzero_si128();
        return reinterpret_cast<sherwood_v11_block *>(&empty_bytes);
    }

    int equal_indices(__m128i hash) const
    {
        __m128i is_equal = _mm_cmpeq_epi32(hashes_simd, hash);
        return _mm_movemask_ps(_mm_castsi128_ps(is_equal));
    }

    int first_empty_index() const
    {
        int first_zero = equal_indices(_mm_setzero_si128());
        return _bit_scan_forward(first_zero | 0b10000);
    }

    void clear_hashes()
    {
        hashes_simd = _mm_setzero_si128();
    }
    bool is_filled(size_t index) const
    {
        return hashes[index] != 0;
    }
    void clear_filled(size_t index)
    {
        hashes[index] = 0;
    }
};

template<typename T>
struct sherwood_v11_block<T, Bool>
{
    static constexpr size_t BlockSize = 4;
    std::array<bool, BlockSize> filled;
    union
    {
        T data[BlockSize];
    };

    static sherwood_v11_block * empty_block()
    {
        static __m128i empty_bytes = _mm_setzero_si128();
        return reinterpret_cast<sherwood_v11_block *>(&empty_bytes);
    }

    int first_empty_index() const
    {
        for (size_t i = 0; i < BlockSize; ++i)
        {
            if (!filled[i])
                return i;
        }
        return BlockSize;
    }

    void clear_hashes()
    {
        filled.fill(false);
    }
    bool is_filled(size_t index) const
    {
        return filled[index];
    }
    void clear_filled(size_t index)
    {
        filled[index] = false;
    }
};

template<typename T, typename FindKey, typename ArgumentHash, typename Hasher, typename ArgumentEqual, typename Equal, typename ArgumentAlloc, typename ByteAlloc, LookupType lookup_type>
class sherwood_v11_table : private ByteAlloc, private Hasher, private Equal
{
    using AllocatorTraits = std::allocator_traits<ByteAlloc>;
    using Block = sherwood_v11_block<T, lookup_type>;
    using BlockPointer = Block *;
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

    sherwood_v11_table()
    {
    }
    explicit sherwood_v11_table(size_type bucket_count, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : ByteAlloc(alloc), Hasher(hash), Equal(equal)
    {
        if (bucket_count)
            rehash(bucket_count);
    }
    sherwood_v11_table(size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v11_table(bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v11_table(size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v11_table(bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    explicit sherwood_v11_table(const ArgumentAlloc & alloc)
        : ByteAlloc(alloc)
    {
    }
    template<typename It>
    sherwood_v11_table(It first, It last, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v11_table(bucket_count, hash, equal, alloc)
    {
        insert(first, last);
    }
    template<typename It>
    sherwood_v11_table(It first, It last, size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v11_table(first, last, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    template<typename It>
    sherwood_v11_table(It first, It last, size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v11_table(first, last, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v11_table(std::initializer_list<T> il, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
        : sherwood_v11_table(bucket_count, hash, equal, alloc)
    {
        if (bucket_count == 0)
            rehash(il.size());
        insert(il.begin(), il.end());
    }
    sherwood_v11_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentAlloc & alloc)
        : sherwood_v11_table(il, bucket_count, ArgumentHash(), ArgumentEqual(), alloc)
    {
    }
    sherwood_v11_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
        : sherwood_v11_table(il, bucket_count, hash, ArgumentEqual(), alloc)
    {
    }
    sherwood_v11_table(const sherwood_v11_table & other)
        : sherwood_v11_table(other, AllocatorTraits::select_on_container_copy_construction(other.get_allocator()))
    {
    }
    sherwood_v11_table(const sherwood_v11_table & other, const ArgumentAlloc & alloc)
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
            deallocate_data(entries0, num_blocks_minus_one);
            throw;
        }
    }
    sherwood_v11_table(sherwood_v11_table && other) noexcept
        : ByteAlloc(std::move(other)), Hasher(std::move(other)), Equal(std::move(other))
        , _max_load_factor(other._max_load_factor)
    {
        swap_pointers(other);
    }
    sherwood_v11_table(sherwood_v11_table && other, const ArgumentAlloc & alloc) noexcept
        : ByteAlloc(alloc), Hasher(std::move(other)), Equal(std::move(other))
        , _max_load_factor(other._max_load_factor)
    {
        swap_pointers(other);
    }
    sherwood_v11_table & operator=(const sherwood_v11_table & other)
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
    sherwood_v11_table & operator=(sherwood_v11_table && other) noexcept
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
    ~sherwood_v11_table()
    {
        clear();
        deallocate_data(entries0, num_blocks_minus_one);
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
        friend class sherwood_v11_table;
        BlockPointer current = BlockPointer();
        size_t index = 0;

    public:
        templated_iterator()
        {
        }
        templated_iterator(BlockPointer entries, size_t index)
            : current(entries)
            , index(index)
        {
        }

        using iterator_category = std::forward_iterator_tag;
        using value_type = ValueType;
        using difference_type = ptrdiff_t;
        using pointer = ValueType *;
        using reference = ValueType &;

        friend bool operator==(const templated_iterator & lhs, const templated_iterator & rhs)
        {
            return lhs.index == rhs.index;
        }
        friend bool operator!=(const templated_iterator & lhs, const templated_iterator & rhs)
        {
            return !(lhs == rhs);
        }

        templated_iterator & operator++()
        {
            do
            {
                if (index % Block::BlockSize == 0)
                    --current;
                if (index-- == 0)
                    break;
            }
            while(!current->is_filled(index % Block::BlockSize));
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
            return current->data[index % Block::BlockSize];
        }
        ValueType * operator->() const
        {
            return current->data + index % Block::BlockSize;
        }

        operator templated_iterator<const value_type>() const
        {
            return { current, index };
        }
    };
    using iterator = templated_iterator<value_type>;
    using const_iterator = templated_iterator<const value_type>;

    iterator begin()
    {
        size_t num_blocks = num_blocks_minus_one ? num_blocks_minus_one + 1 : 0;
        return ++iterator{ entries0 + 2 * num_blocks, 2 * num_blocks * Block::BlockSize };
    }
    const_iterator begin() const
    {
        size_t num_blocks = num_blocks_minus_one ? num_blocks_minus_one + 1 : 0;
        return ++iterator{ entries0 + 2 * num_blocks, 2 * num_blocks * Block::BlockSize };
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    iterator end()
    {
        return { entries0 - 1, std::numeric_limits<size_t>::max() };
    }
    const_iterator end() const
    {
        return { entries0 - 1, std::numeric_limits<size_t>::max() };
    }
    const_iterator cend() const
    {
        return end();
    }

    inline iterator find(const FindKey & key)
    {
        size_t hash = hash_object(key);
        auto [index0, index1] = indices_from_hash(hash);
        if constexpr (lookup_type == SIMD_Hash)
        {
            __m128i simd_hash = _mm_set1_epi32(hash);
            BlockPointer block0 = entries0 + index0;
            int equal_indices0 = block0->equal_indices(simd_hash);
            while (equal_indices0)
            {
                int offset = _bit_scan_forward(equal_indices0);
                if (compares_equal(key, block0->data[offset]))
                    return { block0, size_t(offset) + index0 * Block::BlockSize };
                equal_indices0 &= 0xfffffffe << offset;
            }
            BlockPointer block1 = entries1 + index1;
            int equal_indices1 = block1->equal_indices(simd_hash);
            while (equal_indices1)
            {
                int offset = _bit_scan_forward(equal_indices1);
                if (compares_equal(key, block1->data[offset]))
                    return { block1, size_t(offset) + index1 * Block::BlockSize };
                equal_indices1 &= 0xfffffffe << offset;
            }
            return end();
        }
        else if constexpr (lookup_type == Hash)
        {
            uint32_t hash_as_u32 = hash;
            BlockPointer block0 = entries0 + index0;
            BlockPointer block1 = entries1 + index1;
            for (size_t i = 0; i < Block::BlockSize; ++i)
            {
                if (block0->hashes[i] == hash_as_u32 && compares_equal(key, block0->data[i]))
                    return { block0, i + index0 * Block::BlockSize };
                if (block1->hashes[i] == hash_as_u32 && compares_equal(key, block1->data[i]))
                    return { block1, i + index1 * Block::BlockSize };
            }
            return end();
        }
        else
        {
            BlockPointer block0 = entries0 + index0;
            BlockPointer block1 = entries1 + index1;
            for (size_t i = 0; i < Block::BlockSize; ++i)
            {
                if (block0->filled[i] && compares_equal(key, block0->data[i]))
                    return { block0, i + index0 * Block::BlockSize };
                if (block1->filled[i] && compares_equal(key, block1->data[i]))
                    return { block1, i + index1 * Block::BlockSize };
            }
            return end();
        }
    }
    inline const_iterator find(const FindKey & key) const
    {
        return const_cast<sherwood_v11_table *>(this)->find(key);
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
        if constexpr (lookup_type == Bool)
        {
            BlockPointer block0 = entries0 + index0;
            BlockPointer block1 = entries1 + index1;
            for (size_t i = 0; i < Block::BlockSize; ++i)
            {
                if (block0->filled[i] && compares_equal(key, block0->data[i]))
                    return { { block0, i + index0 * Block::BlockSize }, false };
                if (block1->filled[i] && compares_equal(key, block1->data[i]))
                    return { { block1, i + index1 * Block::BlockSize }, false };
            }
            return emplace_new_key(hash, index0, index1, std::forward<Key>(key), std::forward<Args>(args)...);
        }
        else
        {
            __m128i simd_hash = _mm_set1_epi32(hash);
            BlockPointer block0 = entries0 + index0;
            int equal_indices0 = block0->equal_indices(simd_hash);
            while (equal_indices0)
            {
                int offset = _bit_scan_forward(equal_indices0);
                if (compares_equal(key, block0->data[offset]))
                    return { { block0, size_t(offset) + index0 * Block::BlockSize }, false };
                equal_indices0 &= 0xfffffffe << offset;
            }
            BlockPointer block1 = entries1 + index1;
            int equal_indices1 = block1->equal_indices(simd_hash);
            while (equal_indices1)
            {
                int offset = _bit_scan_forward(equal_indices1);
                if (compares_equal(key, block1->data[offset]))
                    return { { block1, size_t(offset) + index1 * Block::BlockSize }, false };
                equal_indices1 &= 0xfffffffe << offset;
            }
            return emplace_new_key(hash, index0, index1, std::forward<Key>(key), std::forward<Args>(args)...);
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

    void rehash(size_t num_items)
    {
        num_items = std::max(num_items, static_cast<size_t>(std::ceil(num_elements / static_cast<double>(_max_load_factor))));
        if (num_items == 0)
        {
            // todo: does this call destructors?
            reset_to_empty_state();
            return;
        }
        size_t num_blocks = num_items / Block::BlockSize / 2;
        num_blocks = std::max(num_blocks, size_t(2));
        if (num_blocks == num_blocks_minus_one + 1)
            return;
        int next_fib = detailv3::log2(num_blocks);
        size_t memory_requirement = calculate_memory_requirement(num_blocks);
        unsigned char * new_memory = &*AllocatorTraits::allocate(*this, memory_requirement);

        BlockPointer new_buckets = reinterpret_cast<BlockPointer>(new_memory);

        for (BlockPointer it = new_buckets, end = new_buckets + 2 * num_blocks; it < end; ++it)
            it->clear_hashes();

        BlockPointer old_buckets = std::move(entries0);
        entries0 = std::move(new_buckets);
        entries1 = entries0 + num_blocks;
        size_t old_num_blocks = num_blocks_minus_one;
        num_blocks_minus_one = num_blocks - 1;
        if (old_num_blocks)
            ++old_num_blocks;
        fib_shift_amount = 64 - next_fib;
        num_elements = 0;
        for (BlockPointer it = old_buckets, end = old_buckets + 2 * old_num_blocks; it != end; ++it)
        {
            for (size_t i = 0; i < Block::BlockSize; ++i)
            {
                if (it->is_filled(i))
                {
                    emplace(std::move(it->data[i]));
                    AllocatorTraits::destroy(*this, it->data + i);
                }
            }
        }
        deallocate_data(old_buckets, old_num_blocks - 1);
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
        return { to_erase.current, to_erase.index };
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
        if (!num_blocks_minus_one)
            return;
        size_t num_blocks = num_blocks_minus_one + 1;
        for (BlockPointer it = entries0, end = it + 2 * num_blocks; it != end; ++it)
        {
            for (size_t i = 0; i < Block::BlockSize; ++i)
            {
                if (it->is_filled(i))
                {
                    AllocatorTraits::destroy(*this, std::addressof(it->data[i]));
                    it->clear_filled(i);
                }
            }
        }
        num_elements = 0;
    }

    void shrink_to_fit()
    {
        rehash_for_other_container(*this);
    }

    void swap(sherwood_v11_table & other)
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
        return num_blocks_minus_one ? (num_blocks_minus_one + 1) * Block::BlockSize * 2 : 0;
    }
    size_type max_bucket_count() const
    {
        return (AllocatorTraits::max_size(*this)) / sizeof(T);
    }
    float load_factor() const
    {
        return static_cast<double>(num_elements) / ((num_blocks_minus_one + 1) * Block::BlockSize);
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
    BlockPointer entries0 = Block::empty_block();
    BlockPointer entries1 = Block::empty_block();
    size_t num_blocks_minus_one = 0;
    int fib_shift_amount = 63;
    float _max_load_factor = 0.9375f;
    size_t num_elements = 0;

    size_t num_buckets_for_reserve(size_t num_elements) const
    {
        return static_cast<size_t>(std::ceil(num_elements / static_cast<double>(_max_load_factor)));
    }
    void rehash_for_other_container(const sherwood_v11_table & other)
    {
        rehash(std::min(num_buckets_for_reserve(other.size()), other.bucket_count()));
    }
    bool is_full() const
    {
        if (!num_blocks_minus_one)
            return true;
        else
            return num_elements + 1 > 2 * Block::BlockSize * (num_blocks_minus_one + 1) * static_cast<double>(_max_load_factor);
    }

    void swap_pointers(sherwood_v11_table & other)
    {
        using std::swap;
        swap(fib_shift_amount, other.fib_shift_amount);
        swap(entries0, other.entries0);
        swap(entries1, other.entries1);
        swap(num_blocks_minus_one, other.num_blocks_minus_one);
        swap(num_elements, other.num_elements);
        swap(_max_load_factor, other._max_load_factor);
    }

    template<typename... Args>
    SKA_NOINLINE(std::pair<iterator, bool>) emplace_new_key(size_t hash, size_t index0, size_t index1, Args &&... args)
    {
        if (is_full())
        {
            grow();
            return emplace(std::forward<Args>(args)...);
        }
        BlockPointer block0 = entries0 + index0;
        BlockPointer block1 = entries1 + index1;
        size_t num0 = block0->first_empty_index();
        size_t num1 = block1->first_empty_index();
        BlockPointer free_block;
        size_t free_index;
        if (num1 < num0)
        {
            free_block = block1;
            free_index = num1;
        }
        else if (num0 == Block::BlockSize)
        {
            grow();
            return emplace(std::forward<Args>(args)...);
        }
        else
        {
            free_block = block0;
            free_index = num0;
        }
        AllocatorTraits::construct(*this, std::addressof(free_block->data[free_index]), std::forward<Args>(args)...);
        if constexpr (lookup_type == Bool)
        {
            free_block->filled[free_index] = true;
        }
        else
        {
            free_block->hashes[free_index] = hash;
        }
        ++num_elements;
        return { { free_block, free_index }, true };
    }

    void grow()
    {
        rehash(std::max(size_t(1), 2 * bucket_count()));
    }

    size_t calculate_memory_requirement(size_t num_blocks)
    {
        return 2 * sizeof(Block) * num_blocks;
    }

    void deallocate_data(BlockPointer begin, size_t num_blocks_minus_one)
    {
        if (begin == Block::empty_block())
            return;

        ++num_blocks_minus_one;
        size_t memory = calculate_memory_requirement(num_blocks_minus_one);
        unsigned char * as_byte_pointer = reinterpret_cast<unsigned char *>(begin);
        AllocatorTraits::deallocate(*this, typename AllocatorTraits::pointer(as_byte_pointer), memory);
    }

    void reset_to_empty_state()
    {
        deallocate_data(entries0, num_blocks_minus_one);
        entries0 = Block::empty_block();
        entries1 = Block::empty_block();
        num_blocks_minus_one = 0;
        fib_shift_amount = 63;
    }

    template<typename U>
    size_t hash_object(const U & key)
    {
        return const_cast<const sherwood_v11_table &>(*this).hash_object(key);
    }
    template<typename U>
    size_t hash_object(const U & key) const
    {
        if constexpr (lookup_type == Bool)
        {
            return static_cast<const Hasher &>(*this)(key);
        }
        else
        {
            return std::max(static_cast<const Hasher &>(*this)(key), size_t(1));
        }
    }
    std::pair<size_t, size_t> indices_from_hash(size_t hash) const
    {
        size_t crc = _mm_crc32_u64(0, hash);
        size_t fib = hash * 11400714819323198485llu;
        return { crc & num_blocks_minus_one, (fib >> fib_shift_amount) & num_blocks_minus_one };
    }
    template<typename L, typename R>
    bool compares_equal(const L & lhs, const R & rhs)
    {
        return static_cast<Equal &>(*this)(lhs, rhs);
    }

    struct convertible_to_iterator
    {
        BlockPointer it;
        size_t index;

        operator iterator()
        {
            if (!it->control_bytes[index % Block::BlockSize])
                return ++iterator{it, index};
            else
                return { it, index };
        }
        operator const_iterator()
        {
            if (!it->control_bytes[index % Block::BlockSize])
                return ++iterator{it, index};
            else
                return { it, index };
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

template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<K, V> >, detailv11::LookupType lookup_type = detailv11::Hash >
class agl_hash_map
        : public detailv11::sherwood_v11_table
        <
            std::pair<K, V>,
            K,
            H,
            detailv11::KeyOrValueHasher<K, std::pair<K, V>, H>,
            E,
            detailv11::KeyOrValueEquality<K, std::pair<K, V>, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
            lookup_type
        >
{
    using Table = detailv11::sherwood_v11_table
    <
        std::pair<K, V>,
        K,
        H,
        detailv11::KeyOrValueHasher<K, std::pair<K, V>, H>,
        E,
        detailv11::KeyOrValueEquality<K, std::pair<K, V>, E>,
        A,
        typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
        lookup_type
    >;
public:

    using key_type = K;
    using mapped_type = V;

    using Table::Table;
    agl_hash_map()
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

    friend bool operator==(const agl_hash_map & lhs, const agl_hash_map & rhs)
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
    friend bool operator!=(const agl_hash_map & lhs, const agl_hash_map & rhs)
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

template<typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename A = std::allocator<T>, detailv11::LookupType lookup_type = detailv11::Hash >
class agl_hash_set
        : public detailv11::sherwood_v11_table
        <
            T,
            T,
            H,
            detailv11::functor_storage<size_t, H>,
            E,
            detailv11::functor_storage<bool, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
            lookup_type
        >
{
    using Table = detailv11::sherwood_v11_table
    <
        T,
        T,
        H,
        detailv11::functor_storage<size_t, H>,
        E,
        detailv11::functor_storage<bool, E>,
        A,
        typename std::allocator_traits<A>::template rebind_alloc<unsigned char>,
        lookup_type
    >;
public:

    using key_type = T;

    using Table::Table;
    agl_hash_set()
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

    friend bool operator==(const agl_hash_set & lhs, const agl_hash_set & rhs)
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
    friend bool operator!=(const agl_hash_set & lhs, const agl_hash_set & rhs)
    {
        return !(lhs == rhs);
    }
};

} // end namespace ska
