#include "util/heap.hpp"


#ifndef DISABLE_TESTS
#include "test/include_test.hpp"
#include <random>
#include <atomic>

TEST(interval_heap, is_valid)
{
    std::vector<int> heap;
    heap.push_back(1);
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));

    heap.push_back(0);
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 1;
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 100;
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));

    heap.push_back(200);
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 0;
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 10;
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));

    heap.push_back(0);
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 200;
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 5;
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 90;
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));

    heap.push_back(0);
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 200;
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 5;
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));

    heap.push_back(0);
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 200;
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 5;
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 4;
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 80;
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));

    heap.push_back(0);
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 200;
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 5;
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 95;
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 10;
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 90;
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));

    heap.push_back(0);
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 89;
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 91;
    ASSERT_FALSE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.back() = 90;
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
}

TEST(interval_heap, push)
{
    std::vector<int> heap;
    heap.push_back(5);
    interval_heap_push(heap.begin(), heap.end());
    heap.push_back(10);
    interval_heap_push(heap.begin(), heap.end());
    ASSERT_EQ(5, heap[0]);
    ASSERT_EQ(10, heap[1]);
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.push_back(2);
    interval_heap_push(heap.begin(), heap.end());
    ASSERT_EQ(2, heap[0]);
    ASSERT_EQ(10, heap[1]);
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.push_back(12);
    interval_heap_push(heap.begin(), heap.end());
    ASSERT_EQ(2, heap[0]);
    ASSERT_EQ(12, heap[1]);
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.push_back(7);
    interval_heap_push(heap.begin(), heap.end());
    ASSERT_EQ(2, heap[0]);
    ASSERT_EQ(12, heap[1]);
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.push_back(17);
    interval_heap_push(heap.begin(), heap.end());
    ASSERT_EQ(2, heap[0]);
    ASSERT_EQ(17, heap[1]);
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.push_back(16);
    interval_heap_push(heap.begin(), heap.end());
    ASSERT_EQ(2, heap[0]);
    ASSERT_EQ(17, heap[1]);
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    heap.push_back(0);
    interval_heap_push(heap.begin(), heap.end());
    ASSERT_EQ(0, heap[0]);
    ASSERT_EQ(17, heap[1]);
    ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
}

TEST(interval_heap, push_random)
{
    std::mt19937_64 randomness;
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap;
    for (int i = 0; i < 100; ++i)
    {
        heap.push_back(distribution(randomness));
        interval_heap_push(heap.begin(), heap.end());
        ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    }
    std::vector<int> sort_by_min = heap;
    for (auto it = sort_by_min.end(); it != sort_by_min.begin(); --it)
    {
        int smallest = interval_heap_min(sort_by_min.begin(), it);
        interval_heap_pop_min(sort_by_min.begin(), it);
        ASSERT_EQ(smallest, it[-1]);
        ASSERT_TRUE(interval_heap_is_valid(sort_by_min.begin(), it - 1));
    }
    ASSERT_TRUE(std::is_sorted(sort_by_min.begin(), sort_by_min.end(), std::greater<>{}));

    ASSERT_TRUE(std::is_permutation(heap.begin(), heap.end(), sort_by_min.begin(), sort_by_min.end()));

    std::vector<int> sort_by_max = heap;
    for (auto it = sort_by_max.end(); it != sort_by_max.begin(); --it)
    {
        int largest = interval_heap_max(sort_by_max.begin(), it);
        interval_heap_pop_max(sort_by_max.begin(), it);
        ASSERT_EQ(largest, it[-1]);
        ASSERT_TRUE(interval_heap_is_valid(sort_by_max.begin(), it - 1));
    }
    ASSERT_TRUE(std::is_sorted(sort_by_max.begin(), sort_by_max.end()));
    ASSERT_TRUE(std::equal(sort_by_min.begin(), sort_by_min.end(), sort_by_max.rbegin(), sort_by_max.rend()));
}

TEST(interval_heap, make)
{
    std::mt19937_64 randomness;
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap;
    for (int i = 0; i < 100; ++i)
    {
        std::shuffle(heap.begin(), heap.end(), randomness);
        heap.push_back(distribution(randomness));
        interval_heap_make(heap.begin(), heap.end());
        ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    }
}

TEST(interval_heap, min_updated)
{
    std::mt19937_64 randomness;
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap(50);
    for (int i = 0; i < 200; ++i)
    {
        interval_heap_min(heap.begin(), heap.end()) = distribution(randomness);
        interval_heap_min_updated(heap.begin(), heap.end());
        ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    }
}

TEST(interval_heap, max_updated)
{
    std::mt19937_64 randomness;
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap;
    for (int i = 0; i < 200; ++i)
    {
        heap.push_back(distribution(randomness));
        interval_heap_push(heap.begin(), heap.end());
        ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
        interval_heap_max(heap.begin(), heap.end()) = distribution(randomness);
        interval_heap_max_updated(heap.begin(), heap.end());
        ASSERT_TRUE(interval_heap_is_valid(heap.begin(), heap.end()));
    }
}

TEST(minmax_heap, highest_set_bit)
{
    ASSERT_EQ(0, minmax_heap_helpers::highest_set_bit(1));
    ASSERT_EQ(1, minmax_heap_helpers::highest_set_bit(2));
    ASSERT_EQ(2, minmax_heap_helpers::highest_set_bit(4));
    ASSERT_EQ(3, minmax_heap_helpers::highest_set_bit(8));
    ASSERT_EQ(5, minmax_heap_helpers::highest_set_bit(55));
}

