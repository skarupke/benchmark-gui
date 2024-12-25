#include "ptr_hash_map.hpp"

static_assert(std::is_nothrow_move_constructible<ska::ptr_hash_map<int, int> >::value, "expect it to be nothrow movable");
static_assert(std::is_nothrow_move_assignable<ska::ptr_hash_map<int, int> >::value, "expect it to be nothrow movable");

#ifndef DISABLE_TESTS
#include <gtest/gtest.h>
#include <random>
#include <memory>

TEST(sherwood_v4, insert_and_find)
{
    ska::ptr_hash_map<int, int> a;
    ASSERT_EQ(a.begin(), a.end());
    ASSERT_EQ(a.end(), a.find(10));
    ASSERT_EQ(a.end(), a.find(5));
    ASSERT_EQ(0u, a.size());
    a.emplace(10, 20);
    ASSERT_NE(a.begin(), a.end());
    ASSERT_NE(a.end(), a.find(10));
    ASSERT_EQ(1u, a.size());
    ASSERT_EQ(20, a[10]);
    auto it = a.begin();
    ASSERT_EQ(10, it->first);
    ASSERT_EQ(20, it->second);
    ++it;
    ASSERT_EQ(a.end(), it);
}

TEST(sherwood_v4, initializer_list)
{
    ska::ptr_hash_map<int, int> a = { { 1, 3 }, { 2, 4 }, { 3, 5 }, { 4, 6 } };
    ASSERT_EQ(a.end(), a.find(0));
    ASSERT_NE(a.end(), a.find(1));
    ASSERT_EQ(3, a[1]);
    ASSERT_NE(a.end(), a.find(2));
    ASSERT_EQ(4, a[2]);
    ASSERT_NE(a.end(), a.find(3));
    ASSERT_EQ(5, a[3]);
    ASSERT_NE(a.end(), a.find(4));
    ASSERT_EQ(6, a[4]);
    ASSERT_EQ(a.end(), a.find(5));
    ASSERT_EQ(4u, a.size());
    ASSERT_EQ(0, a[5]);
    ASSERT_EQ(5u, a.size());
}

TEST(sherwood_v4, empty_access)
{
    ska::ptr_hash_map<int, int> a;
    ASSERT_EQ(0, a[0]);
}

TEST(sherwood_v4, conflicts)
{
    ska::ptr_hash_map<int, int> a = { { 1, 3 }, { 6, 4 }, { 11, 5 }, { 16, 6 }, { 21, 7 } };
    ASSERT_EQ(a.end(), a.find(0));
    ASSERT_NE(a.end(), a.find(1));
    ASSERT_EQ(3, a[1]);
    ASSERT_NE(a.end(), a.find(6));
    ASSERT_EQ(4, a[6]);
    ASSERT_NE(a.end(), a.find(11));
    ASSERT_EQ(5, a[11]);
    ASSERT_NE(a.end(), a.find(16));
    ASSERT_EQ(6, a[16]);
    ASSERT_NE(a.end(), a.find(21));
    ASSERT_EQ(7, a[21]);
    ASSERT_EQ(5u, a.size());
    ASSERT_EQ(0, a[26]);
    ASSERT_EQ(6u, a.size());
}

