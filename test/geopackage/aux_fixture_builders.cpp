#include "aux_fixture_builders.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>
#include <sqlite3.h>

#include "utilities/FileChecker.h"

namespace ngen {
namespace geopackage {
namespace fixtures {

namespace {

//! Fixed `gpkg_contents.last_change` value, so a rebuilt fixture stays byte-identical to the last.
constexpr const char* LAST_CHANGE = "2026-08-19T00:00:00.000Z";

/**
 * Locate the committed `example.gpkg` these fixtures are derived from.
 *
 * @return Path to the readable copy found
 * @throws std::runtime_error if no candidate path is readable
 */
std::string find_example_gpkg()
{
    const std::string path = utils::FileChecker::find_first_readable({
        "test/data/geopackage/example.gpkg",
        "../test/data/geopackage/example.gpkg",
        "../../test/data/geopackage/example.gpkg"
    });

    if (path.empty()) {
        throw std::runtime_error("fixture: can't find test/data/geopackage/example.gpkg to copy");
    }

    return path;
}

/**
 * An open, writable copy of a GeoPackage, closed however the enclosing scope exits.
 *
 * Fixtures are written through raw SQLite rather than ngen::sqlite::database, which opens
 * read-only. Every statement is checked, so a broken fixture fails loudly at setup rather than as
 * a puzzling assertion later.
 */
class fixture_db
{
  public:
    /**
     * Copy @p source over @p destination and open the copy for writing.
     *
     * @param[in] source Committed GeoPackage to derive from
     * @param[in] destination File to write, replaced if it already exists
     * @throws std::runtime_error if the copy fails or the result cannot be opened
     */
    fixture_db(const std::string& source, const std::string& destination)
      : path_(destination)
    {
        std::error_code copy_error;
        std::filesystem::copy_file(
            source, destination, std::filesystem::copy_options::overwrite_existing, copy_error
        );
        if (copy_error) {
            throw std::runtime_error(
                "fixture " + destination + ": cannot copy from " + source + ": " + copy_error.message()
            );
        }

        if (sqlite3_open(destination.c_str(), &db_) != SQLITE_OK) {
            const std::string message = db_ == nullptr ? "out of memory" : sqlite3_errmsg(db_);
            sqlite3_close(db_);
            throw std::runtime_error("fixture " + destination + ": cannot open for writing: " + message);
        }
    }

    ~fixture_db()
    {
        sqlite3_close(db_);
    }

    fixture_db(const fixture_db&)            = delete;
    fixture_db& operator=(const fixture_db&) = delete;

    /**
     * Run one statement with no parameters.
     *
     * @param[in] sql Statement to execute
     * @throws std::runtime_error if SQLite rejects the statement
     */
    void exec(const std::string& sql)
    {
        char* errmsg = nullptr;
        if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
            const std::string message = errmsg == nullptr ? "unknown error" : errmsg;
            sqlite3_free(errmsg);
            throw std::runtime_error("fixture " + path_ + ": " + message + " in: " + sql);
        }
    }

    /**
     * Register a table in `gpkg_contents` as an attributes table.
     *
     * An attributes table carries no geometry, so it is listed with a NULL srs_id and no bounding
     * box, which is how the hydrofabric preview's own regionalization tables are registered.
     *
     * @param[in] table Table to register
     * @throws std::runtime_error if SQLite rejects the statement
     */
    void register_attributes_table(const std::string& table)
    {
        exec(
            "INSERT INTO \"gpkg_contents\""
            " (\"table_name\", \"data_type\", \"identifier\", \"description\", \"last_change\")"
            " VALUES ('" + table + "', 'attributes', '" + table + "',"
            " 'Auxiliary attribute table fixture', '" + LAST_CHANGE + "')"
        );
    }

  private:
    const std::string path_;
    sqlite3* db_ = nullptr;
};

/**
 * Path to write a named fixture to.
 *
 * @param[in] name File name within the test temp directory
 * @return Absolute path to write
 */
std::string fixture_path(const std::string& name)
{
    return std::string(::testing::TempDir()) + "/" + name;
}

} // namespace

std::string write_example_aux()
{
    const std::string path = fixture_path("example_aux.gpkg");
    fixture_db db{find_example_gpkg(), path};

    // Default key column, one column of each supported type, a NULL cell for "First", no row at
    // all for "Second", and a row for "Third", which is not a feature of the layer.
    db.exec(
        "CREATE TABLE \"aux_params_one\" ("
        "  \"divide_id\"    TEXT,"
        "  \"int_value\"    INTEGER,"
        "  \"real_value\"   REAL,"
        "  \"text_value\"   TEXT,"
        "  \"sparse_value\" REAL"
        ")"
    );
    db.exec(
        "INSERT INTO \"aux_params_one\""
        " (\"divide_id\", \"int_value\", \"real_value\", \"text_value\", \"sparse_value\")"
        " VALUES ('First', 42, 3.5, 'alpha', NULL),"
        "        ('Third', 7, 1.25, 'gamma', 9.75)"
    );
    db.register_attributes_table("aux_params_one");

    // Non-default key column, full coverage of the layer's features, and two column names shared
    // with the table above, so joining both is only unambiguous because of prefixing.
    db.exec(
        "CREATE TABLE \"aux_params_two\" ("
        "  \"catchment_id\" TEXT,"
        "  \"real_value\"   REAL,"
        "  \"text_value\"   TEXT,"
        "  \"donor_id\"     TEXT"
        ")"
    );
    db.exec(
        "INSERT INTO \"aux_params_two\""
        " (\"catchment_id\", \"real_value\", \"text_value\", \"donor_id\")"
        " VALUES ('First', 1.5, 'beta', 'gauge-01'),"
        "        ('Second', 2.5, 'delta', 'gauge-02')"
    );
    db.register_attributes_table("aux_params_two");

    return path;
}

} // namespace fixtures
} // namespace geopackage
} // namespace ngen