TEST(minmax_heap, is_min_item)
{
    ASSERT_TRUE(minmax_heap_helpers::is_min_item(0));

    ASSERT_FALSE(minmax_heap_helpers::is_min_item(1));
    ASSERT_FALSE(minmax_heap_helpers::is_min_item(2));

    ASSERT_TRUE(minmax_heap_helpers::is_min_item(3));
    ASSERT_TRUE(minmax_heap_helpers::is_min_item(4));
    ASSERT_TRUE(minmax_heap_helpers::is_min_item(5));
    ASSERT_TRUE(minmax_heap_helpers::is_min_item(6));

    ASSERT_FALSE(minmax_heap_helpers::is_min_item(7));
    ASSERT_FALSE(minmax_heap_helpers::is_min_item(8));
    ASSERT_FALSE(minmax_heap_helpers::is_min_item(9));
    ASSERT_FALSE(minmax_heap_helpers::is_min_item(10));
    ASSERT_FALSE(minmax_heap_helpers::is_min_item(11));
    ASSERT_FALSE(minmax_heap_helpers::is_min_item(12));
    ASSERT_FALSE(minmax_heap_helpers::is_min_item(13));
    ASSERT_FALSE(minmax_heap_helpers::is_min_item(14));
}

TEST(minmax_heap, parent_index)
{
    ASSERT_EQ(0, minmax_heap_helpers::parent_index(1));
    ASSERT_EQ(0, minmax_heap_helpers::parent_index(2));

    ASSERT_EQ(1, minmax_heap_helpers::parent_index(3));
    ASSERT_EQ(1, minmax_heap_helpers::parent_index(4));

    ASSERT_EQ(2, minmax_heap_helpers::parent_index(5));
    ASSERT_EQ(2, minmax_heap_helpers::parent_index(6));

    ASSERT_EQ(0, minmax_heap_helpers::grandparent_index(3));
    ASSERT_EQ(0, minmax_heap_helpers::grandparent_index(4));
    ASSERT_EQ(0, minmax_heap_helpers::grandparent_index(5));
    ASSERT_EQ(0, minmax_heap_helpers::grandparent_index(6));

    ASSERT_EQ(3, minmax_heap_helpers::parent_index(7));
    ASSERT_EQ(3, minmax_heap_helpers::parent_index(8));

    ASSERT_EQ(4, minmax_heap_helpers::parent_index(9));
    ASSERT_EQ(4, minmax_heap_helpers::parent_index(10));

    ASSERT_EQ(1, minmax_heap_helpers::grandparent_index(7));
    ASSERT_EQ(1, minmax_heap_helpers::grandparent_index(8));
    ASSERT_EQ(1, minmax_heap_helpers::grandparent_index(9));
    ASSERT_EQ(1, minmax_heap_helpers::grandparent_index(10));

    ASSERT_EQ(5, minmax_heap_helpers::parent_index(11));
    ASSERT_EQ(5, minmax_heap_helpers::parent_index(12));

    ASSERT_EQ(6, minmax_heap_helpers::parent_index(13));
    ASSERT_EQ(6, minmax_heap_helpers::parent_index(14));

    ASSERT_EQ(2, minmax_heap_helpers::grandparent_index(11));
    ASSERT_EQ(2, minmax_heap_helpers::grandparent_index(12));
    ASSERT_EQ(2, minmax_heap_helpers::grandparent_index(13));
    ASSERT_EQ(2, minmax_heap_helpers::grandparent_index(14));

    ASSERT_EQ(7, minmax_heap_helpers::parent_index(15));
    ASSERT_EQ(7, minmax_heap_helpers::parent_index(16));

    ASSERT_EQ(3, minmax_heap_helpers::grandparent_index(15));
    ASSERT_EQ(3, minmax_heap_helpers::grandparent_index(16));
}

TEST(minmax_heap, child_index)
{
    ASSERT_EQ(1, minmax_heap_helpers::first_child_index(0));
    ASSERT_EQ(3, minmax_heap_helpers::first_child_index(1));
    ASSERT_EQ(5, minmax_heap_helpers::first_child_index(2));
    ASSERT_EQ(7, minmax_heap_helpers::first_child_index(3));
    ASSERT_EQ(9, minmax_heap_helpers::first_child_index(4));
}

TEST(minmax_heap, is_minmax_heap)
{
    std::vector<int> a = { 8, 71, 41, 31, 10, 11, 16, 46, 51, 31, 21, 13 };
    for (auto it = a.begin() + 1;; ++it)
    {
        ASSERT_TRUE(is_minmax_heap(a.begin(), it));
        if (it == a.end())
            break;
    }
    ASSERT_FALSE(is_minmax_heap(a.begin() + 1, a.end()));
    ASSERT_FALSE(is_minmax_heap(a.begin() + 2, a.end()));
    ASSERT_FALSE(is_minmax_heap(a.begin() + 3, a.end()));
    ASSERT_FALSE(is_minmax_heap(a.begin() + 4, a.end()));
}

TEST(minmax_heap, push_pop)
{
    int num = 100;
    for (int seed = 0; seed < 10; ++seed)
    {
        std::mt19937_64 randomness(seed);
        std::uniform_int_distribution<int> distribution(0, 100);
        std::vector<int> heap;
        heap.reserve(num);
        for (int i = 0; i < num; ++i)
        {
            heap.push_back(distribution(randomness));
            push_minmax_heap(heap.begin(), heap.end());
            ASSERT_TRUE(is_minmax_heap(heap.begin(), heap.end()));
        }
        std::vector<int> sort_by_min = heap;
        for (int i = 0; i < num; ++i)
        {
            ASSERT_TRUE(is_minmax_heap(sort_by_min.begin(), sort_by_min.end() - i));
            pop_minmax_heap_min(sort_by_min.begin(), sort_by_min.end() - i);
        }
        std::vector<int> sorted = heap;
        std::sort(sorted.begin(), sorted.end(), std::greater<>{});
        ASSERT_EQ(sorted, sort_by_min);
        std::vector<int> sort_by_max = heap;
        for (int i = 0; i < num; ++i)
        {
            ASSERT_TRUE(is_minmax_heap(sort_by_max.begin(), sort_by_max.end() - i));
            pop_minmax_heap_max(sort_by_max.begin(), sort_by_max.end() - i);
        }
        std::sort(sorted.begin(), sorted.end());
        ASSERT_EQ(sorted, sort_by_max);
    }
}

