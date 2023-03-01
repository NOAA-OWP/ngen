
#include "gtest/gtest.h"
#include <Formulation_Manager.hpp>

class MultiLayerParserTest : public ::testing::Test {
    protected:

        MultiLayerParserTest() : 
            nexus_data_path("../data/multilayer/nexus_data.geojson"),
            catchment_data_path("../data/multilayer/nexus_data.geojson"),
            realization_config_path("../data/multilayer/realization_config.json")
        {

        }

        ~MultiLayerParserTest() override {

        }

        void SetUp() override {
            nexus_collection = geojson::read(nexus_data_path.c_str());
            std::cout << "Building Catchment collection" << std::endl;

            catchment_collection = geojson::read(catchment_data_path.c_str());
            
            // add the catchments to the nexus collection
            for(auto& feature: *catchment_collection)
            {
                nexus_collection->add_feature(feature);
            }
        }

        void TearDown() override {

        }

    geojson::GeoJSON nexus_collection;
    geojson::GeoJSON catchment_collection;
    std::shared_ptr<realization::Formulation_Manager> manager;

    std::string nexus_data_path;
    std::string catchment_data_path;
    std::string realization_config_path;


};

TEST_F(MultiLayerParserTest, TestInit0)
{
    manager = std::make_shared<realization::Formulation_Manager>(realization_config_path.c_str());

    ASSERT_TRUE(true);
}

TEST_F(MultiLayerParserTest, TestRead0)
{
    manager = std::make_shared<realization::Formulation_Manager>(realization_config_path.c_str());
    manager->read(nexus_collection, utils::getStdOut());

    ASSERT_TRUE(true);
}