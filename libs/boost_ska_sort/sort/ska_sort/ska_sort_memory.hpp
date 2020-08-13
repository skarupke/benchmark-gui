#pragma once

#ifndef BOOST_SKA_SORT_MEMORY_HPP
#define BOOST_SKA_SORT_MEMORY_HPP

#include <boost_ska_sort/sort/ska_sort/ska_sort_base.hpp>
#include <memory>

namespace boost
{

// forward declare boost::shared_ptr
template<class T>
class shared_ptr;

}

namespace boost_ska_sort::sort
{
template<typename T, typename D>
struct ska_sorter<std::unique_ptr<T, D>>
{
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        return sorter.sort([](const std::unique_ptr<T, D> & ptr)
        {
            return ptr.get();
        });
    }
};
template<typename T>
struct ska_sorter<std::shared_ptr<T>>
{
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        return sorter.sort([](const std::shared_ptr<T> & ptr)
        {
            return ptr.get();
        });
    }
};
template<typename T>
struct ska_sorter<boost::shared_ptr<T>>
{
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        return sorter.sort([](const boost::shared_ptr<T> & ptr)
        {
            return ptr.get();
        });
    }
};
}

#endif
