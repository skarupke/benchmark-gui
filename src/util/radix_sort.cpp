//          Copyright Malte Skarupke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See http://www.boost.org/LICENSE_1_0.txt)

#include "util/radix_sort.hpp"
#include <deque>
#include <optional>

#define FULL_TESTS_SLOW_COMPILE_TIME

template<typename It, typename OutIt, typename ExtractKey>
void counting_sort(It begin, It end, OutIt out_begin, ExtractKey && extract_key)
{
    detail::counting_sort_impl(begin, end, out_begin, extract_key);
}
template<typename It, typename OutIt>
void counting_sort(It begin, It end, OutIt out_begin)
{
    using detail::to_unsigned_or_bool;
    detail::counting_sort_impl(begin, end, out_begin, [](auto && a){ return to_unsigned_or_bool(a); });
}

template<typename It, typename OutIt, typename ExtractKey>
bool radix_sort(It begin, It end, OutIt buffer_begin, ExtractKey && extract_key)
{
    return detail::RadixSorter<typename std::result_of<ExtractKey(decltype(*begin))>::type>::sort(begin, end, buffer_begin, extract_key);
}
template<typename It, typename OutIt>
bool radix_sort(It begin, It end, OutIt buffer_begin)
{
    return detail::RadixSorter<decltype(*begin)>::sort(begin, end, buffer_begin, detail::IdentityFunctor());
}

struct NoStdSortFallbackSetting : detail::DefaultSortSettings
{
    template<typename>
    static constexpr std::ptrdiff_t InsertionSortUpperLimit = 1;
    static constexpr std::ptrdiff_t AmericanFlagSortUpperLimit = 1;
};

template<bool DoThreeWaySwap = detail::DefaultSortSettings::ThreeWaySwap>
struct AmericanFlagSortOnlySetting : detail::DefaultSortSettings
{
    static constexpr std::ptrdiff_t AmericanFlagSortUpperLimit = std::numeric_limits<std::ptrdiff_t>::max();
    static constexpr bool ThreeWaySwap = DoThreeWaySwap;
};

template<bool DoThreeWaySwap = detail::DefaultSortSettings::ThreeWaySwap>
struct AmericanFlagSortOnlySettingTest : NoStdSortFallbackSetting
{
    static constexpr std::ptrdiff_t AmericanFlagSortUpperLimit = std::numeric_limits<std::ptrdiff_t>::max();
    static constexpr bool ThreeWaySwap = DoThreeWaySwap;
};

template<std::ptrdiff_t InsertionSortSize,
         std::ptrdiff_t AmericanFlagSortSize = detail::DefaultSortSettings::AmericanFlagSortUpperLimit,
         bool DoThreeWaySwap = detail::DefaultSortSettings::ThreeWaySwap
         , size_t FirstLoopUnrollAmountParam = detail::DefaultSortSettings::FirstLoopUnrollAmount
         , size_t SecondLoopUnrollAmountParam = detail::DefaultSortSettings::SecondLoopUnrollAmount
         , bool UseIndexSortInsteadOfInsertionSort = detail::DefaultSortSettings::UseIndexSort
         , bool UseFasterComparison = detail::DefaultSortSettings::UseFasterCompare>
struct ConfigurableSettings : detail::DefaultSortSettings
{
    template<typename>
    static constexpr std::ptrdiff_t InsertionSortUpperLimit = InsertionSortSize;
    static constexpr bool ThreeWaySwap = DoThreeWaySwap;
    static constexpr std::ptrdiff_t AmericanFlagSortUpperLimit = AmericanFlagSortSize;
    static constexpr size_t FirstLoopUnrollAmount = FirstLoopUnrollAmountParam;
    static constexpr size_t SecondLoopUnrollAmount = SecondLoopUnrollAmountParam;
    static constexpr bool UseIndexSort = UseIndexSortInsteadOfInsertionSort;
    static constexpr bool UseFasterCompare = UseFasterComparison;
};

template<typename It, typename ExtractKey>
static void inplace_radix_sort(It begin, It end, ExtractKey && extract_key)
{
    detail::inplace_radix_sort<NoStdSortFallbackSetting>(begin, end, extract_key);
}

template<typename It>
static void inplace_radix_sort(It begin, It end)
{
    inplace_radix_sort(begin, end, detail::IdentityFunctor());
}

template<typename It, typename ExtractKey>
static void american_flag_sort(It begin, It end, ExtractKey && extract_key)
{
    detail::inplace_radix_sort<AmericanFlagSortOnlySetting<detail::DefaultSortSettings::ThreeWaySwap>>(begin, end, extract_key);
}
template<bool ThreeWaySwap = detail::DefaultSortSettings::ThreeWaySwap, typename It, typename ExtractKey>
static void american_flag_sort_test(It begin, It end, ExtractKey && extract_key)
{
    detail::inplace_radix_sort<AmericanFlagSortOnlySettingTest<ThreeWaySwap>>(begin, end, extract_key);
}

template<typename It>
static void american_flag_sort(It begin, It end)
{
    american_flag_sort(begin, end, detail::IdentityFunctor());
}

template<typename SortSettings, typename It, typename ExtractKey>
static bool counting_sort_ping_pong(It begin, It end, It buffer_begin, ExtractKey && extract_key)
{
    return detail::ping_pong_ska_sort<SortSettings>(begin, end, buffer_begin, extract_key);
}
template<typename SortSettings, typename It>
static bool counting_sort_ping_pong(It begin, It end, It buffer_begin)
{
    return counting_sort_ping_pong<SortSettings>(begin, end, buffer_begin, detail::IdentityFunctor());
}

template<std::ptrdiff_t InsertionSortUpperLimit = detail::DefaultSortSettings::InsertionSortUpperLimit<int>, typename It, typename ExtractKey>
static bool counting_sort_ping_pong_configurable(It begin, It end, It buffer_begin, ExtractKey && extract_key)
{
    return counting_sort_ping_pong<ConfigurableSettings<InsertionSortUpperLimit>>(begin, end, buffer_begin, extract_key);
}
template<std::ptrdiff_t InsertionSortUpperLimit = detail::DefaultSortSettings::InsertionSortUpperLimit<int>, typename It>
static bool counting_sort_ping_pong_configurable(It begin, It end, It buffer_begin)
{
    return counting_sort_ping_pong_configurable<InsertionSortUpperLimit>(begin, end, buffer_begin, detail::IdentityFunctor());
}

template<typename It, typename ExtractKey>
static bool counting_sort_ping_pong_test(It begin, It end, It buffer_begin, ExtractKey && extract_key)
{
    return detail::ping_pong_ska_sort<NoStdSortFallbackSetting>(begin, end, buffer_begin, extract_key);
}

template<std::ptrdiff_t InsertionSortUpperLimit = detail::DefaultSortSettings::InsertionSortUpperLimit<int>, std::ptrdiff_t AmericanFlagSortUpperLimit = detail::DefaultSortSettings::AmericanFlagSortUpperLimit, typename It, typename ExtractKey>
void ska_sort_configurable(It begin, It end, ExtractKey && extract_key)
{
    detail::inplace_radix_sort<ConfigurableSettings<InsertionSortUpperLimit, AmericanFlagSortUpperLimit>>(begin, end, extract_key);
}

template<std::ptrdiff_t InsertionSortUpperLimit = detail::DefaultSortSettings::InsertionSortUpperLimit<int>, std::ptrdiff_t AmericanFlagSortUpperLimit = detail::DefaultSortSettings::AmericanFlagSortUpperLimit, typename It>
void ska_sort_configurable(It begin, It end)
{
    ska_sort_configurable<InsertionSortUpperLimit, AmericanFlagSortUpperLimit>(begin, end, detail::IdentityFunctor());
}

template<std::ptrdiff_t InsertionSortUpperLimit = detail::DefaultSortSettings::InsertionSortUpperLimit<int>, bool ThreeWaySwap = detail::DefaultSortSettings::ThreeWaySwap, typename It, typename ExtractKey>
void ska_byte_sort_configurable(It begin, It end, ExtractKey && extract_key)
{
    detail::inplace_radix_sort<ConfigurableSettings<InsertionSortUpperLimit, 1, ThreeWaySwap>>(begin, end, extract_key);
}
template<std::ptrdiff_t InsertionSortUpperLimit = detail::DefaultSortSettings::InsertionSortUpperLimit<int>, bool ThreeWaySwap = detail::DefaultSortSettings::ThreeWaySwap, typename It>
void ska_byte_sort_configurable(It begin, It end)
{
    ska_byte_sort_configurable<InsertionSortUpperLimit, ThreeWaySwap>(begin, end, detail::IdentityFunctor());
}
template<std::ptrdiff_t InsertionSortUpperLimit = detail::DefaultSortSettings::InsertionSortUpperLimit<int>, bool ThreeWaySwap = detail::DefaultSortSettings::ThreeWaySwap, typename It, typename ExtractKey>
void american_flag_sort_configurable(It begin, It end, ExtractKey && extract_key)
{
    detail::inplace_radix_sort<ConfigurableSettings<InsertionSortUpperLimit, std::numeric_limits<std::ptrdiff_t>::max(), ThreeWaySwap>>(begin, end, extract_key);
}
template<std::ptrdiff_t InsertionSortUpperLimit = detail::DefaultSortSettings::InsertionSortUpperLimit<int>, bool ThreeWaySwap = detail::DefaultSortSettings::ThreeWaySwap, typename It>
void american_flag_sort_configurable(It begin, It end)
{
    american_flag_sort_configurable<InsertionSortUpperLimit, ThreeWaySwap>(begin, end, detail::IdentityFunctor());
}

#ifndef DISABLE_GTEST

#include <vector>
#include <gtest/gtest.h>
#include <random>

template<typename ExtractKey>
struct CompareWithExtractKey
{
    ExtractKey & extract_key;