TEST(sherwood_v4, displace_element)
{
    ska::ptr_hash_map<int, int> a = { { 1, 3 }, { 2, 5 }, { 6, 4 }, { 11, 5 }, { 16, 6 }, { 21, 7 } };
    ASSERT_EQ(a.end(), a.find(0));
    ASSERT_NE(a.end(), a.find(1));
    ASSERT_EQ(3, a[1]);
    ASSERT_NE(a.end(), a.find(2));
    ASSERT_EQ(5, a[2]);
    ASSERT_NE(a.end(), a.find(6));
    ASSERT_EQ(4, a[6]);
    ASSERT_NE(a.end(), a.find(11));
    ASSERT_EQ(5, a[11]);
    ASSERT_NE(a.end(), a.find(16));
    ASSERT_EQ(6, a[16]);
    ASSERT_NE(a.end(), a.find(21));
    ASSERT_EQ(7, a[21]);
    ASSERT_EQ(6u, a.size());
    ASSERT_EQ(0, a[26]);
    ASSERT_EQ(7u, a.size());
}
TEST(sherwood_v4, displace_twice)
{
    ska::ptr_hash_map<int, int> a = { { 1, 3 }, { 2, 5 }, { 0, 4 }, { 5, 5 }, { 10, 6 } };
    ASSERT_EQ(a.end(), a.find(-1));
    ASSERT_NE(a.end(), a.find(1));
    ASSERT_EQ(3, a[1]);
    ASSERT_NE(a.end(), a.find(2));
    ASSERT_EQ(5, a[2]);
    ASSERT_NE(a.end(), a.find(0));
    ASSERT_EQ(4, a[0]);
    ASSERT_NE(a.end(), a.find(5));
    ASSERT_EQ(5, a[5]);
    ASSERT_NE(a.end(), a.find(10));
    ASSERT_EQ(6, a[10]);
    ASSERT_EQ(a.end(), a.find(15));
    ASSERT_EQ(5u, a.size());
    ASSERT_EQ(0, a[15]);
    ASSERT_EQ(6u, a.size());
}

TEST(sherwood_v4, iterate)
{
    ska::ptr_hash_map<int, int> a;
    a.emplace(0, 0);
    ASSERT_NE(a.end(), a.find(0));
    ASSERT_EQ(0, a[0]);
    ASSERT_EQ(a.begin(), a.find(0));
    auto begin = a.begin();
    ++begin;
    ASSERT_EQ(a.end(), begin);
}

TEST(sherwood_v4, erase)
{
    ska::ptr_hash_map<int, int> a = { { 1, 3 }, { 2, 4 }, { 3, 5 }, { 4, 6 } };
    ASSERT_EQ(1u, a.erase(2));
    ASSERT_EQ(a.end(), a.find(2));
    auto found = a.find(3);
    a.erase(found);
    ASSERT_EQ(a.end(), a.find(3));
}

TEST(sherwood_v4, erase_conflicting)
{
    ska::ptr_hash_map<int, int> a;
    a.max_load_factor(1.0f);
    a.insert({ { 1, 3 }, { 6, 4 }, { 11, 5 }, { 16, 6 } });
    ASSERT_EQ(1u, a.erase(6));
    ASSERT_EQ(a.end(), a.find(6));
    auto found = a.find(1);
    decltype(found) after_1 = a.erase(found);
    ASSERT_EQ(a.find(11), after_1);
    ASSERT_EQ(a.end(), a.find(1));
    a.erase(11);
    a.erase(16);
    ASSERT_TRUE(a.empty());
}

TEST(sherwood_v4, erase_distance_to_element)
{
    ska::ptr_hash_map<int, int> a;
    a.max_load_factor(1.0f);
    a.rehash(47);
    a.insert({ { 1, 3 }, { 48, 4 }, { 95, 9 }, { 2, 5 }, { 49, 6 }, { 3, 7 }, { 50, 8 } });
    ASSERT_EQ(1u, a.erase(95));
    ASSERT_EQ(a.end(), a.find(95));
    ASSERT_NE(a.end(), a.find(1));
    ASSERT_NE(a.end(), a.find(48));
    ASSERT_NE(a.end(), a.find(2));
    ASSERT_NE(a.end(), a.find(49));
    ASSERT_NE(a.end(), a.find(3));
    ASSERT_NE(a.end(), a.find(50));
}

