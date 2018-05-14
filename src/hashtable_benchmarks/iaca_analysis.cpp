
#define USE_FLAT_MAP 0
#define USE_BYTELL_MAP 1
#if USE_FLAT_MAP
#include "container/flat_hash_map.hpp"
#elif USE_BYTELL_MAP
#include "container/bytell_hash_map.hpp"
#else
#include <google/dense_hash_map>
#endif

#include <iostream>
#define IACA_BUILD 0
#if IACA_BUILD
#include "iacaMarks.h"
#else
#define IACA_START
#define IACA_END
#endif

#if USE_FLAT_MAP
ska::flat_hash_map<int, int, ska::power_of_two_std_hash<int>, std::equal_to<int>, std::allocator<std::pair<int, int>>> iaca_hash_map;
#elif USE_BYTELL_MAP
ska::bytell_hash_map<int, int, ska::libdivide_std_hash<int>, std::equal_to<int>, std::allocator<std::pair<int, int>>> iaca_hash_map;
#else
google::dense_hash_map<int, int> iaca_hash_map;
#endif

void test_iaca(int to_find)
{
    IACA_START
    //bool found = iaca_hash_map.find(to_find) == iaca_hash_map.end();
    bool found = iaca_hash_map[to_find] != 0;
    IACA_END
    if (found)
        std::cout << "found" << std::endl;
}

