#include "db/benchmark_db.hpp"
#include "debug/assert.hpp"

BenchmarkDB::BenchmarkDB(const char * filename)
    : db(filename)
{
    db.prepare_and_run("CREATE TABLE IF NOT EXISTS benchmarks "
                       "(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "filename TEXT NOT NULL, "
                        "categories TEXT UNIQUE)");
    db.prepare_and_run("CREATE TABLE IF NOT EXISTS results "
                       "(benchmark INTEGER, "
                        "num_iterations INTEGER, "
                        "argument INTEGER, "
                        "time INTEGER, "
                        "num_items_processed INTEGER, "
                        "num_bytes_used INTEGER)");
    db.prepare_and_run("CREATE INDEX IF NOT EXISTS results_benchmark_index "
                       "ON results (benchmark)");
    db.prepare_and_run("CREATE INDEX IF NOT EXISTS benchmark_categories_index "
                       "ON benchmarks (categories)");
    db.prepare_and_run("CREATE INDEX IF NOT EXISTS benchmark_filename_index "
                       "ON benchmarks (filename)");
    db.prepare_and_run("CREATE TABLE IF NOT EXISTS checkbox_state "
                       "(category TEXT, "
                        "checkbox TEXT, "
                        "checked INTEGER)");
    
    load_result = db.prepare("SELECT num_iterations, argument, time, num_items_processed, num_bytes_used FROM results WHERE benchmark = ?1");
    get_benchmark_id = db.prepare("SELECT id FROM benchmarks WHERE categories = ?1");
    insert_benchmark = db.prepare("INSERT INTO benchmarks (filename, categories) VALUES (?1, ?2)");
    add_result = db.prepare("INSERT INTO results (benchmark, num_iterations, argument, time, num_items_processed, num_bytes_used) VALUES(?1, ?2, ?3, ?4, ?5, ?6)");
    delete_results = db.prepare(
        "DELETE FROM results "
        "WHERE benchmark IN ( "
        "   SELECT id "
        "   FROM benchmarks "
        "   WHERE filename = ?1 "
        ")");
    delete_benchmarks = db.prepare("DELETE FROM benchmarks WHERE filename = ?1");
    read_checkbox = db.prepare("SELECT category, checkbox, checked FROM checkbox_state");
    add_checkbox_state = db.prepare("INSERT INTO checkbox_state (category, checkbox, checked) VALUES(?1, ?2, ?3)");
}

BenchmarkDB::~BenchmarkDB() {
    db.prepare_and_run("VACUUM");
}

void BenchmarkDB::BeginTransaction() {
    db.prepare_and_run("BEGIN");
}
void BenchmarkDB::EndTransaction() {
    db.prepare_and_run("END");
}

int BenchmarkDB::AddBenchmark(interned_string executable, const skb::BenchmarkCategories & categories) {
    insert_benchmark.bind(1, executable.view());
    std::string categories_string = categories.CategoriesString();
    insert_benchmark.bind(2, categories_string);
    RAW_VERIFY(!insert_benchmark.step());
    insert_benchmark.reset();

    return GetBenchmarkId(categories_string);
}

int BenchmarkDB::GetBenchmarkId(std::string_view categories_string) {
    get_benchmark_id.bind(1, categories_string);
    if (!get_benchmark_id.step())
    {
        get_benchmark_id.reset();
        return -1;
    }
    int result = get_benchmark_id.GetInt(0);
    RAW_VERIFY(!get_benchmark_id.step());
    get_benchmark_id.reset();
    return result;
}
int BenchmarkDB::GetBenchmarkId(const skb::BenchmarkCategories & categories) {
    return GetBenchmarkId(categories.CategoriesString());
}

void BenchmarkDB::AddResult(int benchmark_id, const skb::RunResults & result) {
    add_result.bind(1, benchmark_id);
    add_result.bind(2, result.num_iterations);
    add_result.bind(3, result.argument);
    add_result.bind(4, result.time.count());
    add_result.bind(5, static_cast<int64_t>(result.num_items_processed));
    add_result.bind(6, static_cast<int64_t>(result.num_bytes_used));
    RAW_VERIFY(!add_result.step());
    add_result.reset();
}

void BenchmarkDB::AddCheckboxState(interned_string category, interned_string checkbox, bool state) {
    add_checkbox_state.bind(1, category);
    add_checkbox_state.bind(2, checkbox);
    add_checkbox_state.bind(3, static_cast<int>(state));
    RAW_VERIFY(!add_checkbox_state.step());
    add_checkbox_state.reset();
}

void BenchmarkDB::DeleteOldResults(interned_string executable) {
    delete_results.bind(1, executable.view());
    RAW_VERIFY(!delete_results.step());
    delete_results.reset();
}

void BenchmarkDB::DeleteOldBenchmarks(interned_string executable) {
    delete_benchmarks.bind(1, executable.view());
    RAW_VERIFY(!delete_benchmarks.step());
    delete_benchmarks.reset();
}
void BenchmarkDB::DeleteCheckboxState() {
    db.prepare_and_run("DELETE FROM checkbox_state");
}