TEST(minmax_heap, make_minmax_heap)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    std::vector<int> control;
    control.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        std::shuffle(heap.begin(), heap.end(), randomness);
        heap.push_back(distribution(randomness));
        control.push_back(heap.back());
        make_minmax_heap(heap.begin(), heap.end());
        ASSERT_TRUE(is_minmax_heap(heap.begin(), heap.end()));
        for (int j = 0; j < i; ++j)
        {
            pop_minmax_heap_max(heap.begin(), heap.end() - j);
        }
        std::sort(control.begin(), control.end());
        ASSERT_EQ(control, heap);
    }
}

TEST(dary_heap, parent_index)
{
    ASSERT_EQ(0, dary_heap_helpers::parent_index<2>(1));
    ASSERT_EQ(0, dary_heap_helpers::parent_index<2>(2));

    ASSERT_EQ(1, dary_heap_helpers::parent_index<2>(3));
    ASSERT_EQ(1, dary_heap_helpers::parent_index<2>(4));

    ASSERT_EQ(2, dary_heap_helpers::parent_index<2>(5));
    ASSERT_EQ(2, dary_heap_helpers::parent_index<2>(6));

    ASSERT_EQ(0, dary_heap_helpers::grandparent_index<2>(3));
    ASSERT_EQ(0, dary_heap_helpers::grandparent_index<2>(4));
    ASSERT_EQ(0, dary_heap_helpers::grandparent_index<2>(5));
    ASSERT_EQ(0, dary_heap_helpers::grandparent_index<2>(6));

    ASSERT_EQ(1, dary_heap_helpers::grandparent_index<2>(7));
    ASSERT_EQ(1, dary_heap_helpers::grandparent_index<2>(8));
    ASSERT_EQ(1, dary_heap_helpers::grandparent_index<2>(9));
    ASSERT_EQ(1, dary_heap_helpers::grandparent_index<2>(10));

    ASSERT_EQ(2, dary_heap_helpers::grandparent_index<2>(11));
    ASSERT_EQ(2, dary_heap_helpers::grandparent_index<2>(12));
    ASSERT_EQ(2, dary_heap_helpers::grandparent_index<2>(13));
    ASSERT_EQ(2, dary_heap_helpers::grandparent_index<2>(14));

    ASSERT_EQ(0, dary_heap_helpers::parent_index<3>(1));
    ASSERT_EQ(0, dary_heap_helpers::parent_index<3>(2));
    ASSERT_EQ(0, dary_heap_helpers::parent_index<3>(3));

    ASSERT_EQ(1, dary_heap_helpers::parent_index<3>(4));
    ASSERT_EQ(1, dary_heap_helpers::parent_index<3>(5));
    ASSERT_EQ(1, dary_heap_helpers::parent_index<3>(6));

    ASSERT_EQ(2, dary_heap_helpers::parent_index<3>(7));
    ASSERT_EQ(2, dary_heap_helpers::parent_index<3>(8));
    ASSERT_EQ(2, dary_heap_helpers::parent_index<3>(9));

    for (int i = 4; i <= 12; ++i)
        ASSERT_EQ(0, dary_heap_helpers::grandparent_index<3>(i));
    for (int i = 13; i <= 21; ++i)
        ASSERT_EQ(1, dary_heap_helpers::grandparent_index<3>(i));
    for (int i = 22; i <= 30; ++i)
        ASSERT_EQ(2, dary_heap_helpers::grandparent_index<3>(i));

    for (int i = 1; i <= 4; ++i)
        ASSERT_EQ(0, dary_heap_helpers::parent_index<4>(i));
    for (int i = 5; i <= 8; ++i)
        ASSERT_EQ(1, dary_heap_helpers::parent_index<4>(i));
    for (int i = 9; i <= 12; ++i)
        ASSERT_EQ(2, dary_heap_helpers::parent_index<4>(i));

    for (int i = 5; i <= 20; ++i)
        ASSERT_EQ(0, dary_heap_helpers::grandparent_index<4>(i));
    for (int i = 21; i <= 36; ++i)
        ASSERT_EQ(1, dary_heap_helpers::grandparent_index<4>(i));
    for (int i = 37; i <= 52; ++i)
        ASSERT_EQ(2, dary_heap_helpers::grandparent_index<4>(i));
}

TEST(dary_heap, is_dary_heap)
{
    std::vector<int> binary_heap = { 100, 19, 36, 17, 3, 25, 1, 2, 7, 5 };
    ASSERT_TRUE(is_dary_heap<2>(binary_heap.begin(), binary_heap.end() - 1));
    ASSERT_FALSE(is_dary_heap<2>(binary_heap.begin(), binary_heap.end()));

    std::vector<int> ternary_heap = { 100, 19, 17, 25, 3, 12, 0, 16, 14, 2, 20, 10, 20, 2, 1, 5 };
    ASSERT_TRUE(is_dary_heap<3>(ternary_heap.begin(), ternary_heap.end() - 1));
    ASSERT_FALSE(is_dary_heap<3>(ternary_heap.begin(), ternary_heap.end()));
}

