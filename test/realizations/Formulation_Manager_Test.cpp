#include "Bmi_Testing_Util.hpp"
#include "Bmi_Cpp_Formulation.hpp"
#include "DataProvider.hpp"
#include "DataProviderSelectors.hpp"
#include "FileChecker.h"
#include "StreamHandler.hpp"
#include "gtest/gtest.h"
#include <Formulation_Manager.hpp>
#include <Catchment_Formulation.hpp>

#include <features/Features.hpp>
#include <JSONGeometry.hpp>
#include <JSONProperty.hpp>
#include <HY_Features.hpp>
#include "realizations/config/catchment_output.hpp"

#include <iostream>
#include <memory>
#include <filesystem>

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>

#include <AorcForcing.hpp>
#include <Bmi_Formulation.hpp>

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

    void add_feature(const std::string& id, geojson::PropertyMap properties)
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

      geojson::Feature feature = std::make_shared<geojson::PolygonFeature>(geojson::PolygonFeature(
        geojson::polygon(three_dimensions),
        id,
        properties
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

    void replace_paths(std::string& input, const std::string& pattern, const std::string& replacement)
    {
        std::vector<std::string> v{path_options.size()};
        for(unsigned int i = 0; i < path_options.size(); i++)
            v[i] = path_options[i] + replacement;
        
        const std::string dir = utils::FileChecker::find_first_readable(v);
        if (dir == "") {
            // std::cerr << "Can't find any of:\n";
            // for (const auto& s : v)
            //   std::cerr << "  - " << s << '\n';
            return;
        }

        boost::replace_all(input, pattern, dir);
    }

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
      
        //BMI_C_INIT_DIR_PATH
        replace_paths(json, "{{BMI_C_INIT_DIR_PATH}}", "data/bmi/test_bmi_c");
        //BMI_CPP_INIT_DIR_PATH
        replace_paths(json, "{{BMI_CPP_INIT_DIR_PATH}}", "data/bmi/test_bmi_cpp");
        //EXTERN_DIR_PATH
        replace_paths(json, "{{EXTERN_LIB_DIR_PATH}}", "extern/test_bmi_cpp/cmake_build/");
        
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
          "\"name\":\"bmi_c++\","
          "\"params\": {"
            "\"model_type_name\": \"test_bmi_cpp\","
            "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
            "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
            "\"main_output_variable\": \"OUTPUT_VAR_2\","
            "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
              "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
              "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "},"
            "\"create_function\": \"bmi_model_create\","
            "\"destroy_function\": \"bmi_model_destroy\","
            "\"uses_forcing_file\": false"
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
    "\"disable_catchment_output\": true,"
    "\"catchments\": { "
        "\"cat-52\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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
    "\"output_root\": \"./output_dir/\","
    "\"global\": { "
      "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{ID}}.*.csv\", " // DIFF from Ex.1
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
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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

const std::string EXAMPLE_3 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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
        "\"cat-27115\": { " // DIFF from Ex.1+2+3
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { " // BLOCK DIFF from Ex.1+2+3
              "\"path\": \"./data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv\" "
          "} "
        "}, "
        "\"cat-67\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
        "], "
          "\"forcing\": { " // BLOCK DIFF from Ex.1+2+3
              "\"path\": \"./data/forcing/cat-67_2015-12-01 00_00_00_2015-12-30 23_00_00.csv\" "
          "} "
        "} "
    "} "
"}";

/**
 * Configuration for model_params parsing at levels:
 * - global single-bmi
 * - catchment-specific single-bmi
 * - catchment-specific multi-bmi
 */
const std::string EXAMPLE_5_a =
"{"
"    \"global\": {"
"        \"formulations\": ["
"            {"
"                \"name\": \"bmi_c++\","
"                \"params\": {"
"                    \"model_type_name\": \"test_bmi_c++\","
"                    \"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
"                    \"init_config\": \"{{BMI_CPP_INIT_DIR_PATH}}/test_bmi_cpp_config_2.txt\","
"                    \"allow_exceed_end_time\": true,"
"                    \"main_output_variable\": \"OUTPUT_VAR_4\","
"                    \"uses_forcing_file\": false,"
"                    \"model_params\": {"
"                        \"MODEL_VAR_1\": {"
"                            \"source\": \"hydrofabric\","
"                            \"from\": \"pi\""
"                        },"
"                        \"MODEL_VAR_2\": {"
"                            \"source\": \"hydrofabric\""
"                        }"
"                    },"
"                    \"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": {"
"                        \"INPUT_VAR_1\": \"APCP_surface\","
"                        \"INPUT_VAR_2\": \"APCP_surface\""
"                    }"
"                }"
"            }"
"        ],"
"        \"forcing\": { "
"            \"file_pattern\": \".*{{id}}.*.csv\","
"            \"path\": \"./data/forcing/\","
"            \"provider\": \"CsvPerFeature\""
"        }"
"    },"
"    \"time\": {"
"        \"start_time\": \"2015-12-01 00:00:00\","
"        \"end_time\": \"2015-12-30 23:00:00\","
"        \"output_interval\": 3600"
"    },"
"    \"catchments\": {"
"        \"cat-67\": {"
"            \"formulations\": ["
"                {"
"                    \"name\": \"bmi_c++\","
"                    \"params\": {"
"                        \"model_type_name\": \"test_bmi_c++\","
"                        \"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
"                        \"init_config\": \"{{BMI_CPP_INIT_DIR_PATH}}/test_bmi_cpp_config_2.txt\","
"                        \"allow_exceed_end_time\": true,"
"                        \"main_output_variable\": \"OUTPUT_VAR_4\","
"                        \"uses_forcing_file\": false,"
"                        \"model_params\": {"
"                            \"MODEL_VAR_1\": {"
"                                \"source\": \"hydrofabric\","
"                                \"from\": \"n\""
"                            },"
"                            \"MODEL_VAR_2\": {"
"                                \"source\": \"hydrofabric\""
"                            }"
"                        },"
"                        \"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": {"
"                            \"INPUT_VAR_1\": \"APCP_surface\","
"                            \"INPUT_VAR_2\": \"APCP_surface\""
"                        }"
"                    }"
"                }"
"            ],"
"            \"forcing\": {"
"                \"path\": \"./data/forcing/cat-67_2015-12-01 00_00_00_2015-12-30 23_00_00.csv\""
"            }"
"        },"
"        \"cat-27115\": {"
"            \"formulations\": ["
"                {"
"                    \"name\": \"bmi_multi\","
"                    \"params\": {"
"                        \"model_type_name\": \"bmi_multi_c++\","
"                        \"forcing_file\": \"\","
"                        \"init_config\": \"\","
"                        \"allow_exceed_end_time\": true,"
"                        \"main_output_variable\": \"OUTPUT_VAR_4\","
"                        \"uses_forcing_file\": false,"
"                        \"modules\": ["
"                            {"
"                                \"name\": \"bmi_c++\","
"                                \"params\": {"
"                                    \"model_type_name\": \"test_bmi_c++\","
"                                    \"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
"                                    \"init_config\": \"{{BMI_CPP_INIT_DIR_PATH}}/test_bmi_cpp_config_2.txt\","
"                                    \"allow_exceed_end_time\": true,"
"                                    \"main_output_variable\": \"OUTPUT_VAR_4\","
"                                    \"uses_forcing_file\": false,"
"                                    \"model_params\": {"
"                                        \"MODEL_VAR_1\": {"
"                                            \"source\": \"hydrofabric\","
"                                            \"from\": \"e\""
"                                        },"
"                                        \"MODEL_VAR_2\": {"
"                                            \"source\": \"hydrofabric\""
"                                        }"
"                                    },"
"                                    \"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": {"
"                                        \"INPUT_VAR_1\": \"Q2D\","
"                                        \"INPUT_VAR_2\": \"Q2D\""
"                                    }"
"                                }"
"                            }"
"                        ]"
"                    }"
"                }"
"            ],"
"            \"forcing\": {"
"                \"path\": \"./data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv\""
"            }"
"        }"
"    }"
"}";

/**
 * Configuration for model_params parsing at level:
 * - global multi-bmi
 */
const std::string EXAMPLE_5_b =
"{"
"    \"global\": {"
"         \"formulations\": ["
"             {"
"                 \"name\": \"bmi_multi\","
"                 \"params\": {"
"                     \"model_type_name\": \"bmi_multi_c++\","
"                     \"forcing_file\": \"\","
"                     \"init_config\": \"\","
"                     \"allow_exceed_end_time\": true,"
"                     \"main_output_variable\": \"OUTPUT_VAR_4\","
"                     \"uses_forcing_file\": false,"
"                     \"modules\": ["
"                         {"
"                             \"name\": \"bmi_c++\","
"                             \"params\": {"
"                                 \"model_type_name\": \"test_bmi_c++\","
"                                  \"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
"                                  \"init_config\": \"{{BMI_CPP_INIT_DIR_PATH}}/test_bmi_cpp_config_2.txt\","
"                                 \"allow_exceed_end_time\": true,"
"                                 \"main_output_variable\": \"OUTPUT_VAR_4\","
"                                 \"uses_forcing_file\": false,"
"                                 \"model_params\": {"
"                                     \"MODEL_VAR_1\": {"
"                                         \"source\": \"hydrofabric\","
"                                         \"from\": \"val\""
"                                     },"
"                                     \"MODEL_VAR_2\": {"
"                                         \"source\": \"hydrofabric\""
"                                     }"
"                                 },"
"                                 \"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": {"
"                                     \"INPUT_VAR_1\": \"APCP_surface\","
"                                     \"INPUT_VAR_2\": \"APCP_surface\""
"                                 }"
"                             }"
"                         }"
"                     ]"
"                 }"
"             }"
"         ],"
"        \"forcing\": { "
"            \"file_pattern\": \".*{{id}}.*.csv\","
"            \"path\": \"./data/forcing/\","
"            \"provider\": \"CsvPerFeature\""
"        }"
"    },"
"    \"time\": {"
"        \"start_time\": \"2015-12-01 00:00:00\","
"        \"end_time\": \"2015-12-30 23:00:00\","
"        \"output_interval\": 3600"
"    }"
"}";

const std::string EXAMPLE_6 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\":\"bmi_c++\","
          "\"params\": {"
            "\"model_type_name\": \"test_bmi_cpp\","
            "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
            "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
            "\"main_output_variable\": \"OUTPUT_VAR_2\","
            "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
              "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
              "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "},"
            "\"create_function\": \"bmi_model_create\","
            "\"destroy_function\": \"bmi_model_destroy\","
            "\"uses_forcing_file\": false"
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
    "\"disable_catchment_output\": false,"
    "\"catchments\": { "
        "\"cat-52\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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


const std::string EXAMPLE_7 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\":\"bmi_c++\","
          "\"params\": {"
            "\"model_type_name\": \"test_bmi_cpp\","
            "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
            "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_{{id}}.txt\","
            "\"main_output_variable\": \"OUTPUT_VAR_2\","
            "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
              "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
              "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "},"
            "\"create_function\": \"bmi_model_create\","
            "\"destroy_function\": \"bmi_model_destroy\","
            "\"uses_forcing_file\": false"
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
    "\"disable_catchment_output\": true"
"}";

const std::string EXAMPLE_8 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\":\"bmi_c++\","
          "\"params\": {"
            "\"model_type_name\": \"test_bmi_cpp\","
            "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
            "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/GARBAGE.txt\","
            "\"main_output_variable\": \"OUTPUT_VAR_2\","
            "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
              "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
              "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "},"
            "\"create_function\": \"bmi_model_create\","
            "\"destroy_function\": \"bmi_model_destroy\","
            "\"uses_forcing_file\": false"
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
  "\"disable_catchment_output\": true,"
    "\"catchments\": { "
        "\"cat-67\": { "
        "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_{{id}}.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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


const std::string EXAMPLE_9 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\":\"bmi_c++\","
          "\"params\": {"
            "\"model_type_name\": \"test_bmi_cpp\","
            "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
            "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
            "\"main_output_variable\": \"OUTPUT_VAR_2\","
            "\"cache_input_variable_metadata\": true,"
            "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
              "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
              "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "},"
            "\"create_function\": \"bmi_model_create\","
            "\"destroy_function\": \"bmi_model_destroy\","
            "\"uses_forcing_file\": false"
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
    "\"disable_catchment_output\": true,"
    "\"catchments\": { "
        "\"cat-52\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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

const std::string EXAMPLE_10 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"cache_input_variable_metadata\": true,"
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"cache_input_variable_metadata\": true,"
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
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

TEST_F(Formulation_Manager_Test, basic_reading_1) {
    std::stringstream stream;

    stream << fix_paths(EXAMPLE_1);

    boost::property_tree::ptree realization_config;
    boost::property_tree::json_parser::read_json(stream, realization_config);

    auto possible_simulation_time = realization_config.get_child_optional("time");
    if (!possible_simulation_time) {
        throw std::runtime_error("ERROR: No simulation time period defined.");
    }

    auto simulation_time_config = realization::config::Time(*possible_simulation_time).make_params();

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 2);

    ASSERT_TRUE(manager.contains("cat-52"));
    ASSERT_TRUE(manager.contains("cat-67"));
    ASSERT_EQ(manager.get_output_config().root, "./");
}

TEST_F(Formulation_Manager_Test, basic_reading_2) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_2);

    boost::property_tree::ptree realization_config;
    boost::property_tree::json_parser::read_json(stream, realization_config);

    auto possible_simulation_time = realization_config.get_child_optional("time");
    if (!possible_simulation_time) {
        throw std::runtime_error("ERROR: No simulation time period defined.");
    }

    auto simulation_time_config = realization::config::Time(*possible_simulation_time).make_params();

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 2);

    ASSERT_TRUE(manager.contains("cat-52"));
    ASSERT_TRUE(manager.contains("cat-67"));
    ASSERT_EQ(manager.get_output_config().root, "./output_dir/");
}

