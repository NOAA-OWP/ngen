
#include "gtest/gtest.h"
#include "FileChecker.h"
#include <Formulation_Manager.hpp>

class MultiLayerParserTest : public ::testing::Test {

    static std::string find_file(std::vector<std::string> dir_opts, const std::string& basename) {
        std::vector<std::string> file_opts(dir_opts.size());
        for (int i = 0; i < dir_opts.size(); ++i)
            file_opts[i] = dir_opts[i] + basename;
        return utils::FileChecker::find_first_readable(file_opts);
    }
    protected:

        std::vector<std::string> path_options = {
            "",
            "../",
            "../../",
            "./test/",
            "../test/",
            "../../test/"
        };

        MultiLayerParserTest()
        {
            nexus_data_path = find_file(path_options, "./data/nexus_data.geojson");
            catchment_data_path = find_file(path_options, "./data/catchment_data_multilayer.geojson");
            realization_config_path = find_file(path_options, "./data/example_multilayer_realization_config.json");
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
    manager->read(catchment_collection, utils::getStdOut());

    ASSERT_TRUE(true);
}