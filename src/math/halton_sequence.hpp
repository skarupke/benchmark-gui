#pragma once

#include <vector>
#include "math/powers_of_two.hpp"
#include <stddef.h>

template<size_t Base>
float HaltonValue(size_t i)
{
    float f = 1.0f;
    float r = 0.0f;
    while (i > 0)
    {
        f /= Base;
        r += f * (i % Base);
        i /= Base;
    }
    return r;
}


template<size_t Base>
int HaltonValue(size_t f, size_t i)
{
    size_t r = 0;
    while (i > 0)
    {
        f /= Base;
        r += f * (i % Base);
        i /= Base;
    }
    return r;
}



template<typename T, typename A>
std::vector<T, A> shuffle_in_halton_order(std::vector<T, A> input)
{
    std::vector<T, A> result;
    result.reserve(input.size());
    size_t base = next_power_of_two(input.size());
    for (size_t i = 0; i < base; ++i)
    {
        size_t index = HaltonValue<2>(base, i);
        if (index >= input.size())
            continue;
        result.push_back(std::move(input[index]));
    }
    return result;
}