namespace {
    realization::Formulation_Manager manager_from_json(const std::string& json) {
        boost::property_tree::ptree tree;
        std::stringstream ss(json);
        boost::property_tree::json_parser::read_json(ss, tree);
        return realization::Formulation_Manager(tree);
    }
}

// Output config parsing (grouping / enable / root / legacy keys) is covered by Output_Test, and
// the factory's config->mode selection by CatchmentOutput_Test. What remains genuinely FM's is that
// it parses the output config at construction and validates it against build support:

// netcdf nexus output maps to per-formulation files when supported, and is
// rejected at construction when NGEN was built without NetCDF.
TEST_F(Formulation_Manager_Test, nexus_netcdf_format_respects_build_support)
{
    const std::string json = R"({ "output": { "nexus": { "format": "netcdf" } } })";
#if NGEN_WITH_NETCDF
    auto manager = manager_from_json(json);
    EXPECT_EQ(manager.get_output_config().nexus.format, realization::config::OutputFormat::netcdf);
#else
    EXPECT_THROW(manager_from_json(json), std::runtime_error);
#endif
}

// ---------------------------------------------------------------------------
// Integration: a real (BMI C++) formulation built by Formulation_Manager feeds the catchment
// output manager the driver builds via the factory. This is the one place the full composition
// (Formulation_Manager -> formulation -> factory -> manager -> file) is exercised end to end; the
// factory's mode selection lives in CatchmentOutput_Test and the CSV layout in
// CatchmentCsvOutputMgr_Test. A mocked (programmatic) hydrofabric avoids on-disk geojson data; the
// manager is scoped so its streams flush/close before the file is read.
// ---------------------------------------------------------------------------

