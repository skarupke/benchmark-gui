#include "util/controlled_random.hpp"



#ifndef DISABLE_TESTS
#include "test/include_test.hpp"
#include <thread>
#include <mutex>

TEST(controlled_random, heap_top_updated)
{
    std::mt19937_64 randomness(6);
    std::uniform_int_distribution<int> distribution;

    std::vector<int> heap;

    for (size_t i = 0; i < 50; ++i)
    {
        heap.push_back(distribution(randomness));
        std::push_heap(heap.begin(), heap.end());
        heap.front() = distribution(randomness);
        heap_top_updated(heap.begin(), heap.end());
        ASSERT_TRUE(std::is_heap(heap.begin(), heap.end()));
    }
}

TEST(controlled_random, random_success)
{
    std::mt19937_64 randomness(7);
    constexpr int num_runs = 10000;
    for (float f = 0.0f; f <= 1.0f; f += 0.01f)
    {
        if (f > 0.999f)
            f = 1.0f;
        ControlledRandom controlled_random(f);
        int num_success = 0;
        for (int i = 0; i < num_runs; ++i)
        {
            if (controlled_random.random_success(randomness))
                ++num_success;
        }
        float lower_bound = num_runs * (f - 0.01f);
        float upper_bound = num_runs * (f + 0.01f);
        ASSERT_LE(lower_bound, static_cast<float>(num_success));
        ASSERT_GE(upper_bound, static_cast<float>(num_success));
    }
}

float ChanceOfSuccessCountUp(float c, std::mt19937_64 & randomness)
{
    float current_c = 0.0f;
    int num_success = 0;
    std::uniform_real_distribution<float> distribution;
    int num_tests = 10000000;
    for (int i = 0; i < num_tests; ++i)
    {
        current_c += c;
        if (distribution(randomness) < current_c)
        {
            ++num_success;
            current_c = 0.0f;
        }
    }
    return static_cast<float>(num_success) / num_tests;
}
float ChanceOfSuccessDivide(float c, std::mt19937_64 & randomness)
{
    float current_c = c;
    int num_success = 0;
    std::uniform_real_distribution<float> distribution;
    int num_tests = 10000000;
    for (int i = 0; i < num_tests; ++i)
    {
        if (distribution(randomness) > current_c)
        {
            ++num_success;
            current_c = c;
        }
        else
            current_c *= 0.25f;
    }
    return static_cast<float>(num_success) / num_tests;
}
float ChanceOfSuccessSquare(float c, std::mt19937_64 & randomness)
{
    float current_c = c;
    int num_success = 0;
    std::uniform_real_distribution<float> distribution;
    int num_tests = 10000000;
    for (int i = 0; i < num_tests; ++i)
    {
        if (distribution(randomness) > current_c)
        {
            ++num_success;
            current_c = c;
        }
        else
            current_c *= c;
    }
    return static_cast<float>(num_success) / num_tests;
}

TEST(controlled_random, DISABLED_generate_random_success_numbers)
{
    std::vector<std::thread> threads;

    unsigned num_threads = std::thread::hardware_concurrency();
    std::mutex print_mutex;
    for (unsigned thread_number = 0; thread_number < num_threads; ++thread_number)
    {
        threads.emplace_back([&print_mutex, thread_number, num_threads]
        {
            std::mt19937_64 randomness(53452347);
            for (int i = thread_number + 1; i < 100; i += num_threads)
            {
                float low = 0.0f;
                float high = 1.0f;
                int num_low = 0;
                int num_high = 0;
                for (;;)
                {
                    float mid = 0.5f * (low + high);
                    if (mid == low || mid == high)
                        break;
                    bool adjust_low = false;
                    //if (i <= 50)
                    //    adjust_low = ChanceOfSuccessCountUp(mid, randomness) < (i * 0.01f);
                    //else
                    //    adjust_low = ChanceOfSuccessDivide(mid, randomness)  > (i * 0.01f);
                    adjust_low = ChanceOfSuccessSquare(mid, randomness) > (i * 0.01f);
                    if (adjust_low)
                    {
                        float new_low = (low + mid) * 0.5f;
                        if (new_low == low)
                            break;
                        low = new_low;
                        ++num_low;
                    }
                    else
                    {
                        float new_high = (mid + high) * 0.5f;
                        if (new_high == high)
                            break;
                        high = new_high;
                        ++num_high;
                    }
                }
                char buffer[256];
                sprintf(buffer, "%.9g", 0.5f * (low + high));
                std::lock_guard<std::mutex> lock(print_mutex);
                std::cout << i << ": " << buffer << " (" << num_low << ", " << num_high << ')' << std::endl;
            }
        });
    }
    for (std::thread & thread : threads)
        thread.join();
}

