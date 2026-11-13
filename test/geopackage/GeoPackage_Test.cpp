#include <boost/geometry/io/wkt/write.hpp>
#include <gtest/gtest.h>

#include "GeoPackageReader.hpp"
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
    const auto gpkg = ngen::geopackage::make_reader(this->path).read("test", {});
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

    const auto point = std::get<geojson::coordinate_t>(*first->geometry());
    EXPECT_EQ(point.get<0>(), 102.0);
    EXPECT_EQ(point.get<1>(), 0.5);

    ASSERT_TRUE(third == nullptr);
}

TEST_F(GeoPackage_Test, geopackage_idsubset_test)
{
    const auto gpkg = ngen::geopackage::make_reader(this->path).read("test", { "First" });
    EXPECT_NE(gpkg->find("First"), -1);
    EXPECT_EQ(gpkg->find("Second"), -1);

    const auto& first = gpkg->get_feature(0);
    EXPECT_EQ(first->get_id(), "First");
    const auto point = std::get<geojson::coordinate_t>(*first->geometry());
    EXPECT_EQ(point.get<0>(), 102.0);
    EXPECT_EQ(point.get<1>(), 0.5);

    ASSERT_TRUE(gpkg->get_feature(1) == nullptr);
}

TEST_F(GeoPackage_Test, geopackage_column_selection_test)
{
    const auto with = ngen::geopackage::make_reader(this->path).read("test", {});

    // The id column alone: no other properties, and no geometry
    const auto ids = ngen::geopackage::make_reader(this->path).read("test", {}, "id", std::vector<std::string>{});
    ASSERT_EQ(ids->get_size(), 2);
    EXPECT_TRUE(ids->get_bounding_box().empty());
    for (int i = 0; i < ids->get_size(); ++i) {
        const auto& feature = ids->get_feature(i);
        EXPECT_EQ(feature->get_id(), with->get_feature(i)->get_id());
        EXPECT_EQ(feature->property_keys(), std::vector<std::string>{"id"});
        EXPECT_EQ(feature->get_type(), geojson::FeatureType::GeometryCollection);
        EXPECT_TRUE(feature->get_geometry_collection().empty());
        EXPECT_TRUE(feature->get_bounding_box().empty());
    }

    // Another column besides
    const auto fids = ngen::geopackage::make_reader(this->path).read("test", {}, "id", std::vector<std::string>{"fid"});
    EXPECT_EQ(fids->get_feature(0)->property_keys(), (std::vector<std::string>{"fid", "id"}));
    EXPECT_TRUE(fids->get_feature(0)->get_geometry_collection().empty());

    // The geometry column, which is decoded as in a full read
    const auto geometry = ngen::geopackage::make_reader(this->path).read("test", {}, "id", std::vector<std::string>{"geom"});
    EXPECT_EQ(geometry->get_bounding_box(), with->get_bounding_box());
    EXPECT_EQ(geometry->get_feature(0)->property_keys(), std::vector<std::string>{"id"});
    const auto point = std::get<geojson::coordinate_t>(*geometry->get_feature(0)->geometry());
    EXPECT_EQ(point.get<0>(), 102.0);
    EXPECT_EQ(point.get<1>(), 0.5);

    EXPECT_THROW(ngen::geopackage::make_reader(this->path).read("test", {}, "id", std::vector<std::string>{"no_such_column"}),
                 std::runtime_error);
}

// this test is essentially the same as the above, however, the coordinates
// are stored in EPSG:3857. When read in, they should convert to EPSG:4326.
TEST_F(GeoPackage_Test, geopackage_projection_test)
{
    const auto gpkg = ngen::geopackage::make_reader(this->path2).read("example_3857", {});
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

    const auto point = std::get<geojson::coordinate_t>(*first->geometry());
    EXPECT_NEAR(point.get<0>(), 102.0, 0.0001);
    EXPECT_NEAR(point.get<1>(), 0.5, 0.0001);

    ASSERT_TRUE(third == nullptr);
}
