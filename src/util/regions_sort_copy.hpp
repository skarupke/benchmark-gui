#pragma once

#include <boost_ska_sort/sort/ska_sort/ska_sort.hpp>

namespace boost_ska_sort::sort
{

template<typename It, typename ExtractKey>
void regions_sort(It begin, It end, ExtractKey && extract_key)
{
    return ska_sort(begin, end, extract_key);
}
template<typename It>
void regions_sort(It begin, It end)
{
    return regions_sort(begin, end, detail_ska_sort::identity_function<typename std::iterator_traits<It>::value_type>());
}
template<typename SortSettings, typename It, typename ExtractKey>
void regions_sort_with_settings(It begin, It end, ExtractKey && extract_key)
{
    return detail_ska_sort::ska_sort_with_settings<SortSettings>(begin, end, extract_key);
}
template<typename SortSettings, typename It>
void regions_sort_with_settings(It begin, It end)
{
    return regions_sort_with_settings<SortSettings>(begin, end, detail_ska_sort::identity_function<typename std::iterator_traits<It>::value_type>());
}

}