TEST(controlled_random, multiple_choices)
{
    std::mt19937_64 randomness(5);
    WeightedDistribution distribution = { 1.0f, 2.0f, 3.0f, 4.0f };
    distribution.initialize_randomness(randomness);
    std::vector<size_t> num_picks(distribution.num_weights());
    for (int i = 0; i < 10000; ++i)
    {
        ++num_picks[distribution.pick_random(randomness)];
    }
    ASSERT_LE(900, num_picks[0]);
    ASSERT_GE(1100, num_picks[0]);
    ASSERT_LE(1900, num_picks[1]);
    ASSERT_GE(2100, num_picks[1]);
    ASSERT_LE(2900, num_picks[2]);
    ASSERT_GE(3100, num_picks[2]);
    ASSERT_LE(3900, num_picks[3]);
    ASSERT_GE(4100, num_picks[3]);
}

TEST(controlled_random, small_numbers)
{
    std::mt19937_64 randomness(5);
    WeightedDistribution distribution = { 0.1f, 0.2f, 0.3f, 0.4f };
    distribution.initialize_randomness(randomness);
    std::vector<size_t> num_picks(distribution.num_weights());
    for (int i = 0; i < 10000; ++i)
    {
        ++num_picks[distribution.pick_random(randomness)];
    }
    ASSERT_LE(900, num_picks[0]);
    ASSERT_GE(1100, num_picks[0]);
    ASSERT_LE(1900, num_picks[1]);
    ASSERT_GE(2100, num_picks[1]);
    ASSERT_LE(2900, num_picks[2]);
    ASSERT_GE(3100, num_picks[2]);
    ASSERT_LE(3900, num_picks[3]);
    ASSERT_GE(4100, num_picks[3]);
}

TEST(controlled_random, multiple_choices_simple)
{
    std::mt19937_64 randomness(5);
    WeightedDistributionSimple distribution = { 1.0f, 2.0f, 3.0f, 4.0f };
    distribution.initialize_randomness(randomness);
    std::vector<size_t> num_picks(distribution.num_weights());
    for (int i = 0; i < 10000; ++i)
    {
        ++num_picks[distribution.pick_random(randomness)];
    }
    ASSERT_LE(900, num_picks[0]);
    ASSERT_GE(1100, num_picks[0]);
    ASSERT_LE(1900, num_picks[1]);
    ASSERT_GE(2100, num_picks[1]);
    ASSERT_LE(2900, num_picks[2]);
    ASSERT_GE(3100, num_picks[2]);
    ASSERT_LE(3900, num_picks[3]);
    ASSERT_GE(4100, num_picks[3]);
}

TEST(controlled_random, multiple_choices_linear)
{
    std::mt19937_64 randomness(5);
    WeightedDistributionLinear distribution = { 1.0f, 2.0f, 3.0f, 4.0f };
    distribution.initialize_randomness(randomness);
    std::vector<size_t> num_picks(distribution.num_weights());
    for (int i = 0; i < 10000; ++i)
    {
        ++num_picks[distribution.pick_random(randomness)];
    }
    ASSERT_LE(900, num_picks[0]);
    ASSERT_GE(1100, num_picks[0]);
    ASSERT_LE(1900, num_picks[1]);
    ASSERT_GE(2100, num_picks[1]);
    ASSERT_LE(2900, num_picks[2]);
    ASSERT_GE(3100, num_picks[2]);
    ASSERT_LE(3900, num_picks[3]);
    ASSERT_GE(4100, num_picks[3]);
}

TEST(controlled_random, multiple_choices_fixed_point)
{
    std::mt19937_64 randomness(5);
    WeightedDistributionFixedPoint distribution = { 1.0f, 2.0f, 3.0f, 4.0f };
    distribution.initialize_randomness(randomness);
    std::vector<size_t> num_picks(distribution.num_weights());
    for (int i = 0; i < 10000; ++i)
    {
        ++num_picks[distribution.pick_random(randomness)];
    }
    ASSERT_LE(900, num_picks[0]);
    ASSERT_GE(1100, num_picks[0]);
    ASSERT_LE(1900, num_picks[1]);
    ASSERT_GE(2100, num_picks[1]);
    ASSERT_LE(2900, num_picks[2]);
    ASSERT_GE(3100, num_picks[2]);
    ASSERT_LE(3900, num_picks[3]);
    ASSERT_GE(4100, num_picks[3]);
}