    template<typename T>
    bool operator()(const T & l, const T & r) const
    {
        return extract_key(l) < extract_key(r);
    }
    template<typename T>
    bool operator()(T & l, T & r) const
    {
        return extract_key(l) < extract_key(r);
    }
};
template<typename T, typename Shuffle, typename ExtractKey>
::testing::AssertionResult TestAllSorts(T & container, Shuffle && shuffle, ExtractKey && extract_key)
{
    T copy;
    copy.resize(container.size());

    auto is_sorted = [&](auto && to_check)
    {
        return std::is_sorted(to_check.begin(), to_check.end(), CompareWithExtractKey<std::remove_reference_t<ExtractKey>>{extract_key});
    };

    shuffle(container);
    if (radix_sort(container.begin(), container.end(), copy.begin(), extract_key))
    {
        if (!is_sorted(copy))
            return ::testing::AssertionFailure() << "radix sort failed";
        copy.swap(container);
    }
    else if (!is_sorted(container))
        return ::testing::AssertionFailure() << "radix sort failed";

    shuffle(container);
    if (ska_sort_ping_pong(container.begin(), container.end(), copy.begin(), extract_key))
    {
        if (!is_sorted(copy))
            return ::testing::AssertionFailure() << "ska_sort_ping_pong failed";
        copy.swap(container);
    }
    else if (!is_sorted(container))
        return ::testing::AssertionFailure() << "ska_sort_ping_pong failed";

    std::deque<typename T::value_type> copy_of_different_type;
    copy_of_different_type.resize(container.size());
    shuffle(container);
    if (ska_sort_ping_pong(container.begin(), container.end(), copy_of_different_type.begin(), extract_key))
    {
        if (!is_sorted(copy_of_different_type))
            return ::testing::AssertionFailure() << "radix sort failed";
        container.clear();
        for (auto & elem : copy_of_different_type)
            container.push_back(std::move(elem));
    }
    else if (!is_sorted(container))
        return ::testing::AssertionFailure() << "radix sort failed";

    shuffle(container);
    if (counting_sort_ping_pong_test(container.begin(), container.end(), copy.begin(), extract_key))
    {
        if (!is_sorted(copy))
            return ::testing::AssertionFailure() << "counting_sort_ping_pong failed";
        copy.swap(container);
    }
    else if (!is_sorted(container))
        return ::testing::AssertionFailure() << "counting_sort_ping_pong failed";

    shuffle(container);
    if (counting_sort_ping_pong_configurable<1>(container.begin(), container.end(), copy.begin(), extract_key))
    {
        if (!is_sorted(copy))
            return ::testing::AssertionFailure() << "counting_sort_ping_pong_configurable failed";
        copy.swap(container);
    }
    else if (!is_sorted(container))
        return ::testing::AssertionFailure() << "counting_sort_ping_pong_configurable failed";

    shuffle(container);
    inplace_radix_sort(container.begin(), container.end(), extract_key);
    if (!is_sorted(container))
        return ::testing::AssertionFailure() << "inplace_radix_sort failed";

    shuffle(container);
    american_flag_sort_test(container.begin(), container.end(), extract_key);
    if (!is_sorted(container))
        return ::testing::AssertionFailure() << "american_flag_sort failed";

    shuffle(container);
    ska_byte_sort_configurable<1>(container.begin(), container.end(), extract_key);
    if (!is_sorted(container))
        return ::testing::AssertionFailure() << "ska_byte_sort failed";

    shuffle(container);
    ska_byte_sort_configurable<1, true>(container.begin(), container.end(), extract_key);
    if (!is_sorted(container))
        return ::testing::AssertionFailure() << "ska_byte_sort with three way swap failed";

    shuffle(container);
    ska_sort(container.begin(), container.end(), extract_key);
    if (!is_sorted(container))
        return ::testing::AssertionFailure() << "ska_sort failed";

    shuffle(container);
    ska_sort_configurable<1>(container.begin(), container.end(), extract_key);
    if (!is_sorted(container))
        return ::testing::AssertionFailure() << "ska_sort_configurable failed";

    shuffle(container);
    detail::insertion_sort(container.begin(), container.end(), [&](auto && l, auto && r)
    {
        return extract_key(l) < extract_key(r);
    });
    if (!is_sorted(container))
        return ::testing::AssertionFailure() << "insertion_sort failed";

    return ::testing::AssertionSuccess();
}

template<typename T>
void EnsurePermutation(T & container, std::vector<size_t> permutation)
{
    using std::swap;
    for (size_t i = 0, end = container.size(); i < end;)
    {
        size_t desired_position = permutation[i];
        if (desired_position == i)
            ++i;
        else
        {
            swap(container[i], container[desired_position]);
            swap(permutation[i], permutation[desired_position]);
        }
    }
}

template<typename T, typename ExtractKey>
::testing::AssertionResult TestAllSorts(T & container, ExtractKey && extract_key)
{
    std::vector<std::pair<typename T::value_type, size_t>> get_sorted_permutation;
    get_sorted_permutation.reserve(container.size());
    for (size_t i = 0, end = container.size(); i < end; ++i)
    {
        get_sorted_permutation.emplace_back(std::move(container[i]), i);
    }
    container.clear();
    std::sort(get_sorted_permutation.begin(), get_sorted_permutation.end(), [&](auto && l, auto && r)
    {
        return extract_key(l.first) < extract_key(r.first);
    });
    std::vector<size_t> permutation;
    permutation.reserve(container.size());
    for (auto & pair : get_sorted_permutation)
    {
        container.push_back(std::move(pair.first));
        permutation.push_back(pair.second);
    }
    return TestAllSorts(container, [&](T & to_shuffle)
    {
        EnsurePermutation(to_shuffle, permutation);
    }, extract_key);
}
template<typename T>
::testing::AssertionResult TestAllSorts(T & container)
{
    return TestAllSorts(container, detail::IdentityFunctor());
}


template<typename T, typename ExtractKey>
::testing::AssertionResult TestVariableSizeSorts(T & container, ExtractKey && extract_key)
{
    T copy;
    copy.resize(container.size());

    auto is_sorted = [&](T & to_check)
    {
        return std::is_sorted(to_check.begin(), to_check.end(), CompareWithExtractKey<std::remove_reference_t<ExtractKey>>{extract_key});
    };

    detail::IdentityFunctor identity;
    OverloadedCallWrapper<ExtractKey, detail::IdentityFunctor> wrapper{{extract_key, identity}};
    std::mt19937_64 randomness(5);
    inplace_radix_sort(container.begin(), container.end(), wrapper);
    if (!is_sorted(container))
        return ::testing::AssertionFailure() << "inplace_radix_sort failed";

    std::shuffle(container.begin(), container.end(), randomness);
    american_flag_sort(container.begin(), container.end(), wrapper);
    if (!is_sorted(container))
        return ::testing::AssertionFailure() << "american_flag_sort failed";

    std::shuffle(container.begin(), container.end(), randomness);
    ska_sort(container.begin(), container.end(), extract_key);
    if (!is_sorted(container))
        return ::testing::AssertionFailure() << "ska_sort failed";

    return ::testing::AssertionSuccess();
}
template<typename T>
::testing::AssertionResult TestVariableSizeSorts(T & container)
{
    return TestVariableSizeSorts(container, detail::IdentityFunctor());
}

#ifdef FUZZER_BUILD
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t * data, std::size_t size)
{
    const int * as_int = reinterpret_cast<const int*>(data);
    std::vector<int> to_sort(as_int, as_int + size / 4);
    ::testing::AssertionResult result = TestAllSorts(to_sort);
    if (!result)
    {
        std::cout << result.message() << std::endl;
        __builtin_trap();
    }
    return 0;
}
#endif

TEST(unroll_loop, cases)
{
    std::vector<int> to_iterate(20);
    std::iota(to_iterate.begin(), to_iterate.end(), 1);
    int expected = std::accumulate(to_iterate.begin(), to_iterate.end(), 0);
    int loop_count = 0;
    auto loop_body = [&](auto to_add){ loop_count += *to_add; };
    detail::unroll_loop_nonempty<1>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    detail::unroll_loop_nonempty<2>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    detail::unroll_loop_nonempty<3>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    detail::unroll_loop_nonempty<4>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    detail::unroll_loop_nonempty<5>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    detail::unroll_loop_nonempty<6>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    detail::unroll_loop_nonempty<7>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
    loop_count = 0;
    detail::unroll_loop_nonempty<8>(to_iterate.begin(), to_iterate.size(), loop_body);
    ASSERT_EQ(loop_count, expected);
}

TEST(counting_sort, simple)
{
    std::vector<uint8_t> to_sort = { 5, 6, 19, 2, 5, 0, 7, 23, 6, 8, 99 };
    std::vector<uint8_t> result(to_sort.size());
    counting_sort(to_sort.begin(), to_sort.end(), result.begin(), [](auto i){ return i; });
    std::sort(to_sort.begin(), to_sort.end());
    ASSERT_EQ(to_sort, result);
}

TEST(counting_sort, string)
{
    std::string to_sort = "Hello, World!";
    std::string result = to_sort;
    counting_sort(to_sort.begin(), to_sort.end(), result.begin());
    std::sort(to_sort.begin(), to_sort.end());
    ASSERT_EQ(to_sort, result);
}

