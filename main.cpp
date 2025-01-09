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
#include "math/halton_sequence.hpp"

thread_local std::mt19937_64 global_randomness(random_seed_seq::get_instance());

void RunOne(skb::BenchmarkResults & benchmark_data, int argument, bool profile_mode)
{
    std::string message = benchmark_data.categories->CategoriesString();
    message += '/';
    message += std::to_string(argument);
    message += ": ";
    std::cout << message;
    std::cout.flush();
    skb::BenchmarkResults::RunAndBaselineResults result = benchmark_data.Run(argument, profile_mode ? skb::BenchmarkResults::ProfileMode : skb::BenchmarkResults::SeparateProcess);
    //skb::BenchmarkResults::RunAndBaselineResults result = profile_mode ? benchmark_data.Run(argument, 10.0f) : benchmark_data.RunAndAddResults(argument);

    double nanoseconds = result.results.time.count() / static_cast<double>(result.results.num_iterations);
    message = std::to_string(static_cast<int64_t>(nanoseconds));

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
                           "filename TEXT NOT NULL, "
                           "categories TEXT UNIQUE)").step());
    RAW_VERIFY(!db.prepare("CREATE TABLE IF NOT EXISTS results "
                           "(benchmark INTEGER, "
                            "num_iterations INTEGER, "
                            "argument INTEGER, "
                            "time INTEGER, "
                            "num_items_processed INTEGER, "
                            "num_bytes_used INTEGER)").step());
    RAW_VERIFY(!db.prepare("CREATE INDEX IF NOT EXISTS results_benchmark_index "
                           "ON results (benchmark)").step());
    RAW_VERIFY(!db.prepare("CREATE INDEX IF NOT EXISTS benchmark_categories_index "
                           "ON benchmarks (categories)").step());
    RAW_VERIFY(!db.prepare("CREATE INDEX IF NOT EXISTS benchmark_filename_index "
                           "ON benchmarks (filename)").step());
}

int GetBenchmarkId(Database & db, const skb::BenchmarkCategories & categories)
{
    SqLiteStatement get_id = db.prepare("SELECT id FROM benchmarks WHERE categories = ?1");
    get_id.bind(1, categories.CategoriesString());
    if (!get_id.step())
        return -1;
    int result = get_id.GetInt(0);
    RAW_VERIFY(!get_id.step());
    return result;
}

int AddBenchmark(Database & db, interned_string executable, const skb::BenchmarkCategories & categories)
{
    SqLiteStatement statement = db.prepare("INSERT INTO benchmarks (filename, categories) VALUES (?1, ?2)");
    statement.bind(1, executable.view());
    statement.bind(2, categories.CategoriesString());
    RAW_VERIFY(!statement.step());

    return GetBenchmarkId(db, categories);
}
void AddResult(Database & db, int benchmark_id, const skb::RunResults & result)
{
    SqLiteStatement statement = db.prepare("INSERT INTO results (benchmark, num_iterations, argument, time, num_items_processed, num_bytes_used) VALUES(?1, ?2, ?3, ?4, ?5, ?6)");
    statement.bind(1, benchmark_id);
    statement.bind(2, result.num_iterations);
    statement.bind(3, result.argument);
    statement.bind(4, result.time.count());
    statement.bind(5, static_cast<int64_t>(result.num_items_processed));
    statement.bind(6, static_cast<int64_t>(result.num_bytes_used));
    RAW_VERIFY(!statement.step());
}

static constexpr size_t NumBenchmarksToKeep = 64;

void load_from_db(Database & db, interned_string filename)
{
    CreateTables(db);
    SqLiteStatement statement = db.prepare("SELECT num_iterations, argument, time, num_items_processed, num_bytes_used FROM results WHERE benchmark = ?1");
    // Up next: need to re-run this whenever I load results from an executable. Also don't delete results from
    // DB on save. Or maybe only delete results for the currently loaded file.
    for (auto & [categories, results] : skb::Benchmark::AllBenchmarks())
    {
        if (results.executable != filename)
            continue;
        int benchmark_id = GetBenchmarkId(db, categories);
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
            auto & subgroup = results.results[argument];
            skb::RunResults result =
            {
                num_iterations,
                argument,
                std::chrono::nanoseconds(time),
                static_cast<size_t>(num_items_processed),
                static_cast<size_t>(num_bytes_used)
            };
            subgroup.push_back(std::move(result));
            CHECK_FOR_PROGRAMMER_ERROR(subgroup.size() <= NumBenchmarksToKeep);
        }
        for (auto & subgroup : results.results)
        {
            skb::BenchmarkResults::MoveMedianToFront(subgroup.second);
        }
        statement.reset();
    }
}