TEST(controlled_random, multiple_choices_fixed_point_small_numbers)
{
    std::mt19937_64 randomness(5);
    WeightedDistributionFixedPoint distribution =
    {
        WeightedDistributionFixedPoint::min_weight,
        2.0f * WeightedDistributionFixedPoint::min_weight,
        3.0f * WeightedDistributionFixedPoint::min_weight,
        4.0f * WeightedDistributionFixedPoint::min_weight
    };
    distribution.initialize_randomness(randomness);
    std::vector<size_t> num_picks(distribution.num_weights());
    for (int i = 0; i < 10000; ++i)
    {
        ++num_picks[distribution.pick_random(randomness)];
    }
    ASSERT_LE(900, num_picks[0]);
    ASSERT_GE(1100, num_picks[0]);
    ASSERT_LE(1900, num_picks[1]);
    ASSERT_GE(2100, num_picks[1]);
    ASSERT_LE(2900, num_picks[2]);
    ASSERT_GE(3100, num_picks[2]);
    ASSERT_LE(3900, num_picks[3]);
    ASSERT_GE(4100, num_picks[3]);
}

TEST(controlled_random, multiple_choices_fixed_point_large_numbers)
{
    std::mt19937_64 randomness(5);
    WeightedDistributionFixedPoint distribution =
    {
        WeightedDistributionFixedPoint::max_weight / 4.0f,
        WeightedDistributionFixedPoint::max_weight / 2.0f,
        WeightedDistributionFixedPoint::max_weight * 0.75f,
        WeightedDistributionFixedPoint::max_weight
    };
    distribution.initialize_randomness(randomness);
    std::vector<size_t> num_picks(distribution.num_weights());
    for (int i = 0; i < 10000; ++i)
    {
        ++num_picks[distribution.pick_random(randomness)];
    }
    ASSERT_LE(900, num_picks[0]);
    ASSERT_GE(1100, num_picks[0]);
    ASSERT_LE(1900, num_picks[1]);
    ASSERT_GE(2100, num_picks[1]);
    ASSERT_LE(2900, num_picks[2]);
    ASSERT_GE(3100, num_picks[2]);
    ASSERT_LE(3900, num_picks[3]);
    ASSERT_GE(4100, num_picks[3]);
}

TEST(controlled_random, multiple_choices_linear_fixed_point)
{
    std::mt19937_64 randomness(5);
    WeightedDistributionLinearFixedPoint distribution = { 1.0f, 2.0f, 3.0f, 4.0f };
    distribution.initialize_randomness(randomness);
    std::vector<size_t> num_picks(distribution.num_weights());
    for (int i = 0; i < 10000; ++i)
    {
        ++num_picks[distribution.pick_random(randomness)];
    }
    ASSERT_LE(900, num_picks[0]);
    ASSERT_GE(1100, num_picks[0]);
    ASSERT_LE(1900, num_picks[1]);
    ASSERT_GE(2100, num_picks[1]);
    ASSERT_LE(2900, num_picks[2]);
    ASSERT_GE(3100, num_picks[2]);
    ASSERT_LE(3900, num_picks[3]);
    ASSERT_GE(4100, num_picks[3]);
}

TEST(controlled_random, multiple_choices_blended_fixed_point)
{
    std::mt19937_64 randomness(5);
    WeightedDistributionBlendedFixedPoint distribution = { 1.0f, 2.0f, 3.0f, 4.0f };
    distribution.initialize_randomness(randomness);
    std::vector<size_t> num_picks(distribution.num_weights());
    for (int i = 0; i < 10000; ++i)
    {
        ++num_picks[distribution.pick_random(randomness)];
    }
    ASSERT_LE(900, num_picks[0]);
    ASSERT_GE(1100, num_picks[0]);
    ASSERT_LE(1900, num_picks[1]);
    ASSERT_GE(2100, num_picks[1]);
    ASSERT_LE(2900, num_picks[2]);
    ASSERT_GE(3100, num_picks[2]);
    ASSERT_LE(3900, num_picks[3]);
    ASSERT_GE(4100, num_picks[3]);
}