TEST(dary_heap, make_dary_heap)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        std::shuffle(heap.begin(), heap.end(), randomness);
        heap.push_back(distribution(randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        ASSERT_TRUE(is_dary_heap<2>(heap.begin(), heap.end()));
        ASSERT_TRUE(std::is_heap(heap.begin(), heap.end()));
        std::shuffle(heap.begin(), heap.end(), randomness);
        make_dary_heap<3>(heap.begin(), heap.end());
        ASSERT_TRUE(is_dary_heap<3>(heap.begin(), heap.end()));
        std::shuffle(heap.begin(), heap.end(), randomness);
        make_dary_heap<4>(heap.begin(), heap.end());
        ASSERT_TRUE(is_dary_heap<4>(heap.begin(), heap.end()));
        std::shuffle(heap.begin(), heap.end(), randomness);
        make_dary_heap<5>(heap.begin(), heap.end());
        ASSERT_TRUE(is_dary_heap<5>(heap.begin(), heap.end()));
        std::shuffle(heap.begin(), heap.end(), randomness);
        make_dary_heap<6>(heap.begin(), heap.end());
        ASSERT_TRUE(is_dary_heap<6>(heap.begin(), heap.end()));
        std::shuffle(heap.begin(), heap.end(), randomness);
        make_dary_heap<7>(heap.begin(), heap.end());
        ASSERT_TRUE(is_dary_heap<7>(heap.begin(), heap.end()));
        std::shuffle(heap.begin(), heap.end(), randomness);
        make_dary_heap<8>(heap.begin(), heap.end());
        ASSERT_TRUE(is_dary_heap<8>(heap.begin(), heap.end()));
    }
}

TEST(dary_heap, push_dary_heap)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap[14];
    int num = 100;
    for (int i = 0; i < 14; ++i)
        heap[i].reserve(num);
    for (int i = 0; i < num; ++i)
    {
        for (int j = 0; j < 14; ++j)
            heap[j].push_back(distribution(randomness));

        int j = 0;
        push_dary_heap<2>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<2>(heap[j].begin(), heap[j].end()));
        ASSERT_TRUE(std::is_heap(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap<3>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<3>(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap<4>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<4>(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap<5>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<5>(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap<6>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<6>(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap<7>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<7>(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap<8>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<8>(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap_grandparent<2>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<2>(heap[j].begin(), heap[j].end()));
        ASSERT_TRUE(std::is_heap(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap_grandparent<3>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<3>(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap_grandparent<4>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<4>(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap_grandparent<5>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<5>(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap_grandparent<6>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<6>(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap_grandparent<7>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<7>(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap_grandparent<8>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<8>(heap[j].begin(), heap[j].end()));
    }
}
TEST(dary_heap, push_dary_heap_great_grandparent)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 1000);
    std::vector<int> heap[2];
    int num = 100;
    for (int i = 0; i < 2; ++i)
        heap[i].reserve(num);
    for (int i = 0; i < num; ++i)
    {
        for (int j = 0; j < 2; ++j)
            heap[j].push_back(distribution(randomness));

        int j = 0;
        push_dary_heap_great_grandparent<2>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<2>(heap[j].begin(), heap[j].end()));
        ASSERT_TRUE(std::is_heap(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap_great_grandparent<4>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<4>(heap[j].begin(), heap[j].end()));
    }
}
TEST(dary_heap, push_dary_heap_great_great_grandparent)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 1000);
    std::vector<int> heap[2];
    int num = 100;
    for (int i = 0; i < 2; ++i)
        heap[i].reserve(num);
    for (int i = 0; i < num; ++i)
    {
        for (int j = 0; j < 2; ++j)
            heap[j].push_back(distribution(randomness));

        int j = 0;
        push_dary_heap_great_great_grandparent<2>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<2>(heap[j].begin(), heap[j].end()));
        ASSERT_TRUE(std::is_heap(heap[j].begin(), heap[j].end()));

        ++j;
        push_dary_heap_great_great_grandparent<4>(heap[j].begin(), heap[j].end());
        ASSERT_TRUE(is_dary_heap<4>(heap[j].begin(), heap[j].end()));
    }
}
TEST(dary_heap, push_binary_heap_binary_search)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 1000);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        heap.push_back(distribution(randomness));

        //push_binary_heap_binary_search(heap.begin(), heap.end());
        push_binary_heap_binary_search(heap.data(), heap.data() + heap.size());
        if (!is_dary_heap<2>(heap.begin(), heap.end()))
            std::cout << i << std::endl;
        ASSERT_TRUE(is_dary_heap<2>(heap.begin(), heap.end()));
        ASSERT_TRUE(std::is_heap(heap.begin(), heap.end()));
    }
}
TEST(dary_heap, push_binary_heap_binary_search_plus_one)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 1000);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        heap.push_back(distribution(randomness));

        push_binary_heap_binary_search_plus_one(heap.begin(), heap.end());
        ASSERT_TRUE(is_dary_heap<2>(heap.begin(), heap.end()));
        ASSERT_TRUE(std::is_heap(heap.begin(), heap.end()));
    }
}
struct CountingComparer
{
    static std::atomic<size_t> num_comparisons;

    template<typename T, typename U>
    bool operator()(const T & l, const U & r) const
    {
        num_comparisons.fetch_add(1, std::memory_order_relaxed);
        return l < r;
    }
};
std::atomic<size_t> CountingComparer::num_comparisons{0};

