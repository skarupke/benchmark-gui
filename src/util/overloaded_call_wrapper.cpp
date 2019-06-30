#include "util/overloaded_call_wrapper.hpp"


#ifndef DISABLE_GTEST

#include <gtest/gtest.h>


TEST(overloaded_call_wrapper, one_overload)
{
    struct IntCaller
    {
        int operator()(int a) const
        {
            return a + 5;
        }
    };

    IntCaller caller;
    using WrapperType = OverloadedCallWrapper<IntCaller>;
    WrapperType wrapper{caller};
    static_assert(WrapperType::IsCallable<int>);
    ASSERT_EQ(10, wrapper(5));
    static_assert(!WrapperType::IsCallable<std::string>);
}
TEST(overloaded_call_wrapper, two_overloads)
{
    struct IntCaller
    {
        int operator()(int a) const
        {
            return a + 5;
        }
    };
    struct IntMember
    {
        int i;
    };

    IntCaller caller;
    IntMember with_int{5};
    using WrapperType = OverloadedCallWrapper<IntCaller, int IntMember::*>;
    WrapperType wrapper{{caller, &IntMember::i}};
    static_assert(WrapperType::IsCallable<int>);
    ASSERT_EQ(10, wrapper(5));
    static_assert(WrapperType::IsCallable<IntMember>);
    ASSERT_EQ(5, wrapper(with_int));
    static_assert(!WrapperType::IsCallable<std::string>);
}

#endif
