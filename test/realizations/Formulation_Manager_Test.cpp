#include "gtest/gtest.h"
#include <Formulation_Manager.hpp>
#include <Catchment_Formulation.hpp>

#include <features/Features.hpp>
#include <JSONGeometry.hpp>
#include <JSONProperty.hpp>

#include <iostream>
#include <memory>

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class Formulation_Manager_Test : public ::testing::Test {

    protected:


    Formulation_Manager_Test() {

    }

    ~Formulation_Manager_Test() override {

    }

    void SetUp() override {

    }

    void TearDown() override {

    }

    void add_feature(std::string id)
    {
      geojson::three_dimensional_coordinates three_dimensions {
          {
              {1.0, 2.0},
              {3.0, 4.0},
              {5.0, 6.0}
          },
          {
              {7.0, 8.0},
              {9.0, 10.0},
              {11.0, 12.0}
          }
      };
      std::vector<double> bounding_box{1.0, 2.0};
      geojson::PropertyMap properties{
          //{"prop_0", geojson::JSONProperty("prop_0", 0)},
          //{"prop_1", geojson::JSONProperty("prop_1", "1")},
          //{"prop_2", geojson::JSONProperty("prop_2", false)},
          //{"prop_3", geojson::JSONProperty("prop_3", 2.0)}
      };

      geojson::Feature feature = std::make_shared<geojson::PolygonFeature>(geojson::PolygonFeature(
        geojson::polygon(three_dimensions),
        id,
        properties
        //bounding_box
      ));

      fabric->add_feature(feature);
    }

    std::vector<std::string> path_options = {
            "",
            "../",
            "../../",
            "./test/",
            "../test/",
            "../../test/"

    };

    std::string fix_paths(std::string json)
    {
        std::vector<std::string> forcing_paths = {
                "./data/forcing/cat-52_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "./data/forcing/cat-67_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "./data/forcing/cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "./data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv"
        };
        std::vector<std::string> v = {};
        for(unsigned int i = 0; i < path_options.size(); i++){
                v.push_back( path_options[i] + "data/forcing" );
        }
        std::string dir = utils::FileChecker::find_first_readable(v);
        if(dir != ""){
                std::string remove = "\"./data/forcing/\"";
                std::string replace = "\""+dir+"\"";
                //std::cerr<<"TRYING TO REPLACE DIRECTORY... "<<remove<<" -> "<<replace<<std::endl;
                boost::replace_all(json, remove , replace);
        }
        for (unsigned int i = 0; i < forcing_paths.size(); i++) {
          if(json.find(forcing_paths[i]) == std::string::npos){
            continue;
          }
          std::vector<std::string> v = {};
          for (unsigned int j = 0; j < path_options.size(); j++) {
            v.push_back(path_options[j] + forcing_paths[i]);
          }
          std::string right_path = utils::FileChecker::find_first_readable(v);
          if(right_path != forcing_paths[i]){
            std::cerr<<"Replacing "<<forcing_paths[i]<<" with "<<right_path<<std::endl;
            boost::replace_all(json, forcing_paths[i] , right_path);
          }
        }
        return json;
    }

    geojson::GeoJSON fabric = std::make_shared<geojson::FeatureCollection>();

};

const double EPSILON = 0.0000001;

