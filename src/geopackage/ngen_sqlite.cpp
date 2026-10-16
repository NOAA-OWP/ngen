#include "ngen_sqlite.hpp"

#include <stdexcept>
#include <algorithm>
#include <string>
#include <iostream>

#include <NGenConfig.h>

namespace ngen {
namespace sqlite {

// ngen::sqlite::sqlite_error =================================================

//! error codes: https://www.sqlite.org/rescode.html
sqlite_error::sqlite_error(const std::string& origin_func, int code, const std::string& extra)
  : std::runtime_error(
    origin_func + " returned code " + std::to_string(code)
        + " (msg: " + sqlite3_errstr(code) + ")"
        + (extra.empty() ? "" : " " + extra)
  ){}

const auto sqlite_not_started_error = sqlite_error{
    "sqlite iteration is has not started, get() is not callable "
    "(call iterator::next() before)"
};

const auto sqlite_already_done_error = sqlite_error{
    "sqlite iteration is done, get() is not callable"
};

// ngen::sqlite::database::iterator ===========================================

database::iterator::iterator(stmt_t&& stmt)
  : stmt_(std::move(stmt))
{
    next();
    ncol_ = sqlite3_column_count(ptr_());
    names_.reserve(ncol_);
    types_.reserve(ncol_);

    for (int i = 0; i < ncol_; i++) {
        names_.emplace_back(sqlite3_column_name(ptr_(), i));
        types_.emplace_back(sqlite3_column_type(ptr_(), i));
    }

    restart();
}

auto database::iterator::ptr_() const noexcept -> sqlite3_stmt*
{
    return stmt_.get();
}

auto database::iterator::done() const noexcept -> bool
{
    return done_;
}

auto database::iterator::next() -> iterator&
{
    if (!done_) {
        const int code = sqlite3_step(ptr_());

        if (code == SQLITE_DONE) {
            done_ = true;
        } else if( code == SQLITE_ROW){
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
            for (int i = 0; i < ncol_ && i < types_.size(); i++) {
                types_[i] = (sqlite3_column_type(ptr_(), i));
            }
        } else {
            throw sqlite_error{"sqlite3_step", code};
        }

        step_++;
    }

    return *this;
}

auto database::iterator::restart() -> iterator&
{
    sqlite3_reset(ptr_());
    step_ = -1;
    done_ = false;
    return *this;
}

auto database::iterator::current_row() const noexcept -> int
{
    return step_;
}

auto database::iterator::num_columns() const noexcept -> int
{
    return ncol_;
}

auto database::iterator::find(const std::string& name) const noexcept -> int
{
    const auto pos  = std::find(names_.begin(), names_.end(), name);
    const auto dist = std::distance(names_.begin(), pos);
    return dist >= names_.size() ? -1 : dist;
}

auto database::iterator::columns() const noexcept
  -> const boost::span<const std::string>
{
    return names_;
}

auto database::iterator::types() const noexcept
  -> const boost::span<const int>
{
    return types_;
}

void database::iterator::handle_get_index_(int col) const
{
    if (done()) {
        throw sqlite_already_done_error;
    }

    if (current_row() == -1) {
        throw sqlite_not_started_error;
    }

    if (col < 0 || col >= ncol_) {
        throw std::out_of_range(
            "column " + std::to_string(col) + " out of range of "
                      + std::to_string(ncol_) + " columns"
        );
    }
}

template<>
auto database::iterator::get<double>(int col) const
  -> double
{
    handle_get_index_(col);
    return sqlite3_column_double(ptr_(), col);
}

template<>
auto database::iterator::get<int>(int col) const
  -> int
{
    handle_get_index_(col);
    return sqlite3_column_int(ptr_(), col);
}

template<>
auto database::iterator::get<std::string>(int col) const
  -> std::string
{
    handle_get_index_(col);
    // TODO: this won't work with non-ASCII text
    int size = sqlite3_column_bytes(ptr_(), col);
    const unsigned char* ptr = sqlite3_column_text(ptr_(), col);
    return { ptr, ptr + size };
}

template<>
auto database::iterator::get<std::vector<uint8_t>>(int col) const
  -> std::vector<uint8_t>
{
    handle_get_index_(col);
    int size = sqlite3_column_bytes(ptr_(), col);
    auto ptr = static_cast<const uint8_t*>(sqlite3_column_blob(ptr_(), col));
    return {ptr, ptr + size};
}

// ngen::sqlite::database =====================================================

namespace {
    //! Percent-encode a filesystem path for use in a SQLite "file:" URI.
    //! Unreserved characters (RFC 3986) and '/' are left as-is; everything else
    //! (notably spaces, '?', '#', '%') is percent-encoded so the path survives
    //! SQLite's URI parsing. SQLite decodes %XX back to the original byte.
    std::string uri_encode_path(const std::string& path)
    {
        static const char hex[] = "0123456789ABCDEF";
        std::string out;
        out.reserve(path.size() + 8); // headroom for a few 3-byte escapes

        // Iterate as unsigned char so bytes >= 0x80 (e.g. UTF-8 path components)
        // are treated as 0..255 and encoded correctly, not as negative values.
        for (unsigned char c : path) {
            // Leave RFC 3986 "unreserved" characters as-is, plus '/', which we
            // keep literal so it stays a path separator rather than becoming
            // %2F. Everything else (spaces, and URI-significant characters like
            // '?', '#', '%') must be escaped so SQLite's URI parser sees the
            // original path when it decodes.
            const bool unreserved =
                (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') ||
                c == '-' || c == '.' || c == '_' || c == '~' || c == '/';
            if (unreserved) {
                out.push_back(static_cast<char>(c));
            } else {
                // Emit the byte as "%HH": '%' then its two uppercase hex
                // digits, high (c / 16) then low (c % 16).
                out.push_back('%');
                out.push_back(hex[c / 16]);
                out.push_back(hex[c % 16]);
            }
        }
        return out;
    }

    //! Apply best-effort read-tuning pragmas to a freshly opened, read-only
    //! connection and warn once if the library clamped mmap_size. See
    //! NGEN_SQLITE_MMAP_SIZE / NGEN_SQLITE_CACHE_SIZE.
    void apply_read_tuning(sqlite3* conn)
    {
        // Best-effort read tuning; non-fatal if the build doesn't support a pragma.
        //  - mmap_size: memory-map reads so SQLite reads pages directly from the OS
        //    page cache without copying each into a per-connection buffer, and
        //    without a read() syscall per page. This is a cap on the bytes (the
        //    first N, from the start of the file) eligible for mmap, not an
        //    allocation; build-time configurable via NGEN_SQLITE_MMAP_SIZE
        //    (0 disables it).
        //  - temp_store=MEMORY: keep transient structures (e.g. the ephemeral table
        //    SQLite builds for "WHERE id IN (...)" subset queries) off disk.
        //  - cache_size: per-connection page cache for pages served by read()
        //    rather than mmap (offsets beyond the window); build-time configurable
        //    via NGEN_SQLITE_CACHE_SIZE.
        const long long requested_mmap = ngen::exec_info::sqlite_mmap_size;
        const std::string mmap_pragma  = "PRAGMA mmap_size="  + std::to_string(requested_mmap) + ";";
        const std::string cache_pragma = "PRAGMA cache_size=" + std::to_string(ngen::exec_info::sqlite_cache_size) + ";";
        sqlite3_exec(conn, mmap_pragma.c_str(),  nullptr, nullptr, nullptr);
        sqlite3_exec(conn, "PRAGMA temp_store=MEMORY;", nullptr, nullptr, nullptr);
        sqlite3_exec(conn, cache_pragma.c_str(), nullptr, nullptr, nullptr);

        // Align with the library: SQLite clamps mmap_size to its build-time
        // SQLITE_MAX_MMAP_SIZE, which can silently leave the tail of a large gpkg on
        // conventional read(). Read back the effective value and warn once if it was
        // reduced below the request.
        if (requested_mmap > 0) {
            long long effective_mmap = -1;
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(conn, "PRAGMA mmap_size;", -1, &stmt, nullptr) == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    effective_mmap = sqlite3_column_int64(stmt, 0);
                }
                sqlite3_finalize(stmt);
            }
#if !NGEN_QUIET
            static bool warned_mmap_clamped = false;
            if (!warned_mmap_clamped && effective_mmap >= 0 && effective_mmap < requested_mmap) {
                warned_mmap_clamped = true;
                std::cerr << "WARNING: requested SQLite mmap_size " << requested_mmap
                          << " bytes was clamped to " << effective_mmap
                          << " by the linked library's SQLITE_MAX_MMAP_SIZE; geopackages larger than "
                          << effective_mmap << " bytes will not be fully memory-mapped."
                          << " Rebuild SQLite with a larger SQLITE_MAX_MMAP_SIZE to map the whole file."
                          << std::endl;
            }
#endif
        }
    }
}

database::database(const std::string& path)
{
    // Open read-only AND immutable. The hydrofabric file does not change during
    // a run, so 'immutable' lets SQLite skip all file locking and change
    // detection on every open -- a win for any read-only access, and especially
    // for the engine's many-readers, one-file MPI pattern, where advisory
    // locking on a shared/parallel filesystem is the real bottleneck (not
    // bandwidth).
    const std::string uri = "file:" + uri_encode_path(path) + "?immutable=1";
    sqlite3*  conn = nullptr;
    const int code = sqlite3_open_v2(uri.c_str(), &conn, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, nullptr);
    if (code != SQLITE_OK) {
        throw sqlite_error{"sqlite3_open_v2", code};
    }
    conn_ = sqlite_t{conn};

    apply_read_tuning(conn_.get());
}

auto database::connection() const noexcept -> sqlite3*
{
    return conn_.get();
}

auto database::contains(const std::string& table) -> bool
{
    auto q = query("SELECT EXISTS(SELECT 1 FROM sqlite_master WHERE type='table' AND name=?)", table);
    q.next();
    return q.get<int>(0);
}

auto database::query(
    const std::string& statement,
    const boost::span<const std::string> binds
) -> iterator
{
    sqlite3_stmt* stmt = nullptr;
    const int code = sqlite3_prepare_v2(
        connection(),
        statement.c_str(),
        statement.length() + 1,
        &stmt,
        nullptr
    );

    if (code != SQLITE_OK) {
        throw sqlite_error{"sqlite3_prepare_v2", code};
    }

    if (!binds.empty()) {
        for (int i = 0; i < binds.size(); i++) {
            const int bind_code = sqlite3_bind_text(stmt, i + 1, binds[i].c_str(), -1, SQLITE_TRANSIENT);
            if (bind_code != SQLITE_OK) {
                throw sqlite_error{"sqlite3_bind_text", code};
            }
        }
    }

    return iterator{stmt_t{stmt}};
}

} // namespace sqlite
} // namespace ngen
