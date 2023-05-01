#ifndef NGEN_GEOPACKAGE_SQLITE_H
#define NGEN_GEOPACKAGE_SQLITE_H


#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <sqlite3.h>

namespace geopackage {

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
static inline std::runtime_error sqlite_error(const std::string& f, int code)
{
    std::string errmsg = f + " returned code " + std::to_string(code);
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

  public:
    sqlite_iter(stmt_t stmt);
    ~sqlite_iter();
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
    T get(const std::string& name) const;
};

inline sqlite_iter::sqlite_iter(stmt_t stmt)
  : stmt(stmt)
{
    this->column_count = sqlite3_column_count(this->ptr());
    this->column_names = std::vector<std::string>();
    this->column_names.reserve(this->column_count);
    this->column_types = std::vector<int>();
    this->column_types.reserve(this->column_count);

    for (int i = 0; i < this->column_count; i++) {
        this->column_names.push_back(sqlite3_column_name(this->ptr(), i));
        this->column_types.push_back(sqlite3_column_type(this->ptr(), i));
    }
};

inline sqlite_iter::~sqlite_iter()
{
    this->stmt.reset();
}

inline sqlite3_stmt* sqlite_iter::ptr() const noexcept
{
    return this->stmt.get();
}

inline bool sqlite_iter::done() const noexcept
{
    return this->iteration_finished;
}

inline sqlite_iter& sqlite_iter::next()
{
    if (!this->done()) {
        const int returncode = sqlite3_step(this->ptr());
        if (returncode == SQLITE_DONE) {
            this->iteration_finished = true;
        }
        this->iteration_step++;
    }

    return *this;
}

inline sqlite_iter& sqlite_iter::restart()
{
    sqlite3_reset(this->ptr());
    this->iteration_step = -1;
    return *this;
}

inline void sqlite_iter::close()
{
    this->~sqlite_iter();
}

inline int sqlite_iter::current_row() const noexcept
{
    return this->iteration_step;
}

inline int sqlite_iter::num_columns() const noexcept
{
    return this->column_count;
}

inline int sqlite_iter::column_index(const std::string& name) const noexcept
{
    const ptrdiff_t pos =
      std::distance(this->column_names.begin(), std::find(this->column_names.begin(), this->column_names.end(), name));

    return pos >= this->column_names.size() ? -1 : pos;
}

template<>
inline std::vector<uint8_t> sqlite_iter::get<std::vector<uint8_t>>(int col) const
{
    return *reinterpret_cast<const std::vector<uint8_t>*>(sqlite3_column_blob(this->ptr(), col));
}

template<>
inline double sqlite_iter::get<double>(int col) const
{
    return sqlite3_column_double(this->ptr(), col);
}

template<>
inline int sqlite_iter::get<int>(int col) const
{
    return sqlite3_column_int(this->ptr(), col);
}

template<>
inline std::string sqlite_iter::get<std::string>(int col) const
{
    // TODO: this won't work with non-ASCII text
    return std::string(reinterpret_cast<const char*>(sqlite3_column_text(this->ptr(), col)));
}

template<typename T>
inline T sqlite_iter::get(const std::string& name) const
{
    const int index = this->column_index(name);
    return this->get<T>(index);
}

/**
 * @brief Wrapper around SQLite3 Databases
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
     * @brief Destroy the sqlite object
     */
    ~sqlite();

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
     * @return read-only SQLite row iterator (see: [sqlite_iter])
     */
    sqlite_iter query(const std::string& statement);

    /**
     * @brief TODO!!!
     *
     * @param statement
     * @param params
     * @return sqlite_iter*
     */
    template<typename... T>
    sqlite_iter query(const std::string& statement, T const&... params);
};

inline sqlite::sqlite(const std::string& path)
{
    sqlite3* conn;
    int      code = sqlite3_open_v2(path.c_str(), &conn, SQLITE_OPEN_READONLY, NULL);
    if (code != SQLITE_OK) {
        throw sqlite_error("sqlite3_open_v2", code);
    }
    this->conn = sqlite_t(conn);
}

inline sqlite::sqlite(sqlite& db)
{
    this->conn = std::move(db.conn);
    this->stmt = db.stmt;
}

inline sqlite& sqlite::operator=(sqlite& db)
{
    this->conn = std::move(db.conn);
    this->stmt = db.stmt;
    return *this;
}

inline sqlite::sqlite(sqlite&& db)
{
    this->conn = std::move(db.conn);
    this->stmt = db.stmt;
}

inline sqlite& sqlite::operator=(sqlite&& db)
{
    this->conn = std::move(db.conn);
    this->stmt = db.stmt;
    return *this;
}

inline sqlite::~sqlite()
{
    this->stmt.reset();
    this->conn.reset();
}

inline sqlite3* sqlite::connection() const noexcept
{
    return this->conn.get();
}

inline bool sqlite::has_table(const std::string& table) noexcept
{
    auto q = this->query("SELECT 1 from sqlite_master WHERE type='table' AND name = ?", table);
    q.next();
    return static_cast<bool>(q.get<int>(0));
};

inline sqlite_iter sqlite::query(const std::string& statement)
{
    sqlite3_stmt* stmt;
    const auto    cstmt = statement.c_str();
    const int     code  = sqlite3_prepare_v2(this->connection(), cstmt, statement.length() + 1, &stmt, NULL);

    if (code != SQLITE_OK) {
        // something happened, can probably switch on result codes
        // https://www.sqlite.org/rescode.html
        throw sqlite_error("sqlite3_prepare_v2", code);
    }

    this->stmt        = stmt_t(stmt, sqlite_deleter{});
    return sqlite_iter(this->stmt);
}

template<typename... T>
inline sqlite_iter sqlite::query(const std::string& statement, T const&... params)
{
    sqlite3_stmt* stmt;
    const auto    cstmt = statement.c_str();
    const int     code  = sqlite3_prepare_v2(this->connection(), cstmt, statement.length() + 1, &stmt, NULL);

    if (code != SQLITE_OK) {
        throw sqlite_error("sqlite3_prepare_v2", code);
    }

    std::vector<std::string> binds{ { params... } };
    for (size_t i = 0; i < binds.size(); i++) {
        sqlite3_bind_text(stmt, i + 1, binds[i].c_str(), -1, SQLITE_STATIC);
    }

    this->stmt        = stmt_t(stmt, sqlite_deleter{});
    return sqlite_iter(this->stmt);
}

} // namespace geopackage

#endif // NGEN_GEOPACKAGE_SQLITE_H