//          Copyright Malte Skarupke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>
#include <algorithm>
#include <type_traits>
#include <tuple>
#include <utility>
#include <variant>
#include <optional>
#include <limits>
#include "util/overloaded_call_wrapper.hpp"

// todo: support all types in ping_pong that I support in the
// in-place version

// maybe todo: sort std::vector<std::list> or std::vector<std::map>
// doesn't have to be super efficient. std::sort also does badly in
// that one. but it may be worth it to get it into boost

namespace detail
{
template<typename count_type, typename It, typename OutIt, typename ExtractKey>
void counting_sort_impl(It begin, It end, OutIt out_begin, ExtractKey && extract_key)
{
    count_type counts[256] = {};
    for (It it = begin; it != end; ++it)
    {
        ++counts[extract_key(*it)];
    }
    count_type total = 0;
    for (count_type & count : counts)
    {
        count_type old_count = count;
        count = total;
        total += old_count;
    }
    for (; begin != end; ++begin)
    {
        std::uint8_t key = extract_key(*begin);
        out_begin[counts[key]++] = std::move(*begin);
    }
}
template<typename It, typename OutIt, typename ExtractKey>
void counting_sort_impl(It begin, It end, OutIt out_begin, ExtractKey && extract_key)
{
    counting_sort_impl<std::uint64_t>(begin, end, out_begin, extract_key);
}
inline bool to_unsigned_or_bool(bool b)
{
    return b;
}
inline unsigned char to_unsigned_or_bool(unsigned char c)
{
    return c;
}
inline unsigned char to_unsigned_or_bool(signed char c)
{
    constexpr unsigned char sign_bit = 128;
    return static_cast<unsigned char>(c) ^ sign_bit;
}
inline unsigned char to_unsigned_or_bool(char c)
{
    if constexpr (std::is_signed_v<char>)
    {
        constexpr unsigned char sign_bit = 128;
        return static_cast<unsigned char>(c) ^ sign_bit;
    }
    else
        return static_cast<unsigned char>(c);
}
inline std::uint16_t to_unsigned_or_bool(char16_t c)
{
    return static_cast<std::uint16_t>(c);
}
inline std::uint32_t to_unsigned_or_bool(char32_t c)
{
    return static_cast<std::uint32_t>(c);
}
inline std::uint32_t to_unsigned_or_bool(wchar_t c)
{
    return static_cast<std::uint32_t>(c);
}
inline unsigned short to_unsigned_or_bool(short i)
{
    constexpr unsigned short sign_bit = static_cast<unsigned short>(1 << (sizeof(short) * 8 - 1));
    return static_cast<unsigned short>(i) ^ sign_bit;
}
inline unsigned short to_unsigned_or_bool(unsigned short i)
{
    return i;
}
inline unsigned int to_unsigned_or_bool(int i)
{
    constexpr unsigned int sign_bit = static_cast<unsigned int>(1 << (sizeof(int) * 8 - 1));
    return static_cast<unsigned int>(i) ^ sign_bit;
}
inline unsigned int to_unsigned_or_bool(unsigned int i)
{
    return i;
}
inline unsigned long to_unsigned_or_bool(long l)
{
    constexpr unsigned long sign_bit = static_cast<unsigned long>(1l << (sizeof(long) * 8 - 1));
    return static_cast<unsigned long>(l) ^ sign_bit;
}
inline unsigned long to_unsigned_or_bool(unsigned long l)
{
    return l;
}
inline unsigned long long to_unsigned_or_bool(long long l)
{
    constexpr unsigned long long sign_bit = static_cast<unsigned long long>(1ll << (sizeof(long long) * 8 - 1));
    return static_cast<unsigned long long>(l) ^ sign_bit;
}
inline unsigned long long to_unsigned_or_bool(unsigned long long l)
{
    return l;
}
inline std::uint32_t to_unsigned_or_bool(float f)
{
    static_assert(std::numeric_limits<float>::is_iec559);
    union
    {
        float f;
        std::uint32_t u;
    } as_union = { f };
    std::uint32_t sign_bit = -std::int32_t(as_union.u >> 31);
    return as_union.u ^ (sign_bit | 0x80000000);
}
inline std::uint64_t to_unsigned_or_bool(double f)
{
    static_assert(std::numeric_limits<double>::is_iec559);
    union
    {
        double d;
        std::uint64_t u;
    } as_union = { f };
    std::uint64_t sign_bit = -std::int64_t(as_union.u >> 63);
    return as_union.u ^ (sign_bit | 0x8000000000000000);
}
template<typename T>
inline size_t to_unsigned_or_bool(T * ptr)
{
    return reinterpret_cast<size_t>(ptr);
}

template<typename It, typename Compare>
void __attribute__((noinline)) insertion_sort_length_2_or_more(It begin, It end, Compare compare)
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
    /*
    It it = begin;
    for (;;)
    {
        It to_move = it;
        ++it;
        if (it == end)
            break;
        if (!compare(*it, *to_move))
            continue;
        auto to_swap = std::move(*it);
        if (compare(to_swap, *begin))
        {
            std::move_backward(begin, it, it + 1);
            *begin = std::move(to_swap);
            continue;
        }
        for (It to_insert = it;;)
        {
            *to_insert = std::move(*to_move);
            --to_move;
            --to_insert;
            if (!compare(to_swap, *to_move))
            {
                *to_insert = std::move(to_swap);
                break;
            }
        }
    }*/
}

template<typename It, typename Compare>
inline It min_element_length_2_or_more(It begin, It end, Compare comp)
{
    It result = begin;
    for (++begin;;)
    {
        if (comp(*begin, *result))
            result = begin;
        ++begin;
        if (begin == end)
            return result;
    }
}

template<typename It, typename Compare>
inline void insertion_sort(It begin, It end, Compare compare)
{
    if (end - begin > 1)
        return insertion_sort_length_2_or_more(begin, end, compare);
}

template<size_t>
struct SizedRadixSorter;

template<>
struct SizedRadixSorter<1>
{
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
    {
        counting_sort_impl(begin, end, buffer_begin, [&](auto && o)
        {
            return to_unsigned_or_bool(extract_key(o));
        });
        return true;
    }

    static constexpr size_t pass_count = 2;
};
template<>
struct SizedRadixSorter<2>
{
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
    {
        std::ptrdiff_t num_elements = end - begin;
        if (num_elements <= (1ll << 32))
            return sort_inline<uint32_t>(begin, end, buffer_begin, buffer_begin + num_elements, extract_key);
        else
            return sort_inline<uint64_t>(begin, end, buffer_begin, buffer_begin + num_elements, extract_key);
    }

