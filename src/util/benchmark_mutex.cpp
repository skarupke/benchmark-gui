#ifdef DONE_PULLING_OUT_BINARY
#if 1
#include <mutex>
#include <thread>
#include <list>

#include "benchmark/benchmark.h"
#include <algorithm>
#include <chrono>
#include <array>
#include <condition_variable>
#include <shared_mutex>
#include "thread/custom_mutex.hpp"
#include <semaphore.h>
#include <cstring>
#include <emmintrin.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <pthread.h>
#include <numeric>
#include <random>
#include "test/include_test.hpp"
#include "custom_benchmark/custom_benchmark.h"
#include <deque>
#include "util/random_seed_seq.hpp"

// todo: try WTF lock (aka parking lot)

struct null_mutex
{
    void lock()
    {
    }
    void unlock()
    {
    }
};

struct pthread_mutex
{
    ~pthread_mutex()
    {
        pthread_mutex_destroy(&mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

private:
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
};
struct pthread_mutex_recursive
{
    ~pthread_mutex_recursive()
    {
        pthread_mutex_destroy(&mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

private:
    pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
};
struct pthread_mutex_adaptive
{
    ~pthread_mutex_adaptive()
    {
        pthread_mutex_destroy(&mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

private:
    pthread_mutex_t mutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;
};
struct pthread_spinlock
{
    pthread_spinlock()
    {
        pthread_spin_init(&spinlock, false);
    }
    ~pthread_spinlock()
    {
        pthread_spin_destroy(&spinlock);
    }
    void lock()
    {
        pthread_spin_lock(&spinlock);
    }
    void unlock()
    {
        pthread_spin_unlock(&spinlock);
    }

private:
    pthread_spinlock_t spinlock;
};

struct ticket_mutex
{
    void lock()
    {
        unsigned my = in.fetch_add(1);
        for (;;)
        {
            unsigned now = out;
            if (my == now)
                break;
            syscall(SYS_futex, &out, FUTEX_WAIT_PRIVATE, now, nullptr, nullptr, 0);
        }
    }
    void unlock()
    {
        unsigned new_value = out.fetch_add(1) + 1;
        if (new_value != in)
            syscall(SYS_futex, &out, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
    }

private:
    std::atomic<unsigned> in{0};
    std::atomic<unsigned> out{0};
};

struct ticket_spinlock
{
    void lock()
    {
        unsigned my = in.fetch_add(1, std::memory_order_relaxed);
        for (int spin_count = 0; out.load(std::memory_order_acquire) != my; ++spin_count)
        {
            if (spin_count < 16)
                _mm_pause();
            else
            {
                std::this_thread::yield();
                spin_count = 0;
            }
        }
    }
    void unlock()
    {
        out.store(out.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }

private:
    std::atomic<unsigned> in{0};
    std::atomic<unsigned> out{0};
};

struct futex_mutex_careful_wake
{
    void lock()
    {
        for (;;)
        {
            unsigned old_state = state.load(std::memory_order_acquire);
            switch (old_state)
            {
            case unlocked:
                if (state.compare_exchange_strong(old_state, locked, std::memory_order_acquire))
                    return;
                [[fallthrough]];
            case locked:
                if (state.exchange(sleeper, std::memory_order_acquire) == unlocked)
                    return;
                [[fallthrough]];
            case sleeper:
                syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, sleeper, nullptr, nullptr, 0);
            }
        }
    }
    void unlock()
    {
        if (state.exchange(unlocked, std::memory_order_release) == sleeper)
            syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
    }

private:
    std::atomic<unsigned> state{unlocked};

    static constexpr unsigned unlocked = 0;
    static constexpr unsigned locked = 1;
    static constexpr unsigned sleeper = 2;
};
struct futex_mutex_counted
{
    void lock()
    {
        unsigned old_state = state.load(std::memory_order_acquire);
        if ((old_state & locked) == 0 && state.compare_exchange_strong(old_state, old_state | locked, std::memory_order_acquire))
            return;
        unsigned new_state = state.fetch_add(sleeper, std::memory_order_acquire) + sleeper;
        for (;;)
        {
            if ((new_state & locked) == 0 && state.compare_exchange_strong(new_state, new_state | locked, std::memory_order_acquire))
            {
                state.fetch_sub(sleeper, std::memory_order_relaxed);
                return;
            }
            if (new_state & locked)
                syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, new_state, nullptr, nullptr, 0);
            new_state = state.load(std::memory_order_acquire);
        }
    }
    void unlock()
    {
        if (state.fetch_and(~locked, std::memory_order_release) >= sleeper)
            syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }

private:
    std::atomic<unsigned> state{unlocked};

    static constexpr unsigned unlocked = 0;
    static constexpr unsigned locked = 1;
    static constexpr unsigned sleeper = 2;
};

#define FUTEX_SIZE_8_BIT	512
#define FUTEX_SIZE_16_BIT	1024
struct futex_mutex_wake_one_8
{
    void lock()
    {
        if (state.exchange(locked, std::memory_order_acquire) == unlocked)
            return;
        while (state.exchange(sleeper, std::memory_order_acquire) != unlocked)
        {
            if (syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE | FUTEX_SIZE_8_BIT, sleeper, nullptr, nullptr, 0))
            {
                CHECK_FOR_PROGRAMMER_ERROR(errno == EAGAIN);
            }
        }
    }
    void unlock()
    {
        if (state.exchange(unlocked, std::memory_order_release) == sleeper)
        {
            //RAW_VERIFY(syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0) >= 0);
            RAW_VERIFY(syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE | FUTEX_SIZE_8_BIT, 1, nullptr, nullptr, 0) >= 0);
        }
    }

    // this function is only called from the condition variable code below
    // it tells this mutex that it has to call FUTEX_WAKE later
    void set_to_wake_later()
    {
        CHECK_FOR_PROGRAMMER_ERROR(state.load(std::memory_order_relaxed) >= locked);
        state.store(sleeper, std::memory_order_relaxed);
    }

private:
    std::atomic<uint8_t> state{unlocked};

    static constexpr uint8_t unlocked = 0;
    static constexpr uint8_t locked = 1;
    static constexpr uint8_t sleeper = 2;
};
struct futex_mutex_wake_one_16
{
    void lock()
    {
        if (state.exchange(locked, std::memory_order_acquire) == unlocked)
            return;
        while (state.exchange(sleeper, std::memory_order_acquire) != unlocked)
        {
            syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE | FUTEX_SIZE_16_BIT, sleeper, nullptr, nullptr, 0);
        }
    }
    void unlock()
    {
        if (state.exchange(unlocked, std::memory_order_release) == sleeper)
        {
            //syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
            syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE | FUTEX_SIZE_16_BIT, 1, nullptr, nullptr, 0);
        }
    }

    // this function is only called from the condition variable code below
    // it tells this mutex that it has to call FUTEX_WAKE later
    void set_to_wake_later()
    {
        CHECK_FOR_PROGRAMMER_ERROR(state.load(std::memory_order_relaxed) >= locked);
        state.store(sleeper, std::memory_order_relaxed);
    }

private:
    std::atomic<uint16_t> state{unlocked};

    static constexpr uint16_t unlocked = 0;
    static constexpr uint16_t locked = 1;
    static constexpr uint16_t sleeper = 2;
};

// code from https://eli.thegreenplace.net/2018/basics-of-futexes/
struct futex_mutex_tricky
{
    void lock()
    {
        int c = cmpxchg(&atom_, 0, 1);
        // If the lock was previously unlocked, there's nothing else for us to do.
        // Otherwise, we'll probably have to wait.
        if (c != 0)
        {
            do
            {
                // If the mutex is locked, we signal that we're waiting by setting the
                // atom to 2. A shortcut checks is it's 2 already and avoids the atomic
                // operation in this case.
                if (c == 2 || cmpxchg(&atom_, 1, 2) != 0)
                {
                    // Here we have to actually sleep, because the mutex is actually
                    // locked. Note that it's not necessary to loop around this syscall;
                    // a spurious wakeup will do no harm since we only exit the do...while
                    // loop when atom_ is indeed 0.
                    syscall(SYS_futex, (int*)&atom_, FUTEX_WAIT, 2, 0, 0, 0);
                }
                // We're here when either:
                // (a) the mutex was in fact unlocked (by an intervening thread).
                // (b) we slept waiting for the atom and were awoken.
                //
                // So we try to lock the atom again. We set teh state to 2 because we
                // can't be certain there's no other thread at this exact point. So we
                // prefer to err on the safe side.
            } while ((c = cmpxchg(&atom_, 0, 2)) != 0);
        }
    }

    void unlock()
    {
        if (atom_.fetch_sub(1) != 1)
        {
            atom_.store(0);
            syscall(SYS_futex, (int*)&atom_, FUTEX_WAKE, 1, 0, 0, 0);
        }
    }

private:
    // 0 means unlocked
    // 1 means locked, no waiters
    // 2 means locked, there are waiters in lock()
    std::atomic<int> atom_;

    static int cmpxchg(std::atomic<int>* atom, int expected, int desired)
    {
        int* ep = &expected;
        std::atomic_compare_exchange_strong(atom, ep, desired);
        return *ep;
    }
};

struct futex_condition_variable_counted
{
    void wait(std::unique_lock<futex_mutex> & lock)
    {
        unsigned to_compare = state.fetch_add(waiters_to_add_bit, std::memory_order_relaxed) + waiters_to_add_bit;
        lock.unlock();
        for (;;)
        {
            syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, to_compare, nullptr, nullptr, 0);
            to_compare = state.load(std::memory_order_relaxed);
            while (to_compare >= need_to_wake_bit)
            {
                if (state.compare_exchange_weak(to_compare, to_compare - (waiters_to_add_bit | need_to_wake_bit), std::memory_order_relaxed))
                    goto done_waiting;
            }
        }
        done_waiting:
        lock.lock();
        if ((to_compare % need_to_wake_bit) >= 2 * waiters_to_add_bit)
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
        unsigned old_value = state.load(std::memory_order_relaxed);
        while ((old_value / need_to_wake_bit) < (old_value % need_to_wake_bit))
        {
            if (state.compare_exchange_weak(old_value, old_value + need_to_wake_bit, std::memory_order_relaxed))
            {
                syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
                break;
            }
        }
    }
    void notify_all()
    {
        unsigned old_value = state.load(std::memory_order_relaxed);
        while ((old_value / need_to_wake_bit) < (old_value % need_to_wake_bit))
        {
            unsigned num_waiting = old_value % need_to_wake_bit;
            if (state.compare_exchange_weak(old_value, num_waiting | (num_waiting * need_to_wake_bit), std::memory_order_relaxed))
            {
                syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
                break;
            }
        }
    }
    // if you can provide the mutex that waiters are sleeping on, there can be a more
    // efficient implementation. the above notify_all() will wake all sleepers and
    // then they will immediately block because they're all trying to acquire the mutex
    // at the same time. this one instead transfers the sleepers to the lock, so they
    // all get woken up one at a time as the lock unlocks.
    void notify_all(futex_mutex & mutex)
    {
        unsigned old_value = state.load(std::memory_order_relaxed);
        while ((old_value / need_to_wake_bit) < (old_value % need_to_wake_bit))
        {
            unsigned num_waiting = old_value % need_to_wake_bit;
            if (state.compare_exchange_weak(old_value, num_waiting | (num_waiting * need_to_wake_bit), std::memory_order_relaxed))
            {
                syscall(SYS_futex, &state, FUTEX_REQUEUE_PRIVATE, 1, std::numeric_limits<int>::max(), &mutex, 0);
                break;
            }
        }
    }
private:
    std::atomic<unsigned> state{nobody_waiting};
    static constexpr unsigned nobody_waiting = 0;
    static constexpr unsigned need_to_wake_bit = 0x10000;
    static constexpr unsigned waiters_to_add_bit = 1;
};

struct futex_condition_variable_two_bytes
{
    void wait(std::unique_lock<futex_mutex_wake_one_8> & lock)
    {
        uint16_t to_compare = state.fetch_add(waiters_to_add_bit, std::memory_order_relaxed) + waiters_to_add_bit;
        lock.unlock();
        for (;;)
        {
            // only wait on 8 bits because we only care about the
            // need_to_wake_bit. if we also waited for the upper bits we
            // could run into a problem in this case:
            // thread A: cv.wait()
            // ... some time later:
            // thread B: cv.notify_one()
            // ... some time later:
            // thread C: cv.wait() // and at the same time:
            // thread D: cv.wait()
            // in this case it can happen that thread A remains blocked but
            // thread C or D can go on. that is not allowed in the standard
            // condition variables. you can not be woken up by something that
            // happened before you waited. in this case thread A should wake
            // up and thread C and D should go to sleep
            // waiting only on the wake bit means that no wait will
            // accidentally wake us up. even with this there is still a
            // potential bug in this sequence:
            // thread A: cv.wait()
            // thread B: cv.wait()
            // ... some time later:
            // thread C: cv.notify_one()
            // ... some time later:
            // thread D: cv.wait() // and at the same time:
            // thread E: cv.wait() // and at the same time:
            // thread F: cv.notify_one()
            // in this case A and B can remain sleeping but D and E can get
            // woken up. which is not allowed by the C++ standard: at most one
            // of them should be woken up because only one notify happened
            // at the same time. the reason for this is that it can take an
            // arbitrary amount of time after the notify before thread A
            // actually does the compare_exchange_weak. so to ensure fairness
            // we need to somehow block until A or B has done the unlock from
            // the first notify_one. I don't know how to do that without a
            // considerably more complex implementation.
            RAW_VERIFY(syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE | FUTEX_SIZE_8_BIT, to_compare % waiters_to_add_bit, nullptr, nullptr, 0) == 0 || errno == EAGAIN);
            to_compare = state.load(std::memory_order_relaxed);
            while (to_compare % waiters_to_add_bit)
            {
                if (state.compare_exchange_weak(to_compare, to_compare - (waiters_to_add_bit | need_to_wake_bit), std::memory_order_relaxed))
                    goto done_waiting;
            }
        }
        done_waiting:
        lock.lock();
        if (to_compare / waiters_to_add_bit >= 2)
            lock.mutex()->set_to_wake_later();
    }
    template<typename Predicate>
    void wait(std::unique_lock<futex_mutex_wake_one_8> & lock, Predicate predicate)
    {
        while (!predicate())
            wait(lock);
    }
    void notify_one()
    {
        uint16_t old_value = state.load(std::memory_order_relaxed);
        while (can_notify(old_value))
        {
            if (state.compare_exchange_weak(old_value, old_value + need_to_wake_bit, std::memory_order_relaxed))
            {
                //RAW_VERIFY(syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0) >= 0);
                RAW_VERIFY(syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE | FUTEX_SIZE_8_BIT, 1, nullptr, nullptr, 0) >= 0);
                break;
            }
        }
    }
    void notify_all()
    {
        uint16_t old_value = state.load(std::memory_order_relaxed);
        while (can_notify(old_value))
        {
            if (state.compare_exchange_weak(old_value, wake_all_pattern(old_value), std::memory_order_relaxed))
            {
                //RAW_VERIFY(syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0) >= 0);
                RAW_VERIFY(syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE | FUTEX_SIZE_8_BIT, std::numeric_limits<int>::max(), nullptr, nullptr, 0) >= 0);
                break;
            }
        }
    }
    // if you can provide the mutex that waiters are sleeping on, there can be a more
    // efficient implementation. the above notify_all() will wake all sleepers and
    // then they will immediately block because they're all trying to acquire the mutex
    // at the same time. this one instead transfers the sleepers to the lock, so they
    // all get woken up one at a time as the lock unlocks.
    void notify_all(futex_mutex_wake_one_8 & mutex)
    {
        uint16_t old_value = state.load(std::memory_order_relaxed);
        while (can_notify(old_value))
        {
            if (state.compare_exchange_weak(old_value, wake_all_pattern(old_value), std::memory_order_relaxed))
            {
                RAW_VERIFY(syscall(SYS_futex, &state, FUTEX_REQUEUE_PRIVATE, 1, std::numeric_limits<int>::max(), &mutex, 0) >= 0);
                break;
            }
        }
    }
private:
    std::atomic<uint16_t> state{nobody_waiting};
    static constexpr uint16_t nobody_waiting = 0;
    static constexpr uint16_t need_to_wake_bit = 1;
    static constexpr uint16_t waiters_to_add_bit = 0x100;

    static bool can_notify(uint16_t old_value)
    {
        return (old_value / waiters_to_add_bit) > (old_value % waiters_to_add_bit);
    }
    static uint16_t wake_all_pattern(uint16_t old_value)
    {
        uint16_t num_waiting = old_value / waiters_to_add_bit;
        return num_waiting * (need_to_wake_bit | waiters_to_add_bit);
    }
};

