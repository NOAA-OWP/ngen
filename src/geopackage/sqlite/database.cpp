#include "SQLite.hpp"

using namespace geopackage;

sqlite::sqlite(const std::string& path)
{
    sqlite3* conn;
    int      code = sqlite3_open_v2(path.c_str(), &conn, SQLITE_OPEN_READONLY, NULL);
    if (code != SQLITE_OK) {
        throw sqlite_error("sqlite3_open_v2", code);
    }
    this->conn = sqlite_t(conn);
}

sqlite::sqlite(sqlite& db)
{
    this->conn = std::move(db.conn);
    this->stmt = db.stmt;
}

sqlite& sqlite::operator=(sqlite& db)
{
    this->conn = std::move(db.conn);
    this->stmt = db.stmt;
    return *this;
}

sqlite::sqlite(sqlite&& db)
{
    this->conn = std::move(db.conn);
    this->stmt = db.stmt;
}

sqlite& sqlite::operator=(sqlite&& db)
{
    this->conn = std::move(db.conn);
    this->stmt = db.stmt;
    return *this;
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
        throw sqlite_error("sqlite3_prepare_v2", code, "(query: " + statement + ")");
    }

    this->stmt = stmt_t(stmt, sqlite_deleter{});
    return sqlite_iter(this->stmt);
}

sqlite_iter sqlite::query(const std::string& statement, const std::vector<std::string>& binds)
{
    sqlite3_stmt* stmt;
    const auto    cstmt = statement.c_str();
    const int     code  = sqlite3_prepare_v2(this->connection(), cstmt, statement.length() + 1, &stmt, NULL);

    if (code != SQLITE_OK) {
        throw sqlite_error("sqlite3_prepare_v2", code, "(query: " + statement + ")");
    }

    if (!binds.empty()) {
        for (size_t i = 0; i < binds.size(); i++) {
            const int code = sqlite3_bind_text(stmt, i + 1, binds[i].c_str(), -1, SQLITE_TRANSIENT);
            if (code != SQLITE_OK) {
                throw sqlite_error("sqlite3_bind_text", code);
            }
        }
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
        throw sqlite_error("sqlite3_prepare_v2", code, "(query: " + statement + ")");
    }

    std::vector<std::string> binds{ { params... } };
    for (size_t i = 0; i < binds.size(); i++) {
        const int code = sqlite3_bind_text(stmt, i + 1, binds[i].c_str(), -1, SQLITE_TRANSIENT);
        if (code != SQLITE_OK) {
            throw sqlite_error("sqlite3_bind_text", code);
        }
    }

    this->stmt        = stmt_t(stmt, sqlite_deleter{});
    return sqlite_iter(this->stmt);
}