    template<typename count_type, typename It, typename OutIt, typename ExtractKey>
    static bool sort_inline(It begin, It end, OutIt out_begin, OutIt out_end, ExtractKey && extract_key)
    {
        count_type counts0[256] = {};
        count_type counts1[256] = {};

        for (It it = begin; it != end; ++it)
        {
            uint16_t key = to_unsigned_or_bool(extract_key(*it));
            ++counts0[key & 0xff];
            ++counts1[(key >> 8) & 0xff];
        }
        count_type total0 = 0;
        count_type total1 = 0;
        for (int i = 0; i < 256; ++i)
        {
            count_type old_count0 = counts0[i];
            count_type old_count1 = counts1[i];
            counts0[i] = total0;
            counts1[i] = total1;
            total0 += old_count0;
            total1 += old_count1;
        }
        for (It it = begin; it != end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it));
            out_begin[counts0[key]++] = std::move(*it);
        }
        for (OutIt it = out_begin; it != out_end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it)) >> 8;
            begin[counts1[key]++] = std::move(*it);
        }
        return false;
    }

    static constexpr size_t pass_count = 3;
};
template<>
struct SizedRadixSorter<4>
{

    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
    {
        std::ptrdiff_t num_elements = end - begin;
        if (num_elements <= (1ll << 32))
            return sort_inline<uint32_t>(begin, end, buffer_begin, buffer_begin + num_elements, extract_key);
        else
            return sort_inline<uint64_t>(begin, end, buffer_begin, buffer_begin + num_elements, extract_key);
    }
    template<typename count_type, typename It, typename OutIt, typename ExtractKey>
    static bool sort_inline(It begin, It end, OutIt out_begin, OutIt out_end, ExtractKey && extract_key)
    {
        count_type counts0[256] = {};
        count_type counts1[256] = {};
        count_type counts2[256] = {};
        count_type counts3[256] = {};

        for (It it = begin; it != end; ++it)
        {
            uint32_t key = to_unsigned_or_bool(extract_key(*it));
            ++counts0[key & 0xff];
            ++counts1[(key >> 8) & 0xff];
            ++counts2[(key >> 16) & 0xff];
            ++counts3[(key >> 24) & 0xff];
        }
        count_type total0 = 0;
        count_type total1 = 0;
        count_type total2 = 0;
        count_type total3 = 0;
        for (int i = 0; i < 256; ++i)
        {
            count_type old_count0 = counts0[i];
            count_type old_count1 = counts1[i];
            count_type old_count2 = counts2[i];
            count_type old_count3 = counts3[i];
            counts0[i] = total0;
            counts1[i] = total1;
            counts2[i] = total2;
            counts3[i] = total3;
            total0 += old_count0;
            total1 += old_count1;
            total2 += old_count2;
            total3 += old_count3;
        }
        for (It it = begin; it != end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it));
            out_begin[counts0[key]++] = std::move(*it);
        }
        for (OutIt it = out_begin; it != out_end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it)) >> 8;
            begin[counts1[key]++] = std::move(*it);
        }
        for (It it = begin; it != end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it)) >> 16;
            out_begin[counts2[key]++] = std::move(*it);
        }
        for (OutIt it = out_begin; it != out_end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it)) >> 24;
            begin[counts3[key]++] = std::move(*it);
        }
        return false;
    }

    static constexpr size_t pass_count = 5;
};
template<>
struct SizedRadixSorter<8>
{
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
    {
        std::ptrdiff_t num_elements = end - begin;
        if (num_elements <= (1ll << 32))
            return sort_inline<uint32_t>(begin, end, buffer_begin, buffer_begin + num_elements, extract_key);
        else
            return sort_inline<uint64_t>(begin, end, buffer_begin, buffer_begin + num_elements, extract_key);
    }
    template<typename count_type, typename It, typename OutIt, typename ExtractKey>
    static bool sort_inline(It begin, It end, OutIt out_begin, OutIt out_end, ExtractKey && extract_key)
    {
        count_type counts0[256] = {};
        count_type counts1[256] = {};
        count_type counts2[256] = {};
        count_type counts3[256] = {};
        count_type counts4[256] = {};
        count_type counts5[256] = {};
        count_type counts6[256] = {};
        count_type counts7[256] = {};

        for (It it = begin; it != end; ++it)
        {
            uint64_t key = to_unsigned_or_bool(extract_key(*it));
            ++counts0[key & 0xff];
            ++counts1[(key >> 8) & 0xff];
            ++counts2[(key >> 16) & 0xff];
            ++counts3[(key >> 24) & 0xff];
            ++counts4[(key >> 32) & 0xff];
            ++counts5[(key >> 40) & 0xff];
            ++counts6[(key >> 48) & 0xff];
            ++counts7[(key >> 56) & 0xff];
        }
        count_type total0 = 0;
        count_type total1 = 0;
        count_type total2 = 0;
        count_type total3 = 0;
        count_type total4 = 0;
        count_type total5 = 0;
        count_type total6 = 0;
        count_type total7 = 0;
        for (int i = 0; i < 256; ++i)
        {
            count_type old_count0 = counts0[i];
            count_type old_count1 = counts1[i];
            count_type old_count2 = counts2[i];
            count_type old_count3 = counts3[i];
            count_type old_count4 = counts4[i];
            count_type old_count5 = counts5[i];
            count_type old_count6 = counts6[i];
            count_type old_count7 = counts7[i];
            counts0[i] = total0;
            counts1[i] = total1;
            counts2[i] = total2;
            counts3[i] = total3;
            counts4[i] = total4;
            counts5[i] = total5;
            counts6[i] = total6;
            counts7[i] = total7;
            total0 += old_count0;
            total1 += old_count1;
            total2 += old_count2;
            total3 += old_count3;
            total4 += old_count4;
            total5 += old_count5;
            total6 += old_count6;
            total7 += old_count7;
        }
        for (It it = begin; it != end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it));
            out_begin[counts0[key]++] = std::move(*it);
        }
        for (OutIt it = out_begin; it != out_end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it)) >> 8;
            begin[counts1[key]++] = std::move(*it);
        }
        for (It it = begin; it != end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it)) >> 16;
            out_begin[counts2[key]++] = std::move(*it);
        }
        for (OutIt it = out_begin; it != out_end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it)) >> 24;
            begin[counts3[key]++] = std::move(*it);
        }
        for (It it = begin; it != end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it)) >> 32;
            out_begin[counts4[key]++] = std::move(*it);
        }
        for (OutIt it = out_begin; it != out_end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it)) >> 40;
            begin[counts5[key]++] = std::move(*it);
        }
        for (It it = begin; it != end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it)) >> 48;
            out_begin[counts6[key]++] = std::move(*it);
        }
        for (OutIt it = out_begin; it != out_end; ++it)
        {
            std::uint8_t key = to_unsigned_or_bool(extract_key(*it)) >> 56;
            begin[counts7[key]++] = std::move(*it);
        }
        return false;
    }

    static constexpr size_t pass_count = 9;
};

template<typename>
struct RadixSorter;
template<>
struct RadixSorter<bool>
{
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
    {
        size_t false_count = 0;
        for (It it = begin; it != end; ++it)
        {
            if (!extract_key(*it))
                ++false_count;
        }
        size_t true_position = false_count;
        false_count = 0;
        for (; begin != end; ++begin)
        {
            if (extract_key(*begin))
                buffer_begin[true_position++] = std::move(*begin);
            else
                buffer_begin[false_count++] = std::move(*begin);
        }
        return true;
    }

    static constexpr size_t pass_count = 2;
};
template<>
struct RadixSorter<signed char> : SizedRadixSorter<sizeof(signed char)>
{
};
template<>
struct RadixSorter<unsigned char> : SizedRadixSorter<sizeof(unsigned char)>
{
};
template<>
struct RadixSorter<signed short> : SizedRadixSorter<sizeof(signed short)>
{
};
template<>
struct RadixSorter<unsigned short> : SizedRadixSorter<sizeof(unsigned short)>
{
};
template<>
struct RadixSorter<signed int> : SizedRadixSorter<sizeof(signed int)>
{
};
template<>
struct RadixSorter<unsigned int> : SizedRadixSorter<sizeof(unsigned int)>
{
};
template<>
struct RadixSorter<signed long> : SizedRadixSorter<sizeof(signed long)>
{
};
template<>
struct RadixSorter<unsigned long> : SizedRadixSorter<sizeof(unsigned long)>
{
};
template<>
struct RadixSorter<signed long long> : SizedRadixSorter<sizeof(signed long long)>
{
};
template<>
struct RadixSorter<unsigned long long> : SizedRadixSorter<sizeof(unsigned long long)>
{
};
template<>
struct RadixSorter<float> : SizedRadixSorter<sizeof(float)>
{
};
template<>
struct RadixSorter<double> : SizedRadixSorter<sizeof(double)>
{
};
template<>
struct RadixSorter<char> : SizedRadixSorter<sizeof(char)>
{
};
template<>
struct RadixSorter<wchar_t> : SizedRadixSorter<sizeof(wchar_t)>
{
};
template<>
struct RadixSorter<char16_t> : SizedRadixSorter<sizeof(char16_t)>
{
};
template<>
struct RadixSorter<char32_t> : SizedRadixSorter<sizeof(char32_t)>
{
};
template<typename K, typename V>
struct RadixSorter<std::pair<K, V>>
{
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
    {
        bool first_result = RadixSorter<V>::sort(begin, end, buffer_begin, [&](auto && o)
        {
            return extract_key(o).second;
        });
        auto extract_first = [&](auto && o)
        {
            return extract_key(o).first;
        };

        if (first_result)
        {
            return !RadixSorter<K>::sort(buffer_begin, buffer_begin + (end - begin), begin, extract_first);
        }
        else
        {
            return RadixSorter<K>::sort(begin, end, buffer_begin, extract_first);
        }
    }

    static constexpr size_t pass_count = RadixSorter<K>::pass_count + RadixSorter<V>::pass_count;
};
template<typename K, typename V>
struct RadixSorter<const std::pair<K, V> &>
{
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
    {
        bool first_result = RadixSorter<V>::sort(begin, end, buffer_begin, [&](auto && o) -> const V &
        {
            return extract_key(o).second;
        });
        auto extract_first = [&](auto && o) -> const K &
        {
            return extract_key(o).first;
        };

        if (first_result)
        {
            return !RadixSorter<K>::sort(buffer_begin, buffer_begin + (end - begin), begin, extract_first);
        }
        else
        {
            return RadixSorter<K>::sort(begin, end, buffer_begin, extract_first);
        }
    }

    static constexpr size_t pass_count = RadixSorter<K>::pass_count + RadixSorter<V>::pass_count;
};
template<size_t I, size_t S, typename Tuple>
struct TupleRadixSorter
{
    using NextSorter = TupleRadixSorter<I + 1, S, Tuple>;
    using ThisSorter = RadixSorter<typename std::tuple_element<I, Tuple>::type>;

    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt out_begin, OutIt out_end, ExtractKey && extract_key)
    {
        bool which = NextSorter::sort(begin, end, out_begin, out_end, extract_key);
        auto extract_i = [&](auto && o)
        {
            return std::get<I>(extract_key(o));
        };
        if (which)
            return !ThisSorter::sort(out_begin, out_end, begin, extract_i);
        else
            return ThisSorter::sort(begin, end, out_begin, extract_i);
    }

    static constexpr size_t pass_count = ThisSorter::pass_count + NextSorter::pass_count;
};
template<size_t I, size_t S, typename Tuple>
struct TupleRadixSorter<I, S, const Tuple &>
{
    using NextSorter = TupleRadixSorter<I + 1, S, const Tuple &>;
    using ThisSorter = RadixSorter<typename std::tuple_element<I, Tuple>::type>;

    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt out_begin, OutIt out_end, ExtractKey && extract_key)
    {
        bool which = NextSorter::sort(begin, end, out_begin, out_end, extract_key);
        auto extract_i = [&](auto && o) -> decltype(auto)
        {
            return std::get<I>(extract_key(o));
        };
        if (which)
            return !ThisSorter::sort(out_begin, out_end, begin, extract_i);
        else
            return ThisSorter::sort(begin, end, out_begin, extract_i);
    }

    static constexpr size_t pass_count = ThisSorter::pass_count + NextSorter::pass_count;
};
template<size_t I, typename Tuple>
struct TupleRadixSorter<I, I, Tuple>
{
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It, It, OutIt, OutIt, ExtractKey &&)
    {
        return false;
    }

    static constexpr size_t pass_count = 0;
};
template<size_t I, typename Tuple>
struct TupleRadixSorter<I, I, const Tuple &>
{
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It, It, OutIt, OutIt, ExtractKey &&)
    {
        return false;
    }

    static constexpr size_t pass_count = 0;
};