TEST(sort, uint8)
{
    std::vector<uint8_t> to_sort = { 5, 6, 19, 2, 5, 0, 7, 23, 6, 255, 8, 99 };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, uint8_256_items)
{
    std::vector<uint8_t> to_sort(256, 255);
    to_sort.back() = 254;
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, int8)
{
    std::vector<int8_t> to_sort = { 5, 6, 19, -4, 2, 5, 0, -55, 7, 23, 6, 8, 127, -128, 99 };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, text)
{
    std::string to_sort = "Hello, World!";
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, u16string)
{
    std::u16string to_sort = u"Hello, World!";
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, u32string)
{
    std::u32string to_sort = U"Hello, World!";
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, int16)
{
    std::vector<int16_t> to_sort = { 5, 6, 19, -4, 2, 5, 0, -55, 7, 1000, 23, 6, 8, 127, -128, -129, -256, -32768, 32767, 99 };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, uint16)
{
    std::vector<uint16_t> to_sort = { 5, 6, 19, 2, 5, 7, 0, 23, 6, 256, 255, 8, 99, 1024, 65535, 65534 };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, int32)
{
    std::vector<int32_t> to_sort = { 5, 6, 19, -4, 2, 5, 0, -55, 7, 1000, 23, 6, 8, 127, -128, -129, -256, 32768, -32769, -32768, 32767, 99, 1000000, -1000001, std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max() - 1, std::numeric_limits<int>::lowest() + 1 };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, uint32)
{
    std::vector<uint32_t> to_sort = { 5, 6, 19, 2, 5, 7, 0, 23, 6, 256, 255, 8, 99, 1024, 65536, 65535, 65534, 1000000, std::numeric_limits<unsigned int>::max() };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, int64)
{
    std::vector<int64_t> to_sort = { 5, 6, 19, std::numeric_limits<std::int32_t>::lowest() + 1, std::numeric_limits<ino64_t>::lowest(), -1000000000000, 1000000000000, std::numeric_limits<int32_t>::max(), std::numeric_limits<int64_t>::max(), -4, 2, 5, 0, -55, 7, 1000, 23, 6, 8, 127, -128, -129, -256, 32768, -32769, -32768, 32767, 99, 1000000, -1000001, std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max() - 1, std::numeric_limits<int>::lowest() + 1 };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, uint64)
{
    std::vector<uint64_t> to_sort = { 5, 6, 19, 2, 5, 7, 0, std::numeric_limits<uint32_t>::max() + 1, 1000000000000, std::numeric_limits<uint64_t>::max(), 23, 6, 256, 255, 8, 99, 1024, 65536, 65535, 65534, 1000000, std::numeric_limits<unsigned int>::max() };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, float)
{
    std::vector<float> to_sort = { 5, 6, 19, std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -4, 2, 5, 0, -55, 7, 1000, 23, 6, 8, 127, -128, -129, -256, 32768, -32769, -32768, 32767, 99, 1000000, -1000001, 0.1f, 2.5f, 17.8f, -12.4f, -0.0000002f, -0.0f, -777777777.7f, 444444444444.4f };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, double)
{
    std::vector<double> to_sort = { 5, 6, 19, std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -4, 2, 5, 0, -55, 7, 1000, 23, 6, 8, 127, -128, -129, -256, 32768, -32769, -32768, 32767, 99, 1000000, -1000001, 0.1, 2.5, 17.8, -12.4, -0.0000002, -0.0, -777777777.7, 444444444444.4 };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, pair)
{
    std::vector<std::pair<int, bool>> to_sort = { { 5, true }, { 5, false }, { 6, false }, { 7, true }, { 4, false }, { 4, true } };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, pair_other_direction)
{
    std::vector<std::pair<bool, int>> to_sort = { { true, 5 }, { false, 5 }, { false, 6 }, { true, 7 }, { false, 4 }, { true, 4 } };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, tuple)
{
    std::vector<std::tuple<bool, int, bool>> to_sort = { std::tuple<bool, int, bool>{ true, 5, true }, std::tuple<bool, int, bool>{ true, 5, false }, std::tuple<bool, int, bool>{ false, 6, false }, std::tuple<bool, int, bool>{ true, 7, true }, std::tuple<bool, int, bool>{ true, 4, false }, std::tuple<bool, int, bool>{ false, 4, true }, std::tuple<bool, int, bool>{ false, 5, false } };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, tuple_single)
{
    std::vector<std::tuple<int>> to_sort = { std::tuple<int>{ 5 }, std::tuple<int>{ -5 }, std::tuple<int>{ 6 }, std::tuple<int>{ 7 }, std::tuple<int>{ 4 }, std::tuple<int>{ 4 }, std::tuple<int>{ 5 } };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, reference)
{
    std::vector<int> to_sort = { 6, 5, 4, 3, 2, 1 };
    ASSERT_TRUE(TestAllSorts(to_sort, [](int & i) -> int & { return i; }));
}
TEST(sort, pair_reference)
{
    std::vector<std::pair<int, bool>> to_sort = { { 5, true }, { 5, false }, { 6, false }, { 7, true }, { 4, false }, { 4, true } };
    ASSERT_TRUE(TestAllSorts(to_sort, [](auto & i) -> decltype(auto) { return i; }));
}
TEST(sort, tuple_reference)
{
    std::vector<std::tuple<bool, int, bool>> to_sort = { std::tuple<bool, int, bool>{ true, 5, true }, std::tuple<bool, int, bool>{ true, 5, false }, std::tuple<bool, int, bool>{ false, 6, false }, std::tuple<bool, int, bool>{ true, 7, true }, std::tuple<bool, int, bool>{ true, 4, false }, std::tuple<bool, int, bool>{ false, 4, true }, std::tuple<bool, int, bool>{ false, 5, false } };
    ASSERT_TRUE(TestAllSorts(to_sort, [](auto & i) -> decltype(auto) { return i; }));
}
TEST(sort, std_array)
{
    std::vector<std::array<float, 4>> to_sort = { {{ 1.0f, 2.0f, 3.0f, 4.0f }}, {{ 0.0f, 3.0f, 4.0f, 5.0f }}, {{ 1.0f, 1.5f, 2.0f, 2.5f }}, {{ 1.0f, 2.0f, 2.5f, 4.0f }}, {{ 1.0f, 2.0f, 2.5f, 3.5f }}, {{ 0.0f, 3.0f, 4.5f, 4.5f }} };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
TEST(sort, move_only)
{
    std::vector<std::unique_ptr<int>> to_sort;
    to_sort.push_back(std::make_unique<int>(5));
    to_sort.push_back(std::make_unique<int>(0));
    to_sort.push_back(std::make_unique<int>(1234567));
    to_sort.push_back(std::make_unique<int>(-1000));
    std::vector<int> sorted;
    sorted.reserve(to_sort.size());
    for (const std::unique_ptr<int> & i : to_sort)
        sorted.push_back(*i);
    std::sort(sorted.begin(), sorted.end());
    ASSERT_TRUE(TestAllSorts(to_sort, [](auto & i){ return *i; }));
}

TEST(sort, vector_bool)
{
    std::vector<bool> to_sort = { true, false, true, true, false, true, true, true, false, true, false, false };
    ASSERT_TRUE(TestAllSorts(to_sort));
}


TEST(sort, error_case)
{
    std::vector<int8_t> data = { 46, 7, 33, -78, -114, -78, 33, 82 };
    ASSERT_TRUE(TestAllSorts(data));
}

TEST(sort, another_error_case)
{
    std::vector<int8_t> data = { -104, 50, 108, 105, 112, 53, 47, 102 };
    ASSERT_TRUE(TestAllSorts(data));
}
TEST(sort, two_extract_keys)
{
    struct WithInt
    {
        int i = 0;
        bool operator<(const WithInt & other) const
        {
            return i < other.i;
        }
    };
    struct WithArray
    {
        std::array<WithInt, 2> ints;
        bool operator<(const WithArray & other) const
        {
            return ints < other.ints;
        }
    };

    std::vector<WithArray> data;
    data.push_back(WithArray{{{{1}, {5}}}});
    data.push_back(WithArray{{{{1}, {-5}}}});
    data.push_back(WithArray{{{{-1}, {5}}}});
    data.push_back(WithArray{{{{-1}, {-5}}}});
    data.push_back(WithArray{{{{5}, {1}}}});
    data.push_back(WithArray{{{{-1}, {100}}}});
    data.push_back(WithArray{{{{100}, {-1}}}});
    data.push_back(WithArray{{{{100}, {20}}}});
    data.push_back(WithArray{{{{100}, {-20}}}});
    ska_sort(data.begin(), data.end(), &WithArray::ints, &WithInt::i);
    ASSERT_TRUE(std::is_sorted(data.begin(), data.end()));
}
TEST(sort, two_extract_keys_from_variant)
{
    struct WithInt
    {
        int i = 0;
        bool operator<(const WithInt & other) const
        {
            return i < other.i;
        }
    };
    struct WithVariant
    {
        std::variant<WithInt, int> ints;
        bool operator<(const WithVariant & other) const
        {
            return ints < other.ints;
        }
    };

    std::vector<WithVariant> data;
    data.push_back(WithVariant{{1}});
    data.push_back(WithVariant{WithInt{1}});
    data.push_back(WithVariant{WithInt{-1}});
    data.push_back(WithVariant{WithInt{-10}});
    data.push_back(WithVariant{WithInt{10}});
    data.push_back(WithVariant{{-1}});
    data.push_back(WithVariant{{10}});
    data.push_back(WithVariant{{-10}});
    ska_sort(data.begin(), data.end(), &WithVariant::ints, &WithInt::i);
    ASSERT_TRUE(std::is_sorted(data.begin(), data.end()));
}

#ifdef FULL_TESTS_SLOW_COMPILE_TIME
TEST(sort, nested_tuple)
{
    std::vector<std::tuple<bool, std::pair<int, std::pair<int, int>>, std::tuple<bool, bool, bool>>> to_sort;
    to_sort.emplace_back(true, std::make_pair(5, std::make_pair(6, 7)), std::make_tuple(true, false, true));
    to_sort.emplace_back(false, std::make_pair(5, std::make_pair(6, 7)), std::make_tuple(true, false, true));
    to_sort.emplace_back(false, std::make_pair(5, std::make_pair(6, 8)), std::make_tuple(true, false, true));
    to_sort.emplace_back(false, std::make_pair(5, std::make_pair(6, 6)), std::make_tuple(true, false, true));
    to_sort.emplace_back(false, std::make_pair(5, std::make_pair(7, 6)), std::make_tuple(true, false, true));
    to_sort.emplace_back(false, std::make_pair(5, std::make_pair(7, 6)), std::make_tuple(true, true, true));
    to_sort.emplace_back(false, std::make_pair(5, std::make_pair(7, 6)), std::make_tuple(true, true, false));
    to_sort.emplace_back(false, std::make_pair(5, std::make_pair(7, 6)), std::make_tuple(false, true, false));
    to_sort.emplace_back(false, std::make_pair(4, std::make_pair(7, 6)), std::make_tuple(false, true, false));
    ASSERT_TRUE(TestAllSorts(to_sort));
}
#endif

TEST(sort, string)
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
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}

TEST(sort, vector)
{
    std::vector<std::vector<int>> to_sort =
    {
        { 1, 2, 3 },
        { 1, 2, 2 },
        { 1, 3, 2 },
        { 2, 3, 2 },
        { 2, 3, 2, 4 },
        { 2, 3, 2, 4, 5 },
        { 3, 2, 4, 5 },
        { 1 },
        {},
    };
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}

#ifdef FULL_TESTS_SLOW_COMPILE_TIME
TEST(sort, string_in_vector)
{
    std::vector<std::vector<std::string>> to_sort =
    {
        { "hi", "there", "you" },
        { "are", "probably", "not", "going" },
        { "to", "pass" },
        { "" },
        { },
        { "this", "test", "the", "first" },
        { "time" },
        { "oh it did pass", "n", "e", "a", "t!" },
        { "hi", "there", "I", "added", "more", "tests" },
        { "hi", "there", "needed", "the", "same", "prefix" },
    };
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}
#endif

#ifdef FULL_TESTS_SLOW_COMPILE_TIME
TEST(sort, vector_tuple_string)
{
    std::vector<std::tuple<std::string, std::string>> to_sort;
    to_sort.emplace_back("hi", "there");
    to_sort.emplace_back("you", "are");
    to_sort.emplace_back("probably", "not");
    to_sort.emplace_back("going", "to");
    to_sort.emplace_back("pass", "");
    to_sort.emplace_back("", "");
    to_sort.emplace_back("", "this");
    to_sort.emplace_back("test", "the");
    to_sort.emplace_back("first", "time");
    to_sort.emplace_back("oh it did pass", "n");
    to_sort.emplace_back("e", "a");
    to_sort.emplace_back("t!", "");
    to_sort.emplace_back("hi", "there");
    to_sort.emplace_back("I", "added");
    to_sort.emplace_back("more", "tests");
    to_sort.emplace_back("hi", "there");
    to_sort.emplace_back("needed", "the");
    to_sort.emplace_back("same", "prefix");
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}
TEST(sort, vector_vector_tuple_string)
{
    std::vector<std::vector<std::tuple<std::string, std::string>>> to_sort;
    to_sort.emplace_back(std::vector<std::tuple<std::string, std::string>>
    {
        {"hi", "there"},
        {"you", "are"},
        {"probably", "not"}
    });
    to_sort.emplace_back(std::vector<std::tuple<std::string, std::string>>
    {
        {"going", "to"},
        {"pass", ""},
        {"", ""}
    });
    to_sort.emplace_back(std::vector<std::tuple<std::string, std::string>>
    {
        {"", "this"},
        {"test", "the"},
        {"first", "time"},
    });
    to_sort.emplace_back(std::vector<std::tuple<std::string, std::string>>
    {
        {"oh it did pass", "n"},
        {"e", "a"},
        {"t!", ""},
    });
    to_sort.emplace_back(std::vector<std::tuple<std::string, std::string>>
    {
        {"hi", "there"},
        {"I", "added"},
        {"more", "tests"},
    });
    to_sort.emplace_back(std::vector<std::tuple<std::string, std::string>>
    {
        {"hi", "there"},
        {"needed", "the"},
        {"same", "prefix"},
    });
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}
#endif

