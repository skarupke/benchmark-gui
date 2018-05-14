#include "small_vector.hpp"

#ifndef DISABLE_TESTS
#include "test/include_test.hpp"

TEST(small_vector, simple)
{
    small_vector<int, 4> a = { 1, 2, 3, 4 };
    ASSERT_EQ(1, a[0]);
    ASSERT_EQ(2, a[1]);
    ASSERT_EQ(3, a[2]);
    ASSERT_EQ(4, a[3]);
    ASSERT_EQ(4u, a.size());
    a.push_back(5);
    ASSERT_EQ(1, a[0]);
    ASSERT_EQ(2, a[1]);
    ASSERT_EQ(3, a[2]);
    ASSERT_EQ(4, a[3]);
    ASSERT_EQ(5, a[4]);
    ASSERT_EQ(5u, a.size());
}
TEST(small_vector, erasing_an_element)
{
    small_vector<int, 4> a = { 1, 2, 3, 4, 5 };

    a.erase(a.begin());
    ASSERT_EQ((small_vector<int, 4>{ 2, 3, 4, 5 }), a);
}
TEST(small_vector, erasing_multiple_elements)
{
    small_vector<int, 4> a = { 1, 2, 3, 4, 5 };

    a.erase(a.begin() + 1, a.begin() + 3);
    ASSERT_EQ((small_vector<int, 4>{ 1, 4, 5 }), a);
}
TEST(small_vector, inserting_self_again)
{
    small_vector<int, 4> a = { 1, 2, 3, 4, 5 };

    ASSERT_GT(10u, a.capacity());
    a.insert(a.begin(), a.begin(), a.end());
    ASSERT_LE(10u, a.capacity());
    ASSERT_EQ((small_vector<int, 4>{ 1, 2, 3, 4, 5, 1, 2, 3, 4, 5 }), a);
}
TEST(small_vector, push_back_self)
{
    small_vector<int, 4> a = { 1, 2, 3, 4 };
    ASSERT_EQ(4u, a.size());
    ASSERT_EQ(a.capacity(), a.size());
    a.push_back(a[3]);
    ASSERT_EQ((small_vector<int, 4>{ 1, 2, 3, 4, 4 }), a);
}
TEST(small_vector, assign)
{
    small_vector<int, 4> a = { 1, 2, 3, 4 };
    a.assign({ 1, 2, 3 });
    ASSERT_EQ((small_vector<int, 4>{ 1, 2, 3 }), a);
    a.assign({ 1, 2, 3, 4, 5 });
    ASSERT_EQ((small_vector<int, 4>{ 1, 2, 3, 4, 5 }), a);
}
TEST(small_vector, swap)
{
    small_vector<int, 4> a = { 1, 2, 3, 4 };
    small_vector<int, 4> b = { 5, 6, 7 };
    swap(a, b);
    ASSERT_EQ((small_vector<int, 4>{ 5, 6, 7 }), a);
    ASSERT_EQ((small_vector<int, 4>{ 1, 2, 3, 4 }), b);
}
TEST(small_vector, shrink_to_fit)
{
    small_vector<int, 2> a = { 1, 2, 3, 4 };
    ASSERT_EQ(4u, a.capacity());
    a.erase(a.begin());
    a.shrink_to_fit();
    ASSERT_EQ(3u, a.capacity());
    a.erase(a.begin());
    a.shrink_to_fit();
    ASSERT_EQ(2u, a.capacity());
    a.erase(a.begin());
    a.shrink_to_fit();
    ASSERT_EQ(2u, a.capacity());
    a.erase(a.begin());
    a.shrink_to_fit();
    ASSERT_EQ(2u, a.capacity());
}

#endif
