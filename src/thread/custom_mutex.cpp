#include "thread/custom_mutex.hpp"

#include "test/include_test.hpp"
#include <thread>

TEST(futex_mutex, four_bytes)
{
    futex_mutex lock;
    std::atomic<int> state{0};
    auto test = [&]
    {
        for (int i = 0; i < 1024; ++i)
        {
            lock.lock();
            ASSERT_EQ(0, state.fetch_add(1, std::memory_order_relaxed));
            ASSERT_EQ(1, state.fetch_sub(1, std::memory_order_relaxed));
            lock.unlock();
        }
    };
    std::vector<std::thread> threads;
    threads.reserve(std::thread::hardware_concurrency());
    while (threads.size() != threads.capacity())
    {
        threads.emplace_back(test);
    }
    for (std::thread & t : threads)
    {
        t.join();
    }
}

TEST(futex_mutex, condition_variable)
{
    futex_mutex mutex;
    futex_condition_variable cv;

    int num_work = 0;
    std::vector<std::thread> threads;
    int num_workers = std::thread::hardware_concurrency();
    threads.reserve(num_workers);
    auto do_work = [&]
    {
        std::unique_lock<futex_mutex> lock(mutex);
        cv.wait(lock, [&]
        {
            return num_work > 0;
        });
        --num_work;
    };
    while (threads.size() != threads.capacity())
        threads.emplace_back(do_work);
    mutex.lock();
    num_work += num_workers;
    mutex.unlock();
    cv.notify_all();
    for (std::thread & t : threads)
        t.join();
    threads.clear();

    while (threads.size() != threads.capacity())
        threads.emplace_back(do_work);
    for (int i = 0; i < num_workers; ++i)
    {
        mutex.lock();
        ++num_work;
        mutex.unlock();
        cv.notify_one();
    }
    for (std::thread & t : threads)
        t.join();
    threads.clear();

    while (threads.size() != threads.capacity())
        threads.emplace_back(do_work);
    mutex.lock();
    num_work += num_workers;
    mutex.unlock();
    cv.notify_all(mutex);
    for (std::thread & t : threads)
        t.join();
    threads.clear();
}
