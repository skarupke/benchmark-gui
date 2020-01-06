#include "custom_benchmark/custom_benchmark.h"
#include <charconv>
#include <cstring>
#include <string_view>
#include <iostream>
#include <filesystem>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

namespace skb
{
int global_counter = 0;
float global_float = 0.0f;
double global_double = 0.0;

const float BenchmarkResults::default_run_time = 0.25f;

State::State(int num_iterations, int argument)
    : num_iterations(std::max(1, num_iterations))
    , argument(argument)
{
}

const interned_string & BenchmarkCategories::TypeIndex()
{
    static const interned_string result = "type";
    return result;
}
const interned_string & BenchmarkCategories::NameIndex()
{
    static const interned_string result = "name";
    return result;
}
const interned_string & BenchmarkCategories::CompilerIndex()
{
    static const interned_string result = "compiler";
    return result;
}
BenchmarkCategories::BenchmarkCategories(interned_string type, interned_string name)
{
    categories[TypeIndex()] = std::move(type);
    categories[NameIndex()] = std::move(name);
}
const interned_string & BenchmarkCategories::GetName() const
{
    auto found = categories.find(NameIndex());
    CHECK_FOR_PROGRAMMER_ERROR(found != categories.end());
    return found->second;
}
const interned_string & BenchmarkCategories::GetType() const
{
    auto found = categories.find(TypeIndex());
    CHECK_FOR_PROGRAMMER_ERROR(found != categories.end());
    return found->second;
}
const interned_string & BenchmarkCategories::GetCompiler() const
{
    auto found = categories.find(CompilerIndex());
    CHECK_FOR_PROGRAMMER_ERROR(found != categories.end());
    return found->second;
}

void BenchmarkCategories::AddCategory(interned_string category, interned_string value)
{
    CHECK_FOR_PROGRAMMER_ERROR(categories.find(category) == categories.end());
    categories.emplace(std::move(category), std::move(value));
}

struct NestedPointerLess : interned_string::pointer_less
{
    using interned_string::pointer_less::operator();

