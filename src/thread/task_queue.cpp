#include "thread/task_queue.hpp"

#include <benchmark/benchmark.h>
#include <debug/assert.hpp>

static constexpr size_t num_runs = 1024;

void task_queue_lib_dispatch(benchmark::State & state)
{
    std::atomic<size_t> sum{0};
    std::vector<std::future<void>> results;
    results.reserve(num_runs);
    while (state.KeepRunning())
    {
        sum = 0;
        for (size_t i = num_runs; i != 0; --i)
        {
            results.push_back(dispatch_async([&]{ sum.fetch_add(1, std::memory_order_relaxed); }));
        }
        for (std::future<void> & future : results)
            future.get();
        results.clear();
    }
    CHECK_FOR_PROGRAMMER_ERROR(sum == num_runs);
    state.SetItemsProcessed(num_runs * state.iterations());
}

BENCHMARK(task_queue_lib_dispatch);

void task_queue_lib_dispatch_function_pointer(benchmark::State & state)
{
    struct SharedState
    {
        std::atomic<size_t> sum{0};
        std::mutex mutex;
        std::condition_variable cv;
    }
    shared_state;
    auto to_run = [](void * ptr)
    {
        SharedState * state = static_cast<SharedState *>(ptr);
        int result = state->sum.fetch_add(1, std::memory_order_relaxed) + 1;
        if (result == num_runs)
        {
            state->mutex.lock();
            state->mutex.unlock();
            state->cv.notify_one();
        }
    };
    dispatch_queue_t global_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    size_t longest = 0;
    size_t shortest = std::numeric_limits<size_t>::max();
    size_t average = 0;
    while (state.KeepRunning())
    {
        auto time_before = std::chrono::high_resolution_clock::now();
        shared_state.sum = 0;
        for (size_t i = 0; i < num_runs; ++i)
        {
            dispatch_async_f(global_queue, &shared_state, to_run);
        }
        {
            std::unique_lock<std::mutex> lock(shared_state.mutex);
            while (shared_state.sum != num_runs)
                shared_state.cv.wait(lock);
        }
        size_t time_passed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time_before).count();
        longest = std::max(longest, time_passed);
        shortest = std::min(shortest, time_passed);
        average += time_passed;
    }
    CHECK_FOR_PROGRAMMER_ERROR(shared_state.sum == num_runs);
    state.SetItemsProcessed(num_runs * state.iterations());
    state.counters["longest"] = longest;
    state.counters["shortest"] = shortest;
    state.counters["average"] = average / state.iterations();
}
BENCHMARK(task_queue_lib_dispatch_function_pointer);

void task_queue_sean_parent(benchmark::State & state)
{
    task_system ts;
    struct SharedState
    {
        std::atomic<size_t> sum{0};
        std::mutex mutex;
        std::condition_variable cv;
    }
    shared_state;
    while (state.KeepRunning())
    {
        shared_state.sum = 0;
        for (size_t i = 0; i < num_runs; ++i)
        {
            ts.async([&shared_state]
            {
                int result = shared_state.sum.fetch_add(1, std::memory_order_relaxed) + 1;
                if (result == num_runs)
                {
                    shared_state.mutex.lock();
                    shared_state.mutex.unlock();
                    shared_state.cv.notify_one();
                }
            });
        }
        {
            std::unique_lock<std::mutex> lock(shared_state.mutex);
            while (shared_state.sum != num_runs)
                shared_state.cv.wait(lock);
        }
    }
    CHECK_FOR_PROGRAMMER_ERROR(shared_state.sum == num_runs);
    state.SetItemsProcessed(num_runs * state.iterations());
}
BENCHMARK(task_queue_sean_parent);

void task_queue_moodycamel(benchmark::State & state)
{
    task_system_moody ts;
    struct SharedState
    {
        std::atomic<size_t> sum{0};
        std::mutex mutex;
        std::condition_variable cv;
    }
    shared_state;
    size_t longest = 0;
    size_t shortest = std::numeric_limits<size_t>::max();
    size_t average = 0;
    while (state.KeepRunning())
    {
        auto time_before = std::chrono::high_resolution_clock::now();
        shared_state.sum = 0;
        for (size_t i = 0; i < num_runs; ++i)
        {
            ts.async([&shared_state]
            {
                int result = shared_state.sum.fetch_add(1, std::memory_order_relaxed) + 1;
                if (result == num_runs)
                {
                    shared_state.mutex.lock();
                    shared_state.mutex.unlock();
                    shared_state.cv.notify_one();
                }
            });
        }
        {
            std::unique_lock<std::mutex> lock(shared_state.mutex);
            while (shared_state.sum != num_runs)
                shared_state.cv.wait(lock);
        }
        size_t time_passed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time_before).count();
        longest = std::max(longest, time_passed);
        shortest = std::min(shortest, time_passed);
        average += time_passed;
    }
    CHECK_FOR_PROGRAMMER_ERROR(shared_state.sum == num_runs);
    state.SetItemsProcessed(num_runs * state.iterations());
    state.counters["longest"] = longest;
    state.counters["shortest"] = shortest;
    state.counters["average"] = average / state.iterations();
}
BENCHMARK(task_queue_moodycamel);

void task_queue_async(benchmark::State & state)
{
    std::atomic<int> sum{0};
    std::vector<std::future<void>> results;
    results.reserve(num_runs);
    while (state.KeepRunning())
    {
        sum = 0;
        while (results.capacity() != results.size())
        {
            results.push_back(std::async(std::launch::async, [&]{ sum.fetch_add(1, std::memory_order_relaxed); }));
        }
        for (std::future<void> & future : results)
            future.get();
        results.clear();
    }
    CHECK_FOR_PROGRAMMER_ERROR(sum == num_runs);
    state.SetItemsProcessed(num_runs * state.iterations());
}
BENCHMARK(task_queue_async);



