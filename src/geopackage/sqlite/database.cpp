#include "NGen_SQLite.hpp"

using namespace geopackage;

/**
 * Get a runtime error based on a function and code.
 *
 * @param f String denoting the function where the error originated
 * @param code sqlite3 result code
 * @param extra additional messages to add to the end of the error
 * @return std::runtime_error
 */
std::runtime_error sqlite_error(const std::string& f, int code, const std::string& extra = "")
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

sqlite::sqlite(const std::string& path)
{
    sqlite3* conn;
    int      code = sqlite3_open_v2(path.c_str(), &conn, SQLITE_OPEN_READONLY, NULL);
    if (code != SQLITE_OK) {
        throw sqlite_error("sqlite3_open_v2", code);
    }
    this->conn = sqlite_t(conn);
}

sqlite3* sqlite::connection() const noexcept
{
    return this->conn.get();
}

bool sqlite::has_table(const std::string& table) noexcept
{
    auto q = this->query("SELECT EXISTS(SELECT 1 from sqlite_master WHERE type='table' AND name=?)", table);
    q.next();
    return q.get<int>(0);
};

sqlite_iter sqlite::query(const std::string& statement)
{
    sqlite3_stmt* stmt;
    const auto    cstmt = statement.c_str();
    const int     code  = sqlite3_prepare_v2(this->connection(), cstmt, statement.length() + 1, &stmt, NULL);

    if (code != SQLITE_OK) {
        // something happened, can probably switch on result codes
        // https://www.sqlite.org/rescode.html
        throw sqlite_error("sqlite3_prepare_v2", code);
    }

    return sqlite_iter(std::move(stmt_t(stmt)));
}

sqlite_iter sqlite::query(const std::string& statement, const std::vector<std::string>& binds)
{
    sqlite3_stmt* stmt;
    const auto    cstmt = statement.c_str();
    const int     code  = sqlite3_prepare_v2(this->connection(), cstmt, statement.length() + 1, &stmt, NULL);

    if (code != SQLITE_OK) {
        throw sqlite_error("sqlite3_prepare_v2", code);
    }

    if (!binds.empty()) {
        for (size_t i = 0; i < binds.size(); i++) {
            const int code = sqlite3_bind_text(stmt, i + 1, binds[i].c_str(), -1, SQLITE_TRANSIENT);
            if (code != SQLITE_OK) {
                throw sqlite_error("sqlite3_bind_text", code);
            }
        }
    }

    return sqlite_iter(std::move(stmt_t(stmt)));
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
        const int code = sqlite3_bind_text(stmt, i + 1, binds[i].c_str(), -1, SQLITE_TRANSIENT);
        if (code != SQLITE_OK) {
            throw sqlite_error("sqlite3_bind_text", code);
        }
    }

    return sqlite_iter(std::move(stmt_t(stmt)));
}