struct futex_condition_variable_two_bytes_increment_only
{
    void wait(std::unique_lock<futex_mutex_wake_one_8> & lock)
    {
        uint16_t to_compare = state.load(std::memory_order_relaxed);
        for (;;)
        {
            if (queue_is_full(to_compare))
            {
                // the queue is full. don't increment. just sleep. notify_one will
                // do notify_all instead. it's bad for performance but we have to
                // handle this case.
                lock.unlock();
                for (;;)
                {
                    syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE | FUTEX_SIZE_16_BIT, to_compare, nullptr, nullptr, 0);
                    if (state.load(std::memory_order_relaxed) != to_compare)
                    {
                        lock.lock();
                        lock.mutex()->set_to_wake_later();
                        return;
                    }
                }
            }
            if (state.compare_exchange_weak(to_compare, to_compare + waiters_to_add_bit, std::memory_order_relaxed))
            {
                to_compare += waiters_to_add_bit;
                break;
            }
        }
        uint16_t wake_bits = to_compare % waiters_to_add_bit;
        lock.unlock();
        for (;;)
        {
            syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE | FUTEX_SIZE_16_BIT, to_compare, nullptr, nullptr, 0);
            to_compare = state.load(std::memory_order_relaxed);
            uint16_t new_wake_bits = to_compare % waiters_to_add_bit;
            if (new_wake_bits != wake_bits)
            {
                lock.lock();
                if (subtract_clamped(new_wake_bits, wake_bits) >= 2)
                    lock.mutex()->set_to_wake_later();
                return;
            }
        }
    }
    template<typename Predicate>
    void wait(std::unique_lock<futex_mutex_wake_one_8> & lock, Predicate predicate)
    {
        while (!predicate())
            wait(lock);
    }
    void notify_one()
    {
        uint16_t old_value = state.load(std::memory_order_relaxed);
        while (has_waiters(old_value))
        {
            if (queue_is_full(old_value))
            {
                if (state.compare_exchange_weak(old_value, wake_all_pattern(old_value), std::memory_order_relaxed))
                {
                    syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
                    break;
                }
            }
            else if (state.compare_exchange_weak(old_value, add_clamped(old_value), std::memory_order_relaxed))
            {
                syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
                break;
            }
        }
    }
    void notify_all()
    {
        uint16_t old_value = state.load(std::memory_order_relaxed);
        while (has_waiters(old_value))
        {
            if (state.compare_exchange_weak(old_value, wake_all_pattern(old_value), std::memory_order_relaxed))
            {
                syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
                break;
            }
        }
    }
    // if you can provide the mutex that waiters are sleeping on, there can be a more
    // efficient implementation. the above notify_all() will wake all sleepers and
    // then they will immediately block because they're all trying to acquire the mutex
    // at the same time. this one instead transfers the sleepers to the lock, so they
    // all get woken up one at a time as the lock unlocks.
    void notify_all(futex_mutex_wake_one_8 & mutex)
    {
        uint16_t old_value = state.load(std::memory_order_relaxed);
        while (has_waiters(old_value))
        {
            if (state.compare_exchange_weak(old_value, wake_all_pattern(old_value), std::memory_order_relaxed))
            {
                syscall(SYS_futex, &state, FUTEX_REQUEUE_PRIVATE, 1, std::numeric_limits<int>::max(), &mutex, 0);
                break;
            }
        }
    }
private:
    std::atomic<uint16_t> state{0};
    static constexpr uint16_t waiters_to_add_bit = 0x100;

    inline static uint16_t wake_all_pattern(uint16_t old_value)
    {
        uint16_t num_waiters = old_value / waiters_to_add_bit;
        return num_waiters * (waiters_to_add_bit + 1);
    }

    inline static bool queue_is_full(uint16_t value)
    {
        return !has_waiters(value + waiters_to_add_bit);
    }

    inline static bool has_waiters(uint16_t old_value)
    {
        return old_value / waiters_to_add_bit != old_value % waiters_to_add_bit;
    }
    inline static uint16_t add_clamped(uint16_t old_value)
    {
        if (old_value % waiters_to_add_bit == waiters_to_add_bit - 1)
            return old_value - (waiters_to_add_bit - 1);
        else
            return old_value + 1;
    }
    inline static uint16_t subtract_clamped(uint16_t a, uint16_t b)
    {
        return (a - b) % waiters_to_add_bit;
    }
};

struct futex_condition_variable_one_byte
{
    void wait(std::unique_lock<futex_mutex> & lock)
    {
        uint8_t loaded = state.load(std::memory_order_relaxed);
        lock.unlock();
        uint8_t wake_all_cycle = get_wake_all_cycle(loaded);
        for (;;)
        {
            while (!(loaded & has_sleeper_bit))
            {
                if (state.compare_exchange_weak(loaded, loaded | has_sleeper_bit, std::memory_order_relaxed))
                {
                    loaded |= has_sleeper_bit;
                    break;
                }
                while ((loaded & all_wake_one_bits) && (loaded & has_sleeper_bit))
                {
                    if (state.compare_exchange_weak(loaded, loaded - (wake_one_bit | has_sleeper_bit), std::memory_order_relaxed))
                        goto done_waiting;
                }
                if (get_wake_all_cycle(loaded) != wake_all_cycle)
                    goto done_waiting;
            }
            syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, loaded, nullptr, nullptr, 0);
            loaded = state.load(std::memory_order_relaxed);
            while ((loaded & all_wake_one_bits) && (loaded & has_sleeper_bit))
            {
                if (state.compare_exchange_weak(loaded, loaded - (wake_one_bit | has_sleeper_bit)))
                    goto done_waiting;
            }
            if (get_wake_all_cycle(loaded) != wake_all_cycle)
                break;
        }
        done_waiting:
        lock.lock();
        if (get_wake_all_cycle(loaded) != wake_all_cycle)
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
        uint8_t old_value = state.load(std::memory_order_relaxed);
        for (;;)
        {
            if ((old_value & all_wake_one_bits) == all_wake_one_bits)
            {
                notify_all();
                return;
            }
            else if (state.compare_exchange_weak(old_value, old_value + wake_one_bit, std::memory_order_relaxed))
            {
                syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 2, nullptr, nullptr, 0);
                break;
            }
        }
    }
    void notify_all()
    {
        uint8_t old_value = state.load(std::memory_order_relaxed);
        while (old_value & has_sleeper_bit)
        {
            if (state.compare_exchange_weak(old_value, old_value + wake_all_bit - (old_value % wake_all_bit)))
            {
                syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
                break;
            }
        }
    }
    // if you can provide the mutex that waiters are sleeping on, there can be a more
    // efficient implementation. the above notify_all() will wake all sleepers and
    // then they will immediately block because they're all trying to acquire the mutex
    // at the same time. this one instead transfers the sleepers to the lock, so they
    // all get woken up one at a time as the lock unlocks.
    void notify_all(futex_mutex & mutex)
    {
        uint8_t old_value = state.load(std::memory_order_relaxed);
        while (old_value & has_sleeper_bit)
        {
            if (state.compare_exchange_weak(old_value, old_value + wake_all_bit - (old_value % wake_all_bit)))
            {
                syscall(SYS_futex, &state, FUTEX_REQUEUE_PRIVATE, 1, std::numeric_limits<int>::max(), &mutex, 0);
                break;
            }
        }
    }
private:
    std::atomic<uint8_t> state{0};
    static constexpr uint8_t has_sleeper_bit = 1;
    static constexpr uint8_t wake_one_bit = 2;
    static constexpr uint8_t all_wake_one_bits = 2 | 4;
    static constexpr uint8_t wake_all_bit = 8;

    static inline uint8_t get_wake_all_cycle(uint8_t value)
    {
        return value / wake_all_bit;
    }
};

struct two_byte_futex_mutex
{
    two_byte_futex_mutex()
    {
        i.as_int = 0;
    }
    void lock()
    {
        int spin_count = 0;
        while (i.a.fast_path.load(std::memory_order_acquire) || i.a.fast_path.exchange(true, std::memory_order_acquire))
        {
            ++spin_count;
            if (spin_count < spin_yield_count)
                _mm_pause();
            else if (spin_count < max_spin_count)
                std::this_thread::yield();
            else
            {
                i.a.need_to_notify.store(true, std::memory_order_release);
                int to_compare = value_to_compare_against();
                spin_count = spin_yield_count - 1;
                syscall(SYS_futex, &i.as_int, FUTEX_WAIT_PRIVATE, to_compare, nullptr, nullptr, 0);
            }
        }
    }
    void unlock()
    {
        i.a.fast_path.store(false, std::memory_order_release);
        // todo: not sure about this. I need to make sure that the load
        // from need_to_modify does not get re-ordered before the store
        // in fast_path. I only need that ordering within this thread
        //
        // I think right now it's valid for the first load to be re-ordered
        // before the store in which case the short circuiting would not
        // run the exchange
        std::atomic_thread_fence(std::memory_order_seq_cst);
        //_mm_mfence();
        if (i.a.need_to_notify.load(std::memory_order_acquire)/* && i.a.need_to_notify.exchange(false, std::memory_order_acq_rel)*/)
        //if (i.a.need_to_notify.exchange(false, std::memory_order_acq_rel))
        {
            i.a.need_to_notify.store(false, std::memory_order_release);
            syscall(SYS_futex, &i.as_int, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
        }
    }

private:
    union int_union
    {
        int_union()
            : as_int(0)
        {
        }
        struct atomics
        {
            std::atomic<bool> fast_path;
            std::atomic<bool> need_to_notify;
        };
        atomics a;
        int as_int;
    };
    int_union i;
    static int value_to_compare_against()
    {
        int_union result;
        result.as_int = 0;
        result.a.fast_path.store(true, std::memory_order_relaxed);
        result.a.need_to_notify.store(true, std::memory_order_relaxed);
        return result.as_int;
    }
    static constexpr int max_spin_count = 64;
    static constexpr int spin_yield_count = 60;
};
#if 0
struct spin_to_futex_mutex
{
    spin_to_futex_mutex()
    {
        i.as_int = 0;
    }
    void lock()
    {
        uint8_t old_state;
        if (try_lock_internal(old_state))
            return;
        for (;;)
        {
            _mm_pause();
            // old_state == locked || old_state == go_to_sleep
            if (old_state == locked)
            {
                old_state = i.state.exchange(go_to_sleep, std::memory_order_acquire);
                // old_state == unlocked || old_state == locked || old_state == go_to_sleep
                if (old_state == locked)
                    break;
                else if (old_state == unlocked)
                {
                    i.state.store(locked, std::memory_order_release);
                    goto wake_and_return;
                }
            }
            // old_state == go_to_sleep
            syscall(SYS_futex, &i.as_int, FUTEX_WAIT_PRIVATE, go_to_sleep, nullptr, nullptr, 0);
            if (try_lock_internal(old_state))
                goto wake_and_return;
        }
        for (;;)
        {
            std::this_thread::yield();
            if (try_lock_internal(old_state))
                break;
            _mm_pause();
            if (try_lock_internal(old_state))
                break;
        }
        wake_and_return:
        need_to_wake_after = true;
    }
    bool try_lock()
    {
        uint8_t old_state;
        return try_lock_internal(old_state);
    }
    void unlock()
    {
        bool will_wake = need_to_wake_after;
        need_to_wake_after = false;
        i.state.store(unlocked, std::memory_order_release);
        if (will_wake)
            syscall(SYS_futex, &i.as_int, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }

private:
    union int_union
    {
        std::atomic<uint8_t> state;
        int as_int;
    };
    int_union i;
    bool need_to_wake_after = false;
    static constexpr uint8_t unlocked = 0;
    static constexpr uint8_t locked = 1;
    static constexpr uint8_t go_to_sleep = 2;
    //static constexpr uint8_t need_to_wake_after = 4;

    inline bool try_lock_internal(uint8_t & old_state)
    {
        old_state = i.state.load(std::memory_order_acquire);
        // old_state == unlocked || old_state == locked || old_state == go_to_sleep
        return old_state == unlocked && i.state.compare_exchange_strong(old_state, locked, std::memory_order_acquire);
    }
};
#elif 0
struct spin_to_futex_mutex
{
    void lock()
    {
        unsigned old_state;
        if (try_lock_internal(old_state))
            return;
        for (;;)
        {
            _mm_pause();
            // old_state == locked || old_state == go_to_sleep
            if (!(old_state & go_to_sleep))
            {
                old_state = state.fetch_or(go_to_sleep, std::memory_order_acquire);
                // old_state == unlocked || old_state == locked || old_state == go_to_sleep
                if (!(old_state & (locked | go_to_sleep)))
                {
                    state.fetch_or(locked, std::memory_order_release);
                    goto wake_and_return;
                }
                else if (!(old_state & go_to_sleep))
                    break;
            }
            // old_state == go_to_sleep
            syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, old_state, nullptr, nullptr, 0);
            if (try_lock_internal(old_state))
                goto wake_and_return;
        }
        for (;;)
        {
            std::this_thread::yield();
            if (try_lock_internal(old_state))
                break;
            _mm_pause();
            if (try_lock_internal(old_state))
                break;
        }
        wake_and_return:
        state.fetch_or(need_to_wake_after, std::memory_order_relaxed);
    }
    bool try_lock()
    {
        unsigned old_state;
        return try_lock_internal(old_state);
    }
    void unlock()
    {
        unsigned will_wake = state.load(std::memory_order_relaxed) & need_to_wake_after;
        state.store(unlocked, std::memory_order_release);
        if (will_wake)
            syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }

private:
    std::atomic<unsigned> state{0};
    static constexpr unsigned unlocked = 0;
    static constexpr unsigned locked = 1;
    static constexpr unsigned go_to_sleep = 2;
    static constexpr unsigned need_to_wake_after = 4;

    inline bool try_lock_internal(unsigned & old_state)
    {
        old_state = state.load(std::memory_order_acquire);
        // old_state == unlocked || old_state == locked || old_state == go_to_sleep
        return !(old_state & (locked | go_to_sleep)) && state.compare_exchange_strong(old_state, locked, std::memory_order_acquire);
    }
};
#else
struct spin_to_futex_mutex
{
    void lock()
    {
        unsigned old_state = unlocked;
        if (state.compare_exchange_strong(old_state, locked, std::memory_order_acquire))
            return;
        for (;;)
        {
            old_state = state.exchange(sleeper, std::memory_order_acquire);
            if (old_state == unlocked)
                return;
            else if (old_state == locked)
            {
                for (;;)
                {
                    _mm_pause();
                    if (state.load(std::memory_order_acquire) == unlocked)
                        break;
                    std::this_thread::yield();
                    if (state.load(std::memory_order_acquire) == unlocked)
                        break;
                }
            }
            else
                syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, sleeper, nullptr, nullptr, 0);
        }
    }
    void unlock()
    {
        unsigned old_state = state.load(std::memory_order_relaxed);
        state.store(unlocked, std::memory_order_release);
        if (old_state == sleeper)
            syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }

private:
    std::atomic<unsigned> state{unlocked};
    static constexpr unsigned unlocked = 0;
    static constexpr unsigned locked = 1;
    static constexpr unsigned sleeper = 2;
};
/*struct spin_to_futex_mutex_exchange
{
    void lock()
    {
        unsigned old_state = state.exchange(locked, std::memory_order_acquire);
        if (old_state == unlocked)
            return;
        unsigned my_write_state = old_state == locked ? spinner : sleeper;
        for (;;)
        {
            old_state = state.exchange(my_write_state, std::memory_order_acquire);
            if (old_state == unlocked)
                return;
            else if (my_write_state == spinner)
            {
                for (;;)
                {
                    _mm_pause();
                    if (state.load(std::memory_order_acquire) == unlocked)
                        break;
                    std::this_thread::yield();
                    if (state.load(std::memory_order_acquire) == unlocked)
                        break;
                }
            }
            else
            {
                syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, sleeper, nullptr, nullptr, 0);
            }
        }
    }
    void unlock()
    {
        unsigned old_state = state.load(std::memory_order_relaxed);
        state.store(unlocked, std::memory_order_release);
        if (old_state == sleeper)
            syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }

private:
    std::atomic<unsigned> state{unlocked};
    static constexpr unsigned unlocked = 0;
    static constexpr unsigned locked = 1;
    static constexpr unsigned spinner = 2;
    static constexpr unsigned sleeper = 3;
};*/
struct spin_to_futex_mutex_add
{
    void lock()
    {
        unsigned old_state = state.fetch_add(1, std::memory_order_acquire);
        if (old_state == unlocked)
            return;
        if (old_state == 1)
        {
            spin_loop:
            for (;;)
            {
                _mm_pause();
                if (state.load(std::memory_order_acquire) == unlocked)
                    break;
                std::this_thread::yield();
                if (state.load(std::memory_order_acquire) == unlocked)
                    break;
            }
        }
        for (;;)
        {
            old_state = state.exchange(sleeper, std::memory_order_acquire);
            if (old_state == unlocked)
                return;
            else if (old_state >= sleeper)
                syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, sleeper, nullptr, nullptr, 0);
            else
                goto spin_loop;
        }
    }
    void unlock()
    {
        unsigned old_state = state.load(std::memory_order_relaxed);
        state.store(unlocked, std::memory_order_release);
        if (old_state >= sleeper)
            syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }
