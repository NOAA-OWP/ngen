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

// example_aux.gpkg is a copy of example.gpkg carrying two auxiliary attribute tables;
// see test/data/geopackage/example_aux.sql for how it is derived and why.
TEST_F(GeoPackage_Test, geopackage_aux_fixture_test)
{
    const auto gpkg = ngen::geopackage::read(this->path_aux, "test", {});
    EXPECT_EQ(2, gpkg->get_size());
    EXPECT_NE(gpkg->find("First"), -1);
    EXPECT_NE(gpkg->find("Second"), -1);

    ngen::sqlite::database db{this->path_aux};
    EXPECT_TRUE(db.contains("aux_params_one"));
    EXPECT_TRUE(db.contains("aux_params_two"));

    auto contents = db.query(
        "SELECT table_name FROM gpkg_contents WHERE data_type = 'attributes' ORDER BY table_name"
    );
    contents.next();
    ASSERT_FALSE(contents.done());
    EXPECT_EQ(contents.get<std::string>(0), "aux_params_one");
    contents.next();
    ASSERT_FALSE(contents.done());
    EXPECT_EQ(contents.get<std::string>(0), "aux_params_two");
    contents.next();
    EXPECT_TRUE(contents.done());

    // one column per supported type, a NULL cell for "First", no row for "Second"
    auto one = db.query("SELECT * FROM aux_params_one WHERE divide_id = 'First'");
    one.next();
    ASSERT_FALSE(one.done());
    EXPECT_EQ(one.get<int>("int_value"), 42);
    EXPECT_DOUBLE_EQ(one.get<double>("real_value"), 3.5);
    EXPECT_EQ(one.get<std::string>("text_value"), "alpha");
    EXPECT_EQ(one.types()[one.find("sparse_value")], SQLITE_NULL);
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
}
