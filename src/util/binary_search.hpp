#pragma once

#include <stdint.h>
#include <bit>
#include <functional>
#include <algorithm>

inline size_t bit_floor(size_t i)
{
    constexpr int num_bits = sizeof(i) * 8;
    return size_t(1) << (num_bits - std::countl_zero(i) - 1);
}
inline size_t bit_ceil(size_t i)
{
    constexpr int num_bits = sizeof(i) * 8;
    return size_t(1) << (num_bits - std::countl_zero(i - 1));
}

template<typename It, typename T, typename Cmp>
It branchless_lower_bound(It begin, It end, const T & value, Cmp && compare)
{
    uint64_t length = end - begin;
    if (length == 0)
        return end;
    size_t step = bit_floor(length);
    if (step != length && compare(begin[step], value))
        begin = end - step;
    for (step /= 2; step != 0; step /= 2)
    {
        //begin += step & -size_t(compare(begin[step], value));
        //begin = compare(begin[step], value) ? begin + step : begin;
        if (compare(begin[step], value))
            begin += step;
    }
    return begin + compare(*begin, value);
}

template<typename It, typename T>
It branchless_lower_bound(It begin, It end, const T & value)
{
    return branchless_lower_bound(begin, end, value, std::less<>{});
}


template<typename It, typename T, typename Cmp>
It branchless_lower_bound_odd(It begin, It end, const T & value, Cmp && compare)
{
    std::size_t length = end - begin;
    if (length == 0)
        return end;
    std::size_t step = bit_floor(length);
    if (step != length && compare(begin[step], value))
    {
        length -= step + 1;
        if (length == 0)
            return end;
        step = bit_ceil(length);
        begin = end - step;
    }
    for (step /= 2; step != 0; step /= 2)
    {
        It candidate = begin + step;
        if (compare(*candidate, value))
            begin = candidate;
    }
    return begin + compare(*begin, value);
}

template<typename It, typename T>
It branchless_lower_bound_odd(It begin, It end, const T & value)
{
    return branchless_lower_bound_odd(begin, end, value, std::less<>{});
}

template<typename It, typename T, typename Cmp>
It broken_branchless_lower_bound_non_pow(It begin, It end, const T & value, Cmp && compare)
{
    size_t length = end - begin;
    if (length == 0)
        return end;
    // length = 1
    // step = 0

    // length = 2
    // step = 1
    // compare(1)
    // step = 0
    // compare 0 or 1

    // length = 3
    // step = 2
    // compare(2)
    // step = 1
    // compare 0 or 3
    // index out of bounds at 3

    // length = 5
    // step = 2
    // compare(2)
    // step = 1
    // compare(3)
    // step = 0
    // we never compare 4

    // length = 6
    // step = 3
    // compare(3)
    // step = 1
    // compare(4)
    // we never compare 4 and 5
    size_t step = length;
    while (step != 1)
    {
        step = (step + 1) / 2;
        if (compare(begin[step], value))
            begin += step;
    }
    return begin + compare(*begin, value);
}

template<typename It, typename T>
It broken_branchless_lower_bound_non_pow(It begin, It end, const T & value)
{
    return broken_branchless_lower_bound_non_pow(begin, end, value, std::less<>{});
}

template<typename It, typename T, typename Cmp>
It branchless_lower_bound_idx(It begin, It end, const T & value, Cmp && compare)
{
    size_t length = end - begin;
    if (length == 0)
        return end;
    size_t idx = 0;
    std::size_t step = bit_floor(length);
    if (step != length && compare(begin[step], value))
    {
        size_t new_length = length - step - 1;
        if (new_length == 0)
            return end;
        step = bit_ceil(new_length);
        idx = length - step;
    }
    for (step /= 2; step != 0; step /= 2)
    {
        if (compare(begin[idx + step], value))
            idx += step;
    }
    return begin + idx + compare(begin[idx], value);
}

template<typename It, typename T>
It branchless_lower_bound_idx(It begin, It end, const T & value)
{
    return branchless_lower_bound_idx(begin, end, value, std::less<>{});
}
template<typename It, typename T, typename Cmp>
It branchless_lower_bound_unroll2(It begin, It end, const T & value, Cmp && compare)
{
    size_t length = end - begin;
    if (length == 0)
        return end;
    int num_steps = 63 - std::countl_zero(length);
    size_t step = 1llu << num_steps;
    if (step != length && compare(begin[step], value))
    {
        num_steps = 64 - std::countl_zero(length - step - 1);
        step = 1llu << num_steps;
        begin = end - step;
    }
    step /= 2;
    if (step) switch(num_steps % 2)
    {
    while (step != 0)
    {
    case 1:
        if (compare(begin[step], value))
            begin += step;
        step /= 2;
        [[fallthrough]];
    case 0:
        if (compare(begin[step], value))
            begin += step;
        step /= 2;
    }
    }
    return begin + compare(*begin, value);
}