private:
    std::atomic<unsigned> state{unlocked};
    static constexpr unsigned unlocked = 0;
    static constexpr unsigned sleeper = 2;
};
struct spin_to_futex_mutex_load
{
    void lock()
    {
        unsigned old_state = state.load(std::memory_order_acquire);
        switch(old_state)
        {
            for (;;)
            {
                [[fallthrough]];
        case locked:
                old_state = state.exchange(sleeper, std::memory_order_acquire);
                if (old_state == unlocked)
                    return;
                else if (old_state == locked)
                {
                    for (;;)
                    {
                        for (;;)
                        {
                            _mm_pause();
                            if (state.load(std::memory_order_acquire) == unlocked)
                                break;
                            std::this_thread::yield();
                            if (state.load(std::memory_order_acquire) == unlocked)
                                break;
                        }
                        break;
                        [[fallthrough]];
        case unlocked:
                        old_state = state.exchange(locked, std::memory_order_acquire);
                        if (old_state == unlocked)
                            return;
                        else if (old_state == sleeper)
                        {
                            old_state = state.exchange(sleeper, std::memory_order_acquire);
                            if (old_state == unlocked)
                                return;
                        }
                    }
                }
                else
                {
                    [[fallthrough]];
        case sleeper:
                    syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, sleeper, nullptr, nullptr, 0);
                }
            }
        }
    }
    void unlock()
    {
        unsigned old_state = state.load(std::memory_order_relaxed);
        state.store(unlocked, std::memory_order_release);
        if (old_state == sleeper)
            syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }

private:
    std::atomic<unsigned> state{unlocked};
    static constexpr unsigned unlocked = 0;
    static constexpr unsigned locked = 1;
    static constexpr unsigned sleeper = 2;
};

#endif

struct semaphore
{
    explicit semaphore(std::ptrdiff_t value = 0)
    {
        sem_init(&sem, false, value);
    }
    ~semaphore()
    {
        sem_destroy(&sem);
    }
    void release()
    {
        int result = sem_post(&sem);
        CHECK_FOR_PROGRAMMER_ERROR(!result);
    }
    void release(ptrdiff_t update)
    {
        for (; update > 0; --update)
        {
            release();
        }
    }

    void acquire()
    {
        while (sem_wait(&sem))
        {
            CHECK_FOR_PROGRAMMER_ERROR(errno == EINTR);
        }
    }

    bool try_acquire()
    {
        while(sem_trywait(&sem))
        {
            if (errno == EAGAIN)
                return false;
            else
                CHECK_FOR_PROGRAMMER_ERROR(errno == EINTR);
        }
        return true;
    }

private:
    sem_t sem;
};

struct semaphore_custom
{
private:
    std::mutex mutex;
    std::condition_variable condition;
    std::ptrdiff_t count = 0;

public:
    explicit semaphore_custom(ptrdiff_t desired = 0)
        : count(desired)
    {
    }

    void release()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            ++count;
        }
        condition.notify_one();
    }
    void release(ptrdiff_t update)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            count += update;
        }
        condition.notify_all();
    }

    void acquire()
    {
        std::unique_lock<std::mutex> lock(mutex);
        while(!count)
            condition.wait(lock);
        --count;
    }

    bool try_acquire()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if(!count)
            return false;
        --count;
        return true;
    }
};

struct semaphore_mutex
{
    void lock()
    {
        sema.acquire();
    }
    void unlock()
    {
        sema.release();
    }
private:
    semaphore sema{1};
};

struct terrible_spinlock
{
    void lock()
    {
        while (locked.test_and_set(std::memory_order_acquire))
        {
        }
    }
    void unlock()
    {
        locked.clear(std::memory_order_release);
    }

private:
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
};
struct spinlock_test_and_set
{
    void lock()
    {
        for (;;)
        {
            if (try_lock())
                break;
            _mm_pause();
            if (try_lock())
                break;
            std::this_thread::yield();
        }
    }
    bool try_lock()
    {
        return !locked.test_and_set(std::memory_order_acquire);
    }
    void unlock()
    {
        locked.clear(std::memory_order_release);
    }

private:
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
};
struct spinlock_test_and_set_once
{
    void lock()
    {
        for (;;)
        {
            if (!locked.exchange(true, std::memory_order_acquire))
                break;
            for (;;)
            {
                _mm_pause();
                if (!locked.load(std::memory_order_acquire))
                    break;
                std::this_thread::yield();
                if (!locked.load(std::memory_order_acquire))
                    break;
            }
        }
    }
    bool try_lock()
    {
        return !locked.load(std::memory_order_acquire) && !locked.exchange(true, std::memory_order_acquire);
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{false};
};
struct spinlock_compare_exchange
{
    void lock()
    {
        for (;;)
        {
            bool unlocked = false;
            if (locked.compare_exchange_weak(unlocked, true, std::memory_order_acquire))
                break;
            for (;;)
            {
                _mm_pause();
                if (!locked.load(std::memory_order_acquire))
                    break;
                std::this_thread::yield();
                if (!locked.load(std::memory_order_acquire))
                    break;
            }
        }
    }
    bool try_lock()
    {
        return !locked.load(std::memory_order_acquire) && !locked.exchange(true, std::memory_order_acquire);
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{false};
};
struct spinlock_compare_exchange_only
{
    void lock()
    {
        for (;;)
        {
            if (try_lock())
                break;
            _mm_pause();
            if (try_lock())
                break;
            std::this_thread::yield();
        }
    }
    bool try_lock()
    {
        bool unlocked = false;
        return locked.compare_exchange_weak(unlocked, true, std::memory_order_acquire);
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{false};
};

struct spinlock_amd
{
    void lock()
    {
        for (;;)
        {
            bool was_locked = locked.load(std::memory_order_relaxed);
            if (!was_locked && locked.compare_exchange_weak(was_locked, true, std::memory_order_acquire))
                break;
            _mm_pause();
        }
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{false};
};

struct spinlock
{
    void lock()
    {
        for (int spin_count = 0; !try_lock(); ++spin_count)
        {
            if (spin_count < 16)
                _mm_pause();
            else
            {
                std::this_thread::yield();
                spin_count = 0;
            }
        }
    }
    bool try_lock()
    {
        return !locked.load(std::memory_order_relaxed) && !locked.exchange(true, std::memory_order_acquire);
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{false};
};

struct k42_spinlock
{
    void lock()
    {
        linked_list me;
        linked_list * pred = state.tail.exchange(&me, std::memory_order_acquire);
        if (pred)
        {
            me.tail.store(&me, std::memory_order_relaxed); // any non-zero value is fine here
            // insert myself into linked list
            pred->next.store(&me, std::memory_order_release);
            // waiting for the write at the end of unlock()
            for (;;)
            {
                if (!me.tail.load(std::memory_order_acquire))
                    break;
                _mm_pause();
                if (!me.tail.load(std::memory_order_acquire))
                    break;
                std::this_thread::yield();
            }
        }
        // at this point I am next in line to acquire the lock. need to store
        // something in state.tail to indicate that this is locked

        // load the value that another thread might have written in
        // the line "insert myself into linked list" above
        linked_list * succ = me.next.load(std::memory_order_acquire);
        if (succ)
        {
            // there is another item in the list. don't need to store
            // anything in state.tail, because that other item will already
            // have stored something
            state.next.store(succ, std::memory_order_relaxed);
        }
        else
        {
            state.next.store(nullptr, std::memory_order_relaxed);
            // check if my value is still in the tail. if yes, write
            // the state variable in there. so other threads will overwrite state.next
            // in the line "insert myself into linked list" above
            linked_list * to_compare = &me;
            if (state.tail.compare_exchange_strong(to_compare, &state, std::memory_order_acq_rel))
                return;

            // somebody tried entering at the same time. wait until the other
            // thread is done with the line "insert myself into linked list"
            // then exit. no need to update state.tail any more because the
            // other thread already wrote a value to it.
            for (;;)
            {
                linked_list * next = me.next.load(std::memory_order_acquire);
                if (next)
                {
                    state.next.store(next, std::memory_order_relaxed);
                    break;
                }
                _mm_pause();
            }
        }
    }
    void unlock()
    {
        // if I was the only one in the list and a second thread entered,
        // state.tail will have pointed at state.next. in that case another
        // thread executing the line "insert myself into linked list" would
        // have updated this variable
        linked_list * succ = state.next.load(std::memory_order_acquire);
        if (!succ)
        {
            // nobody else called lock(). just check if it was the old value and replace
            linked_list * old_value = &state;
            if (state.tail.compare_exchange_strong(old_value, nullptr, std::memory_order_acq_rel))
                return;
            // somebody else started entering at exactly the wrong moment. it was
            // just about to unlock. just wait until they finish the line
            // "insert myself into linked list" then clear the tail pointer and leave
            for (;;)
            {
                succ = state.next.load(std::memory_order_acquire);
                if (succ)
                    break;
                _mm_pause();
            }
        }
        succ->tail.store(nullptr, std::memory_order_release);
    }

private:
    struct linked_list
    {
        std::atomic<linked_list *> next{nullptr};
        std::atomic<linked_list *> tail{nullptr};
    };
    linked_list state;
};

struct k42_mutex
{
    void lock()
    {
#if 0
        linked_list stack_state;
        linked_list * pred = state.tail.exchange(&stack_state, std::memory_order_acquire);
        if (pred)
        {
            stack_state.futex_value.store(1, std::memory_order_relaxed);
            // insert myself into linked list
            pred->next.store(&stack_state, std::memory_order_release);
            // waiting for the write at the end of unlock()
            do
            {
                syscall(SYS_futex, &stack_state.futex_value, FUTEX_WAIT_PRIVATE, 1, nullptr, nullptr, 0);
            } while (stack_state.futex_value.load(std::memory_order_acquire));
        }
        // at this point I am next in line to acquire the lock. need to store
        // something in state.tail to indicate that this is locked. can't
        // store the value that's stored right now, stack_state, because that value
        // will go out of scope

        // load the value that another thread might have written in
        // the line "insert myself into linked list" above
        linked_list * succ = stack_state.next.load(std::memory_order_acquire);
        if (succ)
        {
            // there is another item in the list. don't need to store
            // anything in state.tail, because that other item will already
            // have stored something. just transfer my value into the
            // shared state. just use relaxed atomics because the tail is
            // already updated, so nobody else will mess with this variable
            state.next.store(succ, std::memory_order_relaxed);
        }
        else
        {
            // nobody else entered so far. store a nullptr in the list.
            // in the unlock() case that nullptr will indicate whether the
            // other thread has finished the line "insert myself into linked
            // list" or not.
            state.next.store(nullptr, std::memory_order_relaxed);
            // check if my value is still in the tail. if yes, write
            // the state variable in there. so other threads will overwrite state.next
            // in the line "insert myself into linked list" above
            linked_list * to_compare = &stack_state;
            if (state.tail.compare_exchange_strong(to_compare, &state, std::memory_order_acq_rel))
                return;

            // somebody tried entering at the same time. wait until the other
            // thread is done with the line "insert myself into linked list"
            // then exit. no need to update state.tail any more because the
            // other thread already wrote a value to it.
            for (;;)
            {
                linked_list * next = stack_state.next.load(std::memory_order_acquire);
                if (next)
                {
                    state.next.store(next, std::memory_order_relaxed);
                    break;
                }
                _mm_pause();
            }
        }
#else
        linked_list stack_state;
        linked_list * pred = state.tail.exchange(&stack_state, std::memory_order_acquire);
        if (pred)
        {
            stack_state.futex_value.store(1, std::memory_order_relaxed);
            // insert myself into linked list
            pred->next.store(&stack_state, std::memory_order_release);
            // waiting for the write at the end of unlock()
            do
            {
                syscall(SYS_futex, &stack_state.futex_value, FUTEX_WAIT_PRIVATE, 1, nullptr, nullptr, 0);
            } while (stack_state.futex_value.load(std::memory_order_acquire));
        }
        // at this point I am next in line to acquire the lock. need to store
        // something in state.tail to indicate that this is locked. can't
        // store the value that's stored right now, stack_state, because that value
        // will go out of scope

        // load the value that another thread might have written in
        // the line "insert myself into linked list" above
        linked_list * succ = stack_state.next.load(std::memory_order_acquire);
        state.next.store(succ, std::memory_order_relaxed);
        if (succ)
        {
            // there is another item in the list. don't need to store
            // anything in state.tail, because that other item will already
            // have stored something. just transfer my value into the
            // shared state. just use relaxed atomics because the tail is
            // already updated, so nobody else will mess with this variable
            return;
        }
        // in the else case we just wrote a nullptr to state.next. it's important
        // that that write finishes before the next compare_exchange_strong.
        // before that compare_exchange nobody can modify the state.next pointer
        // because the tail pointer points at our stack. (and the next pointer
        // only gets modified through the tail pointer) the moment that the tail
        // no longer points at the stack, others might modify state.next. so we
        // have to write a nullptr before we do that.

        // check if my value is still in the tail. if yes, write
        // the state variable in there. so other threads will overwrite state.next
        // in the line "insert myself into linked list" above
        linked_list * to_compare = &stack_state;
        if (state.tail.compare_exchange_strong(to_compare, &state, std::memory_order_acq_rel))
            return;

        // somebody tried entering at the same time. wait until the other
        // thread is done with the line "insert myself into linked list"
        // then exit. no need to update state.tail any more because the
        // other thread already wrote a value to it.
        for (;;)
        {
            linked_list * next = stack_state.next.load(std::memory_order_acquire);
            if (next)
            {
                state.next.store(next, std::memory_order_relaxed);
                break;
            }
            _mm_pause();
        }
#endif
    }
    void unlock()
    {
#if 0
        // if I was the only one in the list and a second thread entered,
        // state.tail will have pointed at state.next. in that case the other
        // thread executing the line "insert myself into linked list" would
        // have updated this variable
        linked_list * succ = state.next.load(std::memory_order_acquire);
        if (!succ)
        {
            // nobody else called lock(). just check if it was the old value and replace
            linked_list * old_value = &state;
            if (state.tail.compare_exchange_strong(old_value, nullptr, std::memory_order_acq_rel))
                return;
            // somebody else started entering at exactly the wrong moment. it was
            // just about to unlock. just wait until they finish the line
            // "insert myself into linked list" then clear the tail pointer and leave
            for (;;)
            {
                succ = state.next.load(std::memory_order_acquire);
                if (succ)
                    break;
                _mm_pause();
            }
        }
        succ->futex_value.store(0, std::memory_order_release);
        syscall(SYS_futex, &succ->futex_value, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
#else
        // check if somebody else called lock(). if the tail is still pointing inside
        // the mutex, then I'm all clear
        linked_list * old_value = &state;
        if (state.tail.compare_exchange_strong(old_value, nullptr, std::memory_order_acq_rel))
            return;
        // somebody else has entered. make sure they finished the line
        // "insert myself into linked list" then clear the tail pointer and leave
        for (;;)
        {
            linked_list * succ = state.next.load(std::memory_order_acquire);
            if (succ)
            {
                succ->futex_value.store(0, std::memory_order_release);
                syscall(SYS_futex, &succ->futex_value, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
                break;
            }
            _mm_pause();
        }
#endif
    }

private:
    struct linked_list
    {
        std::atomic<linked_list *> next{nullptr};
        union
        {
            // tail is only used in the main state
            std::atomic<linked_list *> tail;
            // the futex value is used in all other cases
            std::atomic<uint32_t> futex_value;
        };
        linked_list()
            : tail(nullptr)
        {
        }
    };
    linked_list state;
};

template<int SpinNoOpCount, int SpinPauseCount, bool ResetCountAfterYield>
struct parameterized_spinlock
{
    void lock()
    {
        int spin_count = 0;
        auto on_spin = [&]
        {
            if constexpr (SpinNoOpCount > 0)
            {
                if (SpinNoOpCount == std::numeric_limits<int>::max() || spin_count < SpinNoOpCount)
                    return;
            }
            if constexpr (SpinNoOpCount != std::numeric_limits<int>::max() && SpinPauseCount > 0)
            {
                if (SpinPauseCount == std::numeric_limits<int>::max() || spin_count < SpinPauseCount)
                {
                    _mm_pause();
                    return;
                }
            }
            if constexpr (SpinNoOpCount != std::numeric_limits<int>::max() && SpinPauseCount != std::numeric_limits<int>::max())
            {
                std::this_thread::yield();
                if constexpr (ResetCountAfterYield)
                    spin_count = 0;
            }
        };
        for (;;)
        {
            bool expected = false;
            if (locked.compare_exchange_weak(expected, true, std::memory_order_acquire))
                return;
            for (;; ++spin_count)
            {
                on_spin();
                if (!locked.load(std::memory_order_acquire))
                    break;
            }
        }
    }
    bool try_lock()
    {
        bool expected = false;
        return locked.compare_exchange_strong(expected, true, std::memory_order_acquire);
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{false};
};

void benchmark_yield(benchmark::State & state)
{
    while (state.KeepRunning())
    {
        std::this_thread::yield();
    }
}
BENCHMARK(benchmark_yield);

template<typename T>
void benchmark_mutex_lock_unlock(benchmark::State& state)
{
    T m;
    while (state.KeepRunning())
    {
        m.lock();
        m.unlock();
    }
}


#define RegisterBenchmarkWithAllMutexes(benchmark, ...)\
    BENCHMARK_TEMPLATE(benchmark, std::mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, std::shared_mutex) __VA_ARGS__;\
    /*BENCHMARK_TEMPLATE(benchmark, std::recursive_mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, boost::mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, boost::shared_mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, boost::recursive_mutex) __VA_ARGS__;*/\
    BENCHMARK_TEMPLATE(benchmark, pthread_mutex) __VA_ARGS__;\
    /*BENCHMARK_TEMPLATE(benchmark, pthread_mutex_recursive) __VA_ARGS__;*/\
    BENCHMARK_TEMPLATE(benchmark, pthread_mutex_adaptive) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, pthread_spinlock) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, futex_mutex_careful_wake) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, futex_mutex) __VA_ARGS__;\
    /*BENCHMARK_TEMPLATE(benchmark, two_byte_futex_mutex) __VA_ARGS__;*/\
    BENCHMARK_TEMPLATE(benchmark, futex_mutex_tricky) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spin_to_futex_mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, ticket_spinlock) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, ticket_mutex) __VA_ARGS__;\
    /*BENCHMARK_TEMPLATE(benchmark, semaphore_mutex) __VA_ARGS__;*/\
    /*BENCHMARK_TEMPLATE(benchmark, k42_mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, k42_spinlock) __VA_ARGS__;*/\
    BENCHMARK_TEMPLATE(benchmark, spinlock) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spinlock_amd) __VA_ARGS__;\
    /*BENCHMARK_TEMPLATE(benchmark, spinlock_test_and_set) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spinlock_test_and_set_once) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spinlock_compare_exchange) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spinlock_compare_exchange_only) __VA_ARGS__;*/\
    BENCHMARK_TEMPLATE(benchmark, terrible_spinlock) __VA_ARGS__;\
    /*BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 0, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 1, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 4, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 16, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 64, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 256, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 1, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 4, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 16, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 64, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 256, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 4, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 16, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 64, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 256, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<16, 16, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<16, 64, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<16, 256, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<64, 64, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<64, 256, false>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 0, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 1, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 4, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 16, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 64, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 256, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 1, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 4, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 16, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 64, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 256, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 4, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 16, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 64, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 256, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<16, 16, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<16, 64, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<16, 256, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<64, 64, true>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<64, 256, true>) __VA_ARGS__;*/\

RegisterBenchmarkWithAllMutexes(benchmark_mutex_lock_unlock);

void benchmark_shared_mutex_lock_shared(benchmark::State& state)
{
    std::shared_mutex m;
    while (state.KeepRunning())
    {
        m.lock_shared();
        m.unlock_shared();
    }
}
BENCHMARK(benchmark_shared_mutex_lock_shared);

void benchmark_semaphore_release_and_acquire(benchmark::State & state)
{
    semaphore m;
    while (state.KeepRunning())
    {
        m.release();
        m.acquire();
    }
}
BENCHMARK(benchmark_semaphore_release_and_acquire);

void benchmark_custom_semaphore_release_and_acquire(benchmark::State & state)
{
    semaphore_custom m;
    while (state.KeepRunning())
    {
        m.release();
        m.acquire();
    }
}
BENCHMARK(benchmark_custom_semaphore_release_and_acquire);

struct RealtimeThread
{
    RealtimeThread()
    {
    }
    RealtimeThread(std::function<void ()> func)
        : thread_func(std::move(func))
    {
        pthread_attr_t attributes;
        int result = pthread_attr_init(&attributes);
        CHECK_FOR_PROGRAMMER_ERROR(result == 0);
        result = pthread_attr_setschedpolicy(&attributes, SCHED_RR);
        CHECK_FOR_PROGRAMMER_ERROR(result == 0);
        result = pthread_attr_setinheritsched(&attributes, PTHREAD_EXPLICIT_SCHED);
        CHECK_FOR_PROGRAMMER_ERROR(result == 0);
        sched_param param;
        param.sched_priority = 5;
        result = pthread_attr_setschedparam(&attributes, &param);
        CHECK_FOR_PROGRAMMER_ERROR(result == 0);
        result = pthread_create(&thread, &attributes, &thread_start, this);
        CHECK_FOR_PROGRAMMER_ERROR(result == 0);
        result = pthread_attr_destroy(&attributes);
        CHECK_FOR_PROGRAMMER_ERROR(result == 0);
    }
    void join()
    {
        pthread_join(thread, nullptr);
    }

private:
    std::function<void ()> thread_func;
    pthread_t thread = 0;

