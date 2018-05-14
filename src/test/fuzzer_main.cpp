#ifdef FUZZER_BUILD
#include "container/flat_hash_map.hpp"
#include <map>
#include <vector>

#include <cstdint>
#define PRINT_WHILE_FUZZING 0
#if PRINT_WHILE_FUZZING
#include <iostream>
#define FUZZ_LOG(...) std::cout << __VA_ARGS__ << std::endl
#else
#define FUZZ_LOG(...) static_cast<void>(0)
#endif

static constexpr float override_max_load_factor = 0.0f;

template<typename T>
struct custom_allocator : std::allocator<T>
{
    template<typename U>
    struct rebind
    {
        typedef custom_allocator<U> other;
    };
    template<typename U>
    void destroy(U * ptr)
    {
        ptr->~U();
    }
    template<typename U2>
    void destroy(std::pair<int, U2> * ptr)
    {
        ptr->first = 0;
        ptr->second.~U2();
    }
};

/*struct OtherBitsHash
{
    template<typename T>
    size_t operator()(const T & value) const
    {
        return std::hash<T>()(value);
    }

    using hash_policy = ska::power_of_two_hash_policy_other_bits;
};*/

template<typename K, typename V>
using test_hash_map = ska::flat_hash_map<K, V, std::hash<K>, std::equal_to<K>, custom_allocator<std::pair<K, V>>>;

template<typename K, typename V>
void sanity_check(const std::map<K, V> & check, const test_hash_map<K, V> & test)
{
    if (check.size() != test.size())
        __builtin_trap();
    size_t test_size = test.size();
    size_t double_check_size = 0;
    for (auto it = test.begin(), end = test.end(); it != end; ++it)
    {
        ++double_check_size;
        if (test.find(it->first) != it)
            __builtin_trap();
    }
    if (test_size != double_check_size)
        __builtin_trap();
    for (const auto & pair : check)
    {
        auto found = test.find(pair.first);
        if (found == test.end())
            __builtin_trap();
        if (found->first != pair.first)
            __builtin_trap();
        if (found->second != pair.second)
            __builtin_trap();
    }
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t * data, std::size_t size)
{
    FUZZ_LOG("\n\nstarting new fuzz run\n");
    uint32_t num_values = 0;
    std::map<int, uint32_t> check;
    test_hash_map<int, uint32_t> test;
    if (override_max_load_factor > 0.0f)
        test.max_load_factor(override_max_load_factor);
    for (const std::uint8_t * it = data, * end = data + size; it != end;)
    {
        int action = *it % 7;
        switch(action)
        {
        case 0:
            ++it;
            if (end - it >= 4)
            {
                int key = *reinterpret_cast<const int *>(it);
                uint32_t value = ++num_values;
                if (check.emplace(key, value).second)
                {
                    FUZZ_LOG("inserting " << key << " using emplace");
                    if (!test.emplace(key, value).second)
                        __builtin_trap();
                }
                else if (test.emplace(key, value).second)
                {
                    FUZZ_LOG("inserting " << key << " worked but it should already be in the map");
                    __builtin_trap();
                }
                it += 4;
                break;
            }
            else
                return 0;
        case 1:
            ++it;
            if (end - it >= 4)
            {
                int key = *reinterpret_cast<const int *>(it);
                if (check[key] == 0)
                {
                    FUZZ_LOG("inserting " << key << " using operator[]");
                    if (test[key] != 0)
                        __builtin_trap();
                    uint32_t value = ++num_values;
                    FUZZ_LOG("calling operator[] again with " << key);
                    test[key] = check[key] = value;
                }
                else if (test[key] != check[key])
                {
                    FUZZ_LOG("inserting " << key << " worked but it should already be in the map");
                    __builtin_trap();
                }
                it += 4;
                break;
            }
            else
                return 0;
        case 2:
            ++it;
            if (end - it >= 4)
            {
                int key = *reinterpret_cast<const int *>(it);
                auto found = check.find(key);
                if (found != check.end())
                {
                    FUZZ_LOG("calling find and operator[] with key " << key);
                    if (test.find(key) == test.end())
                        __builtin_trap();
                    else if (test[key] != check[key])
                        __builtin_trap();
                }
                else
                {
                    FUZZ_LOG("calling find with key " << key);
                    if (test.find(key) != test.end())
                        __builtin_trap();
                }
                it += 4;
                break;
            }
            else
                return 0;
        case 3:
            ++it;
            FUZZ_LOG("calling sanity_check");
            sanity_check(check, test);
            break;
        case 4:
            ++it;
            if (end - it >= 4)
            {
                int key = *reinterpret_cast<const int *>(it);
                if (check.erase(key))
                {
                    FUZZ_LOG("erasing " << key);
                    if (!test.erase(key))
                        __builtin_trap();
                }
                else if (test.erase(key))
                {
                    FUZZ_LOG("erasing " << key << " worked but it shouldn't have been in the map");
                    __builtin_trap();
                }
                it += 4;
                break;
            }
            else
                return 0;
        case 5:
            ++it;
            if (end - it >= 8)
            {
                size_t to_mod = std::max(test.size(), size_t(1));
                size_t first = reinterpret_cast<const int *>(it)[0];
                size_t second = reinterpret_cast<const int *>(it)[1];
                first = first % to_mod;
                second = second % to_mod;
                if (first > second)
                    std::swap(first, second);
                auto first_it = std::next(test.begin(), first);
                auto second_it = std::next(test.begin(), second);
                FUZZ_LOG("erasing from index " << first << " to " << second);
                for (auto it = first_it; it != second_it; ++it)
                {
                    FUZZ_LOG("erasing " << it->first);
                    if (!check.erase(it->first))
                        __builtin_trap();
                }
                test.erase(first_it, second_it);
                it += 8;
                break;
            }
            else
                return 0;
        case 6:
            ++it;
            if (end - it >= 8)
            {
                size_t to_mod = std::max(test.size(), size_t(1));
                size_t first = reinterpret_cast<const int *>(it)[0];
                size_t second = reinterpret_cast<const int *>(it)[1];
                first = first % to_mod;
                second = second % to_mod;
                if (first > second)
                    std::swap(first, second);
                if (first == second)
                    break;
                FUZZ_LOG("inserting range from " << first << " to " << second);
                for (size_t key = first; key < second; ++key)
                {
                    int value = ++num_values;
                    if (check.emplace(key, value).second)
                    {
                        FUZZ_LOG("inserting " << key);
                        if (!test.emplace(key, value).second)
                            __builtin_trap();
                    }
                    else if (test.emplace(key, value).second)
                    {
                        FUZZ_LOG("inserting " << key << " worked but it should already be in the map");
                        __builtin_trap();
                    }
                }
                it += 8;
                break;
            }
            else
                return 0;
        }
    }
    return 0;
}
#endif