TEST(sherwood_v4, erase_range_distance_to_element)
{
    ska::ptr_hash_map<int, int> a;
    a.max_load_factor(1.0f);
    a.rehash(47);
    a.insert({ { 1, 3 }, { 48, 4 }, { 95, 9 }, { 2, 5 }, { 49, 6 }, { 3, 7 }, { 50, 8 } });
    ASSERT_EQ(95, a.erase(a.find(2), a.find(95))->first);
    ASSERT_NE(a.end(), a.find(95));
    ASSERT_EQ(a.end(), a.find(1));
    ASSERT_EQ(a.end(), a.find(48));
    ASSERT_EQ(a.end(), a.find(2));
    ASSERT_EQ(a.end(), a.find(49));
    ASSERT_NE(a.end(), a.find(3));
    ASSERT_NE(a.end(), a.find(50));
}

TEST(sherwood_v4, erase_range_to_end)
{
    ska::ptr_hash_map<int, int> a;
    a.max_load_factor(1.0f);
    a.rehash(47);
    a.insert({ { 1, 3 }, { 48, 4 }, { 95, 9 }, { 2, 5 }, { 49, 6 }, { 3, 7 }, { 50, 8 } });
    a.erase(a.find(2), a.end());
    ASSERT_EQ(a.end(), a.find(95));
    ASSERT_EQ(a.end(), a.find(1));
    ASSERT_EQ(a.end(), a.find(48));
    ASSERT_EQ(a.end(), a.find(2));
    ASSERT_EQ(a.end(), a.find(49));
    ASSERT_NE(a.end(), a.find(3));
    ASSERT_NE(a.end(), a.find(50));
}

TEST(sherwood_v4, erase_and_reinsert)
{
    ska::ptr_hash_map<int, int> a;
    a.max_load_factor(1.0f);
    a.rehash(47);
    a.insert({ { 1, 3 }, { 48, 4 }, { 2, 5 }, { 3, 7 } });
    ASSERT_EQ(1u, a.erase(2));
    ASSERT_EQ(a.end(), a.find(2));
    ASSERT_NE(a.end(), a.find(1));
    ASSERT_NE(a.end(), a.find(48));
    ASSERT_NE(a.end(), a.find(3));
    a.insert({ 2, 5 });
    ASSERT_NE(a.end(), a.find(2));
}

TEST(sherwood_v4, power_of_two_hashing)
{
    ska::ptr_hash_set<int, ska::power_of_two_std_hash<int>> a;
    a.max_load_factor(1.0f);
    a.insert({ 1, 2, 3, 4 });
    ASSERT_EQ(4u, a.bucket_count());
    ska::ptr_hash_set<int> b;
    b.max_load_factor(1.0f);
    b.insert({ 1, 2, 3, 4 });
    ASSERT_EQ(5u, b.bucket_count());
}

#include "test/container_test_util.hpp"

template<typename T>
struct sherwood_test_v4 : ::testing::Test
{
};

TYPED_TEST_CASE_P(sherwood_test_v4);

TYPED_TEST_P(sherwood_test_v4, empty)
{
    typename TypeParam::template map<int, int> a;
    ASSERT_EQ(a.end(), a.find(5));
    ASSERT_TRUE(a.empty());
    a.emplace();
    a.clear();
    ASSERT_TRUE(a.empty());
}