TEST(dary_heap, DISABLED_push_count)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 1000);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    std::vector<size_t> comparison_counts;
    for (int i = 0; i < num; ++i)
    {
        heap.push_back(distribution(randomness));

        size_t num_comparisons_before = CountingComparer::num_comparisons.load(std::memory_order_relaxed);
        push_dary_heap<2>(heap.begin(), heap.end(), CountingComparer());
        size_t num_comparisons = CountingComparer::num_comparisons.load(std::memory_order_relaxed) - num_comparisons_before;
        if (num_comparisons >= comparison_counts.size())
            comparison_counts.resize(num_comparisons + 1);
        ++comparison_counts[num_comparisons];
    }
    for (size_t i = 0; i < comparison_counts.size(); ++i)
    {
        std::cout << i << ": " << comparison_counts[i] << std::endl;
    }
}
TEST(dary_heap, DISABLED_push_count_one_only)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 1000);
    std::vector<int> heap;
    int num = 16383;
    heap.reserve(num);
    std::vector<size_t> comparison_counts;
    for (int i = 0; i < num; ++i)
    {
        heap.push_back(distribution(randomness));
    }
    for (int i = 0; i < num; ++i)
    {
        std::shuffle(heap.begin(), heap.end(), randomness);
        make_dary_heap<2>(heap.begin(), heap.end() - 1);
        size_t num_comparisons_before = CountingComparer::num_comparisons.load(std::memory_order_relaxed);
        push_dary_heap<2>(heap.begin(), heap.end(), CountingComparer());
        size_t num_comparisons = CountingComparer::num_comparisons.load(std::memory_order_relaxed) - num_comparisons_before;
        if (num_comparisons >= comparison_counts.size())
            comparison_counts.resize(num_comparisons + 1);
        ++comparison_counts[num_comparisons];
    }
    for (size_t i = 0; i < comparison_counts.size(); ++i)
    {
        std::cout << i << ": " << comparison_counts[i] << std::endl;
    }
}
TEST(dary_heap, pop_dary_heap)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap[7];
    int num = 100;
    for (int i = 0; i < 7; ++i)
        heap[i].reserve(num);
    std::vector<int> sorted;
    sorted.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        int to_push = distribution(randomness);
        for (int j = 0; j < 7; ++j)
            heap[j].push_back(to_push);
        sorted.push_back(to_push);
    }
    make_dary_heap<2>(heap[0].begin(), heap[0].end());
    make_dary_heap<3>(heap[1].begin(), heap[1].end());
    make_dary_heap<4>(heap[2].begin(), heap[2].end());
    make_dary_heap<5>(heap[3].begin(), heap[3].end());
    make_dary_heap<6>(heap[4].begin(), heap[4].end());
    make_dary_heap<7>(heap[5].begin(), heap[5].end());
    make_dary_heap<8>(heap[6].begin(), heap[6].end());
    for (int i = 0; i < num; ++i)
    {
        pop_dary_heap<2>(heap[0].begin(), heap[0].end() - i);
        ASSERT_TRUE(is_dary_heap<2>(heap[0].begin(), heap[0].end() - i - 1));
        ASSERT_TRUE(std::is_heap(heap[0].begin(), heap[0].end() - i - 1));

        pop_dary_heap<3>(heap[1].begin(), heap[1].end() - i);
        ASSERT_TRUE(is_dary_heap<3>(heap[1].begin(), heap[1].end() - i - 1));

        pop_dary_heap<4>(heap[2].begin(), heap[2].end() - i);
        ASSERT_TRUE(is_dary_heap<4>(heap[2].begin(), heap[2].end() - i - 1));

        pop_dary_heap<5>(heap[3].begin(), heap[3].end() - i);
        ASSERT_TRUE(is_dary_heap<5>(heap[3].begin(), heap[3].end() - i - 1));

        pop_dary_heap<6>(heap[4].begin(), heap[4].end() - i);
        ASSERT_TRUE(is_dary_heap<6>(heap[4].begin(), heap[4].end() - i - 1));

        pop_dary_heap<7>(heap[5].begin(), heap[5].end() - i);
        ASSERT_TRUE(is_dary_heap<7>(heap[5].begin(), heap[5].end() - i - 1));

        pop_dary_heap<8>(heap[6].begin(), heap[6].end() - i);
        ASSERT_TRUE(is_dary_heap<8>(heap[6].begin(), heap[6].end() - i - 1));
    }
    std::sort(sorted.begin(), sorted.end());
    for (int i = 0; i < 7; ++i)
    {
        ASSERT_EQ(sorted, heap[i]);
    }
}

TEST(dary_heap, pop_binary_heap_grandparent)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    std::vector<int> sorted;
    sorted.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        int to_push = distribution(randomness);
        heap.push_back(to_push);
        sorted.push_back(to_push);
    }
    std::sort(sorted.begin(), sorted.end());
    make_dary_heap<2>(heap.begin(), heap.end());
    for (int i = 0; i < num; ++i)
    {
        pop_binary_heap_grandparent(heap.begin(), heap.end() - i);
        ASSERT_TRUE(is_dary_heap<2>(heap.begin(), heap.end() - i - 1));
        ASSERT_TRUE(std::is_heap(heap.begin(), heap.end() - i - 1));
    }
    ASSERT_EQ(sorted, heap);
}
TEST(dary_heap, pop_binary_heap_unrolled_grandparent)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 1000);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    std::vector<int> sorted;
    sorted.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        int to_push = distribution(randomness);
        heap.push_back(to_push);
        sorted.push_back(to_push);
    }
    std::sort(sorted.begin(), sorted.end());
    make_dary_heap<2>(heap.begin(), heap.end());
    for (int i = 0; i < num; ++i)
    {
        pop_binary_heap_unrolled_grandparent(heap.begin(), heap.end() - i);
        ASSERT_TRUE(is_dary_heap<2>(heap.begin(), heap.end() - i - 1));
        ASSERT_TRUE(std::is_heap(heap.begin(), heap.end() - i - 1));
    }
    ASSERT_EQ(sorted, heap);
}
TEST(dary_heap, pop_quaternary_heap_grandparent)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    std::vector<int> sorted;
    sorted.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        int to_push = distribution(randomness);
        heap.push_back(to_push);
        sorted.push_back(to_push);
    }
    std::sort(sorted.begin(), sorted.end());
    make_dary_heap<4>(heap.begin(), heap.end());
    for (int i = 0; i < num; ++i)
    {
        pop_quaternary_heap_grandparent(heap.begin(), heap.end() - i);
        ASSERT_TRUE(is_dary_heap<4>(heap.begin(), heap.end() - i - 1));
    }
    ASSERT_EQ(sorted, heap);
}

TEST(dary_heap, pop_binary_heap_unrolled)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap;
    std::vector<int> sorted;
    int num = 100;
    heap.reserve(num);
    sorted.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        int to_push = distribution(randomness);
        heap.push_back(to_push);
        sorted.push_back(to_push);
    }
    std::sort(sorted.begin(), sorted.end());
    make_dary_heap<2>(heap.begin(), heap.end());
    for (int i = 0; i < num; ++i)
    {
        pop_binary_heap_unrolled(heap.begin(), heap.end() - i);
        ASSERT_TRUE(is_dary_heap<2>(heap.begin(), heap.end() - i - 1));
        ASSERT_TRUE(std::is_heap(heap.begin(), heap.end() - i - 1));
    }
    ASSERT_EQ(sorted, heap);
}