    template<typename K, typename V>
    bool operator()(const std::pair<K, V> & l, const std::pair<K, V> & r) const
    {
        if ((*this)(l.first, r.first))
            return true;
        else if ((*this)(r.first, l.first))
            return false;
        else
            return (*this)(l.second, r.second);
    }
};

bool BenchmarkCategories::operator<(const BenchmarkCategories & other) const
{
    return std::lexicographical_compare(categories.begin(), categories.end(), other.categories.begin(), other.categories.end(), NestedPointerLess());
}

CategoryBuilder CategoryBuilder::AddCategory(interned_string category, interned_string value) const &
{
    CategoryBuilder result = *this;
    RAW_VERIFY(result.categories.emplace(std::move(category), std::move(value)).second);
    return result;
}
CategoryBuilder CategoryBuilder::AddCategory(interned_string category, interned_string value) &&
{
    CategoryBuilder result = std::move(*this);
    RAW_VERIFY(result.categories.emplace(std::move(category), std::move(value)).second);
    return result;
}

skb::BenchmarkCategories CategoryBuilder::BuildCategories(interned_string type, interned_string name) const &
{
    skb::BenchmarkCategories result(std::move(type), std::move(name));
    for (const auto & category : categories)
    {
        result.AddCategory(category.first, category.second);
    }
    return result;
}
skb::BenchmarkCategories CategoryBuilder::BuildCategories(interned_string type, interned_string name) &&
{
    skb::BenchmarkCategories result(std::move(type), std::move(name));
    for (auto & category : categories)
    {
        result.AddCategory(std::move(category.first), std::move(category.second));
    }
    return result;
}

static const std::vector<interned_string> & AllCompilers()
{
    static std::vector<interned_string> result = { "gcc", "clang" };
    return result;
}

std::map<interned_string, std::vector<BenchmarkResults *>, interned_string::pointer_less> & AllBaselineBenchmarks()
{
    static std::map<interned_string, std::vector<BenchmarkResults *>, interned_string::pointer_less> result;
    return result;
}

Benchmark::Benchmark(BenchmarkCategories categories)
{
    for (const interned_string & compiler : AllCompilers())
    {
        BenchmarkCategories compiler_categories = categories;
        compiler_categories.AddCategory(BenchmarkCategories::CompilerIndex(), compiler);
        AddToAllBenchmarks(compiler_categories);
    }
}

static std::vector<BenchmarkResults *> & AllBenchmarksNumbered()
{
    static std::vector<BenchmarkResults *> result;
    return result;
}

void Benchmark::AddToAllBenchmarks(const BenchmarkCategories & categories)
{
    auto added = AllBenchmarks().emplace(categories, this);
    CHECK_FOR_PROGRAMMER_ERROR(added.second);
    added.first->second.categories = &added.first->first;
    for (const auto & category : categories.GetCategories())
    {
        RAW_VERIFY(AllCategories()[category.first][category.second].insert(&added.first->second).second);
    }
    if (categories.GetType() == "baseline")
    {
        AllBaselineBenchmarks()[categories.GetName()].push_back(&added.first->second);
    }
    added.first->second.my_global_index = AllBenchmarksNumbered().size();
    AllBenchmarksNumbered().push_back(&added.first->second);
    results.push_back(&added.first->second);
}

Benchmark * Benchmark::SetBaseline(interned_string name_of_baseline_benchmark)
{
    for (skb::BenchmarkResults * result : results)
    {
        auto & baselines = AllBaselineBenchmarks();
        auto found = baselines.find(name_of_baseline_benchmark);
        CHECK_FOR_PROGRAMMER_ERROR(found != baselines.end());

        bool found_it = false;
        for (BenchmarkResults * baseline : found->second)
        {
            if (baseline->categories->GetCompiler() != result->categories->GetCompiler())
                continue;

            result->baseline_results = baseline;
            found_it = true;
            break;
        }
        CHECK_FOR_PROGRAMMER_ERROR(found_it);
    }
    return this;
}


std::string BenchmarkCategories::CategoriesString() const
{
    std::vector<interned_string> sorted;
    sorted.reserve(categories.size());
    size_t total_length = GetType().size() + 1 + GetName().size();
    for (const auto & category : categories)
    {
        if (category.first == TypeIndex() || category.first == NameIndex())
            continue;
        sorted.push_back(category.first);
        total_length += category.second.size() + 1;
    }
    std::sort(sorted.begin(), sorted.end(), interned_string::string_less());
    std::string result;
    result.reserve(total_length);
    result += GetType();
    result += '/';
    result += GetName();
    for (const interned_string & category_key : sorted)
    {
        result += '/';
        result += categories.at(category_key);
    }
    CHECK_FOR_PROGRAMMER_ERROR(result.size() == total_length);
    return result;
}

Benchmark::~Benchmark()
{
}

int BenchmarkResults::FindGoodNumberOfIterations(int argument, float desired_running_time) const
{
    auto found = results.find(argument);
    if (found == results.end() || found->second.empty())
        return benchmark->FindGoodNumberOfIterations(argument, desired_running_time);
    auto closest = found->second.begin();
    double closest_time = closest->time.count() / 1000000000.0;
    double num_iterations = closest->num_iterations * (desired_running_time / closest_time);
    num_iterations = std::min(num_iterations, static_cast<double>(std::numeric_limits<int>::max()));
    return static_cast<int>(num_iterations);
}

int Benchmark::FindGoodNumberOfIterations(int argument, float desired_running_time) const
{
    State one_iteration(1, argument);
    Run(one_iteration);
    double in_seconds = one_iteration.GetTotalTime().count() / 1000000000.0;
    double estimated_num_iterations = desired_running_time / in_seconds;
    if (estimated_num_iterations < 2.0)
        return 1.0;
    int rerun_iterations = static_cast<int>(std::min(10000.0, estimated_num_iterations));
    State first_estimate(rerun_iterations, one_iteration.GetArgument());
    Run(first_estimate);
    double estimate_result = first_estimate.GetTotalTime().count() / 1000000000.0;
    double num_iterations = rerun_iterations * (desired_running_time / estimate_result);
    num_iterations = std::min(num_iterations, static_cast<double>(std::numeric_limits<int>::max()));
    return static_cast<int>(num_iterations);
}

std::vector<int> Benchmark::GetAllArguments() const
{
    std::lock_guard<std::mutex> lock(arguments_mutex);
    if (all_arguments.empty())
    {
        if (range_begin == 0 && range_end == 0)
            return {0};
        if (range_begin == range_end)
            return {range_begin};
        double multiplier = range_multiplier;
        if (multiplier <= 0.0)
            multiplier = 8.0;
        for (double i = range_begin; i < range_end; i = std::max(i + 1.0, i * multiplier))
        {
            all_arguments.push_back(static_cast<int>(i));
        }
        if (all_arguments.back() != range_end)
            all_arguments.push_back(range_end);
    }
    return all_arguments;
}

std::map<BenchmarkCategories, BenchmarkResults> & Benchmark::AllBenchmarks()
{
    static std::map<BenchmarkCategories, BenchmarkResults> result;
    return result;
}
ska::flat_hash_map<interned_string, ska::flat_hash_map<interned_string, ska::flat_hash_set<BenchmarkResults *>>> & Benchmark::AllCategories()
{
    static ska::flat_hash_map<interned_string, ska::flat_hash_map<interned_string, ska::flat_hash_set<BenchmarkResults *>>> result;
    return result;
}

void BenchmarkResults::AddResult(RunResults result)
{
    {
        std::lock_guard<std::mutex> lock(results_mutex);
        std::vector<RunResults> & this_results = results[result.argument];
        this_results.push_back(std::move(result));
        MoveMedianToFront(this_results);
    }
    results_added_signal.emit(this);
}
void BenchmarkResults::MoveMedianToFront(std::vector<RunResults> & results)
{
    if (results.empty())
        return;
    auto begin = results.begin();
    auto median = begin + (results.size() - 1) / 2;
    std::nth_element(begin, median, results.end(), [](const RunResults & a, const RunResults & b)
    {
        return a.GetNanosecondsPerItem(nullptr) < b.GetNanosecondsPerItem(nullptr);
    });
    std::iter_swap(begin, median);
}

void BenchmarkResults::ClearResults()
{
    {
        std::lock_guard<std::mutex> lock(results_mutex);
        for (auto & result : results)
            result.second.clear();
    }
    results_added_signal.emit(this);
}

BenchmarkResults::RunAndBaselineResults BenchmarkResults::Run(int argument, float run_time)
{
    int good_number = FindGoodNumberOfIterations(argument, run_time);
    skb::State benchmark_state(good_number, argument);
    benchmark->Run(benchmark_state);

    RunAndBaselineResults result = { benchmark_state.GetResults(), nullptr };
    std::unique_ptr<skb::State> baseline_state;
    if (baseline_results)
    {
        int baseline_good_number = baseline_results->FindGoodNumberOfIterations(argument, default_run_time);
        baseline_state.reset(new skb::State(baseline_good_number, argument));
        baseline_results->benchmark->Run(*baseline_state);
        result.baseline_results.reset(new RunResults(baseline_state->GetResults()));
    }
    return result;
}

BenchmarkResults::RunAndBaselineResults BenchmarkResults::RunAndAddResults(int argument, float run_time)
{
#if RUN_BASELINE_SECOND
    int good_number = FindGoodNumberOfIterations(argument, run_time);
    skb::State benchmark_state(good_number, argument);
    benchmark->Run(benchmark_state);

    RunAndBaselineResults result = { benchmark_state.GetResults(), nullptr };
    std::unique_ptr<skb::State> baseline_state;
    if (baseline_results)
    {
        int baseline_good_number = baseline_results->FindGoodNumberOfIterations(argument, default_run_time);
        baseline_state.reset(new skb::State(baseline_good_number, argument));
        baseline_results->benchmark->Run(*baseline_state);
        result.baseline_results.reset(new RunResults(baseline_state->GetResults()));
        baseline_results->AddResult(*result.baseline_results);
    }
    AddResult(result.results);
    return result;
#else
    std::unique_ptr<skb::State> baseline_state;
    if (baseline_results)
    {
        int baseline_good_number = baseline_results->FindGoodNumberOfIterations(argument, default_run_time);
        baseline_state.reset(new skb::State(baseline_good_number, argument));
        baseline_results->benchmark->Run(*baseline_state);
    }
    int good_number = FindGoodNumberOfIterations(argument, run_time);
    skb::State benchmark_state(good_number, argument);
    benchmark->Run(benchmark_state);

    RunAndBaselineResults result = { benchmark_state.GetResults(), nullptr };
    if (baseline_state)
    {
        result.baseline_results.reset(new RunResults(baseline_state->GetResults()));
        baseline_results->AddResult(*result.baseline_results);
    }
    AddResult(result.results);
    return result;
#endif
}

BenchmarkResults::RunAndBaselineResults BenchmarkResults::Run(int argument, RunType run_type)
{
    bool run_baseline_first = baseline_results && [&]
    {
        std::lock_guard<std::mutex> lock(results_mutex);
        return (results[argument].size() % 2) != 0;
    }();
    RunAndBaselineResults function_result;
    auto run_baseline = [&]
    {
        int baseline_good_number = baseline_results->FindGoodNumberOfIterations(argument, default_run_time);
        if (run_type == SeparateProcess)
        {
            function_result.baseline_results.reset(new RunResults(baseline_results->RunInNewProcess(baseline_good_number, argument)));
        }
        else
        {
            skb::State baseline_state(baseline_good_number, argument);
            baseline_results->benchmark->Run(baseline_state);
            function_result.baseline_results.reset(new RunResults(baseline_state.GetResults()));
        }
    };
    if (run_baseline_first)
        run_baseline();
    int good_number = FindGoodNumberOfIterations(argument, run_type == ProfileMode ? 10.0f : default_run_time);
    if (run_type == SeparateProcess)
        function_result.results = RunInNewProcess(good_number, argument);
    else
    {
        skb::State benchmark_state(good_number, argument);
        benchmark->Run(benchmark_state);
        function_result.results = benchmark_state.GetResults();
    }
    if (baseline_results)
    {
        if (!run_baseline_first)
            run_baseline();
        if (run_type != ProfileMode)
            baseline_results->AddResult(*function_result.baseline_results);
    }
    if (run_type != ProfileMode)
        AddResult(function_result.results);
    return function_result;
}

static std::string my_executable_name;
static std::filesystem::path initial_working_dir;

static constexpr std::string_view subprocess_start_string = "successfully ran ";
static constexpr std::string_view subprocess_num_iterations_string = "\nnum_iterations: ";
static constexpr std::string_view subprocess_time_string = "\ntime: ";
static constexpr std::string_view subprocess_num_items_string = "\nnum_items_processed: ";
static constexpr std::string_view subprocess_num_bytes_string = "\nnum_bytes_used: ";
bool string_starts_with(std::string_view to_check, std::string_view start)
{
    return to_check.size() >= start.size() &&
            std::mismatch(to_check.begin(), to_check.end(), start.begin(), start.end()).second == start.end();
}

template<typename I>
bool StrToInt(std::string_view str, I & to_fill)
{
    std::from_chars_result result = std::from_chars(str.data(), str.data() + str.size(), to_fill);
    return result.ptr != str.data() && result.ptr == str.data() + str.size();
}

RunResults BenchmarkResults::RunInNewProcess(int num_iterations, int argument)
{
    int communication_pipe[2] = { 0, 0 };
    auto check_for_error = [](int result)
    {
        CHECK_FOR_PROGRAMMER_ERROR(result != -1);
    };
    check_for_error(pipe(communication_pipe));
    std::vector<std::string> arguments;
    arguments.push_back(my_executable_name);
    arguments.push_back("--run-benchmark-then-exit");
    arguments.push_back(std::to_string(my_global_index));
    arguments.push_back(std::string(categories->GetName().view()));
    arguments.push_back(std::to_string(argument));
    arguments.push_back(std::to_string(num_iterations));
    std::vector<char *> as_char_pointers;
    as_char_pointers.reserve(arguments.size());
    for (std::string & str : arguments)
        as_char_pointers.push_back(const_cast<char *>(str.c_str()));
    as_char_pointers.push_back(nullptr);
    pid_t pid = fork();
    CHECK_FOR_PROGRAMMER_ERROR(pid != -1);
    if (pid == 0)
    {
        check_for_error(close(communication_pipe[0]));
        check_for_error(dup2(communication_pipe[1], STDOUT_FILENO));
        check_for_error(close(communication_pipe[1]));

        std::filesystem::current_path(initial_working_dir);
        execv(my_executable_name.c_str(), as_char_pointers.data());
        // exec will only return if something goes wrong
        const char * error_str = strerror(errno);
        std::cout << "Erorr: Couldn't launch executable " << my_executable_name << ". Error was " << error_str << std::endl;
        exit(1);
    }
    check_for_error(close(communication_pipe[1]));
    std::string result;
    constexpr int buffer_size = 1024;
    char buffer[buffer_size];
    for (;;)
    {
        ssize_t num_read = read(communication_pipe[0], buffer, buffer_size);
        if (num_read == 0)
            break;
        else if (num_read == -1)
            check_for_error(num_read);
        else
        {
            result.append(buffer, buffer + num_read);
        }
    }
    check_for_error(close(communication_pipe[0]));
    int child_return_code = 0;
    pid_t waited = waitpid(pid, &child_return_code, 0);
    CHECK_FOR_PROGRAMMER_ERROR(waited == pid);
    CHECK_FOR_PROGRAMMER_ERROR(WIFEXITED(child_return_code) && WEXITSTATUS(child_return_code) == 0);
    if (!string_starts_with(result, subprocess_start_string))
    {
        CHECK_FOR_PROGRAMMER_ERROR(!"Something went wrong. Breakpoint here");
    }
    size_t num_iterations_start = result.find(subprocess_num_iterations_string);
    CHECK_FOR_PROGRAMMER_ERROR(num_iterations_start != std::string::npos);
    size_t time_start = result.find(subprocess_time_string, num_iterations_start + subprocess_num_iterations_string.size());
    CHECK_FOR_PROGRAMMER_ERROR(time_start != std::string::npos);
    size_t num_items_start = result.find(subprocess_num_items_string, time_start + subprocess_time_string.size());
    CHECK_FOR_PROGRAMMER_ERROR(num_items_start != std::string::npos);
    size_t num_bytes_start = result.find(subprocess_num_bytes_string, num_items_start + subprocess_num_items_string.size());
    CHECK_FOR_PROGRAMMER_ERROR(num_bytes_start != std::string::npos);
    int actual_num_iterations = 0;
    RAW_VERIFY(StrToInt({ result.data() + num_iterations_start + subprocess_num_iterations_string.size(), time_start - num_iterations_start - subprocess_num_iterations_string.size() }, actual_num_iterations));
    std::chrono::nanoseconds::rep time = 0;
    RAW_VERIFY(StrToInt({ result.data() + time_start + subprocess_time_string.size(), num_items_start - time_start - subprocess_time_string.size() }, time));
    size_t num_items_processed = 0;
    RAW_VERIFY(StrToInt({ result.data() + num_items_start + subprocess_num_items_string.size(), num_bytes_start - num_items_start - subprocess_num_items_string.size() }, num_items_processed));
    size_t num_bytes_used = 0;
    RAW_VERIFY(StrToInt({ result.data() + num_bytes_start + subprocess_num_bytes_string.size(), result.size() - 1 - num_bytes_start - subprocess_num_bytes_string.size() }, num_bytes_used));
    return { actual_num_iterations, argument, std::chrono::nanoseconds(time), num_items_processed, num_bytes_used };
}

double RunResults::GetNanosecondsPerItem(BenchmarkResults * baseline_data) const
{
    if (baseline_data)
    {
        auto found = baseline_data->results.find(argument);
        if (found == baseline_data->results.end())
        {
            State state(baseline_data->benchmark->FindGoodNumberOfIterations(argument, BenchmarkResults::default_run_time), argument);
            baseline_data->benchmark->Run(state);
            RunResults new_results = state.GetResults();
            double diff = GetNanosecondsPerItem(nullptr) - new_results.GetNanosecondsPerItem(nullptr);
            baseline_data->AddResult(std::move(new_results));
            return diff;
        }
        else
        {
            return GetNanosecondsPerItem(nullptr) - found->second.front().GetNanosecondsPerItem(nullptr);
        }
    }
    else if (num_items_processed)
        return time.count() / static_cast<double>(num_items_processed);
    else
        return time.count() / static_cast<double>(num_iterations);
}

LambdaBenchmark::LambdaBenchmark(std::function<void (State &)> func, BenchmarkCategories categories)
    : Benchmark(std::move(categories))
    , function(std::move(func))
{
}

void LambdaBenchmark::Run(State & state) const
{
    return function(state);
}

bool RunSingleBenchmarkFromCommandLine(int argc, char * argv[])
{
    if (argc < 1)
        return false;
    my_executable_name = argv[0];
    initial_working_dir = std::filesystem::current_path();
    if (argc < 2)
        return false;
    if (std::strcmp(argv[1], "--run-benchmark-then-exit") != 0)
        return false;
    if (argc < 6)
    {
        std::cout << "Not enough arguments. Format is --run-benchmark-then-exit <index> <name> <argument> <num_iterations>" << std::endl;
        return true;
    }
    const std::vector<BenchmarkResults *> & all_benchmarks = AllBenchmarksNumbered();
    int index = 0;
    if (!StrToInt(argv[2], index))
    {
        std::cout << "Error parsing the benchmark index" << std::endl;
        return true;
    }
    else if (index < 0)
    {
        std::cout << "Error: Got a negative benchmark index" << std::endl;
        return true;
    }
    else if (static_cast<size_t>(index) >= all_benchmarks.size())
    {
        std::cout << "Error: benchmark index is too big. Num benchmarks: " << all_benchmarks.size() << ", index: " << index << std::endl;
        return true;
    }
    BenchmarkResults * benchmark = all_benchmarks[index];
    if (benchmark->categories->GetName() != argv[3])
    {
        std::cout << "Error: the benchmark had a different name than expected. Expected: " << argv[3] << ", actual: " << benchmark->categories->GetName() << std::endl;
        return true;
    }
    int argument = 0;
    if (!StrToInt(argv[4], argument))
    {
        std::cout << "Error parsing the benchmark argument" << std::endl;
        return true;
    }
    int iteration_count = 0;
    if (!StrToInt(argv[5], iteration_count))
    {
        std::cout << "Error parsing the benchmark iteration count" << std::endl;
        return true;
    }

    skb::State benchmark_state(iteration_count, argument);
    benchmark->benchmark->Run(benchmark_state);
    RunResults results = benchmark_state.GetResults();

    std::cout << subprocess_start_string << argv[3] << subprocess_num_iterations_string << results.num_iterations
              << subprocess_time_string << results.time.count()
              << subprocess_num_items_string << results.num_items_processed
              << subprocess_num_bytes_string << results.num_bytes_used
              << '\n';
    std::cout.flush();
    return true;
}

}
