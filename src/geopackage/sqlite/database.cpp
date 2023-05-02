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
    auto q = this->query("SELECT 1 from sqlite_master WHERE type='table' AND name = ?", table);

    q.next();
    if (q.done()) {
        return false;
    } else {
        return static_cast<bool>(q.get<int>(0));
    }
};

sqlite_iter sqlite::query(const std::string& statement)
{
    sqlite3_stmt* stmt;
    const auto    cstmt = statement.c_str();
    const int     code  = sqlite3_prepare_v2(this->connection(), cstmt, statement.length() + 1, &stmt, NULL);

    if (code != SQLITE_OK) {
        // something happened, can probably switch on result codes
        // https://www.sqlite.org/rescode.html
        throw sqlite_error("[with statement: " + statement + "] " + "sqlite3_prepare_v2", code);
    }

    this->stmt        = stmt_t(stmt, sqlite_deleter{});
    return sqlite_iter(this->stmt);
}