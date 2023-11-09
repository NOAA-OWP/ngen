#include <algorithm>
#include <memory>

#include "NGen_SQLite.hpp"

using namespace geopackage;

// Defined in database.cpp
extern std::runtime_error sqlite_error(const std::string& f, int code, const std::string& extra = "");

/**
 * Runtime error for iterations that haven't started
 */
const auto sqlite_get_notstarted_error = std::runtime_error(
    "sqlite iteration is has not started, get() is not callable (call sqlite_iter::next() before)"
);

/**
 * Runtime error for iterations that are finished
 */
const auto sqlite_get_done_error = std::runtime_error(
    "sqlite iteration is done, get() is not callable"
);

sqlite_iter::sqlite_iter(stmt_t stmt)
    : stmt(std::move(stmt))
{
    // sqlite3_column_type requires the last result code to be
    // SQLITE_ROW, so we need to iterate on the first row.
    // TODO: need to test how this functions if the last result code
    //       was SQLITE_DONE.
    this->next();
    this->column_count = sqlite3_column_count(this->ptr());
    this->column_names.reserve(this->column_count);
    this->column_types.reserve(this->column_count);

    for (int i = 0; i < this->column_count; i++) {
        this->column_names.emplace_back(sqlite3_column_name(this->ptr(), i));
        this->column_types.emplace_back(sqlite3_column_type(this->ptr(), i));
    }

    this->restart();
}

sqlite3_stmt* sqlite_iter::ptr() const noexcept
{
    return this->stmt.get();
}

void sqlite_iter::handle_get_index(int col) const
{
    if (this->done()) {
        throw sqlite_get_done_error;
    }

    if (this->current_row() == -1) {
        throw sqlite_get_notstarted_error;
    }

    if (col < 0 || col >= this->column_count) {
        
        throw std::out_of_range(
            "column " + std::to_string(col) + " out of range of " + std::to_string(this->column_count) + " columns"
        );
    }
}

bool sqlite_iter::done() const noexcept
{
    return this->iteration_finished;
}

sqlite_iter& sqlite_iter::next()
{
    if (!this->done()) {
        const int returncode = sqlite3_step(this->ptr());
        if (returncode == SQLITE_DONE) {
            this->iteration_finished = true;
        }
        else if( returncode == SQLITE_ROW){
            // Update column dtypes for the next row
            // There are a couple ways to do this, each with some nuance:
            // 1) use sqlite3_column_type upon construction of the iterator and inspect the first row
            //    then use those column types for all rows
            // 2) use the sqlite3 table definition schema (i.e. PRAGMA table_info(...);) to get column types
            // 3) check each row during iteration using sqlite3_column_type
            // 1 & 2 may not produce consistent results because if the value in a column is NaN, 
            // then sqlite3_column_type will report that as a 5 or NULL type, even if the schema has a datatype
            // Using 1, when the first row contains NaN but other rows have valid data, then all types are reported as NULL.
            // Using 2, any NaN/NULL data will get interperted as the schema type
            // Using 3 ensures we get a non-null type if there is valid data in the column in that row, and NULL when 
            // no data is present, with a small bit of additional iterator overhead.
            for (int i = 0; i < this->column_count && i < column_types.size(); i++) {
                this->column_types[i] = (sqlite3_column_type(this->ptr(), i));
            }
        } else {
            throw sqlite_error("sqlite3_step", returncode);
        }
        this->iteration_step++;
    }

    return *this;
}

sqlite_iter& sqlite_iter::restart()
{
    sqlite3_reset(this->ptr());
    this->iteration_step = -1;
    this->iteration_finished = false;
    return *this;
}

int sqlite_iter::current_row() const noexcept
{
    return this->iteration_step;
}

int sqlite_iter::num_columns() const noexcept
{
    return this->column_count;
}

int sqlite_iter::column_index(const std::string& name) const noexcept
{
    const size_t pos =
      std::distance(this->column_names.begin(), std::find(this->column_names.begin(), this->column_names.end(), name));

    return pos >= this->column_names.size() ? -1 : pos;
}

template<>
std::vector<uint8_t> sqlite_iter::get<std::vector<uint8_t>>(int col) const
{
    this->handle_get_index(col);
    int size = sqlite3_column_bytes(this->ptr(), col);
    const uint8_t* ptr = static_cast<const uint8_t*>(sqlite3_column_blob(this->ptr(), col));
    std::vector<uint8_t> blob(ptr, ptr+size);
    return blob;
}

template<>
double sqlite_iter::get<double>(int col) const
{
    this->handle_get_index(col);
    return sqlite3_column_double(this->ptr(), col);
}

template<>
int sqlite_iter::get<int>(int col) const
{
    this->handle_get_index(col);
    return sqlite3_column_int(this->ptr(), col);
}

template<>
std::string sqlite_iter::get<std::string>(int col) const
{
    this->handle_get_index(col);
    // TODO: this won't work with non-ASCII text
    int size = sqlite3_column_bytes(this->ptr(), col);
    const unsigned char* ptr = sqlite3_column_text(this->ptr(), col);
    return std::string(ptr, ptr+size);
}
