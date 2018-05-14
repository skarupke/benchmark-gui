#include "util/random_seed_seq.hpp"


#ifndef DISABLE_GTEST
#include "test/include_test.hpp"

TEST(random_seed_seq, mersenne_twister)
{
    random_seed_seq seed_seq;
    std::mt19937_64 randomness(seed_seq);
    int random_number = std::uniform_int_distribution<>(1, 2)(randomness);
    ASSERT_TRUE(random_number == 1 || random_number == 2);
}

static float random_float_0_1()
{
    static thread_local std::mt19937_64 randomness(random_seed_seq::get_instance());
    static thread_local std::uniform_real_distribution<float> distribution;
    return distribution(randomness);
}

TEST(random_seed_seq, zero_one_float)
{
    float random = random_float_0_1();
    ASSERT_TRUE(random >= 0.0f && random < 1.0f);
    random = random_float_0_1();
    ASSERT_TRUE(random >= 0.0f && random < 1.0f);
    random = random_float_0_1();
    ASSERT_TRUE(random >= 0.0f && random < 1.0f);
}

// benchmarks
#if 0

#include "benchmark/benchmark.h"
static void benchmark_random_float_0_1(benchmark::State & state)
{
    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(random_float_0_1());
    }
}
BENCHMARK(benchmark_random_float_0_1);

static float srand_rand_float()
{
    return rand() * (1.0f / RAND_MAX);
}

TEST(random_seed_seq, srand_zero_one_float)
{
    srand(time(nullptr));
    float random = srand_rand_float();
    ASSERT_TRUE(random >= 0.0f && random < 1.0f);
    random = srand_rand_float();
    ASSERT_TRUE(random >= 0.0f && random < 1.0f);
    random = srand_rand_float();
    ASSERT_TRUE(random >= 0.0f && random < 1.0f);
}

static void benchmark_srand_rand(benchmark::State & state)
{
    srand(time(nullptr));
    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(srand_rand_float());
    }
}
BENCHMARK(benchmark_srand_rand);

static void benchmark_empty(benchmark::State & state)
{
    while (state.KeepRunning())
    {
    }
}
BENCHMARK(benchmark_empty);
#endif

#endif
