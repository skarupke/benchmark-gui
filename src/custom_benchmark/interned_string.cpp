#include "custom_benchmark/interned_string.hpp"
#include "container/flat_hash_map.hpp"
#include <mutex>
#include <memory>

static std::mutex & all_interned_strings_mutex()
{
    static std::mutex result;
    return result;
}
static ska::flat_hash_map<std::string_view, std::unique_ptr<const std::string>> & all_interned_strings()
{
    static ska::flat_hash_map<std::string_view, std::unique_ptr<const std::string>> result;
    return result;
}

interned_string::interned_string(std::string_view argument)
{
    std::lock_guard<std::mutex> lock(all_interned_strings_mutex());
    auto & all_strings = all_interned_strings();
    auto found = all_strings.find(argument);
    if (found == all_strings.end())
    {
        std::unique_ptr<const std::string> ptr = std::make_unique<const std::string>(argument);
        str = *ptr;
        all_strings.emplace(str, std::move(ptr));
    }
    else
        str = found->first;
}

#ifndef DISABLE_GTEST

#include "test/include_test.hpp"

TEST(interned_string, simple)
{
    interned_string a("foo");
    interned_string b("bar");
    ASSERT_NE(a, b);
    interned_string c("foo");
    ASSERT_EQ(a, c);
    ASSERT_EQ(a.view().data(), c.view().data());
}

TEST(interned_string, string_less)
{
    interned_string foo("foo");
    interned_string bar("bar");
    interned_string baz("baz");
    interned_string::string_less less;
    ASSERT_TRUE(less(bar, baz));
    ASSERT_TRUE(less(baz, foo));
    ASSERT_TRUE(less(bar, foo));
    ASSERT_FALSE(less(foo, bar));
    ASSERT_FALSE(less(foo, baz));
    ASSERT_FALSE(less(baz, bar));
}

TEST(interned_string, to_interned_string)
{
    interned_string one = to_interned_string(1);
    interned_string two = to_interned_string(2);
    interned_string one_again = to_interned_string(1);
    ASSERT_TRUE(one == one_again);
    ASSERT_FALSE(one == two);
    ASSERT_EQ(1u, one.size());
    ASSERT_EQ(1u, two.size());
    ASSERT_EQ(1u, one_again.size());
    ASSERT_EQ(one, "1");
    ASSERT_EQ(two, "2");
    ASSERT_EQ(one_again, "1");
}

TEST(interned_string, stack_array)
{
    const char buffer[256] = "1";
    interned_string one = to_interned_string(1);
    interned_string from_buffer(buffer);
    ASSERT_EQ(one, from_buffer);
}

#endif