TEST(inplace_radix_sort, pointers)
{
    int array[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    std::vector<int * > to_sort =
    {
        array + 1,
        array + 3,
        array,
        array + 2,
        array + 7,
        array + 8,
        array + 6,
        array + 4,
        array + 5
    };
    ASSERT_TRUE(TestAllSorts(to_sort));
}
/*#include <list>
TEST(sort, vector_of_list)
{
    std::vector<std::list<int>> to_sort = { { 1, 2, 3 }, { 1, 2, 2 }, { 0, 1, 2 } };
    ASSERT_TRUE(TestAllSorts(to_sort));
}*/

struct MovableInt
{
    MovableInt()
        : i()
    {
    }

    MovableInt(int i)
        : i(i)
    {
    }
    MovableInt(MovableInt &&) = default;
    MovableInt & operator=(MovableInt &&) = default;

    bool operator==(const MovableInt & other) const
    {
        return i == other.i;
    }
    bool operator<(const MovableInt & other) const
    {
        return i < other.i;
    }

    int i;
};
int to_radix_sort_key(const MovableInt & i)
{
    return i.i;
}

TEST(sort, vector_of_movables)
{
    std::vector<std::vector<MovableInt>> to_sort;
    to_sort.emplace_back();
    to_sort.back().emplace_back(1); to_sort.back().emplace_back(2); to_sort.back().emplace_back(3);
    to_sort.emplace_back();
    to_sort.back().emplace_back(1); to_sort.back().emplace_back(2); to_sort.back().emplace_back(2);
    to_sort.emplace_back();
    to_sort.back().emplace_back(1); to_sort.back().emplace_back(1); to_sort.back().emplace_back(2);
    to_sort.emplace_back();
    to_sort.back().emplace_back(0); to_sort.back().emplace_back(1); to_sort.back().emplace_back(2);
    to_sort.emplace_back();
    to_sort.back().emplace_back(1); to_sort.back().emplace_back(2); to_sort.back().emplace_back(4);
    to_sort.emplace_back();
    to_sort.back().emplace_back(1); to_sort.back().emplace_back(3); to_sort.back().emplace_back(4);
    to_sort.emplace_back();
    to_sort.back().emplace_back(2); to_sort.back().emplace_back(3); to_sort.back().emplace_back(4);
    to_sort.emplace_back();
    to_sort.back().emplace_back(-2); to_sort.back().emplace_back(-3); to_sort.back().emplace_back(-4);
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}

TEST(sort, movables)
{
    std::vector<MovableInt> to_sort;
    to_sort.emplace_back(1);
    to_sort.emplace_back(2);
    to_sort.emplace_back(0);
    to_sort.emplace_back(-1);
    to_sort.emplace_back(20);
    to_sort.emplace_back(-5);
    ASSERT_TRUE(TestAllSorts(to_sort));
}

#ifdef FULL_TESTS_SLOW_COMPILE_TIME
TEST(sort, vector_of_vector_of_movables)
{
    std::vector<std::vector<std::vector<MovableInt>>> to_sort;
    to_sort.emplace_back();
    to_sort.back().emplace_back();
    to_sort.back().back().emplace_back(1); to_sort.back().back().emplace_back(2);
    to_sort.back().emplace_back();
    to_sort.back().back().emplace_back(2); to_sort.back().back().emplace_back(3);
    to_sort.emplace_back();
    to_sort.back().emplace_back();
    to_sort.back().back().emplace_back(1); to_sort.back().back().emplace_back(2);
    to_sort.emplace_back();
    to_sort.back().emplace_back();
    to_sort.back().back().emplace_back(1); to_sort.back().back().emplace_back(2);
    to_sort.back().back().emplace_back(1); to_sort.back().back().emplace_back(2);
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}
#endif

#ifdef FULL_TESTS_SLOW_COMPILE_TIME
TEST(sort, vector_vector_vector)
{
    std::vector<std::vector<std::vector<std::vector<int>>>> to_sort;
    to_sort.emplace_back();
    to_sort.back().emplace_back();
    to_sort.back().back().emplace_back();
    to_sort.back().back().back().emplace_back(1); to_sort.back().back().back().emplace_back(2);
    to_sort.back().back().emplace_back();
    to_sort.back().back().back().emplace_back(1); to_sort.back().back().back().emplace_back(3);
    to_sort.back().emplace_back();
    to_sort.back().back().emplace_back();
    to_sort.back().back().back().emplace_back(1); to_sort.back().back().back().emplace_back(3);
    to_sort.emplace_back();
    to_sort.back().emplace_back();
    to_sort.back().back().emplace_back();
    to_sort.emplace_back();
    to_sort.back().emplace_back();
    to_sort.back().back().emplace_back();
    to_sort.back().back().back().emplace_back(1); to_sort.back().back().back().emplace_back(3);
    to_sort.back().back().back().emplace_back(1); to_sort.back().back().back().emplace_back(3);
    to_sort.back().emplace_back();
    to_sort.back().back().emplace_back();
    to_sort.back().back().back().emplace_back(1); to_sort.back().back().back().emplace_back(2);
    to_sort.back().back().back().emplace_back(2); to_sort.back().back().back().emplace_back(2);
    to_sort.emplace_back();
    to_sort.back().emplace_back();
    to_sort.back().back().emplace_back();
    to_sort.back().back().back().emplace_back(1); to_sort.back().back().back().emplace_back(2);
    to_sort.emplace_back();
    to_sort.back().emplace_back();
    to_sort.back().back().emplace_back();
    to_sort.back().back().back().emplace_back(1); to_sort.back().back().back().emplace_back(2);
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}
#endif

struct MovableString : private std::string
{
    using std::string::string;
    MovableString() = default;
    MovableString(const MovableString &) = delete;
    MovableString & operator=(const MovableString &) = delete;
    MovableString(MovableString &&) = default;
    MovableString & operator=(MovableString &&) = default;

    friend bool operator<(const MovableString & lhs, const MovableString & rhs)
    {
        return static_cast<const std::string &>(lhs) < static_cast<const std::string &>(rhs);
    }

    friend const std::string & to_radix_sort_key(const MovableString & str)
    {
        return static_cast<const std::string &>(str);
    }
};

struct Customer
{
    Customer() = default;
    Customer(MovableString && first, MovableString && second)
        : first_name(std::move(first)), last_name(std::move(second))
    {
    }

    MovableString first_name;
    MovableString last_name;
};

TEST(sort, no_copy)
{
    // sorting customers by last name then first name
    // I want to return references. I use a MovableString
    // to make it easier to get an error message when copying
    // happens
    std::vector<Customer> to_sort;
    to_sort.emplace_back("a", "b");
    to_sort.emplace_back("foo", "bar");
    to_sort.emplace_back("g", "a");
    to_sort.emplace_back("w", "d");
    to_sort.emplace_back("b", "c");
    ASSERT_TRUE(TestVariableSizeSorts(to_sort, [](const Customer & customer)
    {
        return std::tie(customer.last_name, customer.first_name);
    }));
}

struct ThrowingType
{
    operator int() const
    {
        throw 5;
    }
};

TEST(sort, variant)
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
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}

TEST(sort, variant_in_pair)
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
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}
TEST(sort, variant_in_vector)
{
    std::vector<std::vector<std::variant<int, float, std::string>>> to_sort =
    {
        { 5, 6, 2, 1.0f, 3.1f },
        { 5.0f, 6.0f, 2, 1, 3.1f },
        { 5.0f, 6, 2.0f, 1, 3.1f },
        { 5, 6.0f, 2, 1, 3 },
        { 5, 6.0f, 2, 1, 3.1f },
        { 5.0f, 6, 2.1f, 1, 3 },
        { 5.0f, 6, 1.9f, 1, 3.1f },
        { 5.0f, 6, 1.9f, 1, 3.1f, "foo2" },
        { 5.0f, 6, 1.9f, 1, 3.1f, "foo3" },
        { 5.0f, 6, 1.9f, 1, 3.1f, "foo4" },
        { 5.0f, 6, 1.9f, 1, 3.1f, "foo1" },
    };
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}

TEST(sort, optional)
{
    std::vector<std::optional<int>> to_sort;
    to_sort.emplace_back(5);
    to_sort.emplace_back(std::nullopt);
    to_sort.emplace_back(6);
    to_sort.emplace_back(-3);
    to_sort.emplace_back(2);
    to_sort.emplace_back(7);
    to_sort.emplace_back(std::nullopt);
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}

TEST(sort, optional_in_pair)
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
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}
TEST(sort, optional_in_vector)
{
    std::vector<std::vector<std::optional<int>>> to_sort =
    {
        { 5, 6, 2, std::nullopt },
        { 5, 3, 2, std::nullopt },
        { 5, std::nullopt, 2, std::nullopt },
        { 5, std::nullopt, 4, std::nullopt },
        { 5, std::nullopt, 1, std::nullopt },
        { std::nullopt, std::nullopt, 1, std::nullopt },
        { std::nullopt, std::nullopt, 0, std::nullopt },
    };
    ASSERT_TRUE(TestVariableSizeSorts(to_sort));
}

#endif

// benchmarks
#if 0

#include "benchmark/benchmark.h"

#include <random>
#include <deque>

#if 1
//benchmark_inplace_sort/2M    103155817 ns  103115547 ns          7
//benchmark_inplace_sort/2M     73961470 ns   73923293 ns          9
//benchmark_inplace_sort/2M     69165349 ns   69165032 ns         10
//benchmark_ska_sort/2M        68850732 ns   68845594 ns         10
//benchmark_ska_sort/2M        67791104 ns   67794472 ns         10
static std::vector<int32_t> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<int32_t> result;
    result.reserve(size);
    std::uniform_int_distribution<int32_t> distribution;
    for (int i = 0; i < size; ++i)
    {
        result.push_back(distribution(randomness));
    }
    return result;
}

