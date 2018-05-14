#include "util/view.hpp"
#include <ostream>

std::ostream & operator<<(std::ostream & lhs, StringView<const char> rhs)
{
    std::copy(rhs.begin(), rhs.end(), std::ostreambuf_iterator<char>(lhs));
    return lhs;
}

#ifndef DISABLE_TESTS
#include <gtest/gtest.h>

TEST(view, simple)
{
    ArrayView<int> a;
    ASSERT_TRUE(a.empty());
    StringView<const char> b;
    ASSERT_TRUE(b.empty());
    int c[] = { 1, 2, 3 };
    ArrayView<int> d = c;
    ASSERT_EQ(1, d[0]);
    ASSERT_EQ(2, d[1]);
    ASSERT_EQ(3, d[2]);

    std::array<int, 4> e = { { 4, 5, 6, 7 } };
    ArrayView<int> e_view = e;
    ASSERT_EQ(4, e_view[0]);
    ASSERT_EQ(5, e_view[1]);
    ASSERT_EQ(6, e_view[2]);
    ASSERT_EQ(7, e_view[3]);
}

TEST(view, equal)
{
    StringView<const char> a = "hello, world";
    ASSERT_EQ(a, a);
    ASSERT_EQ("hello, world", a);
    ASSERT_NE("hello, world!", a);
    ASSERT_NE("hello world", a);
    const char * unknown_size = a.begin();
    ASSERT_EQ(a, unknown_size);
}

TEST(view, subrange)
{
    StringView<const char> a = "hello, world";
    ASSERT_EQ("hello", a.subview(0, 5));
    ASSERT_EQ("world", a.subview(7, 5));
    ASSERT_EQ("orld", a.subview(8));
    ASSERT_EQ("orl", a.subview(8, 3));
}

TEST(view, startswith)
{
    StringView<const char> a = "hello, world";
    ASSERT_TRUE(a.startswith("hello"));
    ASSERT_FALSE(a.startswith("world"));
    ASSERT_FALSE(a.endswith("hello"));
    ASSERT_TRUE(a.endswith("world"));
    ASSERT_TRUE(a.startswith('h'));
    ASSERT_TRUE(a.endswith('d'));
}

TEST(view, string_view_not_null_terminated)
{
    const char hi[5] = { 'h', 'e', 'l', 'l', 'o' };
    StringView<const char> hello = hi;
    StringView<const char> there = "hello";
    ASSERT_EQ(hello, there);
}

#include <unordered_map>

TEST(view, as_key)
{
    std::unordered_map<StringView<const char>, int> a;
    a["hi"] = 5;
    a["there"] = 6;
    ASSERT_EQ(5, a["hi"]);
    ASSERT_EQ(6, a["there"]);

    std::string non_const_first = "first";
    std::string non_const_second = "second";
    std::unordered_map<StringView<char>, int> b;
    b[non_const_first] = 7;
    b[non_const_second] = 8;
    ASSERT_EQ(7, b[non_const_first]);
    ASSERT_EQ(8, b[non_const_second]);
}

#endif