TYPED_TEST_P(sherwood_test_v4, simple)
{
    typedef typename TypeParam::template map<int, int> map_type;
    map_type a = { { 1, 5 }, { 2, 6 }, { 3, 7 } };
    ASSERT_EQ(3u, a.size());
    a.insert(std::make_pair(1, 6));
    ASSERT_EQ(3u, a.size());
    ASSERT_EQ(5, a[1]);
    ASSERT_EQ(6, a[2]);
    ASSERT_EQ(7, a[3]);
    ASSERT_EQ(0, a[4]);
    ASSERT_EQ(4u, a.size());
    auto found = a.find(3);
    ASSERT_NE(a.end(), found);
    ASSERT_EQ(3, found->first);
    ASSERT_EQ(7, found->second);
    ASSERT_EQ(a.end(), a.find(5));

    ASSERT_EQ((map_type{ { 1, 5 }, { 2, 6 }, { 3, 7 }, { 4, 0 } }), a);
}
TYPED_TEST_P(sherwood_test_v4, conflicting_insert)
{
    typedef typename TypeParam::template map<int, int> map_type;
    map_type a;
    a.reserve(5);
    a.insert({ { 1, 2 }, { 6, 5 }, { 11, 8 }, { 16, 11 } });
    ASSERT_EQ(2, a[1]);
    ASSERT_EQ(5, a[6]);
    ASSERT_EQ(8, a[11]);
    ASSERT_EQ(11, a[16]);
}
TYPED_TEST_P(sherwood_test_v4, full_map_one_bucket)
{
    typedef typename TypeParam::template map<int, int> map_type;
    map_type a;
    a.reserve(2);
    a.insert({ 1, 2 });
    auto inserted = a.insert({ 3, 4 });
    ASSERT_EQ(2, a[1]);
    ASSERT_TRUE(inserted.second);
    ASSERT_EQ(inserted.first, a.find(3));
    ASSERT_EQ(4, inserted.first->second);
}