#elif 0

static std::vector<float> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<float> result;
    result.reserve(size);
    std::uniform_real_distribution<float> distribution;
    //std::exponential_distribution<float> distribution;
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(distribution(randomness));
    }
    return result;
}
#elif 0

static std::vector<std::pair<bool, float>> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::pair<bool, float>> result;
    result.reserve(size);
    std::uniform_int_distribution<int> int_distribution(0, 1);
    std::uniform_real_distribution<float> real_distribution;
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(int_distribution(randomness) != 0, real_distribution(randomness));
    }
    return result;
}
#elif 0
static std::deque<bool> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::deque<bool> result;
    std::uniform_int_distribution<int> int_distribution(0, 1);
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(int_distribution(randomness) != 0);
    }
    return result;
}
#elif 1
static std::vector<std::int8_t> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::int8_t> result;
    result.reserve(size);
    std::uniform_int_distribution<std::int8_t> int_distribution(-128, 127);
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(int_distribution(randomness));
    }
    return result;
}
#elif 0
static std::vector<std::int8_t> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::int8_t> result;
    result.reserve(size);
    std::uniform_int_distribution<std::int8_t> int_distribution(0, 1);
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(int_distribution(randomness));
    }
    return result;
}
#elif 0
static std::vector<std::uint8_t> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::uint8_t> result;
    result.reserve(size);
    std::geometric_distribution<std::uint8_t> int_distribution(0.05);
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(int_distribution(randomness));
    }
    return result;
}
#elif 0
// iterating over remaining buckets
//benchmark_ska_sort/2M       227948666 ns  227854867 ns          3
//benchmark_ska_sort/2M       226300240 ns  226282990 ns          3
// iterating over all buckets
//benchmark_ska_sort/2M       227441549 ns  227423700 ns          3
//benchmark_ska_sort/2M       226502577 ns  226484056 ns          3
static std::vector<int> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<int> result;
    result.reserve(size);
    std::geometric_distribution<int> int_distribution(0.001);
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(int_distribution(randomness));
    }
    return result;
}
#elif 0
static std::vector<std::int16_t> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::int16_t> result;
    result.reserve(size);
    std::uniform_int_distribution<std::int16_t> int_distribution(-32768, 32767);
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(int_distribution(randomness));
    }
    return result;
}
#elif 0
static std::vector<std::int64_t> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::int64_t> result;
    result.reserve(size);
    std::uniform_int_distribution<std::int64_t> int_distribution(std::numeric_limits<int64_t>::lowest(), std::numeric_limits<int64_t>::max());
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(int_distribution(randomness));
    }
    return result;
}
#elif 0
static std::vector<std::tuple<std::int64_t, std::int64_t>> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::tuple<std::int64_t, std::int64_t>> result;
    result.reserve(size);
    std::uniform_int_distribution<std::int64_t> int_distribution(std::numeric_limits<int64_t>::lowest(), std::numeric_limits<int64_t>::max());
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(int_distribution(randomness), int_distribution(randomness));
    }
    return result;
}
#elif 0
static std::vector<std::tuple<std::int32_t, std::int32_t, std::int64_t>> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::tuple<std::int32_t, std::int32_t, std::int64_t>> result;
    result.reserve(size);
    std::uniform_int_distribution<std::int32_t> int32_distribution(std::numeric_limits<int32_t>::lowest(), std::numeric_limits<int32_t>::max());
    std::uniform_int_distribution<std::int64_t> int64_distribution(std::numeric_limits<int64_t>::lowest(), std::numeric_limits<int64_t>::max());
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(int32_distribution(randomness), int32_distribution(randomness), int64_distribution(randomness));
    }
    return result;
}
#elif 0
static std::vector<std::vector<int>> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::vector<int>> result;
    result.reserve(size);
    std::uniform_int_distribution<int> size_distribution(0, 20);
    std::uniform_int_distribution<int> value_distribution;
    for (int i = 0; i < size; ++i)
    {
        std::vector<int> to_add(size_distribution(randomness));
        std::generate(to_add.begin(), to_add.end(), [&]{ return value_distribution(randomness); });
        result.push_back(std::move(to_add));
    }
    return result;
}
#elif 0
static std::vector<std::vector<std::string>> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::vector<std::string>> result;
    result.reserve(size);
    std::uniform_int_distribution<int> size_distribution(0, 10);
    std::uniform_int_distribution<int> string_length_distribution(0, 5);
    std::uniform_int_distribution<char> string_content_distribution('a', 'c');
    for (int i = 0; i < size; ++i)
    {
        std::vector<std::string> to_add(size_distribution(randomness));
        std::generate(to_add.begin(), to_add.end(), [&]
        {
#if 0
            std::string new_string = "hello";
            for (int i = 0, end = string_length_distribution(randomness); i != end; ++i)
                new_string.push_back('\0');
            std::generate(new_string.begin() + 5, new_string.end(), [&]
            {
                return string_content_distribution(randomness);
            });
#else
            std::string new_string(string_length_distribution(randomness), '\0');
            std::generate(new_string.begin(), new_string.end(), [&]
            {
                return string_content_distribution(randomness);
            });
#endif
            return new_string;
        });
        result.push_back(std::move(to_add));
    }
    return result;
}

#elif 0

static std::vector<std::string> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::string> result;
    result.reserve(size);
    std::uniform_int_distribution<int> string_length_distribution(0, 20);
    std::uniform_int_distribution<char> string_content_distribution('a', 'z');
    for (int i = 0; i < size; ++i)
    {
        std::string to_add(string_length_distribution(randomness), '\0');
        std::generate(to_add.begin(), to_add.end(), [&]
        {
            return string_content_distribution(randomness);
        });
        result.push_back(std::move(to_add));
    }
    return result;
}

#elif 0

extern const std::vector<const char *> & get_word_list();
static std::vector<std::string> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    const std::vector<const char *> & words = get_word_list();
    std::vector<std::string> result;
    result.reserve(size);
    std::uniform_int_distribution<int> string_length_distribution(0, 10);
    //std::uniform_int_distribution<int> string_length_distribution(1, 3);
    std::uniform_int_distribution<size_t> word_picker(0, words.size() - 1);
    for (int i = 0; i < size; ++i)
    {
        std::string to_add;
        for (int i = 0, end = string_length_distribution(randomness); i < end; ++i)
        {
            to_add += words[word_picker(randomness)];
        }
        result.push_back(std::move(to_add));
    }
    return result;
}

#elif 0

// worst case
static std::vector<std::vector<int>> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::vector<int>> result;
    std::uniform_int_distribution<int> random_size(0, 128);
    result.reserve(size);
    for (int i = 0; i < size; ++i)
    {
        std::vector<int> to_add(random_size(randomness));
        std::iota(to_add.begin(), to_add.end(), 0);
        result.push_back(std::move(to_add));
    }
    return result;
}

#elif 0

#include <set>

static std::vector<std::set<int>> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::set<int>> result;
    std::uniform_int_distribution<int> random_size(0, 20);
    std::uniform_int_distribution<int> value(0, 30);
    result.reserve(size);
    for (int i = 0; i < size; ++i)
    {
        std::set<int> to_add;
        for (int j = random_size(randomness); j > 0; --j)
        {
            to_add.insert(value(randomness));
        }
        result.push_back(std::move(to_add));
    }
    return result;
}

#elif 0
extern const std::vector<const char *> & get_kern_log();
static std::vector<std::string> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 &, int size)
{
    const std::vector<const char *> & kern_log = get_kern_log();
    auto end = kern_log.begin() + std::min(int(kern_log.size()), size);
    return std::vector<std::string>(kern_log.begin(), end);
}
#elif 0
template<size_t Size>
struct SizedStruct
{
    uint8_t array[Size] = {};
};
template<>
struct SizedStruct<0>
{
};
#define SORT_ON_FIRST_ONLY
typedef std::int64_t benchmark_sort_key;
#define NUM_SORT_KEYS 1
typedef SizedStruct<1016> benchmark_sort_value;
#if NUM_SORT_KEYS == 1
static std::vector<std::tuple<benchmark_sort_key, benchmark_sort_value>> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::tuple<benchmark_sort_key, benchmark_sort_value>> result;
    result.reserve(size);
    std::uniform_int_distribution<benchmark_sort_key> distribution(std::numeric_limits<benchmark_sort_key>::lowest(), std::numeric_limits<benchmark_sort_key>::max());
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(distribution(randomness), benchmark_sort_value());
    }
    return result;
}
#elif NUM_SORT_KEYS == 2
static std::vector<std::tuple<std::pair<benchmark_sort_key, benchmark_sort_key>, benchmark_sort_value>> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::tuple<std::pair<benchmark_sort_key, benchmark_sort_key>, benchmark_sort_value>> result;
    result.reserve(size);
    std::uniform_int_distribution<benchmark_sort_key> distribution(std::numeric_limits<benchmark_sort_key>::lowest(), std::numeric_limits<benchmark_sort_key>::max());
    for (int i = 0; i < size; ++i)
    {
        result.emplace_back(std::make_pair(distribution(randomness), distribution(randomness)), benchmark_sort_value());
    }
    return result;
}
#else
static std::vector<std::tuple<std::array<benchmark_sort_key, NUM_SORT_KEYS>, benchmark_sort_value>> SKA_SORT_NOINLINE create_radix_sort_data(std::mt19937_64 & randomness, int size)
{
    std::vector<std::tuple<std::array<benchmark_sort_key, NUM_SORT_KEYS>, benchmark_sort_value>> result;
    result.reserve(size);
    std::uniform_int_distribution<benchmark_sort_key> distribution(std::numeric_limits<benchmark_sort_key>::lowest(), std::numeric_limits<benchmark_sort_key>::max());
    for (int i = 0; i < size; ++i)
    {
        std::array<benchmark_sort_key, NUM_SORT_KEYS> key;
        for (int i = 0; i < NUM_SORT_KEYS; ++i)
            key[i] = distribution(randomness);
        result.emplace_back(key, benchmark_sort_value());
    }
    return result;
}
#endif
#endif

#if 1
static constexpr int profile_multiplier = 2;
static constexpr int max_profile_range = 1 << 22;

