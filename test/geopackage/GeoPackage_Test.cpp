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

// Fixture for extra-column tolerance tests.
// Uses example_v3_0_extra_col.gpkg which carries:
//   - flowlines with an extra 'lengthkm' column (auxiliary table, never read)
//   - pois with geom declared as GEOMETRY instead of POINT (auxiliary table, never read)
class GeoPackage_ExtraCol_Test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        this->path = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example_v3_0_extra_col.gpkg",
            "../test/data/geopackage/example_v3_0_extra_col.gpkg",
            "../../test/data/geopackage/example_v3_0_extra_col.gpkg"
        });

        if (this->path.empty()) {
            FAIL() << "can't find test/data/geopackage/example_v3_0_extra_col.gpkg";
        }
    }

    void TearDown() override {}

    std::string path;
};

// Loading nexus from a v3.0 GPKG that also contains flowlines with an extra
// column and pois with a GEOMETRY-typed geom must succeed. The extra column
// ('lengthkm') belongs only to flowlines, which the loader never opens, so
// it must not appear in any nexus feature's properties.
TEST_F(GeoPackage_ExtraCol_Test, geopackage_v3_nexus_extra_col_ignored)
{
    const auto gpkg = ngen::geopackage::read(this->path, "nexus", {});
    ASSERT_EQ(gpkg->get_size(), 1);

    const auto& feat = gpkg->get_feature(0);
    ASSERT_NE(feat, nullptr);

    // v3.0 nexus_id aliased to id; nexus_toid aliased to toid
    EXPECT_EQ(feat->get_id(), "nex-1");
    ASSERT_TRUE(feat->has_property("id"));
    ASSERT_TRUE(feat->has_property("toid"));
    EXPECT_EQ(feat->get_property("id").as_string(), "nex-1");
    EXPECT_EQ(feat->get_property("toid").as_string(), "coastal-000001");

    // 'lengthkm' lives only on the flowlines auxiliary table; must not
    // appear in nexus feature properties since flowlines is never read
    EXPECT_FALSE(feat->has_property("lengthkm"));
}

// Loading divides from the same GPKG must also succeed and synthesize
// 'toid' via the divides -> flowpaths join (cat-1 -> fp-1 -> nex-1).
TEST_F(GeoPackage_ExtraCol_Test, geopackage_v3_divides_toid_synthesized)
{
    const auto gpkg = ngen::geopackage::read(this->path, "divides", {});
    ASSERT_EQ(gpkg->get_size(), 1);

    const auto& feat = gpkg->get_feature(0);
    ASSERT_NE(feat, nullptr);

    EXPECT_EQ(feat->get_id(), "cat-1");
    ASSERT_TRUE(feat->has_property("toid"));
    EXPECT_EQ(feat->get_property("toid").as_string(), "nex-1");
}

// Fixture for detect_version unit tests.
// Uses example_v2_2.gpkg (v2.2 nexus schema: 'id' column) and
// example_v3_0.gpkg (v3.0 nexus schema: 'nexus_id' column).
class GeoPackage_DetectVersion_Test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        this->v2_2_path = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example_v2_2.gpkg",
            "../test/data/geopackage/example_v2_2.gpkg",
            "../../test/data/geopackage/example_v2_2.gpkg"
        });
        if (this->v2_2_path.empty()) {
            FAIL() << "can't find test/data/geopackage/example_v2_2.gpkg";
        }

        this->v3_0_path = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example_v3_0.gpkg",
            "../test/data/geopackage/example_v3_0.gpkg",
            "../../test/data/geopackage/example_v3_0.gpkg"
        });
        if (this->v3_0_path.empty()) {
            FAIL() << "can't find test/data/geopackage/example_v3_0.gpkg";
        }
    }

    void TearDown() override {}

    std::string v2_2_path;
    std::string v3_0_path;
};

// Open example_v2_2.gpkg; detect_version must return V2_2.
TEST_F(GeoPackage_DetectVersion_Test, geopackage_detect_version_v2_2)
{
    ngen::sqlite::database db{this->v2_2_path};
    EXPECT_EQ(
        ngen::geopackage::detect_version(db.connection()),
        ngen::geopackage::HydrofabricVersion::V2_2
    );
}

// Open example_v3_0.gpkg; detect_version must return V3_0.
TEST_F(GeoPackage_DetectVersion_Test, geopackage_detect_version_v3_0)
{
    ngen::sqlite::database db{this->v3_0_path};
    EXPECT_EQ(
        ngen::geopackage::detect_version(db.connection()),
        ngen::geopackage::HydrofabricVersion::V3_0
    );
}

// A nexus table whose columns are unrecognized (neither 'id' nor 'nexus_id')
// must cause detect_version to throw std::runtime_error with a message
// containing "nexus".
TEST_F(GeoPackage_DetectVersion_Test, geopackage_detect_version_throws_on_bad_schema)
{
    // Build a temporary SQLite database with a malformed nexus table.
    const std::string path = std::string(testing::TempDir()) + "/malformed_nexus.gpkg";
    {
        sqlite3* raw = nullptr;
        ASSERT_EQ(sqlite3_open(path.c_str(), &raw), SQLITE_OK);
        // Nexus table present but with neither 'id' nor 'nexus_id' columns.
        ASSERT_EQ(sqlite3_exec(raw, "CREATE TABLE nexus (junk TEXT)",
                               nullptr, nullptr, nullptr), SQLITE_OK);
        sqlite3_close(raw);
    }

    ngen::sqlite::database db{path};
    try {
        ngen::geopackage::detect_version(db.connection());
        FAIL() << "Expected std::runtime_error from detect_version";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("nexus"), std::string::npos)
            << "exception message: " << e.what();
    }
}