TYPED_TEST_P(sherwood_test_v4, move_construct)
{
    typedef typename TypeParam::template map<std::string, std::unique_ptr<int> > map_type;
    map_type a;
    a["foo"] = std::unique_ptr<int>(new int(5));
    ASSERT_EQ(5, *a["foo"]);
    std::pair<std::string, std::unique_ptr<int> > to_insert("foo", std::unique_ptr<int>(new int(6)));
    a.emplace(std::move(to_insert));
    map_type b(std::move(a));
    ASSERT_EQ(1u, b.size());
    ASSERT_EQ(0u, a.size());
    ASSERT_EQ(5, *b["foo"]);
}
TYPED_TEST_P(sherwood_test_v4, copy)
{
    typedef typename TypeParam::template map<int, int> map_type;
    map_type a{ { 1, 2 }, { 3, 4 }, { 5, 6 } };
    map_type b(a);
    ASSERT_EQ(a, b);
}
TYPED_TEST_P(sherwood_test_v4, erase)
{
    typedef typename TypeParam::template map<int, int> map_type;
    map_type a = { { 1, 2 }, { 3, 4 } };
    ASSERT_EQ(2u, a.size());
    ASSERT_EQ(1u, a.erase(3));
    ASSERT_EQ(1u, a.size());
    ASSERT_EQ((map_type{ { 1, 2 } }), a);
}
TYPED_TEST_P(sherwood_test_v4, iterator_erase)
{
    typedef typename TypeParam::template map<int, int> map_type;
    map_type a = { { 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } };
    auto begin = a.erase(a.begin(), std::next(a.begin(), 2));
    ASSERT_EQ(a.begin(), begin);
    ASSERT_EQ(*a.begin(), *begin);
    begin = a.erase(a.begin());
    ASSERT_EQ(a.begin(), begin);
    ASSERT_EQ(*a.begin(), *begin);
    begin = a.erase(a.begin());
    ASSERT_EQ(a.end(), begin);
    ASSERT_TRUE(a.empty());
}
TYPED_TEST_P(sherwood_test_v4, two_element_conflicting_iterator_erase)
{
    typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
    map_type a;
    size_t count = 5;
    a.reserve(count);
    count = a.bucket_count();
    a.insert(
    {
        { 0 * count + 1, 2 },
        { 1 * count + 1, 5 },
    });
    ASSERT_EQ(a.end(), a.erase(a.begin(), a.end()));
}
TYPED_TEST_P(sherwood_test_v4, two_bucket_conflicting_iterator_erase)
{
    typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
    map_type a;
    size_t count = 5;
    a.reserve(count);
    count = a.bucket_count();
    a.insert(
    {
        { 0 * count + 1, 2 },
        { 1 * count + 1, 5 },
        { 1 * count - 1, 8 },
        { 2 * count - 1, 11 },
        { 3 * count - 1, 14 }
    });
    auto begin = a.erase(2 * count - 1);
    ASSERT_EQ(1u, begin);
    ASSERT_EQ(a.begin(), a.find(a.begin()->first));
    ASSERT_EQ((map_type{ { 0 * count + 1, 2 }, { 1 * count + 1, 5 }, { 1 * count - 1, 8 }, { 3 * count - 1, 14 } }), a);
    ASSERT_EQ(a.end(), a.erase(a.begin(), a.end()));
}
TYPED_TEST_P(sherwood_test_v4, range_erase)
{
    typedef typename TypeParam::template map<int, int> map_type;
    map_type a{ { 1, 5 }, { 2, 6 }, { 3, 7 }, { 4, 0 } };
    a.erase(std::next(a.begin()), a.end());
    ASSERT_EQ(1u, a.size());
    a.erase(a.begin(), a.end());
    ASSERT_TRUE(a.empty());
}
TYPED_TEST_P(sherwood_test_v4, erase_all)
{
    typedef typename TypeParam::template map<int, int> map_type;
    map_type a;
    a.rehash(3);
    a.insert({ { 1, 5 }, { 2, 6 }, { 3, 7 } });
    ASSERT_EQ(a.end(), a.erase(a.begin(), a.end()));
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(a.end(), a.begin());
}
TYPED_TEST_P(sherwood_test_v4, move_over_please)
{
    typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
    map_type a;
    a.max_load_factor(1.0f);
    size_t count = 5;
    a.rehash(count);
    count = a.bucket_count();
    // the idea is this: 1 and 2 go to their spots. 5 goes to 0,
    // 10 can't go to 0 so goes to 1 and pushes 1 and 2 over,
    // 15 does the same thing
    a.insert(
    {
        { 0 * count + 1, 5 },
        { 0 * count + 2, 6 },
        { 1 * count + 0, 7 },
        { 2 * count + 0, 8 },
        { 3 * count + 0, 9 }
    });
    ASSERT_EQ(5, a[0 * count + 1]);
    ASSERT_EQ(6, a[0 * count + 2]);
    ASSERT_EQ(7, a[1 * count + 0]);
    ASSERT_EQ(8, a[2 * count + 0]);
    ASSERT_EQ(9, a[3 * count + 0]);
}
TYPED_TEST_P(sherwood_test_v4, move_over_distance)
{
    typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
    map_type a;
    a.max_load_factor(1.0f);
    size_t count = 7;
    a.rehash(count);
    count = a.bucket_count();
    // the idea is this: 1 and 2 go to their spots. 7 goes to 0,
    // 14 can't go to 0 so goes to 1 and pushes 1 and 2 over,
    // now I've had a bug where the distance of element 2 was now
    // incorrect, so I place an object at 3 and it will push
    // 2 out of the way
    a.insert(
    {
        { 0 * count + 1, 5 },
        { 0 * count + 2, 6 },
        { 1 * count + 0, 7 },
        { 2 * count + 0, 8 },
        { 3 * count + 0, 9 },
        { 0 * count + 3, 10 }
    });
    ASSERT_EQ((map_type{ { 0 * count + 1, 5 }, { 0 * count + 2, 6 }, { 1 * count + 0, 7 }, { 2 * count + 0, 8 }, { 3 * count + 0, 9 }, { 0 * count + 3, 10 } }), a);
}
TYPED_TEST_P(sherwood_test_v4, many_collisions)
{
    typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
    map_type a;
    a.max_load_factor(1.0f);
    size_t count = 11;
    a.rehash(count);
    count = a.bucket_count();
    std::initializer_list<std::pair<int, int> > il =
    {
        { 1 * count + 2, 8 },
        { 0 * count + 2, 3 },
        { 0 * count + 1, 2 },
        { 0 * count + 5, 4 },
        { 1 * count + 1, 6 },
        { 2 * count + 1, 7 },
        { 1 * count + 0, 10 },
        { 2 * count + 0, 5 },
    };
    ASSERT_GE(a.bucket_count(), il.size());
    a.insert(il);
    for (auto it = il.begin(); it != il.end(); ++it)
    {
        auto found = a.find(it->first);
        ASSERT_NE(a.end(), found);
        ASSERT_EQ(*it, *found);
    }
}
TYPED_TEST_P(sherwood_test_v4, more_collision_insert)
{
    typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
    map_type a;
    a.max_load_factor(1.0f);
    size_t count = 11;
    a.rehash(count);
    count = a.bucket_count();
    std::initializer_list<std::pair<int, int> > il =
    {
        { 1 * count + 1, 8 },
        { 1 * count + 2, 6 },
        { 2 * count + 1, 3 },
        { 2 * count + 2, 7 },
        { 1 * count + 3, 5 },
    };
    ASSERT_GE(a.bucket_count(), il.size());
    a.insert(il);
    for (auto it = il.begin(); it != il.end(); ++it)
    {
        auto found = a.find(it->first);
        ASSERT_NE(a.end(), found);
        ASSERT_EQ(*it, *found);
    }
}

