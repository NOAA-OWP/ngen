#include <boost/geometry/io/wkt/write.hpp>
#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>
#include <streambuf>

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

// The reader warns (on stderr) when the id column has no supporting index,
// since subset queries degrade to full table scans without one. Verify the
// warning fires for an unindexed layer and is suppressed once an index exists,
// and that reads remain correct with the read-only pragmas applied.
TEST_F(GeoPackage_Test, geopackage_index_warning_test)
{
    const std::string needle = "No index found on column 'id'";

    // --- Unindexed layer: warning expected ---
    std::ostringstream captured;
    std::streambuf* old_cerr = std::cerr.rdbuf(captured.rdbuf());
    const auto gpkg = ngen::geopackage::read(this->path, "test", {});
    std::cerr.rdbuf(old_cerr);

    // Read correctness is unaffected by the pragmas.
    EXPECT_NE(gpkg->find("First"), -1);
    EXPECT_NE(gpkg->find("Second"), -1);
    EXPECT_EQ(2, gpkg->get_size());

    // The warning is emitted for this unindexed layer, except in a NGEN_QUIET
    // build which suppresses such output. Assert the right thing in either case
    // so the test never degrades to a no-op.
    const bool warned = captured.str().find(needle) != std::string::npos;
#ifndef NGEN_QUIET
    EXPECT_TRUE(warned) << "expected a missing-index warning; got: " << captured.str();
#else
    EXPECT_FALSE(warned) << "NGEN_QUIET build should suppress the warning; got: " << captured.str();
#endif

    // --- Indexed copy: warning suppressed ---
    namespace fs = std::filesystem;
    const fs::path indexed = fs::temp_directory_path() / "ngen_indexed_example.gpkg";
    fs::copy_file(this->path, indexed, fs::copy_options::overwrite_existing);

    sqlite3* conn = nullptr;
    ASSERT_EQ(sqlite3_open_v2(indexed.c_str(), &conn, SQLITE_OPEN_READWRITE, nullptr), SQLITE_OK);
    char* errmsg = nullptr;
    const int rc = sqlite3_exec(conn, "CREATE INDEX idx_test_id ON \"test\"(id);", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::string m = errmsg ? errmsg : "(unknown)";
        sqlite3_free(errmsg);
        sqlite3_close(conn);
        fs::remove(indexed);
        FAIL() << "failed to create test index: " << m;
    }
    sqlite3_close(conn);

    std::ostringstream captured2;
    old_cerr = std::cerr.rdbuf(captured2.rdbuf());
    const auto gpkg2 = ngen::geopackage::read(indexed.string(), "test", {});
    std::cerr.rdbuf(old_cerr);
    fs::remove(indexed);

    EXPECT_EQ(2, gpkg2->get_size());
    EXPECT_EQ(captured2.str().find(needle), std::string::npos)
        << "did not expect a missing-index warning once an index exists; got: " << captured2.str();
}

// The database is opened via a "file:...?immutable=1" URI, so paths containing
// characters that are special in URIs (spaces being the common case) must be
// percent-encoded. A path with spaces must still open and read correctly.
TEST_F(GeoPackage_Test, geopackage_uri_encoded_path_test)
{
    namespace fs = std::filesystem;
    const fs::path spaced = fs::temp_directory_path() / "ngen spaced example.gpkg";
    fs::copy_file(this->path, spaced, fs::copy_options::overwrite_existing);

    std::shared_ptr<geojson::FeatureCollection> gpkg;
    ASSERT_NO_THROW(gpkg = ngen::geopackage::read(spaced.string(), "test", {}));
    fs::remove(spaced);

    EXPECT_EQ(2, gpkg->get_size());
    EXPECT_NE(gpkg->find("First"), -1);
    EXPECT_NE(gpkg->find("Second"), -1);
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
