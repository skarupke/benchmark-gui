#pragma once

#ifndef BOOST_SKA_SORT_FIXED_SIZE_BITSET_HPP
#define BOOST_SKA_SORT_FIXED_SIZE_BITSET_HPP

#include <cstdint>
#include <cstring> // for memcpy

#include <boost/config.hpp>

#if (defined(BOOST_MSVC) || (defined(__clang__) && defined(__c2__)) || (defined(BOOST_INTEL) && defined(_MSC_VER))) && defined(_M_X64)
#include <intrin.h>
#elif defined(BOOST_INTEL)
#include <immintrin.h>
#endif

namespace boost_ska_sort::sort::detail_ska_sort
{

#if (defined(BOOST_MSVC) || (defined(__clang__) && defined(__c2__)) || (defined(BOOST_INTEL) && defined(_MSC_VER))) && defined(_M_X64)

#pragma intrinsic(_BitScanForward64)

BOOST_FORCEINLINE unsigned find_lsb(std::uint64_t mask)
{
   unsigned long result;
   _BitScanForward64(&result, mask);
   return result;
}

#elif defined(BOOST_GCC) || defined(__clang__) || (defined(BOOST_INTEL) && defined(__GNUC__))

BOOST_FORCEINLINE unsigned find_lsb(std::uint64_t mask)
{
   return __builtin_ctzll(mask);
}
#elif defined(BOOST_INTEL)
BOOST_FORCEINLINE unsigned find_lsb(std::uint64_t mask)
{
    unsigned __int32 result;
    _BitScanForward64(&result, mask);
    return result;
}
#else
inline unsigned find_lsb(std::uint64_t v)
{
    float f = static_cast<float>(v & -v); // cast the least significant bit in v to a float
    uint32_t as_uint32;
    memcpy(&as_uint32, &f, sizeof(as_uint32));
    // the exponent stores the power of two
    return (as_uint32 >> 23) - 0x7f;
}
#endif



template<size_t NumBits>
struct fixed_size_bit_set
{
    static_assert(NumBits > 0 && NumBits % 64 == 0, "Need to be divisible by 64 to fit into uint64_t");

    inline void set_bit(int index)
    {
        all_bits[index / 64] |= std::uint64_t(1) << (index % 64);
    }
    template<typename Func>
    inline void for_each_set_bit(Func && to_call) const
    {
        int count = 0;
        for (std::uint64_t bits : all_bits)
        {
            while (bits)
            {
                int bit = count_leading_zeroes_64(bits);
                bits &= 0xffff'ffff'ffff'fffe << bit;
                to_call(count + bit);
            }
            count += 64;
        }
    }

private:
    std::uint64_t all_bits[NumBits / 64] = {};

    BOOST_FORCEINLINE static int count_leading_zeroes_64(std::uint64_t bits)
    {
        return find_lsb(bits);
    }
};

}

#endif
