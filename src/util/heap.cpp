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
    int num = 10000;
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
    int num = 10000;
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
    int num = 10000;
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
    int num = 10000;
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
    int num = 10000;
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
    int num = 10000;
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
    int num = 10000;
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
    int num = 10000;
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

#include "custom_benchmark/custom_benchmark.h"
#include "hashtable_benchmarks/benchmark_shared.hpp"
#include <algorithm>
#include <set>

void benchmark_heap_push(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            std::push_heap(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_std_multiset_push(skb::State & state)
{
    std::multiset<int> heap;
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness));
        skb::DoNotOptimize(*heap.begin());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_interval_heap_push(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            interval_heap_push(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_minmax_heap_push(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_minmax_heap(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<int D>
void benchmark_push_dary_heap(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_dary_heap<D>(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    //CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<D>(heap.begin(), heap.end()));
}

template<int D>
void benchmark_push_dary_heap_grandparent(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_dary_heap_grandparent<D>(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    //CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<D>(heap.begin(), heap.end()));
}
template<int D>
void benchmark_push_dary_heap_great_grandparent(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_dary_heap_great_grandparent<D>(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<D>(heap.begin(), heap.end()));
}
template<int D>
void benchmark_push_dary_heap_great_great_grandparent(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_dary_heap_great_great_grandparent<D>(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<D>(heap.begin(), heap.end()));
}

void benchmark_push_binary_heap_binary_search(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_binary_heap_binary_search(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<2>(heap.begin(), heap.end()));
}

void benchmark_push_binary_heap_binary_search_plus_one(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_binary_heap_binary_search_plus_one(heap.begin(), heap.end());
        }
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<2>(heap.begin(), heap.end()));
}

void benchmark_pairing_heap_push(skb::State & state)
{
    PairingHeap<int>::MemoryPool pool;
    PairingHeap<int> heap;
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
        {
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        }
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pairing_pair_heap_push(skb::State & state)
{
    PairingHeapPair<int>::MemoryPool pool;
    PairingHeapPair<int> heap;
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
        {
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        }
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<size_t MergeInterval>
void benchmark_pairing_push_heap_push(skb::State & state)
{
    typename PairingHeapMorePushWork<int, MergeInterval>::MemoryPool pool;
    PairingHeapMorePushWork<int, MergeInterval> heap;
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
        {
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        }
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_make_heap(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        std::make_heap(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_make_interval_heap(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        interval_heap_make(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_make_minmax_heap(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_minmax_heap(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<int D>
void benchmark_make_dary_heap(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(is_dary_heap<D>(heap.begin(), heap.end()));
}

void benchmark_pop_minmax_heap_min(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_minmax_heap(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_minmax_heap_min(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_minmax_heap_max(skb::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_minmax_heap(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_minmax_heap_max(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        std::make_heap(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            std::pop_heap(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_std_multiset(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<std::multiset<int>> heaps;
    static constexpr size_t max_memory_usage = 2 * 1024 * 1024 * size_t(1024);
    static constexpr size_t memory_per_item = 4 * 8;
    static constexpr size_t max_cached_items = max_memory_usage / memory_per_item;
    int num_heaps = std::max(2, std::min(128, static_cast<int>(max_cached_items / num_items)));
    heaps.reserve(num_heaps);
    while (heaps.size() != heaps.capacity())
    {
        std::multiset<int> heap;
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness));
        heaps.push_back(std::move(heap));
    }
    std::uniform_int_distribution<int> random_heap(0, num_heaps - 1);
    while (state.KeepRunning())
    {
        std::multiset<int> heap = heaps[no_inline_random_number(random_heap, randomness)];
        skb::DoNotOptimize(*heap.begin());
        for (int i = 0; i < num_items; ++i)
            heap.erase(heap.begin());
        skb::DoNotOptimize(heap.begin() == heap.end());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_interval_heap_min(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        interval_heap_make(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            interval_heap_pop_min(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_interval_heap_max(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        interval_heap_make(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            interval_heap_pop_max(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<int D>
void benchmark_pop_dary_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_dary_heap<D>(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

template<int D>
void benchmark_pop_dary_heap_linear(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_dary_heap_linear<D>(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

void benchmark_pop_binary_heap_grandparent(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_grandparent(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

void benchmark_pop_quaternary_heap_grandparent(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<4>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_quaternary_heap_grandparent(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

void benchmark_pop_binary_heap_unrolled(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_unrolled(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}
void benchmark_pop_binary_heap_unrolled_grandparent(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_unrolled_grandparent(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}
void benchmark_pop_binary_heap_unrolled_8(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_unrolled_8(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}
void benchmark_pop_binary_heap_unrolled_2(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_unrolled_2(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

void benchmark_pop_binary_heap_unrolled_no_early_out(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<2>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_binary_heap_unrolled_no_early_out(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}
void benchmark_pop_quaternary_heap_unrolled(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<4>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_quaternary_heap_unrolled(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}
void benchmark_pop_quaternary_heap_unrolled_2(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<4>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_quaternary_heap_unrolled_2(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

template<int D>
void benchmark_pop_dary_heap_half(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        for (int i = 0; i < (num_items / 2); ++i)
            pop_dary_heap<D>(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_dary_heap_make_std_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        std::make_heap(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_dary_heap<2>(heap.begin(), heap.end() - i);
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_sorted(heap.begin(), heap.end()));
}

void benchmark_pop_pairing_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeap<int> heap;
    PairingHeap<int>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        for (int i = 0; i < num_items; ++i)
            heap.delete_min(pool);
        skb::DoNotOptimize(heap.empty());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_pairing_pair_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeapPair<int> heap;
    PairingHeapPair<int>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        for (int i = 0; i < num_items; ++i)
            heap.delete_min(pool);
        skb::DoNotOptimize(heap.empty());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<size_t MergeInterval>
void benchmark_pop_pairing_push_heap(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeapMorePushWork<int, MergeInterval> heap;
    typename PairingHeapMorePushWork<int, MergeInterval>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        for (int i = 0; i < num_items; ++i)
            heap.delete_min(pool);
        skb::DoNotOptimize(heap.empty());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_pop_pairing_heap_half(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeap<int> heap;
    PairingHeap<int>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        for (int i = 0; i < (num_items / 2); ++i)
            heap.delete_min(pool);
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

void benchmark_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        for (int i = 0; i < num_items; ++i)
            skb::DoNotOptimize(no_inline_random_number(distribution, randomness));
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_heap_baseline);

void benchmark_make_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        std::make_heap(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_make_heap_baseline);

void benchmark_copy_std_multiset_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<std::multiset<int>> heaps;
    static constexpr size_t max_memory_usage = 2 * 1024 * 1024 * size_t(1024);
    static constexpr size_t memory_per_item = 4 * 8;
    static constexpr size_t max_cached_items = max_memory_usage / memory_per_item;
    int num_heaps = std::max(2, std::min(128, static_cast<int>(max_cached_items / num_items)));
    heaps.reserve(num_heaps);
    while (heaps.size() != heaps.capacity())
    {
        std::multiset<int> heap;
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness));
        heaps.push_back(std::move(heap));
    }
    std::uniform_int_distribution<int> random_heap(0, num_heaps - 1);
    while (state.KeepRunning())
    {
        std::multiset<int> heap = heaps[no_inline_random_number(random_heap, randomness)];
        skb::DoNotOptimize(*heap.begin());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_copy_std_multiset_baseline);

void benchmark_make_minmax_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_minmax_heap(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_make_minmax_heap_baseline);

void benchmark_make_interval_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        interval_heap_make(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_make_interval_heap_baseline);

void benchmark_make_pairing_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeap<int> heap;
    PairingHeap<int>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_make_pairing_heap_baseline);

void benchmark_make_pairing_pair_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeapPair<int> heap;
    PairingHeapPair<int>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
SKA_BENCHMARK("baseline", benchmark_make_pairing_pair_heap_baseline);

template<size_t MergeInterval>
void benchmark_make_pairing_push_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    PairingHeapMorePushWork<int, MergeInterval> heap;
    typename PairingHeapMorePushWork<int, MergeInterval>::MemoryPool pool;
    while (state.KeepRunning())
    {
        heap.clear(pool);
        for (int i = 0; i < num_items; ++i)
            heap.insert(no_inline_random_number(distribution, randomness), pool);
        skb::DoNotOptimize(heap.min());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<int D>
void benchmark_make_dary_heap_baseline(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}

template<typename It, typename Compare>
It min_element_four(It begin, It end, Compare && compare)
{
    size_t size = end - begin;
    if (!size)
        return end;
    size_t num_loops = (size - 1) / 4;
    It smallest = begin;
    ++begin;
    for (; num_loops > 0; --num_loops)
    {
        It min_first_half = begin + !!compare(begin[1], begin[0]);
        It min_second_half = begin + 2 + !!compare(begin[3], begin[2]);
        It min_four = compare(*min_second_half, *min_first_half) ? min_second_half : min_first_half;
        if (compare(*min_four, *smallest))
            smallest = min_four;
        begin += 4;
    }
    for (; begin != end; ++begin)
    {
        if (compare(*begin, *smallest))
            smallest = begin;
    }
    return smallest;
}

template<typename It>
It min_element_four(It begin, It end)
{
    return min_element_four(begin, end, std::less<>{});
}
template<typename It, typename Compare>
It min_element_four_branchy(It begin, It end, Compare && compare)
{
    size_t size = end - begin;
    if (!size)
        return end;
    size_t num_loops = (size - 1) / 4;
    It smallest = begin;
    ++begin;
    for (; num_loops > 0; --num_loops)
    {
        It min_first_half = compare(begin[1], begin[0]) ? begin + 1 : begin;
        It min_second_half = compare(begin[3], begin[2]) ? begin + 3 : begin + 2;
        It min_four = compare(*min_second_half, *min_first_half) ? min_second_half : min_first_half;
        if (compare(*min_four, *smallest))
            smallest = min_four;
        begin += 4;
    }
    for (; begin != end; ++begin)
    {
        if (compare(*begin, *smallest))
            smallest = begin;
    }
    return smallest;
}

template<typename It>
It min_element_four_branchy(It begin, It end)
{
    return min_element_four_branchy(begin, end, std::less<>{});
}
template<typename It, typename Compare>
It min_element_four_parallel(It begin, It end, Compare && compare)
{
    size_t size = end - begin;
    if (!size)
        return end;
    size_t num_loops = size / 4;
    It smallest0 = begin;
    ++begin;
    if (num_loops)
    {
        It smallest1 = begin;
        It smallest2 = begin + 1;
        It smallest3 = begin + 2;
        begin += 3;
        for (--num_loops; num_loops > 0; --num_loops)
        {
            if (compare(begin[0], *smallest0))
                smallest0 = begin;
            if (compare(begin[1], *smallest1))
                smallest1 = begin + 1;
            if (compare(begin[2], *smallest2))
                smallest2 = begin + 2;
            if (compare(begin[3], *smallest3))
                smallest3 = begin + 3;
            begin += 4;
        }
        if (compare(*smallest3, *smallest2))
            smallest2 = smallest3;
        if (compare(*smallest1, *smallest0))
            smallest0 = smallest1;
        if (compare(*smallest2, *smallest0))
            smallest0 = smallest2;
    }
    for (; begin != end; ++begin)
    {
        if (compare(*begin, *smallest0))
            smallest0 = begin;
    }
    return smallest0;
}

template<typename It>
It min_element_four_parallel(It begin, It end)
{
    return min_element_four_parallel(begin, end, std::less<>{});
}

TEST(min_element_four_heap, cases)
{
    std::vector<int> input = { 1, 2, 3, 4, 5, 6 };
    ASSERT_EQ(1, *min_element_four(input.begin(), input.end()));
    input = { 1, 2, 3, 4, 5, 6, 0 };
    ASSERT_EQ(0, *min_element_four(input.begin(), input.end()));
    input = { 1, 2, 3, 4, -1, 5, 6, 0 };
    ASSERT_EQ(-1, *min_element_four(input.begin(), input.end()));
    input = { 1, 2, 3, 4, -1, 5, 6, -2, 0 };
    ASSERT_EQ(-2, *min_element_four(input.begin(), input.end()));
}
TEST(min_element_four_heap, parallel_cases)
{
    std::vector<int> input = { 1, 2, 3 };
    ASSERT_EQ(1, *min_element_four_parallel(input.begin(), input.end()));
    input = { 1, 2, 3, 0 };
    ASSERT_EQ(0, *min_element_four_parallel(input.begin(), input.end()));
    input = { 1, 2, 3, 4, -1, 5, 6, 0 };
    ASSERT_EQ(-1, *min_element_four_parallel(input.begin(), input.end()));
    input = { 1, 2, 3, 4, -1, 5, 6, -2, 0 };
    ASSERT_EQ(-2, *min_element_four_parallel(input.begin(), input.end()));
}

void benchmark_min_element(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> data;
    data.reserve(num_items);
    while (state.KeepRunning())
    {
        data.clear();
        for (int i = 0; i < num_items; ++i)
            data.push_back(no_inline_random_number(distribution, randomness));
        skb::DoNotOptimize(*std::min_element(data.begin(), data.end()));
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}

void benchmark_min_element_four(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> data;
    data.reserve(num_items);
    while (state.KeepRunning())
    {
        data.clear();
        for (int i = 0; i < num_items; ++i)
            data.push_back(no_inline_random_number(distribution, randomness));
        skb::DoNotOptimize(*min_element_four(data.begin(), data.end()));
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}

void benchmark_min_element_branchy(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> data;
    data.reserve(num_items);
    while (state.KeepRunning())
    {
        data.clear();
        for (int i = 0; i < num_items; ++i)
            data.push_back(no_inline_random_number(distribution, randomness));
        skb::DoNotOptimize(*min_element_four_branchy(data.begin(), data.end()));
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}
void benchmark_min_element_parallel(skb::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> data;
    data.reserve(num_items);
    while (state.KeepRunning())
    {
        data.clear();
        for (int i = 0; i < num_items; ++i)
            data.push_back(no_inline_random_number(distribution, randomness));
        skb::DoNotOptimize(*min_element_four_parallel(data.begin(), data.end()));
    }
    state.SetItemsProcessed(state.iterations() * num_items);
}

skb::Benchmark * SetHeapRange(skb::Benchmark * bm)
{
    return bm->SetRange(4, 1024*1024*1024)->SetRangeMultiplier(std::pow(2.0, 0.25));
}

void RegisterHeapBenchmarks()
{
    void register_ocaml_multiply();
    register_ocaml_multiply();
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<2>, "baseline", "benchmark_make_dary_heap_baseline_2");
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<3>, "baseline", "benchmark_make_dary_heap_baseline_3");
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<4>, "baseline", "benchmark_make_dary_heap_baseline_4");
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<5>, "baseline", "benchmark_make_dary_heap_baseline_5");
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<6>, "baseline", "benchmark_make_dary_heap_baseline_6");
    SKA_BENCHMARK_NAME(&benchmark_make_dary_heap_baseline<8>, "baseline", "benchmark_make_dary_heap_baseline_8");
    skb::CategoryBuilder builder;
    {
        skb::CategoryBuilder push = builder.AddCategory("operation", "push");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_heap_push, push.BuildCategories("heap", "std::heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_std_multiset_push, push.BuildCategories("heap", "std::multiset"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_minmax_heap_push, push.BuildCategories("heap", "minmax_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_interval_heap_push, push.BuildCategories("heap", "interval_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<2>, push.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<3>, push.AddCategory("dary_heap d", "3").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<4>, push.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<5>, push.AddCategory("dary_heap d", "5").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<6>, push.AddCategory("dary_heap d", "6").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<8>, push.AddCategory("dary_heap d", "8").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<2>, push.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<3>, push.AddCategory("dary_heap d", "3").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<4>, push.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<5>, push.AddCategory("dary_heap d", "5").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<6>, push.AddCategory("dary_heap d", "6").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_grandparent<8>, push.AddCategory("dary_heap d", "8").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_great_grandparent<2>, push.AddCategory("dary_heap d", "2").AddCategory("grandparent", "great").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_great_grandparent<4>, push.AddCategory("dary_heap d", "4").AddCategory("grandparent", "great").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_great_great_grandparent<2>, push.AddCategory("dary_heap d", "2").AddCategory("grandparent", "great_great").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap_great_great_grandparent<4>, push.AddCategory("dary_heap d", "4").AddCategory("grandparent", "great_great").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_binary_heap_binary_search, push.AddCategory("dary_heap d", "2").AddCategory("grandparent", "binary").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_binary_heap_binary_search_plus_one, push.AddCategory("dary_heap d", "2").AddCategory("grandparent", "binary+1").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pairing_heap_push, push.BuildCategories("heap", "pairing_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pairing_pair_heap_push, push.BuildCategories("heap", "pairing_heap_pair"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pairing_push_heap_push<4>, push.AddCategory("merge interval", "4").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pairing_push_heap_push<8>, push.AddCategory("merge interval", "8").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pairing_push_heap_push<16>, push.AddCategory("merge interval", "16").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_heap_baseline"));
    }
    {
        skb::CategoryBuilder make = builder.AddCategory("operation", "make");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_heap, make.BuildCategories("heap", "std::heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_interval_heap, make.BuildCategories("heap", "interval_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_minmax_heap, make.BuildCategories("heap", "minmax_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<2>, make.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<3>, make.AddCategory("dary_heap d", "3").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<4>, make.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<5>, make.AddCategory("dary_heap d", "5").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<6>, make.AddCategory("dary_heap d", "6").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_dary_heap<8>, make.AddCategory("dary_heap d", "8").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
    }
    {
        skb::CategoryBuilder pop = builder.AddCategory("operation", "pop");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_minmax_heap_min, pop.AddCategory("minmax pop", "min").BuildCategories("heap", "minmax_heap"))->SetBaseline("benchmark_make_minmax_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_minmax_heap_max, pop.AddCategory("minmax pop", "max").BuildCategories("heap", "minmax_heap"))->SetBaseline("benchmark_make_minmax_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_interval_heap_min, pop.AddCategory("minmax pop", "min").BuildCategories("heap", "interval_heap"))->SetBaseline("benchmark_make_interval_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_interval_heap_max, pop.AddCategory("minmax pop", "max").BuildCategories("heap", "interval_heap"))->SetBaseline("benchmark_make_interval_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_heap, pop.BuildCategories("heap", "std::heap"))->SetBaseline("benchmark_make_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_std_multiset, pop.BuildCategories("heap", "std::multiset"))->SetBaseline("benchmark_copy_std_multiset_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<2>, pop.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<3>, pop.AddCategory("dary_heap d", "3").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_3"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<4>, pop.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_4"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap_linear<4>, pop.AddCategory("dary_heap d", "4").AddCategory("compare", "linear").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_4"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<5>, pop.AddCategory("dary_heap d", "5").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_5"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<6>, pop.AddCategory("dary_heap d", "6").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_6"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<8>, pop.AddCategory("dary_heap d", "8").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_8"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap_linear<8>, pop.AddCategory("dary_heap d", "8").AddCategory("compare", "linear").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_8"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_grandparent, pop.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_quaternary_heap_grandparent, pop.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_make_dary_heap_baseline_4"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_unrolled, pop.AddCategory("dary_heap d", "2").AddCategory("unrolled", "with_early_out").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_unrolled_grandparent, pop.AddCategory("dary_heap d", "2").AddCategory("unrolled", "with_early_out").BuildCategories("heap", "dary_heap_grandparent"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_unrolled_8, pop.AddCategory("dary_heap d", "2").AddCategory("unrolled", "with_early_out_8").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_unrolled_2, pop.AddCategory("dary_heap d", "2").AddCategory("unrolled", "with_early_out_2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_binary_heap_unrolled_no_early_out, pop.AddCategory("dary_heap d", "2").AddCategory("unrolled", "no_early_out").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_quaternary_heap_unrolled, pop.AddCategory("dary_heap d", "4").AddCategory("unrolled", "with_early_out").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_4"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_quaternary_heap_unrolled_2, pop.AddCategory("dary_heap d", "4").AddCategory("unrolled", "with_early_out_2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_4"));
        //SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap_make_std_heap, pop.AddCategory("dary_heap d", "2").BuildCategories("heap", "pop_dary_heap_make_std_heap"))->SetBaseline("benchmark_make_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_heap, pop.BuildCategories("heap", "pairing_heap"))->SetBaseline("benchmark_make_pairing_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_pair_heap, pop.BuildCategories("heap", "pairing_heap_pair"))->SetBaseline("benchmark_make_pairing_pair_heap_baseline"));
        SKA_BENCHMARK_NAME(&benchmark_make_pairing_push_heap_baseline<4>, "baseline", "benchmark_make_pairing_push_heap_baseline_4");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_push_heap<4>, pop.AddCategory("merge interval", "4").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_make_pairing_push_heap_baseline_4"));
        SKA_BENCHMARK_NAME(&benchmark_make_pairing_push_heap_baseline<8>, "baseline", "benchmark_make_pairing_push_heap_baseline_8");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_push_heap<8>, pop.AddCategory("merge interval", "8").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_make_pairing_push_heap_baseline_8"));
        SKA_BENCHMARK_NAME(&benchmark_make_pairing_push_heap_baseline<16>, "baseline", "benchmark_make_pairing_push_heap_baseline_16");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_push_heap<16>, pop.AddCategory("merge interval", "16").BuildCategories("heap", "pairing_heap_push"))->SetBaseline("benchmark_make_pairing_push_heap_baseline_16"));
        skb::CategoryBuilder pop_half = builder.AddCategory("operation", "pop_half");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap_half<2>, pop_half.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_pairing_heap_half, pop_half.BuildCategories("heap", "pairing_heap"))->SetBaseline("benchmark_heap_baseline"));
    }
    {
        skb::CategoryBuilder min_element = builder.AddCategory("instruction", "min_element");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_min_element, min_element.BuildCategories("instructions", "min_element"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_min_element_four, min_element.BuildCategories("instructions", "min_element_four"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_min_element_branchy, min_element.BuildCategories("instructions", "min_element_branchy"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_min_element_parallel, min_element.BuildCategories("instructions", "min_element_parallel"))->SetBaseline("benchmark_heap_baseline"));
    }
}

inline int64_t ocaml_multiply(int64_t a, int64_t b)
{
    --b;
    int64_t result = (a >> 1) * b;
    ++result;
    return result;
}

inline int64_t my_ocaml_multiply(int64_t a, int64_t b)
{
    return ((a >> 1) * (b - 1)) | 1;
}

inline int64_t my_ocaml_multiply_xor(int64_t a, int64_t b)
{
    return ((a >> 1) * (b ^ 1)) | 1;
}

inline int64_t ocaml_subtract(int64_t a, int64_t b)
{
    return (a - b) + 1;
}
inline int64_t my_ocaml_subtract(int64_t a, int64_t b)
{
    return (a - b) | 1;
}
inline int64_t xor_baseline(int64_t a, int64_t b)
{
    return a ^ b;
}

SKA_NOINLINE(std::vector<int64_t>) to_multiply(int num_items)
{
    std::vector<int64_t> result;
    result.reserve(num_items);
    for (int i = 0; i < num_items; ++i)
        result.push_back(((i + 1) << 1) | 1);
    return result;
}

struct int_in_range
{
    uint64_t limit;
    // from https://lemire.me/blog/2019/06/06/nearly-divisionless-random-integer-generation-on-various-systems/
    template<typename Randomness>
    size_t operator()(Randomness & random) const
    {
          uint64_t x = random() ;
          __uint128_t m = ( __uint128_t ) x * ( __uint128_t ) limit;
          uint64_t l = ( uint64_t ) m;
          if (l < limit) {
            uint64_t t = -limit % limit;
            while (l < t) {
              x = random() ;
              m = ( __uint128_t ) x * ( __uint128_t ) limit;
              l = ( uint64_t ) m;
            }
          }
          return m >> 64;
    }
};

template<int64_t (*func)(int64_t, int64_t)>
void benchmark_ocaml_multiply(benchmark::State & state)
{
    std::vector<int64_t> data = to_multiply(1000000);
    std::mt19937_64 & randomness = global_randomness;
    //std::uniform_int_distribution<size_t> random_index(0, data.size() - 1);
    int_in_range random_index{data.size()};
    while (state.KeepRunning())
    {
        for (size_t i = 0, end = data.size() - 1; i < end; ++i)
        {
            size_t index_a = no_inline_random_number(random_index, randomness);
            size_t index_b = no_inline_random_number(random_index, randomness);
            data[index_a] = func(data[index_a], data[index_b]);
        }
    }
    for (int64_t & i : data)
        ::benchmark::DoNotOptimize(i);
}
BENCHMARK_TEMPLATE(benchmark_ocaml_multiply, &ocaml_multiply);
BENCHMARK_TEMPLATE(benchmark_ocaml_multiply, &my_ocaml_multiply);
BENCHMARK_TEMPLATE(benchmark_ocaml_multiply, &my_ocaml_multiply_xor);
BENCHMARK_TEMPLATE(benchmark_ocaml_multiply, &ocaml_subtract);
BENCHMARK_TEMPLATE(benchmark_ocaml_multiply, &my_ocaml_subtract);
BENCHMARK_TEMPLATE(benchmark_ocaml_multiply, &xor_baseline);

struct random_range_mod_two
{
    size_t size_minus_one;
    template<typename Randomness>
    size_t operator()(Randomness & random) const
    {
        return random() & size_minus_one;
    }
};

static constexpr size_t num_multiplies = 100000;

template<int64_t (*func)(int64_t, int64_t)>
void skbenchmark_ocaml_multiply(skb::State & state)
{
    std::mt19937_64 & randomness = global_randomness;
    //std::uniform_int_distribution<size_t> random_index(0, data.size() - 1);
    //int_in_range random_index{data.size()};
    int64_t x = state.range(0);
    random_range_mod_two random_index{static_cast<size_t>(x) - 1};
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_multiplies; ++i)
        {
            int64_t random = no_inline_random_number(random_index, randomness);
            int64_t random2 = no_inline_random_number(random_index, randomness);
#if 0
            x = func(random, x);
            skb::DoNotOptimize(x);
            skb::DoNotOptimize(func(random, x));
#else
            skb::DoNotOptimize(func(random, random2));
#endif
        }
    }
    state.SetItemsProcessed(num_multiplies * state.iterations());
}

template<typename It>
size_t find_index_with_end(It begin, It end, int to_find)
{
    size_t num_items = end - begin;
    for (size_t i = 0; i != num_items; ++i)
    {
        if (begin[i] == to_find)
            return i;
    }
    return size_t(-1);
}
template<typename It>
size_t find_index_no_bounds_checking(It begin, It end, int to_find)
{
    if (begin == end)
        return size_t(-1);
    int last = end[-1];
    size_t last_index = (end - begin) - 1;
    if (to_find == last)
        return last_index;
    end[-1] = to_find;
    size_t i = 0;
    for (;; ++i)
    {
        if (begin[i] == to_find)
            break;
    }
    end[-1] = last;
    if (i == last_index)
        return size_t(-1);
    else
        return i;
}
template<typename It>
size_t find_index_no_bounds_checking_unrolled(It begin, It end, int to_find)
{
    if (begin == end)
        return size_t(-1);
    int last = end[-1];
    size_t last_index = (end - begin) - 1;
    if (to_find == last)
        return last_index;
    end[-1] = to_find;
    size_t i = 0;
    for (;; i += 2)
    {
        if (begin[i] == to_find)
            break;
        if (begin[i + 1] == to_find)
        {
            i += 1;
            break;
        }
    }
    end[-1] = last;
    if (i == last_index)
        return size_t(-1);
    else
        return i;
}

template<typename It>
int find_index(It begin, int to_find)
{
    for (int i = 0;; ++i, ++begin)
    {
        if (*begin == to_find)
            return i;
    }
}
template<typename It>
int find_index_random_access(It begin, int to_find)
{
    for (int i = 0;; ++i)
    {
        if (begin[i] == to_find)
            return i;
    }
}
template<typename It>
int find_index_knuth(It begin, int to_find)
{
    int i = 0;
    for (;; i += 2)
    {
        if (begin[i] == to_find)
            break;
        if (begin[i + 1] == to_find)
        {
            ++i;
            break;
        }
    }
    return i;
}
template<typename It>
int find_index_knuth_3(It begin, int to_find)
{
    int i = 0;
    for (;; i += 3)
    {
        if (begin[i] == to_find)
            break;
        if (begin[i + 1] == to_find)
        {
            ++i;
            break;
        }
        if (begin[i + 2] == to_find)
        {
            i += 2;
            break;
        }
    }
    return i;
}
template<typename It>
int find_index_knuth_4(It begin, int to_find)
{
    int i = 0;
    for (;; i += 4)
    {
        if (begin[i] == to_find)
            break;
        if (begin[i + 1] == to_find)
        {
            ++i;
            break;
        }
        if (begin[i + 2] == to_find)
        {
            i += 2;
            break;
        }
        if (begin[i + 3] == to_find)
        {
            i += 3;
            break;
        }
    }
    return i;
}

void benchmark_find_in_vector(skb::State & state)
{
    std::minstd_rand randomness(5);
    std::uniform_int_distribution<int> distribution;
    std::vector<int> input;
    input.reserve(state.range(0));
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));

    std::uniform_int_distribution<size_t> random_index(0, input.size() - 1);
    size_t num_iterations = 1024;
    std::vector<int> to_search = input;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_iterations; ++i)
            skb::DoNotOptimize(find_index(to_search.begin(), input[no_inline_random_number(random_index, randomness)]));
    }
    state.SetItemsProcessed(state.iterations() * input.size() * num_iterations);
}
void benchmark_find_in_vector_random_access(skb::State & state)
{
    std::minstd_rand randomness(5);
    std::uniform_int_distribution<int> distribution;
    std::vector<int> input;
    input.reserve(state.range(0));
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));

    std::uniform_int_distribution<size_t> random_index(0, input.size() - 1);
    size_t num_iterations = 1024;
    std::vector<int> to_search = input;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_iterations; ++i)
            skb::DoNotOptimize(find_index_random_access(to_search.begin(), input[no_inline_random_number(random_index, randomness)]));
    }
    state.SetItemsProcessed(state.iterations() * input.size() * num_iterations);
}
void benchmark_find_in_vector_knuth(skb::State & state)
{
    std::minstd_rand randomness(5);
    std::uniform_int_distribution<int> distribution;
    std::vector<int> input;
    input.reserve(state.range(0));
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));

    std::uniform_int_distribution<size_t> random_index(0, input.size() - 1);
    size_t num_iterations = 1024;
    std::vector<int> to_search = input;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_iterations; ++i)
            skb::DoNotOptimize(find_index_knuth(to_search.begin(), input[no_inline_random_number(random_index, randomness)]));
    }
    state.SetItemsProcessed(state.iterations() * input.size() * num_iterations);
}
void benchmark_find_in_vector_knuth_3(skb::State & state)
{
    std::minstd_rand randomness(5);
    std::uniform_int_distribution<int> distribution;
    std::vector<int> input;
    input.reserve(state.range(0));
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));

    std::uniform_int_distribution<size_t> random_index(0, input.size() - 1);
    size_t num_iterations = 1024;
    std::vector<int> to_search = input;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_iterations; ++i)
            skb::DoNotOptimize(find_index_knuth_3(to_search.begin(), input[no_inline_random_number(random_index, randomness)]));
    }
    state.SetItemsProcessed(state.iterations() * input.size() * num_iterations);
}
void benchmark_find_in_vector_knuth_4(skb::State & state)
{
    std::minstd_rand randomness(5);
    std::uniform_int_distribution<int> distribution;
    std::vector<int> input;
    input.reserve(state.range(0));
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));

    std::uniform_int_distribution<size_t> random_index(0, input.size() - 1);
    size_t num_iterations = 1024;
    std::vector<int> to_search = input;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_iterations; ++i)
            skb::DoNotOptimize(find_index_knuth_4(to_search.begin(), input[no_inline_random_number(random_index, randomness)]));
    }
    state.SetItemsProcessed(state.iterations() * input.size() * num_iterations);
}
void benchmark_find_in_vector_bounds_checked(skb::State & state)
{
    std::minstd_rand randomness(5);
    std::uniform_int_distribution<int> distribution;
    std::vector<int> input;
    input.reserve(state.range(0));
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));

    std::uniform_int_distribution<size_t> random_index(0, input.size() - 1);
    size_t num_iterations = 1024;
    std::vector<int> to_search = input;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_iterations; ++i)
            skb::DoNotOptimize(find_index_with_end(to_search.begin(), to_search.end(), input[no_inline_random_number(random_index, randomness)]));
    }
    state.SetItemsProcessed(state.iterations() * input.size() * num_iterations);
}
void benchmark_find_in_vector_no_bounds_checking(skb::State & state)
{
    std::minstd_rand randomness(5);
    std::uniform_int_distribution<int> distribution;
    std::vector<int> input;
    input.reserve(state.range(0));
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));

    std::uniform_int_distribution<size_t> random_index(0, input.size() - 1);
    size_t num_iterations = 1024;
    std::vector<int> to_search = input;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_iterations; ++i)
            skb::DoNotOptimize(find_index_no_bounds_checking(to_search.begin(), to_search.end(), input[no_inline_random_number(random_index, randomness)]));
    }
    state.SetItemsProcessed(state.iterations() * input.size() * num_iterations);
}
void benchmark_find_in_vector_no_bounds_checking_unrolled(skb::State & state)
{
    std::minstd_rand randomness(5);
    std::uniform_int_distribution<int> distribution;
    std::vector<int> input;
    input.reserve(state.range(0));
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));

    std::uniform_int_distribution<size_t> random_index(0, input.size() - 1);
    size_t num_iterations = 1024;
    std::vector<int> to_search = input;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_iterations; ++i)
            skb::DoNotOptimize(find_index_no_bounds_checking_unrolled(to_search.begin(), to_search.end(), input[no_inline_random_number(random_index, randomness)]));
    }
    state.SetItemsProcessed(state.iterations() * input.size() * num_iterations);
}
void benchmark_find_in_forward_list(skb::State & state)
{
    std::minstd_rand randomness(5);
    std::uniform_int_distribution<int> distribution;
    std::vector<int> input;
    input.reserve(state.range(0));
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));

    std::uniform_int_distribution<size_t> random_index(0, input.size() - 1);
    size_t num_iterations = 1024;
    std::forward_list<int> to_search(input.begin(), input.end());
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_iterations; ++i)
            skb::DoNotOptimize(find_index(to_search.begin(), input[no_inline_random_number(random_index, randomness)]));
    }
    state.SetItemsProcessed(state.iterations() * input.size() * num_iterations);
}
void benchmark_find_index_baseline(skb::State & state)
{
    std::minstd_rand randomness(5);
    std::uniform_int_distribution<int> distribution;
    std::vector<int> input;
    input.reserve(state.range(0));
    while (input.size() != input.capacity())
        input.push_back(distribution(randomness));

    std::uniform_int_distribution<size_t> random_index(0, input.size() - 1);
    size_t num_iterations = 1024;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_iterations; ++i)
            skb::DoNotOptimize(input[no_inline_random_number(random_index, randomness)]);
    }
    state.SetItemsProcessed(state.iterations() * input.size() * num_iterations);
}
SKA_BENCHMARK("baseline", benchmark_find_index_baseline);

void register_ocaml_multiply()
{
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&xor_baseline>, "baseline", "ocaml_multiply_baseline");
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&ocaml_multiply>, "ocaml", "multiply")->SetBaseline("ocaml_multiply_baseline")->SetRange(4, 1024*1024)->SetRangeMultiplier(8);
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&my_ocaml_multiply>, "ocaml", "my_multiply")->SetBaseline("ocaml_multiply_baseline")->SetRange(4, 1024*1024)->SetRangeMultiplier(8);
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&my_ocaml_multiply_xor>, "ocaml", "my_multiply_xor")->SetBaseline("ocaml_multiply_baseline")->SetRange(4, 1024*1024)->SetRangeMultiplier(8);
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&ocaml_subtract>, "ocaml", "subtract")->SetBaseline("ocaml_multiply_baseline")->SetRange(4, 1024*1024)->SetRangeMultiplier(8);
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&my_ocaml_subtract>, "ocaml", "my_subtract")->SetBaseline("ocaml_multiply_baseline")->SetRange(4, 1024*1024)->SetRangeMultiplier(8);
    SKA_BENCHMARK_NAME(&benchmark_find_in_vector, "ocaml", "find_in_vector")->SetBaseline("benchmark_find_index_baseline")->SetRange(4, 1024*1024*128)->SetRangeMultiplier(std::pow(2.0, 0.25));
    SKA_BENCHMARK_NAME(&benchmark_find_in_vector_random_access, "ocaml", "find_in_vector_random_access")->SetBaseline("benchmark_find_index_baseline")->SetRange(4, 1024*1024*128)->SetRangeMultiplier(std::pow(2.0, 0.25));
    SKA_BENCHMARK_NAME(&benchmark_find_in_vector_knuth, "ocaml", "find_in_vector_knuth")->SetBaseline("benchmark_find_index_baseline")->SetRange(4, 1024*1024*128)->SetRangeMultiplier(std::pow(2.0, 0.25));
    SKA_BENCHMARK_NAME(&benchmark_find_in_vector_knuth_3, "ocaml", "find_in_vector_knuth_3")->SetBaseline("benchmark_find_index_baseline")->SetRange(4, 1024*1024*128)->SetRangeMultiplier(std::pow(2.0, 0.25));
    SKA_BENCHMARK_NAME(&benchmark_find_in_vector_knuth_4, "ocaml", "find_in_vector_knuth_4")->SetBaseline("benchmark_find_index_baseline")->SetRange(4, 1024*1024*128)->SetRangeMultiplier(std::pow(2.0, 0.25));
    SKA_BENCHMARK_NAME(&benchmark_find_in_forward_list, "ocaml", "find_in_list")->SetBaseline("benchmark_find_index_baseline")->SetRange(4, 1024*1024*128)->SetRangeMultiplier(std::pow(2.0, 0.25));
    SKA_BENCHMARK_NAME(&benchmark_find_in_vector_bounds_checked, "ocaml", "find_in_vector_bounds_checked")->SetBaseline("benchmark_find_index_baseline")->SetRange(4, 1024*1024*128)->SetRangeMultiplier(std::pow(2.0, 0.25));
    SKA_BENCHMARK_NAME(&benchmark_find_in_vector_no_bounds_checking, "ocaml", "find_in_vector_no_bounds_checking")->SetBaseline("benchmark_find_index_baseline")->SetRange(4, 1024*1024*128)->SetRangeMultiplier(std::pow(2.0, 0.25));
    SKA_BENCHMARK_NAME(&benchmark_find_in_vector_no_bounds_checking_unrolled, "ocaml", "find_in_vector_no_bounds_checking_unrolled")->SetBaseline("benchmark_find_index_baseline")->SetRange(4, 1024*1024*128)->SetRangeMultiplier(std::pow(2.0, 0.25));
}