#if 1
#if 0
static void benchmark_radix_sort(benchmark::State & state)
{
    std::mt19937_64 randomness(77342348);
    auto buffer = create_radix_sort_data(randomness, state.range(0));
    while (state.KeepRunning())
    {
        auto to_sort = create_radix_sort_data(randomness, state.range(0));
        benchmark::DoNotOptimize(to_sort.data());
        benchmark::DoNotOptimize(buffer.data());
#ifdef SORT_ON_FIRST_ONLY
        radix_sort(to_sort.begin(), to_sort.end(), buffer.begin(), [](auto && a){ return std::get<0>(a); });
#else
        bool which = radix_sort(to_sort.begin(), to_sort.end(), buffer.begin());
        if (which)
            assert(std::is_sorted(buffer.begin(), buffer.end()));
        else
            assert(std::is_sorted(to_sort.begin(), to_sort.end()));
#endif
        benchmark::ClobberMemory();
    }
}
BENCHMARK(benchmark_radix_sort)->RangeMultiplier(profile_multiplier)->Range(profile_multiplier, max_profile_range);
#endif

#if 0
static void benchmark_ska_sort_ping_pong(benchmark::State & state)
{
    std::mt19937_64 randomness(77342348);
    auto buffer = create_radix_sort_data(randomness, state.range(0));
    while (state.KeepRunning())
    {
        auto to_sort = create_radix_sort_data(randomness, state.range(0));
#ifdef SORT_ON_FIRST_ONLY
        ska_sort_ping_pong(to_sort.begin(), to_sort.end(), buffer.begin(), [](auto && a) -> decltype(auto){ return std::get<0>(a); });
#else
        bool which = ska_sort_ping_pong(to_sort.begin(), to_sort.end(), buffer.begin());
        if (which)
            assert(std::is_sorted(buffer.begin(), buffer.end()));
        else
            assert(std::is_sorted(to_sort.begin(), to_sort.end()));
#endif
        benchmark::DoNotOptimize(to_sort.data());
        benchmark::DoNotOptimize(buffer.data());
    }
}
BENCHMARK(benchmark_ska_sort_ping_pong)->RangeMultiplier(profile_multiplier)->Range(profile_multiplier, max_profile_range);
#endif

#if 1

static void benchmark_std_sort(benchmark::State & state)
{
    std::mt19937_64 randomness(77342348);
    create_radix_sort_data(randomness, state.range(0));
    while (state.KeepRunning())
    {
        auto to_sort = create_radix_sort_data(randomness, state.range(0));
        benchmark::DoNotOptimize(to_sort.data());
#ifdef SORT_ON_FIRST_ONLY
        std::sort(to_sort.begin(), to_sort.end(), [](auto && l, auto && r){ return std::get<0>(l) < std::get<0>(r); });
#else
        std::sort(to_sort.begin(), to_sort.end());
        assert(std::is_sorted(to_sort.begin(), to_sort.end()));
#endif
        benchmark::ClobberMemory();
    }
}
BENCHMARK(benchmark_std_sort)->RangeMultiplier(profile_multiplier)->Range(profile_multiplier, max_profile_range);

#endif

#if 1

static void benchmark_american_flag_sort(benchmark::State & state)
{
    std::mt19937_64 randomness(77342348);
    create_radix_sort_data(randomness, state.range(0));
    while (state.KeepRunning())
    {
        auto to_sort = create_radix_sort_data(randomness, state.range(0));
        benchmark::DoNotOptimize(to_sort.data());
#ifdef SORT_ON_FIRST_ONLY
        american_flag_sort(to_sort.begin(), to_sort.end(), [](auto && a) -> decltype(auto){ return std::get<0>(a); });
#else
        american_flag_sort(to_sort.begin(), to_sort.end());
        assert(std::is_sorted(to_sort.begin(), to_sort.end()));
#endif
        benchmark::ClobberMemory();
    }
}
BENCHMARK(benchmark_american_flag_sort)->RangeMultiplier(profile_multiplier)->Range(profile_multiplier, max_profile_range);
#endif

#endif


static void benchmark_ska_sort(benchmark::State & state)
{
    std::mt19937_64 randomness(77342348);
    create_radix_sort_data(randomness, state.range(0));
    while (state.KeepRunning())
    {
        auto to_sort = create_radix_sort_data(randomness, state.range(0));
        benchmark::DoNotOptimize(to_sort.data());
#ifdef SORT_ON_FIRST_ONLY
        ska_sort(to_sort.begin(), to_sort.end(), [](auto && a) -> decltype(auto){ return std::get<0>(a); });
#else
        ska_sort(to_sort.begin(), to_sort.end());
        assert(std::is_sorted(to_sort.begin(), to_sort.end()));
#endif
        benchmark::ClobberMemory();
    }
}
BENCHMARK(benchmark_ska_sort)->RangeMultiplier(profile_multiplier)->Range(profile_multiplier, max_profile_range);

#if 0

#include <boost/sort/spreadsort/spreadsort.hpp>

static void benchmark_boost_spread_sort(benchmark::State & state)
{
    std::mt19937_64 randomness(77342348);
    create_radix_sort_data(randomness, state.range(0));
    while (state.KeepRunning())
    {
        auto to_sort = create_radix_sort_data(randomness, state.range(0));
        benchmark::DoNotOptimize(to_sort.data());
#ifdef SORT_ON_FIRST_ONLY
        boost::sort::spreadsort::spreadsort(to_sort.begin(), to_sort.end(), [](auto && a) -> decltype(auto){ return std::get<0>(a); });
#else
        boost::sort::spreadsort::spreadsort(to_sort.begin(), to_sort.end());
        assert(std::is_sorted(to_sort.begin(), to_sort.end()));
#endif
        benchmark::ClobberMemory();
    }
}
BENCHMARK(benchmark_boost_spread_sort)->RangeMultiplier(profile_multiplier)->Range(profile_multiplier, max_profile_range);

#endif

#if 0

static void benchmark_inplace_radix_sort(benchmark::State & state)
{
    std::mt19937_64 randomness(77342348);
    create_radix_sort_data(randomness, state.range(0));
    while (state.KeepRunning())
    {
        auto to_sort = create_radix_sort_data(randomness, state.range(0));
        benchmark::DoNotOptimize(to_sort.data());
#ifdef SORT_ON_FIRST_ONLY
        inplace_radix_sort(to_sort.begin(), to_sort.end(), [](auto && a) -> decltype(auto){ return std::get<0>(a); });
#else
        inplace_radix_sort(to_sort.begin(), to_sort.end());
        assert(std::is_sorted(to_sort.begin(), to_sort.end()));
#endif
        benchmark::ClobberMemory();
    }
}
BENCHMARK(benchmark_inplace_radix_sort)->RangeMultiplier(profile_multiplier)->Range(profile_multiplier, max_profile_range);

#endif

