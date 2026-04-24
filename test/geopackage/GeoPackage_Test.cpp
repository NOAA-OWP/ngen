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

// Fixture for v3.0 nexus remap tests.
// Verifies that 'id' and 'toid' are correctly aliased from 'nexus_id' /
// 'nexus_toid' on v3.0 loads, and that v2.2 loads continue to populate
// 'id' and 'toid' directly from their original columns.
class GeoPackage_NexusRemap_Test : public ::testing::Test
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

// Every v3.0 nexus feature must expose 'id' == nexus_id and 'toid' ==
// nexus_toid after the loader aliases them at the load boundary. The original
// 'nexus_id' / 'nexus_toid' properties are also preserved (additive aliasing).
TEST_F(GeoPackage_NexusRemap_Test, geopackage_v3_nexus_id_toid_aliased)
{
    const auto gpkg = ngen::geopackage::read(this->v3_0_path, "nexus", {});
    ASSERT_EQ(gpkg->get_size(), 2);

    for (int i = 0; i < gpkg->get_size(); ++i) {
        const auto& feat = gpkg->get_feature(i);
        ASSERT_NE(feat, nullptr) << "feature " << i << " is null";
        EXPECT_TRUE(feat->has_property("id"))   << "nexus feature missing 'id'";
        EXPECT_TRUE(feat->has_property("toid")) << "nexus feature missing 'toid'";
        EXPECT_FALSE(feat->get_property("id").as_string().empty())
            << "nexus feature 'id' is empty";
        EXPECT_FALSE(feat->get_property("toid").as_string().empty())
            << "nexus feature 'toid' is empty";
    }

    // nex-1: nexus_id="nex-1", nexus_toid="fp-2"
    const int idx1 = gpkg->find("nex-1");
    ASSERT_NE(idx1, -1);
    const auto& nex1 = gpkg->get_feature(idx1);
    EXPECT_EQ(nex1->get_property("id").as_string(),   "nex-1");
    EXPECT_EQ(nex1->get_property("toid").as_string(), "fp-2");

    // nex-2: nexus_id="nex-2", nexus_toid="coastal-000001"
    const int idx2 = gpkg->find("nex-2");
    ASSERT_NE(idx2, -1);
    const auto& nex2 = gpkg->get_feature(idx2);
    EXPECT_EQ(nex2->get_property("id").as_string(),   "nex-2");
    EXPECT_EQ(nex2->get_property("toid").as_string(), "coastal-000001");
}

// Regression guard: v2.2 nexus features must still get 'id' and 'toid' from
// their original schema columns without the v3.0 alias logic firing.
TEST_F(GeoPackage_NexusRemap_Test, geopackage_v2_2_nexus_id_toid_from_columns)
{
    const auto gpkg = ngen::geopackage::read(this->v2_2_path, "nexus", {});
    ASSERT_EQ(gpkg->get_size(), 2);

    for (int i = 0; i < gpkg->get_size(); ++i) {
        const auto& feat = gpkg->get_feature(i);
        ASSERT_NE(feat, nullptr) << "feature " << i << " is null";
        EXPECT_TRUE(feat->has_property("id"))   << "v2.2 nexus missing 'id'";
        EXPECT_TRUE(feat->has_property("toid")) << "v2.2 nexus missing 'toid'";
    }

    // v2.2 fixture: nex-1 (toid=wb-2), nex-2 (toid=coastal-000001)
    const int idx1 = gpkg->find("nex-1");
    ASSERT_NE(idx1, -1);
    EXPECT_EQ(gpkg->get_feature(idx1)->get_property("id").as_string(),   "nex-1");
    EXPECT_EQ(gpkg->get_feature(idx1)->get_property("toid").as_string(), "wb-2");

    const int idx2 = gpkg->find("nex-2");
    ASSERT_NE(idx2, -1);
    EXPECT_EQ(gpkg->get_feature(idx2)->get_property("id").as_string(),   "nex-2");
    EXPECT_EQ(gpkg->get_feature(idx2)->get_property("toid").as_string(), "coastal-000001");
}

// Fixture for v3.0 divides toid-synthesis tests.
// Uses example_v3_0.gpkg (3 divides, all flowpath_ids resolve) and
// example_v3_0_dangling.gpkg (2 divides: one resolves, one has a
// flowpath_id not present in flowpaths).
class GeoPackage_DividesToidSynthesis_Test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        this->v3_0_path = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example_v3_0.gpkg",
            "../test/data/geopackage/example_v3_0.gpkg",
            "../../test/data/geopackage/example_v3_0.gpkg"
        });
        if (this->v3_0_path.empty()) {
            FAIL() << "can't find test/data/geopackage/example_v3_0.gpkg";
        }

        this->dangling_path = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example_v3_0_dangling.gpkg",
            "../test/data/geopackage/example_v3_0_dangling.gpkg",
            "../../test/data/geopackage/example_v3_0_dangling.gpkg"
        });
        if (this->dangling_path.empty()) {
            FAIL() << "can't find test/data/geopackage/example_v3_0_dangling.gpkg";
        }
    }

    void TearDown() override {}

    std::string v3_0_path;
    std::string dangling_path;
};

