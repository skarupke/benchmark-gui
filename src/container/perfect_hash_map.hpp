#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <functional>

namespace ska
{

inline size_t next_power_of_two(size_t i)
{
    --i;
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    i |= i >> 32;
    ++i;
    return i;
}

template<typename K, typename V, typename H = std::hash<K>, typename A = std::allocator<unsigned char>>
class perfect_hash_table : private A, private H
{
    using value_type = std::pair<K, V>;
    using AllocatorTraits = std::allocator_traits<A>;

    template<typename It>
    struct HashAndIterator
    {
        size_t hash;
        It it;
    };
    template<typename It>
    struct BucketState
    {
        bool empty = true;
        std::vector<HashAndIterator<It>> entries;
    };

public:
    template<typename It>
    perfect_hash_table(It begin, It end)
    {
        size_t num_elements = std::distance(begin, end);
        for (size_t capacity = next_power_of_two(num_elements);; capacity *= 2)
        {
            capacity_minus_one = capacity - 1;
            std::vector<std::vector<std::pair<size_t, It>>> buckets(capacity);
            for (It it = begin; it != end; ++it)
            {
                size_t hash = hash_value(*it);
                size_t index = first_hash(hash, capacity_minus_one);
                bool already_exists = std::any_of(buckets[index].begin(), buckets[index].end(), [hash](const std::pair<size_t, It> & entry)
                {
                    return entry.first == hash;
                });
                if (!already_exists)
                    buckets[index].push_back({ hash, it });
            }
            size_t num_bytes_to_allocate = capacity * (sizeof(std::ptrdiff_t) + sizeof(value_type));
            unsigned char * allocated = AllocatorTraits::allocate(*this, num_bytes_to_allocate);
            indirection = reinterpret_cast<Indirection *>(allocated);
            for (Indirection * it = indirection, * end = indirection + capacity; it != end; ++it)
                it->contains_value = false;
            std::vector<bool> already_used(capacity);
            for (size_t i = 0; i < capacity; ++i)
            {
                auto & entries = buckets[i];
                if (entries.size() <= 1)
                    continue;

                std::ptrdiff_t indirection_value = 1;
                for (; indirection_value < 64; ++indirection_value)
                {
                    size_t handled = 0;
                    size_t handled_end = entries.size();
                    for (; handled != handled_end; ++handled)
                    {
                        size_t new_index = second_hash(entries[handled].first, indirection_value, capacity_minus_one);
                        if (already_used[new_index])
                            break;
                        already_used[new_index] = true;
                    }
                    if (handled != handled_end)
                    {
                        for (size_t j = 0; j < handled; ++j)
                        {
                            size_t new_index = second_hash(entries[j].first, indirection_value, capacity_minus_one);
                            already_used[new_index] = false;
                        }
                    }
                    else
                    {
                        indirection[i].next_index = -indirection_value;
                        break;
                    }
                }
                if (indirection_value == 64)
                {
                    AllocatorTraits::deallocate(*this, reinterpret_cast<unsigned char *>(indirection), num_bytes_to_allocate);
                    indirection = nullptr;
                    break;
                }
            }
            if (!indirection)
                continue;
            values = reinterpret_cast<value_type *>(allocated + capacity * sizeof(std::ptrdiff_t));
            size_t free_index = 0;
            for (size_t i = 0; i < capacity; ++i)
            {
                auto & entries = buckets[i];
                if (entries.empty())
                    continue;
                else if (entries.size() == 1)
                {
                    while (already_used[free_index])
                        ++free_index;
                    indirection[i].next_index = free_index;
                    indirection[free_index].contains_value = true;
                    AllocatorTraits::construct(*this, values + free_index, *entries.front().second);
                    ++_size;
                    already_used[free_index] = true;
                    ++free_index;
                }
                else
                {
                    for (const std::pair<size_t, It> & entry : entries)
                    {
                        size_t new_index = second_hash(entry.first, -indirection[i].next_index, capacity_minus_one);
                        indirection[new_index].contains_value = true;
                        AllocatorTraits::construct(*this, values + new_index, *entry.second);
                        ++_size;
                    }
                }
            }
            break;
        }
    }
    ~perfect_hash_table()
    {
        if (indirection)
        {
            for (size_t i = 0; i <= capacity_minus_one; ++i)
            {
                if (indirection[i].contains_value)
                    AllocatorTraits::destroy(*this, values + i);
            }
            AllocatorTraits::deallocate(*this, reinterpret_cast<unsigned char *>(indirection), (capacity_minus_one + 1) * (sizeof(std::ptrdiff_t) + sizeof(value_type)));
        }
    }

    V & operator[](const K & key)
    {
        size_t hash = hash_value(key);
        Indirection i = indirection[first_hash(hash, capacity_minus_one)];
        if (i.next_index < 0)
        {
            size_t second = second_hash(hash, -i.next_index, capacity_minus_one);
            return values[second].second;
        }
        else
            return values[i.next_index].second;
    }
    const V & operator[](const K & key) const
    {
        size_t hash = hash_value(key);
        Indirection i = indirection[first_hash(hash, capacity_minus_one)];
        if (i.next_index < 0)
        {
            size_t second = second_hash(hash, -i.next_index, capacity_minus_one);
            return values[second].second;
        }
        else
            return values[i.next_index].second;
    }

    const value_type * find(const K & key) const
    {
        size_t hash = hash_value(key);
        Indirection i = indirection[first_hash(hash, capacity_minus_one)];
        if (i.next_index < 0)
        {
            size_t second = second_hash(hash, -i.next_index, capacity_minus_one);
            return values + second;
        }
        else
            return values + i.next_index;
    }

    size_t size() const
    {
        return _size;
    }
    size_t capacity() const
    {
        return capacity_minus_one + 1;
    }

private:
    struct Indirection
    {
        std::ptrdiff_t next_index : 63;
        std::ptrdiff_t contains_value : 1;
    };

    Indirection * indirection = nullptr;
    value_type * values = nullptr;
    size_t _size = 0;
    size_t capacity_minus_one = 0;

    size_t hash_value(const K & k)
    {
        return static_cast<H &>(*this)(k);
    }
    size_t hash_value(const K & k) const
    {
        return static_cast<const H &>(*this)(k);
    }
    template<typename F, typename S>
    size_t hash_value(const std::pair<F, S> & p)
    {
        return static_cast<H &>(*this)(p.first);
    }
    template<typename F, typename S>
    size_t hash_value(const std::pair<F, S> & p) const
    {
        return static_cast<const H &>(*this)(p.first);
    }

    static size_t first_hash(size_t hash, size_t num_buckets_minus_one)
    {
        return hash & num_buckets_minus_one;
    }
    static size_t second_hash(size_t hash, std::ptrdiff_t indirection, size_t num_buckets_minus_one)
    {
        return (hash ^ (hash >> indirection)) & num_buckets_minus_one;
    }
};

}
