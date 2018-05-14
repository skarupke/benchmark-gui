#include <test/include_test.hpp>

#include "benchmark/benchmark.h"

#include <QApplication>

#include <random>
#include <vector>
#include <algorithm>
#include "util/random_seed_seq.hpp"

#include "custom_benchmark/custom_benchmark.h"
#include "custom_benchmark/benchmark_graph.h"
#include <thread>
#include "db/sqlite_wrapper.hpp"
#include "custom_benchmark/main_gui.hpp"
#include "hashtable_benchmarks/lookups.hpp"
#include "hashtable_benchmarks/division.hpp"
#include "math/halton_sequence.hpp"
#include "util/bit_iter.hpp"

// todo: move the end() ptr of flat_hash_map to index -1
// todo: make the bucket sizes for prime numbers be close to 16
// todo: make libdivide be the default prime number policy

static constexpr const size_t num_loops = 10000;

thread_local std::mt19937_64 global_randomness(random_seed_seq::get_instance());

static constexpr size_t memory_benchmark_multiplier = 16;

void benchmark_memory_access(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::vector<size_t> bytes(num_bytes);
    std::iota(bytes.begin(), bytes.end(), size_t(100));
    std::uniform_int_distribution<size_t> random_index(0, num_bytes - 1);
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
            skb::DoNotOptimize(bytes[random_index(global_randomness)]);
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}
void benchmark_memory_access_permutation(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::vector<size_t> bytes(num_bytes);
    {
        std::vector<size_t> offsets(num_bytes);
        std::iota(offsets.begin(), offsets.end(), size_t());
        std::shuffle(offsets.begin(), offsets.end(), global_randomness);
        for (size_t i = 0; i < num_bytes; ++i)
        {
            size_t other_index = i == 0 ? num_bytes - 1 : i - 1;
            size_t & found = bytes[offsets[i]];
            CHECK_FOR_PROGRAMMER_ERROR(found == 0);
            found = offsets[other_index];
        }
    }
    size_t index = bytes[0];
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
        {
            index = bytes[index];
            skb::DoNotOptimize(index);
        }
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}

