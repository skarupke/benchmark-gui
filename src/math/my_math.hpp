#pragma once

template<typename T>
inline T squared(const T & value)
{
    return value * value;
}
template<typename T>
inline const T & clamp(const T & value, const T & min, const T & max)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}

inline int sum_to(int n)
{
    return (n * (n + 1)) / 2;
}