const std::string EXAMPLE_1 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
            "{"
              "\"name\": \"tshirt\", "
              "\"params\": {"
                  "\"maxsmc\": 0.81, "
                  "\"wltsmc\": 1.0, "
                  "\"satdk\": 0.48, "
                  "\"satpsi\": 0.1, "
                  "\"slope\": 0.58, "
                  "\"scaled_distribution_fn_shape_parameter\": 1.3, "
                  "\"multiplier\": 1.0, "
                  "\"alpha_fc\": 1.0, "
                  "\"Klf\": 0.0000672, "
                  "\"Kn\": 0.1, "
                  "\"nash_n\": 8, "
                  "\"Cgw\": 1.08, "
                  "\"expon\": 6.0, "
                  "\"max_groundwater_storage_meters\": 16.0, "
                  "\"nash_storage\": [ "
                      "1.0, "
                      "1.0, "
                      "1.0, "
                      "1.0, "
                      "1.0, "
                      "1.0, "
                      "1.0, "
                      "1.0 "
                  "], "
                  "\"soil_storage_percentage\": 1.0, "
                  "\"groundwater_storage_percentage\": 1.0, "
                  "\"timestep\": 3600, "
                  "\"giuh\": { "
                      "\"giuh_path\": \"./test/data/giuh/GIUH.json\", "
                      "\"crosswalk_path\": \"./data/crosswalk.json\" "
                  "} "
              "} "
            "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{id}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"catchments\": { "
        "\"cat-52\": { "
          "\"formulations\": [ "
            "{"
                "\"name\": \"simple_lumped\", "
                "\"params\": {"
                  "\"sr\": [ "
                      "1.0, "
                      "1.0, "
                      "1.0 "
                  "], "
                  "\"storage\": 1.0, "
                  "\"nash_max_storage\": 10.0, "
                  "\"gw_storage\": 1.0, "
                  "\"gw_max_storage\": 1.0, "
                  "\"smax\": 1000.0, "
                  "\"a\": 1.0, "
                  "\"b\": 10.0, "
                  "\"Ks\": 0.1, "
                  "\"Kq\": 0.01, "
                  "\"n\": 3, "
                  "\"t\": 0 "
                "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "}, "
        "\"cat-67\": { "
        "\"formulations\": [ "
            "{"
              "\"name\": \"tshirt\", "
              "\"params\": {"
                "\"maxsmc\": 0.81, "
                "\"wltsmc\": 1.0, "
                "\"satdk\": 0.48, "
                "\"satpsi\": 0.1, "
                "\"slope\": 0.58, "
                "\"scaled_distribution_fn_shape_parameter\": 1.3, "
                "\"multiplier\": 1.0, "
                "\"alpha_fc\": 1.0, "
                "\"Klf\": 0.0000672, "
                "\"Kn\": 0.1, "
                "\"nash_n\": 8, "
                "\"Cgw\": 1.08, "
                "\"expon\": 6.0, "
                "\"max_groundwater_storage_meters\": 16.0, "
                "\"nash_storage\": [ "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0 "
                "], "
                "\"soil_storage_percentage\": 1.0, "
                "\"groundwater_storage_percentage\": 1.0, "
                "\"timestep\": 3600, "
                "\"giuh\": { "
                    "\"giuh_path\": \"./test/data/giuh/GIUH.json\", "
                    "\"crosswalk_path\": \"./data/crosswalk.json\" "
                "} "
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "} "
    "} "
"}";

const std::string EXAMPLE_2 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\": \"tshirt\", "
          "\"params\": {"
            "\"maxsmc\": 0.81, "
            "\"wltsmc\": 1.0, "
            "\"satdk\": 0.48, "
            "\"satpsi\": 0.1, "
            "\"slope\": 0.58, "
            "\"scaled_distribution_fn_shape_parameter\": 1.3, "
            "\"multiplier\": 1.0, "
            "\"alpha_fc\": 1.0, "
            "\"Klf\": 0.0000672, "
            "\"Kn\": 0.1, "
            "\"nash_n\": 8, "
            "\"Cgw\": 1.08, "
            "\"expon\": 6.0, "
            "\"max_groundwater_storage_meters\": 16.0, "
            "\"nash_storage\": [ "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0 "
            "], "
            "\"soil_storage_percentage\": 1.0, "
            "\"groundwater_storage_percentage\": 1.0, "
            "\"timestep\": 3600, "
            "\"giuh\": { "
                "\"giuh_path\": \"./test/data/giuh/GIUH.json\", "
                "\"crosswalk_path\": \"./data/crosswalk.json\" "
            "} "
          "} "
        "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{ID}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"catchments\": { "
        "\"cat-52\": { "
        "\"formulations\": [ "
          "{"
            "\"name\": \"simple_lumped\", "
            "\"params\": {"
                "\"sr\": [ "
                    "1.0, "
                    "1.0, "
                    "1.0 "
                "], "
                "\"storage\": 1.0, "
                "\"nash_max_storage\": 10.0, "
                "\"gw_storage\": 1.0, "
                "\"gw_max_storage\": 1.0, "
                "\"smax\": 1000.0, "
                "\"a\": 1.0, "
                "\"b\": 10.0, "
                "\"Ks\": 0.1, "
                "\"Kq\": 0.01, "
                "\"n\": 3, "
                "\"t\": 0 "
            "} "
          "} "
        "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "}, "
        "\"cat-67\": { "
          "\"formulations\": [ "
            "{"
              "\"name\": \"tshirt\", "
              "\"params\": {"

                "\"maxsmc\": 0.81, "
                "\"wltsmc\": 1.0, "
                "\"satdk\": 0.48, "
                "\"satpsi\": 0.1, "
                "\"slope\": 0.58, "
                "\"scaled_distribution_fn_shape_parameter\": 1.3, "
                "\"multiplier\": 1.0, "
                "\"alpha_fc\": 1.0, "
                "\"Klf\": 0.0000672, "
                "\"Kn\": 0.1, "
                "\"nash_n\": 8, "
                "\"Cgw\": 1.08, "
                "\"expon\": 6.0, "
                "\"max_groundwater_storage_meters\": 16.0, "
                "\"nash_storage\": [ "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0 "
                "], "
                "\"soil_storage_percentage\": 1.0, "
                "\"groundwater_storage_percentage\": 1.0, "
                "\"timestep\": 3600, "
                "\"giuh\": { "
                    "\"giuh_path\": \"./test/data/giuh/GIUH.json\", "
                    "\"crosswalk_path\": \"./data/crosswalk.json\" "
                "} "
            "} "
          "}"
        "],"
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "} "
    "} "
