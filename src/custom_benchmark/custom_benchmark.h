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
#include "custom_benchmark/interned_string.hpp"
#include "container/flat_hash_map.hpp"

namespace skb
{

extern int global_counter;
extern float global_float;
extern double global_double;

template<typename IntType>
inline void DoNotOptimize(IntType && i)
{
    global_counter ^= i;
}
inline void DoNotOptimize(float f)
{
    global_float += f;
    global_float *= 0.5f;
}
inline void DoNotOptimize(double d)
{
    global_double += d;
    global_double *= 0.5;
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

    enum RunType
    {
        Normal,
        ProfileMode
    };

    RunAndBaselineResults Run(int argument, RunType run_type);
    RunResults RunInNewProcess(int num_iterations, int argument) const;

    sig2::Signal<BenchmarkResults *> results_added_signal;

    mutable std::mutex results_mutex;
    std::map<int, std::vector<RunResults>> results;

    int my_global_index = -1;
    interned_string executable;
};

struct BenchmarkCategories
{
    BenchmarkCategories(interned_string type, interned_string name);

    void AddCategory(interned_string category, interned_string value);
    BenchmarkCategories AddCategoryCopy(interned_string category, interned_string value) const &;
    BenchmarkCategories AddCategoryCopy(interned_string category, interned_string value) &&;

    bool operator==(const BenchmarkCategories & other) const;
    bool operator<(const BenchmarkCategories & other) const;

    const interned_string & GetName() const;
    const interned_string & GetType() const;
    const interned_string & GetCompiler() const;
    const interned_string & GetOptimizer() const;
    const ska::flat_hash_map<interned_string, interned_string> & GetCategories() const
    {
        return categories;
    }

    static const interned_string & TypeIndex();
    static const interned_string & NameIndex();
    static const interned_string & CompilerIndex();
    static const interned_string & OptimizerIndex();

    std::string CategoriesString() const;

    std::string Serialize() const;
    static BenchmarkCategories Deserialize(std::string_view);

private:
    BenchmarkCategories();
    ska::flat_hash_map<interned_string, interned_string> categories;
};

struct CategoryBuilder
{
    CategoryBuilder AddCategory(interned_string category, interned_string value) const &;
    CategoryBuilder AddCategory(interned_string category, interned_string value) &&;

    skb::BenchmarkCategories BuildCategories(interned_string type, interned_string name) const &;
    skb::BenchmarkCategories BuildCategories(interned_string type, interned_string name) &&;

    ska::flat_hash_map<interned_string, interned_string> categories;
};

struct Benchmark
{
    Benchmark(BenchmarkCategories type);
    Benchmark(BenchmarkCategories type, interned_string executable, int index_in_executable);
    virtual ~Benchmark();

    static std::map<BenchmarkCategories, BenchmarkResults> & AllBenchmarks();
    static ska::flat_hash_map<interned_string, ska::flat_hash_map<interned_string, ska::flat_hash_set<BenchmarkResults *>>> & AllCategories();

    Benchmark * SetBaseline(interned_string name_of_baseline_benchmark);

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

    struct RangeOfArguments {
        int begin;
        int end;
        double multiplier;

        std::string Serialize() const;
        static RangeOfArguments Deserialize(std::string_view);
    };
    RangeOfArguments GetArgumentRange() const;

private:
    interned_string baseline;

    int range_begin = 0;
    int range_end = 0;
    double range_multiplier = 0;

protected:
    skb::BenchmarkResults * results = nullptr;

private:
    mutable std::mutex arguments_mutex;
    mutable std::vector<int> all_arguments;

    void AddToAllBenchmarks(const BenchmarkCategories & categories);
};
struct LambdaBenchmark : Benchmark
{
    LambdaBenchmark(std::function<void (State &)> func, BenchmarkCategories categories);
    void Run(State & state) const;

    std::function<void (State & state)> function;
};
struct BenchmarkInOtherProcess : Benchmark
{
    BenchmarkInOtherProcess(BenchmarkCategories type, Benchmark::RangeOfArguments range, interned_string executable, int index_in_executable);
};

bool RunSingleBenchmarkFromCommandLine(int argc, char * argv[]);
std::optional<std::string> LoadAllBenchmarksFromFile(std::string_view filename);
}

#define SKB_CONCAT2(a, b) a ## b
#define SKB_CONCAT(a, b) SKB_CONCAT2(a, b)

#define SKA_BENCHMARK_CATEGORIES(...) (new ::skb::LambdaBenchmark(__VA_ARGS__))
#define SKA_BENCHMARK_NAME(function, category, name) (new ::skb::LambdaBenchmark(function, { category, name }))
#define SKA_BENCHMARK_REGISTER(category, function_name) SKA_BENCHMARK_NAME(&function_name, category, #function_name)
#define SKA_BENCHMARK(category, function_name) static ::skb::Benchmark * SKB_CONCAT(register_benchmark_, __LINE__) = SKA_BENCHMARK_REGISTER(category, function_name)
