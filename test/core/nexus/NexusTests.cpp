#include "gtest/gtest.h"

#include "HY_HydroLocation.hpp"
#include <vector>
#include <memory>

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
    HY_HydroLocation::point_t point(50,30);                                                  //!< Test data point
    HY_HydroLocation::polygon_t polygon;                                                     //!< Test data location
    HY_Features::HY_HydroLocationType type(HY_Features::HY_HydroLocationType::undefined);    //!< Test data type

    std::shared_ptr<HY_HydroLocation> location = std::make_shared<HY_HydroLocation>(polygon, type, point);

    ASSERT_TRUE(location != nullptr);
}

