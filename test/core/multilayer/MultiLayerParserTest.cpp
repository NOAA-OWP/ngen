
#include "gtest/gtest.h"
#include <Formulation_Manager.hpp>

class MultiLayerParserTest : public ::testing::Test {
    protected:

        MultiLayerParserTest() {

        }

        ~MultiLayerParserTest() override {

        }

        void Setup() override {
            nexus_collection = geojson::read(nexus_data_path.c_str());
            std::cout << "Building Catchment collection" << std::endl;

            catchment_collection = geojson::read(catchement_data_path.c_str());
            
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


}

TEST_F(MultiLayerParserTest, TestInit0)
{
    manager = std::make_shared<realization::Formulation_Manager>(realization_config_path.c_str());
}

TEST_F(MultiLayerParserTest, TestRead0)
{
    manager->read(catchment_collection, utils::getStdOut());
}