static void benchmark_generation(benchmark::State & state)
{
    std::mt19937_64 randomness(77342348);
    create_radix_sort_data(randomness, state.range(0));
    while (state.KeepRunning())
    {
        auto to_sort = create_radix_sort_data(randomness, state.range(0));
        benchmark::DoNotOptimize(to_sort.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(benchmark_generation)->RangeMultiplier(profile_multiplier)->Range(profile_multiplier, max_profile_range);

#endif


#if 0
static std::vector<std::int8_t> SKA_SORT_NOINLINE create_limited_radix_sort_data(std::mt19937_64 & randomness, int8_t range_end)
{
    int8_t permutation[256];
    std::iota(permutation, permutation + 256, -128);
    std::shuffle(permutation, permutation + 256, randomness);
    std::vector<std::int8_t> result;
    size_t size = 2 * 1024 * 1024;
    result.reserve(size);
    std::uniform_int_distribution<std::int8_t> int_distribution(-128, range_end);
    for (size_t i = 0; i < size; ++i)
    {
        result.emplace_back(permutation[detail::to_unsigned_or_bool(int_distribution(randomness))]);
    }
    return result;
}
static void benchmark_limited_generation(benchmark::State & state)
{
    std::mt19937_64 randomness(77342348);
    while (state.KeepRunning())
    {
        auto to_sort = create_limited_radix_sort_data(randomness, state.range(0));
        benchmark::DoNotOptimize(to_sort.data());
        benchmark::ClobberMemory();
    }
}

#define LIMITED_RANGE() Arg(-128)->Arg(-127)->Arg(-120)->Arg(-96)->Arg(-64)->Arg(-32)->Arg(0)->Arg(32)->Arg(64)->Arg(96)->Arg(127)
BENCHMARK(benchmark_limited_generation)->LIMITED_RANGE();

static void benchmark_limited_inplace_sort(benchmark::State & state)
{
    std::mt19937_64 randomness(77342348);
    while (state.KeepRunning())
    {
        auto to_sort = create_limited_radix_sort_data(randomness, state.range(0));
        benchmark::DoNotOptimize(to_sort.data());
        ska_sort(to_sort.begin(), to_sort.end());
        assert(std::is_sorted(to_sort.begin(), to_sort.end()));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(benchmark_limited_inplace_sort)->LIMITED_RANGE();
#endif

#if 0
static constexpr int sort_amount = 1024 * 1024;
TEST(benchmark, american_flag_sort)
{
    std::mt19937_64 randomness(77342348);
    for (int i = 0; i < 100; ++i)
    {
        auto to_sort = create_radix_sort_data(randomness, sort_amount);
        american_flag_sort(to_sort.begin(), to_sort.end());
        ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
    }
}
TEST(benchmark, inplace_faster)
{
    std::mt19937_64 randomness(77342348);
    for (int i = 0; i < 100; ++i)
    {
        auto to_sort = create_radix_sort_data(randomness, sort_amount);
        ska_sort(to_sort.begin(), to_sort.end());
        ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
    }
}
#endif

#endif

#if 1
#include <custom_benchmark/custom_benchmark.h>
#include "hashtable_benchmarks/benchmark_shared.hpp"
#include <boost/sort/sort.hpp>
#include "util/ska_sort2.hpp"


template<size_t Size>
struct IntWithPadding
{
    int to_sort_on;
    int padding[Size / 4 - 1];

    IntWithPadding(int value)
        : to_sort_on(value)
    {
        std::fill(std::begin(padding), std::end(padding), value);
    }

    bool operator<(const IntWithPadding & other) const
    {
        return to_sort_on < other.to_sort_on;
    }
};
template<size_t Size>
int to_radix_sort_key(const IntWithPadding<Size> & int_with_padding)
{
    return int_with_padding.to_sort_on;
}
template<typename>
struct IsIntWithPadding
{
    static constexpr bool value = false;
};
template<size_t Size>
struct IsIntWithPadding<IntWithPadding<Size>>
{
    static constexpr bool value = true;
};

template<typename T>
struct SortInput
{
    static SKA_NOINLINE(std::vector<T>) sort_input(size_t num_items)
    {
        std::uniform_int_distribution<T> distribution;
        std::vector<T> result(num_items);
        for (T & item : result)
            item = distribution(global_randomness);
        return result;
    }
};
template<size_t Size>
struct SortInput<IntWithPadding<Size>>
{
    static SKA_NOINLINE(std::vector<IntWithPadding<Size>>) sort_input(size_t num_items)
    {
        std::uniform_int_distribution<int> distribution;
        std::vector<IntWithPadding<Size>> result;
        result.reserve(num_items);
        for (size_t i = 0; i < num_items; ++i)
            result.emplace_back(distribution(global_randomness));
        return result;
    }
};
template<>
struct SortInput<float>
{
    static SKA_NOINLINE(std::vector<float>) sort_input(size_t num_items)
    {
        std::normal_distribution<float> distribution(0.0f, 1000.0f);
        std::vector<float> result(num_items);
        for (float & item : result)
            item = distribution(global_randomness);
        return result;
    }
};
extern const std::vector<const char *> & get_word_list();
extern const std::vector<const char *> & get_kern_log();
template<>
struct SortInput<std::string>
{
    static SKA_NOINLINE(std::vector<std::string>) sort_input(size_t num_items)
    {
        std::uniform_int_distribution<int> list_choice(0, 10);
        std::uniform_int_distribution<int> num_words(1, 10);
        const std::vector<const char *> & word_list = get_word_list();
        const std::vector<const char *> & kern_log = get_kern_log();
        std::uniform_int_distribution<size_t> random_word(0, word_list.size() - 1);
        std::uniform_int_distribution<size_t> random_log(0, kern_log.size() - 1);
        std::vector<std::string> result(num_items);
        for (std::string & item : result)
        {
            if (list_choice(global_randomness) != 0)
            {
                for (int i = 0, end = num_words(global_randomness); i < end; ++i)
                {
                    item += word_list[random_word(global_randomness)];
                }
            }
            else
            {
                item = kern_log[random_log(global_randomness)];
            }
        }
        return result;
    }
};

template<typename It>
SKA_NOINLINE(void) noinline_shuffle(It begin, It end)
{
    std::shuffle(begin, end, global_randomness);
}

template<typename T, typename SortSettings = detail::DefaultSortSettings>
void benchmark_ska_sort(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        detail::ska_sort_with_settings<SortSettings>(input.begin(), input.end());
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
template<typename T, typename SortSettings = detail::DefaultSortSettings>
void benchmark_ska_sort2(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        detail::ska_sort2_with_settings<SortSettings>(input.begin(), input.end());
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
    ska_sort2(input.begin(), input.end());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(input.begin(), input.end()));
}
template<typename T>
void benchmark_american_flag_sort(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        american_flag_sort(input.begin(), input.end());
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
template<typename T>
void benchmark_ska_sort_ping_pong(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    std::vector<T> copy(input);
    while (state.KeepRunning())
    {
        if (ska_sort_ping_pong(input.begin(), input.end(), copy.begin()))
            input.swap(copy);
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
template<typename T, typename SortSettings = detail::DefaultSortSettings>
void benchmark_counting_sort_ping_pong(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    std::vector<T> copy(input);
    while (state.KeepRunning())
    {
        if (counting_sort_ping_pong<SortSettings>(input.begin(), input.end(), copy.begin()))
            input.swap(copy);
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
template<typename T, size_t InsertionSortUpperLimit>
void benchmark_counting_sort_ping_pong_configurable(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    std::vector<T> copy(input);
    while (state.KeepRunning())
    {
        if (counting_sort_ping_pong_configurable<InsertionSortUpperLimit>(input.begin(), input.end(), copy.begin()))
            input.swap(copy);
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
template<typename T, size_t InsertionSortUpperLimit, std::ptrdiff_t AmericanFlagSortUpperLimit = detail::DefaultSortSettings::AmericanFlagSortUpperLimit>
void benchmark_ska_sort_configurable(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        ska_sort_configurable<InsertionSortUpperLimit, AmericanFlagSortUpperLimit>(input.begin(), input.end());
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
template<std::ptrdiff_t InsertionSortUpperLimit = detail::DefaultSortSettings::InsertionSortUpperLimit<int>
         , std::ptrdiff_t AmericanFlagSortUpperLimit = detail::DefaultSortSettings::AmericanFlagSortUpperLimit
         , size_t FirstLoopUnrollAmount = detail::DefaultSortSettings::FirstLoopUnrollAmount
         , size_t SecondLoopUnrollAmount = detail::DefaultSortSettings::SecondLoopUnrollAmount
         , bool UseIndexSort = detail::DefaultSortSettings::UseIndexSort
         , bool UseFasterCompare = detail::DefaultSortSettings::UseFasterCompare, typename It, typename ExtractKey>
void ska_sort2_configurable(It begin, It end, ExtractKey && extract_key)
{
    detail::ska_sort2_with_settings<ConfigurableSettings<InsertionSortUpperLimit, AmericanFlagSortUpperLimit, detail::DefaultSortSettings::ThreeWaySwap, FirstLoopUnrollAmount, SecondLoopUnrollAmount, UseIndexSort, UseFasterCompare>>(begin, end, extract_key);
}
template<std::ptrdiff_t InsertionSortUpperLimit = detail::DefaultSortSettings::InsertionSortUpperLimit<int>
         , std::ptrdiff_t AmericanFlagSortUpperLimit = detail::DefaultSortSettings::AmericanFlagSortUpperLimit
         , size_t FirstLoopUnrollAmount = detail::DefaultSortSettings::FirstLoopUnrollAmount
         , size_t SecondLoopUnrollAmount = detail::DefaultSortSettings::SecondLoopUnrollAmount
         , bool UseIndexSort = detail::DefaultSortSettings::UseIndexSort
         , bool UseFasterCompare = detail::DefaultSortSettings::UseFasterCompare, typename It>
void ska_sort2_configurable(It begin, It end)
{
    ska_sort2_configurable<InsertionSortUpperLimit, AmericanFlagSortUpperLimit, FirstLoopUnrollAmount, SecondLoopUnrollAmount, UseIndexSort, UseFasterCompare>(begin, end, detail::IdentityFunctor());
}
template<typename T, size_t InsertionSortUpperLimit
                   , std::ptrdiff_t AmericanFlagSortUpperLimit = detail::DefaultSortSettings::AmericanFlagSortUpperLimit
                   , size_t FirstLoopUnrollAmount = detail::DefaultSortSettings::FirstLoopUnrollAmount
                   , size_t SecondLoopUnrollAmount = detail::DefaultSortSettings::SecondLoopUnrollAmount
                   , bool UseIndexSort = detail::DefaultSortSettings::UseIndexSort
                   , bool UseFasterCompare = detail::DefaultSortSettings::UseFasterCompare>
void benchmark_ska_sort2_configurable(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        ska_sort2_configurable<InsertionSortUpperLimit, AmericanFlagSortUpperLimit, FirstLoopUnrollAmount, SecondLoopUnrollAmount, UseIndexSort, UseFasterCompare>(input.begin(), input.end());
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
    ska_sort2(input.begin(), input.end());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(input.begin(), input.end()));
}
template<typename T, size_t InsertionSortUpperLimit, bool ThreeWaySwap = detail::DefaultSortSettings::ThreeWaySwap>
void benchmark_ska_byte_sort_configurable(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        ska_byte_sort_configurable<InsertionSortUpperLimit, ThreeWaySwap>(input.begin(), input.end());
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
template<typename T, size_t InsertionSortUpperLimit, bool ThreeWaySwap = detail::DefaultSortSettings::ThreeWaySwap>
void benchmark_american_flag_sort_configurable(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        american_flag_sort_configurable<InsertionSortUpperLimit, ThreeWaySwap>(input.begin(), input.end());
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
template<typename T>
void benchmark_std_sort(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        std::sort(input.begin(), input.end());
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
template<typename T>
void benchmark_spreadsort(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        boost::sort::spreadsort::spreadsort(input.begin(), input.end());
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
#include "Raduls/raduls.h"
#include <cstdlib>
template<typename T>
struct benchmark_raduls
{
    int num_threads = 1;
    void operator()(skb::State & state) const
    {
        size_t num_items = state.range(0);
        std::vector<T> input = SortInput<T>::sort_input(num_items);
        for (T & int_with_padding : input)
        {
            int_with_padding.padding[0] = 0;
        }
        auto dealloc_with_free = [](void * ptr)
        {
            if (ptr)
                free(ptr);
        };
        std::unique_ptr<void, decltype(dealloc_with_free)> input_aligned(std::aligned_alloc(256, sizeof(T) * num_items), dealloc_with_free);
        std::unique_ptr<void, decltype(dealloc_with_free)> tmp(std::aligned_alloc(256, sizeof(T) * num_items), dealloc_with_free);
        T * aligned_begin = static_cast<T *>(input_aligned.get());
        T * aligned_end = aligned_begin + num_items;
        std::uninitialized_copy(input.begin(), input.end(), aligned_begin);
        std::uninitialized_fill(static_cast<T *>(tmp.get()), static_cast<T *>(tmp.get()) + num_items, T(0));
        while (state.KeepRunning())
        {
            raduls::RadixSortMSD(reinterpret_cast<uint8_t*>(aligned_begin), reinterpret_cast<uint8_t*>(tmp.get()), num_items, sizeof(T), 4, num_threads);
            noinline_shuffle(aligned_begin, aligned_end);
        }
        state.SetItemsProcessed(state.iterations() * num_items);
        raduls::RadixSortMSD(reinterpret_cast<uint8_t*>(aligned_begin), reinterpret_cast<uint8_t*>(tmp.get()), num_items, sizeof(T), 4, num_threads);
        CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(aligned_begin, aligned_end));
    }
};
template<typename T>
void benchmark_insertion_sort(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        detail::insertion_sort(input.begin(), input.end(), std::less<>{});
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
template<typename T>
void benchmark_index_sort(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        detail::index_sort(input.begin(), input.end(), std::less<>{});
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
template<typename T>
void benchmark_sort_baseline(skb::State & state)
{
    size_t num_items = state.range(0);
    std::vector<T> input = SortInput<T>::sort_input(num_items);
    while (state.KeepRunning())
    {
        noinline_shuffle(input.begin(), input.end());
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}

template<typename T>
static skb::Benchmark * SortRange(skb::Benchmark * benchmark, double range_multiplier = std::sqrt(2.0))
{
    int max = 256 * 1024 * 1024;
    //if (sizeof(T) < sizeof(int))
    //    max = std::min(max, static_cast<int>(std::numeric_limits<T>::max()));
    return benchmark->SetRange(4, max)->SetRangeMultiplier(range_multiplier);
}

template<>
skb::Benchmark * SortRange<std::string>(skb::Benchmark * benchmark, double range_multiplier)
{
    int max = 4 * 1024 * 1024;
    return benchmark->SetRange(4, max)->SetRangeMultiplier(range_multiplier);
}

struct Uint32SortSettings : detail::DefaultSortSettings
{
    using count_type = uint32_t;
};

struct Int32SortSettings : detail::DefaultSortSettings
{
    using count_type = int32_t;
};

template<typename T, size_t StdSortFallback, size_t FirstLoopUnrollAmount, size_t SecondLoopUnrollAmount>
void RegisterSortForTypeAndUnrollAmount(skb::CategoryBuilder categories_so_far, const std::string & benchmark_type, const std::string & baseline_name)
{
    categories_so_far = categories_so_far.AddCategory("first_loop_unroll", std::to_string(FirstLoopUnrollAmount)).AddCategory("second_loop_unroll", std::to_string(SecondLoopUnrollAmount));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_ska_sort2_configurable<T, StdSortFallback, detail::DefaultSortSettings::AmericanFlagSortUpperLimit, FirstLoopUnrollAmount, SecondLoopUnrollAmount>, categories_so_far.BuildCategories(benchmark_type, "ska_sort2"))->SetBaseline(baseline_name));
}

template<typename T, size_t StdSortFallback>
void RegisterSortForTypeAndFallback(skb::CategoryBuilder categories_so_far, const std::string & benchmark_type, const std::string & baseline_name)
{
    categories_so_far = categories_so_far.AddCategory("std_sort_upper_limit", std::to_string(StdSortFallback));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_counting_sort_ping_pong_configurable<T, StdSortFallback>, categories_so_far.BuildCategories(benchmark_type, "counting_sort_ping_pong"))->SetBaseline(baseline_name));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_ska_sort_configurable<T, StdSortFallback>, categories_so_far.BuildCategories(benchmark_type, "ska_sort"))->SetBaseline(baseline_name));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_ska_sort2_configurable<T, StdSortFallback>, categories_so_far.BuildCategories(benchmark_type, "ska_sort2"))->SetBaseline(baseline_name));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_ska_sort2_configurable<T, StdSortFallback, detail::DefaultSortSettings::AmericanFlagSortUpperLimit, detail::DefaultSortSettings::FirstLoopUnrollAmount, detail::DefaultSortSettings::SecondLoopUnrollAmount, true>, categories_so_far.AddCategory("optimizations", "index_sort").BuildCategories(benchmark_type, "ska_sort2"))->SetBaseline(baseline_name));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_ska_sort2_configurable<T, StdSortFallback, detail::DefaultSortSettings::AmericanFlagSortUpperLimit, detail::DefaultSortSettings::FirstLoopUnrollAmount, detail::DefaultSortSettings::SecondLoopUnrollAmount, detail::DefaultSortSettings::UseIndexSort, !detail::DefaultSortSettings::UseFasterCompare>, categories_so_far.AddCategory("optimizations", detail::DefaultSortSettings::UseFasterCompare ? "no faster compare" : "faster compare").BuildCategories(benchmark_type, "ska_sort2"))->SetBaseline(baseline_name));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_ska_byte_sort_configurable<T, StdSortFallback>, categories_so_far.BuildCategories(benchmark_type, "ska_byte_sort"))->SetBaseline(baseline_name));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_ska_byte_sort_configurable<T, StdSortFallback, true>, categories_so_far.AddCategory("optimizations", "three way swap").BuildCategories(benchmark_type, "ska_byte_sort"))->SetBaseline(baseline_name));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_american_flag_sort_configurable<T, StdSortFallback>, categories_so_far.BuildCategories(benchmark_type, "american_flag_sort"))->SetBaseline(baseline_name));

    /*RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 1, 1>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 1, 2>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 1, 4>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 1, 8>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 2, 1>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 2, 2>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 2, 4>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 2, 8>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 4, 1>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 4, 2>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 4, 4>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 4, 8>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 8, 1>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 8, 2>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 8, 4>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndUnrollAmount<T, StdSortFallback, 8, 8>(categories_so_far, benchmark_type, baseline_name);*/
}
template<typename T, size_t AmericanFlagSortUpperLimit>
void RegisterSortForTypeAndAmericanFlagSortLimit(skb::CategoryBuilder categories_so_far, const std::string & benchmark_type, const std::string & baseline_name)
{
    categories_so_far = categories_so_far.AddCategory("american_flag_sort_upper_limit", std::to_string(AmericanFlagSortUpperLimit));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_ska_sort_configurable<T, detail::DefaultSortSettings::InsertionSortUpperLimit<int>, AmericanFlagSortUpperLimit>, categories_so_far.BuildCategories(benchmark_type, "ska_sort"))->SetBaseline(baseline_name));
}