    static void * thread_start(void * self)
    {
        static_cast<RealtimeThread *>(self)->thread_func();
        return nullptr;
    }
};

template<typename State>
struct ThreadedBenchmarkRunner
{
    ThreadedBenchmarkRunner(State * state, int num_threads)
        : state(state)
    {
        threads.reserve(num_threads);
        for (int i = 0; i < num_threads; ++i)
        {
            threads.emplace_back([this, state, /*num_threads, */i]
            {
                /*if (static_cast<unsigned>(num_threads) == std::thread::hardware_concurrency())
                {
                    cpu_set_t cpuset;
                    CPU_ZERO(&cpuset);
                    CPU_SET(i, &cpuset);
                    pthread_setaffinity_np(threads[i].native_handle(), sizeof(cpu_set_t), &cpuset);
                }*/
                /*cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(5, &cpuset);
                pthread_setaffinity_np(threads[i].native_handle(), sizeof(cpu_set_t), &cpuset);*/
                for (;;)
                {
                    finished.release();
                    all_started_sema.acquire();
                    if (ended)
                        break;
                    state->run_benchmark(i);
                }
            });
        }
        for (size_t i = threads.size(); i > 0; --i)
            finished.acquire();
    }

    void wait_until_started()
    {
        state->start_run();
    }
    void release()
    {
        all_started_sema.release(threads.size());
    }
    void wait_until_finished()
    {
        for (size_t i = threads.size(); i > 0; --i)
            finished.acquire();
        state->end_run();
    }

    void full_run()
    {
        wait_until_started();
        release();
        wait_until_finished();
    }

    void shut_down()
    {
        ended = true;
        all_started_sema.release(threads.size());
        for (auto & thread : threads)
            thread.join();
    }

private:
    State * state;
    std::vector<std::thread> threads;
    //std::vector<RealtimeThread> threads;
    semaphore all_started_sema;
    semaphore finished;
    bool ended = false;
};

template<typename State>
struct ThreadedBenchmarkRunnerMultipleTimes
{
    template<typename... StateArgs>
    ThreadedBenchmarkRunnerMultipleTimes(int num_runners, int num_threads, StateArgs &&... state_args)
    {
        for (int i = 0; i < num_runners; ++i)
        {
            runners.emplace_back(num_threads, state_args...);
        }
    }

    void wait_until_started()
    {
        for (OneRunnerAndState & runner : runners)
            runner.runner.wait_until_started();
    }

    void start_run()
    {
        for (OneRunnerAndState & runner : runners)
            runner.runner.release();
    }

    void finish_run()
    {
        for (OneRunnerAndState & runner : runners)
            runner.runner.wait_until_finished();
    }

    void full_run()
    {
        wait_until_started();
        start_run();
        finish_run();
    }

    void shut_down()
    {
        for (OneRunnerAndState & runner : runners)
            runner.runner.shut_down();
    }

    struct OneRunnerAndState
    {
        State state;
        ThreadedBenchmarkRunner<State> runner;

        template<typename... StateArgs>
        OneRunnerAndState(int num_threads, StateArgs &&... state_args)
            : state(num_threads, std::forward<StateArgs>(state_args)...), runner(&state, num_threads)
        {
        }
    };
    std::list<OneRunnerAndState> runners;
};

template<typename T>
struct ContendedMutexRunner
{
    size_t num_loops = 1;
    size_t sum = 0;
    size_t expected_sum;
    T mutex;

    explicit ContendedMutexRunner(int num_threads, size_t num_loops)
        : num_loops(num_loops)
        , expected_sum(num_threads * num_loops)
    {
    }

    void start_run()
    {
        sum = 0;
    }
    void end_run()
    {
        CHECK_FOR_PROGRAMMER_ERROR(sum == expected_sum);
    }

    void run_benchmark(int)
    {
        for (size_t i = num_loops; i != 0; --i)
        {
            mutex.lock();
            ++sum;
            mutex.unlock();
        }
    }
};
template<typename T>
struct ContendedMutexRunnerSized
{
    size_t num_loops = 1;
    std::vector<size_t> sums;
    size_t expected_sum;
    T mutex;

    explicit ContendedMutexRunnerSized(int num_threads, size_t num_loops, size_t num_sums)
        : num_loops(num_loops)
        , sums(num_sums)
        , expected_sum(num_threads * num_loops)
    {
    }

    void start_run()
    {
        for (size_t & sum : sums)
            sum = 0;
    }
    void end_run()
    {
        CHECK_FOR_PROGRAMMER_ERROR(sums.front() == expected_sum);
    }

    void run_benchmark(int)
    {
        for (size_t i = num_loops; i != 0; --i)
        {
            mutex.lock();
            for (size_t & sum : sums)
                ++sum;
            mutex.unlock();
        }
    }
};

template<typename Randomness>
uint64_t UniformRandom(uint64_t max, Randomness & random)
{
    static_assert(Randomness::min() == 0 && Randomness::max() == std::numeric_limits<uint64_t>::max());
    uint64_t random_value = random();
    if (max == std::numeric_limits<uint64_t>::max())
        return random_value;
    ++max;
    __uint128_t to_range = static_cast<__uint128_t>(random_value) * static_cast<__uint128_t>(max);
    uint64_t lower_bits = static_cast<uint64_t>(to_range);
    if (lower_bits < max)
    {
        uint64_t threshold = -max % max;
        while (lower_bits < threshold)
        {
            random_value = random();
            to_range = static_cast<__uint128_t>(random_value) * static_cast<__uint128_t>(max);
            lower_bits = static_cast<uint64_t>(to_range);
        }
    }
    return static_cast<uint64_t>(to_range >> 64);
    //return std::uniform_int_distribution<size_t>(0, max)(random);
}

template<typename T>
struct ManyContendedMutexRunner
{
    struct OneMutexAndState
    {
        explicit OneMutexAndState(size_t num_sums)
            : sums(num_sums)
        {
        }
        std::vector<size_t> sums;
        T mutex;
    };

    std::deque<OneMutexAndState> state;
    size_t expected_sum;
    struct ThreadState
    {
        template<typename Seed>
        ThreadState(Seed & random_seed, size_t num_loops, std::deque<OneMutexAndState> & state)
            : randomness(random_seed)
        {
            CHECK_FOR_PROGRAMMER_ERROR(num_loops % state.size() == 0);
            iteration_order.reserve(num_loops);
            for (size_t i = 0; i < num_loops; ++i)
            {
                iteration_order.push_back(&state[i % state.size()]);
            }
        }

        std::mt19937_64 randomness;
        std::vector<OneMutexAndState *> iteration_order;
    };

    std::vector<ThreadState> thread_state;

    explicit ManyContendedMutexRunner(int num_threads, size_t num_loops, size_t num_sums)
        : expected_sum(num_loops)
    {
        for (int i = 0; i < num_threads; ++i)
        {
            state.emplace_back(num_sums);
        }
        random_seed_seq seed;
        thread_state.reserve(num_threads);
        for (int i = 0; i < num_threads; ++i)
        {
            thread_state.emplace_back(seed, num_loops, state);
        }
    }

    void start_run()
    {
        for (OneMutexAndState & state : state)
        {
            for (size_t & sum : state.sums)
                sum = 0;
        }
    }
    void end_run()
    {
        CHECK_FOR_PROGRAMMER_ERROR(state.front().sums.front() == expected_sum);
    }

    void run_benchmark(int thread_num)
    {
        ThreadState & state = thread_state[thread_num];
        std::mt19937_64 & my_thread_randomness = state.randomness;
        size_t num_states = state.iteration_order.size();
        for (auto it = state.iteration_order.begin(), end = state.iteration_order.end(); it != end; ++it)
        {
            --num_states;
            if (num_states)
            {
                size_t random_pick = UniformRandom(num_states, my_thread_randomness);
                std::iter_swap(it, it + random_pick);
            }
            OneMutexAndState & one = **it;
            one.mutex.lock();
            for (size_t & sum : one.sums)
                ++sum;
            one.mutex.unlock();
        }
    }
};

template<typename T>
void BenchmarkContendedMutex(benchmark::State & state)
{
    static constexpr size_t num_loops = 1024 * 16;

    ThreadedBenchmarkRunnerMultipleTimes<ContendedMutexRunner<T>> runner(state.range(0), state.range(1), num_loops);
    while (state.KeepRunning())
    {
        runner.full_run();
    }
    runner.shut_down();
}

template<typename T>
struct ContendedMutexRunnerMoreIdle
{
    size_t sum = 0;
    size_t num_loops = 1;
    size_t expected_sum = 0;
    T mutex;

    explicit ContendedMutexRunnerMoreIdle(int num_threads, size_t num_loops)
        : num_loops(num_loops), expected_sum(num_threads * num_loops)
    {
    }

    void start_run()
    {
        sum = 0;
    }
    void end_run()
    {
        CHECK_FOR_PROGRAMMER_ERROR(sum == expected_sum);
    }

    void run_benchmark(int)
    {
        for (size_t i = num_loops; i != 0; --i)
        {
            mutex.lock();
            ++sum;
            mutex.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            /*if (iteration < 1024)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(1024 - iteration) / 32);
            }*/
        }
    }
};

template<typename T>
void BenchmarkContendedMutexMoreIdle(benchmark::State & state)
{
    static constexpr size_t num_loops = 1024;

    ThreadedBenchmarkRunnerMultipleTimes<ContendedMutexRunnerMoreIdle<T>> runner(state.range(0), state.range(1), num_loops);
    while (state.KeepRunning())
    {
        runner.full_run();
    }
    runner.shut_down();
}

template<typename T, size_t N>
struct TopNHeap
{
    void fill(const T & value)
    {
        heap.fill(value);
    }

    void add(const T & value)
    {
        if (value > heap.front())
        {
            std::pop_heap(heap.begin(), heap.end(), std::greater<>());
            heap.back() = value;
            std::push_heap(heap.begin(), heap.end(), std::greater<>());
        }
    }
    void add(T && value)
    {
        if (value > heap.front())
        {
            std::pop_heap(heap.begin(), heap.end(), std::greater<>());
            heap.back() = std::move(value);
            std::push_heap(heap.begin(), heap.end(), std::greater<>());
        }
    }

    void sort()
    {
        std::sort_heap(heap.begin(), heap.end(), std::greater<>());
    }

    template<size_t ON>
    void merge(const TopNHeap<T, ON> & to_merge)
    {
        for (const T & val : to_merge.heap)
            add(val);
    }

    std::array<T, N> heap;
};


template<typename T>
struct LongestWaitRunner
{
    TopNHeap<size_t, 4> longest_waits;
    T mutex;
    size_t current_longest_wait = 0;
    size_t num_loops = 1;

    explicit LongestWaitRunner(int, size_t num_loops)
        : num_loops(num_loops)
    {
        longest_waits.fill(0);
    }

    void start_run()
    {
        current_longest_wait = 0;
    }
    void end_run()
    {
        longest_waits.add(current_longest_wait);
    }

    void run_benchmark(int)
    {
        for (size_t i = num_loops; i != 0; --i)
        {
            auto time_before = std::chrono::high_resolution_clock::now();
            mutex.lock();
            size_t wait_time_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time_before).count();
            current_longest_wait = std::max(wait_time_nanos, current_longest_wait);
            mutex.unlock();
        }
    }
};

template<typename T>
void BenchmarkLongestWait(benchmark::State & state)
{
    static constexpr size_t num_loops = 1024 * 16;
    ThreadedBenchmarkRunnerMultipleTimes<LongestWaitRunner<T>> runner(state.range(0), state.range(1), num_loops);
    while (state.KeepRunning())
    {
        runner.full_run();
    }
    runner.shut_down();
    TopNHeap<size_t, 4> longest_waits_merged;
    longest_waits_merged.fill(0);
    for (auto & runner : runner.runners)
    {
        longest_waits_merged.merge(runner.state.longest_waits);
    }
    longest_waits_merged.sort();
    for (size_t i = 0; i < longest_waits_merged.heap.size(); ++i)
    {
        if (longest_waits_merged.heap[i] != 0)
            state.counters["Wait" + std::to_string(i)] = static_cast<double>(longest_waits_merged.heap[i]);
    }
}

template<typename T>
struct LongestIdleRunner
{
    TopNHeap<size_t, 4> longest_idles;
    T mutex;
    size_t current_longest_idle = 0;
    size_t num_loops = 1;
    bool first = true;
    std::chrono::high_resolution_clock::time_point time_before = std::chrono::high_resolution_clock::now();

    explicit LongestIdleRunner(int, size_t num_loops)
        : num_loops(num_loops)
    {
        longest_idles.fill(0);
    }

    void start_run()
    {
        current_longest_idle = 0;
        first = true;
    }
    void end_run()
    {
        longest_idles.add(current_longest_idle);
    }

    void run_benchmark(int)
    {
        for (size_t i = num_loops; i != 0; --i)
        {
            mutex.lock();
            size_t wait_time_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time_before).count();
            if (first)
                first = false;
            else if(wait_time_nanos > current_longest_idle)
                current_longest_idle = wait_time_nanos;
            time_before = std::chrono::high_resolution_clock::now();
            mutex.unlock();
            //std::this_thread::yield();
        }
    }
};

