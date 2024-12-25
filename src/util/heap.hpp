#pragma once

#include <utility>
#include <cstdint>
#include <functional>
#include <vector>
#include <forward_list>
#include <memory>
#include <debug/assert.hpp>
#include <iterator>
#include <algorithm>

template<typename It, typename ValueType, typename Compare>
void heap_replace_top(It begin, It end, ValueType && value, Compare && compare)
{
    using std::swap;
    *begin = std::forward<ValueType>(value);
    std::ptrdiff_t length = end - begin;
    for (std::ptrdiff_t current = 0, first_child = 1; length > first_child; first_child = current * 2 + 1)
    {
        std::ptrdiff_t second_child = first_child + 1;
        std::ptrdiff_t larger_child = (second_child != length && compare(begin[first_child], begin[second_child])) ? second_child : first_child;
        if (!compare(begin[current], begin[larger_child]))
            return;
        swap(begin[current], begin[larger_child]);
        current = larger_child;
    }
}

namespace interval_heap_helpers
{
inline std::ptrdiff_t parent_min(std::ptrdiff_t index)
{
    return ((index - 2) / 4) * 2;
}
inline std::ptrdiff_t parent_max(std::ptrdiff_t index)
{
    return parent_min(index) + 1;
}
template<typename It, typename Compare>
void heapify_min(It begin, std::ptrdiff_t length, std::ptrdiff_t index, Compare && compare)
{
    using std::swap;
    for (;;)
    {
        std::ptrdiff_t smaller_child = index * 2 + 2;
        if (smaller_child >= length)
            break;
        std::ptrdiff_t right_child = smaller_child + 2;
        if (right_child < length && compare(begin[right_child], begin[smaller_child]))
            smaller_child = right_child;

        if (compare(begin[smaller_child], begin[index]))
        {
            swap(begin[smaller_child], begin[index]);
            std::ptrdiff_t max_sibling = smaller_child | 1;
            if (max_sibling < length && compare(begin[max_sibling], begin[smaller_child]))
                swap(begin[smaller_child], begin[max_sibling]);
            index = smaller_child;
        }
        else
            break;
    }
}

template<typename It, typename Compare>
void heapify_max(It begin, std::ptrdiff_t length, std::ptrdiff_t index, Compare && compare)
{
    using std::swap;
    for (;;)
    {
        std::ptrdiff_t larger_child = index * 2 + 1;
        if (larger_child > length)
            break;
        else if (larger_child == length)
        {
            --larger_child;
            if (compare(begin[index], begin[larger_child]))
                swap(begin[index], begin[larger_child]);
            break;
        }
        std::ptrdiff_t right_child = larger_child + 2;
        if (right_child <= length)
        {
            if (right_child == length)
                --right_child;
            if (compare(begin[larger_child], begin[right_child]))
                larger_child = right_child;
        }

        if (compare(begin[index], begin[larger_child]))
        {
            swap(begin[larger_child], begin[index]);
            std::ptrdiff_t min_sibling = larger_child & ~std::ptrdiff_t(1);
            if (compare(begin[larger_child], begin[min_sibling]))
                swap(begin[larger_child], begin[min_sibling]);
            index = larger_child;
        }
        else
            break;
    }
}
template<typename It, typename Compare>
void trickle_up_min(It begin, std::ptrdiff_t index, std::ptrdiff_t parent, Compare && compare)
{
    auto to_insert = std::move(begin[index]);
    do
    {
        begin[index] = std::move(begin[parent]);
        index = parent;
        if (index == 0)
            break;
        parent = interval_heap_helpers::parent_min(index);
    }
    while (compare(to_insert, begin[parent]));
    begin[index] = std::move(to_insert);
}
template<typename It, typename Compare>
void trickle_up_max(It begin, std::ptrdiff_t index, std::ptrdiff_t parent, Compare && compare)
{
    auto to_insert = std::move(begin[index]);
    do
    {
        begin[index] = std::move(begin[parent]);
        index = parent;
        if (index == 1)
            break;
        parent = interval_heap_helpers::parent_max(index);
    }
    while (compare(begin[parent], to_insert));
    begin[index] = std::move(to_insert);
}
}

