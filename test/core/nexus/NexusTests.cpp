#include "gtest/gtest.h"

#include "HY_PointHydroNexus.hpp"
#include "HY_HydroLocation.hpp"
#include "HY_IndirectPosition.hpp"

#include <vector>
#include <memory>
using namespace hy_features::hydrolocation;

class Nexus_Test : public ::testing::Test {

protected:

    Nexus_Test()
    {

    }

    ~Nexus_Test() override {

    }

    void SetUp() override;

    void TearDown() override;

    std::string abridged_json_file;
    std::string complete_json_file;
    std::string id_map_json_file;

};

void Nexus_Test::SetUp() {


}

void Nexus_Test::TearDown() {

}

//! Test that a HY_Hydrolocation can be created.
TEST_F(Nexus_Test, TestInit0)
{
    point_t point(50,30);                                                   //!< Test data location
    //HY_HydroLocationType type(HY_HydroLocationType::undefined);    //!< Test data type
    //HY_IndirectPosition pos;
    //std::shared_ptr<HY_HydroLocation>location = std::make_shared<HY_HydroLocation>(point, type, pos);
    std::vector<std::string> contrib = {"cat-1"};
    //HY_PointHydroNexus("nex-0", location, std::vector<string>(), contrib);
    HY_PointHydroNexus("nex-0", contrib);
    ASSERT_TRUE( true );
}