"}";

const std::string EXAMPLE_3 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\": \"tshirt\", "
          "\"params\": {"

            "\"maxsmc\": 0.81, "
            "\"wltsmc\": 1.0, "
            "\"satdk\": 0.48, "
            "\"satpsi\": 0.1, "
            "\"slope\": 0.58, "
            "\"scaled_distribution_fn_shape_parameter\": 1.3, "
            "\"multiplier\": 1.0, "
            "\"alpha_fc\": 1.0, "
            "\"Klf\": 0.0000672, "
            "\"Kn\": 0.1, "
            "\"nash_n\": 8, "
            "\"Cgw\": 1.08, "
            "\"expon\": 6.0, "
            "\"max_groundwater_storage_meters\": 16.0, "
            "\"nash_storage\": [ "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0 "
            "], "
            "\"soil_storage_percentage\": 1.0, "
            "\"groundwater_storage_percentage\": 1.0, "
            "\"timestep\": 3600, "
            "\"giuh\": { "
                "\"giuh_path\": \"./test/data/giuh/GIUH.json\", "
                "\"crosswalk_path\": \"./data/crosswalk.json\" "
            "} "
        "} "
      "} "
    "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{ID}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"catchments\": { "
        "\"cat-52\": { "
          "\"formulations\": [ "
            "{"
              "\"name\": \"simple_lumped\", "
              "\"params\": {"
                  "\"sr\": [ "
                      "1.0, "
                      "1.0, "
                      "1.0 "
                  "], "
                  "\"storage\": 1.0, "
                  "\"nash_max_storage\": 10.0, "
                  "\"gw_storage\": 1.0, "
                  "\"gw_max_storage\": 1.0, "
                  "\"smax\": 1000.0, "
                  "\"a\": 1.0, "
                  "\"b\": 10.0, "
                  "\"Ks\": 0.1, "
                  "\"Kq\": 0.01, "
                  "\"n\": 3, "
                  "\"t\": 0 "
              "} "
            "}"
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "}, "
        "\"cat-67\": { "
          "\"formulations\": [ "
            "{"
              "\"name\": \"tshirt_c\", "
              "\"params\": {"

                "\"maxsmc\": 0.439, "
                "\"wltsmc\": 0.066, "
                "\"satdk\": 0.00000338, "
                "\"satpsi\": 0.355, "
                "\"slope\": 1.0, "
                // This is the 'b' (or 'bb') param
                "\"scaled_distribution_fn_shape_parameter\": 4.05, "
                "\"multiplier\": 1000.0, "
                "\"alpha_fc\": 0.33, "
                "\"Klf\": 1.70352, "
                "\"Kn\": 0.03, "
                "\"nash_n\": 2, "
                "\"Cgw\": 0.01, "
                "\"expon\": 6.0, "
                "\"max_groundwater_storage_meters\": 16.0, "
                "\"nash_storage\": [ "
                "   0.0, "
                "   0.0 "
                "], "
                "\"soil_storage_percentage\": 0.667, "
                "\"groundwater_storage_percentage\": 0.5, "
                "\"giuh\": { "
                    "\"cdf_ordinates\": ["
                    "    0.06,"
                    "    0.51,"
                    "    0.28,"
                    "    0.12,"
                    "    0.03"
                    "]"
                "} "
            "} "
          "} "
        "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "} "
    "} "
