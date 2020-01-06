#include "db/sqlite_wrapper.hpp"
#include "sqlite3.h"
#include "debug/error_handling.hpp"

std::unique_ptr<Database> Database::test_db;

void SqLiteStatement::StatementDestructor::operator()(sqlite3_stmt * ptr) const
{
    if (ptr)
        sqlite3_finalize(ptr);
}


void SqLite::SqLiteDestructor::operator()(sqlite3 * ptr) const
{
    if (ptr)
        sqlite3_close(ptr);
}

SqLite::SqLite(const char * filename)
{
    sqlite3 * open_db;
    if (sqlite3_open(filename, &open_db) != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(open_db));
        UNHANDLED_ERROR("couldn't open the database. should pass the error up the callstack");
        sqlite3_close(open_db);
    }
    else
        db.reset(open_db);
}

std::pair<SqLiteStatement, StringView<const char>> SqLite::prepare(StringView<const char> text)
{
    sqlite3_stmt * statement = nullptr;
    const char * remainder = nullptr;
    if (sqlite3_prepare_v2(*this, text.begin(), int(text.size()), &statement, &remainder) != SQLITE_OK)
    {
        fprintf(stderr, "Can't prepare statement: %s\n", sqlite3_errmsg(*this));
        UNHANDLED_ERROR("couldn't prepare a statement. should pass the error up the callstack");
        return {SqLiteStatement(), text};
    }
    return {SqLiteStatement(statement), {remainder, text.end()}};
}

bool SqLiteStatement::step()
{
    int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE)
        return false;
    else if (result == SQLITE_ROW)
        return true;
    else
    {
        UNHANDLED_ERROR("unhandled return value from sqlite3_step");
        return false;
    }
}
void SqLiteStatement::reset()
{
    int result = sqlite3_reset(statement.get());
    if (result != SQLITE_OK)
    {
        UNHANDLED_ERROR("unhandled return value from sqlite3_reset");
    }
}
void SqLiteStatement::bind(int index, int value)
{
    int result = sqlite3_bind_int(statement.get(), index, value);
    if (result != SQLITE_OK)
    {
        UNHANDLED_ERROR("TODO: handle error of sqlite_bind");
    }
}

void SqLiteStatement::bind(int index, int64_t value)
{
    int result = sqlite3_bind_int64(statement.get(), index, value);
    if (result != SQLITE_OK)
    {
        UNHANDLED_ERROR("TODO: handle error of sqlite_bind");
    }
}

void SqLiteStatement::bind(int index, double value)
{
    int result = sqlite3_bind_double(statement.get(), index, value);
    if (result != SQLITE_OK)
    {
        UNHANDLED_ERROR("TODO: handle error of sqlite_bind");
    }
}

/*void SqLiteStatement::bind(int index, StringView<const char> text)
{
    int result = sqlite3_bind_text(statement.get(), index, text.data(), text.size(), SQLITE_TRANSIENT);
    if (result != SQLITE_OK)
    {
        UNHANDLED_ERROR("TODO: handle error of sqlite_bind");
    }
}*/
void SqLiteStatement::bind(int index, std::string_view text)
{
    int result = sqlite3_bind_text(statement.get(), index, text.data(), text.size(), SQLITE_TRANSIENT);
    if (result != SQLITE_OK)
    {
        UNHANDLED_ERROR("TODO: handle error of sqlite_bind");
    }
}

int SqLiteStatement::GetInt(int index)
{
    return sqlite3_column_int(statement.get(), index);
}
int64_t SqLiteStatement::GetInt64(int index)
{
    return sqlite3_column_int64(statement.get(), index);
}
double SqLiteStatement::GetDouble(int index)
{
    return sqlite3_column_double(statement.get(), index);
}
const char * SqLiteStatement::GetString(int index)
{
    return reinterpret_cast<const char *>(sqlite3_column_text(statement.get(), index));
}

static void init_version_table(SqLite & db)
{
    sqlite3_step(db.prepare("create table if not exists versions (type STRING PRIMARY KEY NOT NULL, current INTEGER NOT NULL);").first);
}

Database::Database(const char *filename)
    : SqLite(filename)
{
    if (is_valid())
        init_version_table(*this);
}

