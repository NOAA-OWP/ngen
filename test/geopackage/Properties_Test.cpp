#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "geopackage.hpp"
#include "FileChecker.h"

// Direct coverage of ngen::geopackage::get_property, the single point where a GeoPackage cell
// becomes a JSON property. Rows come from test/data/geopackage/example_aux.gpkg, described in
// test/data/geopackage/example_aux.sql, which between them hold a cell of every storage class.
class Properties_Test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const std::string path = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example_aux.gpkg",
            "../test/data/geopackage/example_aux.gpkg",
            "../../test/data/geopackage/example_aux.gpkg"
        });

        if (path.empty()) {
            FAIL() << "can't find test/data/geopackage/example_aux.gpkg";
        }

        // The iterators below read through this connection, so it outlives them.
        this->db = std::make_unique<ngen::sqlite::database>(path);
    }

    //! The "First" row of @p table, positioned so its columns can be read.
    ngen::sqlite::database::iterator first_row(const std::string& table)
    {
        ngen::sqlite::database::iterator row =
            this->db->query("SELECT * FROM " + table + " WHERE divide_id = 'First'");
        row.next();
        return row;
    }

    //! The storage class SQLite reports for @p column of @p row. A column the fixture no longer
    //! has yields -1, which matches no SQLITE_* constant, so the caller's comparison fails.
    int type_of(ngen::sqlite::database::iterator& row, const std::string& column)
    {
        const int index = row.find(column);
        if (index < 0) {
            ADD_FAILURE() << "fixture drift: no column `" << column << "`";
            return -1;
        }
        return row.types()[index];
    }

    //! Convert @p column of @p row using the storage class SQLite reports for that cell, rather
    //! than a hard-coded constant, so the pairing the readers rely on is what gets exercised.
    geojson::JSONProperty property_of(ngen::sqlite::database::iterator& row, const std::string& column)
    {
        return ngen::geopackage::get_property(row, column, this->type_of(row, column));
    }

    std::unique_ptr<ngen::sqlite::database> db;
};

TEST_F(Properties_Test, integer_column_becomes_a_natural_number)
{
    ngen::sqlite::database::iterator row = this->first_row("aux_params_one");
    const geojson::JSONProperty property = this->property_of(row, "int_value");

    EXPECT_EQ(property.get_type(), geojson::PropertyType::Natural);
    EXPECT_EQ(property.as_natural_number(), 42);
    EXPECT_EQ(property.get_key(), "int_value");
}

TEST_F(Properties_Test, real_column_becomes_a_real_number)
{
    ngen::sqlite::database::iterator row = this->first_row("aux_params_one");
    const geojson::JSONProperty property = this->property_of(row, "real_value");

    EXPECT_EQ(property.get_type(), geojson::PropertyType::Real);
    EXPECT_DOUBLE_EQ(property.as_real_number(), 3.5);
}

TEST_F(Properties_Test, text_column_becomes_a_string)
{
    ngen::sqlite::database::iterator row = this->first_row("aux_params_one");
    const geojson::JSONProperty property = this->property_of(row, "text_value");

    EXPECT_EQ(property.get_type(), geojson::PropertyType::String);
    EXPECT_EQ(property.as_string(), "alpha");
}

// The fallback arm yields the literal string "null" for anything else, so callers that must not
// publish such a cell have to filter on the type themselves, as join_attributes does.
TEST_F(Properties_Test, null_cell_falls_back_to_a_null_string)
{
    ngen::sqlite::database::iterator row = this->first_row("aux_params_one");
    ASSERT_EQ(this->type_of(row, "sparse_value"), SQLITE_NULL);

    const geojson::JSONProperty property = this->property_of(row, "sparse_value");
    EXPECT_EQ(property.get_type(), geojson::PropertyType::String);
    EXPECT_EQ(property.as_string(), "null");
}

TEST_F(Properties_Test, blob_cell_falls_back_to_a_null_string)
{
    ngen::sqlite::database::iterator row = this->first_row("aux_params_blob");
    ASSERT_EQ(this->type_of(row, "blob_value"), SQLITE_BLOB);

    const geojson::JSONProperty property = this->property_of(row, "blob_value");
    EXPECT_EQ(property.get_type(), geojson::PropertyType::String);
    EXPECT_EQ(property.as_string(), "null");
}