void persist_to_db(Database & db)
{
    ska::flat_hash_set<interned_string> all_filenames;
    for (const auto &[_, results] : skb::Benchmark::AllBenchmarks()) {
        all_filenames.insert(results.executable);
    }
    SqLiteStatement begin_transaction = db.prepare("BEGIN");
    SqLiteStatement end_transaction = db.prepare("END");
    RAW_VERIFY(!begin_transaction.step());
    SqLiteStatement delete_old_results = db.prepare(
        "DELETE FROM results "
        "WHERE benchmark IN ( "
        "   SELECT id "
        "   FROM benchmarks "
        "   WHERE filename = ?1 "
        ")");
    SqLiteStatement delete_old_benchmarks = db.prepare("DELETE FROM benchmarks WHERE filename = ?1");
    for (interned_string filename : all_filenames) {
        delete_old_results.bind(1, filename.view());
        RAW_VERIFY(!delete_old_results.step());
        delete_old_results.reset();
        delete_old_benchmarks.bind(1, filename.view());
        RAW_VERIFY(!delete_old_benchmarks.step());
        delete_old_benchmarks.step();
    }
    for (auto & [categories, results] : skb::Benchmark::AllBenchmarks())
    {
        int benchmark_id = AddBenchmark(db, results.executable, categories);
        for (auto & group : results.results)
        {
            if (group.second.size() > NumBenchmarksToKeep)
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
                while (group.second.size() > NumBenchmarksToKeep);
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
                            "checked INTEGER)").step());
    std::map<interned_string, std::map<interned_string, bool, interned_string::pointer_less>, interned_string::pointer_less> state;
    SqLiteStatement statement = db.prepare("SELECT category, checkbox, checked FROM checkbox_state");
    while (statement.step())
    {
        interned_string category(statement.GetString(0));
        interned_string checkbox(statement.GetString(1));
        int checked = statement.GetInt(2);
        state[category][checkbox] = checked != 0;
    }
    root.SetCheckboxState(state);
}
void write_checkbox_state(BenchmarkMainGui & root, Database & db)
{
    RAW_VERIFY(!db.prepare("DELETE FROM checkbox_state").step());
    SqLiteStatement statement = db.prepare("INSERT INTO checkbox_state (category, checkbox, checked) VALUES(?1, ?2, ?3)");
    std::map<interned_string, std::map<interned_string, bool, interned_string::pointer_less>, interned_string::pointer_less> checkbox_state = root.GetCheckboxState();
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

#ifndef FUZZER_BUILD
int main(int argc, char * argv[])
{
    if (skb::RunSingleBenchmarkFromCommandLine(argc, argv))
        return 0;

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    if (result)
        return result;

#if 0
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::RunSpecifiedBenchmarks())
        return 0;
#endif

    QApplication app(argc, argv);

    const char * filename = "../benchmark_timings.db";
#ifdef DEBUG_BUILD
    filename = "../benchmark_timings_debug.db";
#endif

    Database permanent_storage(filename);

    BenchmarkMainGui root;

#ifdef __clang__
    std::string current_compiler = "clang";
#elif defined(__GNUC__) || defined(__GNUG__)
    std::string current_compiler = "gcc";
#endif

    std::atomic<bool> keep_running(true);
    std::mutex results_mutex;
    std::mutex run_first_mutex;
    std::deque<std::pair<skb::BenchmarkResults *, int>> run_argument_first;

    QObject::connect(&root, &BenchmarkMainGui::NewFileLoaded, &root, [&](interned_string filename)
    {
        std::lock_guard<std::mutex> lock(results_mutex);
        load_from_db(permanent_storage, filename);
    });
    QObject::connect(&root.GetGraph(), &BenchmarkGraph::RunBenchmarkFirst, &root, [&](skb::BenchmarkResults * benchmark, int argument)
    {
        std::lock_guard<std::mutex> lock(run_first_mutex);
        run_argument_first.push_back({ benchmark, argument });
    });

    std::thread benchmark_thread([&]
    {
        auto can_run = [&](skb::BenchmarkResults * results)
        {
            return results->categories->GetCompiler() == current_compiler;
        };

        auto get_next_to_run = [&]
        {
            {
                std::lock_guard<std::mutex> lock(run_first_mutex);
                while (!run_argument_first.empty() && !can_run(run_argument_first.front().first))
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
            skb::BenchmarkResults * min_result = nullptr;
            int min_argument = 0;
            size_t min_size = NumBenchmarksToKeep + 12;
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
            else
                return std::pair<skb::BenchmarkResults *, int>(nullptr, 0);
        };

        while (keep_running)
        {
            std::unique_lock<std::mutex> lock(results_mutex);
            std::pair<skb::BenchmarkResults *, int> next = get_next_to_run();
            if (next.first) {
                RunOne(*next.first, next.second, root.ProfileMode());
            }
            else {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
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
    RAW_VERIFY(!permanent_storage.prepare("VACUUM").step());

    return 0;
}
#endif
