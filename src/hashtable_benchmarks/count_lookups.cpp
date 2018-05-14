#include "container/ptr_hash_map.hpp"
#include "benchmark_shared.hpp"
#include <iostream>
#include <map>

struct LookupResult
{
    std::map<int, int> counts;
    float load_factor;
};

template<typename T>
struct num_lookup_counter
{
    LookupResult operator()(int num_entries) const
    {
        T map;
        map.max_load_factor(0.9375f);

        std::uniform_int_distribution<int> distribution;
        for (int i = 0; i < num_entries; ++i)
        {
            map[distribution(global_randomness)] = static_cast<int>(map.size());
        }

        std::map<int, int> counts;
        //for (const auto & pair : map)
        //    ++counts[map.num_lookups(pair.first)];
        for (size_t i = 0; i < map.size(); ++i)
            ++counts[map.num_lookups(distribution(global_randomness))];
        //for (const auto & pair : map.jump_distance_counts())
        //    counts[pair.first] = pair.second;
        return { counts, map.load_factor() };
    }
};

template<typename T, template<typename> typename Counter = num_lookup_counter>
void count_and_print_lookups(const std::string & name)
{
    Counter<T> counter;
    for (int i = 4; i <= 1024 * 1024; i = std::max(i + 1, static_cast<int>(i * 1.01f)))
    {
        LookupResult map_count = counter(i);
        std::cout << name << " size: " << i << ", load_factor: " << map_count.load_factor;
        for (std::pair<int, int> count : map_count.counts)
        {
            std::cout << '\n' << count.first << ": " << count.second;
        }
        std::cout << '\n' << std::endl;
    }
}

void count_num_lookups()
{
    //count_and_print_lookups<ska::flat_hash_map<int, int, power_of_two_hash>>("flat_hash_map default");
    //count_and_print_lookups<ska::flat_hash_map<int, int, power_of_two_hash, std::equal_to<int>, std::allocator<int>, ska::detailv3::JumpToDistance>>("flat_hash_map jump to distance");
    //count_and_print_lookups<ska::ptr_hash_map<int, int, ska::power_of_two_std_hash<int>>>("ptr_hash_map");
    //count_and_print_lookups<ska::flat16_hash_map<int, int, power_of_two_hash>>("flat16_hash_map");
    //count_and_print_lookups<ska::block_hash_map<int, int, power_of_two_hash>>("block_hash_map");
    //count_and_print_lookups<ska::bytell_hash_map<int, int, ska::power_of_two_std_hash<int>, std::equal_to<int>, std::allocator<std::pair<int, int>>, ska::detailv8::NotQuiteFibonacci>>("bytell_hash_map");
}