TYPED_TEST_P(sherwood_test_v4, emplace_hint)
{
    typedef typename TypeParam::template map<int, int> map_type;
    map_type a = { { 1, 2 }, { 3, 4 } };
    auto found = a.find(1);
    ASSERT_NE(a.end(), found);
    auto insert_point = a.erase(found);
    a.insert(insert_point, std::make_pair(1, 3));
    ASSERT_EQ((map_type{ { 1, 3 }, { 3, 4 } }), a);
}

TYPED_TEST_P(sherwood_test_v4, swap)
{
    typedef typename TypeParam::template map<int, int> map_type;
    map_type a = { { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 } };
    map_type b = { { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 }, { 6, 6 }, { 7, 7 } };
    a.swap(b);
    ASSERT_EQ((map_type{ { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 } }), b);
    ASSERT_EQ(5u, b.size());
    ASSERT_EQ((map_type{ { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 }, { 6, 6 }, { 7, 7 } }), a);
    ASSERT_EQ(6u, a.size());
}
TYPED_TEST_P(sherwood_test_v4, destructor_caller)
{
    typedef typename TypeParam::template map<int, CtorDtorCounter, std::hash<int>, std::equal_to<int>, CountingAllocator<std::pair<int, CtorDtorCounter> > > map_type;
    ScopedAssertNoLeaks leak_check(true, false);
    int ctor_counter = 0;
    int dtor_counter = 0;
    {
        map_type a;
        for (int i = 0; i < 4; ++i)
        {
            a.emplace(i, CtorDtorCounter(ctor_counter, dtor_counter));
        }
        a.erase(5);
        a.erase(a.begin(), std::next(a.begin(), 3));
    }
    ASSERT_NE(0, ctor_counter);
    ASSERT_EQ(ctor_counter, dtor_counter);
}

TYPED_TEST_P(sherwood_test_v4, stateful_hasher_test)
{
    typedef typename TypeParam::template map<int, int, stateful_hasher> map_type;
    map_type a{ { 1, 2 }, { 3, 4 }, { 5, 7 }, { 8, 9 } };
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wself-assign-overloaded"
    a = a;
    #pragma clang diagnostic pop
    ASSERT_EQ((map_type{ { 1, 2 }, { 3, 4 }, { 5, 7 }, { 8, 9 } }), a);
}