template<typename... Args>
struct RadixSorter<std::tuple<Args...>>
{
    using SorterImpl = TupleRadixSorter<0, sizeof...(Args), std::tuple<Args...>>;

    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
    {
        return SorterImpl::sort(begin, end, buffer_begin, buffer_begin + (end - begin), extract_key);
    }

    static constexpr size_t pass_count = SorterImpl::pass_count;
};

template<typename... Args>
struct RadixSorter<const std::tuple<Args...> &>
{
    using SorterImpl = TupleRadixSorter<0, sizeof...(Args), const std::tuple<Args...> &>;

    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
    {
        return SorterImpl::sort(begin, end, buffer_begin, buffer_begin + (end - begin), extract_key);
    }

    static constexpr size_t pass_count = SorterImpl::pass_count;
};

template<typename T, size_t S>
struct RadixSorter<std::array<T, S>>
{
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
    {
        auto buffer_end = buffer_begin + (end - begin);
        bool which = false;
        for (size_t i = S; i > 0; --i)
        {
            auto extract_i = [&, i = i - 1](auto && o)
            {
                return extract_key(o)[i];
            };
            if (which)
                which = !RadixSorter<T>::sort(buffer_begin, buffer_end, begin, extract_i);
            else
                which = RadixSorter<T>::sort(begin, end, buffer_begin, extract_i);
        }
        return which;
    }

    static constexpr size_t pass_count = RadixSorter<T>::pass_count * S;
};
template<typename... Args>
struct MaxPassCount;
template<typename First, typename... Args>
struct MaxPassCount<First, Args...>
{
    static constexpr size_t value = std::max(RadixSorter<First>::pass_count, MaxPassCount<Args...>::value);
};
template<typename First>
struct MaxPassCount<First>
{
    static constexpr size_t value = RadixSorter<First>::pass_count;
};

template<typename T>
struct RadixSorter<const T> : RadixSorter<T>
{
};
template<typename T>
struct RadixSorter<T &> : RadixSorter<const T &>
{
};
template<typename T>
struct RadixSorter<T &&> : RadixSorter<T>
{
};
template<typename T>
struct RadixSorter<const T &> : RadixSorter<T>
{
};
template<typename T>
struct RadixSorter<const T &&> : RadixSorter<T>
{
};
// these structs serve two purposes
// 1. they serve as illustration for how to implement the to_radix_sort_key function
// 2. they help produce better error messages. with these overloads you get the
//    error message "no matching function for call to to_radix_sort(your_type)"
//    without these examples, you'd get the error message "to_radix_sort_key was
//    not declared in this scope" which is a much less useful error message
struct ExampleStructA { int i; };
struct ExampleStructB { float f; };
inline int to_radix_sort_key(ExampleStructA a) { return a.i; }
inline float to_radix_sort_key(ExampleStructB b) { return b.f; }
template<typename T, typename Enable = void>
struct FallbackRadixSorter : RadixSorter<decltype(to_radix_sort_key(std::declval<T>()))>
{
    using base = RadixSorter<decltype(to_radix_sort_key(std::declval<T>()))>;

    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
    {
        return base::sort(begin, end, buffer_begin, [&](auto && a) -> decltype(auto)
        {
            return to_radix_sort_key(extract_key(a));
        });
    }
};

template<typename...>
struct void_t
{
    using type = void;
};

template<typename T>
struct FallbackRadixSorter<T, typename void_t<decltype(to_unsigned_or_bool(std::declval<T>()))>::type>
    : RadixSorter<decltype(to_unsigned_or_bool(std::declval<T>()))>
{
};

template<typename T>
struct RadixSorter : FallbackRadixSorter<T>
{
};

template<typename T>
size_t radix_sort_pass_count = RadixSorter<T>::pass_count;