template<typename T>
void BenchmarkLongestIdle(benchmark::State & state)
{
    static constexpr size_t num_loops = 1024 * 16;
    ThreadedBenchmarkRunnerMultipleTimes<LongestIdleRunner<T>> runner(state.range(0), state.range(1), num_loops);
    while (state.KeepRunning())
    {
        runner.full_run();
    }
    runner.shut_down();
    TopNHeap<size_t, 4> longest_idles_merged;
    longest_idles_merged.fill(0);
    for (auto & runner : runner.runners)
    {
        longest_idles_merged.merge(runner.state.longest_idles);
    }
    longest_idles_merged.sort();
    for (size_t i = 0; i < longest_idles_merged.heap.size(); ++i)
    {
        if (longest_idles_merged.heap[i] != 0)
            state.counters["Idle" + std::to_string(i)] = static_cast<double>(longest_idles_merged.heap[i]);
    }
}

static void CustomBenchmarkArguments(benchmark::internal::Benchmark * b)
{
    int hardware_concurrency = std::thread::hardware_concurrency();
    int max_num_threads = hardware_concurrency * 2;
    for (int i = 1; i <= max_num_threads; i *= 2)
    {
        b->Args({ 1, i });
    }
    for (int i = 2; i <= max_num_threads; i *= 2)
    {
        b->Args({ i, i });
    }
    for (int i = 2; i <= max_num_threads; i *= 2)
    {
        int num_threads = std::max(1, hardware_concurrency / i);
        if (num_threads != i)
        {
            b->Args({i, num_threads});
        }
    }
}


template<typename T>
struct ContendedMutexMoreWork
{
    static constexpr size_t NUM_LISTS = 8;
    std::list<size_t> linked_lists[NUM_LISTS];
    T mutex[NUM_LISTS];
    size_t num_loops = 1;

    explicit ContendedMutexMoreWork(int num_threads, size_t num_loops)
        : num_loops(num_loops)
    {
        for (std::list<size_t> & l : linked_lists)
        {
            for (int i = 0; i < num_threads; ++i)
                l.push_back(i);
        }
    }

    void start_run()
    {
    }
    void end_run()
    {
    }

    void run_benchmark(int)
    {
        for (int i = num_loops; i != 0; --i)
        {
            size_t index = i % NUM_LISTS;
            mutex[index].lock();
            linked_lists[index].push_back(i);
            linked_lists[index].pop_front();
            mutex[index].unlock();
        }
    }
};

template<typename T>
void BenchmarkContendedMutexMoreWork(benchmark::State & state)
{
    static constexpr size_t num_loops = 1024;
    ThreadedBenchmarkRunnerMultipleTimes<ContendedMutexMoreWork<T>> runner(state.range(0), state.range(1), num_loops);
    while (state.KeepRunning())
    {
        runner.full_run();
    }
    runner.shut_down();
}

template<typename T>
struct ThroughputRunner
{
    std::chrono::high_resolution_clock::time_point * done_time;
    size_t sum = 0;
    T mutex;

    explicit ThroughputRunner(int, std::chrono::high_resolution_clock::time_point * done_time)
        : done_time(done_time)
    {
    }

    void start_run()
    {
        sum = 0;
    }
    void end_run()
    {
    }

    void run_benchmark(int)
    {
        std::chrono::high_resolution_clock::time_point until = *done_time;
        while (std::chrono::high_resolution_clock::now() < until)
        {
            mutex.lock();
            ++sum;
            mutex.unlock();
        }
    }
};

template<typename T>
void BenchmarkThroughput(benchmark::State & state)
{
    std::chrono::high_resolution_clock::time_point end_time = std::chrono::high_resolution_clock::now();
    int num_states = state.range(0);
    ThreadedBenchmarkRunnerMultipleTimes<ThroughputRunner<T>> runner(num_states, state.range(1), &end_time);
    runner.wait_until_started();
    end_time = std::chrono::high_resolution_clock::now() + std::chrono::seconds(1);
    runner.start_run();
    runner.finish_run();
    size_t sum = std::accumulate(runner.runners.begin(), runner.runners.end(), size_t(0), [](size_t l, const auto & r)
    {
        return l + r.state.sum;
    });
    runner.shut_down();
    state.counters["Throughput"] = sum;
}

template<typename T>
struct ThroughputRunnerMultipleMutex
{
    std::chrono::high_resolution_clock::time_point * done_time;
    struct alignas(64) MutexAndSum
    {
        size_t sum = 0;
        T mutex;
    };
    static constexpr size_t num_mutexes = 8;
    MutexAndSum sums[num_mutexes];

    explicit ThroughputRunnerMultipleMutex(int, std::chrono::high_resolution_clock::time_point * done_time)
        : done_time(done_time)
    {
    }

    void start_run()
    {
        for (MutexAndSum & sum : sums)
            sum.sum = 0;
    }
    void end_run()
    {
    }

    void run_benchmark(int)
    {
        std::chrono::high_resolution_clock::time_point until = *done_time;
        std::mt19937_64 randomness{std::random_device()()};
        std::uniform_int_distribution<int> distribution(0, num_mutexes - 1);
        while (std::chrono::high_resolution_clock::now() < until)
        {
            MutexAndSum & to_increment = sums[distribution(randomness)];
            to_increment.mutex.lock();
            ++to_increment.sum;
            to_increment.mutex.unlock();
        }
    }
};

template<typename T>
void BenchmarkThroughputMultipleMutex(benchmark::State & state)
{
    std::chrono::high_resolution_clock::time_point end_time = std::chrono::high_resolution_clock::now();
    int num_states = state.range(0);
    ThreadedBenchmarkRunnerMultipleTimes<ThroughputRunnerMultipleMutex<T>> runner(num_states, state.range(1), &end_time);
    runner.wait_until_started();
    end_time = std::chrono::high_resolution_clock::now() + std::chrono::seconds(1);
    runner.start_run();
    runner.finish_run();
    size_t sum = std::accumulate(runner.runners.begin(), runner.runners.end(), size_t(0), [](size_t l, const auto & r)
    {
        return l + std::accumulate(std::begin(r.state.sums), std::end(r.state.sums), size_t(0), [](size_t l, const auto & r)
        {
            return l + r.sum;
        });
    });
    runner.shut_down();
    state.counters["Throughput"] = sum;
}

template<typename T>
void BenchmarkDemingWS(benchmark::State & state)
{
    // code from http://demin.ws/blog/english/2012/05/05/atomic-spinlock-mutex/
    T mutex;
    int value = 0;
    semaphore sem;
    semaphore one_loop_done;
    bool done = false;
    auto loop = [&](bool inc, int limit)
    {
        for (int i = 0; i < limit; ++i)
        {
            mutex.lock();
            if (inc)
                ++value;
            else
                --value;
            mutex.unlock();
        }
    };
    auto background_thread = [&](bool inc, int limit)
    {
        for (;;)
        {
            sem.acquire();
            if (done)
                break;
            loop(inc, limit);
            one_loop_done.release();
        }
    };
    int num_increment = 20000000;
    int num_decrement = 10000000;
    std::thread t(background_thread, true, num_increment);
    while (state.KeepRunning())
    {
        value = 0;
        sem.release();
        loop(false, num_decrement);
        one_loop_done.acquire();
    }
    CHECK_FOR_PROGRAMMER_ERROR(value == num_increment - num_decrement);
    done = true;
    sem.release();
    t.join();
}

/*#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>*/


// code from https://github.com/goldshtn/shmemq-blog
template<typename Mutex>
struct alignas(64) shmemq
{
    shmemq(unsigned long max_count, unsigned int element_size)
        : element_size(element_size)
        , max_size(max_count * element_size)
    {
        data.reset(new char[max_size]);
    }

    bool try_enqueue(void * element, size_t len)
    {
      if (len != element_size)
        return false;

      std::lock_guard<Mutex> lock(mutex);

      if (write_index - read_index == max_size)
        return false; // There is no more room in the queue

      memcpy(data.get() + write_index % max_size, element, len);
      write_index += element_size;
      return true;
    }

    bool try_dequeue(void * element, size_t len)
    {
        if (len != element_size)
            return false;

        std::lock_guard<Mutex> lock(mutex);

        if (read_index == write_index)
            return false; // There are no elements that haven't been consumed yet

        memcpy(element, data.get() + read_index % max_size, len);
        read_index += element_size;
        return true;
    }

    Mutex mutex;
    size_t element_size = 0;
    size_t max_size = 0;
    size_t read_index = 0;
    size_t write_index = 0;
    std::unique_ptr<char[]> data;
};

void BenchmarkShmemqBaseline(skb::State & state)
{
    static constexpr int QUEUE_SIZE = 1000;
    static constexpr int REPETITIONS = 10000;
    int DATA_SIZE = state.range(0);

    shmemq<null_mutex> server_queue(QUEUE_SIZE, DATA_SIZE);
    shmemq<null_mutex> client_queue(QUEUE_SIZE, DATA_SIZE);

    auto build_message = [&]
    {
        std::unique_ptr<char[]> result(new char[DATA_SIZE]);
        memset(result.get(), 0, DATA_SIZE);
        int forty_two = 42;
        CHECK_FOR_PROGRAMMER_ERROR(sizeof(forty_two) <= static_cast<size_t>(DATA_SIZE));
        memcpy(result.get(), &forty_two, sizeof(forty_two));
        CHECK_FOR_PROGRAMMER_ERROR(5 + sizeof(forty_two) <= static_cast<size_t>(DATA_SIZE));
        memcpy(result.get() + sizeof(forty_two), "Hello", 5);
        return result;
    };

    while (state.KeepRunning())
    {
        auto msg = build_message();
        std::thread s([]{});
        std::thread c([]{});
        for (int i = 0; i < REPETITIONS; ++i)
        {
            while (!server_queue.try_enqueue(msg.get(), DATA_SIZE))
            {
            }
            while (!server_queue.try_dequeue(msg.get(), DATA_SIZE))
            {
            }
            while (!client_queue.try_enqueue(msg.get(), DATA_SIZE))
            {
            }
            while (!client_queue.try_dequeue(msg.get(), DATA_SIZE))
            {
            }
        }
        s.join();
        c.join();
    }
    state.SetItemsProcessed(REPETITIONS * state.iterations());
}
SKA_BENCHMARK("baseline", BenchmarkShmemqBaseline);

template<typename T, typename State>
void BenchmarkShmemq(State & state)
{
    static constexpr int QUEUE_SIZE = 1000;
    static constexpr int REPETITIONS = 10000;
    int DATA_SIZE = state.range(0);

    shmemq<T> server_queue(QUEUE_SIZE, DATA_SIZE);
    shmemq<T> client_queue(QUEUE_SIZE, DATA_SIZE);

    auto build_message = [&]
    {
        std::unique_ptr<char[]> result(new char[DATA_SIZE]);
        memset(result.get(), 0, DATA_SIZE);
        int forty_two = 42;
        CHECK_FOR_PROGRAMMER_ERROR(sizeof(forty_two) <= static_cast<size_t>(DATA_SIZE));
        memcpy(result.get(), &forty_two, sizeof(forty_two));
        CHECK_FOR_PROGRAMMER_ERROR(5 + sizeof(forty_two) <= static_cast<size_t>(DATA_SIZE));
        memcpy(result.get() + sizeof(forty_two), "Hello", 5);
        return result;
    };

    auto server = [&]
    {
        std::unique_ptr<char[]> msg = build_message();
        for (int i = 0; i < REPETITIONS; ++i)
        {
            while (!server_queue.try_dequeue(msg.get(), DATA_SIZE))
            {
            }
            while (!client_queue.try_enqueue(msg.get(), DATA_SIZE))
            {
            }
        }
    };
    auto client = [&]
    {
        std::unique_ptr<char[]> msg = build_message();
        for (int i = 0; i < REPETITIONS; ++i)
        {
            while (!server_queue.try_enqueue(msg.get(), DATA_SIZE))
            {
            }
            while (!client_queue.try_dequeue(msg.get(), DATA_SIZE))
            {
            }
        }
    };
    while (state.KeepRunning())
    {
        std::thread s(server);
        std::thread c(client);
        s.join();
        c.join();
    }
    state.SetItemsProcessed(REPETITIONS * state.iterations());
}

RegisterBenchmarkWithAllMutexes(BenchmarkShmemq, ->Arg(256));
RegisterBenchmarkWithAllMutexes(BenchmarkDemingWS);
RegisterBenchmarkWithAllMutexes(BenchmarkThroughputMultipleMutex, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkThroughput, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkContendedMutex, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkLongestIdle, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkLongestWait, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkContendedMutexMoreWork, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkContendedMutexMoreIdle, ->Apply(CustomBenchmarkArguments));

TEST(contended_mutex, DISABLED_num_writes_per_thread)
{
    int num_threads = std::thread::hardware_concurrency();
    std::vector<size_t> thread_counts(num_threads);
    std::vector<std::thread> threads;
    std::mutex mutex;
    threads.reserve(num_threads);
    size_t sum = 0;
    std::atomic<bool> done{false};
    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&, thread_counter = thread_counts.data() + i]
        {
            while (!done.load(std::memory_order_relaxed))
            {
                mutex.lock();
                ++sum;
                mutex.unlock();
                ++*thread_counter;
            }
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    done = true;
    for (std::thread & t : threads)
        t.join();
    for (int i = 0; i < num_threads; ++i)
    {
        std::cout << i << ": " << thread_counts[i] << std::endl;
    }
}

template<typename T>
struct BenchmarkContendedMutexSKB
{
    size_t num_tests = 1;
    size_t num_workers_per_test = 1;
    void operator()(skb::State & state)
    {
        int num_loops = state.range(0);

        ThreadedBenchmarkRunnerMultipleTimes<ContendedMutexRunner<T>> runner(num_tests, num_workers_per_test, num_loops);
        while (state.KeepRunning())
        {
            runner.full_run();
        }
        runner.shut_down();
        state.SetItemsProcessed(state.iterations() * num_tests * num_workers_per_test * num_loops);
    }
};
template<typename T>
struct BenchmarkContendedMutexSized
{
    size_t num_tests = 1;
    size_t num_workers_per_test = 1;
    void operator()(skb::State & state)
    {
        size_t num_sums = state.range(0) / sizeof(size_t);
        size_t max_num_sums = 1024 * 1024 * 1024;
        size_t num_loops = size_t(1024 * 16);
        size_t current_num_sums = num_sums * num_workers_per_test * num_loops;
        if (current_num_sums > max_num_sums)
            num_loops = std::max(size_t(1), num_loops / (current_num_sums / max_num_sums));

        size_t sum_items = 0;
        ThreadedBenchmarkRunnerMultipleTimes<ContendedMutexRunnerSized<T>> runner(num_tests, num_workers_per_test, num_loops, num_sums);
        while (state.KeepRunning())
        {
            runner.full_run();
            sum_items += runner.runners.front().state.sums.front();
        }
        runner.shut_down();
        CHECK_FOR_PROGRAMMER_ERROR(sum_items == state.iterations() * num_workers_per_test * num_loops);
        state.SetItemsProcessed(sum_items);
    }
};
template<typename T>
struct BenchmarkManyContendedMutex
{
    size_t num_tests = 1;
    size_t num_workers_per_test = 1;
    void operator()(skb::State & state)
    {
        size_t num_sums = state.range(0) / sizeof(size_t);
        size_t num_loops = 64 * 3;
        //size_t num_loops = num_workers_per_test;

        size_t sum_items = 0;
        ThreadedBenchmarkRunnerMultipleTimes<ManyContendedMutexRunner<T>> runner(num_tests, num_workers_per_test, num_loops, num_sums);
        while (state.KeepRunning())
        {
            runner.full_run();
            auto & all_sums = runner.runners.front().state.state;
            sum_items += all_sums.front().sums.front() * all_sums.size();
        }
        runner.shut_down();
        CHECK_FOR_PROGRAMMER_ERROR(sum_items == state.iterations() * num_workers_per_test * num_loops);
        state.SetItemsProcessed(sum_items);
    }
};


template<typename Mutex>
void RegisterShmemQ(skb::CategoryBuilder categories_so_far, interned_string mutex_type)
{
    categories_so_far = categories_so_far.AddCategory("mutex benchmark", "shmemq");
    SKA_BENCHMARK_CATEGORIES(&BenchmarkShmemq<Mutex, skb::State>, categories_so_far.BuildCategories("mutex", std::move(mutex_type)))->SetRange(16, 1024 * 64)->SetRangeMultiplier(2)->SetBaseline("BenchmarkShmemqBaseline");
}

std::vector<std::pair<int, int>> ContendedMutexArgs()
{
    std::vector<std::pair<int, int>> result;
    result.reserve(50);
    int hardware_concurrency = std::thread::hardware_concurrency();
    int max_num_threads = hardware_concurrency * 2;
    for (int i = 1; i <= max_num_threads; i *= 2)
    {
        result.emplace_back(1, i);
    }
    for (int i = 2; i <= max_num_threads; i *= 2)
    {
        result.emplace_back(i, i);
    }
    for (int i = 2; i <= max_num_threads; i *= 2)
    {
        int num_threads = std::max(1, hardware_concurrency / i);
        if (num_threads != i)
        {
            result.emplace_back(i, num_threads);
        }
    }
    return result;
}