"}";

const std::string EXAMPLE_4 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\": \"tshirt\", "
          "\"params\": {"

            "\"maxsmc\": 0.81, "
            "\"wltsmc\": 1.0, "
            "\"satdk\": 0.48, "
            "\"satpsi\": 0.1, "
            "\"slope\": 0.58, "
            "\"scaled_distribution_fn_shape_parameter\": 1.3, "
            "\"multiplier\": 1.0, "
            "\"alpha_fc\": 1.0, "
            "\"Klf\": 0.0000672, "
            "\"Kn\": 0.1, "
            "\"nash_n\": 8, "
            "\"Cgw\": 1.08, "
            "\"expon\": 6.0, "
            "\"max_groundwater_storage_meters\": 16.0, "
            "\"nash_storage\": [ "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0 "
            "], "
            "\"soil_storage_percentage\": 1.0, "
            "\"groundwater_storage_percentage\": 1.0, "
            "\"timestep\": 3600, "
            "\"giuh\": { "
                "\"giuh_path\": \"./test/data/giuh/GIUH.json\", "
                "\"crosswalk_path\": \"./data/crosswalk.json\" "
            "} "
        "} "
      "} "
    "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{ID}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"catchments\": { "
        "\"cat-27115\": { "
          "\"formulations\": [ "
            "{"
              "\"name\": \"simple_lumped\", "
              "\"params\": {"
                  "\"sr\": [ "
                      "1.0, "
                      "1.0, "
                      "1.0 "
                  "], "
                  "\"storage\": 1.0, "
                  "\"nash_max_storage\": 10.0, "
                  "\"gw_storage\": 1.0, "
                  "\"gw_max_storage\": 1.0, "
                  "\"smax\": 1000.0, "
                  "\"a\": 1.0, "
                  "\"b\": 10.0, "
                  "\"Ks\": 0.1, "
                  "\"Kq\": 0.01, "
                  "\"n\": 3, "
                  "\"t\": 0 "
              "} "
            "}"
          "], "
          "\"forcing\": { "
              "\"path\": \"./data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv\" "
          "} "
        "}, "
        "\"cat-67\": { "
          "\"formulations\": [ "
            "{"
              "\"name\": \"tshirt_c\", "
              "\"params\": {"

                "\"maxsmc\": 0.439, "
                "\"wltsmc\": 0.066, "
                "\"satdk\": 0.00000338, "
                "\"satpsi\": 0.355, "
                "\"slope\": 1.0, "
                // This is the 'b' (or 'bb') param
                "\"scaled_distribution_fn_shape_parameter\": 4.05, "
                "\"multiplier\": 1000.0, "
                "\"alpha_fc\": 0.33, "
                "\"Klf\": 1.70352, "
                "\"Kn\": 0.03, "
                "\"nash_n\": 2, "
                "\"Cgw\": 0.01, "
                "\"expon\": 6.0, "
                "\"max_groundwater_storage_meters\": 16.0, "
                "\"nash_storage\": [ "
                "   0.0, "
                "   0.0 "
                "], "
                "\"soil_storage_percentage\": 0.667, "
                "\"groundwater_storage_percentage\": 0.5, "
                "\"giuh\": { "
                    "\"cdf_ordinates\": ["
                    "    0.06,"
                    "    0.51,"
                    "    0.28,"
                    "    0.12,"
                    "    0.03"
                    "]"
                "} "
            "} "
          "} "
        "], "
          "\"forcing\": { "
              "\"path\": \"./data/forcing/cat-67_2015-12-01 00_00_00_2015-12-30 23_00_00.csv\" "
          "} "
        "} "
    "} "
"}";

TEST_F(Formulation_Manager_Test, basic_reading_1) {
    std::stringstream stream;

    stream << fix_paths(EXAMPLE_1);

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(stream);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 2);

    ASSERT_TRUE(manager.contains("cat-52"));
    ASSERT_TRUE(manager.contains("cat-67"));
}