template<size_t UnrollAmount, typename It, typename Func>
inline void unroll_loop_nonempty(It begin, size_t iteration_count, Func && to_call)
{
    /*static_assert(UnrollAmount >= 1 && UnrollAmount <= 8, "Currently only support up to 8 loop unrollings");
    size_t loop_count = iteration_count / UnrollAmount;
    size_t remainder_count = iteration_count % UnrollAmount;
    switch(remainder_count)
    {
    case 0:
        do
        {
            --loop_count;
            if constexpr (UnrollAmount >= 8)
            {
                to_call(begin);
                ++begin;
            }
            [[fallthrough]];
    case 7:
            if constexpr (UnrollAmount >= 7)
            {
                to_call(begin);
                ++begin;
            }
            [[fallthrough]];
    case 6:
            if constexpr (UnrollAmount >= 6)
            {
                to_call(begin);
                ++begin;
            }
            [[fallthrough]];
    case 5:
            if constexpr (UnrollAmount >= 5)
            {
                to_call(begin);
                ++begin;
            }
            [[fallthrough]];
    case 4:
            if constexpr (UnrollAmount >= 4)
            {
                to_call(begin);
                ++begin;
            }
            [[fallthrough]];
    case 3:
            if constexpr (UnrollAmount >= 3)
            {
                to_call(begin);
                ++begin;
            }
            [[fallthrough]];
    case 2:
            if constexpr (UnrollAmount >= 2)
            {
                to_call(begin);
                ++begin;
            }
            [[fallthrough]];
    case 1:
            to_call(begin);
            ++begin;
        }
        while(loop_count > 0);
    }*/
    static_assert(UnrollAmount >= 1 && UnrollAmount <= 8, "Currently only support up to 8 loop unrollings");
    size_t loop_count = (iteration_count + (UnrollAmount - 1)) / UnrollAmount;
    size_t remainder_count = iteration_count % UnrollAmount;
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

// exactly the same behavior as std::partition.
// I needed to write my own version because one implementation of
// std::partition was a bit too aggressive about unrolling this loop,
// which was a bad thing to do for my code
template<typename It, typename F>
inline It custom_std_partition(It begin, It end, F && func)
{
    for (;; ++begin)
    {
        if (begin == end)
            return begin;
        if (!func(*begin))
            break;
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

template<typename It, typename OutIt, typename F>
inline OutIt partition_copy(It begin, It end, OutIt out_begin, OutIt out_end, F && func)
{
    for (; begin != end; ++begin)
    {
        if (func(*begin))
        {
            *out_begin = std::move(*begin);
            ++out_begin;
        }
        else
        {
            --out_end;
            *out_end = std::move(*begin);
        }
    }
    return out_begin;
}

template<size_t>
struct UnsignedForSize;
template<>
struct UnsignedForSize<1>
{
    typedef uint8_t type;
};
template<>
struct UnsignedForSize<2>
{
    typedef uint16_t type;
};
template<>
struct UnsignedForSize<4>
{
    typedef uint32_t type;
};
template<>
struct UnsignedForSize<8>
{
    typedef uint64_t type;
};
template<typename T, typename ExtractKey>
struct SubKey;
template<size_t Size, typename ExtractKey>
struct SizedSubKey
{
    template<typename T>
    static auto sub_key(T && value, void *, ExtractKey &)
    {
        return to_unsigned_or_bool(value);
    }

    typedef SubKey<void, ExtractKey> next;

    using sub_key_type = typename UnsignedForSize<Size>::type;
};
template<typename T, typename ExtractKey>
struct SubKey<const T, ExtractKey> : SubKey<T, ExtractKey>
{
};
template<typename T, typename ExtractKey>
struct SubKey<T &, ExtractKey> : SubKey<T, ExtractKey>
{
};
template<typename T, typename ExtractKey>
struct SubKey<T &&, ExtractKey> : SubKey<T, ExtractKey>
{
};
template<typename T, typename ExtractKey>
struct SubKey<const T &, ExtractKey> : SubKey<T, ExtractKey>
{
};
template<typename T, typename ExtractKey>
struct SubKey<const T &&, ExtractKey> : SubKey<T, ExtractKey>
{
};
template<typename T, typename ExtractKey, typename Enable = void>
struct FallbackSubKey2
    : SubKey<decltype(to_radix_sort_key(std::declval<T>())), ExtractKey>
{
    using base = SubKey<decltype(to_radix_sort_key(std::declval<T>())), ExtractKey>;

    template<typename U>
    static decltype(auto) sub_key(U && value, void * data, ExtractKey & key)
    {
        return base::sub_key(to_radix_sort_key(value), data, key);
    }
};
template<typename T, typename ExtractKey, typename Enable = void>
struct FallbackSubKey : FallbackSubKey2<T, ExtractKey>
{
};
template<typename T, typename ExtractKey>
struct FallbackSubKey<T, ExtractKey, typename std::enable_if<!std::is_same_v<std::decay_t<T>, std::decay_t<decltype(std::declval<ExtractKey>()(std::declval<T>()))>>>::type>
    : SubKey<decltype(std::declval<ExtractKey>()(std::declval<T>())), ExtractKey>
{
    using base = SubKey<decltype(std::declval<ExtractKey>()(std::declval<T>())), ExtractKey>;

    template<typename U>
    static decltype(auto) sub_key(U && value, void * data, ExtractKey & key)
    {
        return base::sub_key(key(value), data, key);
    }
};
template<typename T, typename ExtractKey>
struct FallbackSubKey<T, ExtractKey, typename void_t<decltype(to_unsigned_or_bool(std::declval<T>()))>::type>
    : SubKey<decltype(to_unsigned_or_bool(std::declval<T>())), ExtractKey>
{
};
template<typename T, typename ExtractKey>
struct SubKey : FallbackSubKey<T, ExtractKey>
{
};
template<typename ExtractKey>
struct SubKey<bool, ExtractKey>
{
    template<typename T>
    static bool sub_key(T && value, void *, ExtractKey &)
    {
        return value;
    }

    typedef SubKey<void, ExtractKey> next;

    using sub_key_type = bool;
};
template<typename ExtractKey>
struct SubKey<void, ExtractKey>;
template<typename ExtractKey>
struct SubKey<unsigned char, ExtractKey> : SizedSubKey<sizeof(unsigned char), ExtractKey>
{
};
template<typename ExtractKey>
struct SubKey<unsigned short, ExtractKey> : SizedSubKey<sizeof(unsigned short), ExtractKey>
{
};
template<typename ExtractKey>
struct SubKey<unsigned int, ExtractKey> : SizedSubKey<sizeof(unsigned int), ExtractKey>
{
};
template<typename ExtractKey>
struct SubKey<unsigned long, ExtractKey> : SizedSubKey<sizeof(unsigned long), ExtractKey>
{
};
template<typename ExtractKey>
struct SubKey<unsigned long long, ExtractKey> : SizedSubKey<sizeof(unsigned long long), ExtractKey>
{
};
template<typename T, typename ExtractKey>
struct SubKey<T *, ExtractKey> : SizedSubKey<sizeof(T *), ExtractKey>
{
};
template<typename F, typename S, typename Current, typename ExtractKey>
struct PairSecondSubKey : Current
{
    static decltype(auto) sub_key(const std::pair<F, S> & value, void * sort_data, ExtractKey & key)
    {
        return Current::sub_key(value.second, sort_data, key);
    }

    using next = typename std::conditional<std::is_same<SubKey<void, ExtractKey>, typename Current::next>::value, SubKey<void, ExtractKey>, PairSecondSubKey<F, S, typename Current::next, ExtractKey>>::type;
};
template<typename F, typename S, typename Current, typename ExtractKey>
struct PairFirstSubKey : Current
{
    static decltype(auto) sub_key(const std::pair<F, S> & value, void * sort_data, ExtractKey & key)
    {
        return Current::sub_key(value.first, sort_data, key);
    }

    using next = typename std::conditional<std::is_same<SubKey<void, ExtractKey>, typename Current::next>::value, PairSecondSubKey<F, S, SubKey<S, ExtractKey>, ExtractKey>, PairFirstSubKey<F, S, typename Current::next, ExtractKey>>::type;
};
template<typename F, typename S, typename ExtractKey>
struct SubKey<std::pair<F, S>, ExtractKey> : PairFirstSubKey<F, S, SubKey<F, ExtractKey>, ExtractKey>
{
};

template<size_t Index, typename Current, typename... Args>
struct TupleSubKey;

template<size_t Index, typename Next, typename... Args>
struct NextTupleSubKey
{
    using type = TupleSubKey<Index, Next, Args...>;
};
template<size_t Index, typename ExtractKey, typename First, typename Second, typename... More>
struct NextTupleSubKey<Index, SubKey<void, ExtractKey>, First, Second, More...>
{
    using type = TupleSubKey<Index + 1, SubKey<Second, ExtractKey>, Second, More...>;
};
template<size_t Index, typename ExtractKey, typename First>
struct NextTupleSubKey<Index, SubKey<void, ExtractKey>, First>
{
    using type = SubKey<void, ExtractKey>;
};

template<size_t Index, typename Current, typename... Args>
struct TupleSubKey : Current
{
    template<typename Tuple, typename ExtractKey>
    static decltype(auto) sub_key(const Tuple & value, void * sort_data, ExtractKey & key)
    {
        return Current::sub_key(std::get<Index>(value), sort_data, key);
    }

    using next = typename NextTupleSubKey<Index, typename Current::next, Args...>::type;
};
template<typename First, typename... More, typename ExtractKey>
struct SubKey<std::tuple<First, More...>, ExtractKey> : TupleSubKey<0, SubKey<First, ExtractKey>, First, More...>
{
};
template<typename... Args, typename ExtractKey>
struct SubKey<std::variant<Args...>, ExtractKey>
{
    template<typename Variant>
    static decltype(auto) sub_key(const Variant & value, void *, ExtractKey &)
    {
        return value;
    }
    using next = SubKey<void, ExtractKey>;
    using sub_key_type = std::variant<Args...>;
};
template<typename Arg, typename ExtractKey>
struct SubKey<std::optional<Arg>, ExtractKey>
{
    static decltype(auto) sub_key(const std::optional<Arg> & value, void *, ExtractKey &)
    {
        return value;
    }
    using next = SubKey<void, ExtractKey>;
    using sub_key_type = std::optional<Arg>;
};

struct VariantBaseSortData
{
    void * next_sort_data;
};

template<typename It, typename ExtractKey>
struct VariantSortData : VariantBaseSortData
{
    void (*next_sort)(It, It, std::ptrdiff_t, ExtractKey &, void *);
};
template<typename PreviousSubKey, typename IndexType, typename ExtractKey>
struct VariantIndexSubKey : SubKey<IndexType, ExtractKey>
{
    template<typename Variant>
    static decltype(auto) sub_key(const Variant & value, void * sort_data, ExtractKey & key)
    {
        VariantBaseSortData * variant_sort_data = static_cast<VariantBaseSortData *>(sort_data);
        return static_cast<IndexType>(PreviousSubKey::sub_key(value, variant_sort_data->next_sort_data, key).index() + 1);
    }
};
template<typename PreviousSubKey, size_t Index, typename Variant, typename ExtractKey>
struct VariantItemSubKey : SubKey<std::variant_alternative_t<Index, Variant>, ExtractKey>
{
    template<typename T>
    static decltype(auto) sub_key(const T & value, void * sort_data, ExtractKey & key)
    {
        VariantBaseSortData * variant_sort_data = static_cast<VariantBaseSortData *>(sort_data);
        return SubKey<std::variant_alternative_t<Index, Variant>, ExtractKey>::sub_key(std::get<Index>(PreviousSubKey::sub_key(value, variant_sort_data->next_sort_data, key)), sort_data, key);
    }
};
template<typename PreviousSubKey, typename ExtractKey>
struct OptionalBoolSubKey : SubKey<bool, ExtractKey>
{
    template<typename Optional>
    static decltype(auto) sub_key(const Optional & value, void * sort_data, ExtractKey & key)
    {
        VariantBaseSortData * variant_sort_data = static_cast<VariantBaseSortData *>(sort_data);
        return PreviousSubKey::sub_key(value, variant_sort_data->next_sort_data, key).has_value();
    }
};
template<typename PreviousSubKey, typename Arg, typename ExtractKey>
struct OptionalItemSubKey : SubKey<Arg, ExtractKey>
{
    template<typename T>
    static decltype(auto) sub_key(const T & value, void * sort_data, ExtractKey & key)
    {
        VariantBaseSortData * variant_sort_data = static_cast<VariantBaseSortData *>(sort_data);
        return SubKey<Arg, ExtractKey>::sub_key(*PreviousSubKey::sub_key(value, variant_sort_data->next_sort_data, key), sort_data, key);
    }
};

struct BaseListSortData
{
    size_t current_index;
    size_t recursion_limit;
    void * next_sort_data;
};
template<typename It, typename ExtractKey>
struct ListSortData : BaseListSortData
{
    void (*next_sort)(It, It, std::ptrdiff_t, ExtractKey &, void *);
};
template<typename It, typename OutIt, typename ExtractKey>
struct ListPingPongSortData : BaseListSortData
{
    bool (*next_sort_ping)(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *);
    bool (*next_sort_pong)(OutIt, OutIt, It, std::ptrdiff_t, ExtractKey &, void *);
};

template<typename CurrentSubKey, typename T, typename ExtractKey>
struct ListElementSubKey : SubKey<typename std::decay<decltype(std::declval<T>()[0])>::type, ExtractKey>
{
    using base = SubKey<typename std::decay<decltype(std::declval<T>()[0])>::type, ExtractKey>;

    using next = ListElementSubKey;

    template<typename U>
    static decltype(auto) sub_key(U && value, void * sort_data, ExtractKey & key)
    {
        BaseListSortData * list_sort_data = static_cast<BaseListSortData *>(sort_data);
        const T & list = CurrentSubKey::sub_key(value, list_sort_data->next_sort_data, key);
        return base::sub_key(list[list_sort_data->current_index], list_sort_data->next_sort_data, key);
    }
};

template<typename T, typename ExtractKey>
struct ListSubKey
{
    using next = SubKey<void, ExtractKey>;

    using sub_key_type = T;

    static const T & sub_key(const T & value, void *, ExtractKey &)
    {
        return value;
    }
};

template<typename T, typename ExtractKey>
struct FallbackSubKey2<T, ExtractKey, typename void_t<decltype(std::declval<T>()[0])>::type> : ListSubKey<T, ExtractKey>
{
};

template<size_t NumBits>
struct FixedSizeBitSet
{
    static_assert(NumBits > 0 && NumBits % 64 == 0, "Need to be divisible by 64 to fit into uint64_t");

    inline void SetBit(uint8_t index)
    {
        all_bits[index / 64] |= uint64_t(1) << (index % 64);
    }
    inline uint64_t SetAndGetBit(uint8_t index)
    {
        uint64_t & bits = all_bits[index / 64];
        uint64_t mask = uint64_t(1) << (index % 64);
        uint64_t old_value = bits & mask;
        bits |= mask;
        return old_value;
    }
    template<typename T, typename Func>
    inline void ForEachSetBit(T (&array)[NumBits], Func && to_call) const
    {
        T * current = array;
        for (uint64_t bits : all_bits)
        {
            while (bits)
            {
                int bit = CountLeadingZeroes64(bits);
                bits &= 0xfffffffffffffffe << bit;
                to_call(current[bit]);
            }
            current += 64;
        }
    }
    template<typename Func>
    inline void ForEachSetBit(Func && to_call) const
    {
        int count = 0;
        for (uint64_t bits : all_bits)
        {
            while (bits)
            {
                int bit = CountLeadingZeroes64(bits);
                bits &= 0xfffffffffffffffe << bit;
                to_call(count + bit);
            }
            count += 64;
        }
    }

private:
    uint64_t all_bits[NumBits / 64] = {};

    inline static int CountLeadingZeroes64(uint64_t bits)
    {
        return __builtin_ctzll(bits);
    }
};

template<typename It, typename ExtractKey>
inline void StdSortFallback(It begin, It end, ExtractKey & extract_key)
{
    std::sort(begin, end, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); });
}

template<std::ptrdiff_t UpperLimit, typename It, typename Compare>
inline bool __attribute__((always_inline)) InsertionSortIfLessThanThreshold(It begin, It end, std::ptrdiff_t num_elements, Compare && compare)
{
    if (num_elements <= 1)
        return true;
    if (num_elements >= UpperLimit)
        return false;
    insertion_sort_length_2_or_more(begin, end, compare);
    return true;
}

template<typename SortSettings, typename It, typename Compare>
inline static bool __attribute__((always_inline)) DefaultSortIfLessThanThreshold(It begin, It end, std::ptrdiff_t num_elements, Compare && compare)
{
    using DereferencedType = decltype(*begin);
    return InsertionSortIfLessThanThreshold<SortSettings::template InsertionSortUpperLimit<DereferencedType>>(begin, end, num_elements, compare);
}

struct DefaultSortSettings
{
    static constexpr std::ptrdiff_t InsertionSortUpperLimitForSize(size_t type_size)
    {
        if (type_size < 16)
            return 32;
        else if (type_size < 24)
            return 24;
        else
            return 16;
    }

    template<typename T>
    static constexpr std::ptrdiff_t InsertionSortUpperLimit = InsertionSortUpperLimitForSize(sizeof(T));
    static constexpr std::ptrdiff_t AmericanFlagSortUpperLimit = 2048;
    static constexpr bool ThreeWaySwapBoost = false;
    static constexpr bool ThreeWaySwapStepanov = false;
    static constexpr bool UseFasterCompare = true;
    static constexpr bool SkaByteSortPrefetch = false;
    static constexpr bool SkipSortedItems = true;
    static constexpr std::ptrdiff_t PrefetchAmountLimit = 0;
    static constexpr size_t FirstLoopUnrollAmount = 4;
    static constexpr size_t SecondLoopUnrollAmount = 4;
    using count_type = size_t;
};

template<typename SortSettings, typename CurrentSubKey, typename SubKeyType = typename CurrentSubKey::sub_key_type>
struct InplaceSorter;

template<typename SortSettings, typename CurrentSubKey, size_t NumBytes, size_t Offset = 0>
struct UnsignedInplaceSorter
{
    static constexpr size_t ShiftAmount = (((NumBytes - 1) - Offset) * 8);
    template<typename T, typename ExtractKey>
    inline static uint8_t current_byte(T && elem, void * sort_data, ExtractKey & key)
    {
        return CurrentSubKey::sub_key(elem, sort_data, key) >> ShiftAmount;
    }
    template<typename It, typename ExtractKey>
    static void sort(It begin, It end, std::ptrdiff_t num_elements, ExtractKey & extract_key, void (*next_sort)(It, It, std::ptrdiff_t, ExtractKey &, void *), void * sort_data)
    {
        if (num_elements < SortSettings::AmericanFlagSortUpperLimit)
        {
            if (num_elements <= std::numeric_limits<uint8_t>::max())
                american_flag_sort<uint8_t>(begin, end, extract_key, next_sort, sort_data);
            else
                american_flag_sort<typename SortSettings::count_type>(begin, end, extract_key, next_sort, sort_data);
        }
        else
            ska_byte_sort<typename SortSettings::count_type>(begin, end, extract_key, next_sort, sort_data);
    }

    template<typename count_type, typename It, typename ExtractKey>
    static void __attribute__((noinline)) american_flag_sort(It begin, It end, ExtractKey & extract_key, void (*next_sort)(It, It, std::ptrdiff_t, ExtractKey &, void *), void * sort_data)
    {
        count_type counts_and_offsets[256] = {};
        FixedSizeBitSet<256> partitions_used;
        count_type partition_ends[256];
        for (It it = begin; it != end; ++it)
        {
            uint8_t index = current_byte(extract_key(*it), sort_data, extract_key);
            ++counts_and_offsets[index];
            partitions_used.SetBit(index);
        }
        count_type total = 0;
        uint8_t remaining_partitions[256];
        int num_partitions = 0;
        partitions_used.ForEachSetBit([&](int i)
        {
            count_type count = std::exchange(counts_and_offsets[i], total);
            total += count;
            partition_ends[i] = total;
            remaining_partitions[num_partitions] = i;
            ++num_partitions;
        });
        if (num_partitions > 1)
        {
            uint8_t * current_block_ptr = remaining_partitions;
            count_type * current_block = counts_and_offsets + *current_block_ptr;
            count_type * current_block_end = partition_ends + *current_block_ptr;
            uint8_t * last_block = remaining_partitions + num_partitions - 1;
            It it = begin;
            It block_end = begin + *current_block_end;
            It last_element = end - 1;
            for (;;)
            {
                count_type * block = counts_and_offsets + current_byte(extract_key(*it), sort_data, extract_key);
                if (block == current_block)
                {
                    ++it;
                    if (it == last_element)
                        break;
                    else if (it == block_end)
                    {
                        for (;;)
                        {
                            ++current_block_ptr;
                            if (current_block_ptr == last_block)
                                goto recurse;
                            current_block = counts_and_offsets + *current_block_ptr;
                            current_block_end = partition_ends + *current_block_ptr;
                            if (*current_block != *current_block_end)
                                break;
                        }

                        it = begin + *current_block;
                        block_end = begin + *current_block_end;
                    }
                }
                else
                {
                    auto target_it = begin + (*block)++;
                    count_type * target_of_target_block = counts_and_offsets + current_byte(extract_key(*target_it), sort_data, extract_key);
                    auto tmp = std::move(*it);
                    if (target_of_target_block == current_block)
                    {
                        *it = std::move(*target_it);
                    }
                    else
                    {
                        It target_of_target_it = begin + (*target_of_target_block)++;
                        *it = std::move(*target_of_target_it);
                        *target_of_target_it = std::move(*target_it);
                    }
                    *target_it = std::move(tmp);
                }
            }
        }
        recurse:
        if (Offset + 1 != NumBytes || next_sort)
        {
            count_type start_offset = 0;
            It partition_begin = begin;
            for (uint8_t * it = remaining_partitions, * remaining_end = remaining_partitions + num_partitions;;)
            {
                count_type end_offset = partition_ends[*it];
                It partition_end = begin + end_offset;
                std::ptrdiff_t num_elements = end_offset - start_offset;
                if (!DefaultSortIfLessThanThreshold<SortSettings>(partition_begin, partition_end, num_elements, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); }))
                {
                    UnsignedInplaceSorter<SortSettings, CurrentSubKey, NumBytes, Offset + 1>::sort(partition_begin, partition_end, num_elements, extract_key, next_sort, sort_data);
                }
                start_offset = end_offset;
                partition_begin = partition_end;
                ++it;
                if (it == remaining_end)
                    break;
            }
        }
    }

    template<typename count_type, typename It, typename ExtractKey>
    static void __attribute__((noinline)) ska_byte_sort(It begin, It end, ExtractKey & extract_key, void (*next_sort)(It, It, std::ptrdiff_t, ExtractKey &, void *), void * sort_data)
    {
        count_type counts_and_offsets[256] = {};
        FixedSizeBitSet<256> partitions_used;
        count_type partition_ends_plus_one[257];
        partition_ends_plus_one[0] = 0;
        count_type * const partition_ends = partition_ends_plus_one + 1;
        for (It it = begin; it != end; ++it)
        {
            uint8_t index = current_byte(extract_key(*it), sort_data, extract_key);
            ++counts_and_offsets[index];
            partitions_used.SetBit(index);
        }
        uint8_t remaining_partitions[256];
        count_type total = 0;
        int num_partitions = 0;
        partitions_used.ForEachSetBit([&](int i)
        {
            count_type * count_and_offset = counts_and_offsets + i;
            count_type * partition_end = partition_ends + i;
            count_type count = *count_and_offset;
            *count_and_offset = total;
            partition_end[-1] = total;
            total += count;
            *partition_end = total;
            remaining_partitions[num_partitions] = i;
            ++num_partitions;
        });
        for (uint8_t * last_remaining = remaining_partitions + num_partitions, * end_partition = remaining_partitions + 1; last_remaining > end_partition;)
        {
            last_remaining = custom_std_partition(remaining_partitions, last_remaining, [&](uint8_t partition)
            {
                count_type & begin_offset = counts_and_offsets[partition];
                count_type end_offset = partition_ends[partition];
                if (begin_offset == end_offset)
                    return false;

                if constexpr (SortSettings::ThreeWaySwapBoost)
                {
                    for (It it = begin + begin_offset, end = begin + end_offset; it != end; ++it)
                    {
                        uint8_t target_partition = current_byte(extract_key(*it), sort_data, extract_key);
                        count_type offset = counts_and_offsets[target_partition]++;
                        auto target_it = begin + offset;
                        uint8_t target_of_target_partition = current_byte(extract_key(*target_it), sort_data, extract_key);
                        auto tmp = std::move(*it);
                        if (target_of_target_partition == partition)
                        {
                            *it = std::move(*target_it);
                        }
                        else
                        {
                            It target_of_target_it = begin + counts_and_offsets[target_of_target_partition]++;
                            *it = std::move(*target_of_target_it);
                            *target_of_target_it = std::move(*target_it);
                        }
                        *target_it = std::move(tmp);
                    }
                }
                else
                {
                    unroll_loop_nonempty<4>(begin + begin_offset, end_offset - begin_offset, [begin, sort_data, counts_and_offsets = &*counts_and_offsets, &extract_key](It it)
                    {
                        uint8_t target_partition = current_byte(extract_key(*it), sort_data, extract_key);
                        count_type offset = counts_and_offsets[target_partition]++;
                        std::iter_swap(it, begin + offset);
                    });
                }
                return begin_offset != end_offset;
            });
        }
        if (Offset + 1 != NumBytes || next_sort)
        {
            for (uint8_t * it = remaining_partitions + num_partitions;;)
            {
                --it;
                uint8_t partition = *it;
                count_type start_offset = partition_ends[partition - 1];
                count_type end_offset = partition_ends[partition];
                It partition_begin = begin + start_offset;
                It partition_end = begin + end_offset;
                std::ptrdiff_t partition_num_elements = end_offset - start_offset;
                if (!DefaultSortIfLessThanThreshold<SortSettings>(partition_begin, partition_end, partition_num_elements, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); }))
                {
                    UnsignedInplaceSorter<SortSettings, CurrentSubKey, NumBytes, Offset + 1>::sort(partition_begin, partition_end, partition_num_elements, extract_key, next_sort, sort_data);
                }
                if (it == remaining_partitions)
                    break;
            }
        }
    }


    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort_ping_pong(It begin, It end, OutIt buffer_begin, std::ptrdiff_t, ExtractKey & extract_key, bool (*next_sort_ping)(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *), bool (*next_sort_pong)(OutIt, OutIt, It, std::ptrdiff_t, ExtractKey &, void *), void * sort_data)
    {
        return counting_sort_ping_pong<typename SortSettings::count_type>(begin, end, buffer_begin, extract_key, next_sort_ping, next_sort_pong, sort_data);
    }

    template<typename count_type, typename It, typename OutIt, typename ExtractKey>
    static bool counting_sort_ping_pong(It begin, It end, OutIt buffer_begin, ExtractKey & extract_key, bool (*next_sort_ping)(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *), bool (*next_sort_pong)(OutIt, OutIt, It, std::ptrdiff_t, ExtractKey &, void *), void * sort_data)
    {
        count_type counts[256] = {};
        FixedSizeBitSet<256> counts_used;
        for (It it = begin; it != end; ++it)
        {
            uint8_t index = current_byte(extract_key(*it), sort_data, extract_key);
            ++counts[index];
            counts_used.SetBit(index);
        }
        count_type total = 0;
        counts_used.ForEachSetBit(counts, [&](count_type & count)
        {
            count_type old_count = count;
            count = total;
            total += old_count;
        });
        for (It it = begin; it != end; ++it)
        {
            std::uint8_t key = current_byte(extract_key(*it), sort_data, extract_key);
            count_type index = counts[key]++;
            buffer_begin[index] = std::move(*it);
        }
        if (Offset + 1 != NumBytes || next_sort_ping)
        {
            count_type partition_begin = 0;
            counts_used.ForEachSetBit(counts, [&](count_type partition_end)
            {
                std::ptrdiff_t num_elements = partition_end - partition_begin;
                OutIt out_partition_begin_it = buffer_begin + partition_begin;
                OutIt out_partition_end_it = buffer_begin + partition_end;
                if (!DefaultSortIfLessThanThreshold<SortSettings>(out_partition_begin_it, out_partition_end_it, num_elements, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); }))
                {
                    It in_partition_begin_it = begin + partition_begin;
                    if (UnsignedInplaceSorter<SortSettings, CurrentSubKey, NumBytes, Offset + 1>::sort_ping_pong(out_partition_begin_it, out_partition_end_it, in_partition_begin_it, num_elements, extract_key, next_sort_pong, next_sort_ping, sort_data))
                    {
                        It in_partition_end_it = begin + partition_end;
                        std::move(in_partition_begin_it, in_partition_end_it, out_partition_begin_it);
                    }
                }
                partition_begin = partition_end;
            });
        }
        return true;
    }
};