template<typename Mutex>
interned_string ContendedMutexBaselineName(const BenchmarkContendedMutexSized<Mutex> & benchmark)
{
    return interned_string("ContendedMutexSizedBaseline" + std::to_string(benchmark.num_tests) + "_" + std::to_string(benchmark.num_workers_per_test));
}
template<typename Mutex>
interned_string ManyContendedMutexBaselineName(const BenchmarkManyContendedMutex<Mutex> & benchmark)
{
    return interned_string("ManyContendedMutexBaseline" + std::to_string(benchmark.num_tests) + "_" + std::to_string(benchmark.num_workers_per_test));
}

void RegisterContendedMutexBaseline()
{
    for (auto [num_tasks, num_threads_per_task] : ContendedMutexArgs())
    {
        BenchmarkContendedMutexSized<std::mutex> runner;
        BenchmarkManyContendedMutex<std::mutex> runner2;
        runner.num_tests = runner2.num_tests = num_tasks;
        runner.num_workers_per_test = runner2.num_workers_per_test = num_threads_per_task;
        SKA_BENCHMARK_CATEGORIES(runner, { "baseline", ContendedMutexBaselineName(runner) });
        SKA_BENCHMARK_CATEGORIES(runner2, { "baseline", ManyContendedMutexBaselineName(runner2) });
    }
}

template<typename Mutex>
void RegisterContendedMutex(skb::CategoryBuilder categories_so_far, interned_string mutex_name)
{
    skb::CategoryBuilder single_sum = categories_so_far.AddCategory("mutex benchmark", "contended_increment");
    skb::CategoryBuilder many_sums = categories_so_far.AddCategory("mutex benchmark", "sized_increments");
    skb::CategoryBuilder many_mutexes = categories_so_far.AddCategory("mutex benchmark", "many_increments");

    for (auto [num_tasks, num_threads_per_task] : ContendedMutexArgs())
    {
        BenchmarkContendedMutexSKB<Mutex> runner;
        BenchmarkContendedMutexSized<Mutex> runner2;
        BenchmarkManyContendedMutex<Mutex> runner3;
        runner.num_tests = runner2.num_tests = runner3.num_tests = num_tasks;
        runner.num_workers_per_test = runner2.num_workers_per_test = runner3.num_workers_per_test = num_threads_per_task;
        interned_string num_tasks_string = to_interned_string(num_tasks);
        interned_string num_threads_string = to_interned_string(num_threads_per_task);
        skb::CategoryBuilder categories = single_sum.AddCategory("num counts", num_tasks_string).AddCategory("workers per count", num_threads_string);
        SKA_BENCHMARK_CATEGORIES(runner, categories.BuildCategories("mutex", mutex_name))->SetRange(1024, 1024 * 1024)->SetRangeMultiplier(2);
        skb::CategoryBuilder categories2 = many_sums.AddCategory("num counts", num_tasks_string).AddCategory("workers per count", num_threads_string);
        SKA_BENCHMARK_CATEGORIES(runner2, categories2.BuildCategories("mutex", mutex_name))->SetRange(sizeof(size_t), 1024 * 1024 * sizeof(size_t))->SetRangeMultiplier(2)->SetBaseline(ContendedMutexBaselineName(runner2));
        skb::CategoryBuilder categories3 = many_mutexes.AddCategory("num counts", num_tasks_string).AddCategory("workers per count", num_threads_string);
        SKA_BENCHMARK_CATEGORIES(runner3, categories3.BuildCategories("mutex", mutex_name))->SetRange(sizeof(size_t), 1024 * 1024 * sizeof(size_t))->SetRangeMultiplier(2)->SetBaseline(ManyContendedMutexBaselineName(runner3));
    }
}

template<typename Mutex>
void RegisterAllBenchmarks(skb::CategoryBuilder categories_so_far, interned_string mutex_type)
{
    RegisterShmemQ<Mutex>(categories_so_far, mutex_type);
    RegisterContendedMutex<Mutex>(categories_so_far, std::move(mutex_type));
}

template<int NoopSpinCount, int PauseSpinCount>
void RegisterParameterizedSpinlock(skb::CategoryBuilder categories_so_far)
{
    auto spin_count_name = [](int count)
    {
        if (count == std::numeric_limits<int>::max())
            return interned_string("always");
        else
            return to_interned_string(count);
    };
    categories_so_far = categories_so_far.AddCategory("noop spin count", spin_count_name(NoopSpinCount))
                                         .AddCategory("pause spin count", spin_count_name(PauseSpinCount));
    RegisterAllBenchmarks<parameterized_spinlock<NoopSpinCount, PauseSpinCount, true>>(categories_so_far.AddCategory("reset after yield", "yes"), "parameterized_spinlock");
    if constexpr (NoopSpinCount != std::numeric_limits<int>::max() && PauseSpinCount != std::numeric_limits<int>::max())
        RegisterAllBenchmarks<parameterized_spinlock<NoopSpinCount, PauseSpinCount, false>>(categories_so_far.AddCategory("reset after yield", "no"), "parameterized_spinlock");
}

void RegisterMutex()
{
    RegisterContendedMutexBaseline();
    skb::CategoryBuilder categories_so_far;
    RegisterAllBenchmarks<std::mutex>(categories_so_far, "std::mutex");
    RegisterAllBenchmarks<std::shared_mutex>(categories_so_far, "std::shared_mutex");
    RegisterAllBenchmarks<std::recursive_mutex>(categories_so_far, "std::recursive_mutex");
    RegisterAllBenchmarks<boost::mutex>(categories_so_far, "boost::mutex");
    RegisterAllBenchmarks<boost::shared_mutex>(categories_so_far, "boost::shared_mutex");
    RegisterAllBenchmarks<boost::recursive_mutex>(categories_so_far, "boost::recursive_mutex");
    RegisterAllBenchmarks<pthread_mutex>(categories_so_far, "pthread_mutex");
    RegisterAllBenchmarks<pthread_mutex_recursive>(categories_so_far, "pthread_mutex_recursive");
    RegisterAllBenchmarks<pthread_mutex_adaptive>(categories_so_far, "pthread_mutex_adaptive");
    RegisterAllBenchmarks<futex_mutex_careful_wake>(categories_so_far, "futex_mutex");
    RegisterAllBenchmarks<futex_mutex>(categories_so_far.AddCategory("futex size", "32 bits"), "futex_mutex_wake_one");
    //RegisterAllBenchmarks<futex_mutex_wake_one_8>(categories_so_far.AddCategory("futex size", "8 bits"), "futex_mutex_wake_one");
    //RegisterAllBenchmarks<futex_mutex_wake_one_16>(categories_so_far.AddCategory("futex size", "16 bits"), "futex_mutex_wake_one");
    RegisterAllBenchmarks<futex_mutex_counted>(categories_so_far, "futex_mutex_counted");
    RegisterAllBenchmarks<spin_to_futex_mutex>(categories_so_far, "spin_to_futex_mutex");
    RegisterAllBenchmarks<spin_to_futex_mutex_add>(categories_so_far, "spin_to_futex_mutex_add");
    RegisterAllBenchmarks<spin_to_futex_mutex_load>(categories_so_far, "spin_to_futex_mutex_load");
    RegisterAllBenchmarks<ticket_mutex>(categories_so_far, "ticket_mutex");
    RegisterAllBenchmarks<ticket_spinlock>(categories_so_far, "ticket_spinlock");
    RegisterAllBenchmarks<semaphore_mutex>(categories_so_far, "semaphore_mutex");
    RegisterAllBenchmarks<spinlock>(categories_so_far, "spinlock");
    RegisterAllBenchmarks<spinlock_amd>(categories_so_far, "spinlock_amd");
    RegisterAllBenchmarks<spinlock_test_and_set>(categories_so_far, "spinlock_test_and_set");
    RegisterAllBenchmarks<spinlock_test_and_set_once>(categories_so_far, "spinlock_test_and_set_once");
    RegisterAllBenchmarks<spinlock_compare_exchange>(categories_so_far, "spinlock_compare_exchange");
    RegisterAllBenchmarks<spinlock_compare_exchange_only>(categories_so_far, "spinlock_compare_exchange_only");
    RegisterAllBenchmarks<k42_spinlock>(categories_so_far, "k42_spinlock");
    RegisterAllBenchmarks<k42_mutex>(categories_so_far, "k42_mutex");
    RegisterAllBenchmarks<terrible_spinlock>(categories_so_far, "terrible_spinlock");
    RegisterParameterizedSpinlock<0, 0>(categories_so_far);
    RegisterParameterizedSpinlock<0, 1>(categories_so_far);
    RegisterParameterizedSpinlock<0, 4>(categories_so_far);
    RegisterParameterizedSpinlock<0, 16>(categories_so_far);
    RegisterParameterizedSpinlock<0, 64>(categories_so_far);
    RegisterParameterizedSpinlock<0, 256>(categories_so_far);
    RegisterParameterizedSpinlock<0, 4096>(categories_so_far);
    RegisterParameterizedSpinlock<0, std::numeric_limits<int>::max()>(categories_so_far);
    RegisterParameterizedSpinlock<1, 0>(categories_so_far);
    RegisterParameterizedSpinlock<1, 4>(categories_so_far);
    RegisterParameterizedSpinlock<1, 16>(categories_so_far);
    RegisterParameterizedSpinlock<1, 64>(categories_so_far);
    RegisterParameterizedSpinlock<1, 256>(categories_so_far);
    RegisterParameterizedSpinlock<1, 4096>(categories_so_far);
    RegisterParameterizedSpinlock<1, std::numeric_limits<int>::max()>(categories_so_far);
    RegisterParameterizedSpinlock<4, 0>(categories_so_far);
    RegisterParameterizedSpinlock<4, 16>(categories_so_far);
    RegisterParameterizedSpinlock<4, 64>(categories_so_far);
    RegisterParameterizedSpinlock<4, 256>(categories_so_far);
    RegisterParameterizedSpinlock<4, 4096>(categories_so_far);
    RegisterParameterizedSpinlock<4, std::numeric_limits<int>::max()>(categories_so_far);
    RegisterParameterizedSpinlock<16, 0>(categories_so_far);
    RegisterParameterizedSpinlock<16, 64>(categories_so_far);
    RegisterParameterizedSpinlock<16, 256>(categories_so_far);
    RegisterParameterizedSpinlock<16, 4096>(categories_so_far);
    RegisterParameterizedSpinlock<16, std::numeric_limits<int>::max()>(categories_so_far);
    RegisterParameterizedSpinlock<64, 0>(categories_so_far);
    RegisterParameterizedSpinlock<64, 256>(categories_so_far);
    RegisterParameterizedSpinlock<64, 4096>(categories_so_far);
    RegisterParameterizedSpinlock<64, std::numeric_limits<int>::max()>(categories_so_far);
    RegisterParameterizedSpinlock<256, 0>(categories_so_far);
    RegisterParameterizedSpinlock<256, 4096>(categories_so_far);
    RegisterParameterizedSpinlock<256, std::numeric_limits<int>::max()>(categories_so_far);
    RegisterParameterizedSpinlock<4096, 0>(categories_so_far);
    RegisterParameterizedSpinlock<4096, std::numeric_limits<int>::max()>(categories_so_far);
    RegisterParameterizedSpinlock<std::numeric_limits<int>::max(), 0>(categories_so_far);

#if 0
    /*BENCHMARK_TEMPLATE(benchmark, two_byte_futex_mutex) __VA_ARGS__;*/\

#endif
}