TYPED_TEST_P(sherwood_test_v4, load_factor)
{
    typename TypeParam::template map<int, int> a;
    ASSERT_EQ(0, a.load_factor());
}
TYPED_TEST_P(sherwood_test_v4, dont_grow_when_you_wont_insert)
{
    typename TypeParam::template map<int, int> a;
    a.reserve(3);
    size_t bucket_count_before = a.bucket_count();
    a.insert({ { 1, 2 }, { 3, 4 }, { 5, 6 }, { 5, 6 } });
    ASSERT_EQ(bucket_count_before, a.bucket_count());
}
TYPED_TEST_P(sherwood_test_v4, allow_growing_with_max_load_factor_1)
{
    typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
    map_type a;
    a.max_load_factor(1.0f);
    a.reserve(3);
    a.insert({ { 1, 2 }, { 4, 4 }, { 7, 6 }, { 10, 8 } });
    ASSERT_EQ((map_type{ { 1, 2 }, { 4, 4 }, { 7, 6 }, { 10, 8 } }), a);
}
TYPED_TEST_P(sherwood_test_v4, max_load_factor)
{
    typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
    map_type a;
    a.max_load_factor(0.4f);
    a.reserve(1);
    ASSERT_LE(3u, a.bucket_count());
}

REGISTER_TYPED_TEST_CASE_P(sherwood_test_v4, empty, simple, move_construct, copy,
                           erase, iterator_erase,
                           two_element_conflicting_iterator_erase, range_erase,
                           move_over_please, emplace_hint, swap,
                           destructor_caller,
                           stateful_hasher_test, load_factor, max_load_factor,
                           dont_grow_when_you_wont_insert, allow_growing_with_max_load_factor_1,
                           conflicting_insert,
                           full_map_one_bucket, move_over_distance, many_collisions,
                           more_collision_insert,
                           two_bucket_conflicting_iterator_erase, erase_all);

TEST(ska_ptr_hash_set, simple)
{
    ska::ptr_hash_set<int> a = { 4, 3, 7, 12, 6, 5, 19, -7, 4 };
    ASSERT_EQ(8u, a.size());
    ASSERT_EQ(1u, a.count(4));
    auto found = a.find(1);
    ASSERT_EQ(a.end(), found);
    found = a.find(5);
    ASSERT_NE(a.end(), found);
    ASSERT_EQ(5, *found);
    a.erase(5);
    found = a.find(5);
    ASSERT_EQ(a.end(), found);
    ASSERT_EQ(7u, a.size());
    a.insert(1);
    found = a.find(1);
    ASSERT_NE(a.end(), found);
    ASSERT_EQ(8u, a.size());
    ASSERT_EQ(1, *found);
    a.erase(found);
    ASSERT_EQ(7u, a.size());
    found = a.find(1);
    ASSERT_EQ(a.end(), found);
}

TEST(ska_ptr_hash_set, range_erase)
{
    size_t bucket_count = 11;
    ska::ptr_hash_set<int> x;
    x.insert(bucket_count * 1);
    x.insert(bucket_count * 2);
    x.insert(bucket_count * 3);
    x.insert(bucket_count * 4);
    x.insert(bucket_count * 5);
    x.insert(bucket_count * 6);
    ASSERT_EQ(bucket_count, x.bucket_count());
    ASSERT_EQ(6u, x.size());
    auto begin = std::next(x.begin(), 1);
    auto end = std::next(begin, 3);
    std::vector<int> erased(begin, end);
    ASSERT_EQ(3u, erased.size());
    std::vector<int> not_erased(end, x.end());
    not_erased.insert(not_erased.end(), x.begin(), begin);
    ASSERT_EQ(3u, not_erased.size());
    ASSERT_EQ(not_erased.front(), *x.erase(begin, end));
    ASSERT_EQ(3u, x.size());
    for (int i : erased)
    {
        ASSERT_EQ(x.end(), x.find(i));
    }
    for (int i : not_erased)
    {
        ASSERT_NE(x.end(), x.find(i));
    }
}

struct sherwood_v4_tester
{
    template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<K, V> > >
    using map = ska::ptr_hash_map<K, V, H, E, A>;
};

INSTANTIATE_TYPED_TEST_CASE_P(sherwood_v4, sherwood_test_v4, sherwood_v4_tester);

#endif