template<typename SortSettings, typename CurrentSubKey, size_t NumBytes>
struct UnsignedInplaceSorter<SortSettings, CurrentSubKey, NumBytes, NumBytes>
{
    template<typename It, typename ExtractKey>
    inline static void sort(It begin, It end, std::ptrdiff_t num_elements, ExtractKey & extract_key, void (*next_sort)(It, It, std::ptrdiff_t, ExtractKey &, void *), void * next_sort_data)
    {
        next_sort(begin, end, num_elements, extract_key, next_sort_data);
    }
    template<typename It, typename OutIt, typename ExtractKey>
    inline static bool sort_ping_pong(It begin, It end, OutIt buffer_begin, std::ptrdiff_t num_elements, ExtractKey & extract_key, bool (*next_sort_ping)(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *), bool (*)(OutIt, OutIt, It, std::ptrdiff_t, ExtractKey &, void *), void * next_sort_data)
    {
        return next_sort_ping(begin, end, buffer_begin, num_elements, extract_key, next_sort_data);
    }
};

template<typename It, typename ExtractKey, typename ElementKey>
size_t CommonPrefix(It begin, It end, size_t start_index, ExtractKey && extract_key, ElementKey && element_key)
{
    const auto & largest_match_list = extract_key(*begin);
    size_t largest_match = largest_match_list.size();
    if (largest_match == start_index)
        return start_index;
    for (++begin; begin != end; ++begin)
    {
        const auto & current_list = extract_key(*begin);
        size_t current_size = current_list.size();
        if (current_size < largest_match)
        {
            largest_match = current_size;
            if (largest_match == start_index)
                return start_index;
        }
        if (element_key(largest_match_list[start_index]) != element_key(current_list[start_index]))
            return start_index;
        for (size_t i = start_index + 1; i < largest_match; ++i)
        {
            if (element_key(largest_match_list[i]) != element_key(current_list[i]))
            {
                largest_match = i;
                break;
            }
        }
    }
    return largest_match;
}