namespace {
    const std::string MOCK_LINK_KEY = "toid";

    // Catchment-only collection (each draining to nexus_id), as manager.read()
    // expects -- read() builds a formulation for every feature it is given.
    geojson::GeoJSON build_catchments(const std::vector<std::string>& catchment_ids,
                                      const std::string& nexus_id)
    {
        geojson::three_dimensional_coordinates coords{
            {{1.0,2.0},{3.0,4.0},{5.0,6.0}}, {{7.0,8.0},{9.0,10.0},{11.0,12.0}}
        };
        auto fabric = std::make_shared<geojson::FeatureCollection>();
        for (const auto& id : catchment_ids) {
            geojson::PropertyMap props{ {MOCK_LINK_KEY, geojson::JSONProperty(MOCK_LINK_KEY, nexus_id)} };
            fabric->add_feature(std::make_shared<geojson::PolygonFeature>(
                geojson::PolygonFeature(geojson::polygon(coords), id, props)));
        }
        return fabric;
    }

    std::vector<std::string> read_lines(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) lines.push_back(line);
        return lines;
    }

    // A global test-BMI-C++ realization config with an output block, driven
    // through fix_paths() by the caller.
    std::string bmi_cpp_output_config(const std::string& output_root, const std::string& catchment_grouping) {
        return std::string("{ ")
          + "\"global\": { \"formulations\": [ { \"name\":\"bmi_c++\", \"params\": {"
          +   "\"model_type_name\": \"test_bmi_cpp\","
          +   "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
          +   "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
          +   "\"main_output_variable\": \"OUTPUT_VAR_2\","
          +   "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { \"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG "\", \"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\" },"
          +   "\"create_function\": \"bmi_model_create\", \"destroy_function\": \"bmi_model_destroy\", \"uses_forcing_file\": false"
          + "} } ], \"forcing\": { \"file_pattern\": \".*{{id}}.*.csv\", \"path\": \"./data/forcing/\", \"provider\": \"CsvPerFeature\" } }, "
          + "\"time\": { \"start_time\": \"2015-12-01 00:00:00\", \"end_time\": \"2015-12-30 23:00:00\", \"output_interval\": 3600 }, "
          + "\"output\": { \"root\": \"" + output_root + "\", \"catchment\": { \"grouping\": \"" + catchment_grouping + "\" } } "
          + "}";
    }
}

