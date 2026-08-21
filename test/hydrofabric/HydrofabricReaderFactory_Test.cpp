#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>

#include <sqlite3.h>

#include "HydrofabricReaderFactory.hpp"
#include "FileChecker.h"
#include "fixture_builders.hpp"

namespace fixtures = ngen::hydrofabric::fixtures;

namespace {

//! Copy a file byte for byte, so a fixture can be given a name that says nothing about its format.
std::string copy_as(const std::string& source, const std::string& name)
{
    const std::string destination = std::string(testing::TempDir()) + "/" + name;

    std::ifstream in{source, std::ios::binary};
    std::ofstream out{destination, std::ios::binary | std::ios::trunc};
    out << in.rdbuf();

    return destination;
}

} // namespace

// Covers what the factory decides before any feature is read: which format the paths name, whether
// the files are a hydrofabric at all, and which schema version they are.
class HydrofabricReaderFactory_Test : public ::testing::Test
{
  protected:
    static void SetUpTestSuite()
    {
        v4_0_path         = fixtures::write_v4_0();
        v4_0_nexus_path   = fixtures::write_v4_0_nexus_only();
        v4_0_divides_path = fixtures::write_v4_0_divides_only();

        geojson_catchment_path = utils::FileChecker::find_first_readable({
            "data/catchment_data.geojson",
            "../data/catchment_data.geojson",
            "../../data/catchment_data.geojson"
        });
        geojson_nexus_path = utils::FileChecker::find_first_readable({
            "data/nexus_data.geojson",
            "../data/nexus_data.geojson",
            "../../data/nexus_data.geojson"
        });
    }

    static std::string v4_0_path;
    static std::string v4_0_nexus_path;
    static std::string v4_0_divides_path;
    static std::string geojson_catchment_path;
    static std::string geojson_nexus_path;
};

std::string HydrofabricReaderFactory_Test::v4_0_path;
std::string HydrofabricReaderFactory_Test::v4_0_nexus_path;
std::string HydrofabricReaderFactory_Test::v4_0_divides_path;
std::string HydrofabricReaderFactory_Test::geojson_catchment_path;
std::string HydrofabricReaderFactory_Test::geojson_nexus_path;

// One path naming both roles is the common case; both roles must read from the one file.
TEST_F(HydrofabricReaderFactory_Test, factory_single_geopackage_path)
{
    const std::unique_ptr<ngen::hydrofabric::HydrofabricReader> hydrofabric =
        ngen::hydrofabric::make_hydrofabric_reader(v4_0_path);

    EXPECT_EQ(hydrofabric->read_divides({})->get_size(), 3);
    EXPECT_EQ(hydrofabric->read_nexus({})->get_size(), 2);
}

// A v4.0 hydrofabric split across two files must be identified as v4.0 and read as such. Detecting
// against the nexus file alone cannot do this: with no `divides` layer beside it, `flowpath_toid`
// appears absent and the file reads as v4.0beta1, whose divides would then be joined through a
// `flowpaths` table that is not there.
TEST_F(HydrofabricReaderFactory_Test, factory_split_geopackage_paths_detect_v4_0)
{
    const std::unique_ptr<ngen::hydrofabric::HydrofabricReader> hydrofabric =
        ngen::hydrofabric::make_hydrofabric_reader(v4_0_divides_path, v4_0_nexus_path);

    const geojson::GeoJSON divides = hydrofabric->read_divides({});
    ASSERT_EQ(divides->get_size(), 3);

    // Native flowpath_toid, which only the v4.0 reader consults.
    const int cat = divides->find("cat-1");
    ASSERT_NE(cat, -1);
    EXPECT_EQ(divides->get_feature(cat)->get_property("toid").as_string(), "nex-1");

    const geojson::GeoJSON nexus = hydrofabric->read_nexus({});
    ASSERT_EQ(nexus->get_size(), 2);
    EXPECT_NE(nexus->find("nex-1"), -1);
}