template<typename It, typename OutIt>
bool unify_ping_pong_results(bool first_half_result, bool second_half_result, It begin, It middle, It end, OutIt out_begin, OutIt out_middle, OutIt out_end, std::ptrdiff_t first_half_size, std::ptrdiff_t second_half_size)
{
    if (first_half_result == second_half_result)
        return first_half_result;
    if (first_half_size > second_half_size)
    {
        if (second_half_result)
            std::move(out_middle, out_end, middle);
        else
            std::move(middle, end, out_middle);
        return first_half_result;
    }
    else if (first_half_result)
        std::move(out_begin, out_middle, begin);
    else
        std::move(begin, middle, out_begin);
    return second_half_result;
}

template<typename SortSettings, typename CurrentSubKey, typename ListType>
struct ListInplaceSorter
{
    template<typename It, typename ExtractKey>
    static void sort(It begin, It end, ExtractKey & extract_key, ListSortData<It, ExtractKey> * sort_data)
    {
        using ElementSubKey = ListElementSubKey<CurrentSubKey, ListType, ExtractKey>;
        size_t current_index = sort_data->current_index;
        void * next_sort_data = sort_data->next_sort_data;
        auto current_key = [&, next_sort_data](auto && elem) -> decltype(auto)
        {
            return CurrentSubKey::sub_key(extract_key(elem), next_sort_data, extract_key);
        };
        auto element_key = [&, sort_data](auto && elem) -> decltype(auto)
        {
            return ElementSubKey::base::sub_key(extract_key(elem), sort_data, extract_key);
        };
        sort_data->current_index = current_index = CommonPrefix(begin, end, current_index, current_key, element_key);
        It end_of_shorter_ones = std::partition(begin, end, [&](auto && elem)
        {
            return current_key(elem).size() <= current_index;
        });
        std::ptrdiff_t num_shorter_ones = end_of_shorter_ones - begin;
        if (sort_data->next_sort && !DefaultSortIfLessThanThreshold<SortSettings>(begin, end_of_shorter_ones, num_shorter_ones, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); }))
        {
            sort_data->next_sort(begin, end_of_shorter_ones, num_shorter_ones, extract_key, next_sort_data);
        }
        std::ptrdiff_t num_elements = end - end_of_shorter_ones;
        if (!DefaultSortIfLessThanThreshold<SortSettings>(end_of_shorter_ones, end, num_elements, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); }))
        {
            void (*sort_next_element)(It, It, std::ptrdiff_t, ExtractKey &, void *) = static_cast<void (*)(It, It, std::ptrdiff_t, ExtractKey &, void *)>(&sort_from_recursion);
            InplaceSorter<SortSettings, ElementSubKey>::sort(end_of_shorter_ones, end, num_elements, extract_key, sort_next_element, sort_data);
        }
    }
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort_ping_pong(It begin, It end, OutIt buffer_begin, std::ptrdiff_t num_elements, ExtractKey & extract_key, ListPingPongSortData<It, OutIt, ExtractKey> * sort_data)
    {
        using ElementSubKey = ListElementSubKey<CurrentSubKey, ListType, ExtractKey>;
        size_t current_index = sort_data->current_index;
        void * next_sort_data = sort_data->next_sort_data;
        auto current_key = [&, next_sort_data](auto && elem) -> decltype(auto)
        {
            return CurrentSubKey::sub_key(extract_key(elem), next_sort_data, extract_key);
        };
        auto element_key = [&, sort_data](auto && elem) -> decltype(auto)
        {
            return ElementSubKey::base::sub_key(elem, sort_data, extract_key);
        };
        sort_data->current_index = current_index = CommonPrefix(begin, end, current_index, current_key, element_key);
        OutIt buffer_end = buffer_begin + num_elements;
        OutIt end_of_shorter_ones = partition_copy(begin, end, buffer_begin, buffer_end, [&](auto && elem)
        {
            return current_key(elem).size() <= current_index;
        });
        std::ptrdiff_t num_shorter_ones = end_of_shorter_ones - buffer_begin;
        bool result_for_shorter_ones = false;
        if (sort_data->next_sort_pong && !DefaultSortIfLessThanThreshold<SortSettings>(buffer_begin, end_of_shorter_ones, num_shorter_ones, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); }))
        {
            result_for_shorter_ones = sort_data->next_sort_pong(buffer_begin, end_of_shorter_ones, begin, num_shorter_ones, extract_key, next_sort_data);
        }
        std::ptrdiff_t num_longer_ones = buffer_end - end_of_shorter_ones;
        bool result_for_longer_ones = false;
        It middle = begin + num_shorter_ones;
        if (!DefaultSortIfLessThanThreshold<SortSettings>(end_of_shorter_ones, buffer_end, num_longer_ones, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); }))
        {
            bool (*sort_next_element_ping)(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *) = static_cast<bool (*)(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *)>(&sort_from_recursion_ping_pong);
            bool (*sort_next_element_pong)(OutIt, OutIt, It, std::ptrdiff_t, ExtractKey &, void *) = static_cast<bool (*)(OutIt, OutIt, It, std::ptrdiff_t, ExtractKey &, void *)>(&sort_from_recursion_ping_pong);
            result_for_longer_ones = InplaceSorter<SortSettings, ElementSubKey>::sort_ping_pong(end_of_shorter_ones, buffer_end, middle, num_longer_ones, extract_key, sort_next_element_pong, sort_next_element_ping, sort_data);
        }
        return !unify_ping_pong_results(result_for_shorter_ones, result_for_longer_ones, buffer_begin, end_of_shorter_ones, buffer_end, begin, middle, end, num_shorter_ones, num_longer_ones);
    }

    template<typename It, typename ExtractKey>
    static void sort_from_recursion(It begin, It end, std::ptrdiff_t, ExtractKey & extract_key, void * next_sort_data)
    {
        ListSortData<It, ExtractKey> offset = *static_cast<ListSortData<It, ExtractKey> *>(next_sort_data);
        ++offset.current_index;
        --offset.recursion_limit;
        if (offset.recursion_limit == 0)
        {
            StdSortFallback(begin, end, extract_key);
        }
        else
        {
            sort(begin, end, extract_key, &offset);
        }
    }

    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort_from_recursion_ping_pong(It begin, It end, OutIt buffer_begin, std::ptrdiff_t num_elements, ExtractKey & extract_key, void * next_sort_data)
    {
        ListPingPongSortData<It, OutIt, ExtractKey> offset = *static_cast<ListPingPongSortData<It, OutIt, ExtractKey> *>(next_sort_data);
        ++offset.current_index;
        --offset.recursion_limit;
        if (offset.recursion_limit == 0)
        {
            StdSortFallback(begin, end, extract_key);
            return false;
        }
        else
        {
            return sort_ping_pong(begin, end, buffer_begin, num_elements, extract_key, &offset);
        }
    }


    template<typename It, typename ExtractKey>
    static void sort(It begin, It end, std::ptrdiff_t, ExtractKey & extract_key, void (*next_sort)(It, It, std::ptrdiff_t, ExtractKey &, void *), void * next_sort_data)
    {
        ListSortData<It, ExtractKey> offset;
        offset.current_index = 0;
        offset.recursion_limit = 16;
        offset.next_sort = next_sort;
        offset.next_sort_data = next_sort_data;
        sort(begin, end, extract_key, &offset);
    }
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort_ping_pong(It begin, It end, OutIt buffer_begin, std::ptrdiff_t num_elements, ExtractKey & extract_key, bool (*next_sort_ping)(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *), bool (*next_sort_pong)(OutIt, OutIt, It, std::ptrdiff_t, ExtractKey &, void *), void * next_sort_data)
    {
        ListPingPongSortData<It, OutIt, ExtractKey> offset;
        offset.current_index = 0;
        offset.recursion_limit = 16;
        offset.next_sort_ping = next_sort_ping;
        offset.next_sort_pong = next_sort_pong;
        offset.next_sort_data = next_sort_data;
        return sort_ping_pong(begin, end, buffer_begin, num_elements, extract_key, &offset);
    }
};