TEST_F(Formulation_Manager_Test, aggregated_catchment_output_writes_single_file)
{
    namespace fs = std::filesystem;
    const std::string out_dir = "./test_agg_output_agg";
    fs::remove_all(out_dir);

    std::stringstream stream;
    stream << fix_paths(bmi_cpp_output_config(out_dir, "per_formulation"));
    boost::property_tree::ptree cfg;
    boost::property_tree::json_parser::read_json(stream, cfg);
    auto sim_time = realization::config::Time(*cfg.get_child_optional("time")).make_params();

    std::ostream* raw = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw, [](void*){});
    utils::StreamHandler out(s_ptr);

    {
        auto manager = std::make_shared<realization::Formulation_Manager>(cfg);
        manager->read(sim_time, build_catchments({"cat-52", "cat-67"}, "nex-1"), out);
        // Mirror the driver's per_formulation wiring: gather every catchment's columns, then
        // construct the manager with the full set (output is owned by the driver, not HY_Features);
        // the aggregated file name defaults.
        std::vector<utils::FeatureDescriptor> registrations;
        for (const std::string id : {"cat-52", "cat-67"}) {
            registrations.emplace_back(id, manager->get_formulation(id)->get_output_fields());
        }
        auto mgr = realization::config::make_catchment_output_mgr(
            manager->get_output_config(), std::move(registrations));
    } // streams flush/close on destruction

    const std::string agg_file = out_dir + "/cat_output.csv";
    ASSERT_TRUE(fs::exists(agg_file));
    auto lines = read_lines(agg_file);
    ASSERT_EQ(lines.size(), 1u);                              // header written exactly once
    EXPECT_EQ(lines[0].rfind("catchment_id,Time Step,Time,", 0), 0u);
    EXPECT_FALSE(fs::exists(out_dir + "/cat-52.csv"));        // not per-feature
    EXPECT_FALSE(fs::exists(out_dir + "/cat-67.csv"));

    fs::remove_all(out_dir);
}

