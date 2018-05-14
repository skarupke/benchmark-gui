#include "math/fibonacci.h"

struct Mat2x2
{
    std::size_t tl;
    std::size_t tr;
    std::size_t bl;
    std::size_t br;

    Mat2x2 operator*(const Mat2x2 & other) const
    {
        return
        {
            tl * other.tl + tr * other.bl, tl * other.tr + tr * other.br,
            bl * other.tl + br * other.bl, bl * other.tr + br * other.br
        };
    }
    Mat2x2 & operator*=(const Mat2x2 & other)
    {
        *this = *this * other;
        return *this;
    }
};

template<typename T>
T custom_pow(T base, int exp, T one = 1)
{
    T result = one;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (exp == 0)
            return result;
        base *= base;
    }
}

std::size_t fibonacci(int i)
{
    Mat2x2 fib_matrix =
    {
        1, 1,
        1, 0
    };
    return custom_pow(fib_matrix, i, { 1, 0, 0, 1 }).tl;
}


#ifndef DISABLE_GTEST
#include <test/include_test.hpp>

TEST(custom_pow, simple)
{
    ASSERT_EQ(2, custom_pow(2, 1));
    ASSERT_EQ(4, custom_pow(2, 2));
    ASSERT_EQ(8, custom_pow(2, 3));
    ASSERT_EQ(16, custom_pow(2, 4));
    ASSERT_EQ(32, custom_pow(2, 5));
    ASSERT_EQ(64, custom_pow(2, 6));
    ASSERT_EQ(128, custom_pow(2, 7));
    ASSERT_EQ(256, custom_pow(2, 8));
}

TEST(fibonacci, simple)
{
    ASSERT_EQ(1u, fibonacci(0));
    ASSERT_EQ(1u, fibonacci(1));
    ASSERT_EQ(2u, fibonacci(2));
    ASSERT_EQ(3u, fibonacci(3));
    ASSERT_EQ(5u, fibonacci(4));
    ASSERT_EQ(8u, fibonacci(5));
    ASSERT_EQ(13u, fibonacci(6));
    ASSERT_EQ(21u, fibonacci(7));
    ASSERT_EQ(34u, fibonacci(8));
    ASSERT_EQ(55u, fibonacci(9));
    ASSERT_EQ(89u, fibonacci(10));
    ASSERT_EQ(144u, fibonacci(11));
    ASSERT_EQ(233u, fibonacci(12));
    ASSERT_EQ(377u, fibonacci(13));
    ASSERT_EQ(610u, fibonacci(14));
    ASSERT_EQ(987u, fibonacci(15));
    ASSERT_EQ(1597u, fibonacci(16));
    ASSERT_EQ(2584u, fibonacci(17));
    ASSERT_EQ(4181u, fibonacci(18));
    ASSERT_EQ(6765u, fibonacci(19));
    ASSERT_EQ(10946u, fibonacci(20));
    ASSERT_EQ(17711u, fibonacci(21));
}

#endif

