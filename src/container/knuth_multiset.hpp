#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <algorithm>

template<typename T>
struct knuth_multi_set
{
    void reserve(size_t size)
    {
        elems.reserve(size == 0 ? 0 : (size + 1));
        counts.reserve(size);
    }

    void clear()
    {
        elems.clear();
        counts.clear();
    }

    int insert_1(const T & x)
    {
        size_t i = 0;
        for (size_t size = elems.size(); i < size; ++i)
        {
            if (elems[i] == x)
                goto found;
        }
        elems.push_back(x);
        counts.push_back(0);
      found:
        return ++counts[i];
    }
    int insert_1_no_goto(const T & x)
    {
        size_t i = 0;
        for (size_t size = elems.size(); i < size; ++i)
        {
            if (elems[i] == x)
                return ++counts[i];
        }
        elems.push_back(x);
        counts.push_back(1);
        return 1;
    }
    int insert_1a(const T & x)
    {
        size_t i = 0;
        size_t size = elems.size();
        while (i < size && elems[i] != x)
            ++i;
        if (i == size)
        {
            elems.push_back(x);
            counts.push_back(0);
        }
        return ++counts[i];
    }
    int insert_0(const T & x)
    {
        size_t i = elems.size();
        while (i-- != 0)
        {
            if (elems[i] == x)
                return ++counts[i];
        }
        elems.push_back(x);
        counts.push_back(1);
        return 1;
    }
    int insert_2(const T & x)
    {
        size_t size = elems.size();
        elems.push_back(x);
        size_t i = 0;
        while (elems[i] != x)
            ++i;
        if (i == size)
        {
            counts.push_back(1);
            return 1;
        }
        else
        {
            elems.pop_back();
            return ++counts[i];
        }
    }

    int insert_2a(const T & x)
    {
        size_t size = elems.size();
        elems.push_back(x);
        size_t i = 0;
        goto test;
      loop:
        i += 2;
      test:
        if (elems[i] == x)
            goto found;
        if (elems[i + 1] != x)
            goto loop;
        ++i;
      found:
        if (i == size)
        {
            counts.push_back(1);
            return 1;
        }
        else
        {
            elems.pop_back();
            return ++counts[i];
        }
    }
    int insert_2a_loop(const T & x)
    {
        size_t size = elems.size();
        elems.push_back(x);
        size_t i = 0;
        for (;;)
        {
            if (elems[i] == x)
                break;
            if (elems[i + 1] == x)
            {
                ++i;
                break;
            }
            i += 2;
        }
        if (i == size)
        {
            counts.push_back(1);
            return 1;
        }
        else
        {
            elems.pop_back();
            return ++counts[i];
        }
    }
    int insert_stl(const T & x)
    {
        auto found = std::find(elems.begin(), elems.end(), x);
        if (found == elems.end())
        {
            elems.push_back(x);
            counts.push_back(1);
            return 1;
        }
        else
            return ++counts[found - elems.begin()];
    }

private:
    std::vector<T> elems;
    std::vector<int> counts;
};

template<typename T>
struct knuth_multi_set_no_pushback
{
    void reserve(size_t size)
    {
        if (size == 0)
        {
            elems.reset();
            counts.reset();
        }
        else
        {
            elems.reset(new T[size + 1]);
            counts.reset(new int[size]);
        }
    }

    void clear()
    {
        size = 0;
    }

    int insert_1(const T & x)
    {
        size_t i = 0;
        for (; i < size; ++i)
        {
            if (elems[i] == x)
                goto found;
        }
        ++size;
        elems[i] = x;
        counts[i] = 0;
      found:
        return ++counts[i];
    }
    int insert_1_no_goto(const T & x)
    {
        size_t i = 0;
        for (; i < size; ++i)
        {
            if (elems[i] == x)
                return ++counts[i];
        }
        ++size;
        elems[i] = x;
        counts[i] = 1;
        return 1;
    }
    int insert_1a(const T & x)
    {
        size_t i = 0;
        while (i < size && elems[i] != x)
            ++i;
        if (i == size)
        {
            ++size;
            elems[i] = x;
            counts[i] = 0;
        }
        return ++counts[i];
    }
    int insert_0(const T & x)
    {
        size_t i = size;
        while (i-- != 0)
        {
            if (elems[i] == x)
                return ++counts[i];
        }
        elems[size] = x;
        counts[size] = 1;
        ++size;
        return 1;
    }
    int insert_0_asm(const T & x)
    {
        size_t i = size;
        T * ptr = elems.get();
        asm("1: sub $0x1, %0\n"
            "jb 2f\n"
            "cmp %2, (%3, %0, 4)\n"
            "jne 1b\n"
            "2:"
            : "=r"(i)
            : "0"(i), "r"(x), "r"(ptr));
        if (i == static_cast<size_t>(-1))
        {
            elems[size] = x;
            counts[size] = 1;
            ++size;
            return 1;
        }
        else
            return ++counts[i];
    }
    int insert_2(const T & x)
    {
        elems[size] = x;
        size_t i = 0;
        while (elems[i] != x)
            ++i;
        if (i == size)
        {
            counts[size] = 1;
            ++size;
            return 1;
        }
        else
        {
            return ++counts[i];
        }
    }

    int insert_2a(const T & x)
    {
        elems[size] = x;
        size_t i = 0;
        goto test;
      loop:
        i += 2;
      test:
        if (elems[i] == x)
            goto found;
        if (elems[i + 1] != x)
            goto loop;
        ++i;
      found:
        if (i == size)
        {
            ++size;
            counts[i] = 1;
            return 1;
        }
        else
        {
            return ++counts[i];
        }
    }
    int insert_2a_loop(const T & x)
    {
        elems[size] = x;
        size_t i = 0;
        for (;;)
        {
            if (elems[i] == x)
                break;
            if (elems[i + 1] == x)
            {
                ++i;
                break;
            }
            i += 2;
        }
        if (i == size)
        {
            ++size;
            counts[i] = 1;
            return 1;
        }
        else
        {
            return ++counts[i];
        }
    }

    int insert_stl(const T & x)
    {
        auto begin = elems.get();
        auto end = begin + size;
        auto found = std::find(begin, end, x);
        if (found == end)
        {
            elems[size] = x;
            counts[size] = 1;
            ++size;
            return 1;
        }
        else
            return ++counts[found - begin];
    }

private:
    std::unique_ptr<T[]> elems;
    std::unique_ptr<int[]> counts;
    size_t size = 0;
};