// A GeoPackage with no `nexus` layer is not a hydrofabric, and saying so beats reading it as some
// version it never claimed to be.
TEST_F(HydrofabricReaderFactory_Test, factory_rejects_geopackage_without_nexus)
{
    try {
        ngen::hydrofabric::make_hydrofabric_reader(v4_0_divides_path);
        FAIL() << "expected std::runtime_error for a GeoPackage with no nexus layer";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("not a hydrofabric"), std::string::npos)
            << "exception message: " << e.what();
    }
}

// A `nexus` layer matching no known schema is an error, not a version to guess at.
TEST_F(HydrofabricReaderFactory_Test, factory_rejects_unrecognized_nexus_schema)
{
    const std::string path = std::string(testing::TempDir()) + "/factory_malformed_nexus.gpkg";
    {
        std::remove(path.c_str());
        sqlite3* raw = nullptr;
        ASSERT_EQ(sqlite3_open(path.c_str(), &raw), SQLITE_OK);
        ASSERT_EQ(sqlite3_exec(raw, "CREATE TABLE nexus (junk TEXT)",
                               nullptr, nullptr, nullptr), SQLITE_OK);
        sqlite3_close(raw);
    }

    try {
        ngen::hydrofabric::make_hydrofabric_reader(path);
        FAIL() << "expected std::runtime_error for an unrecognized nexus schema";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("nexus"), std::string::npos)
            << "exception message: " << e.what();
    }
}

// Two GeoJSON paths select the GeoJSON reader without any detection at all.
TEST_F(HydrofabricReaderFactory_Test, factory_geojson_paths)
{
    ASSERT_FALSE(geojson_catchment_path.empty()) << "can't find data/catchment_data.geojson";
    ASSERT_FALSE(geojson_nexus_path.empty()) << "can't find data/nexus_data.geojson";

    const std::unique_ptr<ngen::hydrofabric::HydrofabricReader> hydrofabric =
        ngen::hydrofabric::make_hydrofabric_reader(geojson_catchment_path, geojson_nexus_path);

    EXPECT_EQ(hydrofabric->read_divides({})->get_size(), 3);
    EXPECT_EQ(hydrofabric->read_nexus({})->get_size(), 3);
}

// A hydrofabric is one thing; half of one in each format is a mistake worth reporting.
TEST_F(HydrofabricReaderFactory_Test, factory_rejects_mixed_formats)
{
    ASSERT_FALSE(geojson_nexus_path.empty()) << "can't find data/nexus_data.geojson";

    try {
        ngen::hydrofabric::make_hydrofabric_reader(v4_0_path, geojson_nexus_path);
        FAIL() << "expected std::runtime_error for mixed-format hydrofabric paths";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("one format"), std::string::npos)
            << "exception message: " << e.what();
    }
}

// Identifying a hydrofabric is a step the factory takes before it builds anything, and one a
// caller could take on its own; these cover it directly rather than through what it produces.

// A pair of GeoJSON paths is identified from the paths alone -- neither file is opened, because
// the format carries no release marker to read.
TEST_F(HydrofabricReaderFactory_Test, detect_geojson_paths)
{
    ASSERT_FALSE(geojson_catchment_path.empty()) << "can't find data/catchment_data.geojson";
    ASSERT_FALSE(geojson_nexus_path.empty()) << "can't find data/nexus_data.geojson";

    EXPECT_EQ(
        ngen::hydrofabric::detect_hydrofabric(geojson_catchment_path, geojson_nexus_path),
        ngen::hydrofabric::HydrofabricVersion::V1_GEOJSON
    );
}

// A GeoPackage is opened and its schema read, so the release comes back rather than just the
// format -- including across a split pair, where neither half could be identified alone.
TEST_F(HydrofabricReaderFactory_Test, detect_geopackage_release)
{
    EXPECT_EQ(
        ngen::hydrofabric::detect_hydrofabric(v4_0_path, v4_0_path),
        ngen::hydrofabric::HydrofabricVersion::V4_0
    );
    EXPECT_EQ(
        ngen::hydrofabric::detect_hydrofabric(v4_0_divides_path, v4_0_nexus_path),
        ngen::hydrofabric::HydrofabricVersion::V4_0
    );
}

