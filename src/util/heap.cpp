#include "util/heap.hpp"


#ifndef DISABLE_TESTS
#include "test/include_test.hpp"
#include <random>

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

TEST(minmax_heap, custom_make_heap)
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
        custom_make_heap(heap.begin(), heap.end());
        ASSERT_TRUE(std::is_heap(heap.begin(), heap.end()));
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
    std::vector<int> heap[7];
    int num = 100;
    for (int i = 0; i < 7; ++i)
        heap[i].reserve(num);
    for (int i = 0; i < num; ++i)
    {
        for (int j = 0; j < 7; ++j)
            heap[j].push_back(distribution(randomness));

        push_dary_heap<2>(heap[0].begin(), heap[0].end());
        ASSERT_TRUE(is_dary_heap<2>(heap[0].begin(), heap[0].end()));
        ASSERT_TRUE(std::is_heap(heap[0].begin(), heap[0].end()));

        push_dary_heap<3>(heap[1].begin(), heap[1].end());
        ASSERT_TRUE(is_dary_heap<3>(heap[1].begin(), heap[1].end()));

        push_dary_heap<4>(heap[2].begin(), heap[2].end());
        ASSERT_TRUE(is_dary_heap<4>(heap[2].begin(), heap[2].end()));

        push_dary_heap<5>(heap[3].begin(), heap[3].end());
        ASSERT_TRUE(is_dary_heap<5>(heap[3].begin(), heap[3].end()));

        push_dary_heap<6>(heap[4].begin(), heap[4].end());
        ASSERT_TRUE(is_dary_heap<6>(heap[4].begin(), heap[4].end()));

        push_dary_heap<7>(heap[5].begin(), heap[5].end());
        ASSERT_TRUE(is_dary_heap<7>(heap[5].begin(), heap[5].end()));

        push_dary_heap<8>(heap[6].begin(), heap[6].end());
        ASSERT_TRUE(is_dary_heap<8>(heap[6].begin(), heap[6].end()));
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

#endif

#include "custom_benchmark/custom_benchmark.h"
#include "hashtable_benchmarks/benchmark_shared.hpp"
#include <algorithm>

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

void benchmark_custom_make_heap(skb::State & state)
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
        custom_make_heap(heap.begin(), heap.end());
        skb::DoNotOptimize(heap.back());
    }
    state.SetItemsProcessed(num_items * state.iterations());
    CHECK_FOR_PROGRAMMER_ERROR(std::is_heap(heap.begin(), heap.end()));
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

skb::Benchmark * SetHeapRange(skb::Benchmark * bm)
{
    return bm->SetRange(4, 1024*1024*1024)->SetRangeMultiplier(std::sqrt(2.0));
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
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_minmax_heap_push, push.BuildCategories("heap", "minmax_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_interval_heap_push, push.BuildCategories("heap", "interval_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<2>, push.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<3>, push.AddCategory("dary_heap d", "3").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<4>, push.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<5>, push.AddCategory("dary_heap d", "5").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<6>, push.AddCategory("dary_heap d", "6").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_push_dary_heap<8>, push.AddCategory("dary_heap d", "8").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_heap_baseline"));
    }
    {
        skb::CategoryBuilder make = builder.AddCategory("operation", "make");
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_make_heap, make.BuildCategories("heap", "std::heap"))->SetBaseline("benchmark_heap_baseline"));
        //SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_custom_make_heap, make.BuildCategories("heap", "custom_make_heap"))->SetBaseline("benchmark_heap_baseline"));
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
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_heap, pop.BuildCategories("heap", "std::heap"))->SetBaseline("benchmark_make_heap_baseline"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<2>, pop.AddCategory("dary_heap d", "2").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_2"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<3>, pop.AddCategory("dary_heap d", "3").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_3"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<4>, pop.AddCategory("dary_heap d", "4").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_4"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<5>, pop.AddCategory("dary_heap d", "5").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_5"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<6>, pop.AddCategory("dary_heap d", "6").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_6"));
        SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap<8>, pop.AddCategory("dary_heap d", "8").BuildCategories("heap", "dary_heap"))->SetBaseline("benchmark_make_dary_heap_baseline_8"));
        //SetHeapRange(SKA_BENCHMARK_CATEGORIES(&benchmark_pop_dary_heap_make_std_heap, pop.AddCategory("dary_heap d", "2").BuildCategories("heap", "pop_dary_heap_make_std_heap"))->SetBaseline("benchmark_make_heap_baseline"));
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

void register_ocaml_multiply()
{
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&xor_baseline>, "baseline", "ocaml_multiply_baseline");
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&ocaml_multiply>, "ocaml", "multiply")->SetBaseline("ocaml_multiply_baseline")->SetRange(4, 1024*1024)->SetRangeMultiplier(8);
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&my_ocaml_multiply>, "ocaml", "my_multiply")->SetBaseline("ocaml_multiply_baseline")->SetRange(4, 1024*1024)->SetRangeMultiplier(8);
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&my_ocaml_multiply_xor>, "ocaml", "my_multiply_xor")->SetBaseline("ocaml_multiply_baseline")->SetRange(4, 1024*1024)->SetRangeMultiplier(8);
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&ocaml_subtract>, "ocaml", "subtract")->SetBaseline("ocaml_multiply_baseline")->SetRange(4, 1024*1024)->SetRangeMultiplier(8);
    SKA_BENCHMARK_NAME(&skbenchmark_ocaml_multiply<&my_ocaml_subtract>, "ocaml", "my_subtract")->SetBaseline("ocaml_multiply_baseline")->SetRange(4, 1024*1024)->SetRangeMultiplier(8);
}
