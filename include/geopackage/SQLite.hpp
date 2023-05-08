#ifndef NGEN_GEOPACKAGE_SQLITE_H
#define NGEN_GEOPACKAGE_SQLITE_H

#include <memory>
#include <stdexcept>
#include <vector>

#include <sqlite3.h>

namespace geopackage {

const auto sqlite_get_notstarted_error = std::runtime_error(
    "sqlite iteration is has not started, get() is not callable (call sqlite_iter::next() before)"
);

const auto sqlite_get_done_error = std::runtime_error(
    "sqlite iteration is done, get() is not callable"
);

/**
 * @brief Deleter used to provide smart pointer support for sqlite3 structs.
 */
struct sqlite_deleter
{
    void operator()(sqlite3* db) { sqlite3_close_v2(db); }
    void operator()(sqlite3_stmt* stmt) { sqlite3_finalize(stmt); }
};

/**
 * @brief Smart pointer (unique) type for sqlite3 database
 */
using sqlite_t = std::unique_ptr<sqlite3, sqlite_deleter>;

/**
 * @brief Smart pointer (shared) type for sqlite3 prepared statements
 */
using stmt_t = std::shared_ptr<sqlite3_stmt>;

/**
 * @brief Get a runtime error based on a function and code.
 *
 * @param f String denoting the function where the error originated
 * @param code sqlite3 result code
 * @return std::runtime_error
 */
inline std::runtime_error sqlite_error(const std::string& f, int code, const std::string& extra = "")
{
    std::string errmsg = f + " returned code "
                           + std::to_string(code)
                           + " (msg: "
                           + std::string(sqlite3_errstr(code))
                           + ")";

    if (!extra.empty()) {
      errmsg += " ";
      errmsg += extra;
    }

    return std::runtime_error(errmsg);
}

/**
 * @brief SQLite3 row iterator
 *
 * Provides a simple iterator-like implementation
 * over rows of a SQLite3 query.
 */
class sqlite_iter
{
  private:
    stmt_t stmt;
    int    iteration_step     = -1;
    bool   iteration_finished = false;

    // column metadata
    int                      column_count;
    std::vector<std::string> column_names;
    std::vector<int>         column_types;

    // returns the raw pointer to the sqlite statement
    sqlite3_stmt* ptr() const noexcept;

    // checks if int is out of range, and throws error if so
    void handle_get_index(int) const;

  public:
    sqlite_iter(stmt_t stmt);
    bool                            done() const noexcept;
    sqlite_iter&                    next();
    sqlite_iter&                    restart();
    void                            close();
    int                             current_row() const noexcept;
    int                             num_columns() const noexcept;
    int                             column_index(const std::string& name) const noexcept;
    const std::vector<std::string>& columns() const noexcept { return this->column_names; }
    const std::vector<int>&         types() const noexcept { return this->column_types; }

    template<typename T>
    T get(int col) const;

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
 * @brief Wrapper around a SQLite3 database
 */
class sqlite
{
  private:
    sqlite_t conn = nullptr;
    stmt_t   stmt = nullptr;

  public:
    sqlite() = default;

    /**
     * @brief Construct a new sqlite object from a path to database
     *
     * @param path File path to sqlite3 database
     */
    sqlite(const std::string& path);

    sqlite(sqlite& db);
    sqlite& operator=(sqlite& db);

    /**
     * @brief Take ownership of a sqlite3 database
     *
     * @param db sqlite3 database object
     */
    sqlite(sqlite&& db);

    /**
     * @brief Move assignment operator
     *
     * @param db sqlite3 database object
     * @return sqlite& reference to sqlite3 database
     */
    sqlite& operator=(sqlite&& db);

    /**
     * @brief Return the originating sqlite3 database pointer
     *
     * @return sqlite3*
     */
    sqlite3* connection() const noexcept;

    /**
     * @brief Check if SQLite database contains a given table
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