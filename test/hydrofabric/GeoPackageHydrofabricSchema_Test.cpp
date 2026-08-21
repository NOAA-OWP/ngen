#include <gtest/gtest.h>

#include <cstdio>

#include <sqlite3.h>

#include "GeoPackageHydrofabricSchema.hpp"
#include "ngen_sqlite.hpp"
#include "fixture_builders.hpp"

namespace fixtures = ngen::hydrofabric::fixtures;

// Uses the v2.2 fixture ('id' nexus column) and the two v4 variants, which
// share the 'nexus_id' schema and are told apart by divides.flowpath_toid:
//   v4.0beta1 — no divides.flowpath_toid  -> V4_0_BETA1
//   v4.0      — has divides.flowpath_toid -> V4_0
class GeoPackageHydrofabricSchema_Test : public ::testing::Test
{
  protected:
    static void SetUpTestSuite()
    {
        v2_2_path         = fixtures::write_v2_2();
        v4_0beta1_path    = fixtures::write_v4_0beta1();
        v4_0_path         = fixtures::write_v4_0();
        v4_0_nexus_path   = fixtures::write_v4_0_nexus_only();
        v4_0_divides_path = fixtures::write_v4_0_divides_only();
    }

    static std::string v2_2_path;
    static std::string v4_0beta1_path;
    static std::string v4_0_path;
    static std::string v4_0_nexus_path;
    static std::string v4_0_divides_path;
};

std::string GeoPackageHydrofabricSchema_Test::v2_2_path;
std::string GeoPackageHydrofabricSchema_Test::v4_0beta1_path;
std::string GeoPackageHydrofabricSchema_Test::v4_0_path;
std::string GeoPackageHydrofabricSchema_Test::v4_0_nexus_path;
std::string GeoPackageHydrofabricSchema_Test::v4_0_divides_path;

// Open the v2.2 fixture; detect_version must return V2_2.
TEST_F(GeoPackageHydrofabricSchema_Test, geopackage_detect_version_v2_2)
{
    ngen::sqlite::database db{v2_2_path};
    EXPECT_EQ(
        ngen::hydrofabric::detect_version(db),
        ngen::hydrofabric::HydrofabricVersion::V2_2
    );
}

// Open the v4.0beta1 fixture; nexus_id marks it v4, and the absence of
// divides.flowpath_toid narrows it to the beta1 variant.
TEST_F(GeoPackageHydrofabricSchema_Test, geopackage_detect_version_v4_0beta1)
{
    ngen::sqlite::database db{v4_0beta1_path};
    EXPECT_EQ(
        ngen::hydrofabric::detect_version(db),
        ngen::hydrofabric::HydrofabricVersion::V4_0_BETA1
    );
}

// Open the v4.0 fixture; nexus_id marks it v4, and the presence of a native
// divides.flowpath_toid narrows it to the release variant.
TEST_F(GeoPackageHydrofabricSchema_Test, geopackage_detect_version_v4_0)
{
    ngen::sqlite::database db{v4_0_path};
    EXPECT_EQ(
        ngen::hydrofabric::detect_version(db),
        ngen::hydrofabric::HydrofabricVersion::V4_0
    );
}

// The pure overload discriminates on the divides column list. A v4 nexus
// column set with no divides columns at all (e.g. a nexus-only GPKG) falls
// back to beta1, since the variant only matters when reading divides.
TEST_F(GeoPackageHydrofabricSchema_Test, geopackage_detect_version_column_lists)
{
    const std::vector<std::string> v4_nexus{"fid", "geom", "nexus_id", "nexus_toid", "vpuid"};

    EXPECT_EQ(
        ngen::hydrofabric::detect_version(v4_nexus, {"divide_id", "flowpath_id", "flowpath_toid"}),
        ngen::hydrofabric::HydrofabricVersion::V4_0
    );
    EXPECT_EQ(
        ngen::hydrofabric::detect_version(v4_nexus, {"divide_id", "flowpath_id"}),
        ngen::hydrofabric::HydrofabricVersion::V4_0_BETA1
    );
    EXPECT_EQ(
        ngen::hydrofabric::detect_version(v4_nexus),
        ngen::hydrofabric::HydrofabricVersion::V4_0_BETA1
    );
}

