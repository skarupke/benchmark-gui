#pragma once

#ifndef BOOST_SKA_SORT_ALGORITHM_HPP
#define BOOST_SKA_SORT_ALGORITHM_HPP

#include <boost/config.hpp>
#include <utility>
#include <algorithm>

namespace boost_ska_sort::sort::detail_ska_sort
{

template<typename It, typename Compare>
BOOST_NOINLINE void insertion_sort_length_2_or_more(It begin, It end, Compare compare)
{
    for (It it = begin + 1;;)
    {
        if (compare(it[0], it[-1]))
        {
            auto to_swap = std::move(*it);
            It to_move = it - 1;
            for (;; --to_move)
            {
                to_move[1] = std::move(to_move[0]);
                if (to_move == begin || !compare(to_swap, to_move[-1]))
                    break;
            }
            *to_move = std::move(to_swap);
        }
        ++it;
        if (it == end)
            break;
    }
}
template<typename It, typename Compare>
inline void insertion_sort(It begin, It end, Compare compare)
{
    if (end - begin > 1)
        return insertion_sort_length_2_or_more(begin, end, compare);
}

// unrolls the loop UnrollAmount times
// iteration_count has to be non-zero
template<std::size_t UnrollAmount, typename It, typename Func>
inline void unroll_loop_nonempty(It begin, size_t iteration_count, Func && to_call)
{
    static_assert(UnrollAmount >= 1 && UnrollAmount <= 8, "Currently only support up to 8 loop unrollings");
    std::size_t loop_count = (iteration_count + (UnrollAmount - 1)) / UnrollAmount;
    std::size_t remainder_count = iteration_count % UnrollAmount;
    begin += remainder_count;
    switch(remainder_count)
    {
    case 0:
        do
        {
            begin += UnrollAmount;
            if constexpr (UnrollAmount >= 8)
                to_call(begin - 8);
            [[fallthrough]];
    case 7:
            if constexpr (UnrollAmount >= 7)
                to_call(begin - 7);
            [[fallthrough]];
    case 6:
            if constexpr (UnrollAmount >= 6)
                to_call(begin - 6);
            [[fallthrough]];
    case 5:
            if constexpr (UnrollAmount >= 5)
                to_call(begin - 5);
            [[fallthrough]];
    case 4:
            if constexpr (UnrollAmount >= 4)
                to_call(begin - 4);
            [[fallthrough]];
    case 3:
            if constexpr (UnrollAmount >= 3)
                to_call(begin - 3);
            [[fallthrough]];
    case 2:
            if constexpr (UnrollAmount >= 2)
                to_call(begin - 2);
            [[fallthrough]];
    case 1:
            to_call(begin - 1);
            --loop_count;
        }
        while(loop_count > 0);
    }
}

// exactly the same behavior as std::partition, except it only works
// on no-empty ranges.
// I needed to write my own version because one implementation of
// std::partition was a bit too aggressive about unrolling this loop,
// which was a bad thing to do for my code. and then my code only
// ran on non-empty ranges, so I decided to take advantage of that
template<typename It, typename F>
inline It std_partition_non_empty(It begin, It end, F && func)
{
    while (func(*begin))
    {
        ++begin;
        if (begin == end)
            return begin;
    }
    It it = begin;
    for(++it; it != end; ++it)
    {
        if (!func(*it))
            continue;

        std::iter_swap(begin, it);
        ++begin;
    }
    return begin;
}
}

#endif