TEST_F(Formulation_Manager_Test, basic_run_1) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_1);

    boost::property_tree::ptree realization_config;
    boost::property_tree::json_parser::read_json(stream, realization_config);

    auto possible_simulation_time = realization_config.get_child_optional("time");
    if (!possible_simulation_time) {
        throw std::runtime_error("ERROR: No simulation time period defined.");
    }

    auto simulation_time_config = realization::config::Time(*possible_simulation_time).make_params();

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 2);

    std::map<std::string, std::map<long, double>> calculated_results;

    double dt = 3600.0;

    for (std::pair<std::string, std::shared_ptr<realization::Catchment_Formulation>> formulation : manager) {
        if (calculated_results.count(formulation.first) == 0) {
            calculated_results.emplace(formulation.first, std::map<long, double>());
        }

        double calculation;

        for (long t = 0; t < 4; t++) {
            calculation = formulation.second->get_response(t, dt);

            calculated_results.at(formulation.first).emplace(t, calculation);
        }
    }
}

TEST_F(Formulation_Manager_Test, basic_run_3) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_3);

    boost::property_tree::ptree realization_config;
    boost::property_tree::json_parser::read_json(stream, realization_config);

    auto possible_simulation_time = realization_config.get_child_optional("time");
    if (!possible_simulation_time) {
        throw std::runtime_error("ERROR: No simulation time period defined.");
    }

    auto simulation_time_config = realization::config::Time(*possible_simulation_time).make_params();

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));

    std::vector<double> expected_results = {571.4, 570.6, 569.0};

    std::vector<double> actual_results(expected_results.size());

    for (int i = 0; i < expected_results.size(); i++) {
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

/**
 * Testing config the same as EX 1 (like in basic_run_1) but with `cache_input_variable_metadata` true for global
 * formulation config (which is not all formulations for that configuration, as there are two independently specified).
 */
TEST_F(Formulation_Manager_Test, basic_run_9) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_9);

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(stream);

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 2);

    std::map<std::string, std::map<long, double>> calculated_results;

    double dt = 3600.0;

    for (std::pair<std::string, std::shared_ptr<realization::Catchment_Formulation>> formulation : manager) {
        if (calculated_results.count(formulation.first) == 0) {
            calculated_results.emplace(formulation.first, std::map<long, double>());
        }

        double calculation;

        for (long t = 0; t < 4; t++) {
            calculation = formulation.second->get_response(t, dt);

            calculated_results.at(formulation.first).emplace(t, calculation);
        }
    }
}