// All 3 divides in example_v3_0.gpkg resolve via the divides -> flowpaths
// join, so every feature must carry a non-empty 'toid'. Check the exact
// mapping: cat-1 -> fp-1 -> nex-1, cat-2 -> fp-2 -> nex-2, cat-3 -> fp-3 -> nex-1.
TEST_F(GeoPackage_DividesToidSynthesis_Test, geopackage_v3_divides_toid_all_resolved)
{
    const auto gpkg = ngen::geopackage::read(this->v3_0_path, "divides", {});
    ASSERT_EQ(gpkg->get_size(), 3);

    for (int i = 0; i < gpkg->get_size(); ++i) {
        const auto& feat = gpkg->get_feature(i);
        ASSERT_NE(feat, nullptr) << "divide feature " << i << " is null";
        EXPECT_TRUE(feat->has_property("toid"))
            << "divide " << feat->get_id() << " missing synthesized 'toid'";
        EXPECT_FALSE(feat->get_property("toid").as_string().empty())
            << "divide " << feat->get_id() << " has empty 'toid'";
    }

    const int idx1 = gpkg->find("cat-1");
    ASSERT_NE(idx1, -1);
    EXPECT_EQ(gpkg->get_feature(idx1)->get_property("toid").as_string(), "nex-1");

    const int idx2 = gpkg->find("cat-2");
    ASSERT_NE(idx2, -1);
    EXPECT_EQ(gpkg->get_feature(idx2)->get_property("toid").as_string(), "nex-2");

    const int idx3 = gpkg->find("cat-3");
    ASSERT_NE(idx3, -1);
    EXPECT_EQ(gpkg->get_feature(idx3)->get_property("toid").as_string(), "nex-1");
}

// example_v3_0_dangling.gpkg has cat-1 (flowpath_id=fp-1, resolves to nex-1)
// and cat-2 (flowpath_id=fp-DANGLING, not present in flowpaths). The loader
// must succeed; cat-1 must have toid="nex-1"; cat-2 must have no 'toid'.
// Exactly 1 divide is unlinked, which is what the summary WARN line counts.
TEST_F(GeoPackage_DividesToidSynthesis_Test, geopackage_v3_divides_dangling_flowpath_no_toid)
{
    const auto gpkg = ngen::geopackage::read(this->dangling_path, "divides", {});
    ASSERT_EQ(gpkg->get_size(), 2);

    const int idx1 = gpkg->find("cat-1");
    ASSERT_NE(idx1, -1) << "cat-1 not found";
    EXPECT_TRUE(gpkg->get_feature(idx1)->has_property("toid"));
    EXPECT_EQ(gpkg->get_feature(idx1)->get_property("toid").as_string(), "nex-1");

    const int idx2 = gpkg->find("cat-2");
    ASSERT_NE(idx2, -1) << "cat-2 not found";
    EXPECT_FALSE(gpkg->get_feature(idx2)->has_property("toid"))
        << "cat-2 has a dangling flowpath_id and must not receive a synthesized toid";

    std::size_t unlinked = 0;
    for (int i = 0; i < gpkg->get_size(); ++i) {
        const auto& f = gpkg->get_feature(i);
        if (f && !f->has_property("toid")) {
            ++unlinked;
        }
    }
    EXPECT_EQ(unlinked, std::size_t(1))
        << "expected exactly 1 unlinked divide (the WARN count should be 1)";
}

// Fixture for subset-tolerance regression test.
// Uses example_v3_0_minimal.gpkg, which contains only nexus, divides, and
// flowpaths (no auxiliary tables). The test verifies that both layers load
// and that a combined collection links all 3 divides to their target nexuses.
class GeoPackage_SubsetTolerance_Test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        this->path = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example_v3_0_minimal.gpkg",
            "../test/data/geopackage/example_v3_0_minimal.gpkg",
            "../../test/data/geopackage/example_v3_0_minimal.gpkg"
        });

        if (this->path.empty()) {
            FAIL() << "can't find test/data/geopackage/example_v3_0_minimal.gpkg";
        }
    }

    void TearDown() override {}

    std::string path;
};

// Both layers of a GPKG that contains only nexus/divides/flowpaths (no
// auxiliary tables) must load successfully with the expected feature counts.
// Merging both collections and running link_features_from_property must
// resolve all 3 divide->nexus edges (cat-1->nex-1, cat-2->nex-2,
// cat-3->nex-1), confirming end-to-end connectivity without auxiliary tables.
TEST_F(GeoPackage_SubsetTolerance_Test, geopackage_v3_minimal_loads_and_links_end_to_end)
{
    const auto divides = ngen::geopackage::read(this->path, "divides", {});
    const auto nexus   = ngen::geopackage::read(this->path, "nexus",   {});

    ASSERT_EQ(divides->get_size(), 3);
    ASSERT_EQ(nexus->get_size(),   2);

    // Merge both layers into a single collection so link_features_from_property
    // can resolve divide toid -> nexus id lookups across both layers.
    geojson::FeatureCollection combined;
    for (int i = 0; i < divides->get_size(); ++i) {
        combined.add_feature(divides->get_feature(i));
    }
    for (int i = 0; i < nexus->get_size(); ++i) {
        combined.add_feature(nexus->get_feature(i));
    }

    std::string toid_key = "toid";
    const int links = combined.link_features_from_property(nullptr, &toid_key);

    // All 3 divides have a synthesized toid that resolves to a nexus in the
    // combined collection; nexus toids (fp-2, coastal-000001) are absent from
    // the collection so they do not add to the link count.
    EXPECT_EQ(links, 3);
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