template<typename It, typename Compare>
bool interval_heap_is_valid(It begin, It end, Compare && compare)
{
    std::ptrdiff_t length = end - begin;
    for (std::ptrdiff_t i = 1; i < length; i += 2)
    {
        if (compare(begin[i], begin[i - 1]))
            return false;
    }
    for (std::ptrdiff_t i = 2; i < length; i += 2)
    {
        std::ptrdiff_t parent = interval_heap_helpers::parent_min(i);
        if (compare(begin[i], begin[parent]))
            return false;
    }
    if ((length & 1) && length > 1)
    {
        std::ptrdiff_t parent_max = interval_heap_helpers::parent_min(length - 1) + 1;
        if (compare(begin[parent_max], end[-1]))
            return false;
    }
    for (std::ptrdiff_t i = 3; i < length; i += 2)
    {
        std::ptrdiff_t parent = interval_heap_helpers::parent_max(i);
        if (compare(begin[parent], begin[i]))
            return false;
    }
    return true;
}
template<typename It>
bool interval_heap_is_valid(It begin, It end)
{
    return interval_heap_is_valid(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void interval_heap_push(It begin, It end, Compare && compare)
{
    using std::swap;
    std::ptrdiff_t index = (end - begin) - 1;
    std::ptrdiff_t parent;
    if ((index & 1) == 0)
    {
        if (index == 0)
            return;
        parent = interval_heap_helpers::parent_min(index);
        if (compare(begin[index], begin[parent]))
        {
            interval_heap_helpers::trickle_up_min(begin, index, parent, std::forward<Compare>(compare));
            return;
        }
        ++parent;
        if (compare(begin[parent], begin[index]))
            interval_heap_helpers::trickle_up_max(begin, index, parent, std::forward<Compare>(compare));
    }
    else if (compare(begin[index], begin[index - 1]))
    {
        swap(begin[index], begin[index - 1]);
        --index;
        parent = interval_heap_helpers::parent_min(index);
        if (compare(begin[index], begin[parent]))
            interval_heap_helpers::trickle_up_min(begin, index, parent, std::forward<Compare>(compare));
    }
    else
    {
        parent = interval_heap_helpers::parent_max(index);
        if (compare(begin[parent], begin[index]))
            interval_heap_helpers::trickle_up_max(begin, index, parent, std::forward<Compare>(compare));
    }
}

template<typename It>
void interval_heap_push(It begin, It end)
{
    interval_heap_push(begin, end, std::less<>{});
}

template<typename It>
decltype(auto) interval_heap_min(It begin, It end)
{
    static_cast<void>(end);
    return *begin;
}
template<typename It>
decltype(auto) interval_heap_max(It begin, It end)
{
    if (begin + 1 == end)
        return begin[0];
    else
        return begin[1];
}

template<typename It, typename Compare>
void interval_heap_pop_min(It begin, It end, Compare && compare)
{
    using std::swap;
    std::ptrdiff_t length = (end - begin) - 1;
    if (length == 0)
        return;
    std::ptrdiff_t index = 0;
    swap(begin[index], end[-1]);
    interval_heap_helpers::heapify_min(begin, length, index, std::forward<Compare>(compare));
}

template<typename It>
void interval_heap_pop_min(It begin, It end)
{
    return interval_heap_pop_min(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void interval_heap_pop_max(It begin, It end, Compare && compare)
{
    using std::swap;
    std::ptrdiff_t length = (end - begin) - 1;
    if (length <= 1)
        return;
    std::ptrdiff_t index = 1;
    swap(begin[index], end[-1]);
    interval_heap_helpers::heapify_max(begin, length, index, std::forward<Compare>(compare));
}

template<typename It>
void interval_heap_pop_max(It begin, It end)
{
    return interval_heap_pop_max(begin, end, std::less<>{});
}

// if you have updated the min value, call this function to get
// the heap back into correct order.
template<typename It, typename Compare>
void interval_heap_min_updated(It begin, It end, Compare && compare)
{
    using std::swap;
    std::ptrdiff_t length = end - begin;
    if (length < 2)
        return;
    if (compare(begin[1], begin[0]))
        swap(begin[1], begin[0]);
    interval_heap_helpers::heapify_min(begin, length, 0, std::forward<Compare>(compare));
}

template<typename It>
void interval_heap_min_updated(It begin, It end)
{
    interval_heap_min_updated(begin, end, std::less<>{});
}

// if you have updated the max value, call this function to get
// the heap back into correct order.
template<typename It, typename Compare>
void interval_heap_max_updated(It begin, It end, Compare && compare)
{
    using std::swap;
    std::ptrdiff_t length = end - begin;
    if (length < 2)
        return;
    if (compare(begin[1], begin[0]))
        swap(begin[1], begin[0]);
    interval_heap_helpers::heapify_max(begin, length, 1, std::forward<Compare>(compare));
}

template<typename It>
void interval_heap_max_updated(It begin, It end)
{
    interval_heap_max_updated(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void interval_heap_make(It begin, It end, Compare && compare)
{
    using std::swap;
    std::ptrdiff_t length = end - begin;
    std::ptrdiff_t index = length / 2;
    for (std::ptrdiff_t i = 1; i < length; i += 2)
    {
        if (compare(begin[i], begin[i - 1]))
        {
            swap(begin[i], begin[i - 1]);
        }
    }
    switch (index & 1)
    {
    case 0:
    while (index != 0)
    {
        --index;
        [[fallthrough]];
    case 1:
        interval_heap_helpers::heapify_max(begin, length, index, compare);
        --index;
        interval_heap_helpers::heapify_min(begin, length, index, compare);
    }
    }
}

template<typename It>
void interval_heap_make(It begin, It end)
{
    interval_heap_make(begin, end, std::less<>{});
}


namespace minmax_heap_helpers
{
// returns the index of the highest set bit. undefined if no bits are set.
// examples:
// highest_set_bit(1) = 0
// highest_set_bit(4) = 2
// highest_set_bit(55) = 5
inline int highest_set_bit(uint64_t i)
{
#ifdef _MSC_VER
    unsigned long result;
    _BitScanReverse64(&result, i);
    return result;
#else
    return 63 - __builtin_clzl(i);
#endif
}

inline bool is_new_item_min(uint64_t length)
{
    return (highest_set_bit(length) & 1) == 0;
}

inline bool is_min_item(uint64_t index)
{
    return is_new_item_min(index + 1);
}

inline uint64_t grandparent_index(uint64_t index)
{
    return (index - 3) / 4;
}

inline uint64_t parent_index(uint64_t index)
{
    return (index - 1) / 2;
}

inline uint64_t first_child_index(uint64_t index)
{
    return (index * 2) + 1;
}
inline uint64_t last_grandchild_index(uint64_t index)
{
    return (index * 4) + 6;
}
template<typename It, typename Compare>
uint64_t smallest_descendant(It begin, uint64_t length, uint64_t first_child, uint64_t first_grandchild, Compare && compare)
{
    uint64_t second_child = first_child + 1;
    if (first_grandchild >= length)
        return first_child + (second_child != length && compare(begin[second_child], begin[first_child]));
    uint64_t second_grandchild = first_grandchild + 1;
    if (second_grandchild == length)
        return compare(begin[first_grandchild], begin[second_child]) ? first_grandchild : second_child;
    uint64_t min_grandchild = first_grandchild + !!compare(begin[second_grandchild], begin[first_grandchild]);
    uint64_t third_grandchild = second_grandchild + 1;
    if (third_grandchild == length)
        return compare(begin[min_grandchild], begin[second_child]) ? min_grandchild : second_child;
    else
        return compare(begin[min_grandchild], begin[third_grandchild]) ? min_grandchild : third_grandchild;
}
template<typename It, typename Compare>
uint64_t largest_descendant(It begin, uint64_t length, uint64_t first_child, uint64_t first_grandchild, Compare && compare)
{
    uint64_t second_child = first_child + 1;
    if (first_grandchild >= length)
        return first_child + (second_child != length && compare(begin[first_child], begin[second_child]));
    uint64_t second_grandchild = first_grandchild + 1;
    if (second_grandchild == length)
        return compare(begin[second_child], begin[first_grandchild]) ? first_grandchild : second_child;
    uint64_t max_grandchild = first_grandchild + !!compare(begin[first_grandchild], begin[second_grandchild]);
    uint64_t third_grandchild = second_grandchild + 1;
    if (third_grandchild == length)
        return compare(begin[second_child], begin[max_grandchild]) ? max_grandchild : second_child;
    else
        return compare(begin[max_grandchild], begin[third_grandchild]) ? third_grandchild : max_grandchild;
}

template<typename It, typename Compare>
void push_down_min(It begin, typename std::iterator_traits<It>::value_type value, uint64_t index, uint64_t length, Compare && compare)
{
    using std::swap;
    for (;;)
    {
        uint64_t last_grandchild = last_grandchild_index(index);
        if (last_grandchild < length)
        {
            auto it = begin + last_grandchild;
            uint64_t min_first_half = last_grandchild - 2 - !!compare(it[-3], it[-2]);
            uint64_t min_second_half = last_grandchild - !!compare(it[-1], it[0]);
            uint64_t smallest = compare(begin[min_second_half], begin[min_first_half]) ? min_second_half : min_first_half;
            if (!compare(begin[smallest], value))
                break;
            begin[index] = std::move(begin[smallest]);
            index = smallest;
            uint64_t parent = parent_index(index);
            if (compare(begin[parent], value))
                swap(begin[parent], value);
        }
        else
        {
            uint64_t first_child = first_child_index(index);
            if (first_child >= length)
                break;
            uint64_t first_grandchild = last_grandchild - 3;
            uint64_t smallest = smallest_descendant(begin, length, first_child, first_grandchild, compare);
            if (!compare(begin[smallest], value))
                break;
            begin[index] = std::move(begin[smallest]);
            index = smallest;
            if (smallest < first_grandchild)
                break;
            uint64_t parent = parent_index(index);
            if (compare(begin[parent], value))
            {
                begin[index] = std::move(begin[parent]);
                index = parent;
            }
            break;
        }
    }
    begin[index] = std::move(value);
}

template<typename It, typename Compare>
void push_down_min_one_child_only(It begin, uint64_t index, Compare&& compare)
{
    using std::swap;
    uint64_t child = first_child_index(index);
    if (compare(begin[child], begin[index]))
        swap(begin[index], begin[child]);
}

template<typename It, typename Compare>
void push_down_min_one_level_only(It begin, uint64_t index, Compare&& compare)
{
    using std::swap;
    uint64_t first_child = first_child_index(index);
    uint64_t smaller_child = first_child + !!compare(begin[first_child + 1], begin[first_child]);
    if (compare(begin[smaller_child], begin[index]))
        swap(begin[index], begin[smaller_child]);
}

template<typename It, typename Compare>
void push_down_max(It begin, typename std::iterator_traits<It>::value_type value, uint64_t index, uint64_t length, Compare&& compare)
{
    using std::swap;
    for (;;)
    {
        uint64_t last_grandchild = last_grandchild_index(index);
        if (last_grandchild < length)
        {
            auto it = begin + last_grandchild;
            uint64_t max_first_half = last_grandchild - 2 - !!compare(it[-2], it[-3]);
            uint64_t max_second_half = last_grandchild - !!compare(it[0], it[-1]);
            uint64_t largest = compare(begin[max_first_half], begin[max_second_half]) ? max_second_half : max_first_half;
            if (!compare(value, begin[largest]))
                break;
            begin[index] = std::move(begin[largest]);
            index = largest;
            uint64_t parent = parent_index(index);
            if (compare(value, begin[parent]))
                swap(begin[parent], value);
        }
        else
        {
            uint64_t first_child = first_child_index(index);
            if (first_child >= length)
                break;
            uint64_t first_grandchild = last_grandchild - 3;
            uint64_t largest = largest_descendant(begin, length, first_child, first_grandchild, compare);
            if (!compare(value, begin[largest]))
                break;
            begin[index] = std::move(begin[largest]);
            index = largest;
            if (largest < first_grandchild)
                break;
            uint64_t parent = parent_index(index);
            if (compare(value, begin[parent]))
            {
                begin[index] = std::move(begin[parent]);
                index = parent;
            }
            break;
        }
    }
    begin[index] = std::move(value);
}

template<typename It, typename Compare>
void push_down_max_one_child_only(It begin, uint64_t index, Compare&& compare)
{
    using std::swap;
    uint64_t child = first_child_index(index);
    if (compare(begin[index], begin[child]))
        swap(begin[index], begin[child]);
}

template<typename It, typename Compare>
void push_down_max_one_level_only(It begin, uint64_t index, Compare&& compare)
{
    using std::swap;
    uint64_t first_child = first_child_index(index);
    uint64_t bigger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
    if (compare(begin[index], begin[bigger_child]))
        swap(begin[index], begin[bigger_child]);
}

}


template<typename It, typename Compare>
bool is_minmax_heap(It begin, It end, Compare&& compare)
{
    uint64_t length = static_cast<uint64_t>(end - begin);
    auto test_index = [](uint64_t index, auto compare_index)
    {
        uint64_t first_child = minmax_heap_helpers::first_child_index(index);
        uint64_t second_child = first_child + 1;
        uint64_t first_grandchild = minmax_heap_helpers::first_child_index(first_child);
        uint64_t second_grandchild = first_grandchild + 1;
        uint64_t third_grandchild = minmax_heap_helpers::first_child_index(second_child);
        uint64_t fourth_grandchild = third_grandchild + 1;
        return compare_index(first_child) && compare_index(second_child)
            && compare_index(first_grandchild) && compare_index(second_grandchild)
            && compare_index(third_grandchild) && compare_index(fourth_grandchild);
    };
    for (uint64_t i = 0; i < length; ++i)
    {
        if (minmax_heap_helpers::is_min_item(i))
        {
            auto compare_one = [&](uint64_t child)
            {
                return child >= length || !compare(begin[child], begin[i]);
            };
            if (!test_index(i, compare_one))
                return false;
        }
        else
        {
            auto compare_one = [&](uint64_t child)
            {
                return child >= length || !compare(begin[i], begin[child]);
            };
            if (!test_index(i, compare_one))
                return false;
        }
    }
    return true;
}
template<typename It>
bool is_minmax_heap(It begin, It end)
{
    return is_minmax_heap(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void push_minmax_heap(It begin, It end, Compare&& compare)
{
    uint64_t length = static_cast<uint64_t>(end - begin);
    uint64_t index = length - 1;
    uint64_t parent = minmax_heap_helpers::parent_index(index);
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    if (minmax_heap_helpers::is_new_item_min(length))
    {
        if (index == 0)
            static_cast<void>(0);
        else if (compare(begin[parent], value))
        {
            begin[index] = std::move(begin[parent]);
            index = parent;
            goto push_up_max;
        }
        else
        {
            for (;;)
            {
                {
                    uint64_t grandparent = minmax_heap_helpers::grandparent_index(index);
                    if (compare(value, begin[grandparent]))
                    {
                        begin[index] = std::move(begin[grandparent]);
                        index = grandparent;
                    }
                    else
                        break;
                }
push_up_min:
                if (!index)
                    break;
            }
        }
    }
    else if (compare(value, begin[parent]))
    {
        begin[index] = std::move(begin[parent]);
        index = parent;
        goto push_up_min;
    }
    else
    {
push_up_max:
        while (index > 2)
        {
            uint64_t grandparent = minmax_heap_helpers::grandparent_index(index);
            if (compare(begin[grandparent], value))
            {
                begin[index] = std::move(begin[grandparent]);
                index = grandparent;
            }
            else
                break;
        }
    }
    begin[index] = std::move(value);
}
template<typename It>
void push_minmax_heap(It begin, It end)
{
    push_minmax_heap(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void pop_minmax_heap_min(It begin, It end, Compare&& compare)
{
    uint64_t length = static_cast<uint64_t>(end - begin) - 1;
    if (length == 0)
        return;
    minmax_heap_helpers::push_down_min(begin, std::exchange(end[-1], std::move(begin[0])), 0, length, compare);
}

template<typename It>
void pop_minmax_heap_min(It begin, It end)
{
    pop_minmax_heap_min(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void pop_minmax_heap_max(It begin, It end, Compare&& compare)
{
    uint64_t length = static_cast<uint64_t>(end - begin) - 1;
    if (length <= 1)
        return;

    uint64_t index = 1 + !!compare(begin[1], begin[2]);
    minmax_heap_helpers::push_down_max(begin, std::exchange(end[-1], std::move(begin[index])), index, length, std::forward<Compare>(compare));
}
template<typename It>
void pop_minmax_heap_max(It begin, It end)
{
    pop_minmax_heap_max(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void make_minmax_heap(It begin, It end, Compare && compare)
{
    uint64_t length = end - begin;
    uint64_t index = length / 2;
    if (index == 0)
        return;
    // optimization: there can be only one item that has only one child
    // handling that item up front simplifies the second loop a little, since
    // we know that all other items have two children
    if ((length & 1) == 0)
    {
        --index;
        if (minmax_heap_helpers::is_min_item(index))
            minmax_heap_helpers::push_down_min_one_child_only(begin, index, compare);
        else
            minmax_heap_helpers::push_down_max_one_child_only(begin, index, compare);
        if (index == 0)
            return;
    }
    // optimization: half of all the items will have no grandchildren. this
    // simplifies the push_down function a lot, so we handle these items
    // first. we could then do another optimization where we know that
    // after the first half, the next quarter of items has grandchildren but
    // no great-grandchildren, but the code is already big enough
    if (length != 4)
    {
        uint64_t lowest_index_with_no_grandchildren = length / 4;
        for (;;)
        {
            int highest_bit = minmax_heap_helpers::highest_set_bit(index);
            uint64_t loop_until = std::max(lowest_index_with_no_grandchildren, (static_cast<uint64_t>(1) << highest_bit) - 1);
            --index;
            if (highest_bit & 1)
            {
                for (;; --index)
                {
                    minmax_heap_helpers::push_down_max_one_level_only(begin, index, compare);
                    if (index == loop_until)
                        break;
                }
            }
            else
            {
                for (;; --index)
                {
                    minmax_heap_helpers::push_down_min_one_level_only(begin, index, compare);
                    if (index == loop_until)
                        break;
                }
                if (index == 0)
                    return;
            }
            if (index == lowest_index_with_no_grandchildren)
                break;
        }
    }
    int highest_bit = minmax_heap_helpers::highest_set_bit(index);
    uint64_t loop_until = (static_cast<uint64_t>(1) << highest_bit) - 1;
    switch (highest_bit & 1)
    {
        for (;;)
        {
    case 0:
            for (;;)
            {
                --index;
                minmax_heap_helpers::push_down_min(begin, std::move(begin[index]), index, length, compare);
                if (index == loop_until)
                    break;
            }
            if (index == 0)
                return;
            loop_until /= 2;
            [[fallthrough]];
    case 1:
            for (;;)
            {
                --index;
                minmax_heap_helpers::push_down_max(begin, std::move(begin[index]), index, length, compare);
                if (index == loop_until)
                    break;
            }
            loop_until /= 2;
        }
    }
}
template<typename It>
void make_minmax_heap(It begin, It end)
{
    return make_minmax_heap(begin, end, std::less<>{});
}

namespace dary_heap_helpers
{
template<int D>
uint64_t first_child_index(uint64_t index)
{
    return index * D + 1;
}
template<int D>
uint64_t last_child_index(uint64_t index)
{
    return index * D + D;
}
template<int D>
uint64_t last_grandchild_index(uint64_t index)
{
    return index * (D * D) + (D * D + D);
}
template<int D>
uint64_t parent_index(uint64_t index)
{
    return (index - 1) / D;
}
template<int D>
uint64_t grandparent_index(uint64_t index)
{
    return (index - (D + 1)) / (D * D);
}
template<int D>
uint64_t great_grandparent_index(uint64_t index)
{
    return (index - (D * D + D + 1)) / (D * D * D);
}
template<int D>
uint64_t great_great_grandparent_index(uint64_t index)
{
    return (index - (D * D * D + D * D + D + 1)) / (D * D * D * D);
}
template<int D>
uint64_t index_with_no_grandchild(uint64_t length)
{
    return grandparent_index<D>(length - 1) + 1;
}
template<int D, typename It, typename Compare>
inline It largest_child(It first_child_it, Compare && compare)
{
    if constexpr (D == 1)
        return first_child_it;
    else if constexpr (D == 2)
        return first_child_it + !!compare(first_child_it[0], first_child_it[1]);
    else
    {
        It first_half_largest = largest_child<D / 2>(first_child_it, compare);
        It second_half_largest = largest_child<D - D / 2>(first_child_it + D / 2, compare);
        return compare(*first_half_largest, *second_half_largest) ? second_half_largest : first_half_largest;
    }
}
template<int D, typename It, typename Compare>
inline It largest_child_linear(It first_child_it, Compare && compare)
{
    if constexpr (D == 1)
        return first_child_it;
    else if constexpr (D == 2)
        return first_child_it + !!compare(first_child_it[0], first_child_it[1]);
    else if constexpr (D == 3)
    {
        It largest = first_child_it + !!compare(first_child_it[0], first_child_it[1]);
        first_child_it += 2;
        if (compare(*largest, *first_child_it))
            largest = first_child_it;
        return largest;
    }
    else if constexpr (D == 4)
    {
        It largest = first_child_it + !!compare(first_child_it[0], first_child_it[1]);
        first_child_it += 2;
        if (compare(*largest, *first_child_it))
            largest = first_child_it;
        ++first_child_it;
        if (compare(*largest, *first_child_it))
            largest = first_child_it;
        return largest;
    }
    else
    {
        It largest = first_child_it + !!compare(first_child_it[0], first_child_it[1]);
        It end = first_child_it + D;
        first_child_it += 2;
        for (;;)
        {
            if (compare(*largest, *first_child_it))
                largest = first_child_it;
            ++first_child_it;
            if (first_child_it == end)
                break;
        }
        return largest;
    }
}
template<int D, typename It, typename Compare>
It largest_child(It first_child_it, int num_children, Compare && compare)
{
    if constexpr (D == 2)
        return first_child_it;
    else if constexpr (D == 3)
    {
        if (num_children == 1)
            return first_child_it;
        else
            return first_child_it + !!compare(first_child_it[0], first_child_it[1]);
    }
    else if constexpr (D == 4)
    {
        switch (num_children)
        {
        case 1: return first_child_it;
        case 2: return first_child_it + !!compare(first_child_it[0], first_child_it[1]);
        default:
            It largest = first_child_it + !!compare(first_child_it[0], first_child_it[1]);
            return compare(*largest, first_child_it[2]) ? first_child_it + 2 : largest;
        }
    }
    else
    {
        switch(num_children)
        {
        case 1: return first_child_it;
        case 2: return first_child_it + !!compare(first_child_it[0], first_child_it[1]);
        case 3:
        {
            It largest = first_child_it + !!compare(first_child_it[0], first_child_it[1]);
            return compare(*largest, first_child_it[2]) ? first_child_it + 2 : largest;
        }
        case 4:
        {
            It largest_first_half = first_child_it + !!compare(first_child_it[0], first_child_it[1]);
            It largest_second_half = first_child_it + 2 + !!compare(first_child_it[2], first_child_it[3]);
            return compare(*largest_first_half, *largest_second_half) ? largest_second_half : largest_first_half;
        }
        default:
            int half = num_children / 2;
            It first_half_largest = largest_child<D>(first_child_it, half, compare);
            It second_half_largest = largest_child<D>(first_child_it + half, num_children - half, compare);
            return compare(*first_half_largest, *second_half_largest) ? second_half_largest : first_half_largest;
        }
    }
}


template<int D, typename It, typename Compare>
void trickle_down(It begin, size_t length, size_t index, Compare && compare)
{
    typename std::iterator_traits<It>::value_type value = std::move(begin[index]);
    for (;;)
    {
        uint64_t last_child = dary_heap_helpers::last_child_index<D>(index);
        uint64_t first_child = last_child - (D - 1);
        if (last_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child<D>(begin + first_child, compare);
            if (!compare(value, *largest_child))
                break;
            begin[index] = std::move(*largest_child);
            index = largest_child - begin;
        }
        else if (first_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child<D>(begin + first_child, length - first_child, compare);
            if (compare(value, *largest_child))
            {
                begin[index] = std::move(*largest_child);
                index = largest_child - begin;
            }
            break;
        }
        else
            break;
    }
    begin[index] = std::move(value);
}
template<int D, typename It, typename Compare>
void heapify(It begin, It to_update, It end, Compare && compare)
{
    using std::swap;
    size_t index = to_update - begin;
    if (index)
    {
        size_t parent = dary_heap_helpers::parent_index<D>(index);
        if (compare(begin[parent], *to_update))
        {
            do
            {
                swap(begin[parent], begin[index]);
                index = parent;
                if (index == 0)
                    break;
                parent = dary_heap_helpers::parent_index<D>(index);
            }
            while (compare(begin[parent], begin[index]));
            return;
        }
    }
    trickle_down<D>(begin, end - begin, index, compare);
}

template<int D, typename It, typename Compare>
void remove_from_heap(It begin, It to_remove, It end, Compare && compare)
{
    std::iter_swap(to_remove, end - 1);
    heapify<D>(begin, to_remove, end, compare);
}
}

template<int D, typename It, typename Compare>
void make_dary_heap(It begin, It end, Compare && compare)
{
    using std::swap;
    uint64_t length = end - begin;
    if (length <= 1)
        return;
    uint64_t index = (length - 2) / D;
    // optimization: there can be only one item that has fewer than D children
    // handling that item up front simplifies the second loop a little, since
    // we know that all other items have two children
    int num_children_end = (length - 1) % D;
    if (num_children_end)
    {
        It largest_child = dary_heap_helpers::largest_child<D>(begin + dary_heap_helpers::first_child_index<D>(index), num_children_end, compare);
        if (compare(begin[index], *largest_child))
            swap(begin[index], *largest_child);
        if (index == 0)
            return;
        --index;
    }
    // optimization: half of all the items will have no grandchildren. this
    // simplifies the push_down function a lot, so we handle these items
    // first. we could then do another optimization where we know that
    // after the first half, the next quarter of items has grandchildren but
    // no great-grandchildren, but the code is already big enough
    if (index > 0)
    {
        uint64_t lowest_index_with_no_grandchildren = dary_heap_helpers::index_with_no_grandchild<D>(length);
        for (;;)
        {
            It largest_child = dary_heap_helpers::largest_child<D>(begin + dary_heap_helpers::first_child_index<D>(index), compare);
            if (compare(begin[index], *largest_child))
                swap(begin[index], *largest_child);
            if (index-- == lowest_index_with_no_grandchildren)
                break;
        }
    }
    for (;; --index)
    {
        typename std::iterator_traits<It>::value_type value = std::move(begin[index]);
        uint64_t move_down_index = index;
        for (;;)
        {
            uint64_t last_child_index = dary_heap_helpers::last_child_index<D>(move_down_index);
            uint64_t first_child_index = last_child_index - (D - 1);
            It largest_child = begin;
            if (last_child_index < length)
                largest_child = dary_heap_helpers::largest_child<D>(begin + first_child_index, compare);
            else if (first_child_index >= length)
                break;
            else
                largest_child = dary_heap_helpers::largest_child<D>(begin + first_child_index, length - first_child_index, compare);
            if (!compare(value, *largest_child))
                break;
            begin[move_down_index] = std::move(*largest_child);
            move_down_index = largest_child - begin;
        }
        begin[move_down_index] = std::move(value);
        if (index == 0)
            break;
    }
}
template<int D, typename It>
void make_dary_heap(It begin, It end)
{
    make_dary_heap<D>(begin, end, std::less<>{});
}

template<int D, typename It, typename Compare>
bool is_dary_heap(It begin, It end, Compare && compare)
{
    uint64_t length = end - begin;
    for (uint64_t i = 1; i < length; ++i)
    {
        uint64_t parent = dary_heap_helpers::parent_index<D>(i);
        if (compare(begin[parent], begin[i]))
            return false;
    }
    return true;
}
template<int D, typename It>
bool is_dary_heap(It begin, It end)
{
    return is_dary_heap<D>(begin, end, std::less<>{});
}

template<int D, typename It, typename Compare>
void push_dary_heap(It begin, It end, Compare && compare)
{
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    uint64_t index = (end - begin) - 1;
    while (index > 0)
    {
        uint64_t parent = dary_heap_helpers::parent_index<D>(index);
        if (!compare(begin[parent], value))
            break;
        begin[index] = std::move(begin[parent]);
        index = parent;
    }
    begin[index] = std::move(value);
}

template<int D, typename It>
void push_dary_heap(It begin, It end)
{
    return push_dary_heap<D>(begin, end, std::less<>{});
}

template<int D, typename It, typename Compare>
void push_dary_heap_grandparent(It begin, It end, Compare && compare)
{
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    uint64_t index = (end - begin) - 1;
    uint64_t parent;
    while (index > D)
    {
        uint64_t grandparent = dary_heap_helpers::grandparent_index<D>(index);
        parent = dary_heap_helpers::parent_index<D>(index);
        if (!compare(begin[grandparent], value))
            goto last_loop_iter;
        begin[index] = std::move(begin[parent]);
        begin[parent] = std::move(begin[grandparent]);
        index = grandparent;
    }
    if (index > 0)
    {
        parent = dary_heap_helpers::parent_index<D>(index);
last_loop_iter:
        if (compare(begin[parent], value))
        {
            begin[index] = std::move(begin[parent]);
            index = parent;
        }
    }
    begin[index] = std::move(value);
}

template<int D, typename It>
void push_dary_heap_grandparent(It begin, It end)
{
    return push_dary_heap_grandparent<D>(begin, end, std::less<>{});
}

template<int D, typename It, typename Compare>
void push_dary_heap_great_grandparent(It begin, It end, Compare && compare)
{
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    uint64_t index = (end - begin) - 1;
    uint64_t grandparent;
    uint64_t parent;
    while (index > D * D + D)
    {
        uint64_t great_grandparent = dary_heap_helpers::great_grandparent_index<D>(index);
        grandparent = dary_heap_helpers::grandparent_index<D>(index);
        parent = dary_heap_helpers::parent_index<D>(index);
        if (!compare(begin[great_grandparent], value))
            goto check_grandparent;
        begin[index] = std::move(begin[parent]);
        begin[parent] = std::move(begin[grandparent]);
        begin[grandparent] = std::move(begin[great_grandparent]);
        index = great_grandparent;
    }
    if (index > D)
    {
        grandparent = dary_heap_helpers::grandparent_index<D>(index);
        parent = dary_heap_helpers::parent_index<D>(index);
check_grandparent:
        if (!compare(begin[grandparent], value))
            goto check_parent;
        begin[index] = std::move(begin[parent]);
        begin[parent] = std::move(begin[grandparent]);
        index = grandparent;
    }
    if (index > 0)
    {
        parent = dary_heap_helpers::parent_index<D>(index);
check_parent:
        if (compare(begin[parent], value))
        {
            begin[index] = std::move(begin[parent]);
            index = parent;
        }
    }
    begin[index] = std::move(value);
}

template<int D, typename It>
void push_dary_heap_great_grandparent(It begin, It end)
{
    return push_dary_heap_great_grandparent<D>(begin, end, std::less<>{});
}

template<int D, typename It, typename Compare>
void push_dary_heap_great_great_grandparent(It begin, It end, Compare && compare)
{
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    uint64_t index = (end - begin) - 1;
    uint64_t parent;
    uint64_t grandparent;
    uint64_t great_grandparent;
    while (index > D * D * D + D * D + D)
    {
        uint64_t great_great_grandparent = dary_heap_helpers::great_great_grandparent_index<D>(index);
        parent = dary_heap_helpers::parent_index<D>(index);
        grandparent = dary_heap_helpers::grandparent_index<D>(index);
        great_grandparent = dary_heap_helpers::great_grandparent_index<D>(index);
        if (!compare(begin[great_great_grandparent], value))
            goto check_great_grandparent;
        begin[index] = std::move(begin[parent]);
        begin[parent] = std::move(begin[grandparent]);
        begin[grandparent] = std::move(begin[great_grandparent]);
        begin[great_grandparent] = std::move(begin[great_great_grandparent]);
        index = great_great_grandparent;
    }
    if (index > D * D + D)
    {
        great_grandparent = dary_heap_helpers::great_grandparent_index<D>(index);
        parent = dary_heap_helpers::parent_index<D>(index);
        grandparent = dary_heap_helpers::grandparent_index<D>(index);
check_great_grandparent:
        if (!compare(begin[great_grandparent], value))
            goto check_grandparent;
        begin[index] = std::move(begin[parent]);
        begin[parent] = std::move(begin[grandparent]);
        begin[grandparent] = std::move(begin[great_grandparent]);
        index = great_grandparent;
    }
    if (index > D)
    {
        grandparent = dary_heap_helpers::grandparent_index<D>(index);
        parent = dary_heap_helpers::parent_index<D>(index);
check_grandparent:
        if (!compare(begin[grandparent], value))
            goto check_parent;
        begin[index] = std::move(begin[parent]);
        begin[parent] = std::move(begin[grandparent]);
        index = grandparent;
    }
    if (index > 0)
    {
        parent = dary_heap_helpers::parent_index<D>(index);
check_parent:
        if (compare(begin[parent], value))
        {
            begin[index] = std::move(begin[parent]);
            index = parent;
        }
    }
    begin[index] = std::move(value);
}

template<int D, typename It>
void push_dary_heap_great_great_grandparent(It begin, It end)
{
    return push_dary_heap_great_great_grandparent<D>(begin, end, std::less<>{});
}

// num_ancestors = 1
// step = 1
// to_subtract = 1
// first case: num_ancestors -= step = 0
//   -> next step: 0
//   -> next to_subtract: 0
// second_case: num_ancestors = step - 1 = 0
//   -> next step: 0
//   -> next to_subtract: 0
//
// num_ancestors = 2
// step = 1
// to_subtract = 1
// first case: num_ancestors -= step = 1
//   -> next step: 1
//   -> next to_subtract: 1
// second_case: num_ancestprs = step - 1 = 0
//   -> next step: 0
//   -> next to_subtract: 0
//
// num_ancestors: 3
// step = 2
// to_subtract = 3
// first case: num_ancestors -= step = 1
//   -> next step: 1
//   -> next to_subtract: 1
// second_case: num_ancestors = step - 1 = 1
//   -> next step: 1
//   -> next to_subtract: 1
//
// num_ancestors: 4
// step = 2
// to_subtract = 3
// first case: num_ancestors -= step = 2
//   -> next step: 1
//   -> next to_subtract: 1
// second case: num_ancestors = step - 1 = 1
//   -> next step: 1
//   -> next to_subtract: 1
//
// num_ancestors: 5
// step = 3
// to_subtract = 7
// first case: num_ancestors -= step = 2
//   -> next step: 1
//   -> next to_subtract: 1
// second case: num_ancestors = step - 1 = 2
//   -> next step: 1
//   -> next to_subtract: 1
//
// num_ancestors: 6
// step = 3
// to_subtract = 7
// first case: num_ancestors -= step = 3
//   -> next step: 2
//   -> next to_subtract: 3
// second case: num_ancestors = step - 1 = 2
//   -> next step: 1
//   -> next to_subtract: 1
//
// num_ancestors: 7
// step = 4
// to_subtract = 15
// first case: num_ancestors -= step = 3
//   -> next step: 2
//   -> next to_subtract: 3
// second case: num_ancestors = step - 1 = 3
//   -> next step: 2
//   -> next to_subtract: 3
//
// num_ancestors: 8
// step = 4
// to_subtract = 15
// first case: num_ancestors -= step = 4
//   -> next step: 2
//   -> next to_subtract: 3
// second case: num_ancestors = step - 1 = 3
//   -> next step: 2
//   -> next to_subtract: 3
//
// num_ancestors: 9
// step = 5
// to_subtract = 31
// first case: num_ancestors -= step = 4
//   -> next step: 2
//   -> next to_subtract: 3
// second case: num_ancestors = step - 1 = 4
//   -> next step: 2
//   -> next to_subtract: 3
//
// num_ancestors: 10
// step = 5
// to_subtract = 31
// first case: num_ancestors -= step = 5
//   -> next step: 3
//   -> next to_subtract: 7
// second case: num_ancestors = step - 1 = 4
//   -> next step: 2
//   -> next to_subtract: 3
//
// num_ancestors: 11
// step = 6
// to_subtract = 63
// first case: num_ancestors -= step = 5
//   -> next step: 3
//   -> next to_subtract: 7
// second case: num_ancestors = step - 1 = 5
//   -> next step: 3
//   -> next to_subtract: 7
//
// num_ancestors: 12
// step = 6
// to_subtract = 63
// first case: num_ancestors -= step = 6
//   -> next step: 3
//   -> next to_subtract: 7
// second case: num_ancestors = step - 1 = 5
//   -> next step: 3
//   -> next to_subtract: 7
//
// num_ancestors: 13
// step = 7
// to_subtract = 127
// first case: num_ancestors -= step = 6
//   -> next step: 3
//   -> next to_subtract: 7
// second case: num_ancestors = step - 1 = 6
//   -> next step: 3
//   -> next to_subtract: 7
//
// num_ancestors: 14
// step = 7
// to_subtract = 127
// first case: num_ancestors -= step = 7
//   -> next step: 4
//   -> next to_subtract: 15
// second case: num_ancestors = step - 1 = 6
//   -> next step: 3
//   -> next to_subtract: 7
//
// num_ancestors: 15
// step = 8
// to_subtract = 255
// first case: num_ancestors -= step = 7
//   -> next step: 4
//   -> next to_subtract: 15
// second case: num_ancestors = step - 1 = 7
//   -> next step: 4
//   -> next to_subtract: 15
//
// num_ancestors: 16
// step = 8
// to_subtract = 255
// first case: num_ancestors -= step = 8
//   -> next step: 4
//   -> next to_subtract: 15
// second case: num_ancestors = step - 1 = 7
//   -> next step: 4
//   -> next to_subtract: 15
//
// num_ancestors: 17
// step = 9
// to_subtract = 511
// first case: num_ancestors -= step = 8
//   -> next step: 4
//   -> next to_subtract: 15
// second case: num_ancestors = step - 1 = 8
//   -> next step: 4
//   -> next to_subtract: 15
//
// num_ancestors: 18
// step = 9
// to_subtract = 511
// first case: num_ancestors -= step = 9
//   -> next step: 5
//   -> next to_subtract: 31
// second case: num_ancestors = step - 1 = 8
//   -> next step: 4
//   -> next to_subtract: 15
//
//
// num_ancestors: 1 = 0b1
// step = 1                   Y
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 2 = 0b10
// step = 1
// num_ancestors -= step = 1
// next step = 1              X
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 3 = 0b11
// step = 2                   Y
// num_ancestors -= step = 1
// next step = 1
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 4 = 0b100
// step = 2
// num_ancestors -= step = 2
// next step = 1
// num_ancestors -= step = 1
// next step = 1              X
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 5 = 0b101
// step = 3                   Y
// num_ancestors -= step = 2
// next step = 1
// num_ancestors -= step = 1
// next step = 1              X
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 6 = 0b110
// step = 3
// num_ancestors -= step = 3
// next step = 2              X
// num_ancestors -= step = 1
// next step = 1
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 7 = 0b111
// step = 4                   Y
// num_ancestors -= step = 3
// next step = 2
// num_ancestors -= step = 1
// next step = 1
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 8 = 0b1000
// step = 4
// num_ancestors -= step = 4
// next step = 2
// num_ancestors -= step = 2
// next step = 1
// num_ancestors -= step = 1
// next step = 1              X
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 9 = 0b1001
// step = 5                   Y
// num_ancestors -= step = 4
// next step = 2
// num_ancestors -= step = 2
// next step = 1
// num_ancestors -= step = 1
// next step = 1              X
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 10 = 0b1010
// step = 5
// num_ancestors -= step = 5
// next step = 3              X
// num_ancestors -= step = 2
// next step = 1
// num_ancestors -= step = 1
// next step = 1              X
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 11 = 0b1011
// step = 6                   Y
// num_ancestors -= step = 5
// next step = 3
// num_ancestors -= step = 2
// next step = 1
// num_ancestors -= step = 1
// next step = 1              X
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 12 = 0b1100
// step = 6
// num_ancestors -= step = 6
// next step = 3
// num_ancestors -= step = 2
// next step = 1
// num_ancestors -= step = 1
// next step = 1              X
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 13 = 0b1101
// step = 7                   Y
// num_ancestors -= step = 6
// next step = 3
// num_ancestors -= step = 2
// next step = 1
// num_ancestors -= step = 1
// next step = 1              X
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 14 = 0b1110
// step = 7
// num_ancestors -= step = 7
// next step = 4              X
// num_ancestors -= step = 3
// next step = 2
// num_ancestors -= step = 1
// next step = 1
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 15 = 0b1111
// step = 8                   Y
// num_ancestors -= step = 7
// next step = 4
// num_ancestors -= step = 3
// next step = 2
// num_ancestors -= step = 1
// next step = 1
// num_ancestors -= step = 0
// next step = 0
//
// num_ancestors: 16 = 0b10000
// step = 8
// num_ancestors -= step = 8
// next step = 4
// num_ancestors -= step = 4
// next step = 2
// num_ancestors -= step = 2
// next step = 1
// num_ancestors -= step = 1
// next step = 1              X
// num_ancestors -= step = 0
// next step = 0
template<typename It, typename Compare>
void push_binary_heap_binary_search(It begin, It end, Compare && compare)
{
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    uint64_t size = end - begin;
    uint64_t index = size - 1;
    int num_ancestors = minmax_heap_helpers::highest_set_bit(size);
    int step = (num_ancestors + 1) / 2;
    while (num_ancestors)
    {
        uint64_t to_subtract = (static_cast<uint64_t>(1) << step) - 1;
        uint64_t ancestor = (index - to_subtract) >> step;
        if (compare(begin[ancestor], value))
        {
            for (int i = 0; i < step; ++i)
            {
                uint64_t parent_index = dary_heap_helpers::parent_index<2>(index);
                begin[index] = std::move(begin[parent_index]);
                index = parent_index;
            }
            num_ancestors -= step;
            step = num_ancestors + 1;
        }
        else
            num_ancestors = step - 1;
        step /= 2;
    }
    begin[index] = std::move(value);
}

template<typename It>
void push_binary_heap_binary_search(It begin, It end)
{
    return push_binary_heap_binary_search(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void push_binary_heap_binary_search_plus_one(It begin, It end, Compare && compare)
{
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    uint64_t size = end - begin;
    uint64_t index = size;
    int num_ancestors = minmax_heap_helpers::highest_set_bit(size);
    int step = (num_ancestors + 1) / 2;
    while (num_ancestors)
    {
        uint64_t to_subtract = static_cast<uint64_t>(1) << step;
        uint64_t ancestor = (index - to_subtract) >> step;
        if (compare(begin[ancestor], value))
        {
            --index;
            for (int i = 0; i < step; ++i)
            {
                uint64_t parent_index = dary_heap_helpers::parent_index<2>(index);
                begin[index] = std::move(begin[parent_index]);
                index = parent_index;
            }
            ++index;
            num_ancestors -= step;
            step = num_ancestors + 1;
        }
        else
            num_ancestors = step - 1;
        step /= 2;
    }
    begin[index - 1] = std::move(value);
}

template<typename It>
void push_binary_heap_binary_search_plus_one(It begin, It end)
{
    return push_binary_heap_binary_search_plus_one(begin, end, std::less<>{});
}

template<int D, typename It, typename Compare>
void pop_dary_heap(It begin, It end, Compare && compare)
{
    uint64_t length = (end - begin) - 1;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    uint64_t index = 0;
    for (;;)
    {
        uint64_t last_child = dary_heap_helpers::last_child_index<D>(index);
        uint64_t first_child = last_child - (D - 1);
        if (last_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child<D>(begin + first_child, compare);
            if (!compare(value, *largest_child))
                break;
            begin[index] = std::move(*largest_child);
            index = largest_child - begin;
        }
        else if (first_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child<D>(begin + first_child, length - first_child, compare);
            if (compare(value, *largest_child))
            {
                begin[index] = std::move(*largest_child);
                index = largest_child - begin;
            }
            break;
        }
        else
            break;
    }
    begin[index] = std::move(value);
}
template<int D, typename It>
void pop_dary_heap(It begin, It end)
{
    return pop_dary_heap<D>(begin, end, std::less<>{});
}

template<int D, typename It, typename Compare>
void pop_dary_heap_linear(It begin, It end, Compare && compare)
{
    uint64_t length = (end - begin) - 1;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    uint64_t index = 0;
    for (;;)
    {
        uint64_t last_child = dary_heap_helpers::last_child_index<D>(index);
        uint64_t first_child = last_child - (D - 1);
        if (last_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child_linear<D>(begin + first_child, compare);
            if (!compare(value, *largest_child))
                break;
            begin[index] = std::move(*largest_child);
            index = largest_child - begin;
        }
        else if (first_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child<D>(begin + first_child, length - first_child, compare);
            if (compare(value, *largest_child))
            {
                begin[index] = std::move(*largest_child);
                index = largest_child - begin;
            }
            break;
        }
        else
            break;
    }
    begin[index] = std::move(value);
}
template<int D, typename It>
void pop_dary_heap_linear(It begin, It end)
{
    return pop_dary_heap_linear<D>(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void pop_binary_heap_grandparent(It begin, It end, Compare && compare)
{
    uint64_t length = (end - begin) - 1;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    uint64_t index = 0;
    for (;;)
    {
        uint64_t last_child = dary_heap_helpers::last_child_index<2>(index);
        uint64_t first_child = last_child - 1;
        uint64_t last_grandchild = dary_heap_helpers::last_grandchild_index<2>(index);
        if (last_grandchild < length)
        {
            uint64_t larger_grandchild;
            if (compare(begin[first_child], begin[last_child]))
            {
                larger_grandchild = last_grandchild - !!compare(begin[last_grandchild], begin[last_grandchild - 1]);
                first_child = last_child;
            }
            else
                larger_grandchild = last_grandchild - 3 + !!compare(begin[last_grandchild - 3], begin[last_grandchild - 2]);
            if (compare(value, begin[larger_grandchild]))
            {
                begin[index] = std::move(begin[first_child]);
                begin[first_child] = std::move(begin[larger_grandchild]);
                index = larger_grandchild;
            }
            else if (compare(value, begin[first_child]))
            {
                begin[index] = std::move(begin[first_child]);
                index = first_child;
                break;
            }
            else
                break;
        }
        else if (last_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child<2>(begin + first_child, compare);
            if (!compare(value, *largest_child))
                break;
            begin[index] = std::move(*largest_child);
            index = largest_child - begin;
        }
        else if (first_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child<2>(begin + first_child, length - first_child, compare);
            if (compare(value, *largest_child))
            {
                begin[index] = std::move(*largest_child);
                index = largest_child - begin;
            }
            break;
        }
        else
            break;
    }
    begin[index] = std::move(value);
}
template<typename It>
void pop_binary_heap_grandparent(It begin, It end)
{
    return pop_binary_heap_grandparent(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void pop_quaternary_heap_grandparent(It begin, It end, Compare && compare)
{
    uint64_t length = (end - begin) - 1;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    uint64_t index = 0;
    for (;;)
    {
        uint64_t last_child = dary_heap_helpers::last_child_index<4>(index);
        uint64_t first_child = last_child - 3;
        uint64_t last_grandchild = dary_heap_helpers::last_grandchild_index<4>(index);
        if (last_grandchild < length)
        {
            It largest_child = dary_heap_helpers::largest_child<4>(begin + first_child, compare);
            size_t first_grandchild = dary_heap_helpers::first_child_index<4>(largest_child - begin);
            It largest_grandchild = dary_heap_helpers::largest_child<4>(begin + first_grandchild, compare);
            if (compare(value, *largest_grandchild))
            {
                begin[index] = std::move(*largest_child);
                *largest_child = std::move(*largest_grandchild);
                index = largest_grandchild - begin;
            }
            else if (compare(value, *largest_child))
            {
                begin[index] = std::move(*largest_child);
                index = largest_child - begin;
                break;
            }
            else
                break;
        }
        else if (last_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child<4>(begin + first_child, compare);
            if (!compare(value, *largest_child))
                break;
            begin[index] = std::move(*largest_child);
            index = largest_child - begin;
        }
        else if (first_child < length)
        {
            It largest_child = dary_heap_helpers::largest_child<4>(begin + first_child, length - first_child, compare);
            if (compare(value, *largest_child))
            {
                begin[index] = std::move(*largest_child);
                index = largest_child - begin;
            }
            break;
        }
        else
            break;
    }
    begin[index] = std::move(value);
}
template<typename It>
void pop_quaternary_heap_grandparent(It begin, It end)
{
    return pop_quaternary_heap_grandparent(begin, end, std::less<>{});
}


template<typename It, typename Compare>
void pop_binary_heap_unrolled(It begin, It end, Compare && compare)
{
    uint64_t old_length = end - begin;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    size_t index = 0;
    // num loops:
    // length = 0 -> 0
    // length = 1 -> 0
    // length = 2 -> 0
    // length = 3..6 -> 1
    // length = 7..14 -> 2
    // length = 15..30 -> 3
    //
    // old_length = 1..3 -> 0
    // old_length = 4..7 -> 1
    // old_length = 8..15 -> 2
    // old_length = 16..31 -> 3
    //
    // = highest_set_bit(old_length) - 2
    size_t num_loops_plus_one = minmax_heap_helpers::highest_set_bit(old_length);
    size_t first_child = 1;
    switch (num_loops_plus_one)
    {
    default:
    {
        --num_loops_plus_one;
        static constexpr size_t unroll_amount = 4;
        size_t unrolled_loop_count = num_loops_plus_one / unroll_amount;
        switch (num_loops_plus_one % unroll_amount)
        {
            do
            {
        case 0:
                --unrolled_loop_count;
            {
                size_t larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
            }
        case 3:
            {
                size_t larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
            }
                [[fallthrough]];
        case 2:
            {
                size_t larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
            }
                [[fallthrough]];
        case 1:
                size_t larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
            }
            while (unrolled_loop_count);
        }
    }
        [[fallthrough]];
    case 1:
        if (first_child < old_length - 1)
        {
            size_t larger_child = first_child + (first_child < old_length - 2 && compare(begin[first_child], begin[first_child + 1]));
            if (compare(value, begin[larger_child]))
            {
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
            }
        }
        [[fallthrough]];
    case 0:
early_out:
        begin[index] = std::move(value);
    }
}
template<typename It>
void pop_binary_heap_unrolled(It begin, It end)
{
    return pop_binary_heap_unrolled(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void pop_binary_heap_unrolled_grandparent(It begin, It end, Compare && compare)
{
    uint64_t old_length = end - begin;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    size_t index = 0;
    // num loops:
    // length = 0 -> 0
    // length = 1 -> 0
    // length = 2 -> 0
    // length = 3..6 -> 1
    // length = 7..14 -> 2
    // length = 15..30 -> 3
    //
    // old_length = 1..3 -> 0
    // old_length = 4..7 -> 1
    // old_length = 8..15 -> 2
    // old_length = 16..31 -> 3
    //
    // = highest_set_bit(old_length) - 2
    size_t num_loops_plus_one = minmax_heap_helpers::highest_set_bit(old_length);
    size_t first_child = 1;
    switch (num_loops_plus_one)
    {
    default:
    {
        size_t num_loops = num_loops_plus_one - 1;
        static constexpr size_t unroll_amount = 4;
        for (size_t unrolled_loop_count = num_loops / unroll_amount; unrolled_loop_count; --unrolled_loop_count)
        {
            size_t larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
            size_t larger_grandchild = dary_heap_helpers::first_child_index<2>(larger_child);
            larger_grandchild = larger_grandchild + !!compare(begin[larger_grandchild], begin[larger_grandchild + 1]);
            size_t larger_great_grandchild = dary_heap_helpers::first_child_index<2>(larger_grandchild);
            larger_great_grandchild = larger_great_grandchild + !!compare(begin[larger_great_grandchild], begin[larger_great_grandchild + 1]);
            size_t larger_great_great_grandchild = dary_heap_helpers::first_child_index<2>(larger_great_grandchild);
            larger_great_great_grandchild = larger_great_great_grandchild + !!compare(begin[larger_great_great_grandchild], begin[larger_great_great_grandchild + 1]);
            if (compare(value, begin[larger_great_great_grandchild]))
            {
                begin[index] = std::move(begin[larger_child]);
                begin[larger_child] = std::move(begin[larger_grandchild]);
                begin[larger_grandchild] = std::move(begin[larger_great_grandchild]);
                begin[larger_great_grandchild] = std::move(begin[larger_great_great_grandchild]);
                index = larger_great_great_grandchild;
                first_child = dary_heap_helpers::first_child_index<2>(index);
            }
            else if (compare(value, begin[larger_great_grandchild]))
            {
                begin[index] = std::move(begin[larger_child]);
                begin[larger_child] = std::move(begin[larger_grandchild]);
                begin[larger_grandchild] = std::move(begin[larger_great_grandchild]);
                index = larger_great_grandchild;
                goto early_out;
            }
            else if (compare(value, begin[larger_grandchild]))
            {
                begin[index] = std::move(begin[larger_child]);
                begin[larger_child] = std::move(begin[larger_grandchild]);
                index = larger_grandchild;
                goto early_out;
            }
            else if (compare(value, begin[larger_child]))
            {
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                goto early_out;
            }
            else
                goto early_out;
        }
        size_t larger_child;
        switch (num_loops % unroll_amount)
        {
        case 3:
            larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
            if (!compare(value, begin[larger_child]))
                goto early_out;
            begin[index] = std::move(begin[larger_child]);
            index = larger_child;
            first_child = dary_heap_helpers::first_child_index<2>(index);
            [[fallthrough]];
        case 2:
            larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
            if (!compare(value, begin[larger_child]))
                goto early_out;
            begin[index] = std::move(begin[larger_child]);
            index = larger_child;
            first_child = dary_heap_helpers::first_child_index<2>(index);
            [[fallthrough]];
        case 1:
            larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
            if (!compare(value, begin[larger_child]))
                goto early_out;
            begin[index] = std::move(begin[larger_child]);
            index = larger_child;
            first_child = dary_heap_helpers::first_child_index<2>(index);
        case 0:
            static_cast<void>(0);
        }
    }
        [[fallthrough]];
    case 1:
        if (first_child < old_length - 1)
        {
            size_t larger_child = first_child + (first_child < old_length - 2 && compare(begin[first_child], begin[first_child + 1]));
            if (compare(value, begin[larger_child]))
            {
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
            }
        }
        [[fallthrough]];
    case 0:
early_out:
        begin[index] = std::move(value);
    }
}
template<typename It>
void pop_binary_heap_unrolled_grandparent(It begin, It end)
{
    return pop_binary_heap_unrolled_grandparent(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void pop_binary_heap_unrolled_8(It begin, It end, Compare && compare)
{
    uint64_t old_length = end - begin;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    size_t index = 0;
    size_t num_loops_plus_one = minmax_heap_helpers::highest_set_bit(old_length);
    size_t first_child = 1;
    switch (num_loops_plus_one)
    {
    default:
    {
        size_t num_loops = num_loops_plus_one - 1;
        static constexpr size_t unroll_amount = 8;
        size_t unrolled_loop_count = num_loops / unroll_amount;
        size_t larger_child;
        switch (num_loops % unroll_amount)
        {
            do
            {
        case 0:
                --unrolled_loop_count;
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
        case 7:
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
        case 6:
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
        case 5:
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
        case 4:
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
        case 3:
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
        case 2:
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
        case 1:
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
            }
            while (unrolled_loop_count);
        }
    }
        [[fallthrough]];
    case 1:
        if (first_child < old_length - 1)
        {
            size_t larger_child = first_child + (first_child < old_length - 2 && compare(begin[first_child], begin[first_child + 1]));
            if (compare(value, begin[larger_child]))
            {
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
            }
        }
        [[fallthrough]];
    case 0:
early_out:
        begin[index] = std::move(value);
    }
}
template<typename It>
void pop_binary_heap_unrolled_8(It begin, It end)
{
    return pop_binary_heap_unrolled_8(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void pop_binary_heap_unrolled_2(It begin, It end, Compare && compare)
{
    uint64_t old_length = end - begin;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    size_t index = 0;
    // num loops:
    // length = 0 -> 0
    // length = 1 -> 0
    // length = 2 -> 0
    // length = 3..6 -> 1
    // length = 7..14 -> 2
    // length = 15..30 -> 3
    //
    // old_length = 1..3 -> 0
    // old_length = 4..7 -> 1
    // old_length = 8..15 -> 2
    // old_length = 16..31 -> 3
    //
    // = highest_set_bit(old_length) - 2
    size_t num_loops_plus_one = minmax_heap_helpers::highest_set_bit(old_length);
    size_t first_child = 1;
    switch (num_loops_plus_one)
    {
    default:
    {
        --num_loops_plus_one;
        static constexpr size_t unroll_amount = 2;
        size_t unrolled_loop_count = num_loops_plus_one / unroll_amount;
        switch (num_loops_plus_one % unroll_amount)
        {
            do
            {
        case 0:
                --unrolled_loop_count;
            {
                size_t larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
            }
        case 1:
                size_t larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                if (!compare(value, begin[larger_child]))
                    goto early_out;
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
            }
            while (unrolled_loop_count);
        }
    }
        [[fallthrough]];
    case 1:
        if (first_child < old_length - 1)
        {
            size_t larger_child = first_child + (first_child < old_length - 2 && compare(begin[first_child], begin[first_child + 1]));
            if (compare(value, begin[larger_child]))
            {
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
            }
        }
        [[fallthrough]];
    case 0:
early_out:
        begin[index] = std::move(value);
    }
}
template<typename It>
void pop_binary_heap_unrolled_2(It begin, It end)
{
    return pop_binary_heap_unrolled_2(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void pop_binary_heap_unrolled_no_early_out(It begin, It end, Compare && compare)
{
    uint64_t old_length = end - begin;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    size_t index = 0;
    size_t num_loops_plus_one = minmax_heap_helpers::highest_set_bit(old_length);
    size_t first_child = 1;
    switch (num_loops_plus_one)
    {
    default:
    {
        size_t num_loops = num_loops_plus_one - 1;
        static constexpr size_t unroll_amount = 4;
        size_t unrolled_loop_count = num_loops / unroll_amount;
        size_t larger_child;
        switch (num_loops % unroll_amount)
        {
            do
            {
        case 0:
                --unrolled_loop_count;
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
        case 3:
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
        case 2:
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
                [[fallthrough]];
        case 1:
                larger_child = first_child + !!compare(begin[first_child], begin[first_child + 1]);
                begin[index] = std::move(begin[larger_child]);
                index = larger_child;
                first_child = dary_heap_helpers::first_child_index<2>(index);
            }
            while (unrolled_loop_count);
        }
    }
        [[fallthrough]];
    case 1:
        if (first_child < old_length - 1)
        {
            size_t larger_child = first_child + (first_child < old_length - 2 && compare(begin[first_child], begin[first_child + 1]));
            begin[index] = std::move(begin[larger_child]);
            index = larger_child;
        }
        [[fallthrough]];
    case 0:
        size_t parent = dary_heap_helpers::parent_index<2>(index);
        while (index > 2)
        {
            uint64_t grandparent = dary_heap_helpers::grandparent_index<2>(index);
            if (!compare(begin[grandparent], value))
                break;
            begin[index] = std::move(begin[parent]);
            begin[parent] = std::move(begin[grandparent]);
            index = grandparent;
            parent = dary_heap_helpers::parent_index<2>(index);
        }
        if (index > 0 && compare(begin[parent], value))
        {
            begin[index] = std::move(begin[parent]);
            index = parent;
        }
        begin[index] = std::move(value);
    }
}
template<typename It>
void pop_binary_heap_unrolled_no_early_out(It begin, It end)
{
    return pop_binary_heap_unrolled_no_early_out(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void pop_quaternary_heap_unrolled(It begin, It end, Compare && compare)
{
    size_t old_length = end - begin;
    size_t length = old_length - 1;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    size_t index = 0;
    size_t num_loops_plus_one = minmax_heap_helpers::highest_set_bit(length * 3 + 1) / 2;
    size_t first_child = 1;
    switch (num_loops_plus_one)
    {
    default:
    {
        size_t num_loops = num_loops_plus_one - 1;
        static constexpr size_t unroll_amount = 4;
        size_t unrolled_loop_count = num_loops / unroll_amount;
        It larger_child = begin;
        switch (num_loops % unroll_amount)
        {
            do
            {
        case 0:
                --unrolled_loop_count;
                larger_child = dary_heap_helpers::largest_child<4>(begin + first_child, compare);
                if (!compare(value, *larger_child))
                    goto early_out;
                begin[index] = std::move(*larger_child);
                index = larger_child - begin;
                first_child = dary_heap_helpers::first_child_index<4>(index);
                [[fallthrough]];
        case 3:
                larger_child = dary_heap_helpers::largest_child<4>(begin + first_child, compare);
                if (!compare(value, *larger_child))
                    goto early_out;
                begin[index] = std::move(*larger_child);
                index = larger_child - begin;
                first_child = dary_heap_helpers::first_child_index<4>(index);
                [[fallthrough]];
        case 2:
                larger_child = dary_heap_helpers::largest_child<4>(begin + first_child, compare);
                if (!compare(value, *larger_child))
                    goto early_out;
                begin[index] = std::move(*larger_child);
                index = larger_child - begin;
                first_child = dary_heap_helpers::first_child_index<4>(index);
                [[fallthrough]];
        case 1:
                larger_child = dary_heap_helpers::largest_child<4>(begin + first_child, compare);
                if (!compare(value, *larger_child))
                    goto early_out;
                begin[index] = std::move(*larger_child);
                index = larger_child - begin;
                first_child = dary_heap_helpers::first_child_index<4>(index);
            }
            while (unrolled_loop_count);
        }
    }
        [[fallthrough]];
    case 1:
        if (first_child < length)
        {
            size_t num_children = length - first_child;
            It larger_child = num_children >= 4 ? dary_heap_helpers::largest_child<4>(begin + first_child, compare)
                                                : dary_heap_helpers::largest_child<4>(begin + first_child, num_children, compare);
            if (compare(value, *larger_child))
            {
                begin[index] = std::move(*larger_child);
                index = larger_child - begin;
            }
        }
        [[fallthrough]];
    case 0:
early_out:
        begin[index] = std::move(value);
    }
}
template<typename It>
void pop_quaternary_heap_unrolled(It begin, It end)
{
    return pop_quaternary_heap_unrolled(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void pop_quaternary_heap_unrolled_2(It begin, It end, Compare && compare)
{
    size_t old_length = end - begin;
    size_t length = old_length - 1;
    typename std::iterator_traits<It>::value_type value = std::move(end[-1]);
    end[-1] = std::move(begin[0]);
    size_t index = 0;
    size_t num_loops_plus_one = minmax_heap_helpers::highest_set_bit(length * 3 + 1) / 2;
    size_t first_child = 1;
    switch (num_loops_plus_one)
    {
    default:
    {
        --num_loops_plus_one;
        static constexpr size_t unroll_amount = 2;
        size_t unrolled_loop_count = num_loops_plus_one / unroll_amount;
        It larger_child = begin;
        switch (num_loops_plus_one % unroll_amount)
        {
            do
            {
        case 0:
                --unrolled_loop_count;
                larger_child = dary_heap_helpers::largest_child<4>(begin + first_child, compare);
                if (!compare(value, *larger_child))
                    goto early_out;
                begin[index] = std::move(*larger_child);
                index = larger_child - begin;
                first_child = dary_heap_helpers::first_child_index<4>(index);
                [[fallthrough]];
        case 1:
                larger_child = dary_heap_helpers::largest_child<4>(begin + first_child, compare);
                if (!compare(value, *larger_child))
                    goto early_out;
                begin[index] = std::move(*larger_child);
                index = larger_child - begin;
                first_child = dary_heap_helpers::first_child_index<4>(index);
            }
            while (unrolled_loop_count);
        }
    }
        [[fallthrough]];
    case 1:
        if (first_child < length)
        {
            size_t num_children = length - first_child;
            It larger_child = num_children >= 4 ? dary_heap_helpers::largest_child<4>(begin + first_child, compare)
                                                : dary_heap_helpers::largest_child<4>(begin + first_child, num_children, compare);
            if (compare(value, *larger_child))
            {
                begin[index] = std::move(*larger_child);
                index = larger_child - begin;
            }
        }
        [[fallthrough]];
    case 0:
early_out:
        begin[index] = std::move(value);
    }
}
template<typename It>
void pop_quaternary_heap_unrolled_2(It begin, It end)
{
    return pop_quaternary_heap_unrolled_2(begin, end, std::less<>{});
}

template<typename T, typename Compare = std::less<>>
struct PairingHeap
{
private:

    template<typename U>
    struct NonDeletingUniquePtr
    {
        NonDeletingUniquePtr()
            : ptr()
        {
        }
        explicit NonDeletingUniquePtr(U * ptr)
            : ptr(ptr)
        {
        }
        NonDeletingUniquePtr(NonDeletingUniquePtr && other)
            : ptr(other.ptr)
        {
            other.ptr = nullptr;
        }
        NonDeletingUniquePtr & operator=(NonDeletingUniquePtr && other)
        {
            swap(other);
            return *this;
        }
        ~NonDeletingUniquePtr()
        {
#ifdef DEBUG_BUILD
            CHECK_FOR_PROGRAMMER_ERROR(!ptr);
#endif
        }

        U & operator*() const
        {
            return ptr;
        }
        U * operator->() const
        {
            return ptr;
        }
        void swap(NonDeletingUniquePtr & other)
        {
            std::swap(ptr, other.ptr);
        }

        explicit operator bool() const
        {
            return ptr != nullptr;
        }
        bool operator !() const
        {
            return ptr == nullptr;
        }

        U * release()
        {
            U * result = ptr;
            ptr = nullptr;
            return result;
        }

    private:
        U * ptr;
    };

    struct PairingTree
    {
        explicit PairingTree(const T & elem) noexcept(noexcept(T(std::declval<const T &>())))
            : elem(elem)
        {
        }
        explicit PairingTree(T && elem) noexcept(noexcept(T(std::declval<T &&>())))
            : elem(std::move(elem))
        {
        }
        ~PairingTree()
        {
            delete sub_heaps.release();
            NonDeletingUniquePtr<PairingTree> tail = std::move(next);
            for (;;)
            {
                if (!tail)
                    break;
                NonDeletingUniquePtr<PairingTree> new_tail = std::move(tail->next);
                delete tail.release();
                tail = std::move(new_tail);
            }
        }

        T elem;
        NonDeletingUniquePtr<PairingTree> sub_heaps;
        NonDeletingUniquePtr<PairingTree> next;

        void add_to_front(NonDeletingUniquePtr<PairingTree> && to_add) noexcept
        {
            if (next)
            {
                NonDeletingUniquePtr<PairingTree> * tail = &to_add->next;
                while (*tail)
                    tail = &(*tail)->next;
                *tail = std::move(next);
            }
            next = std::move(to_add);
        }
    };
    // root->next will always be null
    NonDeletingUniquePtr<PairingTree> root;

    inline static void meld_neither_empty(NonDeletingUniquePtr<PairingTree> & target, NonDeletingUniquePtr<PairingTree> && other) noexcept
    {
        Compare cmp;
        if (cmp(other->elem, target->elem))
            target.swap(other);
        NonDeletingUniquePtr<PairingTree> & to_add_to = target->sub_heaps;
        if (to_add_to)
            to_add_to->add_to_front(std::move(other));
        else
            to_add_to = std::move(other);
    }
    inline void meld_non_empty(NonDeletingUniquePtr<PairingTree> && other) noexcept
    {
        if (empty())
            root = std::move(other);
        else
            meld_neither_empty(root, std::move(other));
    }

public:

    PairingHeap() = default;
    PairingHeap(PairingHeap &&) = default;
    PairingHeap & operator=(PairingHeap &&) = default;
    ~PairingHeap()
    {
        delete root.release();
    }

    struct MemoryPool
    {
    private:
        NonDeletingUniquePtr<PairingTree> pool;
    public:
        MemoryPool() = default;
        MemoryPool(MemoryPool &&) = default;
        MemoryPool & operator=(MemoryPool &&) = default;
        ~MemoryPool()
        {
            clear();
        }

        bool empty() const noexcept
        {
            return !pool;
        }

        void clear() noexcept
        {
            delete pool.release();
        }

        void add(NonDeletingUniquePtr<PairingTree> && to_add) noexcept
        {
            if (pool)
                pool->add_to_front(std::move(to_add));
            else
                pool = std::move(to_add);
        }

        NonDeletingUniquePtr<PairingTree> take_one() noexcept
        {
            NonDeletingUniquePtr<PairingTree> result(std::move(pool));
            pool = std::move(result->next);
            if (result->sub_heaps)
                add(std::move(result->sub_heaps));
            return result;
        }
    };

    bool empty() const noexcept
    {
        return !root;
    }
    const T & min() const noexcept
    {
        return root->elem;
    }
    void meld(PairingHeap && other) noexcept
    {
        if (other.empty())
            return;
        meld_non_empty(std::move(other.root));
    }
    void insert(T && elem, MemoryPool & pool) noexcept(noexcept(T(std::declval<T &&>())))
    {
        NonDeletingUniquePtr<PairingTree> new_heap;
        if (pool.empty())
            new_heap = NonDeletingUniquePtr<PairingTree>(new PairingTree(std::move(elem)));
        else
        {
            new_heap = pool.take_one();
            new_heap->elem = std::move(elem);
        }
        meld_non_empty(std::move(new_heap));
    }
    void insert(const T & elem, MemoryPool & pool) noexcept(noexcept(T(std::declval<const T &>())))
    {
        NonDeletingUniquePtr<PairingTree> new_heap;
        if (pool.empty())
            new_heap = NonDeletingUniquePtr<PairingTree>(new PairingTree(elem));
        else
        {
            new_heap = pool.take_one();
            new_heap->elem = elem;
        }
        meld_non_empty(std::move(new_heap));
    }
    void delete_min(MemoryPool & pool) noexcept
    {
        NonDeletingUniquePtr<PairingTree> sub_heaps(std::move(root->sub_heaps));
        pool.add(std::move(root));
        if (!sub_heaps)
            return;

        NonDeletingUniquePtr<PairingTree> tail(std::move(sub_heaps->next));
        while (tail)
        {
            NonDeletingUniquePtr<PairingTree> first_only(std::move(tail));
            NonDeletingUniquePtr<PairingTree> second_only(std::move(first_only->next));
            if (second_only)
            {
                tail = std::move(second_only->next);
                meld_neither_empty(first_only, std::move(second_only));
            }
            meld_neither_empty(sub_heaps, std::move(first_only));
        }
        root = std::move(sub_heaps);
    }
    void clear(MemoryPool & pool) noexcept
    {
        if (root)
            pool.add(std::move(root));
    }
};


template<typename T, typename Compare = std::less<>>
struct PairingHeapPair
{
private:

    template<typename U>
    struct NonDeletingUniquePtr
    {
        NonDeletingUniquePtr()
            : ptr()
        {
        }
        explicit NonDeletingUniquePtr(U * ptr)
            : ptr(ptr)
        {
        }
        NonDeletingUniquePtr(NonDeletingUniquePtr && other)
            : ptr(other.ptr)
        {
            other.ptr = nullptr;
        }
        NonDeletingUniquePtr & operator=(NonDeletingUniquePtr && other)
        {
            assert_valid();
            swap(other);
            return *this;
        }
        ~NonDeletingUniquePtr()
        {
//#ifdef DEBUG_BUILD
            CHECK_FOR_PROGRAMMER_ERROR(!ptr);
//#endif
        }

        U & operator*() const
        {
            assert_valid();
            return ptr;
        }
        U * operator->() const
        {
            assert_valid();
            return ptr;
        }
        void swap(NonDeletingUniquePtr & other)
        {
            assert_valid();
            other.assert_valid();
            std::swap(ptr, other.ptr);
        }

        explicit operator bool() const
        {
            assert_consistent();
            return ptr != nullptr;
        }
        bool operator !() const
        {
            assert_consistent();
            return ptr == nullptr;
        }

        U * release()
        {
            assert_valid();
            U * result = ptr;
            ptr = nullptr;
            return result;
        }

    private:
        U * ptr;

        void assert_consistent() const
        {
            size_t as_size_t = 0;
            memcpy(&as_size_t, &ptr, sizeof(as_size_t));
            if ((as_size_t & 1) == 0)
                return;
            as_size_t ^= 1;
            CHECK_FOR_PROGRAMMER_ERROR(as_size_t);
        }
        void assert_valid() const
        {
            size_t as_size_t = 0;
            memcpy(&as_size_t, &ptr, sizeof(as_size_t));
            CHECK_FOR_PROGRAMMER_ERROR((as_size_t & 1) == 0);
        }
    };

    struct PairingTree
    {
        explicit PairingTree(const T & elem) noexcept(noexcept(T(std::declval<const T &>())))
            : elem(elem)
            , sub_heaps_0()
        {
        }
        explicit PairingTree(T && elem) noexcept(noexcept(T(std::declval<T &&>())))
            : elem(std::move(elem))
            , sub_heaps_0()
        {
        }
        ~PairingTree()
        {
            if (as_int & 1)
                as_int ^= 1;
            NonDeletingUniquePtr<PairingTree> tail_0 = std::move(sub_heaps_0);
            for (;;)
            {
                if (!tail_0)
                    break;
                if (tail_0->as_int & 1)
                    tail_0->as_int ^= 1;
                NonDeletingUniquePtr<PairingTree> new_tail = std::move(tail_0->sub_heaps_0);
                delete tail_0.release();
                tail_0 = std::move(new_tail);
            }
            NonDeletingUniquePtr<PairingTree> tail_1 = std::move(sub_heaps_1);
            for (;;)
            {
                if (!tail_1)
                    break;
                NonDeletingUniquePtr<PairingTree> new_tail = std::move(tail_1->sub_heaps_1);
                delete tail_1.release();
                tail_1 = std::move(new_tail);
            }
            sub_heaps_0.~NonDeletingUniquePtr();
        }

        T elem;
        union
        {
            NonDeletingUniquePtr<PairingTree> sub_heaps_0;
            size_t as_int;
        };
        NonDeletingUniquePtr<PairingTree> sub_heaps_1;
    };
    NonDeletingUniquePtr<PairingTree> root;

    inline static void meld_neither_empty(NonDeletingUniquePtr<PairingTree> & target, NonDeletingUniquePtr<PairingTree> && other) noexcept
    {
        Compare cmp;
        if (cmp(other->elem, target->elem))
            target.swap(other);
        if (!target->sub_heaps_0)
            target->sub_heaps_0 = std::move(other);
        else if (!target->sub_heaps_1)
            target->sub_heaps_1 = std::move(other);
        else
        {
            if (target->as_int & 1)
                meld_neither_empty(target->sub_heaps_1, std::move(other));
            else
                meld_neither_empty(target->sub_heaps_0, std::move(other));
            target->as_int ^= 1;
        }
    }
    inline void meld_non_empty(NonDeletingUniquePtr<PairingTree> && other) noexcept
    {
        if (empty())
            root = std::move(other);
        else
            meld_neither_empty(root, std::move(other));
    }

public:

    PairingHeapPair() = default;
    PairingHeapPair(PairingHeapPair &&) = default;
    PairingHeapPair & operator=(PairingHeapPair &&) = default;
    ~PairingHeapPair()
    {
        delete root.release();
    }

    struct MemoryPool
    {
    private:
        NonDeletingUniquePtr<PairingTree> pool;

        static void add(NonDeletingUniquePtr<PairingTree> & parent, NonDeletingUniquePtr<PairingTree> && to_add) noexcept
        {
            if (!parent->sub_heaps_0)
                parent->sub_heaps_0 = std::move(to_add);
            else if (!parent->sub_heaps_1)
                parent->sub_heaps_1 = std::move(to_add);
            else if (!to_add->sub_heaps_0)
            {
                to_add->sub_heaps_0 = std::move(parent);
                parent = std::move(to_add);
            }
            else if (!to_add->sub_heaps_1)
            {
                to_add->sub_heaps_1 = std::move(parent);
                parent = std::move(to_add);
            }
            else
            {
                if (parent->as_int & 1)
                    add(parent->sub_heaps_1, std::move(to_add));
                else
                    add(parent->sub_heaps_0, std::move(to_add));
                parent->as_int ^= 1;
            }
        }
    public:
        MemoryPool() = default;
        MemoryPool(MemoryPool &&) = default;
        MemoryPool & operator=(MemoryPool &&) = default;
        ~MemoryPool()
        {
            clear();
        }

        bool empty() const noexcept
        {
            return !pool;
        }

        void clear() noexcept
        {
            delete pool.release();
        }

        void add(NonDeletingUniquePtr<PairingTree> && to_add) noexcept
        {
            if (pool)
                add(pool, std::move(to_add));
            else
                pool = std::move(to_add);
        }

        NonDeletingUniquePtr<PairingTree> take_one() noexcept
        {
            NonDeletingUniquePtr<PairingTree> result(std::move(pool));
            if (result->as_int & 1)
                result->as_int ^= 1;
            pool = std::move(result->sub_heaps_0);
            if (result->sub_heaps_1)
                add(std::move(result->sub_heaps_1));
            return result;
        }
    };

    bool empty() const noexcept
    {
        return !root;
    }
    const T & min() const noexcept
    {
        return root->elem;
    }
    void meld(PairingHeapPair && other) noexcept
    {
        if (other.empty())
            return;
        meld_non_empty(std::move(other.root));
    }
    void insert(T && elem, MemoryPool & pool) noexcept(noexcept(T(std::declval<T &&>())))
    {
        NonDeletingUniquePtr<PairingTree> new_heap;
        if (pool.empty())
            new_heap = NonDeletingUniquePtr<PairingTree>(new PairingTree(std::move(elem)));
        else
        {
            new_heap = pool.take_one();
            new_heap->elem = std::move(elem);
        }
        meld_non_empty(std::move(new_heap));
    }
    void insert(const T & elem, MemoryPool & pool) noexcept(noexcept(T(std::declval<const T &>())))
    {
        NonDeletingUniquePtr<PairingTree> new_heap;
        if (pool.empty())
            new_heap = NonDeletingUniquePtr<PairingTree>(new PairingTree(elem));
        else
        {
            new_heap = pool.take_one();
            new_heap->elem = elem;
        }
        meld_non_empty(std::move(new_heap));
    }
    void delete_min(MemoryPool & pool) noexcept
    {
        NonDeletingUniquePtr<PairingTree> new_root;
        NonDeletingUniquePtr<PairingTree> other_child;
        if (root->as_int & 1)
        {
            root->as_int ^= 1;
            new_root = std::move(root->sub_heaps_0);
            other_child = std::move(root->sub_heaps_1);
        }
        else
        {
            new_root = std::move(root->sub_heaps_1);
            other_child = std::move(root->sub_heaps_0);
        }
        pool.add(std::move(root));
        root = std::move(new_root);
        if (other_child)
            meld_non_empty(std::move(other_child));
    }
    void clear(MemoryPool & pool) noexcept
    {
        if (root)
            pool.add(std::move(root));
    }
};

template<typename T, size_t MergeInterval, typename Compare = std::less<>>
struct PairingHeapMorePushWork
{
private:

    template<typename U>
    struct NonDeletingUniquePtr
    {
        NonDeletingUniquePtr()
            : ptr()
        {
        }
        explicit NonDeletingUniquePtr(U * ptr)
            : ptr(ptr)
        {
        }
        NonDeletingUniquePtr(NonDeletingUniquePtr && other)
            : ptr(other.ptr)
        {
            other.ptr = nullptr;
        }
        NonDeletingUniquePtr & operator=(NonDeletingUniquePtr && other)
        {
            swap(other);
            return *this;
        }
        ~NonDeletingUniquePtr()
        {
#ifdef DEBUG_BUILD
            CHECK_FOR_PROGRAMMER_ERROR(!ptr);
#endif
        }

        U & operator*() const
        {
            return ptr;
        }
        U * operator->() const
        {
            return ptr;
        }
        void swap(NonDeletingUniquePtr & other)
        {
            std::swap(ptr, other.ptr);
        }

        explicit operator bool() const
        {
            return ptr != nullptr;
        }
        bool operator !() const
        {
            return ptr == nullptr;
        }

        U * release()
        {
            U * result = ptr;
            ptr = nullptr;
            return result;
        }

    private:
        U * ptr;
    };

    struct PairingTree
    {
        explicit PairingTree(const T & elem) noexcept(noexcept(T(std::declval<const T &>())))
            : elem(elem)
            , next()
        {
        }
        explicit PairingTree(T && elem) noexcept(noexcept(T(std::declval<T &&>())))
            : elem(std::move(elem))
            , next()
        {
        }
        ~PairingTree()
        {
            delete sub_heaps.release();
            NonDeletingUniquePtr<PairingTree> tail = std::move(next);
            for (;;)
            {
                if (!tail)
                    break;
                NonDeletingUniquePtr<PairingTree> new_tail = std::move(tail->next);
                delete tail.release();
                tail = std::move(new_tail);
            }
            next.~NonDeletingUniquePtr();
        }

        T elem;
        NonDeletingUniquePtr<PairingTree> sub_heaps;
        union
        {
            NonDeletingUniquePtr<PairingTree> next;
            size_t root_num_inserts;
        };

        void add_to_front(NonDeletingUniquePtr<PairingTree> && to_add) noexcept
        {
            if (next)
            {
                NonDeletingUniquePtr<PairingTree> * tail = &to_add->next;
                while (*tail)
                    tail = &(*tail)->next;
                *tail = std::move(next);
            }
            next = std::move(to_add);
        }
    };
    // root->next will always be null
    NonDeletingUniquePtr<PairingTree> root;

    inline static void meld_neither_empty(NonDeletingUniquePtr<PairingTree> & target, NonDeletingUniquePtr<PairingTree> && other) noexcept
    {
        Compare cmp;
        if (cmp(other->elem, target->elem))
            target.swap(other);
        NonDeletingUniquePtr<PairingTree> & to_add_to = target->sub_heaps;
        if (to_add_to)
            to_add_to->add_to_front(std::move(other));
        else
            to_add_to = std::move(other);
    }
    inline void meld_non_empty(NonDeletingUniquePtr<PairingTree> && other) noexcept
    {
        if (empty())
            root = std::move(other);
        else
        {
            size_t num_inserts = std::exchange(root->root_num_inserts, 0);
            meld_neither_empty(root, std::move(other));
            root->root_num_inserts = num_inserts;
        }
    }

    static void merge_pairs(NonDeletingUniquePtr<PairingTree> & sub_heaps)
    {
        NonDeletingUniquePtr<PairingTree> tail(std::move(sub_heaps->next));
        while (tail)
        {
            NonDeletingUniquePtr<PairingTree> first_only(std::move(tail));
            NonDeletingUniquePtr<PairingTree> second_only(std::move(first_only->next));
            if (second_only)
            {
                tail = std::move(second_only->next);
                meld_neither_empty(first_only, std::move(second_only));
            }
            meld_neither_empty(sub_heaps, std::move(first_only));
        }
    }

public:

    PairingHeapMorePushWork() = default;
    PairingHeapMorePushWork(PairingHeapMorePushWork &&) = default;
    PairingHeapMorePushWork & operator=(PairingHeapMorePushWork &&) = default;
    ~PairingHeapMorePushWork()
    {
        if (root)
        {
            root->root_num_inserts = 0;
            delete root.release();
        }
    }

    struct MemoryPool
    {
    private:
        NonDeletingUniquePtr<PairingTree> pool;
    public:
        MemoryPool() = default;
        MemoryPool(MemoryPool &&) = default;
        MemoryPool & operator=(MemoryPool &&) = default;
        ~MemoryPool()
        {
            clear();
        }

        bool empty() const noexcept
        {
            return !pool;
        }

        void clear() noexcept
        {
            delete pool.release();
        }

        void add(NonDeletingUniquePtr<PairingTree> && to_add) noexcept
        {
            if (pool)
                pool->add_to_front(std::move(to_add));
            else
                pool = std::move(to_add);
        }

        NonDeletingUniquePtr<PairingTree> take_one() noexcept
        {
            NonDeletingUniquePtr<PairingTree> result(std::move(pool));
            pool = std::move(result->next);
            if (result->sub_heaps)
                add(std::move(result->sub_heaps));
            return result;
        }
    };

    bool empty() const noexcept
    {
        return !root;
    }
    const T & min() const noexcept
    {
        return root->elem;
    }
    void meld(PairingHeapMorePushWork && other) noexcept
    {
        if (other.empty())
            return;
        meld_non_empty(std::move(other.root));
    }
    void insert(T && elem, MemoryPool & pool) noexcept(noexcept(T(std::declval<T &&>())))
    {
        NonDeletingUniquePtr<PairingTree> new_heap;
        if (pool.empty())
            new_heap = NonDeletingUniquePtr<PairingTree>(new PairingTree(std::move(elem)));
        else
        {
            new_heap = pool.take_one();
            new_heap->elem = std::move(elem);
        }
        meld_non_empty(std::move(new_heap));
        ++root->root_num_inserts;
        if ((root->root_num_inserts % MergeInterval) == 0 && root->sub_heaps)
            merge_pairs(root->sub_heaps);
    }
    void insert(const T & elem, MemoryPool & pool) noexcept(noexcept(T(std::declval<const T &>())))
    {
        NonDeletingUniquePtr<PairingTree> new_heap;
        if (pool.empty())
            new_heap = NonDeletingUniquePtr<PairingTree>(new PairingTree(elem));
        else
        {
            new_heap = pool.take_one();
            new_heap->elem = elem;
        }
        meld_non_empty(std::move(new_heap));
        ++root->root_num_inserts;
        if ((root->root_num_inserts % MergeInterval) == 0 && root->sub_heaps)
            merge_pairs(root->sub_heaps);
    }
    void delete_min(MemoryPool & pool) noexcept
    {
        NonDeletingUniquePtr<PairingTree> sub_heaps(std::move(root->sub_heaps));
        size_t num_inserts = std::exchange(root->root_num_inserts, 0);
        pool.add(std::move(root));
        if (!sub_heaps)
            return;

        merge_pairs(sub_heaps);
        root = std::move(sub_heaps);
        root->root_num_inserts = num_inserts;
    }
    void clear(MemoryPool & pool) noexcept
    {
        if (root)
        {
            root->root_num_inserts = 0;
            pool.add(std::move(root));
        }
    }
};

#include <iostream>
template<typename T, typename Compare = std::less<>>
struct PairingHeapForSort
{
private:

    template<typename U>
    struct NonDeletingUniquePtr
    {
        NonDeletingUniquePtr()
            : ptr()
        {
        }
        explicit NonDeletingUniquePtr(U * ptr)
            : ptr(ptr)
        {
        }
        NonDeletingUniquePtr(NonDeletingUniquePtr && other)
            : ptr(other.ptr)
        {
            other.ptr = nullptr;
        }
        NonDeletingUniquePtr & operator=(NonDeletingUniquePtr && other)
        {
            swap(other);
            return *this;
        }
        ~NonDeletingUniquePtr()
        {
#ifdef DEBUG_BUILD
            CHECK_FOR_PROGRAMMER_ERROR(!ptr);
#endif
        }

        U & operator*() const
        {
            return ptr;
        }
        U * operator->() const
        {
            return ptr;
        }
        void swap(NonDeletingUniquePtr & other)
        {
            std::swap(ptr, other.ptr);
        }

        explicit operator bool() const
        {
            return ptr != nullptr;
        }
        bool operator !() const
        {
            return ptr == nullptr;
        }

        U * release()
        {
            U * result = ptr;
            ptr = nullptr;
            return result;
        }

    private:
        U * ptr;
    };

    struct PairingTree
    {
        explicit PairingTree(const T & elem) noexcept(noexcept(T(std::declval<const T &>())))
            : elem(elem)
        {
        }
        explicit PairingTree(T && elem) noexcept(noexcept(T(std::declval<T &&>())))
            : elem(std::move(elem))
        {
        }
        void cleanup()
        {
#ifdef DEBUG_BUILD
            if (sub_heaps)
            {
                sub_heaps->cleanup();
                sub_heaps.release();
            }
            NonDeletingUniquePtr<PairingTree> tail = std::move(next);
            for (;;)
            {
                if (!tail)
                    break;
                NonDeletingUniquePtr<PairingTree> new_tail = std::move(tail->next);
                tail->cleanup();
                tail.release();
                tail = std::move(new_tail);
            }
#endif
        }

        T elem;
        NonDeletingUniquePtr<PairingTree> sub_heaps;
        NonDeletingUniquePtr<PairingTree> next;

        void add_to_front(NonDeletingUniquePtr<PairingTree> && to_add) noexcept
        {
            if (next)
            {
                NonDeletingUniquePtr<PairingTree> * tail = &to_add->next;
                while (*tail)
                    tail = &(*tail)->next;
                *tail = std::move(next);
            }
            next = std::move(to_add);
        }
    };
    // root->next will always be null
    NonDeletingUniquePtr<PairingTree> root;

    inline static void meld_neither_empty(NonDeletingUniquePtr<PairingTree> & target, NonDeletingUniquePtr<PairingTree> && other, const Compare & cmp) noexcept
    {
        if (cmp(other->elem, target->elem))
            target.swap(other);
        NonDeletingUniquePtr<PairingTree> & to_add_to = target->sub_heaps;
        if (to_add_to)
            to_add_to->add_to_front(std::move(other));
        else
            to_add_to = std::move(other);
    }

    static NonDeletingUniquePtr<PairingTree> merge_pairs(NonDeletingUniquePtr<PairingTree> sub_heaps, const Compare & cmp)
    {
        NonDeletingUniquePtr<PairingTree> tail(std::move(sub_heaps->next));
        static size_t max_num_loops = 0;
        size_t num_loops = 0;
        while (tail)
        {
            ++num_loops;
            NonDeletingUniquePtr<PairingTree> first_only(std::move(tail));
            NonDeletingUniquePtr<PairingTree> second_only(std::move(first_only->next));
            if (second_only)
            {
                tail = std::move(second_only->next);
                meld_neither_empty(first_only, std::move(second_only), cmp);
            }
            meld_neither_empty(sub_heaps, std::move(first_only), cmp);
        }
        if (num_loops > max_num_loops)
        {
            max_num_loops = num_loops;
            //std::cout << "num loops: " << num_loops << std::endl;
        }
        return sub_heaps;
    }

public:

    PairingHeapForSort() = default;
    PairingHeapForSort(PairingHeapForSort &&) = default;
    PairingHeapForSort & operator=(PairingHeapForSort &&) = default;
    ~PairingHeapForSort()
    {
        if (root)
        {
            root->cleanup();
            root.release();
        }
    }

    struct MemoryPool
    {
    private:
        NonDeletingUniquePtr<PairingTree> pool;
        std::vector<PairingTree> memory;
    public:
        explicit MemoryPool(size_t size)
        {
            memory.reserve(size);
        }

        MemoryPool(MemoryPool &&) = delete;
        MemoryPool & operator=(MemoryPool &&) = delete;
        ~MemoryPool()
        {
            if (pool)
            {
                pool->cleanup();
                pool.release();
            }
        }

        /*void add(NonDeletingUniquePtr<PairingTree> && to_add) noexcept
        {
            if (pool)
                pool->add_to_front(std::move(to_add));
            else
                pool = std::move(to_add);
        }*/
        void add_no_tail(NonDeletingUniquePtr<PairingTree> && to_add) noexcept
        {
            //CHECK_FOR_PROGRAMMER_ERROR(!to_add->next);
            if (pool)
                to_add->next = std::move(pool);
            pool = std::move(to_add);
        }

        NonDeletingUniquePtr<PairingTree> take_one(T && elem) noexcept
        {
            if (!pool)
            {
                CHECK_FOR_PROGRAMMER_ERROR(memory.size() != memory.capacity());
                memory.emplace_back(std::move(elem));
                return NonDeletingUniquePtr<PairingTree>(&memory.back());
            }
            else
            {
                NonDeletingUniquePtr<PairingTree> result(std::move(pool));
                pool = std::move(result->next);
                //CHECK_FOR_PROGRAMMER_ERROR(!result->sub_heaps);
                //if (result->sub_heaps)
                //    add(std::move(result->sub_heaps));
                result->elem = std::move(elem);
                return result;
            }
        }
    };

    bool empty() const noexcept
    {
        return !root;
    }
    const T & min() const noexcept
    {
        return root->elem;
    }
    void insert(T && elem, MemoryPool & pool, const Compare & cmp) noexcept(noexcept(T(std::declval<T &&>())))
    {
        NonDeletingUniquePtr<PairingTree> new_tree = pool.take_one(std::move(elem));
        if (!root)
            root = std::move(new_tree);
        else if (cmp(new_tree->elem, root->elem))
        {
            new_tree->sub_heaps = std::move(root);
            root = std::move(new_tree);
        }
        else
        {
            new_tree->next = std::move(root->sub_heaps);
            root->sub_heaps = std::move(new_tree);
        }
    }
    void delete_min(MemoryPool & pool, const Compare & cmp) noexcept
    {
        NonDeletingUniquePtr<PairingTree> sub_heaps(std::move(root->sub_heaps));
        pool.add_no_tail(std::move(root));
        if (!sub_heaps)
            return;

        root = merge_pairs(std::move(sub_heaps), cmp);
    }
    void decrease_min(const Compare & cmp) noexcept
    {
        if (!root->sub_heaps)
            return;

        NonDeletingUniquePtr<PairingTree> next_largest = merge_pairs(std::move(root->sub_heaps), cmp);
        meld_neither_empty(root, std::move(next_largest), cmp);
    }
};

template<typename It, typename Compare>
void heap_pair_heap_sort(It begin, It end, Compare compare)
{
    auto heap_order = [compare](const auto & l, const auto & r)
    {
        return compare(r, l);
    };
    make_dary_heap<2>(begin, end, heap_order);
    size_t size = end - begin;
    if (size < 3)
        return;

    auto pairing_heap_order = [begin, compare](size_t l, size_t r)
    {
        return compare(begin[l], begin[r]);
    };
    using PairingHeap = PairingHeapForSort<size_t, decltype(pairing_heap_order)>;
    typename PairingHeap::MemoryPool pool(size);
    PairingHeap heap;
    static size_t largest_heap_size = 0;
    size_t heap_size = 2;
    heap.insert(1, pool, pairing_heap_order);
    heap.insert(2, pool, pairing_heap_order);
    for (size_t i = 1, end = size - 1; i < end;)
    {
        size_t min_index = heap.min();
        if (min_index < i)
        {
            heap.delete_min(pool, pairing_heap_order);
            --heap_size;
            continue;
        }
        else if (min_index == i)
        {
            heap.delete_min(pool, pairing_heap_order);
            --heap_size;
        }
        else
        {
            using std::swap;
            swap(begin[i], begin[min_index]);
            dary_heap_helpers::trickle_down<2>(begin, size, min_index, heap_order);
            heap.decrease_min(pairing_heap_order);
        }
        size_t child = dary_heap_helpers::first_child_index<2>(i);
        if (child < size)
        {
            heap.insert(size_t(child), pool, pairing_heap_order);
            ++heap_size;
            if (child + 1 < size)
            {
                heap.insert(child + 1, pool, pairing_heap_order);
                ++heap_size;
            }
            if (heap_size > largest_heap_size)
            {
                largest_heap_size = heap_size;
                //std::cout << "heap_size: " << heap_size << std::endl;
            }
        }
        ++i;
    }
}
template<typename It>
void heap_pair_heap_sort(It begin, It end)
{
    heap_pair_heap_sort(begin, end, std::less<>{});
}

template<typename It, typename Compare>
void heap_heap_sort(It begin, It end, Compare && compare)
{
    auto heap_order = [compare](const auto & l, const auto & r)
    {
        return compare(r, l);
    };
    make_dary_heap<2>(begin, end, heap_order);
    size_t size = end - begin;
    if (size < 3)
        return;

    struct SubHeap
    {
        It begin;
        size_t index;
        size_t length;
    };
    auto sub_heap_order = [compare](const SubHeap & l, const SubHeap & r)
    {
        return compare(r.begin[r.index], l.begin[l.index]);
    };
    std::vector<SubHeap> sub_heaps;
    auto print_state = [&]
    {
        /*const char * separator = "";
        for (auto it = begin; it != end; ++it)
        {
            std::cout << separator << *it;
            separator = ", ";
        }
        for (const SubHeap & heap : sub_heaps)
        {
            std::cout << '\n' << (heap.begin - begin) << ", " << heap.index << ", " << heap.length << ", " << heap.begin[heap.index];
        }
        std::cout << std::endl;*/
    };
    auto split_heap = [&sub_heaps, &heap_order, &sub_heap_order](const SubHeap & heap)
    {
        size_t first_child = dary_heap_helpers::first_child_index<2>(heap.index);
        if (first_child >= heap.length)
            return;
        size_t second_child = first_child + 1;
        auto check_new_heap_is_good = [&]
        {
            /*auto end = sub_heaps.end() - 1;
            It to_find_begin = end->begin + end->index;
            auto found = std::find_if(sub_heaps.begin(), end, [=](const SubHeap & existing_heap)
            {
                It existing_begin = existing_heap.begin + existing_heap.index;
                return to_find_begin == existing_begin;
            });
            CHECK_FOR_PROGRAMMER_ERROR(found == end);*/
        };
        if (second_child == heap.length)
        {
            sub_heaps.push_back({ heap.begin, first_child, heap.length });
            check_new_heap_is_good();
            return;
        }
        // example: heap of size 7
        // highest_bit = 2
        int highest_bit = minmax_heap_helpers::highest_set_bit(heap.length);
        // lower_half = 4 - 1 = 3
        size_t lower_half = (size_t(1) << highest_bit) - 1;
        if (lower_half > second_child)
        {
            sub_heaps.push_back({ heap.begin, first_child, lower_half });
            check_new_heap_is_good();
            sub_heaps.push_back({ heap.begin, second_child, lower_half });
            check_new_heap_is_good();
            // problem: this is not necessarily where the last layer begins
            // example 1: heap.begin = 0, index = 0, size = 32, lower_half = 31
            //            last layer goes from 31 to 32. seems good
            // example 2: heap.begin = 0, index = 1, size = 31, lower_half = 15
            //            last layer goes from 15 to 23. seems good
            // example 3: heap.begin = 0, index = 2, size = 31, lower_half = 15
            //            last layer goes from 23 to 31. oops
            size_t last_layer_first_index = heap.index;
            while (last_layer_first_index < lower_half)
                last_layer_first_index = dary_heap_helpers::first_child_index<2>(last_layer_first_index);
            size_t last_layer_last_index = heap.index;
            while (last_layer_last_index < lower_half)
                last_layer_last_index = dary_heap_helpers::last_child_index<2>(last_layer_last_index);
            last_layer_last_index = std::min(last_layer_last_index + 1, heap.length);
            size_t last_layer_size = last_layer_last_index - last_layer_first_index;
            It last_layer_begin = heap.begin + last_layer_first_index;
            //size_t last_layer_size = heap.length - lower_half;
            make_dary_heap<2>(last_layer_begin, last_layer_begin + last_layer_size, heap_order);
            sub_heaps.push_back({ last_layer_begin, 0, last_layer_size });
            check_new_heap_is_good();
            push_dary_heap<2>(sub_heaps.begin(), sub_heaps.end() - 2, sub_heap_order);
        }
        else
        {
            sub_heaps.push_back({ heap.begin, first_child, heap.length });
            check_new_heap_is_good();
            sub_heaps.push_back({ heap.begin, second_child, heap.length });
            check_new_heap_is_good();
        }
        push_dary_heap<2>(sub_heaps.begin(), sub_heaps.end() - 1, sub_heap_order);
        push_dary_heap<2>(sub_heaps.begin(), sub_heaps.end(), sub_heap_order);
    };
    sub_heaps.reserve(size);
    split_heap({ begin, 0, size });

    for (auto it = begin + 1, it_end = end - 1; it < it_end;)
    {
        SubHeap next = sub_heaps.front();
        It smallest_it = next.begin + next.index;
        print_state();
        if (smallest_it < it)
        {
            pop_dary_heap<2>(sub_heaps.begin(), sub_heaps.end(), sub_heap_order);
            sub_heaps.pop_back();
            continue;
        }
        else if (smallest_it == it)
        {
            pop_dary_heap<2>(sub_heaps.begin(), sub_heaps.end(), sub_heap_order);
            sub_heaps.pop_back();
            split_heap(next);
        }
        else
        {
            std::iter_swap(it, smallest_it);
            dary_heap_helpers::trickle_down<2>(next.begin, next.length, next.index, heap_order);
            auto found = std::find_if(sub_heaps.begin(), sub_heaps.end(), [&](const SubHeap & sub_heap)
            {
                return sub_heap.begin + sub_heap.index == it;
            });
            CHECK_FOR_PROGRAMMER_ERROR(found != sub_heaps.end());
            SubHeap to_split = *found;
            // found is in the wrong place, but also needs to be removed
            // sub_heaps.begin() is also in the wrong place now and needs to trickle down
            //
            // how to do this correctly?
            //
            // X _ _ F _ _ _ L
            //
            // if L == F then
            // pop L and trickle down X
            //
            // if *L > *X then
            // L _ _ X _ _ _ F
            // then heapify X, pop F and finally trickle down L
            //
            // if *L < *X then
            // X _ _ L _ _ _ F
            // then pop F, heapify L and finally trickle down X
            print_state();
            auto last = sub_heaps.end() - 1;
            if (found == last)
            {
                sub_heaps.pop_back();
                dary_heap_helpers::trickle_down<2>(sub_heaps.begin(), sub_heaps.size(), 0, sub_heap_order);
            }
            else if (sub_heap_order(sub_heaps.front(), *last))
            {
                *found = std::move(next);
                sub_heaps.front() = std::move(*last);
                sub_heaps.pop_back();
                dary_heap_helpers::heapify<2>(sub_heaps.begin(), found, sub_heaps.end(), sub_heap_order);
                dary_heap_helpers::trickle_down<2>(sub_heaps.begin(), sub_heaps.size(), 0, sub_heap_order);
            }
            else
            {
                *found = std::move(*last);
                sub_heaps.pop_back();
                dary_heap_helpers::heapify<2>(sub_heaps.begin(), found, sub_heaps.end(), sub_heap_order);
                dary_heap_helpers::trickle_down<2>(sub_heaps.begin(), sub_heaps.size(), 0, sub_heap_order);
            }
            split_heap(to_split);
        }
        ++it;
    }
}
template<typename It>
void heap_heap_sort(It begin, It end)
{
    heap_heap_sort(begin, end, std::less<>{});
}

