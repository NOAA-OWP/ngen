#include <gtest/gtest.h>

#include "HydrofabricReaderFactory.hpp"
#include "FileChecker.h"
#include "fixture_builders.hpp"

namespace fixtures = ngen::hydrofabric::fixtures;

// Uses the extra-column v4.0beta1 fixture, which carries:
//   - flowlines with an extra 'lengthkm' column (auxiliary table, never read)
//   - pois with geom declared as GEOMETRY instead of POINT (auxiliary table, never read)
class GeoPackage_ExtraCol_Test : public ::testing::Test
{
  protected:
    static void SetUpTestSuite()
    {
        path = fixtures::write_v4_0beta1_extra_col();
    }

    static std::string path;
};

std::string GeoPackage_ExtraCol_Test::path;

// Loading nexus from a v4 GPKG with an extra-column flowlines table and a
// GEOMETRY-typed pois table must still succeed.
TEST_F(GeoPackage_ExtraCol_Test, geopackage_v4_nexus_extra_col_ignored)
{
    const auto gpkg = ngen::hydrofabric::make_hydrofabric_reader(path)->read_nexus({});
    ASSERT_EQ(gpkg->get_size(), 1);

    const auto& feat = gpkg->get_feature(0);
    ASSERT_NE(feat, nullptr);

    // v4 nexus_id aliased to id; nexus_toid aliased to toid
    EXPECT_EQ(feat->get_id(), "nex-1");
    ASSERT_TRUE(feat->has_property("id"));
    ASSERT_TRUE(feat->has_property("toid"));
    EXPECT_EQ(feat->get_property("id").as_string(), "nex-1");
    EXPECT_EQ(feat->get_property("toid").as_string(), "coastal-000001");

    // 'lengthkm' lives only on the (unread) flowlines table
    EXPECT_FALSE(feat->has_property("lengthkm"));
}

// Loading divides from the same GPKG must also succeed and synthesize
// 'toid' via the divides -> flowpaths join (cat-1 -> fp-1 -> nex-1).
TEST_F(GeoPackage_ExtraCol_Test, geopackage_v4_divides_toid_synthesized)
{
    const auto gpkg = ngen::hydrofabric::make_hydrofabric_reader(path)->read_divides({});
    ASSERT_EQ(gpkg->get_size(), 1);

    const auto& feat = gpkg->get_feature(0);
    ASSERT_NE(feat, nullptr);

    EXPECT_EQ(feat->get_id(), "cat-1");
    ASSERT_TRUE(feat->has_property("toid"));
    EXPECT_EQ(feat->get_property("toid").as_string(), "nex-1");
}

// Covers 'id'/'toid' aliasing from 'nexus_id'/'nexus_toid' on v4 loads, and
// direct population from the original columns on v2.2 loads.
class GeoPackage_NexusRemap_Test : public ::testing::Test
{
  protected:
    static void SetUpTestSuite()
    {
        v2_2_path      = fixtures::write_v2_2();
        v4_0beta1_path = fixtures::write_v4_0beta1();
    }

    static std::string v2_2_path;
    static std::string v4_0beta1_path;
};

std::string GeoPackage_NexusRemap_Test::v2_2_path;
std::string GeoPackage_NexusRemap_Test::v4_0beta1_path;

