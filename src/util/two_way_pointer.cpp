#include "two_way_pointer.hpp"

#ifndef DISABLE_TESTS
#include "test/wrap_catch.hpp"
#include <iostream>

namespace
{

TEST(two_way_pointer, void)
{
    TwoWayPointer<void, float> a;
    TwoWayPointer<float, void> b(&a, 10.5f);
    REQUIRE(*a == 10.5f);
    TwoWayPointer<void, float> second_a = std::move(a);
    REQUIRE(*second_a == 10.5f);
    REQUIRE(!a);
    TwoWayPointer<float, void> second_b = std::move(b);
    REQUIRE(*second_a == 10.5f);
    REQUIRE(!b);


    REQUIRE(a == nullptr);
    REQUIRE(b == nullptr);
    REQUIRE(nullptr == a);
    REQUIRE(nullptr == b);
}

TEST(two_way_pointer, content)
{
    TwoWayPointer<std::string, float> a("foo");
    TwoWayPointer<float, std::string> b(&a, 10.5f);
    REQUIRE(*a == 10.5f);
    REQUIRE(*b == "foo");
    a.cut_connection();
    REQUIRE(!a);
    REQUIRE(!b);
    REQUIRE(a.value == "foo");
    REQUIRE(b.value == 10.5f);
}

}

#endif