TEST_F(Formulation_Manager_Test, basic_reading_2) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_2);

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(stream);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 2);

    ASSERT_TRUE(manager.contains("cat-52"));
    ASSERT_TRUE(manager.contains("cat-67"));
}

TEST_F(Formulation_Manager_Test, basic_run_1) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_1);

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(stream);

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 2);

    std::map<std::string, std::map<long, double>> calculated_results;

    pdm03_struct pdm_et_data;
    pdm_et_data.scaled_distribution_fn_shape_parameter = 1.3;
    pdm_et_data.vegetation_adjustment = 0.99;
    pdm_et_data.model_time_step = 0.0;
    pdm_et_data.max_height_soil_moisture_storerage_tank = 400.0;
    pdm_et_data.maximum_combined_contents = pdm_et_data.max_height_soil_moisture_storerage_tank / (1.0+pdm_et_data.scaled_distribution_fn_shape_parameter);

    std::shared_ptr<pdm03_struct> et_params_ptr = std::make_shared<pdm03_struct>(pdm_et_data);

    double dt = 3600.0;

    for (std::pair<std::string, std::shared_ptr<realization::Formulation>> formulation : manager) {
        if (calculated_results.count(formulation.first) == 0) {
            calculated_results.emplace(formulation.first, std::map<long, double>());
        }

        double calculation;

        formulation.second->set_et_params(et_params_ptr);

        for (long t = 0; t < 4; t++) {
            calculation = formulation.second->get_response(t, dt);

            calculated_results.at(formulation.first).emplace(t, calculation);
        }
    }
}

TEST_F(Formulation_Manager_Test, basic_run_3) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_3);

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(stream);

    this->add_feature("cat-67");
    manager.read(this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));

    std::vector<double> expected_results = {191.106140 / 1000.0, 177.198214 / 1000.0, 165.163302 / 1000.0};

    std::vector<double> actual_results(expected_results.size());

    for (int i = 0; i < expected_results.size(); i++) {
        // Remember that for the Tshirt_C_Realization, the timestep sizes are implicit
        actual_results[i] = manager.get_formulation("cat-67")->get_response(i, 3600);
    }

    for (int i = 0; i < actual_results.size(); i++) {
        double actual = actual_results[i];
        // This is an error margin of the largest of 0.1% of actual value, or 1 mm
        // TODO: this may not be precise enough long-term
        double error_margin = std::max(actual * 0.001, 0.001);
        double expected = expected_results[i];
        double diff = actual > expected ? actual - expected : expected - actual;
        ASSERT_LE(diff, error_margin);
    }
}

TEST_F(Formulation_Manager_Test, read_extra) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_3);

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(stream);

    ASSERT_TRUE(manager.is_empty());
    
    this->add_feature("cat-67");
    manager.read(this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));
}

TEST_F(Formulation_Manager_Test, forcing_provider_specification) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_4);

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(stream);

    this->add_feature("cat-67");
    this->add_feature("cat-27115");
    manager.read(this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 2);
    ASSERT_TRUE(manager.contains("cat-67"));
    ASSERT_TRUE(manager.contains("cat-27115"));

    //NOT
    std::vector<double> expected_results = {191.106140 / 1000.0};

    std::vector<double> actual_results(expected_results.size());

    pdm03_struct pdm_et_data;
    pdm_et_data.scaled_distribution_fn_shape_parameter = 1.3;
    pdm_et_data.vegetation_adjustment = 0.99;
    pdm_et_data.model_time_step = 0.0;
    pdm_et_data.max_height_soil_moisture_storerage_tank = 400.0;
    pdm_et_data.maximum_combined_contents = pdm_et_data.max_height_soil_moisture_storerage_tank / (1.0+pdm_et_data.scaled_distribution_fn_shape_parameter);

    std::shared_ptr<pdm03_struct> et_params_ptr = std::make_shared<pdm03_struct>(pdm_et_data);

    for (std::pair<std::string, std::shared_ptr<realization::Formulation>> formulation : manager) {
        formulation.second->set_et_params(et_params_ptr);
        formulation.second->get_response(0, 3600);
    }

    for (int i = 0; i < actual_results.size(); i++) {
        double actual = actual_results[i];
        // This is an error margin of the largest of 0.1% of actual value, or 1 mm
        // TODO: this may not be precise enough long-term
    }
}

