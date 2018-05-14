#include "util/functional.hpp"

#ifndef DISABLE_TESTS
#include "test/include_test.hpp"
TEST(functional, member_function_functor)
{
    struct S
    {
        int foo()
        {
            return 5;
        }
        int bar() const
        {
            return 10;
        }
    };
    auto ptr_a = to_functor(&S::foo);
    auto ptr_b = to_functor(&S::bar);
    S s;
    ASSERT_EQ(5, ptr_a(s));
    ASSERT_EQ(10, ptr_b(s));
    const S s2{};
    ASSERT_EQ(10, ptr_b(s2));
}

TEST(functional, member_ptr)
{
    struct S
    {
        int foo = 5;
    };
    auto ptr_a = to_functor(&S::foo);
    S s;
    ASSERT_EQ(5, ptr_a(s));
}

#endif
