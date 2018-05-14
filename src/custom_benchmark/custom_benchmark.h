#pragma once

#include <vector>
#include <chrono>
#include <string>
#include <memory>
#include <map>
#include <set>
#include "container/small_vector.hpp"
#include <mutex>
#include <signals/connection.hpp>
#include "benchmark/benchmark.h"

namespace skb
{

extern int global_counter;

template<typename IntType>
inline void DoNotOptimize(IntType && i)
{
    global_counter ^= i;
}
inline void DoNotOptimize(const std::string & str)
{
    if (str.empty())
        global_counter ^= -1;
    else
        global_counter ^= str[0];
}
inline void DoNotOptimize(std::string & str)
{
    DoNotOptimize(const_cast<const std::string &>(str));
}
inline void DoNotOptimize(std::string && str)
{
    DoNotOptimize(str);
}

struct BenchmarkResults;

struct RunResults
{
    int num_iterations;
    int argument;
    std::chrono::nanoseconds time;
    size_t num_items_processed;
    size_t num_bytes_used;

    double GetNanosecondsPerItem(BenchmarkResults * baseline_data) const;
};

struct State
{
    State(int num_iterations, int argument);

    inline bool KeepRunning()
    {
        if (current_iterations)
        {
            if (current_iterations == num_iterations)
            {
                total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start);
                return false;
            }
            ++current_iterations;
        }
        else
        {
            current_iterations = 1;
            start = std::chrono::high_resolution_clock::now();
        }
        return true;
    }

    RunResults GetResults() const
    {
        return { num_iterations, argument, GetTotalTime(), num_items_processed, num_bytes_used };
    }

    std::chrono::nanoseconds GetTotalTime() const
    {
        return total_time - pause_time;
    }

    int iterations() const
    {
        return num_iterations;
    }

    void SetItemsProcessed(size_t value)
    {
        num_items_processed = value;
    }
    size_t GetNumItemsProcessed() const
    {
        return num_items_processed;
    }
    void SetNumBytes(size_t allocated, size_t freed)
    {
        num_bytes_allocated = allocated;
        num_bytes_used = allocated - freed;
    }
    size_t GetNumBytesAllocated() const
    {
        return num_bytes_allocated;
    }
    size_t GetNumBytesUsed() const
    {
        return num_bytes_used;
    }

    int GetArgument() const
    {
        return argument;
    }
    int range(size_t which) const
    {
        if (which == 0)
            return argument;
        else
            return 0;
    }

    void PauseTiming()
    {
        pause_start = std::chrono::high_resolution_clock::now();
    }
    void ResumeTiming()
    {
        pause_time += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - pause_start);
    }

private:
    int current_iterations = 0;
    int num_iterations = 1;
    int argument = 0;
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::nanoseconds total_time;
    std::chrono::high_resolution_clock::time_point pause_start;
    std::chrono::nanoseconds pause_time{0};
    size_t num_items_processed = 0;
    size_t num_bytes_allocated = 0;
    size_t num_bytes_used = 0;
};

struct Benchmark;
struct BenchmarkCategories;

struct BenchmarkResults
{
    static const float default_run_time;

    BenchmarkResults(Benchmark * benchmark)
        : benchmark(benchmark)
    {
    }

    Benchmark * benchmark;
    const BenchmarkCategories * categories = nullptr;
    BenchmarkResults * baseline_results = nullptr;

    int FindGoodNumberOfIterations(int argument, float desired_running_time) const;
    void AddResult(RunResults results);
    void ClearResults();
    static void MoveMedianToFront(std::vector<RunResults> & results);

    struct RunAndBaselineResults
    {
        RunResults results;
        std::unique_ptr<RunResults> baseline_results;
    };

    RunAndBaselineResults Run(int argument, float run_time = default_run_time);
    RunAndBaselineResults RunAndAddResults(int argument, float run_time = default_run_time);

    sig2::Signal<BenchmarkResults *> results_added_signal;

    mutable std::mutex results_mutex;
    std::map<int, std::vector<RunResults>> results;
};

struct BenchmarkCategories
{
    BenchmarkCategories(std::string type, std::string name);

    void AddCategory(std::string category, std::string value);

    bool operator<(const BenchmarkCategories & other) const;

    const std::string & GetName() const;
    const std::string & GetType() const;
    const std::string & GetCompiler() const;
    const std::map<std::string, std::string> & GetCategories() const
    {
        return categories;
    }

    static const std::string & TypeIndex();
    static const std::string & NameIndex();
    static const std::string & CompilerIndex();

    std::string CategoriesString() const;

private:
    std::map<std::string, std::string> categories;
};

struct Benchmark
{
    virtual void Run(State & state) const = 0;
    Benchmark(BenchmarkCategories type);
    virtual ~Benchmark();

    int FindGoodNumberOfIterations(int argument, float desired_running_time) const;

    static std::map<BenchmarkCategories, BenchmarkResults> & AllBenchmarks();
    static std::map<std::string, std::map<std::string, std::set<BenchmarkResults *>>> & AllCategories();

    Benchmark * SetBaseline(std::string name_of_baseline_benchmark);

    Benchmark * SetRange(int range_begin, int range_end)
    {
        this->range_begin = range_begin;
        this->range_end = range_end;
        return this;
    }

    Benchmark * SetRangeMultiplier(double multiplier)
    {
        this->range_multiplier = multiplier;
        return this;
    }

    std::vector<int> GetAllArguments() const;

private:
    std::string baseline;

    int range_begin = 0;
    int range_end = 0;
    double range_multiplier = 0;

    std::vector<skb::BenchmarkResults *> results;

    mutable std::mutex arguments_mutex;
    mutable std::vector<int> all_arguments;

    void AddToAllBenchmarks(const BenchmarkCategories & categories);
};
struct LambdaBenchmark : Benchmark
{
    LambdaBenchmark(std::function<void (State &)> func, BenchmarkCategories categories);
    void Run(State & state) const override;

    std::function<void (State & state)> function;
};
}

#define SKB_CONCAT2(a, b) a ## b
#define SKB_CONCAT(a, b) SKB_CONCAT2(a, b)

#define SKA_BENCHMARK_CATEGORIES(function, categories) (new ::skb::LambdaBenchmark(function, categories))
#define SKA_BENCHMARK_NAME(function, category, name) (new ::skb::LambdaBenchmark(function, { category, name }))
#define SKA_BENCHMARK(category, function_name) static ::skb::Benchmark * SKB_CONCAT(register_benchmark_, __LINE__) = SKA_BENCHMARK_NAME(&function_name, category, #function_name)