template<typename It, typename T>
It branchless_lower_bound_unroll2(It begin, It end, const T & value)
{
    return branchless_lower_bound_unroll2(begin, end, value, std::less<>{});
}
template<typename It, typename T, typename Cmp>
It branchless_lower_bound_unroll4(It begin, It end, const T & value, Cmp && compare)
{
    size_t length = end - begin;
    if (length == 0)
        return end;
    int num_steps = 63 - std::countl_zero(length);
    size_t step = 1llu << num_steps;
    if (step != length && compare(begin[step], value))
    {
        num_steps = 64 - std::countl_zero(length - step - 1);
        step = 1llu << num_steps;
        begin = end - step;
    }
    step /= 2;
    if (step) switch(num_steps % 4)
    {
    while (step != 0)
    {
    case 3:
        if (compare(begin[step], value))
            begin += step;
        step /= 2;
        [[fallthrough]];
    case 2:
        if (compare(begin[step], value))
            begin += step;
        step /= 2;
        [[fallthrough]];
    case 1:
        if (compare(begin[step], value))
            begin += step;
        step /= 2;
        [[fallthrough]];
    case 0:
        if (compare(begin[step], value))
            begin += step;
        step /= 2;
    }
    }
    return begin + compare(*begin, value);
}

template<typename It, typename T>
It branchless_lower_bound_unroll4(It begin, It end, const T & value)
{
    return branchless_lower_bound_unroll4(begin, end, value, std::less<>{});
}
template<typename It, typename T, typename Cmp>
It branchless_lower_bound_unroll_all(It begin, It end, const T & value, Cmp && compare)
{
    size_t length = end - begin;
    if (length == 0)
        return end;
    int num_steps = 63 - std::countl_zero(length);
    size_t step = 1llu << num_steps;
    if (step != length && compare(begin[step], value))
    {
        num_steps = 64 - std::countl_zero(length - step - 1);
        step = 1llu << num_steps;
        begin = end - step;
    }
    switch(num_steps)
    {
    case 63:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 62:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 61:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 60:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 59:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 58:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 57:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 56:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 55:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 54:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 53:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 52:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 51:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 50:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 49:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 48:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 47:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 46:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 45:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 44:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 43:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 42:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 41:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 40:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 39:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 38:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 37:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 36:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 35:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 34:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 33:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 32:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 31:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 30:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 29:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 28:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 27:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 26:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 25:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 24:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 23:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 22:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 21:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 20:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 19:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 18:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 17:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 16:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 15:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 14:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 13:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 12:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 11:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 10:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 9:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 8:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 7:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 6:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 5:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 4:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 3:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 2:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 1:
        step /= 2;
        if (compare(begin[step], value)) begin += step;
        [[fallthrough]];
    case 0:
        break;
    }
    return begin + compare(*begin, value);
}

template<typename It, typename T>
It branchless_lower_bound_unroll_all(It begin, It end, const T & value)
{
    return branchless_lower_bound_unroll_all(begin, end, value, std::less<>{});
}

template<typename It, typename T, typename Cmp>
It branchless_lower_bound2(It first, It last, const T& value, Cmp && compare)
{
    while (last > first)
    {
        size_t length = last - first;
        size_t mid = length / 2;
        bool result = compare(first[mid], value);
        first += int( result) * (mid + 1);
        last  -= int(!result) * (length - mid);
    }
    return first;
}

template<typename It, typename T>
It branchless_lower_bound2(It begin, It end, const T & value)
{
    return branchless_lower_bound2(begin, end, value, std::less<>{});
}

template<typename It, typename T, typename Cmp>
It branchless_lower_bound3(It first, It last, const T& value, Cmp && compare)
{
    for (;;)
    {
        size_t length = last - first;
        if (length == 0)
            return first;
        size_t mid = length / 2;
        bool result = compare(first[mid], value);
        first += int( result) * (mid + 1);
        last  -= int(!result) * (length - mid);
    }
}

template<typename It, typename T>
It branchless_lower_bound3(It begin, It end, const T & value)
{
    return branchless_lower_bound3(begin, end, value, std::less<>{});
}

template<typename It, typename T, typename Cmp>
It branchless_lower_bound4(It first, It last, const T& value, Cmp && compare)
{
    for (;;)
    {
        size_t length = last - first;
        if (length == 0)
            return first;
        size_t mid = length / 2;
        size_t result = compare(first[mid], value);
        result = -result;
        first +=  result & (mid + 1);
        last  -= ~result & (length - mid);
    }
}

template<typename It, typename T>
It branchless_lower_bound4(It begin, It end, const T & value)
{
    return branchless_lower_bound4(begin, end, value, std::less<>{});
}

template<typename It, typename T, typename Cmp>
It custom_lower_bound(It first, It last, const T& value, Cmp && compare)
{
    auto length = last - first;
    while (length)
    {
        auto mid = length / 2;
        if (compare(first[mid], value))
        {
            ++mid;
            first += mid;
            length -= mid;
        }
        else
            length = mid;
    }
    return first;
}

