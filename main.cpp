#include <test/include_test.hpp>

#include "benchmark/benchmark.h"

#include <QApplication>

#include <random>
#include <vector>
#include <algorithm>

#include "custom_benchmark/custom_benchmark.h"
#include "custom_benchmark/benchmark_graph.h"
#include <thread>
#include "custom_benchmark/main_gui.hpp"
#include "math/halton_sequence.hpp"
#include "custom_benchmark/profile_mode.hpp"
#include "db/benchmark_db.hpp"
#include "thread/ticket_mutex.hpp"

void RunOne(skb::BenchmarkResults & benchmark_data, int argument, bool profile_mode)
{
    std::string message = benchmark_data.categories->CategoriesString();
    message += '/';
    message += std::to_string(argument);
    message += ": ";
    std::cout << message;
    std::cout.flush();
    skb::BenchmarkResults::RunAndBaselineResults result = benchmark_data.Run(argument, profile_mode ? skb::BenchmarkResults::ProfileMode : skb::BenchmarkResults::Normal);

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

static constexpr size_t NumBenchmarksToKeep = 64;

void load_from_db(BenchmarkDB & db, interned_string filename)
{
    // Up next: need to re-run this whenever I load results from an executable. Also don't delete results from
    // DB on save. Or maybe only delete results for the currently loaded file.
    for (auto & [categories, results] : skb::Benchmark::AllBenchmarks())
    {
        if (results.executable != filename)
            continue;
        int benchmark_id = db.GetBenchmarkId(categories);
        if (benchmark_id == -1)
            continue;
        db.load_result.bind(1, benchmark_id);
        while (db.load_result.step())
        {
            int num_iterations = db.load_result.GetInt(0);
            int argument = db.load_result.GetInt(1);
            int64_t time = db.load_result.GetInt64(2);
            int64_t num_items_processed = db.load_result.GetInt64(3);
            int64_t num_bytes_used = db.load_result.GetInt64(4);
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
        db.load_result.reset();
    }
}

void persist_to_db(BenchmarkDB & db)
{
    ska::flat_hash_set<interned_string> all_filenames;
    for (const auto &[_, results] : skb::Benchmark::AllBenchmarks()) {
        all_filenames.insert(results.executable);
    }
    db.BeginTransaction();
    for (interned_string filename : all_filenames) {
        db.DeleteOldResults(filename);
        db.DeleteOldBenchmarks(filename);
    }
    for (auto & [categories, results] : skb::Benchmark::AllBenchmarks())
    {
        int benchmark_id = db.AddBenchmark(results.executable, categories);
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
                db.AddResult(benchmark_id, result);
            }
        }
    }
    db.EndTransaction();
}

void read_checkbox_state(BenchmarkMainGui & root, BenchmarkDB & db)
{
    std::map<interned_string, std::map<interned_string, bool, interned_string::pointer_less>, interned_string::pointer_less> state;
    while (db.read_checkbox.step())
    {
        interned_string category(db.read_checkbox.GetString(0));
        interned_string checkbox(db.read_checkbox.GetString(1));
        int checked = db.read_checkbox.GetInt(2);
        state[category][checkbox] = checked != 0;
    }
    db.read_checkbox.reset();
    if (root.CheckboxStateMatches(state)) {
        root.SetCheckboxState(state);
    }
}
void write_checkbox_state(BenchmarkMainGui & root, BenchmarkDB & db)
{
    std::map<interned_string, std::map<interned_string, bool, interned_string::pointer_less>, interned_string::pointer_less> checkbox_state = root.GetCheckboxState();
    if (checkbox_state.empty())
    {
        // if we have no checkbox state, don't do anything. Don't even delete the old
        // state. This probably just meant that someone opened and closed the program
        // without loading an executable. In that case it's better to load the checkbox
        // state next time.
        return;
    }
    db.DeleteCheckboxState();
    for (auto & [category, checkboxes] : checkbox_state)
    {
        for (auto & [checkbox, state] : checkboxes)
        {
            db.AddCheckboxState(category, checkbox, state);
        }
    }
}

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

    BenchmarkDB permanent_storage(filename);

    BenchmarkMainGui root;

    std::atomic<bool> keep_running(true);
    ticket_mutex results_mutex;
    std::mutex run_first_mutex;
    std::deque<std::pair<skb::BenchmarkResults *, int>> run_argument_first;

    QObject::connect(&root, &BenchmarkMainGui::NewFileLoaded, &root, [&](interned_string filename)
    {
        std::lock_guard<ticket_mutex> lock(results_mutex);
        load_from_db(permanent_storage, filename);
        read_checkbox_state(root, permanent_storage);
    });
    QObject::connect(&root.GetGraph(), &BenchmarkGraph::RunBenchmarkFirst, &root, [&](skb::BenchmarkResults * benchmark, int argument)
    {
        std::lock_guard<std::mutex> lock(run_first_mutex);
        run_argument_first.push_back({ benchmark, argument });
        skb::DisableProfileMode();
    });

    skb::DisableProfileMode();
    std::thread benchmark_thread([&]
    {
        auto get_next_to_run = [&]
        {
            {
                std::lock_guard<std::mutex> lock(run_first_mutex);
                if (!run_argument_first.empty())
                {
                    if (root.ProfileMode())
                    {
                        if (run_argument_first.size() > 1)
                            run_argument_first.erase(run_argument_first.begin(), run_argument_first.end() - 1);
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
                for (const auto & [argument, repeat_results] : result->results)
                {
                    // todo: why do I ever have this?
                    if (argument <= 0)
                        continue;
                    if (xlimit > 0 && argument > xlimit)
                        continue;
                    if (repeat_results.size() < min_size)
                    {
                        min_size = repeat_results.size();
                        min_argument = argument;
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
            std::unique_lock<ticket_mutex> lock(results_mutex);
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

    root.show();
    result = app.exec();
    if (result)
        return result;

    keep_running = false;
    benchmark_thread.join();

    persist_to_db(permanent_storage);
    write_checkbox_state(root, permanent_storage);

    return 0;
}
