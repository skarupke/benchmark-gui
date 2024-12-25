#pragma once

#include <atomic>

/*struct one_byte_mutex
{
    one_byte_mutex()
        : state(0)
    {
    }

    void lock()
    {
        if (state.exchange(locked, std::memory_order_acquire) == unlocked)
            return;
        while (state.exchange(sleeper, std::memory_order_acquire) != unlocked)
            state.wait(sleeper, std::memory_order_relaxed);
    }
    void unlock()
    {
        if (state.exchange(unlocked, std::memory_order_release) == sleeper)
            state.notify_one();
    }

private:
    std::atomic<uint8_t> state;

    static constexpr unsigned unlocked = 0;
    static constexpr unsigned locked = 1;
    static constexpr unsigned sleeper = 2;
};*/