TEST(dary_heap, pop_binary_heap_unrolled_8)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    std::vector<int> sorted;
    sorted.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        int to_push = distribution(randomness);
        heap.push_back(to_push);
        sorted.push_back(to_push);
    }
    std::sort(sorted.begin(), sorted.end());
    make_dary_heap<2>(heap.begin(), heap.end());
    for (int i = 0; i < num; ++i)
    {
        pop_binary_heap_unrolled_8(heap.begin(), heap.end() - i);
        ASSERT_TRUE(is_dary_heap<2>(heap.begin(), heap.end() - i - 1));
        ASSERT_TRUE(std::is_heap(heap.begin(), heap.end() - i - 1));
    }
    ASSERT_EQ(sorted, heap);
}

TEST(dary_heap, pop_binary_heap_unrolled_2)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    std::vector<int> sorted;
    sorted.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        int to_push = distribution(randomness);
        heap.push_back(to_push);
        sorted.push_back(to_push);
    }
    std::sort(sorted.begin(), sorted.end());
    make_dary_heap<2>(heap.begin(), heap.end());
    for (int i = 0; i < num; ++i)
    {
        pop_binary_heap_unrolled_2(heap.begin(), heap.end() - i);
        ASSERT_TRUE(is_dary_heap<2>(heap.begin(), heap.end() - i - 1));
        ASSERT_TRUE(std::is_heap(heap.begin(), heap.end() - i - 1));
    }
    ASSERT_EQ(sorted, heap);
}

TEST(dary_heap, pop_binary_heap_unrolled_no_early_out)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    std::vector<int> sorted;
    sorted.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        int to_push = distribution(randomness);
        heap.push_back(to_push);
        sorted.push_back(to_push);
    }
    std::sort(sorted.begin(), sorted.end());
    make_dary_heap<2>(heap.begin(), heap.end());
    for (int i = 0; i < num; ++i)
    {
        pop_binary_heap_unrolled_no_early_out(heap.begin(), heap.end() - i);
        ASSERT_TRUE(is_dary_heap<2>(heap.begin(), heap.end() - i - 1));
        ASSERT_TRUE(std::is_heap(heap.begin(), heap.end() - i - 1));
    }
    ASSERT_EQ(sorted, heap);
}

TEST(dary_heap, pop_quaternary_heap_unrolled)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 1000);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    std::vector<int> sorted;
    sorted.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        int to_push = distribution(randomness);
        heap.push_back(to_push);
        sorted.push_back(to_push);
    }
    std::sort(sorted.begin(), sorted.end());
    make_dary_heap<4>(heap.begin(), heap.end());
    for (int i = 0; i < num; ++i)
    {
        pop_quaternary_heap_unrolled(heap.begin(), heap.end() - i);
        ASSERT_TRUE(is_dary_heap<4>(heap.begin(), heap.end() - i - 1));
    }
    ASSERT_EQ(sorted, heap);
}

TEST(dary_heap, pop_quaternary_heap_unrolled_2)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 1000);
    std::vector<int> heap;
    int num = 100;
    heap.reserve(num);
    std::vector<int> sorted;
    sorted.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        int to_push = distribution(randomness);
        heap.push_back(to_push);
        sorted.push_back(to_push);
    }
    std::sort(sorted.begin(), sorted.end());
    make_dary_heap<4>(heap.begin(), heap.end());
    for (int i = 0; i < num; ++i)
    {
        pop_quaternary_heap_unrolled_2(heap.begin(), heap.end() - i);
        ASSERT_TRUE(is_dary_heap<4>(heap.begin(), heap.end() - i - 1));
    }
    ASSERT_EQ(sorted, heap);
}

TEST(pairing_heap, sort)
{
    std::vector<int> input = { 5, 3, 10, 2, 1, -1 };
    PairingHeap<int>::MemoryPool pool;
    PairingHeap<int> heap;
    for (int i : input)
        heap.insert(i, pool);
    std::vector<int> sorted;
    sorted.reserve(input.size());
    ASSERT_TRUE(pool.empty());
    while (!heap.empty())
    {
        sorted.push_back(heap.min());
        heap.delete_min(pool);
    }
    ASSERT_EQ(input.size(), sorted.size());
    ASSERT_TRUE(std::is_sorted(sorted.begin(), sorted.end()));
    ASSERT_TRUE(std::is_permutation(input.begin(), input.end(), sorted.begin(), sorted.end()));
    heap.clear(pool);
    ASSERT_TRUE(heap.empty());

    for (int i : input)
    {
        ASSERT_FALSE(pool.empty());
        heap.insert(i, pool);
    }
    ASSERT_TRUE(pool.empty());
}

TEST(pairing_heap, pop_bigger)
{
    std::vector<int> input;
    input.reserve(100);
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 100);
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));
    PairingHeap<int>::MemoryPool pool;
    PairingHeap<int> heap;
    for (int i : input)
        heap.insert(i, pool);
    std::vector<int> sorted;
    sorted.reserve(input.size());
    ASSERT_TRUE(pool.empty());
    while (!heap.empty())
    {
        sorted.push_back(heap.min());
        heap.delete_min(pool);
    }
    std::sort(input.begin(), input.end());
    ASSERT_EQ(input, sorted);
}

