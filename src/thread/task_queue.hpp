#pragma once

#include <functional>
#include <future>
#include <type_traits>
//#include <dispatch/dispatch.h>
#include <deque>
#include <vector>
#include <thread>
#include <atomic>
#include "thread/custom_mutex.hpp"
#include "thread/blockingconcurrentqueue.h"

/*template<typename Function, typename... Args>
auto dispatch_async(Function && f, Args &&... args)
{
    using result_type = std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>;
    using packaged_type = std::packaged_task<result_type()>;

    auto p = new packaged_type(std::bind(std::forward<Function>(f), std::forward<Args>(args)...));
    auto result = p->get_future();
    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
                     p, [](void * ptr)
    {
        packaged_type * p = static_cast<packaged_type *>(ptr);
        (*p)();
        delete p;
    });
    return result;
}*/

struct notification_queue
{
    bool try_pop(std::function<void ()> & result)
    {
        std::unique_lock<mutex_type> lock(mutex, std::try_to_lock);
        if (!lock || queue.empty())
            return false;
        result = std::move(queue.front());
        queue.pop_front();
        return true;
    }
    bool pop(std::function<void ()> & result)
    {
        std::unique_lock<mutex_type> lock(mutex);
        while (queue.empty() && !is_done)
        {
            ready.wait(lock);
        }
        if (queue.empty())
            return false;
        result = std::move(queue.front());
        queue.pop_front();
        return true;
    }

    template<typename F>
    bool try_push(F && f)
    {
        {
            std::unique_lock<mutex_type> lock(mutex, std::try_to_lock);
            if (!lock)
                return false;
            queue.emplace_back(std::forward<F>(f));
        }
        ready.notify_one();
        return true;
    }
    template<typename F>
    void push(F && f)
    {
        {
            std::lock_guard<mutex_type> lock(mutex);
            queue.emplace_back(std::forward<F>(f));
        }
        ready.notify_one();
    }
    void done()
    {
        {
            std::lock_guard<mutex_type> lock(mutex);
            is_done = true;
        }
        ready.notify_all();
    }

private:
    bool is_done{false};
    std::deque<std::function<void ()>> queue;
#if 1
    using mutex_type = std::mutex;
    std::mutex mutex;
    std::condition_variable ready;
#else
    using mutex_type = futex_mutex;
    futex_mutex mutex;
    futex_condition_variable ready;
#endif
};

struct task_system
{
    task_system()
    {
        threads.reserve(count);
        for (unsigned i = 0; i < count; ++i)
        {
            threads.emplace_back([&, i]
            {
                run(i);
            });
        }
    }
    ~task_system()
    {
        for (notification_queue & queue : queues)
            queue.done();
        for (std::thread & thread : threads)
            thread.join();
    }

    template<typename F>
    void async(F && f)
    {
        unsigned i = next_index();
        for (unsigned n = count * 32, iter = i; n != 0; --n)
        {
            if (queues[iter].try_push(std::forward<F>(f)))
                return;
            ++iter;
            if (iter == count)
                iter = 0;
        }
        queues[i].push(std::forward<F>(f));
    }

private:
    void run(unsigned i)
    {
        while (true)
        {
            std::function<void ()> f;
            for (unsigned n = count, iter = i; n != 0; --n)
            {
                if (queues[iter].try_pop(f))
                    break;
                ++iter;
                if (iter == count)
                    iter = 0;
            }
            if (!f && !queues[i].pop(f))
                break;
            f();
        }
    }

    unsigned next_index()
    {
        unsigned i = index.fetch_add(1, std::memory_order_relaxed);
        if (i >= count)
        {
            unsigned old_value = i + 1;
            index.compare_exchange_weak(old_value, old_value - count, std::memory_order_relaxed);
            do
            {
                i -= count;
            }
            while (i >= count);
        }
        return i;
    }

    const unsigned count{std::thread::hardware_concurrency()};
    std::atomic<unsigned> index{0};
    std::vector<std::thread> threads;
    std::vector<notification_queue> queues{count};
};

struct task_system_moody
{
    task_system_moody()
    {
        threads.reserve(count);
        for (unsigned i = 0; i < count; ++i)
        {
            threads.emplace_back([&, i]
            {
                run(i);
            });
        }
    }
    ~task_system_moody()
    {
        done = true;
        for (auto & queue : queues)
            queue.queue.enqueue(queue.producer, []{});
        for (std::thread & thread : threads)
            thread.join();
        std::function<void ()> f;
        for (auto & queue : queues)
        {
            while (queue.queue.try_dequeue(queue.consumer, f))
                f();
        }
    }

    template<typename F>
    void async(F && f)
    {
        unsigned i = next_index();
        for (unsigned n = count * 32, iter = i; n != 0; --n)
        {
            Queue & queue = queues[iter];
            if (queue.queue.try_enqueue(queue.producer, std::forward<F>(f)))
                return;
            ++iter;
            if (iter == count)
                iter = 0;
        }
        Queue & queue = queues[i];
        queue.queue.enqueue(queue.producer, std::forward<F>(f));
    }

private:
    void run(unsigned i)
    {
        unsigned yield_interval = count * 2;
        while (!done)
        {
            std::function<void ()> f;
            for (unsigned iter = i, n = yield_interval;;)
            {
                Queue & queue = queues[iter];
                if (queue.queue.try_dequeue(queue.consumer, f))
                    break;
                ++iter;
                if (iter == count)
                    iter = 0;
                if (--n == 0)
                {
                    std::this_thread::yield();
                    n = yield_interval;
                }
            }
            f();
        }
    }

    unsigned next_index()
    {
        unsigned i = index.fetch_add(1, std::memory_order_relaxed);
        if (i >= count)
        {
            unsigned old_value = i + 1;
            index.compare_exchange_weak(old_value, old_value - count, std::memory_order_relaxed);
            do
            {
                i -= count;
            }
            while (i >= count);
        }
        return i;
    }

    const unsigned count{std::thread::hardware_concurrency()};
    //const unsigned count{1};
    std::atomic<unsigned> index{0};
    std::atomic<bool> done{false};
    std::vector<std::thread> threads;
    struct Queue
    {
        Queue()
            : producer(queue)
            , consumer(queue)
        {
        }

        moodycamel::ConcurrentQueue<std::function<void ()>> queue;
        moodycamel::ProducerToken producer;
        moodycamel::ConsumerToken consumer;
    };

    std::vector<Queue> queues{count};
};