TEST(futex_mutex, DISABLED_one_byte)
{
    futex_mutex_wake_one_8 lock;
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

TEST(k42_mutex, spinlock)
{
    k42_spinlock lock;
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

TEST(k42_mutex, mutex)
{
    k42_mutex lock;
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

TEST(futex_mutex, condition_variable_counted)
{
    futex_mutex mutex;
    futex_condition_variable_counted cv;

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

TEST(futex_mutex, DISABLED_two_byte_condition_variable)
{
    futex_mutex_wake_one_8 mutex;
    futex_condition_variable_two_bytes cv;

    int num_work = 0;
    std::vector<std::thread> threads;
    int num_workers = std::thread::hardware_concurrency();
    threads.reserve(num_workers);
    auto do_work = [&]
    {
        std::unique_lock<futex_mutex_wake_one_8> lock(mutex);
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

TEST(futex_mutex, DISABLED_two_byte_condition_variable_add_only)
{
    futex_mutex_wake_one_8 mutex;
    futex_condition_variable_two_bytes_increment_only cv;

    int num_work = 0;
    std::vector<std::thread> threads;
    int num_workers = std::thread::hardware_concurrency();
    threads.reserve(num_workers);
    auto do_work = [&]
    {
        std::unique_lock<futex_mutex_wake_one_8> lock(mutex);
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
#else
#include <mutex>
#include <thread>
#include <list>

#include "benchmark/benchmark.h"
#include <algorithm>
#include <chrono>
#include <array>
#include <condition_variable>
#include <shared_mutex>
#include <atomic>
#include <cstring>
#include <emmintrin.h>
#include <numeric>
#include <random>
#include <deque>
#ifdef _WIN32
#include <intrin.h>
#pragma intrinsic(_umul128)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#else
#include <semaphore.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <pthread.h>
#endif

// todo: try WTF lock (aka parking lot)

inline void futex_wait(std::atomic<uint32_t> & to_wait_on, uint32_t expected)
{
#ifdef _WIN32
    WaitOnAddress(&to_wait_on, &expected, sizeof(expected), INFINITE);
#else
    syscall(SYS_futex, &to_wait_on, FUTEX_WAIT_PRIVATE, expected, nullptr, nullptr, 0);
#endif
}
template<typename T>
inline void futex_wake_single(std::atomic<T> & to_wake)
{
#ifdef _WIN32
    WakeByAddressSingle(&to_wake);
#else
    syscall(SYS_futex, &to_wake, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
#endif
}
template<typename T>
inline void futex_wake_all(std::atomic<T> & to_wake)
{
#ifdef _WIN32
    WakeByAddressAll(&to_wake);
#else
    syscall(SYS_futex, &to_wake, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
#endif
}

struct ticket_mutex
{
    void lock()
    {
        unsigned my = in.fetch_add(1);
        for (;;)
        {
            unsigned now = out;
            if (my == now)
                break;
            futex_wait(out, now);
        }
    }
    void unlock()
    {
        unsigned new_value = out.fetch_add(1) + 1;
        if (new_value != in)
            futex_wake_all(out);
    }

private:
    std::atomic<uint32_t> in{ 0 };
    std::atomic<uint32_t> out{ 0 };
};

struct ticket_spinlock
{
    void lock()
    {
        unsigned my = in.fetch_add(1, std::memory_order_relaxed);
        for (int spin_count = 0; out.load(std::memory_order_acquire) != my; ++spin_count)
        {
            if (spin_count < 16)
                _mm_pause();
            else
            {
                std::this_thread::yield();
                spin_count = 0;
            }
        }
    }
    void unlock()
    {
        out.store(out.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }

private:
    std::atomic<unsigned> in{ 0 };
    std::atomic<unsigned> out{ 0 };
};

#ifdef _WIN32
struct critical_section
{
    critical_section()
    {
        InitializeCriticalSection(&cs);
    }
    ~critical_section()
    {
        DeleteCriticalSection(&cs);
    }
    void lock()
    {
        EnterCriticalSection(&cs);
    }
    void unlock()
    {
        LeaveCriticalSection(&cs);
    }

private:
    CRITICAL_SECTION cs;
};

struct critical_section_spin
{
    critical_section_spin()
    {
        InitializeCriticalSectionAndSpinCount(&cs, 4000);
    }
    ~critical_section_spin()
    {
        DeleteCriticalSection(&cs);
    }
    void lock()
    {
        EnterCriticalSection(&cs);
    }
    void unlock()
    {
        LeaveCriticalSection(&cs);
    }

private:
    CRITICAL_SECTION cs;
};

struct srw_lock
{
    void lock()
    {
        AcquireSRWLockExclusive(&l);
    }
    void unlock()
    {
        ReleaseSRWLockExclusive(&l);
    }

private:
    SRWLOCK l = SRWLOCK_INIT;
};
#else
struct pthread_mutex
{
    ~pthread_mutex()
    {
        pthread_mutex_destroy(&mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

private:
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
};
struct pthread_mutex_recursive
{
    ~pthread_mutex_recursive()
    {
        pthread_mutex_destroy(&mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

private:
    pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
};
struct pthread_mutex_adaptive
{
    ~pthread_mutex_adaptive()
    {
        pthread_mutex_destroy(&mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

private:
    pthread_mutex_t mutex = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;
};
#endif

struct futex_mutex
{
    void lock()
    {
        if (state.exchange(locked, std::memory_order_acquire) == unlocked)
            return;
        while (state.exchange(sleeper, std::memory_order_acquire) != unlocked)
        {
            futex_wait(state, sleeper);
        }
    }
    void unlock()
    {
        if (state.exchange(unlocked, std::memory_order_release) == sleeper)
            futex_wake_single(state);
    }

private:
    std::atomic<uint32_t> state{ unlocked };

    static constexpr uint32_t unlocked = 0;
    static constexpr uint32_t locked = 1;
    static constexpr uint32_t sleeper = 2;
};

struct spin_to_futex_mutex
{
    void lock()
    {
        unsigned old_state = unlocked;
        if (state.compare_exchange_strong(old_state, locked, std::memory_order_acquire))
            return;
        for (;;)
        {
            old_state = state.exchange(sleeper, std::memory_order_acquire);
            if (old_state == unlocked)
                return;
            else if (old_state == locked)
            {
                for (;;)
                {
                    _mm_pause();
                    if (state.load(std::memory_order_acquire) == unlocked)
                        break;
                    std::this_thread::yield();
                    if (state.load(std::memory_order_acquire) == unlocked)
                        break;
                }
            }
            else
                futex_wait(state, sleeper);
        }
    }
    void unlock()
    {
        unsigned old_state = state.load(std::memory_order_relaxed);
        state.store(unlocked, std::memory_order_release);
        if (old_state == sleeper)
            futex_wake_single(state);
    }

private:
    std::atomic<unsigned> state{ unlocked };
    static constexpr unsigned unlocked = 0;
    static constexpr unsigned locked = 1;
    static constexpr unsigned sleeper = 2;
};

#ifdef _WIN32

struct semaphore
{
    semaphore()
        : semaphore(0)
    {
    }
    explicit semaphore(LONG initial_amount)
        : windows_semaphore(CreateSemaphoreA(nullptr, initial_amount, std::numeric_limits<LONG>::max(), nullptr))
    {
    }
    ~semaphore()
    {
        CloseHandle(windows_semaphore);
    }
    void acquire()
    {
        WaitForSingleObject(windows_semaphore, INFINITE);
    }
    void release(ptrdiff_t update = 1)
    {
        ReleaseSemaphore(windows_semaphore, static_cast<LONG>(update), nullptr);
    }

private:
    HANDLE windows_semaphore{ 0 };
};

#else

struct semaphore
{
    semaphore()
        : semaphore(0)
    {
    }
    explicit semaphore(std::ptrdiff_t value)
    {
        sem_init(&sem, false, value);
    }
    ~semaphore()
    {
        sem_destroy(&sem);
    }
    void release()
    {
        int result = sem_post(&sem);
        assert(!result); static_cast<void>(result);
    }
    void release(ptrdiff_t update)
    {
        for (; update > 0; --update)
        {
            release();
        }
    }

    void acquire()
    {
        while (sem_wait(&sem))
        {
            assert(errno == EINTR);
        }
    }

private:
    sem_t sem;
};
#endif

struct semaphore_custom
{
private:
    std::mutex mutex;
    std::condition_variable condition;
    std::ptrdiff_t count = 0;

public:
    explicit semaphore_custom(ptrdiff_t desired = 0)
        : count(desired)
    {
    }

    void release()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            ++count;
        }
        condition.notify_one();
    }
    void release(ptrdiff_t update)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            count += update;
        }
        condition.notify_all();
    }

    void acquire()
    {
        std::unique_lock<std::mutex> lock(mutex);
        while (!count)
            condition.wait(lock);
        --count;
    }
};

struct semaphore_mutex
{
    void lock()
    {
        sema.acquire();
    }
    void unlock()
    {
        sema.release();
    }
private:
    semaphore sema{ 1 };
};

struct terrible_spinlock
{
    void lock()
    {
        while (locked.test_and_set(std::memory_order_acquire))
        {
        }
    }
    void unlock()
    {
        locked.clear(std::memory_order_release);
    }

private:
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
};
struct spinlock_test_and_set
{
    void lock()
    {
        for (;;)
        {
            if (try_lock())
                break;
            _mm_pause();
            if (try_lock())
                break;
            std::this_thread::yield();
        }
    }
    bool try_lock()
    {
        return !locked.test_and_set(std::memory_order_acquire);
    }
    void unlock()
    {
        locked.clear(std::memory_order_release);
    }

private:
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
};
struct spinlock_test_and_set_once
{
    void lock()
    {
        for (;;)
        {
            if (!locked.exchange(true, std::memory_order_acquire))
                break;
            for (;;)
            {
                _mm_pause();
                if (!locked.load(std::memory_order_acquire))
                    break;
                std::this_thread::yield();
                if (!locked.load(std::memory_order_acquire))
                    break;
            }
        }
    }
    bool try_lock()
    {
        return !locked.load(std::memory_order_acquire) && !locked.exchange(true, std::memory_order_acquire);
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{ false };
};
struct spinlock_compare_exchange
{
    void lock()
    {
        for (;;)
        {
            bool unlocked = false;
            if (locked.compare_exchange_weak(unlocked, true, std::memory_order_acquire))
                break;
            for (;;)
            {
                _mm_pause();
                if (!locked.load(std::memory_order_acquire))
                    break;
                std::this_thread::yield();
                if (!locked.load(std::memory_order_acquire))
                    break;
            }
        }
    }
    bool try_lock()
    {
        return !locked.load(std::memory_order_acquire) && !locked.exchange(true, std::memory_order_acquire);
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{ false };
};
struct spinlock_compare_exchange_only
{
    void lock()
    {
        for (;;)
        {
            if (try_lock())
                break;
            _mm_pause();
            if (try_lock())
                break;
            std::this_thread::yield();
        }
    }
    bool try_lock()
    {
        bool unlocked = false;
        return locked.compare_exchange_weak(unlocked, true, std::memory_order_acquire);
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{ false };
};

struct spinlock_amd
{
    void lock()
    {
        for (;;)
        {
            bool was_locked = locked.load(std::memory_order_relaxed);
            if (!was_locked && locked.compare_exchange_weak(was_locked, true, std::memory_order_acquire))
                break;
            _mm_pause();
        }
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{ false };
};

struct spinlock
{
    void lock()
    {
        for (int spin_count = 0; !try_lock(); ++spin_count)
        {
            if (spin_count < 16)
                _mm_pause();
            else
            {
                std::this_thread::yield();
                spin_count = 0;
            }
        }
    }
    bool try_lock()
    {
        return !locked.load(std::memory_order_relaxed) && !locked.exchange(true, std::memory_order_acquire);
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{ false };
};

template<int SpinNoOpCount, int SpinPauseCount, bool ResetCountAfterYield>
struct parameterized_spinlock
{
    void lock()
    {
        int spin_count = 0;
        auto on_spin = [&]
        {
            if constexpr (SpinNoOpCount > 0)
            {
                if (SpinNoOpCount == std::numeric_limits<int>::max() || spin_count < SpinNoOpCount)
                    return;
            }
            if constexpr (SpinNoOpCount != std::numeric_limits<int>::max() && SpinPauseCount > 0)
            {
                if (SpinPauseCount == std::numeric_limits<int>::max() || spin_count < SpinPauseCount)
                {
                    _mm_pause();
                    return;
                }
            }
            if constexpr (SpinNoOpCount != std::numeric_limits<int>::max() && SpinPauseCount != std::numeric_limits<int>::max())
            {
                std::this_thread::yield();
                if constexpr (ResetCountAfterYield)
                    spin_count = 0;
            }
        };
        for (;;)
        {
            bool expected = false;
            if (locked.compare_exchange_weak(expected, true, std::memory_order_acquire))
                return;
            for (;; ++spin_count)
            {
                on_spin();
                if (!locked.load(std::memory_order_acquire))
                    break;
            }
        }
    }
    bool try_lock()
    {
        bool expected = false;
        return locked.compare_exchange_strong(expected, true, std::memory_order_acquire);
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked{ false };
};

template<typename T>
void benchmark_mutex_lock_unlock(benchmark::State& state)
{
    T m;
    while (state.KeepRunning())
    {
        m.lock();
        m.unlock();
    }
}

#ifdef _WIN32
#define RegisterBenchmarkWithWindowsMutexes(benchmark, ...)\
    BENCHMARK_TEMPLATE(benchmark, critical_section) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, critical_section_spin) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, srw_lock) __VA_ARGS__;
#define RegisterBenchmarkWithPthreadMutexes(benchmark, ...)
#else
#define RegisterBenchmarkWithWindowsMutexes(benchmark, ...)
#define RegisterBenchmarkWithPthreadMutexes(benchmark, ...)\
    BENCHMARK_TEMPLATE(benchmark, pthread_mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, pthread_mutex_recursive) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, pthread_mutex_adaptive) __VA_ARGS__;
#endif

#define RegisterBenchmarkWithAllMutexes(benchmark, ...)\
    BENCHMARK_TEMPLATE(benchmark, std::mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, std::shared_mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, std::recursive_mutex) __VA_ARGS__;\
    RegisterBenchmarkWithWindowsMutexes(benchmark, __VA_ARGS__);\
    RegisterBenchmarkWithPthreadMutexes(benchmark, __VA_ARGS__);\
    BENCHMARK_TEMPLATE(benchmark, futex_mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spin_to_futex_mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, ticket_spinlock) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, ticket_mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, semaphore_mutex) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spinlock) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spinlock_amd) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spinlock_test_and_set) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spinlock_test_and_set_once) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spinlock_compare_exchange) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, spinlock_compare_exchange_only) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, terrible_spinlock) __VA_ARGS__;\
    /*BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 0>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 1>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 4>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 16>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 64>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<0, 256>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 1>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 4>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 16>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 64>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<1, 256>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 4>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 16>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 64>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<4, 256>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<16, 16>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<16, 64>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<16, 256>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<64, 64>) __VA_ARGS__;\
    BENCHMARK_TEMPLATE(benchmark, parameterized_spinlock<64, 256>) __VA_ARGS__;*/\

RegisterBenchmarkWithAllMutexes(benchmark_mutex_lock_unlock);

void benchmark_shared_mutex_lock_shared(benchmark::State& state)
{
    std::shared_mutex m;
    while (state.KeepRunning())
    {
        m.lock_shared();
        m.unlock_shared();
    }
}
BENCHMARK(benchmark_shared_mutex_lock_shared);

void benchmark_semaphore_release_and_acquire(benchmark::State& state)
{
    semaphore m;
    while (state.KeepRunning())
    {
        m.release();
        m.acquire();
    }
}
BENCHMARK(benchmark_semaphore_release_and_acquire);

void benchmark_custom_semaphore_release_and_acquire(benchmark::State& state)
{
    semaphore_custom m;
    while (state.KeepRunning())
    {
        m.release();
        m.acquire();
    }
}
BENCHMARK(benchmark_custom_semaphore_release_and_acquire);


template<typename State>
struct ThreadedBenchmarkRunner
{
    ThreadedBenchmarkRunner(State* state, int num_threads)
        : state(state)
    {
        threads.reserve(num_threads);
        for (int i = 0; i < num_threads; ++i)
        {
            threads.emplace_back([this, state, i]
            {
                finished.release();
                for (;;)
                {
                    all_started_sema.acquire();
                    if (ended)
                        break;
                    state->run_benchmark(i);
                    finished.release();
                }
            });
        }
        for (size_t i = threads.size(); i > 0; --i)
            finished.acquire();
    }

    void wait_until_started()
    {
        state->start_run();
    }
    void release()
    {
        all_started_sema.release(threads.size());
    }
    void wait_until_finished()
    {
        for (size_t i = threads.size(); i > 0; --i)
            finished.acquire();
        state->end_run();
    }

    void full_run()
    {
        wait_until_started();
        release();
        wait_until_finished();
    }

    void shut_down()
    {
        ended = true;
        all_started_sema.release(threads.size());
        for (std::thread& thread : threads)
            thread.join();
    }

private:
    State* state;
    std::vector<std::thread> threads;
    semaphore all_started_sema;
    semaphore finished;
    bool ended = false;
};

template<typename State>
struct ThreadedBenchmarkRunnerMultipleTimes
{
    template<typename... StateArgs>
    ThreadedBenchmarkRunnerMultipleTimes(int num_runners, int num_threads, StateArgs&&... state_args)
    {
        for (int i = 0; i < num_runners; ++i)
        {
            runners.emplace_back(num_threads, state_args...);
        }
    }

    void wait_until_started()
    {
        for (OneRunnerAndState& runner : runners)
            runner.runner.wait_until_started();
    }

    void start_run()
    {
        for (OneRunnerAndState& runner : runners)
            runner.runner.release();
    }

    void finish_run()
    {
        for (OneRunnerAndState& runner : runners)
            runner.runner.wait_until_finished();
    }

    void full_run()
    {
        wait_until_started();
        start_run();
        finish_run();
    }

    void shut_down()
    {
        for (OneRunnerAndState& runner : runners)
            runner.runner.shut_down();
    }

    struct OneRunnerAndState
    {
        State state;
        ThreadedBenchmarkRunner<State> runner;

        template<typename... StateArgs>
        OneRunnerAndState(int num_threads, StateArgs&&... state_args)
            : state(num_threads, std::forward<StateArgs>(state_args)...), runner(&state, num_threads)
        {
        }
    };
    std::list<OneRunnerAndState> runners;
};

template<typename T>
struct ContendedMutexRunner
{
    size_t num_loops = 1;
    size_t sum = 0;
    size_t expected_sum;
    T mutex;

    explicit ContendedMutexRunner(int num_threads, size_t num_loops)
        : num_loops(num_loops)
        , expected_sum(num_threads* num_loops)
    {
    }

    void start_run()
    {
        sum = 0;
    }
    void end_run()
    {
        assert(sum == expected_sum);
    }

    void run_benchmark(int)
    {
        for (size_t i = num_loops; i != 0; --i)
        {
            mutex.lock();
            ++sum;
            mutex.unlock();
        }
    }
};
template<typename T>
struct ContendedMutexRunnerSized
{
    size_t num_loops = 1;
    std::vector<size_t> sums;
    size_t expected_sum;
    T mutex;

    explicit ContendedMutexRunnerSized(int num_threads, size_t num_loops, size_t num_sums)
        : num_loops(num_loops)
        , sums(num_sums)
        , expected_sum(num_threads* num_loops)
    {
    }

    void start_run()
    {
        for (size_t& sum : sums)
            sum = 0;
    }
    void end_run()
    {
        assert(sums.front() == expected_sum);
    }

    void run_benchmark(int)
    {
        for (size_t i = num_loops; i != 0; --i)
        {
            mutex.lock();
            for (size_t& sum : sums)
                ++sum;
            mutex.unlock();
        }
    }
};

#ifdef _WIN32
template<typename Randomness>
uint64_t UniformRandom(uint64_t max, Randomness & random)
{
    static_assert(Randomness::min() == 0 && Randomness::max() == std::numeric_limits<uint64_t>::max());
    uint64_t random_value = random();
    if (max == std::numeric_limits<uint64_t>::max())
        return random_value;
    ++max;
    uint64_t high_bits;
    uint64_t lower_bits = _umul128(random_value, max, &high_bits);
    if (lower_bits < max)
    {
        uint64_t threshold = -max % max;
        while (lower_bits < threshold)
        {
            random_value = random();
            uint64_t lower_bits = _umul128(random_value, max, &high_bits);
        }
    }
    return high_bits;
}
#else
template<typename Randomness>
uint64_t UniformRandom(uint64_t max, Randomness & random)
{
    static_assert(Randomness::min() == 0 && Randomness::max() == std::numeric_limits<uint64_t>::max());
    uint64_t random_value = random();
    if (max == std::numeric_limits<uint64_t>::max())
        return random_value;
    ++max;
    __uint128_t to_range = static_cast<__uint128_t>(random_value) * static_cast<__uint128_t>(max);
    uint64_t lower_bits = static_cast<uint64_t>(to_range);
    if (lower_bits < max)
    {
        uint64_t threshold = -max % max;
        while (lower_bits < threshold)
        {
            random_value = random();
            to_range = static_cast<__uint128_t>(random_value)* static_cast<__uint128_t>(max);
            lower_bits = static_cast<uint64_t>(to_range);
        }
    }
    return static_cast<uint64_t>(to_range >> 64);
}
#endif

template<typename T>
struct ManyContendedMutexRunner
{
    struct OneMutexAndState
    {
        explicit OneMutexAndState(size_t num_sums)
            : sums(num_sums)
        {
        }
        std::vector<size_t> sums;
        T mutex;
    };

    std::deque<OneMutexAndState> state;
    size_t expected_sum;
    struct ThreadState
    {
        template<typename Seed>
        ThreadState(Seed& random_seed, size_t num_loops, std::deque<OneMutexAndState>& state)
            : randomness(random_seed)
        {
            CHECK_FOR_PROGRAMMER_ERROR(num_loops % state.size() == 0);
            iteration_order.reserve(num_loops);
            for (size_t i = 0; i < num_loops; ++i)
            {
                iteration_order.push_back(&state[i % state.size()]);
            }
        }

        std::mt19937_64 randomness;
        std::vector<OneMutexAndState*> iteration_order;
    };

    std::vector<ThreadState> thread_state;

    explicit ManyContendedMutexRunner(int num_threads, size_t num_loops, size_t num_sums)
        : expected_sum(num_loops)
    {
        for (int i = 0; i < num_threads; ++i)
        {
            state.emplace_back(num_sums);
        }
        //random_seed_seq seed;
        std::random_device true_random;
        thread_state.reserve(num_threads);
        for (int i = 0; i < num_threads; ++i)
        {
            thread_state.emplace_back(true_random(), num_loops, state);
        }
    }

    void start_run()
    {
        for (OneMutexAndState& state : state)
        {
            for (size_t& sum : state.sums)
                sum = 0;
        }
    }
    void end_run()
    {
        CHECK_FOR_PROGRAMMER_ERROR(state.front().sums.front() == expected_sum);
    }

    void run_benchmark(int thread_num)
    {
        ThreadState& state = thread_state[thread_num];
        std::mt19937_64& my_thread_randomness = state.randomness;
        size_t num_states = state.iteration_order.size();
        for (auto it = state.iteration_order.begin(), end = state.iteration_order.end(); it != end; ++it)
        {
            --num_states;
            if (num_states)
            {
                size_t random_pick = UniformRandom(num_states, my_thread_randomness);
                std::iter_swap(it, it + random_pick);
            }
            OneMutexAndState& one = **it;
            one.mutex.lock();
            for (size_t& sum : one.sums)
                ++sum;
            one.mutex.unlock();
        }
    }
};

template<typename T>
void BenchmarkContendedMutex(benchmark::State& state)
{
    static constexpr size_t num_loops = 1024 * 16;

    ThreadedBenchmarkRunnerMultipleTimes<ContendedMutexRunner<T>> runner(state.range(0), state.range(1), num_loops);
    while (state.KeepRunning())
    {
        runner.full_run();
    }
    runner.shut_down();
}

template<typename T>
struct ContendedMutexRunnerMoreIdle
{
    size_t sum = 0;
    size_t num_loops = 1;
    size_t expected_sum = 0;
    T mutex;

    explicit ContendedMutexRunnerMoreIdle(int num_threads, size_t num_loops)
        : num_loops(num_loops), expected_sum(num_threads* num_loops)
    {
    }

    void start_run()
    {
        sum = 0;
    }
    void end_run()
    {
        assert(sum == expected_sum);
    }

    void run_benchmark(int)
    {
        for (size_t i = num_loops; i != 0; --i)
        {
            mutex.lock();
            ++sum;
            mutex.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            /*if (iteration < 1024)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(1024 - iteration) / 32);
            }*/
        }
    }
};

template<typename T>
void BenchmarkContendedMutexMoreIdle(benchmark::State& state)
{
    static constexpr size_t num_loops = 1024;

    ThreadedBenchmarkRunnerMultipleTimes<ContendedMutexRunnerMoreIdle<T>> runner(state.range(0), state.range(1), num_loops);
    while (state.KeepRunning())
    {
        runner.full_run();
    }
    runner.shut_down();
}

template<typename T, size_t N>
struct TopNHeap
{
    void fill(const T& value)
    {
        heap.fill(value);
    }

    void add(const T& value)
    {
        if (value > heap.front())
        {
            std::pop_heap(heap.begin(), heap.end(), std::greater<>());
            heap.back() = value;
            std::push_heap(heap.begin(), heap.end(), std::greater<>());
        }
    }
    void add(T&& value)
    {
        if (value > heap.front())
        {
            std::pop_heap(heap.begin(), heap.end(), std::greater<>());
            heap.back() = std::move(value);
            std::push_heap(heap.begin(), heap.end(), std::greater<>());
        }
    }

    void sort()
    {
        std::sort_heap(heap.begin(), heap.end(), std::greater<>());
    }

    template<size_t ON>
    void merge(const TopNHeap<T, ON>& to_merge)
    {
        for (const T& val : to_merge.heap)
            add(val);
    }

    std::array<T, N> heap;
};


template<typename T>
struct LongestWaitRunner
{
    TopNHeap<size_t, 4> longest_waits;
    T mutex;
    size_t current_longest_wait = 0;
    size_t num_loops = 1;

    explicit LongestWaitRunner(int, size_t num_loops)
        : num_loops(num_loops)
    {
        longest_waits.fill(0);
    }

    void start_run()
    {
        current_longest_wait = 0;
    }
    void end_run()
    {
        longest_waits.add(current_longest_wait);
    }

    void run_benchmark(int)
    {
        for (size_t i = num_loops; i != 0; --i)
        {
            auto time_before = std::chrono::high_resolution_clock::now();
            mutex.lock();
            size_t wait_time_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time_before).count();
            current_longest_wait = std::max(wait_time_nanos, current_longest_wait);
            mutex.unlock();
        }
    }
};

template<typename T>
void BenchmarkLongestWait(benchmark::State& state)
{
    static constexpr size_t num_loops = 1024 * 16;
    ThreadedBenchmarkRunnerMultipleTimes<LongestWaitRunner<T>> runner(state.range(0), state.range(1), num_loops);
    while (state.KeepRunning())
    {
        runner.full_run();
    }
    runner.shut_down();
    TopNHeap<size_t, 4> longest_waits_merged;
    longest_waits_merged.fill(0);
    for (auto& runner : runner.runners)
    {
        longest_waits_merged.merge(runner.state.longest_waits);
    }
    longest_waits_merged.sort();
    for (size_t i = 0; i < longest_waits_merged.heap.size(); ++i)
    {
        state.counters["Wait" + std::to_string(i)] = static_cast<double>(longest_waits_merged.heap[i]);
    }
}

template<typename T>
struct LongestIdleRunner
{
    TopNHeap<size_t, 4> longest_idles;
    T mutex;
    size_t current_longest_idle = 0;
    size_t num_loops = 1;
    bool first = true;
    std::chrono::high_resolution_clock::time_point time_before = std::chrono::high_resolution_clock::now();

    explicit LongestIdleRunner(int, size_t num_loops)
        : num_loops(num_loops)
    {
        longest_idles.fill(0);
    }

    void start_run()
    {
        current_longest_idle = 0;
        first = true;
    }
    void end_run()
    {
        longest_idles.add(current_longest_idle);
    }

    void run_benchmark(int)
    {
        for (size_t i = num_loops; i != 0; --i)
        {
            mutex.lock();
            size_t wait_time_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time_before).count();
            if (first)
                first = false;
            else if (wait_time_nanos > current_longest_idle)
                current_longest_idle = wait_time_nanos;
            time_before = std::chrono::high_resolution_clock::now();
            mutex.unlock();
        }
    }
};

template<typename T>
void BenchmarkLongestIdle(benchmark::State& state)
{
    static constexpr size_t num_loops = 1024 * 16;
    ThreadedBenchmarkRunnerMultipleTimes<LongestIdleRunner<T>> runner(state.range(0), state.range(1), num_loops);
    while (state.KeepRunning())
    {
        runner.full_run();
    }
    runner.shut_down();
    TopNHeap<size_t, 4> longest_idles_merged;
    longest_idles_merged.fill(0);
    for (auto& runner : runner.runners)
    {
        longest_idles_merged.merge(runner.state.longest_idles);
    }
    longest_idles_merged.sort();
    for (size_t i = 0; i < longest_idles_merged.heap.size(); ++i)
    {
        state.counters["Idle" + std::to_string(i)] = static_cast<double>(longest_idles_merged.heap[i]);
    }
}

static void CustomBenchmarkArguments(benchmark::internal::Benchmark* b)
{
    int hardware_concurrency = std::thread::hardware_concurrency();
    int max_num_threads = hardware_concurrency * 2;
    for (int i = 1; i <= max_num_threads; i *= 2)
    {
        b->Args({ 1, i });
    }
    for (int i = 2; i <= max_num_threads; i *= 2)
    {
        b->Args({ i, i });
    }
    for (int i = 2; i <= max_num_threads; i *= 2)
    {
        int num_threads = std::max(1, hardware_concurrency / i);
        if (num_threads != i)
        {
            b->Args({ i, num_threads });
        }
    }
}


template<typename T>
struct ContendedMutexMoreWork
{
    static constexpr size_t NUM_LISTS = 8;
    std::list<size_t> linked_lists[NUM_LISTS];
    T mutex[NUM_LISTS];
    size_t num_loops = 1;

    explicit ContendedMutexMoreWork(int num_threads, size_t num_loops)
        : num_loops(num_loops)
    {
        for (std::list<size_t>& l : linked_lists)
        {
            for (int i = 0; i < num_threads; ++i)
                l.push_back(i);
        }
    }

    void start_run()
    {
    }
    void end_run()
    {
    }

    void run_benchmark(int)
    {
        for (size_t i = num_loops; i != 0; --i)
        {
            size_t index = i % NUM_LISTS;
            mutex[index].lock();
            linked_lists[index].push_back(i);
            linked_lists[index].pop_front();
            mutex[index].unlock();
        }
    }
};

template<typename T>
void BenchmarkContendedMutexMoreWork(benchmark::State& state)
{
    static constexpr size_t num_loops = 1024;
    ThreadedBenchmarkRunnerMultipleTimes<ContendedMutexMoreWork<T>> runner(state.range(0), state.range(1), num_loops);
    while (state.KeepRunning())
    {
        runner.full_run();
    }
    runner.shut_down();
}

template<typename T>
struct ThroughputRunner
{
    std::chrono::high_resolution_clock::time_point* done_time;
    size_t sum = 0;
    T mutex;

    explicit ThroughputRunner(int, std::chrono::high_resolution_clock::time_point* done_time)
        : done_time(done_time)
    {
    }

    void start_run()
    {
        sum = 0;
    }
    void end_run()
    {
    }

    void run_benchmark(int)
    {
        std::chrono::high_resolution_clock::time_point until = *done_time;
        while (std::chrono::high_resolution_clock::now() < until)
        {
            mutex.lock();
            ++sum;
            mutex.unlock();
        }
    }
};

template<typename T>
void BenchmarkThroughput(benchmark::State& state)
{
    std::chrono::high_resolution_clock::time_point end_time = std::chrono::high_resolution_clock::now();
    int num_states = state.range(0);
    ThreadedBenchmarkRunnerMultipleTimes<ThroughputRunner<T>> runner(num_states, state.range(1), &end_time);
    runner.wait_until_started();
    end_time = std::chrono::high_resolution_clock::now() + std::chrono::seconds(1);
    runner.start_run();
    runner.finish_run();
    size_t sum = std::accumulate(runner.runners.begin(), runner.runners.end(), size_t(0), [](size_t l, const auto& r)
    {
        return l + r.state.sum;
    });
    runner.shut_down();
    state.counters["Throughput"] = sum;
}

template<typename T>
struct ThroughputRunnerMultipleMutex
{
    std::chrono::high_resolution_clock::time_point* done_time;
    struct alignas(64) MutexAndSum
    {
        size_t sum = 0;
        T mutex;
    };
    static constexpr size_t num_mutexes = 8;
    MutexAndSum sums[num_mutexes];

    explicit ThroughputRunnerMultipleMutex(int, std::chrono::high_resolution_clock::time_point* done_time)
        : done_time(done_time)
    {
    }

    void start_run()
    {
        for (MutexAndSum& sum : sums)
            sum.sum = 0;
    }
    void end_run()
    {
    }

    void run_benchmark(int)
    {
        std::chrono::high_resolution_clock::time_point until = *done_time;
        std::mt19937_64 randomness{ std::random_device()() };
        std::uniform_int_distribution<int> distribution(0, num_mutexes - 1);
        while (std::chrono::high_resolution_clock::now() < until)
        {
            MutexAndSum& to_increment = sums[distribution(randomness)];
            to_increment.mutex.lock();
            ++to_increment.sum;
            to_increment.mutex.unlock();
        }
    }
};

template<typename T>
void BenchmarkThroughputMultipleMutex(benchmark::State& state)
{
    std::chrono::high_resolution_clock::time_point end_time = std::chrono::high_resolution_clock::now();
    int64_t num_states = state.range(0);
    ThreadedBenchmarkRunnerMultipleTimes<ThroughputRunnerMultipleMutex<T>> runner(num_states, state.range(1), &end_time);
    runner.wait_until_started();
    end_time = std::chrono::high_resolution_clock::now() + std::chrono::seconds(1);
    runner.start_run();
    runner.finish_run();
    size_t sum = std::accumulate(runner.runners.begin(), runner.runners.end(), size_t(0), [](size_t l, const auto& r)
    {
        return l + std::accumulate(std::begin(r.state.sums), std::end(r.state.sums), size_t(0), [](size_t l, const auto& r)
        {
            return l + r.sum;
        });
    });
    runner.shut_down();
    state.counters["Throughput"] = sum;
}

template<typename T>
void BenchmarkDemingWS(benchmark::State& state)
{
    // code from http://demin.ws/blog/english/2012/05/05/atomic-spinlock-mutex/
    T mutex;
    int value = 0;
    semaphore sem;
    semaphore one_loop_done;
    bool done = false;
    auto loop = [&](bool inc, int limit)
    {
        for (int i = 0; i < limit; ++i)
        {
            mutex.lock();
            if (inc)
                ++value;
            else
                --value;
            mutex.unlock();
        }
    };
    auto background_thread = [&](bool inc, int limit)
    {
        for (;;)
        {
            sem.acquire();
            if (done)
                break;
            loop(inc, limit);
            one_loop_done.release();
        }
    };
    int num_increment = 20000000;
    int num_decrement = 10000000;
    std::thread t(background_thread, true, num_increment);
    while (state.KeepRunning())
    {
        value = 0;
        sem.release();
        loop(false, num_decrement);
        one_loop_done.acquire();
    }
    assert(value == num_increment - num_decrement);
    done = true;
    sem.release();
    t.join();
}

/*#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>*/


// code from https://github.com/goldshtn/shmemq-blog
template<typename Mutex>
struct alignas(64) shmemq
{
    shmemq(unsigned long max_count, unsigned int element_size)
        : element_size(element_size)
        , max_size(max_count * element_size)
    {
        data.reset(new char[max_size]);
    }

    bool try_enqueue(void* element, size_t len)
    {
        if (len != element_size)
            return false;

        std::lock_guard<Mutex> lock(mutex);

        if (write_index - read_index == max_size)
            return false; // There is no more room in the queue

        memcpy(data.get() + write_index % max_size, element, len);
        write_index += element_size;
        return true;
    }

    bool try_dequeue(void* element, size_t len)
    {
        if (len != element_size)
            return false;

        std::lock_guard<Mutex> lock(mutex);

        if (read_index == write_index)
            return false; // There are no elements that haven't been consumed yet

        memcpy(element, data.get() + read_index % max_size, len);
        read_index += element_size;
        return true;
    }

    Mutex mutex;
    size_t element_size = 0;
    size_t max_size = 0;
    size_t read_index = 0;
    size_t write_index = 0;
    std::unique_ptr<char[]> data;
};

template<typename T, typename State>
void BenchmarkShmemq(State& state)
{
    static constexpr int QUEUE_SIZE = 1000;
    static constexpr int REPETITIONS = 10000;
    int DATA_SIZE = state.range(0);

    shmemq<T> server_queue(QUEUE_SIZE, DATA_SIZE);
    shmemq<T> client_queue(QUEUE_SIZE, DATA_SIZE);

    auto build_message = [&]
    {
        std::unique_ptr<char[]> result(new char[DATA_SIZE]);
        memset(result.get(), 0, DATA_SIZE);
        int forty_two = 42;
        assert(sizeof(forty_two) <= static_cast<size_t>(DATA_SIZE));
        memcpy(result.get(), &forty_two, sizeof(forty_two));
        assert(5 + sizeof(forty_two) <= static_cast<size_t>(DATA_SIZE));
        memcpy(result.get() + sizeof(forty_two), "Hello", 5);
        return result;
    };

    auto server = [&]
    {
        std::unique_ptr<char[]> msg = build_message();
        for (int i = 0; i < REPETITIONS; ++i)
        {
            while (!server_queue.try_dequeue(msg.get(), DATA_SIZE))
            {
            }
            while (!client_queue.try_enqueue(msg.get(), DATA_SIZE))
            {
            }
        }
    };
    auto client = [&]
    {
        std::unique_ptr<char[]> msg = build_message();
        for (int i = 0; i < REPETITIONS; ++i)
        {
            while (!server_queue.try_enqueue(msg.get(), DATA_SIZE))
            {
            }
            while (!client_queue.try_dequeue(msg.get(), DATA_SIZE))
            {
            }
        }
    };
    while (state.KeepRunning())
    {
        std::thread s(server);
        std::thread c(client);
        s.join();
        c.join();
    }
    state.SetItemsProcessed(REPETITIONS * state.iterations());
}

RegisterBenchmarkWithAllMutexes(BenchmarkShmemq, ->Arg(256));
RegisterBenchmarkWithAllMutexes(BenchmarkDemingWS);
RegisterBenchmarkWithAllMutexes(BenchmarkThroughputMultipleMutex, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkThroughput, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkContendedMutex, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkLongestIdle, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkLongestWait, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkContendedMutexMoreWork, ->Apply(CustomBenchmarkArguments));
RegisterBenchmarkWithAllMutexes(BenchmarkContendedMutexMoreIdle, ->Apply(CustomBenchmarkArguments));
void RegisterMutex() {}
#endif
#else
void RegisterMutex() {}
#endif
