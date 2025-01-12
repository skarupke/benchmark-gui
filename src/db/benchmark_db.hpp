#pragma once

#include "db/sqlite_wrapper.hpp"
#include "custom_benchmark/custom_benchmark.h"

struct BenchmarkDB {
    BenchmarkDB(const char * filename);
    ~BenchmarkDB();

    int AddBenchmark(interned_string executable, const skb::BenchmarkCategories & categories);
    int GetBenchmarkId(std::string_view categories_string);
    int GetBenchmarkId(const skb::BenchmarkCategories & categories);

    void AddResult(int benchmark_id, const skb::RunResults & result);

    void AddCheckboxState(interned_string category, interned_string checkbox, bool state);

    void BeginTransaction();
    void EndTransaction();

    void DeleteOldResults(interned_string executable);
    void DeleteOldBenchmarks(interned_string executable);
    void DeleteCheckboxState();

    SqLiteStatement load_result;
    SqLiteStatement read_checkbox;
private:
    Database db;
    SqLiteStatement get_benchmark_id;
    SqLiteStatement insert_benchmark;
    SqLiteStatement add_result;
    SqLiteStatement delete_results;
    SqLiteStatement delete_benchmarks;
    SqLiteStatement add_checkbox_state;
};
