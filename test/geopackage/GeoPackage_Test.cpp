#include <gtest/gtest.h>

#include "GeoPackage.hpp"
#include "FileChecker.h"

class GeoPackage_Test : public ::testing::Test
{
  protected:
    void SetUp() override 
    {
        this->path = utils::FileChecker::find_first_readable({
            "test/data/routing/gauge_01073000.gpkg",
            "../test/data/routing/gauge_01073000.gpkg",
            "../../test/data/routing/gauge_01073000.gpkg"
        });

        if (this->path.empty()) {
            FAIL() << "can't find gauge_01073000.gpkg";
        }
    }

    void TearDown() override {};

    std::string path;
};

TEST_F(GeoPackage_Test, geopackage_read_test)
{
    const auto gpkg = geopackage::read(this->path, "flowpaths");
}