#if 1
void benchmark_predictable_memory_access(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::vector<size_t> bytes(num_bytes);
    std::iota(bytes.begin(), bytes.end(), size_t(100));
    std::uniform_int_distribution<size_t> random_index(0, num_bytes - 1);
    constexpr size_t num_indices = 16 * 1024 * 1024;
    std::vector<size_t> indices(num_indices);
    for (size_t & index : indices)
        index = random_index(global_randomness);
    size_t current_index = 0;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
        {
            skb::DoNotOptimize(bytes[indices[current_index++]]);
            if (current_index == num_indices)
                current_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}
#else
std::vector<size_t> random_bytes(size_t num)
{
    std::vector<size_t> result(num / sizeof(size_t));
    std::iota(result.begin(), result.end(), size_t(100));
    return result;
}
// version for the slides
void benchmark_cache_miss(benchmark::State & state)
{
    std::vector<size_t> bytes = random_bytes(state.range(0));
    std::vector<size_t> indices = random_indices(bytes.size());
    size_t current_index = 0;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < 10000; ++i)
        {
            size_t random_index = indices[current_index++];
            benchmark::DoNotOptimize(bytes[random_index]);
            if (current_index == indices.size())
                current_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
void benchmark_cache_miss(benchmark::State & state)
{
    std::vector<size_t> bytes = random_bytes(state.range(0));


    while (state.KeepRunning())
    {
        for (size_t i = 0; i < 10000; ++i)
        {
            size_t random_index = indices[current_index++];
            benchmark::DoNotOptimize(bytes[random_index]);
            if (current_index == indices.size())
                current_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
void benchmark_cache_miss(benchmark::State & state)
{
    std::vector<size_t> bytes = random_bytes(state.range(0));
    std::mt19937_64 randomness;
    std::uniform_int_distribution<size_t> distribution(0, bytes.size() - 1);
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < 10000; ++i)
        {
            size_t random_index = indices[current_index++];
            benchmark::DoNotOptimize(bytes[random_index]);
            if (current_index == indices.size())
                current_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
void benchmark_cache_miss(benchmark::State & state)
{
    std::vector<size_t> bytes = random_bytes(state.range(0));
    std::mt19937_64 randomness;
    std::uniform_int_distribution<size_t> distribution(0, bytes.size() - 1);
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < 10000; ++i)
        {

            benchmark::DoNotOptimize(bytes[random_index]);


        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
void benchmark_cache_miss(benchmark::State & state)
{
    std::vector<size_t> bytes = random_bytes(state.range(0));
    std::mt19937_64 randomness;
    std::uniform_int_distribution<size_t> distribution(0, bytes.size() - 1);
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < 10000; ++i)
        {
            size_t random_index = distribution(randomness);
            benchmark::DoNotOptimize(bytes[random_index]);


        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
#endif
void benchmark_sequential_memory_access(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::vector<size_t> bytes(num_bytes);
    std::iota(bytes.begin(), bytes.end(), size_t(100));
    while (state.KeepRunning())
    {
        for (size_t i : bytes)
            skb::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations() * num_bytes);
}
void benchmark_sequential_memory_access_baseline(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_bytes; ++i)
            skb::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations() * num_bytes);
}

void benchmark_memory_access_baseline(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::uniform_int_distribution<size_t> random_index(0, num_bytes - 1);

    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
            skb::DoNotOptimize(random_index(global_randomness));
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}
void benchmark_predictable_memory_access_baseline(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    std::uniform_int_distribution<size_t> random_index(0, num_bytes - 1);
    constexpr size_t num_indices = 16 * 1024 * 1024;
    std::vector<size_t> indices(num_indices);
    for (size_t & index : indices)
        index = random_index(global_randomness);
    size_t current_index = 0;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
        {
            skb::DoNotOptimize(indices[current_index++]);
            if (current_index == num_indices)
                current_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}
void benchmark_memory_access_permutation_baseline(skb::State & state)
{
    size_t num_bytes = state.range(0) / sizeof(size_t);
    num_bytes *= memory_benchmark_multiplier;
    while (state.KeepRunning())
    {
        for (size_t i = 0; i < num_loops; ++i)
        {
            skb::DoNotOptimize(i);
        }
    }
    state.SetItemsProcessed(state.iterations() * num_loops);
}

static constexpr int memory_access_min = 2048 / memory_benchmark_multiplier;
static constexpr int memory_access_max = 4 * 1024 * 1024 * (1024 / memory_benchmark_multiplier);
SKA_BENCHMARK("baseline", benchmark_memory_access_baseline);
SKA_BENCHMARK("baseline", benchmark_predictable_memory_access_baseline);
SKA_BENCHMARK("baseline", benchmark_sequential_memory_access_baseline);
SKA_BENCHMARK("baseline", benchmark_memory_access_permutation_baseline);
SKA_BENCHMARK("memory access", benchmark_memory_access)->SetBaseline("benchmark_memory_access_baseline")->SetRange(memory_access_min, memory_access_max)->SetRangeMultiplier(2.0);
SKA_BENCHMARK("memory access", benchmark_memory_access_permutation)->SetBaseline("benchmark_memory_access_permutation_baseline")->SetRange(memory_access_min, memory_access_max)->SetRangeMultiplier(2.0);
SKA_BENCHMARK("memory access", benchmark_predictable_memory_access)->SetBaseline("benchmark_predictable_memory_access_baseline")->SetRange(memory_access_min, memory_access_max)->SetRangeMultiplier(2.0);
SKA_BENCHMARK("memory access", benchmark_sequential_memory_access)->SetBaseline("benchmark_sequential_memory_access_baseline")->SetRange(memory_access_min, memory_access_max)->SetRangeMultiplier(2.0);

void RunOne(skb::BenchmarkResults & benchmark_data, int argument, bool profile_mode)
{
    skb::BenchmarkResults::RunAndBaselineResults result = profile_mode ? benchmark_data.Run(argument, 10.0f) : benchmark_data.RunAndAddResults(argument);

    std::string message = benchmark_data.categories->CategoriesString();
    message += '/';
    message += std::to_string(argument);
    message += ": ";
    double nanoseconds = result.results.time.count() / static_cast<double>(result.results.num_iterations);
    message += std::to_string(static_cast<int64_t>(nanoseconds));

    if (result.baseline_results)
    {
        message += " baseline: ";
        double baseline_nanoseonds = result.baseline_results->time.count() / static_cast<double>(result.baseline_results->num_iterations);
        message += std::to_string(static_cast<int64_t>(baseline_nanoseonds));
        message += " diff: ";
        message += std::to_string(static_cast<int64_t>(nanoseconds - baseline_nanoseonds));
    }

    if (result.results.num_items_processed)
    {
        double nanoseconds_per_item = result.results.GetNanosecondsPerItem(nullptr);
        message += " per item: ";
        if (result.baseline_results)
            nanoseconds_per_item -= result.baseline_results->GetNanosecondsPerItem(nullptr);
        message += std::to_string(nanoseconds_per_item);
        message += "ns";
    }

    std::cout << message << std::endl;
}

void CreateTables(Database & db)
{
    RAW_VERIFY(!db.prepare("CREATE TABLE IF NOT EXISTS benchmarks "
                           "(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "categories TEXT UNIQUE)").first.step());
    RAW_VERIFY(!db.prepare("CREATE TABLE IF NOT EXISTS results "
                           "(benchmark INTEGER, "
                            "num_iterations INTEGER, "
                            "argument INTEGER, "
                            "time INTEGER, "
                            "num_items_processed INTEGER, "
                            "num_bytes_used INTEGER)").first.step());
    RAW_VERIFY(!db.prepare("CREATE INDEX IF NOT EXISTS results_benchmark_index "
                           "ON results (benchmark)").first.step());
}

int GetBenchmarkId(Database & db, const skb::BenchmarkCategories & categories)
{
    SqLiteStatement get_id = db.prepare("SELECT id FROM benchmarks WHERE categories = ?1").first;
    get_id.bind(1, categories.CategoriesString());
    if (!get_id.step())
        return -1;
    int result = get_id.GetInt(0);
    RAW_VERIFY(!get_id.step());
    return result;
}

int AddBenchmark(Database & db, const skb::BenchmarkCategories & categories)
{
    SqLiteStatement statement = db.prepare("INSERT OR IGNORE INTO benchmarks (categories) VALUES (?1)").first;
    statement.bind(1, categories.CategoriesString());
    RAW_VERIFY(!statement.step());

    return GetBenchmarkId(db, categories);
}
void AddResult(Database & db, int benchmark_id, const skb::RunResults & result)
{
    SqLiteStatement statement = db.prepare("INSERT INTO results (benchmark, num_iterations, argument, time, num_items_processed, num_bytes_used) VALUES(?1, ?2, ?3, ?4, ?5, ?6)").first;
    statement.bind(1, benchmark_id);
    statement.bind(2, result.num_iterations);
    statement.bind(3, result.argument);
    statement.bind(4, result.time.count());
    statement.bind(5, static_cast<int64_t>(result.num_items_processed));
    statement.bind(6, static_cast<int64_t>(result.num_bytes_used));
    RAW_VERIFY(!statement.step());
}

void load_from_db(Database & db)
{
    CreateTables(db);
    SqLiteStatement statement = db.prepare("SELECT num_iterations, argument, time, num_items_processed, num_bytes_used FROM results WHERE benchmark = ?1").first;
    for (auto & benchmark : skb::Benchmark::AllBenchmarks())
    {
        int benchmark_id = GetBenchmarkId(db, *benchmark.second.categories);
        if (benchmark_id == -1)
            continue;
        statement.bind(1, benchmark_id);
        while (statement.step())
        {
            int num_iterations = statement.GetInt(0);
            int argument = statement.GetInt(1);
            int64_t time = statement.GetInt64(2);
            int64_t num_items_processed = statement.GetInt64(3);
            int64_t num_bytes_used = statement.GetInt64(4);
            auto & subgroup = benchmark.second.results[argument];
            skb::RunResults result =
            {
                num_iterations,
                argument,
                std::chrono::nanoseconds(time),
                static_cast<size_t>(num_items_processed),
                static_cast<size_t>(num_bytes_used)
            };
            subgroup.push_back(std::move(result));
        }
        for (auto & subgroup : benchmark.second.results)
        {
            skb::BenchmarkResults::MoveMedianToFront(subgroup.second);
        }
        statement.reset();
    }
}

void persist_to_db(Database & db)
{
    SqLiteStatement begin_transaction = db.prepare("BEGIN").first;
    SqLiteStatement end_transaction = db.prepare("END").first;
    RAW_VERIFY(!begin_transaction.step());
    SqLiteStatement delete_old_results = db.prepare("DELETE FROM results").first;
    SqLiteStatement delete_old_benchmarks = db.prepare("DELETE FROM benchmarks").first;
    RAW_VERIFY(!delete_old_results.step());
    RAW_VERIFY(!delete_old_benchmarks.step());
    for (auto & benchmark : skb::Benchmark::AllBenchmarks())
    {
        int benchmark_id = AddBenchmark(db, *benchmark.second.categories);
        for (auto & group : benchmark.second.results)
        {
            if (group.second.size() > 64)
            {
                std::sort(group.second.begin(), group.second.end(), [](const skb::RunResults & a, const skb::RunResults & b)
                {
                    return a.GetNanosecondsPerItem(nullptr) < b.GetNanosecondsPerItem(nullptr);
                });
                do
                {
                    group.second.pop_back();
                    group.second.erase(group.second.begin());
                }
                while (group.second.size() > 64);
            }
            for (const skb::RunResults & result : group.second)
            {
                AddResult(db, benchmark_id, result);
            }
        }
    }
    RAW_VERIFY(!end_transaction.step());
}

void read_checkbox_state(BenchmarkMainGui & root, Database & db)
{
    RAW_VERIFY(!db.prepare("CREATE TABLE IF NOT EXISTS checkbox_state "
                           "(category TEXT, "
                            "checkbox TEXT, "
                            "checked INTEGER)").first.step());
    std::map<std::string, std::map<std::string, bool>> state;
    SqLiteStatement statement = db.prepare("SELECT category, checkbox, checked FROM checkbox_state").first;
    while (statement.step())
    {
        const char * category = statement.GetString(0);
        const char * checkbox = statement.GetString(1);
        int checked = statement.GetInt(2);
        state[category][checkbox] = checked != 0;
    }
    root.SetCheckboxState(state);
}
void write_checkbox_state(BenchmarkMainGui & root, Database & db)
{
    RAW_VERIFY(!db.prepare("DELETE FROM checkbox_state").first.step());
    SqLiteStatement statement = db.prepare("INSERT INTO checkbox_state (category, checkbox, checked) VALUES(?1, ?2, ?3)").first;
    std::map<std::string, std::map<std::string, bool>> checkbox_state = root.GetCheckboxState();
    for (auto & category : checkbox_state)
    {
        for (auto & checkbox : category.second)
        {
            statement.bind(1, category.first);
            statement.bind(2, checkbox.first);
            statement.bind(3, static_cast<int>(checkbox.second));
            RAW_VERIFY(!statement.step());
            statement.reset();
        }
    }
}

extern void count_num_lookups();
extern void test_iaca(int to_find);

#ifndef FUZZER_BUILD
int main(int argc, char * argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    if (result)
        return result;

#if 0
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
#endif

    test_iaca(5);
    count_num_lookups();

    QApplication app(argc, argv);

    const char * filename = "../benchmark_timings.db";
#ifdef DEBUG_BUILD
    filename = "../benchmark_timings_debug.db";
#endif

    Database permanent_storage(filename);

    RegisterHashtableBenchmarks();
    RegisterDivision();
    RegisterBitIterBenchmarks();
    load_from_db(permanent_storage);

    BenchmarkMainGui root;

#ifdef __clang__
    std::string current_compiler = "clang";
#elif defined(__GNUC__) || defined(__GNUG__)
    std::string current_compiler = "gcc";
#endif

    std::atomic<bool> keep_running(true);
    std::mutex run_first_mutex;
    std::vector<skb::BenchmarkResults *> run_first;
    std::deque<std::pair<skb::BenchmarkResults *, int>> run_argument_first;

    QObject::connect(&root, &BenchmarkMainGui::RunBenchmarkFirst, &root, [&](const std::vector<skb::BenchmarkResults *> & data)
    {
        std::lock_guard<std::mutex> lock(run_first_mutex);
        run_first.insert(run_first.end(), data.begin(), data.end());
    });
    QObject::connect(&root.GetGraph(), &BenchmarkGraph::RunBenchmarkFirst, &root, [&](skb::BenchmarkResults * benchmark, int argument)
    {
        std::lock_guard<std::mutex> lock(run_first_mutex);
        run_argument_first.push_back({ benchmark, argument });
    });

    std::thread benchmark_thread([&]
    {
        std::deque<std::pair<skb::BenchmarkResults *, int>> to_run;

        {
            std::map<size_t, std::vector<std::pair<skb::BenchmarkResults *, int>>> batches;
            for (auto & benchmark : skb::Benchmark::AllBenchmarks())
            {
                if (benchmark.first.GetType() == "baseline" || benchmark.first.GetCompiler() != current_compiler)
                    continue;
                std::vector<int> arguments = shuffle_in_halton_order(benchmark.second.benchmark->GetAllArguments());
                size_t batch_size = 16;
                for (size_t i = 0, end = arguments.size(); i < end;)
                {
                    std::vector<std::pair<skb::BenchmarkResults *, int>> & batch = batches[i];
                    for (size_t batch_end = std::min(i + batch_size, end); i < batch_end; ++i)
                    {
                        batch.emplace_back(&benchmark.second, arguments[i]);
                    }
                }
            }
            for (auto & batch : batches)
            {
                to_run.insert(to_run.end(), batch.second.begin(), batch.second.end());
            }
        }

        std::stable_sort(to_run.begin(), to_run.end(), [](const std::pair<skb::BenchmarkResults *, int> & a, const std::pair<skb::BenchmarkResults *, int> & b)
        {
            return a.first->results[a.second].size() < b.first->results[b.second].size();
        });

        auto can_run = [&](skb::BenchmarkResults * results)
        {
            return results->categories->GetCompiler() == current_compiler;
        };

        auto get_next_to_run = [&]
        {
            {
                std::lock_guard<std::mutex> lock(run_first_mutex);
                if (!run_argument_first.empty())
                {
                    while (!can_run(run_argument_first.front().first))
                        run_argument_first.pop_front();
                    if (!run_argument_first.empty())
                    {
                        if (root.ProfileMode())
                        {
                            while (run_argument_first.size() > 1)
                            {
                                if (can_run(run_argument_first.back().first))
                                    run_argument_first.pop_front();
                                else
                                    run_argument_first.pop_back();
                            }
                            return run_argument_first.front();
                        }
                        else
                        {
                            std::pair<skb::BenchmarkResults *, int> result = run_argument_first.front();
                            run_argument_first.pop_front();
                            return result;
                        }
                    }
                }
                const std::vector<skb::BenchmarkResults *> & visible = root.GetGraph().GetData();
                if (root.ShouldPreferVisible())
                {
                    skb::BenchmarkResults * min_result = nullptr;
                    int min_argument = 0;
                    size_t min_size = 16;
                    int xlimit = root.GetGraph().GetXLimit();
                    for (skb::BenchmarkResults * result : visible)
                    {
                        if (!can_run(result))
                            continue;
                        for (const auto & arg : result->results)
                        {
                            if (xlimit > 0 && arg.first > xlimit)
                                continue;
                            if (arg.second.size() < min_size)
                            {
                                min_size = arg.second.size();
                                min_argument = arg.first;
                                min_result = result;
                            }
                        }
                    }
                    if (min_result)
                        return std::make_pair(min_result, min_argument);
                }
                if (!run_first.empty())
                {
                    std::sort(run_first.begin(), run_first.end());
                    std::stable_partition(to_run.begin(), to_run.end(), [&run_first](const std::pair<skb::BenchmarkResults *, int> & run)
                    {
                        return std::binary_search(run_first.begin(), run_first.end(), run.first);
                    });
                    run_first.clear();
                }
            }
            std::pair<skb::BenchmarkResults *, int> one_argument = to_run.front();
            to_run.pop_front();
            to_run.push_back(one_argument);
            return one_argument;
        };

        while (keep_running)
        {
            std::pair<skb::BenchmarkResults *, int> next = get_next_to_run();
            RunOne(*next.first, next.second, root.ProfileMode());
        }
    });

    read_checkbox_state(root, permanent_storage);
    root.show();
    result = app.exec();
    if (result)
        return result;

    keep_running = false;
    benchmark_thread.join();

    persist_to_db(permanent_storage);
    write_checkbox_state(root, permanent_storage);
    RAW_VERIFY(!permanent_storage.prepare("VACUUM").first.step());

    return 0;
}
#endif