/**
 * Testing config the same as EX 3 (like in basic_run_3) but with `cache_input_variable_metadata` true for global and
 * one catchment formulation config (but not the other).
 */
TEST_F(Formulation_Manager_Test, basic_run_10) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_10);

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(stream);

    this->add_feature("cat-67");
    manager.read(this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));

    std::vector<double> expected_results = {571.4, 570.6, 569.0};

    std::vector<double> actual_results(expected_results.size());

    for (int i = 0; i < expected_results.size(); i++) {
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

    boost::property_tree::ptree realization_config;
    boost::property_tree::json_parser::read_json(stream, realization_config);

    auto possible_simulation_time = realization_config.get_child_optional("time");
    if (!possible_simulation_time) {
        throw std::runtime_error("ERROR: No simulation time period defined.");
    }

    auto simulation_time_config = realization::config::Time(*possible_simulation_time).make_params();

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));
}

TEST_F(Formulation_Manager_Test, init_config_pattern_match_global) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_7);

    boost::property_tree::ptree realization_config;
    boost::property_tree::json_parser::read_json(stream, realization_config);

    auto possible_simulation_time = realization_config.get_child_optional("time");
    if (!possible_simulation_time) {
        throw std::runtime_error("ERROR: No simulation time period defined.");
    }

    auto simulation_time_config = realization::config::Time(*possible_simulation_time).make_params();

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));
}

