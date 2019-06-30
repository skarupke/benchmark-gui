#include "custom_benchmark/custom_benchmark.h"

namespace skb
{
int global_counter = 0;

const float BenchmarkResults::default_run_time = 0.25f;

State::State(int num_iterations, int argument)
    : num_iterations(std::max(1, num_iterations))
    , argument(argument)
{
}

const std::string & BenchmarkCategories::TypeIndex()
{
    static const std::string result = "type";
    return result;
}
const std::string & BenchmarkCategories::NameIndex()
{
    static const std::string result = "name";
    return result;
}
const std::string & BenchmarkCategories::CompilerIndex()
{
    static const std::string result = "compiler";
    return result;
}
BenchmarkCategories::BenchmarkCategories(std::string type, std::string name)
{
    categories[TypeIndex()] = std::move(type);
    categories[NameIndex()] = std::move(name);
}
const std::string & BenchmarkCategories::GetName() const
{
    auto found = categories.find(NameIndex());
    CHECK_FOR_PROGRAMMER_ERROR(found != categories.end());
    return found->second;
}
const std::string & BenchmarkCategories::GetType() const
{
    auto found = categories.find(TypeIndex());
    CHECK_FOR_PROGRAMMER_ERROR(found != categories.end());
    return found->second;
}
const std::string & BenchmarkCategories::GetCompiler() const
{
    auto found = categories.find(CompilerIndex());
    CHECK_FOR_PROGRAMMER_ERROR(found != categories.end());
    return found->second;
}

void BenchmarkCategories::AddCategory(std::string category, std::string value)
{
    CHECK_FOR_PROGRAMMER_ERROR(categories.find(category) == categories.end());
    categories.emplace(std::move(category), std::move(value));
}

bool BenchmarkCategories::operator<(const BenchmarkCategories & other) const
{
    return categories < other.categories;
}

CategoryBuilder CategoryBuilder::AddCategory(std::string category, std::string value) const &
{
    CategoryBuilder result = *this;
    RAW_VERIFY(result.categories.emplace(std::move(category), std::move(value)).second);
    return result;
}
CategoryBuilder CategoryBuilder::AddCategory(std::string category, std::string value) &&
{
    CategoryBuilder result = std::move(*this);
    RAW_VERIFY(result.categories.emplace(std::move(category), std::move(value)).second);
    return result;
}

skb::BenchmarkCategories CategoryBuilder::BuildCategories(std::string type, std::string name) const &
{
    skb::BenchmarkCategories result(std::move(type), std::move(name));
    for (const auto & category : categories)
    {
        result.AddCategory(category.first, category.second);
    }
    return result;
}
skb::BenchmarkCategories CategoryBuilder::BuildCategories(std::string type, std::string name) &&
{
    skb::BenchmarkCategories result(std::move(type), std::move(name));
    for (auto & category : categories)
    {
        result.AddCategory(std::move(category.first), std::move(category.second));
    }
    return result;
}

static const std::vector<std::string> & AllCompilers()
{
    static std::vector<std::string> result = { "gcc", "clang" };
    return result;
}

std::map<std::string, std::vector<BenchmarkResults *>> & AllBaselineBenchmarks()
{
    static std::map<std::string, std::vector<BenchmarkResults *>> result;
    return result;
}

Benchmark::Benchmark(BenchmarkCategories categories)
{
    for (const std::string & compiler : AllCompilers())
    {
        BenchmarkCategories compiler_categories = categories;
        compiler_categories.AddCategory(BenchmarkCategories::CompilerIndex(), compiler);
        AddToAllBenchmarks(compiler_categories);
    }
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
    results.push_back(&added.first->second);
}

Benchmark * Benchmark::SetBaseline(std::string name_of_baseline_benchmark)
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
    std::string result = GetType();
    result += '/';
    result += GetName();
    for (auto & category : GetCategories())
    {
        if (category.first == TypeIndex() || category.first == NameIndex())
            continue;

        result += '/';
        result += category.second;
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
        return 1;
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
std::map<std::string, std::map<std::string, std::set<BenchmarkResults *>>> & Benchmark::AllCategories()
{
    static std::map<std::string, std::map<std::string, std::set<BenchmarkResults *>>> result;
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
}