// A nexus table whose columns are unrecognized (neither 'id' nor 'nexus_id')
// must cause detect_version to throw std::runtime_error with a message
// containing "nexus".
TEST_F(GeoPackageHydrofabricSchema_Test, geopackage_detect_version_throws_on_bad_schema)
{
    const std::string path = std::string(testing::TempDir()) + "/malformed_nexus.gpkg";
    {
        std::remove(path.c_str());
        sqlite3* raw = nullptr;
        ASSERT_EQ(sqlite3_open(path.c_str(), &raw), SQLITE_OK);
        // Nexus table present but with neither 'id' nor 'nexus_id' columns.
        ASSERT_EQ(sqlite3_exec(raw, "CREATE TABLE nexus (junk TEXT)",
                               nullptr, nullptr, nullptr), SQLITE_OK);
        sqlite3_close(raw);
    }

    ngen::sqlite::database db{path};
    try {
        ngen::hydrofabric::detect_version(db);
        FAIL() << "Expected std::runtime_error from detect_version";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("nexus"), std::string::npos)
            << "exception message: " << e.what();
    }
}

// A v4.0 hydrofabric split across two files: `nexus` in one, `divides` in the other. Each column
// list has to come from the file that holds its layer, because an absent table reads as an empty
// column list, and an empty divides column list means "no flowpath_toid", i.e. beta1.
TEST_F(GeoPackageHydrofabricSchema_Test, geopackage_detect_version_two_databases)
{
    ngen::sqlite::database nexus_db{v4_0_nexus_path};
    ngen::sqlite::database divides_db{v4_0_divides_path};

    EXPECT_EQ(
        ngen::hydrofabric::detect_version(nexus_db, divides_db),
        ngen::hydrofabric::HydrofabricVersion::V4_0
    );

    // Against the nexus file alone, the same hydrofabric can only look like beta1.
    EXPECT_EQ(
        ngen::hydrofabric::detect_version(nexus_db),
        ngen::hydrofabric::HydrofabricVersion::V4_0_BETA1
    );
}

// A GeoPackage with no `nexus` layer is not a hydrofabric, and UNRECOGNIZED says exactly that.
TEST_F(GeoPackageHydrofabricSchema_Test, geopackage_detect_hydrofabric_without_nexus)
{
    ngen::sqlite::database db{v4_0_divides_path};

    EXPECT_EQ(
        ngen::hydrofabric::detect_hydrofabric(db),
        ngen::hydrofabric::HydrofabricVersion::UNRECOGNIZED
    );
}

// A `nexus` layer that is present and identifiable makes the file a hydrofabric of that version.
TEST_F(GeoPackageHydrofabricSchema_Test, geopackage_detect_hydrofabric_identifies_version)
{
    ngen::sqlite::database db{v4_0_path};

    EXPECT_EQ(
        ngen::hydrofabric::detect_hydrofabric(db),
        ngen::hydrofabric::HydrofabricVersion::V4_0
    );
}

// A `nexus` layer matching no known schema is rejected rather than reported as a version, so the
// error reaches every caller without any of them having to check for it.
TEST_F(GeoPackageHydrofabricSchema_Test, geopackage_detect_hydrofabric_throws_on_bad_schema)
{
    const std::string path = std::string(testing::TempDir()) + "/detect_malformed_nexus.gpkg";
    {
        std::remove(path.c_str());
        sqlite3* raw = nullptr;
        ASSERT_EQ(sqlite3_open(path.c_str(), &raw), SQLITE_OK);
        ASSERT_EQ(sqlite3_exec(raw, "CREATE TABLE nexus (junk TEXT)",
                               nullptr, nullptr, nullptr), SQLITE_OK);
        sqlite3_close(raw);
    }

    ngen::sqlite::database db{path};
    EXPECT_THROW(ngen::hydrofabric::detect_hydrofabric(db), std::runtime_error);
}
