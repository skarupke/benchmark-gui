#include "custom_benchmark/custom_benchmark.h"
#include <charconv>
#include <cstring>
#include <string_view>
#include <iostream>
#include <filesystem>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <charconv>

namespace skb
{
int global_counter = 0;
float global_float = 0.0f;
double global_double = 0.0;

const char * const LIST_ALL_BENCHMARKS = "--list-all-benchmarks";
const char * const RUN_BENCHMARK_THEN_EXIT = "--run-benchmark-then-exit";

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
BenchmarkCategories::BenchmarkCategories() = default;
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
    CHECK_FOR_PROGRAMMER_ERROR(category.view().find('\n') == std::string::npos);
    CHECK_FOR_PROGRAMMER_ERROR(value.view().find('\n') == std::string::npos);
    CHECK_FOR_PROGRAMMER_ERROR(categories.find(category) == categories.end());
    categories.emplace(std::move(category), std::move(value));
}
BenchmarkCategories BenchmarkCategories::AddCategoryCopy(interned_string category, interned_string value) const &
{
    BenchmarkCategories copy = *this;
    copy.AddCategory(category, value);
    return copy;
}
BenchmarkCategories BenchmarkCategories::AddCategoryCopy(interned_string category, interned_string value) &&
{
    BenchmarkCategories copy = std::move(*this);
    copy.AddCategory(category, value);
    return copy;
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
bool BenchmarkCategories::operator==(const BenchmarkCategories & other) const
{
    return categories == other.categories;
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

static std::vector<BenchmarkResults *> & AllBenchmarksNumbered()
{
    static std::vector<BenchmarkResults *> result;
    return result;
}

Benchmark::Benchmark(BenchmarkCategories categories)
{
    for (const interned_string & compiler : AllCompilers())
    {
        BenchmarkCategories compiler_categories = categories;
        compiler_categories.AddCategory(BenchmarkCategories::CompilerIndex(), compiler);
        skb::BenchmarkResults * new_results = AddToAllBenchmarks(compiler_categories);
        new_results->my_global_index = AllBenchmarksNumbered().size();
        AllBenchmarksNumbered().push_back(new_results);
    }
}
Benchmark::Benchmark(BenchmarkCategories type, interned_string executable, int index_in_executable)
{
    skb::BenchmarkResults * new_results = AddToAllBenchmarks(type);
    new_results->executable = executable;
    new_results->my_global_index = index_in_executable;
}

skb::BenchmarkResults * Benchmark::AddToAllBenchmarks(const BenchmarkCategories & categories)
{
    auto [added, did_add] = AllBenchmarks().emplace(categories, this);
    CHECK_FOR_PROGRAMMER_ERROR(did_add);
    skb::BenchmarkResults * new_results = &added->second;
    new_results->categories = &added->first;
    for (const auto & category : categories.GetCategories())
    {
        RAW_VERIFY(AllCategories()[category.first][category.second].insert(new_results).second);
    }
    if (categories.GetType() == "baseline")
    {
        AllBaselineBenchmarks()[categories.GetName()].push_back(new_results);
    }
    results.push_back(new_results);
    return new_results;
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

std::string EscapeString(std::string_view str)
{
    std::string result;
    result.reserve(str.size() + std::count_if(str.begin(), str.end(), [](char c)
    {
         return c == '\\' || c == '"';
    }));
    for (char c : str) {
        if (c == '\\' || c == '"') {
            result.push_back('\\');
        }
        result.push_back(c);
    }
    return result;
}
std::string UnescapeString(std::string_view str)
{
    std::string result(str);
    result.erase(std::remove_if(result.begin(), result.end(), [last_was_backslash = false](char c) mutable {
        if (c == '\\') {
            last_was_backslash = !last_was_backslash;
        } else {
            last_was_backslash = false;
        }
        return last_was_backslash;
    }), result.end());
    return result;
}

std::string BenchmarkCategories::Serialize() const
{
    std::stringstream str;
    bool first = true;
    for (const auto& [category, value] : categories)
    {
        if (first) first = false;
        else str << ',';
        str << '"' << EscapeString(category) << "\":\"" << EscapeString(value) << '"';
    }
    return str.str();
}
size_t EndOfEscapedString(std::string_view str, size_t start = 0) {
    bool last_was_backslash = false;
    for (; start < str.size(); ++start) {
        char c = str[start];
        if (c == '\\') {
            last_was_backslash = !last_was_backslash;
        } else if (!last_was_backslash && c == '"') {
            return start;
        } else {
            last_was_backslash = false;
        }
    }
    return str.size();
}
BenchmarkCategories BenchmarkCategories::Deserialize(std::string_view str)
{
    BenchmarkCategories result;
    size_t start = 0;
    while (start < str.size()) {
        size_t end = EndOfEscapedString(str, start + 1);
        interned_string category(UnescapeString(str.substr(start + 1, end - start - 1)));
        size_t end_of_value = EndOfEscapedString(str, end + 3);
        interned_string value(UnescapeString(str.substr(end + 3, end_of_value - end - 3)));
        result.AddCategory(category, value);
        start = end_of_value + 2;
    }
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
        if (multiplier <= 1.0)
            multiplier = 8.0;
        all_arguments.push_back(range_begin);
        for (double i = range_begin * multiplier; i < range_end;)
        {
            int rounded = static_cast<int>(i + 0.5f);
            if (rounded <= all_arguments.back())
                all_arguments.push_back(all_arguments.back() + 1);
            else
                all_arguments.push_back(rounded);
            for (;;)
            {
                i = i * multiplier;
                if (i >= all_arguments.back())
                    break;
            }
        }
        if (all_arguments.back() != range_end)
            all_arguments.push_back(range_end);
    }
    return all_arguments;
}

Benchmark::RangeOfArguments Benchmark::GetArgumentRange() const
{
    return { range_begin, range_end, range_multiplier };
}
std::string Benchmark::RangeOfArguments::Serialize() const
{
    char buffer[128];
    auto [ptr, err] = std::to_chars(std::begin(buffer), std::end(buffer), multiplier);
    std::stringstream result;
    result << begin << ',' << end << ',' << std::string_view(buffer, ptr);
    return result.str();
}

template<typename I>
bool StrToNumber(std::string_view str, I & to_fill)
{
    std::from_chars_result result = std::from_chars(str.data(), str.data() + str.size(), to_fill);
    return result.ptr != str.data() && result.ptr == str.data() + str.size();
}
Benchmark::RangeOfArguments Benchmark::RangeOfArguments::Deserialize(std::string_view str)
{
    auto end = str.data() + str.size();
    auto first_comma = std::find(str.data(), end, ',');
    int range_begin;
    RAW_VERIFY(StrToNumber({str.data(), first_comma}, range_begin));
    auto second_comma = std::find(first_comma + 1, end, ',');
    int range_end;
    RAW_VERIFY(StrToNumber({first_comma + 1, second_comma}, range_end));
    double range_multiplier;
    RAW_VERIFY(StrToNumber({second_comma + 1, end}, range_multiplier));
    return { range_begin, range_end, range_multiplier };
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


void check_for_error(int result)
{
    CHECK_FOR_PROGRAMMER_ERROR(result != -1);
}

std::string read_pipe(int pipe)
{
    std::string result;
    constexpr int buffer_size = 1024;
    char buffer[buffer_size];
    for (;;)
    {
        ssize_t num_read = read(pipe, buffer, buffer_size);
        if (num_read == 0)
            break;
        else if (num_read == -1)
            check_for_error(num_read);
        else
            result.append(buffer, buffer + num_read);
    }
    check_for_error(close(pipe));
    return result;
}

struct ChildProcessOutput {
    int return_code;
    std::string stdout;
};

ChildProcessOutput RunProcess(const std::vector<std::string> & arguments, bool forward_stderr)
{
    int communication_pipe[2] = { 0, 0 };
    check_for_error(pipe(communication_pipe));
    std::vector<char *> as_char_pointers;
    as_char_pointers.reserve(arguments.size());
    for (const std::string & str : arguments)
        as_char_pointers.push_back(const_cast<char *>(str.c_str()));
    as_char_pointers.push_back(nullptr);
    pid_t pid = vfork();
    CHECK_FOR_PROGRAMMER_ERROR(pid != -1);
    if (pid == 0)
    {
        check_for_error(close(communication_pipe[0]));
        check_for_error(dup2(communication_pipe[1], STDOUT_FILENO));
        if (forward_stderr)
            check_for_error(dup2(communication_pipe[1], STDERR_FILENO));
        check_for_error(close(communication_pipe[1]));

        std::filesystem::current_path(initial_working_dir);
        execv(arguments[0].c_str(), as_char_pointers.data());
        // exec will only return if something goes wrong
        const char * error_str = strerror(errno);
        std::cout << "Error: Couldn't launch executable " << arguments[0] << ". Error was " << error_str << std::endl;
        exit(1);
    }
    check_for_error(close(communication_pipe[1]));
    std::string result = read_pipe(communication_pipe[0]);
    int child_return_code = 0;
    pid_t waited = waitpid(pid, &child_return_code, 0);
    CHECK_FOR_PROGRAMMER_ERROR(waited == pid);
    return { child_return_code, result };
}

RunResults BenchmarkResults::RunInNewProcess(int num_iterations, int argument)
{
    std::vector<std::string> arguments;
    if (this->executable.view().empty())
        arguments.push_back(my_executable_name);
    else
        arguments.push_back(std::string(executable.view()));
    arguments.push_back(RUN_BENCHMARK_THEN_EXIT);
    arguments.push_back(std::to_string(my_global_index));
    arguments.push_back(std::string(categories->GetName().view()));
    arguments.push_back(std::to_string(argument));
    arguments.push_back(std::to_string(num_iterations));
    auto [child_return_code, result] = RunProcess(arguments, false);
    CHECK_FOR_PROGRAMMER_ERROR(WIFEXITED(child_return_code) && WEXITSTATUS(child_return_code) == 0);
    size_t num_iterations_start = result.find(subprocess_num_iterations_string);
    CHECK_FOR_PROGRAMMER_ERROR(num_iterations_start != std::string::npos);
    size_t time_start = result.find(subprocess_time_string, num_iterations_start + subprocess_num_iterations_string.size());
    CHECK_FOR_PROGRAMMER_ERROR(time_start != std::string::npos);
    size_t num_items_start = result.find(subprocess_num_items_string, time_start + subprocess_time_string.size());
    CHECK_FOR_PROGRAMMER_ERROR(num_items_start != std::string::npos);
    size_t num_bytes_start = result.find(subprocess_num_bytes_string, num_items_start + subprocess_num_items_string.size());
    CHECK_FOR_PROGRAMMER_ERROR(num_bytes_start != std::string::npos);
    int actual_num_iterations = 0;
    RAW_VERIFY(StrToNumber({ result.data() + num_iterations_start + subprocess_num_iterations_string.size(), time_start - num_iterations_start - subprocess_num_iterations_string.size() }, actual_num_iterations));
    std::chrono::nanoseconds::rep time = 0;
    RAW_VERIFY(StrToNumber({ result.data() + time_start + subprocess_time_string.size(), num_items_start - time_start - subprocess_time_string.size() }, time));
    size_t num_items_processed = 0;
    RAW_VERIFY(StrToNumber({ result.data() + num_items_start + subprocess_num_items_string.size(), num_bytes_start - num_items_start - subprocess_num_items_string.size() }, num_items_processed));
    size_t num_bytes_used = 0;
    RAW_VERIFY(StrToNumber({ result.data() + num_bytes_start + subprocess_num_bytes_string.size(), result.size() - 1 - num_bytes_start - subprocess_num_bytes_string.size() }, num_bytes_used));
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

BenchmarkInOtherProcess::BenchmarkInOtherProcess(BenchmarkCategories type, Benchmark::RangeOfArguments range, interned_string executable, int index_in_executable)
    : Benchmark(std::move(type), executable, index_in_executable)
{
    SetRange(range.begin, range.end);
    SetRangeMultiplier(range.multiplier);
    for (BenchmarkResults * results : results) {
        for (int i : GetAllArguments()) {
            results->results[i];
        }
    }
}

void BenchmarkInOtherProcess::Run(State &) const
{
    CHECK_FOR_PROGRAMMER_ERROR(!"This should never be called. I know the types don't make sense");
}


void ListAllBenchmarks()
{
    std::cout << "V1\n";
    for (const BenchmarkResults * benchmark : AllBenchmarksNumbered())
    {
        if (benchmark->baseline_results) {
            std::cout << "BenchmarkWithBaseline\n"
                      << benchmark->benchmark->GetArgumentRange().Serialize() << '\n'
                      << benchmark->categories->Serialize() << '\n'
                      << benchmark->baseline_results->categories->GetName().view() << '\n';
        } else {
            std::cout << "BenchmarkWithoutBaseline\n"
                      << benchmark->benchmark->GetArgumentRange().Serialize() << '\n'
                      << benchmark->categories->Serialize() << '\n';
        }
    }
    std::cout.flush();
}

std::vector<std::string> SplitString(const std::string & str, char to_split)
{
    std::vector<std::string> result;
    for (size_t i = 0, found = str.find(to_split);;) {
        result.push_back(str.substr(i, found - i));
        if (found == std::string::npos)
            break;
        i = found + 1;
        found = str.find(to_split, i);
    }
    return result;
}

std::vector<std::unique_ptr<BenchmarkInOtherProcess>> benchmarks_in_other_files;
void LoadAllBenchmarks(interned_string executable, const std::string & run_results)
{
    std::vector<std::string> lines = SplitString(run_results, '\n');
    CHECK_FOR_INVALID_DATA(lines[0] == "V1");
    std::vector<std::pair<size_t, interned_string>> baseline_to_fill_in;
    int benchmark_index_in_other_file = 0;
    for (size_t line = 1;; ++benchmark_index_in_other_file) {
        if (line == lines.size() || (line == lines.size() - 1 && lines[line].empty()))
            break;
        CHECK_FOR_INVALID_DATA(lines.size() > line + 2);
        Benchmark::RangeOfArguments range = Benchmark::RangeOfArguments::Deserialize(lines[line + 1]);
        BenchmarkCategories categories = BenchmarkCategories::Deserialize(lines[line + 2]);
        if (lines[line] == "BenchmarkWithBaseline") {
            CHECK_FOR_INVALID_DATA(lines.size() > line + 3);
            baseline_to_fill_in.emplace_back(benchmarks_in_other_files.size(), lines[line + 3]);
            line += 4;
        } else if (lines[line] == "BenchmarkWithoutBaseline") {
            line += 3;
        } else {
            CHECK_FOR_INVALID_DATA(!"Got wrong line", lines[line]);
        }
        benchmarks_in_other_files.push_back(std::make_unique<BenchmarkInOtherProcess>(std::move(categories), range, executable, benchmark_index_in_other_file));
    }
    for (const auto & [index, baseline] : baseline_to_fill_in) {
        benchmarks_in_other_files[index]->SetBaseline(baseline);
    }
}

std::optional<std::string> LoadAllBenchmarksFromFile(std::string_view executable)
{
    std::vector<std::string> arguments;
    arguments.emplace_back(executable);
    arguments.emplace_back(LIST_ALL_BENCHMARKS);
    auto [child_return_code, result] = RunProcess(arguments, true);
    if (!WIFEXITED(child_return_code) || WEXITSTATUS(child_return_code) != 0) {
        return {"Error running child process: " + result};
    }
    LoadAllBenchmarks(interned_string(executable), result);
    return std::nullopt;
}

bool RunSingleBenchmarkFromCommandLine(int argc, char * argv[])
{
    if (argc < 1)
        return false;
    my_executable_name = argv[0];
    initial_working_dir = std::filesystem::current_path();
    if (argc < 2)
        return false;
    if (std::strcmp(argv[1], LIST_ALL_BENCHMARKS) == 0)
    {
        ListAllBenchmarks();
        return true;
    }
    else if (std::strcmp(argv[1], RUN_BENCHMARK_THEN_EXIT) != 0)
        return false;
    if (argc < 6)
    {
        std::cout << "Not enough arguments. Format is --run-benchmark-then-exit <index> <name> <argument> <num_iterations>" << std::endl;
        return true;
    }
    const std::vector<BenchmarkResults *> & all_benchmarks = AllBenchmarksNumbered();
    int index = 0;
    if (!StrToNumber(argv[2], index))
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
    if (!StrToNumber(argv[4], argument))
    {
        std::cout << "Error parsing the benchmark argument" << std::endl;
        return true;
    }
    int iteration_count = 0;
    if (!StrToNumber(argv[5], iteration_count))
    {
        std::cout << "Error parsing the benchmark iteration count" << std::endl;
        return true;
    }

    skb::State benchmark_state(iteration_count, argument);
    std::cout << subprocess_start_string << argv[3] << subprocess_num_iterations_string << iteration_count;
    std::cout.flush();
    benchmark->benchmark->Run(benchmark_state);
    RunResults results = benchmark_state.GetResults();

    std::cout << subprocess_time_string << results.time.count()
              << subprocess_num_items_string << results.num_items_processed
              << subprocess_num_bytes_string << results.num_bytes_used
              << '\n';
    std::cout.flush();
    return true;
}

}

#include "test/include_test.hpp"
TEST(benchmark_serialize, roundtrip) {
    skb::BenchmarkCategories a("Hello", "World");
    {
        std::string str = a.Serialize();
        skb::BenchmarkCategories b = skb::BenchmarkCategories::Deserialize(str);
        ASSERT_EQ(a, b);
    }
    a.AddCategory("foo", "bar");
    {
        std::string str = a.Serialize();
        skb::BenchmarkCategories b = skb::BenchmarkCategories::Deserialize(str);
        ASSERT_EQ(a, b);
    }
    a.AddCategory("\" \\\"", "\\\"\"\\");
    {
        std::string str = a.Serialize();
        skb::BenchmarkCategories b = skb::BenchmarkCategories::Deserialize(str);
        ASSERT_EQ(a, b);
    }
    a.AddCategory(",,,", ":::");
    std::string str = a.Serialize();
    skb::BenchmarkCategories b = skb::BenchmarkCategories::Deserialize(str);
    ASSERT_EQ(a, b);
}
TEST(benchmark_serialize, split_string)
{
    ASSERT_EQ(std::vector<std::string>({"a"}), skb::SplitString("a", '-'));
    ASSERT_EQ(std::vector<std::string>({"a", "bcd"}), skb::SplitString("a-bcd", '-'));
    ASSERT_EQ(std::vector<std::string>({"a", "", "bcd"}), skb::SplitString("a--bcd", '-'));
    ASSERT_EQ(std::vector<std::string>({"a", "", "bcd", "e"}), skb::SplitString("a--bcd-e", '-'));
    ASSERT_EQ(std::vector<std::string>({"a", "", "bcd", "e", ""}), skb::SplitString("a--bcd-e-", '-'));
}
