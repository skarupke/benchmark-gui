#pragma once

#include <mutex>
#include <condition_variable>

struct ticket_mutex {
    void lock();
    void unlock();

private:
    std::condition_variable cv;
    std::mutex mutex;
    unsigned counter = 0;
    unsigned next = 0;
};