void benchmark_heap_baseline(benchmark::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        for (int i = 0; i < num_items; ++i)
            benchmark::DoNotOptimize(no_inline_random_number(distribution, randomness));
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
BENCHMARK(benchmark_heap_baseline)->Range(4, 1024*1024*16);

void benchmark_push_minmax_heap(benchmark::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_minmax_heap(heap.begin(), heap.end());
        }
        benchmark::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
BENCHMARK(benchmark_push_minmax_heap)->Range(4, 1024*1024*16);

template<int D>
void benchmark_push_dary_heap(benchmark::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
        {
            heap.push_back(no_inline_random_number(distribution, randomness));
            push_dary_heap<D>(heap.begin(), heap.end());
        }
        benchmark::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
BENCHMARK_TEMPLATE(benchmark_push_dary_heap, 2)->Range(4, 1024*1024*16);
BENCHMARK_TEMPLATE(benchmark_push_dary_heap, 4)->Range(4, 1024*1024*16);

void benchmark_make_minmax_heap(benchmark::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_minmax_heap(heap.begin(), heap.end());
        benchmark::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
BENCHMARK(benchmark_make_minmax_heap)->Range(4, 1024*1024*16);

template<int D>
void benchmark_make_dary_heap(benchmark::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        benchmark::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
BENCHMARK_TEMPLATE(benchmark_make_dary_heap, 2)->Range(4, 1024*1024*16);
BENCHMARK_TEMPLATE(benchmark_make_dary_heap, 4)->Range(4, 1024*1024*16);

void benchmark_pop_minmax_heap_min(benchmark::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_minmax_heap(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_minmax_heap_min(heap.begin(), heap.end() - i);
        benchmark::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
BENCHMARK(benchmark_pop_minmax_heap_min)->Range(4, 1024*1024*16);

void benchmark_pop_minmax_heap_max(benchmark::State & state)
{
    std::vector<int> heap;
    int num_items = state.range(0);
    heap.reserve(num_items);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_minmax_heap(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_minmax_heap_max(heap.begin(), heap.end() - i);
        benchmark::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
BENCHMARK(benchmark_pop_minmax_heap_max)->Range(4, 1024*1024*16);

template<int D>
void benchmark_pop_dary_heap(benchmark::State & state)
{
    int num_items = state.range(0);
    std::uniform_int_distribution<int> distribution;
    std::mt19937_64 & randomness = global_randomness;
    std::vector<int> heap;
    heap.reserve(num_items);
    while (state.KeepRunning())
    {
        heap.clear();
        for (int i = 0; i < num_items; ++i)
            heap.push_back(no_inline_random_number(distribution, randomness));
        make_dary_heap<D>(heap.begin(), heap.end());
        for (int i = 0; i < num_items; ++i)
            pop_dary_heap<D>(heap.begin(), heap.end() - i);
        benchmark::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
}
BENCHMARK_TEMPLATE(benchmark_pop_dary_heap, 2)->Range(4, 1024*1024*16);
BENCHMARK_TEMPLATE(benchmark_pop_dary_heap, 4)->Range(4, 1024*1024*16);