// Every v4 nexus feature must expose 'id' == nexus_id and 'toid' ==
// nexus_toid; original 'nexus_id'/'nexus_toid' properties are also preserved.
TEST_F(GeoPackage_NexusRemap_Test, geopackage_v4_nexus_id_toid_aliased)
{
    const auto gpkg = ngen::hydrofabric::make_hydrofabric_reader(v4_0beta1_path)->read_nexus({});
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
// their original schema columns without the v4 alias logic firing.
TEST_F(GeoPackage_NexusRemap_Test, geopackage_v2_2_nexus_id_toid_from_columns)
{
    const auto gpkg = ngen::hydrofabric::make_hydrofabric_reader(v2_2_path)->read_nexus({});
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

// beta1-only: toid is synthesized via a divides->flowpaths join (v4.0 reads
// flowpath_toid natively instead; see GeoPackage_DividesNativeToid_Test).
// The v4.0beta1 fixture has 3 divides, all of which resolve; the dangling
// fixture has 2, one with a flowpath_id absent from flowpaths.
class GeoPackage_DividesToidSynthesis_Test : public ::testing::Test
{
  protected:
    static void SetUpTestSuite()
    {
        v4_0beta1_path = fixtures::write_v4_0beta1();
        dangling_path  = fixtures::write_v4_0beta1_dangling();
    }

    static std::string v4_0beta1_path;
    static std::string dangling_path;
};

std::string GeoPackage_DividesToidSynthesis_Test::v4_0beta1_path;
std::string GeoPackage_DividesToidSynthesis_Test::dangling_path;

// All 3 divides in the v4.0beta1 fixture resolve via the divides -> flowpaths
// join, so every feature must carry a non-empty 'toid'. Check the exact
// mapping: cat-1 -> fp-1 -> nex-1, cat-2 -> fp-2 -> nex-2, cat-3 -> fp-3 -> nex-1.
TEST_F(GeoPackage_DividesToidSynthesis_Test, geopackage_v4_divides_toid_all_resolved)
{
    const auto gpkg = ngen::hydrofabric::make_hydrofabric_reader(v4_0beta1_path)->read_divides({});
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

// The dangling fixture has cat-1 (flowpath_id=fp-1, resolves to nex-1) and
// cat-2 (flowpath_id=fp-DANGLING, not present in flowpaths). The loader
// must succeed; cat-1 must have toid="nex-1"; cat-2 must have no 'toid'.
// Exactly 1 divide is unlinked, which is what the summary WARN line counts.
TEST_F(GeoPackage_DividesToidSynthesis_Test, geopackage_v4_divides_dangling_flowpath_no_toid)
{
    const auto gpkg = ngen::hydrofabric::make_hydrofabric_reader(dangling_path)->read_divides({});
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

// The v4.0 fixture carries divides.flowpath_toid natively and shares its
// topology with the v4.0beta1 one. example_v4_0_real.gpkg is a stripped
// NOAA-OWP gage subset exercising the same path against genuine data.
class GeoPackage_DividesNativeToid_Test : public ::testing::Test
{
  protected:
    static void SetUpTestSuite()
    {
        v4_0_path      = fixtures::write_v4_0();
        v4_0beta1_path = fixtures::write_v4_0beta1();

        // The one fixture that cannot be built here: it is a subset of a real
        // hydrofabric that does not live in this repository.
        real_path = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example_v4_0_real.gpkg",
            "../test/data/geopackage/example_v4_0_real.gpkg",
            "../../test/data/geopackage/example_v4_0_real.gpkg"
        });
    }

    void SetUp() override
    {
        ASSERT_FALSE(real_path.empty())
            << "can't find test/data/geopackage/example_v4_0_real.gpkg";
    }

    static std::string v4_0_path;
    static std::string v4_0beta1_path;
    static std::string real_path;
};

std::string GeoPackage_DividesNativeToid_Test::v4_0_path;
std::string GeoPackage_DividesNativeToid_Test::v4_0beta1_path;
std::string GeoPackage_DividesNativeToid_Test::real_path;

// Every divide in example_v4_0.gpkg must take 'toid' straight from the native
// flowpath_toid column: cat-1 -> nex-1, cat-2 -> nex-2, cat-3 -> nex-1.
TEST_F(GeoPackage_DividesNativeToid_Test, geopackage_v4_0_divides_native_toid)
{
    const auto gpkg = ngen::hydrofabric::make_hydrofabric_reader(v4_0_path)->read_divides({});
    ASSERT_EQ(gpkg->get_size(), 3);

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

// The two variants describe identical topology, so reading each must yield
// the same divide -> toid mapping.
TEST_F(GeoPackage_DividesNativeToid_Test, geopackage_v4_0_matches_beta1_toids)
{
    const auto native = ngen::hydrofabric::make_hydrofabric_reader(v4_0_path)->read_divides({});
    const auto joined = ngen::hydrofabric::make_hydrofabric_reader(v4_0beta1_path)->read_divides({});
    ASSERT_EQ(native->get_size(), joined->get_size());

    for (int i = 0; i < native->get_size(); ++i) {
        const std::string id = native->get_feature(i)->get_id();
        const int j = joined->find(id);
        ASSERT_NE(j, -1) << id << " present in v4.0 but not in v4.0beta1";
        EXPECT_EQ(
            native->get_feature(i)->get_property("toid").as_string(),
            joined->get_feature(j)->get_property("toid").as_string()
        ) << "toid disagrees between variants for " << id;
    }
}

// A real NOAA-OWP v4.0 gage subset must load with every divide linked, and its
// single terminal nexus must survive the nexus remap with a 'toid' present.
TEST_F(GeoPackage_DividesNativeToid_Test, geopackage_v4_0_real_hydrofabric_loads)
{
    const auto divides = ngen::hydrofabric::make_hydrofabric_reader(real_path)->read_divides({});
    ASSERT_EQ(divides->get_size(), 190);
    for (int i = 0; i < divides->get_size(); ++i) {
        const auto& feat = divides->get_feature(i);
        ASSERT_NE(feat, nullptr);
        EXPECT_TRUE(feat->has_property("toid"))
            << "divide " << feat->get_id() << " missing 'toid'";
    }

    const auto nexus = ngen::hydrofabric::make_hydrofabric_reader(real_path)->read_nexus({});
    ASSERT_EQ(nexus->get_size(), 75);

    int terminal = 0;
    for (int i = 0; i < nexus->get_size(); ++i) {
        const auto& feat = nexus->get_feature(i);
        ASSERT_NE(feat, nullptr);
        EXPECT_TRUE(feat->has_property("id"));
        EXPECT_TRUE(feat->has_property("toid"));
        if (feat->get_id().rfind("tnx-", 0) == 0) {
            ++terminal;
        }
    }
    EXPECT_EQ(terminal, 1) << "expected exactly 1 terminal (tnx-) nexus";
}

// Uses the minimal v4.0beta1 fixture, which contains only nexus, divides,
// and flowpaths (no auxiliary tables).
class GeoPackage_SubsetTolerance_Test : public ::testing::Test
{
  protected:
    static void SetUpTestSuite()
    {
        path = fixtures::write_v4_0beta1_minimal();
    }

    static std::string path;
};

std::string GeoPackage_SubsetTolerance_Test::path;

// Both layers must load with the expected feature counts, and linking a
// combined collection must resolve all 3 divide->nexus edges
// (cat-1->nex-1, cat-2->nex-2, cat-3->nex-1).
TEST_F(GeoPackage_SubsetTolerance_Test, geopackage_v4_minimal_loads_and_links_end_to_end)
{
    const auto divides = ngen::hydrofabric::make_hydrofabric_reader(path)->read_divides({});
    const auto nexus   = ngen::hydrofabric::make_hydrofabric_reader(path)->read_nexus({});

    ASSERT_EQ(divides->get_size(), 3);
    ASSERT_EQ(nexus->get_size(),   2);

    // Merge both layers so link_features_from_property can resolve
    // divide toid -> nexus id lookups across them.
    geojson::FeatureCollection combined;
    for (int i = 0; i < divides->get_size(); ++i) {
        combined.add_feature(divides->get_feature(i));
    }
    for (int i = 0; i < nexus->get_size(); ++i) {
        combined.add_feature(nexus->get_feature(i));
    }

    std::string toid_key = "toid";
    const int links = combined.link_features_from_property(nullptr, &toid_key);

    // nexus toids (fp-2, coastal-000001) are absent from the collection,
    // so only the 3 divide->nexus edges count.
    EXPECT_EQ(links, 3);
}
