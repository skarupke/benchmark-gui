#pragma once

#include <iterator>
#include <algorithm>
#include <array>
#include "heap.hpp"

// finds the matching closing bracket. for example
//
// "(hi (there) paul)"
//   ^               ^
//   it             end
//
// open = '('
// close = ')'
//
// will return a pointer to the last paren, not the
// first closing paren
template<typename It, typename End, typename T>
It find_matching(It it, End end, const T & open, const T & close)
{
    int nested_count = 1;
    for (; it != end; ++it)
    {
        if (*it == close)
        {
            if (--nested_count == 0)
            {
                return it;
            }
        }
        else if (*it == open)
        {
            ++nested_count;
        }
    }
    return it;
}


template<typename It1, typename End1, typename It2, typename End2, typename Func>
std::pair<It1, It2> mismatch_ignoring(It1 lit, End1 lend, It2 rit, End2 rend, Func && func)
{
    for (;;)
    {
        lit = std::find_if_not(lit, lend, func);
        rit = std::find_if_not(rit, rend, func);
        if ((lit == lend) != (rit == rend))
            return std::make_pair(lit, rit);
        auto lit_next = std::find_if(lit + 1, lend, func);
        auto rit_next = std::find_if(rit + 1, rend, func);
        if (!std::equal(lit, lit_next, rit, rit_next))
            return std::make_pair(lit, rit);
        if (lit_next == lend)
            return std::make_pair(lend, std::find_if_not(rit_next, rend, func));
        else if (rit_next == rend)
            return std::make_pair(std::find_if_not(lit_next, lend, func), rend);
        lit = lit_next;
        rit = rit_next;
    }
}

template<typename It>
It find_true(It begin, It end)
{
    for (; begin != end; ++begin)
    {
        if (*begin)
            return begin;
    }
    return end;
}

template<typename It>
bool any_true(It begin, It end)
{
    return find_true(begin, end) != end;
}

template<typename It, typename Func, typename Compare>
auto max_element_transformed(It begin, It end, Func && transform, Compare && compare)
{
    auto max_value = transform(*begin);
    auto max_it = begin;
    for (++begin; begin != end; ++begin)
    {
        auto value = transform(*begin);
        if (!compare(max_value, value))
            continue;
        max_value = std::move(value);
        max_it = begin;
    }
    return std::make_pair(std::move(max_it), std::move(max_value));
}
template<typename It, typename Func>
auto max_element_transformed(It begin, It end, Func && transform)
{
    return max_element_transformed(begin, end, transform, std::less<>{});
}

template<typename It, typename Func, typename Compare>
auto min_element_transformed(It begin, It end, Func && transform, Compare && compare)
{
    return max_element_transformed(begin, end, transform, [&compare](auto && lhs, auto && rhs){ return compare(rhs, lhs); });
}
template<typename It, typename Func>
auto min_element_transformed(It begin, It end, Func && transform)
{
    return min_element_transformed(begin, end, transform, std::less<>{});
}

template<typename It>
It sort_and_unique(It begin, It end)
{
    std::sort(begin, end);
    return std::unique(begin, end);
}
template<typename T>
T sort_and_unique(T container)
{
    container.erase(sort_and_unique(container.begin(), container.end()), container.end());
    return container;
}

template<typename It, typename T, typename Func>
It binary_find(It begin, It end, const T & value, Func && compare)
{
    auto found = std::lower_bound(begin, end, value, compare);
    if (found == end || compare(value, *found))
        return end;
    else
        return found;
}
template<typename It, typename T>
It binary_find(It begin, It end, const T & value)
{
    return binary_find(begin, end, value, std::less<>{});
}

template<typename It, typename Func>
size_t index_of(It begin, It end, Func && condition)
{
    for (size_t i = 0; begin != end; ++begin, ++i)
    {
        if (condition(*begin))
            return i;
    }
    return static_cast<size_t>(-1);
}

template<typename It, typename V>
size_t find_index(It begin, It end, const V & value)
{
    for (size_t i = 0; begin != end; ++begin, ++i)
    {
        if (*begin == value)
            return i;
    }
    return static_cast<size_t>(-1);
}

// Creates pointers to the top N elements in [in_begin, in_end], in sorted order.
// The pointers are stored in [out_begin, out_end]
// and N = out_end - out_begin
template<typename It, typename OutIt, typename Compare>
OutIt partial_sort_pointer(It in_begin, It in_end, OutIt out_begin, OutIt out_end, Compare && compare)
{
    if (out_begin == out_end)
        return out_end;
    OutIt heap_end = out_begin;
    while (in_begin != in_end && heap_end != out_end)
    {
        *heap_end = std::addressof(*in_begin);
        ++heap_end;
        ++in_begin;
    }
    auto heap_compare = [&](auto && lhs, auto && rhs){ return compare(*lhs, *rhs); };
    std::make_heap(out_begin, heap_end, heap_compare);

    for (; in_begin != in_end; ++in_begin)
    {
        if (compare(*in_begin, **out_begin))
        {
            heap_replace_top(out_begin, heap_end, std::addressof(*in_begin), heap_compare);
        }
    }
    std::sort_heap(out_begin, heap_end, heap_compare);
    return heap_end;
}
template<typename It, typename OutIt>
OutIt partial_sort_pointer(It in_begin, It in_end, OutIt out_begin, OutIt out_end)
{
    return partial_sort_pointer(in_begin, in_end, out_begin, out_end, std::less<>());
}