TEST(controlled_random, multiple_choices_blended_fixed_point_small_numbers)
{
    std::mt19937_64 randomness(5);
    WeightedDistributionBlendedFixedPoint distribution =
    {
        WeightedDistributionBlendedFixedPoint::min_weight,
        2.0f * WeightedDistributionBlendedFixedPoint::min_weight,
        3.0f * WeightedDistributionBlendedFixedPoint::min_weight,
        4.0f * WeightedDistributionBlendedFixedPoint::min_weight
    };
    distribution.initialize_randomness(randomness);
    std::vector<size_t> num_picks(distribution.num_weights());
    for (int i = 0; i < 10000; ++i)
    {
        ++num_picks[distribution.pick_random(randomness)];
    }
    ASSERT_LE(900, num_picks[0]);
    ASSERT_GE(1100, num_picks[0]);
    ASSERT_LE(1900, num_picks[1]);
    ASSERT_GE(2100, num_picks[1]);
    ASSERT_LE(2900, num_picks[2]);
    ASSERT_GE(3100, num_picks[2]);
    ASSERT_LE(3900, num_picks[3]);
    ASSERT_GE(4100, num_picks[3]);
}

TEST(controlled_random, multiple_choices_blended_fixed_point_large_numbers)
{
    std::mt19937_64 randomness(5);
    WeightedDistributionBlendedFixedPoint distribution =
    {
        WeightedDistributionBlendedFixedPoint::max_weight / 4.0f,
        WeightedDistributionBlendedFixedPoint::max_weight / 2.0f,
        WeightedDistributionBlendedFixedPoint::max_weight * 0.75f,
        WeightedDistributionBlendedFixedPoint::max_weight
    };
    distribution.initialize_randomness(randomness);
    std::vector<size_t> num_picks(distribution.num_weights());
    for (int i = 0; i < 10000; ++i)
    {
        ++num_picks[distribution.pick_random(randomness)];
    }
    ASSERT_LE(900, num_picks[0]);
    ASSERT_GE(1100, num_picks[0]);
    ASSERT_LE(1900, num_picks[1]);
    ASSERT_GE(2100, num_picks[1]);
    ASSERT_LE(2900, num_picks[2]);
    ASSERT_GE(3100, num_picks[2]);
    ASSERT_LE(3900, num_picks[3]);
    ASSERT_GE(4100, num_picks[3]);
}

template<typename Randomness>
size_t pick_true_random(const std::vector<float> & weights, Randomness & randomness)
{
    float sum = 0.0f;
    for (float f : weights)
        sum += f;
    float random = std::uniform_real_distribution<float>(0.0f, sum)(randomness);
    size_t result = 0;
    for (size_t end = weights.size() - 1; result < end; ++result)
    {
        random -= weights[result];
        if (random <= 0.0f)
            break;
    }
    return result;
}

TEST(controlled_random, plot_wait_times)
{
    std::mt19937_64 randomness(6);
    //WeightedDistributionSimple distribution = { 1.0f, 2.0f, 3.0f, 4.0f };
    //WeightedDistribution distribution = { 1.0f, 2.0f, 3.0f, 4.0f };
    //WeightedDistributionFixedPoint distribution = { 1.0f, 2.0f, 3.0f, 4.0f };
    WeightedDistributionBlendedFixedPoint distribution = { 1.0f, 2.0f, 3.0f, 4.0f };
    //std::vector<float> distribution = { 1.0f, 2.0f, 3.0f, 4.0f };
    distribution.initialize_randomness(randomness);
    int wait_time = 0;
    int repetition_count = 0;
    std::map<size_t, size_t> wait_times;
    std::map<size_t, size_t> repetition_counts;
    for (int i = 0; i < 1000000; ++i)
    {
        if (distribution.pick_random(randomness) == 2)
        //if (pick_true_random(distribution, randomness) == 2)
        {
            ++wait_times[wait_time];
            wait_time = 0;
            ++repetition_count;
        }
        else
        {
            ++wait_time;
            if (repetition_count)
                ++repetition_counts[repetition_count];
            repetition_count = 0;
        }
    }
    size_t biggest_wait = std::prev(wait_times.end())->first;
    for (size_t i = 0; i <= biggest_wait; ++i)
    {
        std::cout << i << ": " << wait_times[i] << '\n';
    }
    std::cout << '\n';
    size_t longest_repetition = std::prev(repetition_counts.end())->first;
    for (size_t i = 0; i <= longest_repetition; ++i)
    {
        std::cout << i << ": " << repetition_counts[i] << '\n';
    }
    std::cout.flush();
}

