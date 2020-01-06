#include "benchmark/benchmark.h"
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <vector>
#include <atomic>

void benchmark_futex_wake(benchmark::State & state)
{
    uint32_t futex = 0;
    while (state.KeepRunning())
    {
        syscall(SYS_futex, &futex, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    }
    /* without size check:
benchmark_futex_wake_median                 80 ns         80 ns    8155790
*/

    /* with size check:
benchmark_futex_wake_median                 81 ns         81 ns    8437751
*/
}
BENCHMARK(benchmark_futex_wake);

void benchmark_futex_wake_sleeper(benchmark::State & state)
{
    int num_futexes = std::thread::hardware_concurrency();
    std::vector<std::atomic<uint32_t>> futexes(num_futexes);
    for (auto & f : futexes)
        f = 0;
    std::vector<std::thread> background_threads;
    background_threads.reserve(num_futexes);
    while (background_threads.size() != background_threads.capacity())
    {
        background_threads.emplace_back([&, index = background_threads.size()]
        {
            std::atomic<uint32_t> & futex = futexes[index];
            while (futex == 0)
            {
                syscall(SYS_futex, &futex, FUTEX_WAIT_PRIVATE, 0, nullptr, nullptr, 0);
            }
        });
    }
    int i = 0;
    while (state.KeepRunning())
    {
        syscall(SYS_futex, &futexes[i], FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
        ++i;
        if (i == num_futexes)
            i = 0;
    }
    for (auto & f : futexes)
    {
        f = 1;
        syscall(SYS_futex, &f, FUTEX_WAKE_PRIVATE, static_cast<int>(background_threads.size()), nullptr, nullptr, 0);
    }
    for (std::thread & t : background_threads)
        t.join();
    /* without size check:
benchmark_futex_wake_sleeper_median       1408 ns       1408 ns     558732
*/

    /* with size check:
benchmark_futex_wake_sleeper_median       1409 ns       1408 ns     604738
*/
}
BENCHMARK(benchmark_futex_wake_sleeper);
