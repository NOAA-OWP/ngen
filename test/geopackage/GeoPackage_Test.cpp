#include <boost/geometry/io/wkt/write.hpp>
#include <gtest/gtest.h>

#include "geopackage.hpp"
#include "FileChecker.h"

class GeoPackage_Test : public ::testing::Test
{
  protected:
    void SetUp() override 
    {
        this->path = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example.gpkg",
            "../test/data/geopackage/example.gpkg",
            "../../test/data/geopackage/example.gpkg"
        });

        if (this->path.empty()) {
            FAIL() << "can't find test/data/geopackage/example.gpkg";
        }

        this->path2 = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example_3857.gpkg",
            "../test/data/geopackage/example_3857.gpkg",
            "../../test/data/geopackage/example_3857.gpkg"
        });

        if (this->path2.empty()) {
            FAIL() << "can't find test/data/geopackage/example_3857.gpkg";
        }

        this->path_aux = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example_aux.gpkg",
            "../test/data/geopackage/example_aux.gpkg",
            "../../test/data/geopackage/example_aux.gpkg"
        });

        if (this->path_aux.empty()) {
            FAIL() << "can't find test/data/geopackage/example_aux.gpkg";
        }
    }

    void TearDown() override {};

    std::string path;
    std::string path2;
    std::string path_aux;
};

TEST_F(GeoPackage_Test, geopackage_read_test)
{
    const auto gpkg = ngen::geopackage::read(this->path, "test", {});
    EXPECT_NE(gpkg->find("First"), -1);
    EXPECT_NE(gpkg->find("Second"), -1);
    const auto bbox = gpkg->get_bounding_box();
    EXPECT_EQ(bbox.size(), 4);
    EXPECT_EQ(bbox[0], 102.0);
    EXPECT_EQ(bbox[1], 0.0);
    EXPECT_EQ(bbox[2], 105.0);
    EXPECT_EQ(bbox[3], 1.0);
    EXPECT_EQ(2, gpkg->get_size());

    const auto& first = gpkg->get_feature(0);
    const auto& third = gpkg->get_feature(2);
    EXPECT_EQ(first->get_id(), "First");

    const auto point = boost::get<geojson::coordinate_t>(first->geometry());
    EXPECT_EQ(point.get<0>(), 102.0);
    EXPECT_EQ(point.get<1>(), 0.5);

    ASSERT_TRUE(third == nullptr);
}

TEST_F(GeoPackage_Test, geopackage_idsubset_test)
{
    const auto gpkg = ngen::geopackage::read(this->path, "test", { "First" });
    EXPECT_NE(gpkg->find("First"), -1);
    EXPECT_EQ(gpkg->find("Second"), -1);

    const auto& first = gpkg->get_feature(0);
    EXPECT_EQ(first->get_id(), "First");
    const auto point = boost::get<geojson::coordinate_t>(first->geometry());
    EXPECT_EQ(point.get<0>(), 102.0);
    EXPECT_EQ(point.get<1>(), 0.5);

    ASSERT_TRUE(gpkg->get_feature(1) == nullptr);
}

// this test is essentially the same as the above, however, the coordinates
// are stored in EPSG:3857. When read in, they should convert to EPSG:4326.
TEST_F(GeoPackage_Test, geopackage_projection_test)
{
    const auto gpkg = ngen::geopackage::read(this->path2, "example_3857", {});
    EXPECT_NE(gpkg->find("First"), -1);
    EXPECT_NE(gpkg->find("Second"), -1);
    const auto bbox = gpkg->get_bounding_box();
    EXPECT_EQ(bbox.size(), 4);
    EXPECT_NEAR(bbox[0], 102.0, 0.0001);
    EXPECT_NEAR(bbox[1], 0.0, 0.0001);
    EXPECT_NEAR(bbox[2], 105.0, 0.0001);
    EXPECT_NEAR(bbox[3], 1.0, 0.0001);
    EXPECT_EQ(2, gpkg->get_size());

    const auto& first = gpkg->get_feature(0);
    const auto& third = gpkg->get_feature(2);
    EXPECT_EQ(first->get_id(), "First");

    const auto point = boost::get<geojson::coordinate_t>(first->geometry());
    EXPECT_NEAR(point.get<0>(), 102.0, 0.0001);
    EXPECT_NEAR(point.get<1>(), 0.5, 0.0001);

    ASSERT_TRUE(third == nullptr);
}

