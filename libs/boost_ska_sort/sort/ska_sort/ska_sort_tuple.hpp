#pragma once

#ifndef BOOST_SKA_SORT_TUPLE_HPP
#define BOOST_SKA_SORT_TUPLE_HPP

#include <boost_ska_sort/sort/ska_sort/ska_sort_base.hpp>
#include <tuple>

namespace boost_ska_sort::sort
{
namespace detail_ska_sort
{
template<size_t Index>
struct get_at_index_tuple
{
    template<typename... Args>
    decltype(auto) operator()(const std::tuple<Args...> & value) const
    {
        return std::get<Index>(value);
    }
};
}

template<typename... Args>
struct ska_sorter<std::tuple<Args...>>
{
    template<size_t Index, typename Sorter>
    static void sort_at_index(Sorter & sorter)
    {
        if constexpr (Index + 1 == std::tuple_size_v<std::tuple<Args...>>)
        {
            sorter.sort(detail_ska_sort::get_at_index_tuple<Index>{});
        }
        else
        {
            sorter.sort(detail_ska_sort::get_at_index_tuple<Index>{}, &sort_at_index<Index + 1, Sorter>);
        }
    }
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        if constexpr (std::tuple_size_v<std::tuple<Args...>> == 0)
            sorter.skip();
        else
            sort_at_index<0>(sorter);
    }
};

}

#endif
