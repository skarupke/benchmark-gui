#include "thread/ticket_mutex.hpp"

void ticket_mutex::lock() {
    std::unique_lock<std::mutex> lock(mutex);
    unsigned ticket = next++;
    while (ticket != counter) {
        cv.wait(lock);
    }
}
void ticket_mutex::unlock() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        ++counter;
    }
    cv.notify_all();
}