namespace
{
struct PointerLess
{
    template<typename T>
    bool operator()(const std::unique_ptr<T> & l, const std::unique_ptr<T> & r) const
    {
        return *l < *r;
    }
};
}
TEST(pairing_heap, no_copies)
{
    std::vector<int> input = { 5, 3, 10, 2, 1, -1 };
    PairingHeap<std::unique_ptr<int>, PointerLess>::MemoryPool pool;
    PairingHeap<std::unique_ptr<int>, PointerLess> heap;
    for (int i : input)
        heap.insert(std::make_unique<int>(i), pool);
    std::vector<int> sorted;
    sorted.reserve(input.size());
    while (!heap.empty())
    {
        sorted.push_back(*heap.min());
        heap.delete_min(pool);
    }
    ASSERT_EQ(input.size(), sorted.size());
    ASSERT_TRUE(std::is_sorted(sorted.begin(), sorted.end()));
    ASSERT_TRUE(std::is_permutation(input.begin(), input.end(), sorted.begin(), sorted.end()));
}

TEST(pairing_heap, meld)
{
    std::vector<int> input_a = { 5, 3, 10, 2, 1, -1 };
    std::vector<int> input_b = { 2, 3, 4, 5, 10, 1, -2 };
    PairingHeap<int>::MemoryPool pool;
    PairingHeap<int> heap_a;
    PairingHeap<int> heap_b;
    for (int i : input_a)
        heap_a.insert(i, pool);
    for (int i : input_b)
        heap_b.insert(i, pool);
    std::vector<int> sorted;
    sorted.reserve(input_a.size() + input_b.size());
    ASSERT_TRUE(pool.empty());
    heap_a.meld(std::move(heap_b));
    ASSERT_TRUE(heap_b.empty());
    while (!heap_a.empty())
    {
        sorted.push_back(heap_a.min());
        heap_a.delete_min(pool);
    }
    std::vector<int> expected;
    expected.reserve(input_a.size() + input_b.size());
    expected.insert(expected.end(), input_a.begin(), input_a.end());
    expected.insert(expected.end(), input_b.begin(), input_b.end());
    std::sort(expected.begin(), expected.end());
    ASSERT_EQ(expected, sorted);
}

TEST(pairing_pair_heap, sort)
{
    std::vector<int> input = { 5, 3, 10, 2, 1, -1 };
    PairingHeapPair<int>::MemoryPool pool;
    PairingHeapPair<int> heap;
    for (int i : input)
        heap.insert(i, pool);
    std::vector<int> sorted;
    sorted.reserve(input.size());
    ASSERT_TRUE(pool.empty());
    while (!heap.empty())
    {
        sorted.push_back(heap.min());
        heap.delete_min(pool);
    }
    ASSERT_EQ(input.size(), sorted.size());
    ASSERT_TRUE(std::is_sorted(sorted.begin(), sorted.end()));
    ASSERT_TRUE(std::is_permutation(input.begin(), input.end(), sorted.begin(), sorted.end()));
    heap.clear(pool);
    ASSERT_TRUE(heap.empty());

    for (int i : input)
    {
        ASSERT_FALSE(pool.empty());
        heap.insert(i, pool);
    }
    ASSERT_TRUE(pool.empty());
}

TEST(pairing_pair_heap, sort_more)
{
    std::mt19937_64 randomness;
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> input;
    input.reserve(10000);
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));
    PairingHeapPair<int>::MemoryPool pool;
    PairingHeapPair<int> heap;
    for (int i : input)
        heap.insert(i, pool);
    std::vector<int> sorted;
    sorted.reserve(input.size());
    ASSERT_TRUE(pool.empty());
    while (!heap.empty())
    {
        sorted.push_back(heap.min());
        heap.delete_min(pool);
    }
    ASSERT_EQ(input.size(), sorted.size());
    ASSERT_TRUE(std::is_sorted(sorted.begin(), sorted.end()));
    ASSERT_TRUE(std::is_permutation(input.begin(), input.end(), sorted.begin(), sorted.end()));
    heap.clear(pool);
    ASSERT_TRUE(heap.empty());

    for (int i : input)
    {
        ASSERT_FALSE(pool.empty());
        heap.insert(i, pool);
    }
    ASSERT_TRUE(pool.empty());
}

TEST(pairing_pair_heap, sort_more_loop)
{
    std::mt19937_64 randomness;
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> input;
    input.reserve(10000);
    PairingHeapPair<int> heap;
    PairingHeapPair<int>::MemoryPool pool;
    for (int j = 0; j < 10; ++j)
    {
        input.clear();
        while (input.size() != input.capacity())
            input.push_back(distribution(randomness));
        for (int i : input)
            heap.insert(i, pool);
        std::vector<int> sorted;
        sorted.reserve(input.size());
        ASSERT_TRUE(pool.empty());
        while (!heap.empty())
        {
            sorted.push_back(heap.min());
            heap.delete_min(pool);
        }
        ASSERT_EQ(input.size(), sorted.size());
        ASSERT_TRUE(std::is_sorted(sorted.begin(), sorted.end()));
        ASSERT_TRUE(std::is_permutation(input.begin(), input.end(), sorted.begin(), sorted.end()));
        ASSERT_TRUE(heap.empty());
    }

    for (int i : input)
    {
        ASSERT_FALSE(pool.empty());
        heap.insert(i, pool);
    }
    ASSERT_TRUE(pool.empty());
}

TEST(pairing_push_heap, sort_more)
{
    std::mt19937_64 randomness;
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> input;
    input.reserve(10000);
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));
    PairingHeapMorePushWork<int, 4>::MemoryPool pool;
    PairingHeapMorePushWork<int, 4> heap;
    for (int i : input)
        heap.insert(i, pool);
    std::vector<int> sorted;
    sorted.reserve(input.size());
    ASSERT_TRUE(pool.empty());
    while (!heap.empty())
    {
        sorted.push_back(heap.min());
        heap.delete_min(pool);
    }
    ASSERT_EQ(input.size(), sorted.size());
    ASSERT_TRUE(std::is_sorted(sorted.begin(), sorted.end()));
    ASSERT_TRUE(std::is_permutation(input.begin(), input.end(), sorted.begin(), sorted.end()));
    heap.clear(pool);
    ASSERT_TRUE(heap.empty());

    for (int i : input)
    {
        ASSERT_FALSE(pool.empty());
        heap.insert(i, pool);
    }
    ASSERT_TRUE(pool.empty());
}

