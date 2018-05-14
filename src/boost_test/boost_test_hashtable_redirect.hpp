#pragma once

#include "container/bytell_hash_map.hpp"
#include "container/ptr_hash_map.hpp"

#define BOOST_TEST_HASH_MAP1 ska::bytell_hash_map
template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<const K, V>>>
using boost_test_hash_map1 = BOOST_TEST_HASH_MAP1<K, V, H, E, A>;

#define BOOST_TEST_HASH_MAP2 ska::ptr_hash_map
template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<const K, V>>>
using boost_test_hash_map2 = BOOST_TEST_HASH_MAP2<K, V, H, E, A>;

#define BOOST_TEST_HASH_SET ska::bytell_hash_set
template<typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename A = std::allocator<T>>
using boost_test_hash_set = BOOST_TEST_HASH_SET<T, H, E, A>;