TEST_F(Formulation_Manager_Test, init_config_pattern_match_specific) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_8);

    boost::property_tree::ptree realization_config;
    boost::property_tree::json_parser::read_json(stream, realization_config);

    auto possible_simulation_time = realization_config.get_child_optional("time");
    if (!possible_simulation_time) {
        throw std::runtime_error("ERROR: No simulation time period defined.");
    }

    auto simulation_time_config = realization::config::Time(*possible_simulation_time).make_params();

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));
}

TEST_F(Formulation_Manager_Test, forcing_provider_specification) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_4);

    boost::property_tree::ptree realization_config;
    boost::property_tree::json_parser::read_json(stream, realization_config);

    auto possible_simulation_time = realization_config.get_child_optional("time");
    if (!possible_simulation_time) {
        throw std::runtime_error("ERROR: No simulation time period defined.");
    }

    auto simulation_time_config = realization::config::Time(*possible_simulation_time).make_params();

    std::ostream* raw_pointer = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(raw_pointer, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-67");
    this->add_feature("cat-27115");
    manager.read(simulation_time_config, this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 2);
    ASSERT_TRUE(manager.contains("cat-67"));
    ASSERT_TRUE(manager.contains("cat-27115"));

    //NOT
    std::vector<double> expected_results = {191.106140 / 1000.0};

    std::vector<double> actual_results(expected_results.size());

    for (std::pair<std::string, std::shared_ptr<realization::Catchment_Formulation>> formulation : manager) {
        formulation.second->get_response(0, 3600);
    }

    for (int i = 0; i < actual_results.size(); i++) {
        double actual = actual_results[i];
        // This is an error margin of the largest of 0.1% of actual value, or 1 mm
        // TODO: this may not be precise enough long-term
    }
}