TEST(pairing_push_heap, sort_more_loop)
{
    std::mt19937_64 randomness;
    std::uniform_int_distribution<int> distribution(0, 100);
    std::vector<int> input;
    input.reserve(10000);
    PairingHeapMorePushWork<int, 4> heap;
    PairingHeapMorePushWork<int, 4>::MemoryPool pool;
    for (int j = 0; j < 10; ++j)
    {
        input.clear();
        while (input.size() != input.capacity())
            input.push_back(distribution(randomness));
        for (int i : input)
            heap.insert(i, pool);
        std::vector<int> sorted;
        sorted.reserve(input.size());
        ASSERT_TRUE(pool.empty());
        while (!heap.empty())
        {
            sorted.push_back(heap.min());
            heap.delete_min(pool);
        }
        ASSERT_EQ(input.size(), sorted.size());
        ASSERT_TRUE(std::is_sorted(sorted.begin(), sorted.end()));
        ASSERT_TRUE(std::is_permutation(input.begin(), input.end(), sorted.begin(), sorted.end()));
        ASSERT_TRUE(heap.empty());
    }

    for (int i : input)
    {
        ASSERT_FALSE(pool.empty());
        heap.insert(i, pool);
    }
    ASSERT_TRUE(pool.empty());
}

TEST(heap_pair_heap_sort, simple)
{
    std::vector<int> to_sort = { 4, 3, 2, 1 };
    heap_pair_heap_sort(to_sort.begin(), to_sort.end());
    ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
}

TEST(heap_pair_heap_sort, DISABLED_increasing_length)
{
    std::mt19937_64 randomness(5);
    std::uniform_int_distribution<int> distribution(0, 10000);
    std::vector<int> to_sort;
    to_sort.reserve(10000);
    while (to_sort.size() != to_sort.capacity())
    {
        std::shuffle(to_sort.begin(), to_sort.end(), randomness);
        to_sort.push_back(distribution(randomness));
        heap_pair_heap_sort(to_sort.begin(), to_sort.end());
        if (!std::is_sorted(to_sort.begin(), to_sort.end()))
        {
            std::cout << "Not sorted: " << to_sort.size() << std::endl;
        }
        ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
    }
    ASSERT_TRUE(false);
}
TEST(heap_heap_sort, simple)
{
    std::vector<int> to_sort = { 4, 3, 2, 1 };
    heap_heap_sort(to_sort.begin(), to_sort.end());
    ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
}
TEST(heap_heap_sort, five)
{
    std::vector<int> to_sort = { 4, 1, 3, 2, 5 };
    heap_heap_sort(to_sort.begin(), to_sort.end());
    ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
}
TEST(heap_heap_sort, eight)
{
    std::vector<int> to_sort = { 3, 4, 2, 5, 1, 7, 6, 8 };
    heap_heap_sort(to_sort.begin(), to_sort.end());
    ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
}
TEST(heap_heap_sort, nine)
{
    std::vector<int> to_sort = { 6, 5, 7, 8, 9, 4, 1, 2, 3 };
    heap_heap_sort(to_sort.begin(), to_sort.end());
    ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
}

TEST(heap_heap_sort, thirteen)
{
    std::vector<int> to_sort = { 1, 11, 10, 5, 4, 6, 7, 3, 12, 2, 9, 8, 13 };
    heap_heap_sort(to_sort.begin(), to_sort.end());
    ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
}
TEST(heap_heap_sort, thirty_two)
{
    std::vector<int> to_sort = { 21, 29, 27, 31, 1, 5, 14, 22,
                                 16, 6, 12, 28, 32, 15, 17, 7,
                                 24, 9, 13, 20, 26, 18, 23, 30,
                                 4, 19, 3, 2, 25, 11, 10, 8 };
    heap_heap_sort(to_sort.begin(), to_sort.end());
    ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
}
TEST(heap_heap_sort, thirty_three)
{
    std::vector<int> to_sort = { 32, 1, 10, 16, 24, 6,
                                 26, 15, 29, 12, 27, 25,
                                 21, 5, 4, 22, 13, 19,
                                 17, 33, 18, 11, 20, 8,
                                 23, 14, 30, 31, 28, 7,
                                 3, 9, 2 };
    heap_heap_sort(to_sort.begin(), to_sort.end());
    ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
}

template<typename It, typename Compare>
void heap_sort(It begin, It end, Compare && compare)
{
    if (end - begin < 2)
        return;
    make_dary_heap<2>(begin, end, compare);
    auto last_pop = begin + 1;
    for (;;)
    {
        pop_binary_heap_unrolled(begin, end, compare);
//        pop_dary_heap<2>(begin, end, compare);
        --end;
        if (end == last_pop)
            break;
    }
}
template<typename It>
void heap_sort(It begin, It end)
{
    heap_sort(begin, end, std::less<>{});
}


TEST(heap_heap_sort, DISABLED_increasing_length)
{
    std::mt19937_64 randomness(4);
    std::uniform_int_distribution<int> distribution(0, 10000);
    std::vector<int> to_sort;
    std::vector<int> before_sorting;
    to_sort.reserve(10000);
    while (to_sort.size() != to_sort.capacity())
    {
        std::shuffle(to_sort.begin(), to_sort.end(), randomness);
        to_sort.push_back(distribution(randomness));
        before_sorting = to_sort;
        heap_heap_sort(to_sort.begin(), to_sort.end());
        if (!std::is_sorted(to_sort.begin(), to_sort.end()))
        {
            std::cout << "Not sorted: " << to_sort.size() << ", before:";
            for (int i : before_sorting)
            {
                std::cout << '\n' << i;
            }
            std::cout << "\nAfter:";
            for (int i : to_sort)
            {
                std::cout << '\n' << i;
            }
            std::cout << std::endl;
        }
        ASSERT_TRUE(std::is_sorted(to_sort.begin(), to_sort.end()));
    }
    ASSERT_TRUE(false);
}

#endif

