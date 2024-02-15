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
    }

    void TearDown() override {};

    std::string path;
    std::string path2;
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