// A GeoPackage with no `nexus` layer is identified as not a hydrofabric. That is a value here,
// not an error: only asking for a *reader* makes it one.
TEST_F(HydrofabricReaderFactory_Test, detect_geopackage_without_nexus)
{
    EXPECT_EQ(
        ngen::hydrofabric::detect_hydrofabric(v4_0_divides_path, v4_0_divides_path),
        ngen::hydrofabric::HydrofabricVersion::UNRECOGNIZED
    );
}

// Mixed formats are refused at identification, before either file is opened.
TEST_F(HydrofabricReaderFactory_Test, detect_rejects_mixed_formats)
{
    ASSERT_FALSE(geojson_nexus_path.empty()) << "can't find data/nexus_data.geojson";

    try {
        ngen::hydrofabric::detect_hydrofabric(v4_0_path, geojson_nexus_path);
        FAIL() << "expected std::runtime_error for mixed-format hydrofabric paths";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("one format"), std::string::npos)
            << "exception message: " << e.what();
    }
}

// Format comes from the file's own header, so these cover the names an extension test got wrong.

// The case that matters most, because ngen produces it: loading a subdivided hydrofabric under MPI
// appends a rank suffix to both data paths, leaving a GeoPackage named `....gpkg.0`.
TEST_F(HydrofabricReaderFactory_Test, detect_geopackage_with_rank_suffix)
{
    const std::string suffixed = copy_as(v4_0_path, "example_v4_0.gpkg.0");

    EXPECT_EQ(
        ngen::hydrofabric::detect_hydrofabric(suffixed, suffixed),
        ngen::hydrofabric::HydrofabricVersion::V4_0
    );
}

// A GeoPackage keeps being one under a name that says otherwise, including no extension at all.
TEST_F(HydrofabricReaderFactory_Test, detect_geopackage_under_any_name)
{
    const std::string renamed = copy_as(v4_0_path, "hydrofabric_without_an_extension");

    EXPECT_EQ(
        ngen::hydrofabric::detect_hydrofabric(renamed, renamed),
        ngen::hydrofabric::HydrofabricVersion::V4_0
    );
}

// And the converse: a GeoJSON hydrofabric misnamed as a GeoPackage is still read as GeoJSON,
// rather than handed to SQLite to fail on.
TEST_F(HydrofabricReaderFactory_Test, detect_geojson_misnamed_as_geopackage)
{
    ASSERT_FALSE(geojson_catchment_path.empty()) << "can't find data/catchment_data.geojson";
    ASSERT_FALSE(geojson_nexus_path.empty()) << "can't find data/nexus_data.geojson";

    const std::string catchments = copy_as(geojson_catchment_path, "catchment_data.gpkg");
    const std::string nexuses    = copy_as(geojson_nexus_path, "nexus_data.gpkg");

    EXPECT_EQ(
        ngen::hydrofabric::detect_hydrofabric(catchments, nexuses),
        ngen::hydrofabric::HydrofabricVersion::V1_GEOJSON
    );

    // And it loads, rather than merely being identified.
    const std::unique_ptr<ngen::hydrofabric::HydrofabricReader> hydrofabric =
        ngen::hydrofabric::make_hydrofabric_reader(catchments, nexuses);
    EXPECT_EQ(hydrofabric->read_divides({})->get_size(), 3);
}

// Reading the header means a missing file is reported as one, at identification, rather than
// surfacing later as whichever parser it was guessed into.
TEST_F(HydrofabricReaderFactory_Test, detect_rejects_unreadable_path)
{
    const std::string missing = std::string(testing::TempDir()) + "/no_such_hydrofabric.gpkg";
    std::remove(missing.c_str());

    try {
        ngen::hydrofabric::detect_hydrofabric(missing, missing);
        FAIL() << "expected std::runtime_error for an unreadable path";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("cannot open"), std::string::npos)
            << "exception message: " << e.what();
    }
}