template<typename It, typename T>
It custom_lower_bound(It begin, It end, const T & value)
{
    return custom_lower_bound(begin, end, value, std::less<>{});
}

template<typename It, typename T, typename Cmp>
It lower_bound_plus_linear(It first, It last, const T& value, Cmp && compare)
{
    auto length = last - first;
    while (length > 8)
    {
        auto mid = length / 2;
        if (compare(first[mid], value))
        {
            ++mid;
            first += mid;
            length -= mid;
        }
        else
            length = mid;
    }
    return std::find_if(first, last, [&](const auto & x)
    {
        return !compare(x, value);
    });
}

template<typename It, typename T>
It lower_bound_plus_linear(It begin, It end, const T & value)
{
    return lower_bound_plus_linear(begin, end, value, std::less<>{});
}

// https://en.algorithmica.org/hpc/data-structures/binary-search/
template<typename It, typename T, typename Cmp>
It algorithmica_lower_bound(It begin, It end, const T & value, Cmp && cmp)
{
    size_t len = end - begin;
    while (len > 1)
    {
        int half = len / 2;
        begin += cmp(begin[half - 1], value) * half; // will be replaced with a "cmov"
        len -= half;
    }
    if (begin != end)
        return begin + cmp(*begin, value);
    return begin;
}
template<typename It, typename T>
It algorithmica_lower_bound(It begin, It end, const T & value)
{
    return algorithmica_lower_bound(begin, end, value, std::less<>{});
}
template<typename It, typename T, typename Cmp>
It algorithmica_lower_bound_prefetch(It begin, It end, const T & value, Cmp && cmp)
{
    size_t len = end - begin;
    while (len > 1)
    {
        int half = len / 2;
        begin += cmp(begin[half - 1], value) * half; // will be replaced with a "cmov"
        __builtin_prefetch(std::addressof(begin[len / 2 - 1]));
        __builtin_prefetch(std::addressof(begin[half + len / 2 - 1]));

        len -= half;
    }
    if (begin != end)
        return begin + cmp(*begin, value);
    return begin;
}
template<typename It, typename T>
It algorithmica_lower_bound_prefetch(It begin, It end, const T & value)
{
    return algorithmica_lower_bound_prefetch(begin, end, value, std::less<>{});
}
template<typename It, typename T, typename Cmp>
It erthink_lower_bound(It begin, It end, const T & value, Cmp && cmp)
{
    size_t size = end - begin;
    while (size > 2)
    {
        It middle = begin + size / 2;
        size = (size + 1) / 2;
        begin = cmp(*middle, value) ? middle : begin;
    }
    begin += size > 1 && cmp(*begin, value);
    begin += size > 0 && cmp(*begin, value);
    return begin;
}
template<typename It, typename T>
It erthink_lower_bound(It begin, It end, const T & value)
{
    return erthink_lower_bound(begin, end, value, std::less<>{});
}
template<typename It, typename T, typename Cmp>
It erthink_lower_bound_tight(It begin, It end, const T & value, Cmp && cmp)
{
    size_t size = end - begin;
    if (size != 0)
    {
        while (size > 1)
        {
            It middle = begin + size / 2;
            size = (size + 1) / 2;
            if (cmp(*middle, value))
                begin = middle;
        }
        begin += cmp(*begin, value);
    }
    return begin;
}
template<typename It, typename T>
It erthink_lower_bound_tight(It begin, It end, const T & value)
{
    return erthink_lower_bound_tight(begin, end, value, std::less<>{});
}


template<typename It, typename T, typename Cmp>
It lower_bound_cmp_first(It begin, It end, const T & value, Cmp && cmp)
{
    if (begin == end || !cmp(*begin, value))
        return begin;
    for (std::size_t size = end - begin;; size = (size + 1) / 2)
    {
        if (std::size_t step = size / 2; step == 0)
            return begin + 1;
        else if (cmp(begin[step], value))
            begin += step;
    }
}
template<typename It, typename T>
It lower_bound_cmp_first(It begin, It end, const T & value)
{
    return lower_bound_cmp_first(begin, end, value, std::less<>{});
}

template<typename It, typename T, typename Cmp>
It lower_bound_skip_last(It begin, It end, const T & value, Cmp && cmp)
{
    if (begin == end)
        return end;
    It it = begin;
    for (std::size_t size = end - begin;; size = (size + 1) / 2)
    {
        std::size_t step = size / 2;
        if (step == 0)
            break;
        if (cmp(it[step], value))
            it += step;
    }
    if (it == begin)
        return it + cmp(*it, value);
    else
        return it + 1;
}
template<typename It, typename T>
It lower_bound_skip_last(It begin, It end, const T & value)
{
    return lower_bound_skip_last(begin, end, value, std::less<>{});
}

