#pragma once

#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <atomic>
#include "debug/assert.hpp"
#include <mutex>

struct futex_mutex
{
    void lock()
    {
        if (state.exchange(locked, std::memory_order_acquire) == unlocked)
            return;
        while (state.exchange(sleeper, std::memory_order_acquire) != unlocked)
        {
            syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, sleeper, nullptr, nullptr, 0);
        }
    }
    bool try_lock()
    {
        unsigned expected = unlocked;
        return state.compare_exchange_strong(expected, locked, std::memory_order_acquire);
    }
    void unlock()
    {
        if (state.exchange(unlocked, std::memory_order_release) == sleeper)
            syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }

    // this function is only called from the condition variable code below
    // it tells this mutex that it has to call FUTEX_WAKE later
    void set_to_wake_later()
    {
        CHECK_FOR_PROGRAMMER_ERROR(state.load(std::memory_order_relaxed) >= locked);
        state.store(sleeper, std::memory_order_relaxed);
    }

private:
    std::atomic<unsigned> state{unlocked};

    static constexpr unsigned unlocked = 0;
    static constexpr unsigned locked = 1;
    static constexpr unsigned sleeper = 2;
};

struct futex_condition_variable
{
    void wait(std::unique_lock<futex_mutex> & lock)
    {
        unsigned old_state = state.load(std::memory_order_relaxed);
        lock.unlock();
        syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, old_state, nullptr, nullptr, 0);
        lock.lock();
        lock.mutex()->set_to_wake_later();
    }
    template<typename Predicate>
    void wait(std::unique_lock<futex_mutex> & lock, Predicate predicate)
    {
        while (!predicate())
            wait(lock);
    }
    void notify_one()
    {
        state.fetch_add(1, std::memory_order_relaxed);
        syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }
    void notify_all()
    {
        state.fetch_add(1, std::memory_order_relaxed);
        syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
    }
    // if you can provide the mutex that waiters are sleeping on, there can be a more
    // efficient implementation. the above notify_all() will wake all sleepers and
    // then they will immediately block because they're all trying to acquire the mutex
    // at the same time. this one instead transfers the sleepers to the lock, so they
    // all get woken up one at a time as the lock unlocks.
    void notify_all(futex_mutex & mutex)
    {
        state.fetch_add(1, std::memory_order_relaxed);
        syscall(SYS_futex, &state, FUTEX_REQUEUE_PRIVATE, 1, std::numeric_limits<int>::max(), &mutex, 0);
    }
private:
    std::atomic<unsigned> state{0};
};