TEST_F(Formulation_Manager_Test, read_external_attributes) {
    std::stringstream stream_a;
    stream_a << fix_paths(EXAMPLE_5_a);

    std::stringstream stream_b;
    stream_b << fix_paths(EXAMPLE_5_b);

    std::ostream* ptr = &std::cout;
    std::shared_ptr<std::ostream> s_ptr(ptr, [](void*) {});
    utils::StreamHandler catchment_output(s_ptr);

    time_step_t ts = 2;

    /**
     * Lambda to add a feature to the fabric, and then assert that its properties exists.
     *
     * Assertions:
     * - Asserts that the feature with `id` correctly has all properties in `properties`.
     */
    auto add_and_check_feature = [&, this](const std::string& id, geojson::PropertyMap properties) {
      this->add_feature(id, properties);
      auto feature = this->fabric->get_feature(id);
      for (auto& pair : properties)
        ASSERT_TRUE(feature->has_property(pair.first));
    };

    /**
     * Lambda to check that formulation values are present in output.
     * 
     * Assertions:
     * - Asserts that the formulation manager contains the given catchment ID.
     * - Asserts that each expected value appears in the formulation's output
     *   values vector at timestep `ts`.
     *
     * @note The output values are retrieved as a vector of doubles
     *       (get_output_values_for_timestep) and checked directly with
     *       `std::find` -- no line formatting or string parsing.
     */
    auto check_formulation_values = [&](auto fm, const std::string& id, std::initializer_list<double> expected) {
        ASSERT_TRUE(fm.contains(id));
        auto formulation = fm.get_formulation(id);
        formulation->get_response(ts, 3600);
        std::vector<double> values = formulation->get_output_values_for_timestep(ts);

        for (auto& expect : expected) {
            ASSERT_NE(std::find(values.begin(), values.end(), expect), values.end());
        }
    };

    boost::property_tree::ptree realization_config_a;
    boost::property_tree::json_parser::read_json(stream_a, realization_config_a);

    auto possible_simulation_time_a = realization_config_a.get_child_optional("time");
    if (!possible_simulation_time_a) {
        throw std::runtime_error("ERROR: No simulation time period defined.");
    }

    auto simulation_time_config_a = realization::config::Time(*possible_simulation_time_a).make_params();

    auto manager = realization::Formulation_Manager(realization_config_a);
  
    add_and_check_feature("cat-67", geojson::PropertyMap{
      { "MODEL_VAR_2", geojson::JSONProperty{"MODEL_VAR_2", 10 } },
      { "n",           geojson::JSONProperty{"n",           1.70352 } }
    });

    add_and_check_feature("cat-52", geojson::PropertyMap{
      { "MODEL_VAR_2", geojson::JSONProperty{"MODEL_VAR_2", 15 } },
      { "pi",           geojson::JSONProperty{"pi",         3.14159 } }
    });

    add_and_check_feature("cat-27115", geojson::PropertyMap{
      { "MODEL_VAR_2", geojson::JSONProperty{"MODEL_VAR_2", 20 } },
      { "e",           geojson::JSONProperty{"e",           2.71828 } }
    });

    manager.read(simulation_time_config_a, this->fabric, catchment_output);

    ASSERT_EQ(manager.get_size(), 3);
    check_formulation_values(manager, "cat-67",    { 1.70352, 10.0 });
    check_formulation_values(manager, "cat-52",    { 3.14159, 15.0 });
    check_formulation_values(manager, "cat-27115", { 2.71828, 20.0 });

    this->fabric->remove_feature_by_id("cat-67");
    this->fabric->remove_feature_by_id("cat-52");
    this->fabric->remove_feature_by_id("cat-27115");

    boost::property_tree::ptree realization_config_b;
    boost::property_tree::json_parser::read_json(stream_b, realization_config_b);

    auto possible_simulation_time_b = realization_config_b.get_child_optional("time");
    if (!possible_simulation_time_b) {
        throw std::runtime_error("ERROR: No simulation time period defined.");
    }

    auto simulation_time_config_b = realization::config::Time(*possible_simulation_time_b).make_params();

    manager = realization::Formulation_Manager(realization_config_b);
   
    //Test that two hydrofabric features, using global formulation (EXAMPLE_5_b)
    //end up with unique hydrofabric parameters in the formulations after they
    //are created and linked to the attributes.  Uses the same parameter name with
    //different values.
 
    add_and_check_feature("cat-67", geojson::PropertyMap{
      { "MODEL_VAR_2", geojson::JSONProperty{"MODEL_VAR_2", 9231 } },
      { "val",           geojson::JSONProperty{"val",       7.41722 } }
    });
    
    add_and_check_feature("cat-27", geojson::PropertyMap{
      { "MODEL_VAR_2", geojson::JSONProperty{"MODEL_VAR_2", 18 } },
      { "val",          geojson::JSONProperty{"val", 3} }
    });
    
    manager.read(simulation_time_config_b, this->fabric, catchment_output);
    
    check_formulation_values(manager, "cat-27",    { 3.00000, 18.0 });
    check_formulation_values(manager, "cat-67", { 7.41722, 9231 });
}
