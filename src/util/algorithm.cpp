#include "util/algorithm.hpp"


#ifndef DISABLE_TESTS
#include "test/include_test.hpp"

TEST(mismatch_ignoring, simple)
{
    auto is_space = [](char c){ return c == ' '; };
    std::string a = "abc";
    std::string b = "abc";
    auto mismatch = mismatch_ignoring(a.begin(), a.end(), b.begin(), b.end(), is_space);
    ASSERT_EQ(std::make_pair(a.end(), b.end()), mismatch);
    a = "ab c ";
    b = " ab c";
    mismatch = mismatch_ignoring(a.begin(), a.end(), b.begin(), b.end(), is_space);
    ASSERT_EQ(std::make_pair(a.end(), b.end()), mismatch);
    a = "a b c";
    b = "a bc";
    mismatch = mismatch_ignoring(a.begin(), a.end(), b.begin(), b.end(), is_space);
    ASSERT_EQ('b', *mismatch.first);
    ASSERT_EQ('b', *mismatch.second);
}

TEST(max_element_transformed, simple)
{
    std::vector<int> a = { 1, -2, 3, -4 };
    auto max2 = max_element_transformed(a.begin(), a.end(), [](int x){ return x * 2; });
    ASSERT_NE(a.end(), max2.first);
    ASSERT_EQ(3, *max2.first);
    ASSERT_EQ(6, max2.second);
    auto maxabs = max_element_transformed(a.begin(), a.end(), [](int x){ return std::abs(x); });
    ASSERT_NE(a.end(), maxabs.first);
    ASSERT_EQ(-4, *maxabs.first);
    ASSERT_EQ(4, maxabs.second);
}
TEST(max_element_transformed, equal)
{
    std::vector<int> a = { 1, 1 };
    auto max = max_element_transformed(a.begin(), a.end(), [](int x){ return x * 2; });
    ASSERT_EQ(a.begin(), max.first);
    ASSERT_EQ(1, *max.first);
    ASSERT_EQ(2, max.second);
}

TEST(partial_sort_pointer, simple)
{
    std::vector<int> a = { 5, 4, 7, 1, 2, 4, 9, 5, 3, 2 };
    std::array<int *, 4> sorted;
    auto end = partial_sort_pointer(a.begin(), a.end(), sorted.begin(), sorted.end());
    ASSERT_EQ(sorted.end(), end);
    ASSERT_EQ(1, *sorted[0]);
    ASSERT_EQ(2, *sorted[1]);
    ASSERT_EQ(2, *sorted[2]);
    ASSERT_EQ(3, *sorted[3]);
}

TEST(partial_sort_pointer, too_short)
{
    std::vector<int> a = { 5, 4, 7 };
    std::array<int *, 4> sorted;
    auto end = partial_sort_pointer(a.begin(), a.end(), sorted.begin(), sorted.end());
    ASSERT_EQ(sorted.end() - 1, end);
    ASSERT_EQ(4, *sorted[0]);
    ASSERT_EQ(5, *sorted[1]);
    ASSERT_EQ(7, *sorted[2]);
}

TEST(partial_sort_pointer, top_seven)
{
    std::vector<int> a = { 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    std::array<int *, 7> sorted;
    auto end = partial_sort_pointer(a.begin(), a.end(), sorted.begin(), sorted.end());
    ASSERT_EQ(sorted.end(), end);
    ASSERT_EQ(1, *sorted[0]);
    ASSERT_EQ(2, *sorted[1]);
    ASSERT_EQ(3, *sorted[2]);
    ASSERT_EQ(4, *sorted[3]);
    ASSERT_EQ(5, *sorted[4]);
    ASSERT_EQ(6, *sorted[5]);
    ASSERT_EQ(7, *sorted[6]);
}

#include "debug/assert.hpp"

template<size_t UnrollAmount, typename It, typename Func>
inline void unroll_loop_nonempty2(It begin, size_t iteration_count, Func && to_call)
{
    static_assert(UnrollAmount >= 1 && UnrollAmount <= 8, "Currently only support up to 8 loop unrollings");
    size_t loop_count = (iteration_count + (UnrollAmount - 1)) / UnrollAmount;
    size_t remainder_count = iteration_count % UnrollAmount;
    begin += remainder_count;
    switch(remainder_count)
    {
    case 0:
        do
        {
            begin += UnrollAmount;
            if constexpr (UnrollAmount >= 8)
                to_call(begin - 8);
            [[fallthrough]];
    case 7:
            if constexpr (UnrollAmount >= 7)
                to_call(begin - 7);
            [[fallthrough]];
    case 6:
            if constexpr (UnrollAmount >= 6)
                to_call(begin - 6);
            [[fallthrough]];
    case 5:
            if constexpr (UnrollAmount >= 5)
                to_call(begin - 5);
            [[fallthrough]];
    case 4:
            if constexpr (UnrollAmount >= 4)
                to_call(begin - 4);
            [[fallthrough]];
    case 3:
            if constexpr (UnrollAmount >= 3)
                to_call(begin - 3);
            [[fallthrough]];
    case 2:
            if constexpr (UnrollAmount >= 2)
                to_call(begin - 2);
            [[fallthrough]];
    case 1:
            to_call(begin - 1);
            --loop_count;
        }
        while(loop_count > 0);
    }
}

#include <numeric>

TEST(unroll_loop2, cases)
{
    std::vector<int> to_iterate(20);
    std::iota(to_iterate.begin(), to_iterate.end(), 1);
    int expected = std::accumulate(to_iterate.begin(), to_iterate.end(), 0);
    int loop_count = 0;
    auto loop_body = [&](auto to_add){ loop_count += *to_add; };
    unroll_loop_nonempty2<1>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    unroll_loop_nonempty2<2>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    unroll_loop_nonempty2<3>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    unroll_loop_nonempty2<4>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    unroll_loop_nonempty2<5>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    unroll_loop_nonempty2<6>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    unroll_loop_nonempty2<7>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    unroll_loop_nonempty2<8>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
}

/*TEST(int_convert, all_cases)
{
    auto add = [](int i)
    {
        return static_cast<unsigned int>(i) + static_cast<unsigned int>(1 << (sizeof(int) * 8 - 1));
    };
    auto with_xor = [](int i)
    {
        return static_cast<unsigned int>(i) ^ static_cast<unsigned int>(1 << (sizeof(int) * 8 - 1));
    };
    for (int i = std::numeric_limits<int>::lowest(); i < std::numeric_limits<int>::max(); ++i)
    {
        ASSERT_EQ(add(i), with_xor(i));
    }
    ASSERT_EQ(add(std::numeric_limits<int>::max()), with_xor(std::numeric_limits<int>::max()));
}*/

#endif

