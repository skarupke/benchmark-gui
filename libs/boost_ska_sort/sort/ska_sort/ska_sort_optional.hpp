#pragma once

#ifndef BOOST_SKA_SORT_OPTIONAL_HPP
#define BOOST_SKA_SORT_OPTIONAL_HPP

#include <boost_ska_sort/sort/ska_sort/ska_sort_base.hpp>
#include <optional>

namespace boost
{

// forward declare boost::optional
template<class T>
class optional;

}

namespace boost_ska_sort::sort
{
template<typename T>
struct ska_sorter<std::optional<T>>
{
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        sorter.sort([](const std::optional<T> & value) -> bool
        {
            return value.has_value();
        }, &sort_after_bool<Sorter>);
    }

	template<typename Sorter>
	static void sort_after_bool(Sorter & sorter)
	{
		if (sorter.first_item().has_value())
		{
			sorter.sort([](const std::optional<T> & value) -> const T &
			{
				return *value;
			});
		}
		else
			sorter.skip();
	}
};
template<typename T>
struct ska_sorter<boost::optional<T>>
{
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        sorter.sort([](const boost::optional<T> & value) -> bool
        {
            return value.has_value();
        }, &sort_after_bool<Sorter>);
    }

	template<typename Sorter>
	static void sort_after_bool(Sorter & sorter)
	{
		if (sorter.first_item().has_value())
		{
			sorter.sort([](const boost::optional<T> & value) -> const T &
			{
				return *value;
			});
		}
		else
			sorter.skip();
	}
};

}


#endif