template<typename T>
void RegisterSortForType(skb::CategoryBuilder categories_so_far, const std::string & sorted_type, const std::string & baseline_type)
{
    categories_so_far = categories_so_far.AddCategory("sorted_type", sorted_type);
    std::string baseline_name = "baseline_sorting_" + baseline_type;
    SKA_BENCHMARK_NAME(benchmark_sort_baseline<T>, "baseline", baseline_name);
    std::string benchmark_type = "sorting";

    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_std_sort<T>, categories_so_far.BuildCategories(benchmark_type, "std::sort"))->SetBaseline(baseline_name));
    //SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_american_flag_sort<T>, categories_so_far.BuildCategories(benchmark_type, "american_flag_sort"))->SetBaseline(baseline_name));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_ska_sort<T>, categories_so_far.BuildCategories(benchmark_type, "ska_sort"))->SetBaseline(baseline_name));
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_ska_sort2<T>, categories_so_far.BuildCategories(benchmark_type, "ska_sort2"))->SetBaseline(baseline_name));
    if constexpr (!std::is_same_v<T, std::string>)
    {
        SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_ska_sort_ping_pong<T>, categories_so_far.BuildCategories(benchmark_type, "ska_sort_ping_pong"))->SetBaseline(baseline_name));
    }
    SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_counting_sort_ping_pong<T>, categories_so_far.BuildCategories(benchmark_type, "counting_sort_ping_pong"))->SetBaseline(baseline_name));


    //auto with_uint32 = &benchmark_counting_sort_ping_pong<T, Uint32SortSettings>;
    //SortRange<T>(SKA_BENCHMARK_CATEGORIES(with_uint32, categories_so_far.AddCategory("count_type", "uint32").BuildCategories(benchmark_type, "counting_sort_ping_pong"))->SetBaseline(baseline_name));

    //auto with_int32 = &benchmark_counting_sort_ping_pong<T, Int32SortSettings>;
    //SortRange<T>(SKA_BENCHMARK_CATEGORIES(with_int32, categories_so_far.AddCategory("count_type", "int32").BuildCategories(benchmark_type, "counting_sort_ping_pong"))->SetBaseline(baseline_name));

    //RegisterSortForTypeAndFallback<T, 1>(categories_so_far, benchmark_type, baseline_name);
    //RegisterSortForTypeAndFallback<T, 4>(categories_so_far, benchmark_type, baseline_name);
    //RegisterSortForTypeAndFallback<T, 8>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndFallback<T, 16>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndFallback<T, 24>(categories_so_far, benchmark_type, baseline_name);
    RegisterSortForTypeAndFallback<T, 32>(categories_so_far, benchmark_type, baseline_name);
    //RegisterSortForTypeAndFallback<T, 64>(categories_so_far, benchmark_type, baseline_name);

    //RegisterSortForTypeAndAmericanFlagSortLimit<T, 1024>(categories_so_far, benchmark_type, baseline_name);
    //RegisterSortForTypeAndAmericanFlagSortLimit<T, 1536>(categories_so_far, benchmark_type, baseline_name);
    //RegisterSortForTypeAndAmericanFlagSortLimit<T, 2048>(categories_so_far, benchmark_type, baseline_name);
    //RegisterSortForTypeAndAmericanFlagSortLimit<T, 3072>(categories_so_far, benchmark_type, baseline_name);
    //RegisterSortForTypeAndAmericanFlagSortLimit<T, 4096>(categories_so_far, benchmark_type, baseline_name);

    if constexpr (!IsIntWithPadding<T>::value)
    {
        SortRange<T>(SKA_BENCHMARK_CATEGORIES(&benchmark_spreadsort<T>, categories_so_far.BuildCategories(benchmark_type, "boost::spreadsort"))->SetBaseline(baseline_name));
    }
    else if constexpr (sizeof(T) <= raduls::MAX_REC_SIZE_IN_BYTES)
    {
        SortRange<T>(SKA_BENCHMARK_CATEGORIES(benchmark_raduls<T>{1}, categories_so_far.AddCategory("num threads", "1").BuildCategories(benchmark_type, "RADULS2"))->SetBaseline(baseline_name));
        SortRange<T>(SKA_BENCHMARK_CATEGORIES(benchmark_raduls<T>{2}, categories_so_far.AddCategory("num threads", "2").BuildCategories(benchmark_type, "RADULS2"))->SetBaseline(baseline_name));
        SortRange<T>(SKA_BENCHMARK_CATEGORIES(benchmark_raduls<T>{4}, categories_so_far.AddCategory("num threads", "4").BuildCategories(benchmark_type, "RADULS2"))->SetBaseline(baseline_name));
        SortRange<T>(SKA_BENCHMARK_CATEGORIES(benchmark_raduls<T>{8}, categories_so_far.AddCategory("num threads", "8").BuildCategories(benchmark_type, "RADULS2"))->SetBaseline(baseline_name));
        SortRange<T>(SKA_BENCHMARK_CATEGORIES(benchmark_raduls<T>{16}, categories_so_far.AddCategory("num threads", "16").BuildCategories(benchmark_type, "RADULS2"))->SetBaseline(baseline_name));
    }
    SKA_BENCHMARK_CATEGORIES(&benchmark_insertion_sort<T>, categories_so_far.BuildCategories(benchmark_type, "insertion sort"))->SetBaseline(baseline_name)->SetRange(4, 1024)->SetRangeMultiplier(std::sqrt(2.0));
    SKA_BENCHMARK_CATEGORIES(&benchmark_index_sort<T>, categories_so_far.BuildCategories(benchmark_type, "index sort"))->SetBaseline(baseline_name)->SetRange(4, 32)->SetRangeMultiplier(std::sqrt(2.0));
}

void RegisterSorting()
{
    skb::CategoryBuilder categories_so_far;
    RegisterSortForType<int>(categories_so_far, "int", "int");
    RegisterSortForType<uint8_t>(categories_so_far, "uint8_t", "uint8_t");
    RegisterSortForType<float>(categories_so_far, "float", "float");
    RegisterSortForType<std::string>(categories_so_far, "string", "string");

    RegisterSortForType<IntWithPadding<8>>(categories_so_far.AddCategory("struct size", "8"), "int", "int_size_8");
    RegisterSortForType<IntWithPadding<16>>(categories_so_far.AddCategory("struct size", "16"), "int", "int_size_16");
    RegisterSortForType<IntWithPadding<32>>(categories_so_far.AddCategory("struct size", "32"), "int", "int_size_32");
    RegisterSortForType<IntWithPadding<64>>(categories_so_far.AddCategory("struct size", "64"), "int", "int_size_64");
}

#endif

