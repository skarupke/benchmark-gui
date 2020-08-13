#pragma once

#ifndef BOOST_SKA_SORT_VARIANT_HPP
#define BOOST_SKA_SORT_VARIANT_HPP

#include <boost_ska_sort/sort/ska_sort/ska_sort_base.hpp>
#include <variant>

namespace boost_ska_sort::sort
{
namespace detail_ska_sort
{
template<size_t Index>
struct get_at_index_variant
{
    template<typename... Args>
    decltype(auto) operator()(const std::variant<Args...> & value) const
    {
        return std::get<Index>(value);
    }
};
}

template<typename... Args>
struct ska_sorter<std::variant<Args...>>
{
    template<typename>
    struct indexed_sorter;
    template<size_t... Indices>
    struct indexed_sorter<std::integer_sequence<size_t, Indices...>>
    {
        template<size_t Index, typename Sorter>
        static void sort_subindex(Sorter & sorter)
        {
            sorter.sort(detail_ska_sort::get_at_index_variant<Index>{});
        }

        template<typename Sorter>
        static void sort_after_index(Sorter & sorter)
        {
            size_t index = sorter.first_item().index();
            if (index == std::variant_npos)
            {
                sorter.skip();
                return;
            }
            static constexpr void (*subindex_sorts[sizeof...(Args)])(Sorter &) =
            {
                &sort_subindex<Indices, Sorter>...,
            };
            subindex_sorts[index](sorter);
        }
    };
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        sorter.sort([](const std::variant<Args...> & value)
        {
            if constexpr (sizeof...(Args) < std::numeric_limits<std::uint8_t>::max())
                return static_cast<std::uint8_t>(value.index() + 1);
            else if constexpr(sizeof...(Args) < std::numeric_limits<std::uint16_t>::max())
                return static_cast<std::uint16_t>(value.index() + 1);
            else if constexpr(sizeof...(Args) < std::numeric_limits<std::uint32_t>::max())
                return static_cast<std::uint32_t>(value.index() + 1);
            else
                return value.index() + 1;
        }, &indexed_sorter<std::make_index_sequence<sizeof...(Args)>>::template sort_after_index<Sorter>);
    }
};

}

#endif