#include "custom_benchmark/custom_benchmark.h"
#include "hashtable_benchmarks/benchmark_shared.hpp"

template<typename Distribution>
void BenchmarkDistribution(Distribution distribution, skb::State & state)
{
    std::uniform_real_distribution<float> random_distribution(WeightedDistributionFixedPoint::min_weight, WeightedDistributionFixedPoint::max_weight);
    for (int i = 0, end = state.range(0); i < end; ++i)
    {
        distribution.add_weight(random_distribution(global_randomness));
    }
    while (state.KeepRunning())
    {
        skb::DoNotOptimize(distribution.pick_random(global_randomness));
    }
}

void benchmark_controlled_random(skb::State & state)
{
    WeightedDistribution distribution;
    distribution.use_exponential = false;
    distribution.lerp_from_random_to_deterministic = 0.25f;
    BenchmarkDistribution(distribution, state);
}
void benchmark_controlled_random_simple(skb::State & state)
{
    BenchmarkDistribution(WeightedDistributionSimple(), state);
}
void benchmark_controlled_random_fixed_point(skb::State & state)
{
    BenchmarkDistribution(WeightedDistributionFixedPoint(), state);
}
void benchmark_controlled_random_linear(skb::State & state)
{
    BenchmarkDistribution(WeightedDistributionLinear(), state);
}
void benchmark_controlled_random_linear_fixed_point(skb::State & state)
{
    BenchmarkDistribution(WeightedDistributionLinearFixedPoint(), state);
}
void benchmark_controlled_random_blend_fixed_point(skb::State & state)
{
    BenchmarkDistribution(WeightedDistributionBlendedFixedPoint(), state);
}
void benchmark_empty_baseline(skb::State & state)
{
    while (state.KeepRunning())
    {
    }
}
SKA_BENCHMARK("baseline", benchmark_empty_baseline);

void RegisterControlledRandom()
{
    static constexpr int controlled_random_min = 4;
    static constexpr int controlled_random_max = 1024;
    const double controlled_random_multiplier = std::sqrt(2.0);
    skb::BenchmarkCategories fixed_point("controlled_random", "fixed_point");
    skb::BenchmarkCategories simple("controlled_random", "simple");
    skb::BenchmarkCategories blend("controlled_random", "blend");
    skb::BenchmarkCategories linear("controlled_random", "linear");
    skb::BenchmarkCategories linear_fixed_point("controlled_random", "linear fixed_point");
    skb::BenchmarkCategories blend_fixed_point("controlled_random", "blend fixed_point");
    blend.AddCategory("blend_amount", "25% deterministic");
    blend_fixed_point.AddCategory("blend_amount", "25% deterministic");
    SKA_BENCHMARK_CATEGORIES(&benchmark_controlled_random_fixed_point, fixed_point)->SetBaseline("benchmark_empty_baseline")->SetRange(controlled_random_min, controlled_random_max)->SetRangeMultiplier(controlled_random_multiplier);
    SKA_BENCHMARK_CATEGORIES(&benchmark_controlled_random_simple, simple)->SetBaseline("benchmark_empty_baseline")->SetRange(controlled_random_min, controlled_random_max)->SetRangeMultiplier(controlled_random_multiplier);
    SKA_BENCHMARK_CATEGORIES(&benchmark_controlled_random, blend)->SetBaseline("benchmark_empty_baseline")->SetRange(controlled_random_min, controlled_random_max)->SetRangeMultiplier(controlled_random_multiplier);
    SKA_BENCHMARK_CATEGORIES(&benchmark_controlled_random_linear, linear)->SetBaseline("benchmark_empty_baseline")->SetRange(controlled_random_min, controlled_random_max)->SetRangeMultiplier(controlled_random_multiplier);
    SKA_BENCHMARK_CATEGORIES(&benchmark_controlled_random_linear_fixed_point, linear_fixed_point)->SetBaseline("benchmark_empty_baseline")->SetRange(controlled_random_min, controlled_random_max)->SetRangeMultiplier(controlled_random_multiplier);
    SKA_BENCHMARK_CATEGORIES(&benchmark_controlled_random_blend_fixed_point, blend_fixed_point)->SetBaseline("benchmark_empty_baseline")->SetRange(controlled_random_min, controlled_random_max)->SetRangeMultiplier(controlled_random_multiplier);
}

#endif

