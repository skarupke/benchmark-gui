#include "util/finger_tree.hpp"


#include "test/include_test.hpp"

template<typename T>
std::vector<T> to_vec(const FingerTree<T> & tree)
{
    std::vector<T> result;
    tree.iter([&](const T & elem)
    {
        result.push_back(elem);
        return true;
    });
    return result;
}

TEST(finger_tree, empty)
{
    FingerTree<int> tree;
    ASSERT_TRUE(tree.empty());
    std::vector<int> as_vec = to_vec(tree);
    ASSERT_TRUE(as_vec.empty());
}

TEST(finger_tree, single_item)
{
    FingerTree<int> tree;
    tree.push_left(5);
    ASSERT_EQ(5, tree.left());
    ASSERT_EQ(5, tree.right());
    std::vector<int> as_vec = to_vec(tree);
    ASSERT_EQ(1u, as_vec.size());
    ASSERT_EQ(5, as_vec[0]);
}

TEST(finger_tree, many_items)
{
    constexpr int num_items = 2000;
    FingerTree<int> tree;
    for (int i = num_items - 1; i >= 0; --i)
    {
        tree.push_left(i);
        ASSERT_EQ(i, tree.left());

        /*std::cout << "i: " << i << ". Tree:\n";
        tree.print_tree(std::cout);
        std::cout << std::endl;*/
    }
    ASSERT_EQ(num_items - 1, tree.right());
    std::vector<int> as_vec = to_vec(tree);
    for (size_t i = 0; i < static_cast<size_t>(num_items); ++i)
    {
        ASSERT_EQ(i, as_vec[i]);
    }
}

TEST(finger_tree, many_items_right)
{
    constexpr int num_items = 2000;
    FingerTree<int> tree;
    for (int i = 0; i < num_items; ++i)
    {
        tree.push_right(i);
        ASSERT_EQ(i, tree.right());

        /*std::cout << "i: " << i << ". Tree:\n";
        tree.print_tree(std::cout);
        std::cout << std::endl;*/
    }
    ASSERT_EQ(0, tree.left());
    std::vector<int> as_vec = to_vec(tree);
    for (size_t i = 0; i < static_cast<size_t>(num_items); ++i)
    {
        ASSERT_EQ(i, as_vec[i]);
    }
}

TEST(finger_tree, pop_left)
{
    FingerTree<int> tree;
    for (int i = 0; i < 100; ++i)
    {
        tree.push_left(99 - i);
    }
    for (int i = 0; i < 100; ++i)
    {
        /*std::cout << i << ":\n";
        tree.print_tree(std::cout);
        std::cout << std::endl;*/
        ASSERT_FALSE(tree.empty());
        ASSERT_EQ(i, tree.left());
        tree.pop_left();
    }
    ASSERT_TRUE(tree.empty());
}
TEST(finger_tree, push_right_pop_left)
{
    FingerTree<int> tree;
    for (int i = 0; i < 100; ++i)
    {
        tree.push_right(i);
    }
    for (int i = 0; i < 100; ++i)
    {
        /*std::cout << i << ":\n";
        tree.print_tree(std::cout);
        std::cout << std::endl;*/
        ASSERT_FALSE(tree.empty());
        ASSERT_EQ(i, tree.left());
        tree.pop_left();
    }
    ASSERT_TRUE(tree.empty());
}

TEST(finger_tree, pop_right)
{
    FingerTree<int> tree;
    for (int i = 0; i < 100; ++i)
    {
        tree.push_right(99 - i);
    }
    for (int i = 0; i < 100; ++i)
    {
        /*std::cout << i << ":\n";
        tree.print_tree(std::cout);
        std::cout << std::endl;*/
        ASSERT_FALSE(tree.empty());
        ASSERT_EQ(i, tree.right());
        tree.pop_right();
    }
    ASSERT_TRUE(tree.empty());
}
TEST(finger_tree, push_left_pop_right)
{
    FingerTree<int> tree;
    for (int i = 0; i < 100; ++i)
    {
        tree.push_left(i);
    }
    for (int i = 0; i < 100; ++i)
    {
        /*std::cout << i << ":\n";
        tree.print_tree(std::cout);
        std::cout << std::endl;*/
        ASSERT_FALSE(tree.empty());
        ASSERT_EQ(i, tree.right());
        tree.pop_right();
    }
    ASSERT_TRUE(tree.empty());
}
TEST(finger_tree, DISABLED_iter_at_measure)
{
    FingerTree<int> tree;
    for (int i = 0; i < 100; ++i)
    {
        tree.push_right(i % 2);
    }
    int sum = 0;
    tree.iter_right_from([](int i){ return i >= 90; }, [&](int i)
    {
        sum += i;
        return true;
    });
    ASSERT_EQ(5, sum);
}