// example_aux.gpkg is a copy of example.gpkg carrying four auxiliary attribute tables;
// see test/data/geopackage/example_aux.sql for how it is derived and why.
TEST_F(GeoPackage_Test, geopackage_aux_fixture_test)
{
    const auto gpkg = ngen::geopackage::read(this->path_aux, "test", {});
    EXPECT_EQ(2, gpkg->get_size());
    EXPECT_NE(gpkg->find("First"), -1);
    EXPECT_NE(gpkg->find("Second"), -1);

    const std::vector<std::string> expected_tables = {
        "aux_params_blob", "aux_params_dupe", "aux_params_one", "aux_params_two"
    };

    ngen::sqlite::database db{this->path_aux};
    for (const std::string& table : expected_tables) {
        EXPECT_TRUE(db.contains(table)) << table;
    }

    auto contents = db.query(
        "SELECT table_name FROM gpkg_contents WHERE data_type = 'attributes' ORDER BY table_name"
    );
    for (const std::string& table : expected_tables) {
        contents.next();
        ASSERT_FALSE(contents.done());
        EXPECT_EQ(contents.get<std::string>(0), table);
    }
    contents.next();
    EXPECT_TRUE(contents.done());

    // one column per supported type, a NULL cell for "First", no row for "Second"
    auto one = db.query("SELECT * FROM aux_params_one WHERE divide_id = 'First'");
    one.next();
    ASSERT_FALSE(one.done());
    EXPECT_EQ(one.get<int>("int_value"), 42);
    EXPECT_DOUBLE_EQ(one.get<double>("real_value"), 3.5);
    EXPECT_EQ(one.get<std::string>("text_value"), "alpha");
    const int sparse_column = one.find("sparse_value");
    ASSERT_NE(sparse_column, -1);
    EXPECT_EQ(one.types()[sparse_column], SQLITE_NULL);
    one.next();
    EXPECT_TRUE(one.done());

    auto missing = db.query("SELECT * FROM aux_params_one WHERE divide_id = 'Second'");
    missing.next();
    EXPECT_TRUE(missing.done());

    // a row for a divide that is not a feature of the layer
    auto extra = db.query("SELECT * FROM aux_params_one WHERE divide_id = 'Third'");
    extra.next();
    EXPECT_FALSE(extra.done());

    // non-default key column, full feature coverage, column names shared with table one
    auto two = db.query("SELECT * FROM aux_params_two ORDER BY catchment_id");
    two.next();
    ASSERT_FALSE(two.done());
    EXPECT_EQ(two.get<std::string>("catchment_id"), "First");
    EXPECT_DOUBLE_EQ(two.get<double>("real_value"), 1.5);
    EXPECT_EQ(two.get<std::string>("text_value"), "beta");
    EXPECT_EQ(two.get<std::string>("donor_id"), "gauge-01");
    two.next();
    ASSERT_FALSE(two.done());
    EXPECT_EQ(two.get<std::string>("catchment_id"), "Second");
    two.next();
    EXPECT_TRUE(two.done());

    // a cell of a type no property can hold, beside one that can, for both features
    auto blob = db.query("SELECT * FROM aux_params_blob ORDER BY divide_id");
    blob.next();
    ASSERT_FALSE(blob.done());
    EXPECT_EQ(blob.get<std::string>("divide_id"), "First");
    const int blob_column = blob.find("blob_value");
    ASSERT_NE(blob_column, -1);
    EXPECT_EQ(blob.types()[blob_column], SQLITE_BLOB);
    EXPECT_EQ(blob.get<int>("int_value"), 11);
    blob.next();
    ASSERT_FALSE(blob.done());
    EXPECT_EQ(blob.get<std::string>("divide_id"), "Second");
    blob.next();
    EXPECT_TRUE(blob.done());

    // two rows for "First", so no scan order makes its value definite, and one for "Second"
    auto dupe = db.query("SELECT COUNT(*) FROM aux_params_dupe WHERE divide_id = 'First'");
    dupe.next();
    ASSERT_FALSE(dupe.done());
    EXPECT_EQ(dupe.get<int>(0), 2);

    auto dupe_other = db.query("SELECT COUNT(*) FROM aux_params_dupe WHERE divide_id = 'Second'");
    dupe_other.next();
    ASSERT_FALSE(dupe_other.done());
    EXPECT_EQ(dupe_other.get<int>(0), 1);
}

// The one gate every interpolated table name passes through, on the read and join paths alike.
TEST_F(GeoPackage_Test, geopackage_quote_table_name_test)
{
    EXPECT_EQ(ngen::geopackage::quote_table_name("divides"), "\"divides\"");
    EXPECT_EQ(ngen::geopackage::quote_table_name("model attributes-2"), "\"model attributes-2\"");

    EXPECT_THROW(ngen::geopackage::quote_table_name("sqlite_master"), std::runtime_error);
    EXPECT_THROW(ngen::geopackage::quote_table_name("';"), std::runtime_error);

    // quoting, rather than the character check, is what keeps a name with punctuation inert
    EXPECT_EQ(ngen::geopackage::quote_table_name("odd\"name"), "\"odd\"\"name\"");
}