template<typename SortSettings, typename CurrentSubKey>
struct InplaceSorter<SortSettings, CurrentSubKey, bool>
{
    template<typename It, typename ExtractKey>
    static void sort(It begin, It end, std::ptrdiff_t, ExtractKey & extract_key, void (*next_sort)(It, It, std::ptrdiff_t, ExtractKey &, void *), void * sort_data)
    {
        It middle = std::partition(begin, end, [&](auto && a){ return !CurrentSubKey::sub_key(extract_key(a), sort_data, extract_key); });
        if (next_sort)
        {
            std::ptrdiff_t num_false = middle - begin;
            if (!DefaultSortIfLessThanThreshold<SortSettings>(begin, middle, num_false, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); }))
                next_sort(begin, middle, middle - begin, extract_key, sort_data);
            std::ptrdiff_t num_true = end - middle;
            if (!DefaultSortIfLessThanThreshold<SortSettings>(middle, end, num_true, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); }))
                next_sort(middle, end, end - middle, extract_key, sort_data);
        }
    }
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort_ping_pong(It begin, It end, OutIt buffer_begin, std::ptrdiff_t num_elements, ExtractKey & extract_key, bool (*)(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *), bool (*next_sort_pong)(OutIt, OutIt, It, std::ptrdiff_t, ExtractKey &, void *), void * sort_data)
    {
        OutIt out_end = buffer_begin + num_elements;
        OutIt middle = partition_copy(begin, end, buffer_begin, out_end, [&](auto && entry)
        {
            return !CurrentSubKey::sub_key(extract_key(entry), sort_data, extract_key);
        });
        if (next_sort_pong)
        {
            std::ptrdiff_t first_half_size = middle - buffer_begin;
            std::ptrdiff_t second_half_size = out_end - middle;
            It second_half_out = begin + first_half_size;
            bool first_half = next_sort_pong(buffer_begin, middle, begin, first_half_size, extract_key, sort_data);
            bool second_half = next_sort_pong(middle, out_end, second_half_out, second_half_size, extract_key, sort_data);
            return unify_ping_pong_results(first_half, second_half, buffer_begin, middle, out_end, begin, second_half_out, end, first_half_size, second_half_size);
        }
        else
            return true;
    }
};

template<typename SortSettings, typename CurrentSubKey>
struct InplaceSorter<SortSettings, CurrentSubKey, uint8_t> : UnsignedInplaceSorter<SortSettings, CurrentSubKey, 1>
{
};
template<typename SortSettings, typename CurrentSubKey>
struct InplaceSorter<SortSettings, CurrentSubKey, uint16_t> : UnsignedInplaceSorter<SortSettings, CurrentSubKey, 2>
{
};
template<typename SortSettings, typename CurrentSubKey>
struct InplaceSorter<SortSettings, CurrentSubKey, uint32_t> : UnsignedInplaceSorter<SortSettings, CurrentSubKey, 4>
{
};
template<typename SortSettings, typename CurrentSubKey>
struct InplaceSorter<SortSettings, CurrentSubKey, uint64_t> : UnsignedInplaceSorter<SortSettings, CurrentSubKey, 8>
{
};

template<typename SortSettings, typename CurrentSubKey, typename... Args>
struct InplaceSorter<SortSettings, CurrentSubKey, std::variant<Args...>>
{
    template<typename It, typename ExtractKey>
    static void sort_after_subindex(It begin, It end, std::ptrdiff_t num_elements, ExtractKey & extract_key, void * sort_data)
    {
        VariantSortData<It, ExtractKey> * variant_sort_data = static_cast<VariantSortData<It, ExtractKey> *>(sort_data);
        variant_sort_data->next_sort(begin, end, num_elements, extract_key, variant_sort_data->next_sort_data);
    }
    template<size_t Index, typename It, typename ExtractKey>
    static void sort_subindex(It begin, It end, std::ptrdiff_t num_elements, ExtractKey & extract_key, void * sort_data)
    {
        VariantSortData<It, ExtractKey> * variant_sort_data = static_cast<VariantSortData<It, ExtractKey> *>(sort_data);
        InplaceSorter<SortSettings, VariantItemSubKey<CurrentSubKey, Index, std::variant<Args...>, ExtractKey>>::sort(begin, end, num_elements, extract_key, variant_sort_data->next_sort ? &sort_after_subindex<It, ExtractKey> : nullptr, sort_data);
    }

    template<typename IndexSequence>
    struct IndexedSorter;
    template<size_t... Indices>
    struct IndexedSorter<std::integer_sequence<size_t, Indices...>>
    {
        template<typename It, typename ExtractKey>
        static void sort_after_index(It begin, It end, std::ptrdiff_t num_elements, ExtractKey & extract_key, void * sort_data)
        {
            VariantSortData<It, ExtractKey> * variant_sort_data = static_cast<VariantSortData<It, ExtractKey> *>(sort_data);
            auto && variant = CurrentSubKey::sub_key(extract_key(*begin), variant_sort_data->next_sort_data, extract_key);
            if (variant.valueless_by_exception())
            {
                if (variant_sort_data->next_sort)
                {
                    variant_sort_data->next_sort(begin, end, num_elements, extract_key, variant_sort_data->next_sort_data);
                }
            }
            else
            {
                size_t index = variant.index();
                static constexpr void (*subindex_sorts[sizeof...(Args)])(It, It, std::ptrdiff_t, ExtractKey &, void *) =
                {
                    &sort_subindex<Indices, It, ExtractKey>...,
                };
                subindex_sorts[index](begin, end, num_elements, extract_key, sort_data);
            }
        }
    };

