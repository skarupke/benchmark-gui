#include "util/ska_sort2.hpp"


#ifndef DISABLE_GTEST

#include "test/include_test.hpp"
#include <random>

namespace
{

struct AmericanFlagSortSettings : detail::DefaultSortSettings
{
    template<typename>
    static constexpr std::ptrdiff_t InsertionSortUpperLimit = 1;
    static constexpr std::ptrdiff_t AmericanFlagSortUpperLimit = std::numeric_limits<std::ptrdiff_t>::max();
};
struct SkaByteSortSettings : detail::DefaultSortSettings
{
    template<typename>
    static constexpr std::ptrdiff_t InsertionSortUpperLimit = 1;
    static constexpr std::ptrdiff_t AmericanFlagSortUpperLimit = 1;
};
struct NoFasterCompareSettings : detail::DefaultSortSettings
{
    static constexpr bool UseFasterCompare = !detail::DefaultSortSettings::UseFasterCompare;
};

template<typename Container>
::testing::AssertionResult TestAllSkaSort2Combinations(const Container & container)
{
    Container copy = container;
    ska_sort2(copy.begin(), copy.end());
    if (!std::is_sorted(copy.begin(), copy.end()))
    {
        return ::testing::AssertionFailure() << "ska_sort2 didn't sort";
    }
    copy = container;
    detail::ska_sort2_with_settings<AmericanFlagSortSettings>(copy.begin(), copy.end());
    if (!std::is_sorted(copy.begin(), copy.end()))
    {
        return ::testing::AssertionFailure() << "ska_sort2 american flag sort didn't sort";
    }
    copy = container;
    detail::ska_sort2_with_settings<SkaByteSortSettings>(copy.begin(), copy.end());
    if (!std::is_sorted(copy.begin(), copy.end()))
    {
        return ::testing::AssertionFailure() << "ska_sort2 byte sort didn't sort";
    }
    copy = container;
    detail::ska_sort2_with_settings<NoFasterCompareSettings>(copy.begin(), copy.end());
    if (!std::is_sorted(copy.begin(), copy.end()))
    {
        return ::testing::AssertionFailure() << "ska_sort2 with faster compare didn't sort";
    }
    return ::testing::AssertionSuccess();
}

TEST(ska_sort2, bytes)
{
    std::vector<unsigned char> data = { 1, 3, 2, 50, 2, 3, 70, 3, 255, 2 };
    ASSERT_TRUE(TestAllSkaSort2Combinations(data));
}
TEST(ska_sort2, chars)
{
    std::vector<char> data = { 'a', 'z', 'b', 'r', 'w', 'v', 'x', 'u', ',', '4', '2', '7', 'W', '_' };
    ASSERT_TRUE(TestAllSkaSort2Combinations(data));
}
TEST(ska_sort2, unsigned_short)
{
    std::vector<unsigned short> data = { 1, 0, 65535, 16000, 16001, 16002, 16001, 16005, 16010, 16007, 10, 70, 3 };
    ASSERT_TRUE(TestAllSkaSort2Combinations(data));
}
TEST(ska_sort2, short)
{
    std::vector<short> data = { 1, 0, std::numeric_limits<short>::max(), -10, std::numeric_limits<short>::lowest(), 16000, -16000, 16001, 16002, 16001, 16005, 16010, 16007, 10, 70, 3 };
    ASSERT_TRUE(TestAllSkaSort2Combinations(data));
}
TEST(ska_sort2, bool)
{
    std::vector<short> data = { true, false, true, true, false, false, true, true, true, true, false };
    ASSERT_TRUE(TestAllSkaSort2Combinations(data));
}
TEST(ska_sort2, pair)
{
    std::vector<std::pair<short, int>> data =
    {
        { 1, 0 },
        { 1, 2 },
        { 1, -3 },
        { 10, 2 },
        { 1, -4 },
        { 10, 0 },
        { -10, 7 },
        { -10, -10 },
        { 10, 10 }
    };
    ASSERT_TRUE(TestAllSkaSort2Combinations(data));
}
TEST(ska_sort2, tuple)
{
    std::vector<std::tuple<bool, int, bool>> data = { std::tuple<bool, int, bool>{ true, 5, true }, std::tuple<bool, int, bool>{ true, 5, false }, std::tuple<bool, int, bool>{ false, 6, false }, std::tuple<bool, int, bool>{ true, 7, true }, std::tuple<bool, int, bool>{ true, 4, false }, std::tuple<bool, int, bool>{ false, 4, true }, std::tuple<bool, int, bool>{ false, 5, false } };
    ASSERT_TRUE(TestAllSkaSort2Combinations(data));
}
TEST(ska_sort2, tuple_single)
{
    std::vector<std::tuple<int>> data = { std::tuple<int>{ 5 }, std::tuple<int>{ -5 }, std::tuple<int>{ 6 }, std::tuple<int>{ 7 }, std::tuple<int>{ 4 }, std::tuple<int>{ 4 }, std::tuple<int>{ 5 } };
    ASSERT_TRUE(TestAllSkaSort2Combinations(data));
}
TEST(ska_sort2, optional)
{
    std::vector<std::optional<int>> to_sort;
    to_sort.emplace_back(5);
    to_sort.emplace_back(std::nullopt);
    to_sort.emplace_back(6);
    to_sort.emplace_back(-3);
    to_sort.emplace_back(2);
    to_sort.emplace_back(7);
    to_sort.emplace_back(std::nullopt);
    ASSERT_TRUE(TestAllSkaSort2Combinations(to_sort));
}

TEST(ska_sort2, optional_in_pair)
{
    std::vector<std::pair<std::optional<int>, int>> to_sort;
    to_sort.emplace_back(5, 5);
    to_sort.emplace_back(std::nullopt, 4);
    to_sort.emplace_back(std::nullopt, 3);
    to_sort.emplace_back(std::nullopt, 8);
    to_sort.emplace_back(5, 2);
    to_sort.emplace_back(5, 100);
    to_sort.emplace_back(-5, 100);
    to_sort.emplace_back(-5, 2);
    to_sort.emplace_back(-5, 3);
    ASSERT_TRUE(TestAllSkaSort2Combinations(to_sort));
}

TEST(ska_sort2, string)
{
    std::vector<std::string> to_sort =
    {
        "Hi",
        "There",
        "Hello",
        "World!",
        "Foo",
        "Bar",
        "Baz",
        "",
    };
    ASSERT_TRUE(TestAllSkaSort2Combinations(to_sort));
}
TEST(ska_sort2, string_unicode)
{
    std::vector<std::string> to_sort =
    {
        "Hi",
        "There",
        "précis"
        "Hello",
        "prepayment",
        "World!",
        "scans",
        "séances",
        "Foo",
        "Bar",
        "née"
        "Baz",
        "naive"
        "",
        "some",
        "more",
        "strings",
        "to",
        "get",
        "over",
        ",",
        "16",
        "or",
        "32",
        "or some",
        "limit",
        "like",
        "that",
        "10000",
        "20000",
        "foo",
        "pirate",
        "rhino",
        "city",
        "blanket",
        "balloon",
    };
    ASSERT_TRUE(TestAllSkaSort2Combinations(to_sort));
}

TEST(ska_sort2, string_unicode_direct_compare)
{
    auto comparer = SkaSorter<std::string>::SortAtIndex{1};
    ASSERT_EQ(comparer("née", "naive"), std::less<std::string>{}("née", "naive"));
    ASSERT_TRUE(comparer("aaa", "aaaaa"));
    auto at_17 = SkaSorter<std::string>::SortAtIndex{17};
    ASSERT_EQ(at_17("aaaaaaaaaaaaaaaaaaaab", "aaaaaaaaaaaaaaaaaaaabb"), std::less<std::string>{}("aaaaaaaaaaaaaaaaaaaab", "aaaaaaaaaaaaaaaaaaaabb"));
}

TEST(ska_sort2, string_bad_case)
{
    std::vector<std::string> to_sort;
    for (int i = 0; i < 20; ++i)
    {
        to_sort.push_back("a");
        for (int j = 0; j < i; ++j)
        {
            to_sort.back() += 'a';
        }
    }
    for (int i = 0; i < 20; ++i)
    {
        to_sort.push_back(to_sort[19]);
        for (int j = 20; j > i; --j)
        {
            to_sort.back() += 'b';
        }
    }
    ASSERT_TRUE(TestAllSkaSort2Combinations(to_sort));
}

TEST(ska_sort2, float)
{
    std::vector<float> to_sort = { 5, 6, 19, std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -4, 2, 5, 0, -55, 7, 1000, 23, 6, 8, 127, -128, -129, -256, 32768, -32769, -32768, 32767, 99, 1000000, -1000001, 0.1f, 2.5f, 17.8f, -12.4f, -0.0000002f, -0.0f, -777777777.7f, 444444444444.4f };
    ASSERT_TRUE(TestAllSkaSort2Combinations(to_sort));
}
static const std::vector<float> float_bug_input =
{
    -3305.68f, -2684.34f, -2234.62f, -1988.51f, -1965.32f, -1808.13f, -1800.42f, -1722.81f, -1704.51f, -1649.78f,
    -1591.76f, -1581.61f, -1566.55f, -1459.65f, -1382.87f, -1333.64f, -1319.45f, -1186.65f, -1098.14f, -1039.01f,
    -973.805f, -951.14f, -926.392f, -920.247f, -922.534f, -922.763f, -919.381f, -850.919f, -796.569f, -787.428f, -780.044f,
    -772.276f, -756.935f, -720.871f, -673.421f, -664.041f, -628.639f, -580.13f, -519.876f, -508.197f, -507.302f, -471.265f,
    -464.019f, -429.188f, -410.241f, -366.956f, -365.482f, -327.256f, -299.654f, -295.048f, -278.138f, -262.674f, -205.262f,
    -192.724f, -164.104f, -145.508f, -140.691f, -133.544f, -44.4212f, -20.0017f, -0.324011f, 19.3389f, 33.6625f, 79.592f,
    98.4721f, 111.029f, 142.54f, 158.674f, 166.794f, 183.275f, 187.558f, 206.012f, 217.915f, 225.862f, 240.096f, 248.95f,
    287.886f, 287.903f, 303.676f, 329.983f, 355.284f, 355.761f, 401.808f, 422.257f, 440.037f, 447.795f, 485.2f, 513.239f,
    530.438f, 540.15f, 551.101f, 591.771f, 604.096f, 627.521f, 643.663f, 644.669f, 729.3f, 740.218f, 747.142f, 754.765f,
    794.868f, 871.391f, 918.005f, 921.496f, 924.987f, 937.024f, 937.706f, 992.288f, 995.07f, 1000.17f, 1007.06f, 1062.04f,
    1069.21f, 1072.04f, 1162.66f, 1173.71f, 1180.34f, 1184.88f, 1249.75f, 1263.56f, 1298.19f, 1312.38f, 1365.59f, 1368.41f,
    1431.62f, 1462.83f, 1470.19f, 1508.33f
};
TEST(ska_sort2, float_bug)
{
    ASSERT_TRUE(TestAllSkaSort2Combinations(float_bug_input));
    std::vector<std::pair<bool, float>> in_pair;
    for (size_t i = 0; i < float_bug_input.size(); ++i)
    {
        in_pair.emplace_back(i < float_bug_input.size() / 2, float_bug_input[i]);
    }
    ASSERT_TRUE(TestAllSkaSort2Combinations(in_pair));
}
TEST(ska_sort2, float_compare)
{
    detail::FloatSorter<float>::CompareAsUint compare_positive;
    detail::FloatSorter<float>::CompareAsUintNegative compare_negative;
    for (size_t i = 1; i < float_bug_input.size(); ++i)
    {
        float first = float_bug_input[i - 1];
        float second = float_bug_input[i];
        if ((first < 0) != (second < 0))
            continue;
        if (first < 0)
        {
            ASSERT_TRUE(compare_negative(first, second) == (first < second));
            ASSERT_TRUE(compare_negative(second, first) == (second < first));
        }
        else
        {
            ASSERT_TRUE(compare_positive(first, second) == (first < second));
            ASSERT_TRUE(compare_positive(second, first) == (second < first));
        }
    }
}

TEST(ska_sort2, double)
{
    std::vector<double> to_sort = { 5, 6, 19, std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -4, 2, 5, 0, -55, 7, 1000, 23, 6, 8, 127, -128, -129, -256, 32768, -32769, -32768, 32767, 99, 1000000, -1000001, 0.1, 2.5, 17.8, -12.4, -0.0000002, -0.0, -777777777.7, 444444444444.4, 1.0 / 3.0, -1.0 / 3.0 };
    ASSERT_TRUE(TestAllSkaSort2Combinations(to_sort));
}

TEST(ska_sort2, long_double)
{
    std::vector<long double> to_sort = { 5, 6, 19, std::numeric_limits<long double>::infinity(), -std::numeric_limits<long double>::infinity(), -4, 2, 5, 0, -55, 7, 1000, 23, 6, 8, 127, -128, -129, -256, 32768, -32769, -32768, 32767, 99, 1000000, -1000001, 0.1, 2.5, 17.8, -12.4, -0.0000002, -0.0, -777777777.7, 444444444444.4, static_cast<long double>(1.0) / static_cast<double>(3.0), -static_cast<long double>(1.0) / static_cast<long double>(3.0) };
    ASSERT_TRUE(TestAllSkaSort2Combinations(to_sort));
}

TEST(ska_sort2, DISABLED_float_backwards)
{
    auto to_unsigned_or_bool = [](float f)
    {
        union
        {
            float f;
            std::uint32_t u;
        } as_union = { f };
        std::uint32_t sign_bit = -std::int32_t(as_union.u >> 31);
        return as_union.u ^ (sign_bit | 0x80000000);
    };
    auto to_unsigned_or_bool_backwards = [](float f)
    {
        union
        {
            float f;
            std::uint32_t u;
        } as_union = { f };
        //std::uint32_t sign_bit = -std::int32_t(as_union.u >> 31);
        return as_union.u;// ^ 0x7fffffff;
    };
    std::vector<float> to_sort = { 5, 6, 19, std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -4, 2, 5, 0, -55, 7, 1000, 23, 6, 8, 127, -128, -129, -256, 32768, -32769, -32768, 32767, 99, 1000000, -1000001, 0.1f, 2.5f, 17.8f, -12.4f, -0.0000002f, -0.0f, -777777777.7f, 444444444444.4f };
    std::vector<float> copy = to_sort;
    std::vector<float> copy_backwards = to_sort;
    std::sort(to_sort.begin(), to_sort.end());
    std::sort(copy.begin(), copy.end(), [&](float a, float b)
    {
        return to_unsigned_or_bool(a) < to_unsigned_or_bool(b);
    });
    std::sort(copy_backwards.rbegin(), copy_backwards.rend(), [&](float a, float b)
    {
        return to_unsigned_or_bool_backwards(a) < to_unsigned_or_bool_backwards(b);
    });
    ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
    ASSERT_TRUE(std::is_sorted(copy.begin(), copy.end()));
    ASSERT_TRUE(std::is_sorted(copy_backwards.begin(), copy_backwards.end()));
}

// todo: enable
#if 1

struct ThrowingType
{
    operator int() const
    {
        throw 5;
    }
};

TEST(ska_sort2, variant)
{
    std::vector<std::variant<int, std::string, float>> to_sort;
    to_sort.emplace_back(5);
    to_sort.emplace_back(0);
    to_sort.emplace_back("foo");
    to_sort.emplace_back("foo");
    to_sort.emplace_back("foo");
    to_sort.emplace_back("bar");
    to_sort.emplace_back(1.0f);
    to_sort.emplace_back(-2.0f);
    to_sort.emplace_back(-1);
    to_sort.emplace_back("baz3");
    to_sort.emplace_back("baz");
    to_sort.emplace_back("baz2");
    to_sort.emplace_back(7.0f);
    ASSERT_THROW(to_sort[2].emplace<0>(ThrowingType()), int);
    ASSERT_TRUE(to_sort[2].valueless_by_exception());
    ASSERT_THROW(to_sort[3].emplace<0>(ThrowingType()), int);
    ASSERT_TRUE(to_sort[3].valueless_by_exception());
    ASSERT_TRUE(TestAllSkaSort2Combinations(to_sort));
}

TEST(ska_sort2, variant_in_pair)
{
    std::vector<std::pair<std::variant<int, float, std::string>, int>> to_sort;
    to_sort.emplace_back(5, 5);
    to_sort.emplace_back(0, 4);
    to_sort.emplace_back("foo", 2);
    to_sort.emplace_back("foo", 1);
    to_sort.emplace_back("foo", 3);
    to_sort.emplace_back(5, 3);
    to_sort.emplace_back(5, 1);
    to_sort.emplace_back(0, 8);
    to_sort.emplace_back(0, 2);
    to_sort.emplace_back(1.0f, 2);
    to_sort.emplace_back(1.0f, 21);
    to_sort.emplace_back(-2.0f, 3);
    to_sort.emplace_back(-2.0f, 4);
    to_sort.emplace_back(1.0f, -3);
    to_sort.emplace_back(1.0f, 3);
    to_sort.emplace_back(-2.0f, 2);
    ASSERT_THROW(to_sort[2].first.emplace<0>(ThrowingType()), int);
    ASSERT_TRUE(to_sort[2].first.valueless_by_exception());
    ASSERT_THROW(to_sort[3].first.emplace<0>(ThrowingType()), int);
    ASSERT_TRUE(to_sort[3].first.valueless_by_exception());
    ASSERT_THROW(to_sort[4].first.emplace<0>(ThrowingType()), int);
    ASSERT_TRUE(to_sort[4].first.valueless_by_exception());
    ASSERT_TRUE(TestAllSkaSort2Combinations(to_sort));
}

#endif

}
#endif
