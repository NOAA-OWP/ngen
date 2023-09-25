#ifndef NGEN_GEOPACKAGE_SQLITE_H
#define NGEN_GEOPACKAGE_SQLITE_H

#include <memory>
#include <stdexcept>
#include <vector>
#include <string>

#include <sqlite3.h>

namespace geopackage {

/**
 * Deleter used to provide smart pointer support for sqlite3 structs.
 */
struct sqlite_deleter
{
    void operator()(sqlite3* db) { sqlite3_close_v2(db); }
    void operator()(sqlite3_stmt* stmt) { sqlite3_finalize(stmt); }
};

/**
 * Smart pointer (shared) type for sqlite3 prepared statements
 */
using stmt_t = std::unique_ptr<sqlite3_stmt, sqlite_deleter>;

/**
 * SQLite3 row iterator
 *
 * Provides a simple iterator-like implementation
 * over rows of a SQLite3 query.
 */
class sqlite_iter
{
  private:
    stmt_t stmt;
    int iteration_step = -1;
    bool iteration_finished = false;

    int column_count;
    std::vector<std::string> column_names;

    // vector of SQLITE data types, see: https://www.sqlite.org/datatype3.html
    std::vector<int> column_types;

    // returns the raw pointer to the sqlite statement
    sqlite3_stmt* ptr() const noexcept;

    // checks if int is out of range, and throws error if so
    void handle_get_index(int) const;

  public:
    sqlite_iter(stmt_t stmt);

    /**
     * Check if a row iterator is finished
     * 
     * @return true if next() returned SQLITE_DONE
     * @return false if there is more rows available
     */
    bool done() const noexcept;

    /**
     * Step into the next row of a SQLite query
     * 
     * If the query is finished, next() acts idempotently,
     * but will change done() to return true.
     * @return sqlite_iter& returns itself
     */
    sqlite_iter& next();

    /**
     * Restart an iteration to its initial state.
     * next() must be called after calling this.
     * 
     * @return sqlite_iter& returns itself
     */
    sqlite_iter& restart();

    /**
     * Get the current row index for the iterator
     * 
     * @return int the current row index, or -1 if next() hasn't been called
     */
    int current_row() const noexcept;

    /**
     * Get the number of columns within this iterator
     * @return int number of columns in query
     */
    int num_columns() const noexcept;

    /**
     * Return the column index for a named column 
     * 
     * @param name column name to search for
     * @return int index of given column name, or -1 if not found.
     */
    int column_index(const std::string& name) const noexcept;

    /**
     * Get a vector of column names
     * 
     * @return const std::vector<std::string>& column names as a vector of strings
     */
    const std::vector<std::string>& columns() const noexcept { return this->column_names; }

    /**
     * Get a vector of column types
     *
     * See https://www.sqlite.org/datatype3.html for type values. The integers
     * are the affinity for data types.
     * @return const std::vector<int>& column types as a vector of ints
     */
    const std::vector<int>& types() const noexcept { return this->column_types; }

    /**
     * Get a column value from a row iterator by index
     * 
     * @tparam T Type to parse value as, i.e. int
     * @param col Column index to parse
     * @return T value at column `col`
     */
    template<typename T>
    T get(int col) const;

    /**
     * Get a column value from a row iterator by name
     * 
     * @tparam T Type to parse value as, i.e. int
     * @return T value at the named column
     */
    template<typename T>
    T get(const std::string&) const;
};

template<typename T>
inline T sqlite_iter::get(const std::string& name) const
{
    const int index = this->column_index(name);
    return this->get<T>(index);
}

/**
 * Wrapper around a SQLite3 database
 */
class sqlite
{
  private:
    /**
     * Smart pointer (unique) type for sqlite3 database
     */
    using sqlite_t = std::unique_ptr<sqlite3, sqlite_deleter>;
  
    sqlite_t conn = nullptr;

  public:
    sqlite() = delete;

    /**
     * Construct a new sqlite object from a path to database
     *
     * @param path File path to sqlite3 database
     */
    sqlite(const std::string& path);

    sqlite(sqlite& db) = delete;
  
    sqlite& operator=(sqlite& db) = delete;

    /**
     * Take ownership of a sqlite3 database
     *
     * @param db sqlite3 database object
     */
    sqlite(sqlite&& db) = default;

    /**
     * Move assignment operator
     *
     * @param db sqlite3 database object
     * @return sqlite& reference to sqlite3 database
     */
    sqlite& operator=(sqlite&& db) = default;

    /**
     * Return the originating sqlite3 database pointer
     *
     * @return sqlite3*
     */
    sqlite3* connection() const noexcept;

    /**
     * Check if SQLite database contains a given table
     *
     * @param table name of table
     * @return true if table does exist
     * @return false if table does not exist
     */
    bool has_table(const std::string& table) noexcept;

    /**
     * Query the SQLite Database and get the result
     * @param statement String query
     * @return sqlite_iter SQLite row iterator
     */
    sqlite_iter query(const std::string& statement);

    /**
     * Query the SQLite Database with multiple boundable text parameters.
     * 
     * @param statement String query with parameters
     * @param binds text parameters to bind to statement
     * @return sqlite_iter SQLite row iterator
     */
    sqlite_iter query(const std::string& statement, const std::vector<std::string>& binds);

    /**
     * Query the SQLite Database with a bound statement and get the result
     * @param statement String query with parameters
     * @param params parameters to bind to statement
     * @return sqlite_iter SQLite row iterator
     */
    template<typename... T>
    sqlite_iter query(const std::string& statement, T const&... params);
};

} // namespace geopackage

#endif // NGEN_GEOPACKAGE_SQLITE_H
