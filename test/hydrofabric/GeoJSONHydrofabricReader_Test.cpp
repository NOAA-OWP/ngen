#include <gtest/gtest.h>

#include "HydrofabricReaderFactory.hpp"
#include "FileChecker.h"

// The original hydrofabric format: divides and nexuses in two separate GeoJSON files, carrying
// "id" and "toid" directly, with no schema version to detect and nothing to translate.
class GeoJSONHydrofabricReader_Test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        this->catchment_path = utils::FileChecker::find_first_readable({
            "data/catchment_data.geojson",
            "../data/catchment_data.geojson",
            "../../data/catchment_data.geojson"
        });

        if (this->catchment_path.empty()) {
            FAIL() << "can't find data/catchment_data.geojson";
        }

        this->nexus_path = utils::FileChecker::find_first_readable({
            "data/nexus_data.geojson",
            "../data/nexus_data.geojson",
            "../../data/nexus_data.geojson"
        });

        if (this->nexus_path.empty()) {
            FAIL() << "can't find data/nexus_data.geojson";
        }
    }

    std::string catchment_path;
    std::string nexus_path;
};

// Each role must come from its own file, with ids and toids untouched.
TEST_F(GeoJSONHydrofabricReader_Test, geojson_hydrofabric_reads_both_roles)
{
    const std::unique_ptr<ngen::hydrofabric::HydrofabricReader> hydrofabric =
        ngen::hydrofabric::make_hydrofabric_reader(this->catchment_path, this->nexus_path);

    const geojson::GeoJSON divides = hydrofabric->read_divides({});
    ASSERT_EQ(divides->get_size(), 3);
    const int cat = divides->find("cat-67");
    ASSERT_NE(cat, -1);
    EXPECT_EQ(divides->get_feature(cat)->get_property("toid").as_string(), "nex-68");

    const geojson::GeoJSON nexus = hydrofabric->read_nexus({});
    ASSERT_EQ(nexus->get_size(), 3);
    const int nex = nexus->find("nex-26");
    ASSERT_NE(nex, -1);
    EXPECT_EQ(nexus->get_feature(nex)->get_property("toid").as_string(), "cat-26");
}

// An id subset must narrow the result to the intersection of the file and the subset, rather than
// erroring on the ids the file does not have.
TEST_F(GeoJSONHydrofabricReader_Test, geojson_hydrofabric_honors_id_subsets)
{
    const std::unique_ptr<ngen::hydrofabric::HydrofabricReader> hydrofabric =
        ngen::hydrofabric::make_hydrofabric_reader(this->catchment_path, this->nexus_path);

    const geojson::GeoJSON divides = hydrofabric->read_divides({"cat-67", "cat-NOT-PRESENT"});
    ASSERT_EQ(divides->get_size(), 1);
    EXPECT_NE(divides->find("cat-67"), -1);
}

// Without SQLite there is no GeoPackage implementation to dispatch to, and this is the only build
// in which that refusal is reachable -- so it is the only build that can cover it.
//
// It needs a real GeoPackage, because the format is now read from the file rather than its name,
// and this build cannot write one: the fixture builders are not compiled here. A committed
// GeoPackage serves, since recognizing one only means reading its first bytes.
#if !NGEN_WITH_SQLITE3
TEST(HydrofabricReaderFactory_NoSqlite_Test, factory_refuses_geopackage_paths)
{
    const std::string geopackage_path = utils::FileChecker::find_first_readable({
        "test/data/geopackage/example.gpkg",
        "../test/data/geopackage/example.gpkg",
        "../../test/data/geopackage/example.gpkg"
    });
    ASSERT_FALSE(geopackage_path.empty()) << "can't find test/data/geopackage/example.gpkg";

    try {
        ngen::hydrofabric::make_hydrofabric_reader(geopackage_path);
        FAIL() << "expected std::runtime_error without SQLite support";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("SQLite3 support required"), std::string::npos)
            << "exception message: " << e.what();
    }
}
#endif