    template<typename It, typename ExtractKey>
    static void sort(It begin, It end, std::ptrdiff_t num_elements, ExtractKey & extract_key, void (*next_sort)(It, It, std::ptrdiff_t, ExtractKey &, void *), void * sort_data)
    {
        using IndexType = typename std::conditional<sizeof...(Args) < static_cast<size_t>(std::numeric_limits<uint8_t>::max()), uint8_t,
                        typename std::conditional<sizeof...(Args) < static_cast<size_t>(std::numeric_limits<uint16_t>::max()), uint16_t, uint64_t>::type>::type;
        VariantSortData<It, ExtractKey> next_sort_data;
        next_sort_data.next_sort_data = sort_data;
        next_sort_data.next_sort = next_sort;
        InplaceSorter<SortSettings, VariantIndexSubKey<CurrentSubKey, IndexType, ExtractKey>>::sort(begin, end, num_elements, extract_key, &IndexedSorter<std::make_index_sequence<sizeof...(Args)>>::template sort_after_index<It, ExtractKey>, &next_sort_data);
    }
};

template<typename SortSettings, typename CurrentSubKey, typename Arg>
struct InplaceSorter<SortSettings, CurrentSubKey, std::optional<Arg>>
{
    template<typename It, typename ExtractKey>
    static void sort_after_item(It begin, It end, std::ptrdiff_t num_elements, ExtractKey & extract_key, void * sort_data)
    {
        VariantSortData<It, ExtractKey> * variant_sort_data = static_cast<VariantSortData<It, ExtractKey> *>(sort_data);
        variant_sort_data->next_sort(begin, end, num_elements, extract_key, variant_sort_data->next_sort_data);
    }

    template<typename It, typename ExtractKey>
    static void sort_after_bool(It begin, It end, std::ptrdiff_t num_elements, ExtractKey & extract_key, void * sort_data)
    {
        VariantSortData<It, ExtractKey> * variant_sort_data = static_cast<VariantSortData<It, ExtractKey> *>(sort_data);
        auto && optional = CurrentSubKey::sub_key(extract_key(*begin), variant_sort_data->next_sort_data, extract_key);
        if (optional.has_value())
        {
            VariantSortData<It, ExtractKey> * variant_sort_data = static_cast<VariantSortData<It, ExtractKey> *>(sort_data);
            InplaceSorter<SortSettings, OptionalItemSubKey<CurrentSubKey, Arg, ExtractKey>>::sort(begin, end, num_elements, extract_key, variant_sort_data->next_sort ? &sort_after_item<It, ExtractKey> : nullptr, sort_data);
        }
        else if (variant_sort_data->next_sort)
        {
            variant_sort_data->next_sort(begin, end, num_elements, extract_key, variant_sort_data->next_sort_data);
        }
    }

    template<typename It, typename ExtractKey>
    static void sort(It begin, It end, std::ptrdiff_t num_elements, ExtractKey & extract_key, void (*next_sort)(It, It, std::ptrdiff_t, ExtractKey &, void *), void * sort_data)
    {
        VariantSortData<It, ExtractKey> next_sort_data;
        next_sort_data.next_sort_data = sort_data;
        next_sort_data.next_sort = next_sort;
        InplaceSorter<SortSettings, OptionalBoolSubKey<CurrentSubKey, ExtractKey>>::sort(begin, end, num_elements, extract_key, &sort_after_bool<It, ExtractKey>, &next_sort_data);
    }
};

template<typename SortSettings, typename CurrentSubKey, typename SubKeyType, typename Enable = void>
struct FallbackInplaceSorter;

template<typename SortSettings, typename CurrentSubKey, typename SubKeyType>
struct InplaceSorter : FallbackInplaceSorter<SortSettings, CurrentSubKey, SubKeyType>
{
};

template<typename SortSettings, typename CurrentSubKey, typename SubKeyType>
struct FallbackInplaceSorter<SortSettings, CurrentSubKey, SubKeyType, decltype(static_cast<void>(std::declval<SubKeyType>()[0]))>
    : ListInplaceSorter<SortSettings, CurrentSubKey, SubKeyType>
{
};

template<typename SortSettings, typename CurrentSubKey>
struct SortStarter;
template<typename SortSettings, typename ExtractKey>
struct SortStarter<SortSettings, SubKey<void, ExtractKey>>
{
    template<typename It>
    static void sort(It, It, std::ptrdiff_t, ExtractKey &, void *)
    {
    }
    template<typename It, typename OutIt>
    static bool sort_ping_pong(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *)
    {
        return false;
    }
};

template<typename SortSettings, typename CurrentSubKey>
struct SortStarter
{
    template<typename It, typename ExtractKey>
    static void sort(It begin, It end, std::ptrdiff_t num_elements, ExtractKey & extract_key, void * next_sort_data = nullptr)
    {
        if (DefaultSortIfLessThanThreshold<SortSettings>(begin, end, num_elements, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); }))
            return;

        void (*next_sort)(It, It, std::ptrdiff_t, ExtractKey &, void *) = static_cast<void (*)(It, It, std::ptrdiff_t, ExtractKey &, void *)>(&SortStarter<SortSettings, typename CurrentSubKey::next>::sort);
        if (next_sort == static_cast<void (*)(It, It, std::ptrdiff_t, ExtractKey &, void *)>(&SortStarter<SortSettings, SubKey<void, ExtractKey>>::sort))
            next_sort = nullptr;
        InplaceSorter<SortSettings, CurrentSubKey>::sort(begin, end, num_elements, extract_key, next_sort, next_sort_data);
    }
    template<typename It, typename OutIt, typename ExtractKey>
    static bool sort_ping_pong(It begin, It end, OutIt buffer_begin, std::ptrdiff_t num_elements, ExtractKey & extract_key, void * next_sort_data = nullptr)
    {
        if (DefaultSortIfLessThanThreshold<SortSettings>(begin, end, num_elements, [&](auto && l, auto && r){ return extract_key(l) < extract_key(r); }))
            return false;

        bool (*next_sort_ping)(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *) = static_cast<bool (*)(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *)>(&SortStarter<SortSettings, typename CurrentSubKey::next>::sort_ping_pong);
        bool (*next_sort_pong)(OutIt, OutIt, It, std::ptrdiff_t, ExtractKey &, void *) = static_cast<bool (*)(OutIt, OutIt, It, std::ptrdiff_t, ExtractKey &, void *)>(&SortStarter<SortSettings, typename CurrentSubKey::next>::sort_ping_pong);
        if (next_sort_ping == static_cast<bool (*)(It, It, OutIt, std::ptrdiff_t, ExtractKey &, void *)>(&SortStarter<SortSettings, SubKey<void, ExtractKey>>::sort_ping_pong))
        {
            next_sort_ping = nullptr;
            next_sort_pong = nullptr;
        }
        return InplaceSorter<SortSettings, CurrentSubKey>::sort_ping_pong(begin, end, buffer_begin, num_elements, extract_key, next_sort_ping, next_sort_pong, next_sort_data);
    }
};

template<typename SortSettings, typename It, typename ExtractKey>
void inplace_radix_sort(It begin, It end, ExtractKey & extract_key)
{
    using SubKey = SubKey<decltype(extract_key(*begin)), ExtractKey>;
    SortStarter<SortSettings, SubKey>::sort(begin, end, end - begin, extract_key);
}

template<typename SortSettings, typename It, typename OutIt, typename ExtractKey>
bool ping_pong_ska_sort(It begin, It end, OutIt buffer_begin, ExtractKey & extract_key)
{
    using SubKey = SubKey<decltype(extract_key(*begin)), ExtractKey>;
    return SortStarter<SortSettings, SubKey>::sort_ping_pong(begin, end, buffer_begin, end - begin, extract_key);
}

struct IdentityFunctor
{
    template<typename T>
    decltype(auto) operator()(T && i) const
    {
        return std::forward<T>(i);
    }
};
template<typename SortSettings, typename It, typename... ExtractKey>
void ska_sort_with_settings(It begin, It end, ExtractKey &&... extract_key)
{
    detail::IdentityFunctor identity;
    OverloadedCallWrapper<ExtractKey..., detail::IdentityFunctor> wrapper{{std::forward<ExtractKey>(extract_key)..., identity}};
    detail::inplace_radix_sort<SortSettings>(begin, end, wrapper);
}

}

template<typename It, typename... ExtractKey>
void ska_sort(It begin, It end, ExtractKey &&... extract_key)
{
    detail::ska_sort_with_settings<detail::DefaultSortSettings>(begin, end, std::forward<ExtractKey>(extract_key)...);
}

template<typename It, typename OutIt, typename ExtractKey>
bool ska_sort_ping_pong(It begin, It end, OutIt buffer_begin, ExtractKey && key)
{
    std::ptrdiff_t num_elements = end - begin;
    if (num_elements < 128 || detail::radix_sort_pass_count<typename std::result_of<ExtractKey(decltype(*begin))>::type> >= 8)
    {
        return detail::ping_pong_ska_sort<detail::DefaultSortSettings>(begin, end, buffer_begin, key);
        //ska_sort(begin, end, key);
        //return false;
    }
    else
        return detail::RadixSorter<typename std::result_of<ExtractKey(decltype(*begin))>::type>::sort(begin, end, buffer_begin, key);
}
template<typename It>
bool ska_sort_ping_pong(It begin, It end, It buffer_begin)
{
    return ska_sort_ping_pong(begin, end, buffer_begin, detail::IdentityFunctor());
}
