#include "gtest/gtest.h"
#include "FileChecker.h"
#include <Formulation_Manager.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm/sort.hpp>
#include "DomainLayer.hpp"
#include "Layer.hpp"

class DomainLayerTest : public ::testing::Test {

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

        DomainLayerTest()
        {
            nexus_data_path = find_file(path_options, "./data/nexus_data.geojson");
            catchment_data_path = find_file(path_options, "./data/catchment_data.geojson");
            realization_config_path = find_file(path_options, "./data/example_domainlayer_realization_config.json");
        }

        ~DomainLayerTest() override {

        }

        void SetUp() override {
            nexus_collection = geojson::read(nexus_data_path.c_str());
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

TEST_F(DomainLayerTest, TestInit0)
{
    manager = std::make_shared<realization::Formulation_Manager>(realization_config_path.c_str());

    ASSERT_TRUE(true);
}

TEST_F(DomainLayerTest, TestRead0)
{
    manager = std::make_shared<realization::Formulation_Manager>(realization_config_path.c_str());
    manager->read(catchment_collection, utils::getStdOut());

    // check domain layer description struct is parsed correctly
    ngen::LayerDataStorage& layer_meta_data = manager->get_layer_metadata();
    std::vector<int>& keys = layer_meta_data.get_keys();
    std::vector<double> time_steps;
    for(int i = 0; i < keys.size(); ++i)
    {
        auto& m_data = layer_meta_data.get_layer(keys[i]);
        double c_value = UnitsHelper::get_converted_value(m_data.time_step_units,m_data.time_step,"s");
        time_steps.push_back(c_value);
    }
    //boost::range::sort(keys, std::greater<int>());
    boost::range::sort(keys);
    std::vector<std::shared_ptr<ngen::Layer> > layers;
    layers.resize(keys.size());

    for(long i = 0; i < keys.size(); ++i)
    {
      auto desc = layer_meta_data.get_layer(keys[i]);
      if (i == 0) {
          ASSERT_EQ(desc.name, "surface layer");
      } else {
          ASSERT_EQ(desc.name, "domain_layer");
      }
      ASSERT_EQ(desc.time_step_units, "s");
      ASSERT_EQ(desc.id, i);
      ASSERT_EQ(desc.time_step, 3600);
    }

}
