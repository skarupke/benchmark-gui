#pragma once

#include <memory>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

struct SqLiteStatement
{
    struct StatementDestructor
    {
        void operator()(sqlite3_stmt * ptr) const;
    };

    SqLiteStatement()
    {
    }
    SqLiteStatement(sqlite3_stmt * statement)
        : statement(statement)
    {
    }

    operator sqlite3_stmt *()
    {
        return statement.get();
    }

    bool is_valid() const
    {
        return bool(statement);
    }

    bool step();
    void reset();

    void bind(int index, int value);
    void bind(int index, int64_t value);
    void bind(int index, double value);
    void bind(int index, std::string_view text);

    int GetInt(int index);
    int64_t GetInt64(int index);
    double GetDouble(int index);
    const char * GetString(int index);

private:
    std::unique_ptr<sqlite3_stmt, StatementDestructor> statement;
};

struct SqLite
{
    struct SqLiteDestructor
    {
        void operator()(sqlite3 * ptr) const;
    };

    SqLite(const char * filename);

    operator sqlite3 *()
    {
        return db.get();
    }

    bool is_valid() const
    {
        return bool(db);
    }

    SqLiteStatement prepare(std::string_view text);
    void prepare_and_run(std::string_view text);
    std::pair<SqLiteStatement, std::string_view> prepare_part(std::string_view text);

private:
    std::unique_ptr<sqlite3, SqLiteDestructor> db;
};

struct Database : public SqLite
{
    static std::unique_ptr<Database> test_db;

    Database(const char * filename);
};

