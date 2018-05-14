#pragma once

#include <random>

struct random_seed_seq
{
    template<typename It>
    void generate(It begin, It end)
    {
        for (; begin != end; ++begin)
        {
            *begin = device();
        }
    }

    static random_seed_seq & get_instance()
    {
        static thread_local random_seed_seq result;
        return result;
    }

private:
    std::random_device device;
};
