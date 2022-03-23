#include <functional>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "Bmi_Module_Formulation.hpp"
#include "Bmi_C_Formulation.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "FileChecker.h"
#include "Formulation_Manager.hpp"
#include "Forcing.h"

using namespace realization;
using namespace std;

/**
 * Integration test for the BMI PET model library.
 */
class Bmi_C_Pet_IT : public ::testing::Test {
protected:

    static std::string find_file(std::vector<std::string> dir_opts, const std::string& basename) {
        std::vector<std::string> file_opts(dir_opts.size());
        for (int i = 0; i < dir_opts.size(); ++i)
            file_opts[i] = dir_opts[i] + basename;
        return utils::FileChecker::find_first_readable(file_opts);
    }

    static std::string get_friend_bmi_init_config(const Bmi_C_Formulation& formulation) {
        return formulation.get_bmi_init_config();
    }

    static std::string get_friend_bmi_main_output_var(const Bmi_C_Formulation& formulation) {
        return formulation.get_bmi_main_output_var();
    }

    static std::string get_friend_forcing_file_path(const Bmi_C_Formulation& formulation) {
        return formulation.get_forcing_file_path();
    }

    static bool get_friend_is_bmi_using_forcing_file(const Bmi_C_Formulation& formulation) {
        return formulation.is_bmi_using_forcing_file();
    }

    static std::string get_friend_model_type_name(Bmi_C_Formulation& formulation) {
        return formulation.get_model_type_name();
    }

    static double get_friend_var_value_as_double(Bmi_C_Formulation& formulation, const string& var_name) {
        return formulation.get_var_value_as_double(var_name);
    }

    void SetUp() override;

    void TearDown() override;

    std::vector<std::string> forcing_dir_opts;
    std::vector<std::string> bmi_init_cfg_dir_opts;
    std::vector<std::string> lib_dir_opts;

    std::vector<std::string> config_json;
    std::vector<std::string> catchment_ids;
    std::vector<std::string> model_type_name;
    std::vector<std::string> forcing_file;
    std::vector<std::string> lib_file;
    std::vector<std::string> init_config;
    std::vector<std::string> main_output_variable;
    std::vector<std::string> registration_functions;
    std::vector<bool> uses_forcing_file;
    std::vector<std::shared_ptr<forcing_params>> forcing_params_examples;
    std::vector<geojson::GeoJSON> config_properties;
    std::vector<boost::property_tree::ptree> config_prop_ptree;

};

void Bmi_C_Pet_IT::SetUp() {
    testing::Test::SetUp();

#define EX_COUNT 2 

    forcing_dir_opts = {"./data/forcing/", "../data/forcing/", "../../data/forcing/"};
    bmi_init_cfg_dir_opts = {"./data/bmi/c/pet/", "../data/bmi/c/pet/", "../../data/bmi/c/pet/"};
    lib_dir_opts = {"./extern/evapotranspiration/evapotranspiration/cmake_build/", "../extern/evapotranspiration/evapotranspiration/cmake_build/", "../../extern/evapotranspiration/evapotranspiration/cmake_build/"};

    config_json = std::vector<std::string>(EX_COUNT);
    catchment_ids = std::vector<std::string>(EX_COUNT);
    model_type_name = std::vector<std::string>(EX_COUNT);
    forcing_file = std::vector<std::string>(EX_COUNT);
    lib_file = std::vector<std::string>(EX_COUNT);
    registration_functions = std::vector<std::string>(EX_COUNT);
    init_config = std::vector<std::string>(EX_COUNT);
    main_output_variable = std::vector<std::string>(EX_COUNT);
    uses_forcing_file = std::vector<bool>(EX_COUNT);
    forcing_params_examples = std::vector<std::shared_ptr<forcing_params>>(EX_COUNT);
    config_properties = std::vector<geojson::GeoJSON>(EX_COUNT);
    config_prop_ptree = std::vector<boost::property_tree::ptree>(EX_COUNT);

    /* Set up the basic/explicit example index details in the arrays */
    catchment_ids[0] = "cat-27";
    model_type_name[0] = "bmi_c_pet";
    forcing_file[0] = find_file(forcing_dir_opts, "cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
    lib_file[0] = find_file(lib_dir_opts, "libpetbmi.so");
    registration_functions[0] = "register_bmi_pet";
    init_config[0] = find_file(bmi_init_cfg_dir_opts, "cat-27_bmi_config.ini");
    main_output_variable[0] = "water_potential_evaporation_flux";
    uses_forcing_file[0] = true;

    catchment_ids[1] = "cat-67";
    model_type_name[1] = "bmi_c_pet";
    forcing_file[1] = find_file(forcing_dir_opts, "cat-67_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
    lib_file[1] = find_file(lib_dir_opts, "libpetbmi.so");
    registration_functions[1] = "register_bmi_pet";
    init_config[1] = find_file(bmi_init_cfg_dir_opts, "cat-67_bmi_config.ini");
    main_output_variable[1] = "water_potential_evaporation_flux";
    uses_forcing_file[1] = true;

    std::string output_variables = "                \"output_variables\": [\"water_potential_evaporation_flux\"],\n";

    /* Set up the derived example details */
    int i = 0;
    for (int i = 0; i < EX_COUNT; i++) {
        std::shared_ptr<forcing_params> params = std::make_shared<forcing_params>(
                forcing_params(forcing_file[i], "legacy", "2015-12-01 00:00:00", "2015-12-30 23:00:00"));
        std::string variables_line = (i == 0) ? output_variables : "";
        forcing_params_examples[i] = params;
             config_json[i] = "{"
                         "    \"global\": {},"
                         "    \"catchments\": {"
                         "        \"" + catchment_ids[i] + "\": {"
                         "            \"bmi_c\": {"
                         "                \"model_type_name\": \"" + model_type_name[i] + "\","
                         "                \"library_file\": \"" + lib_file[i] + "\","
                         "                \"forcing_file\": \"" + forcing_file[i] + "\","
                         "                \"init_config\": \"" + init_config[i] + "\","
                         "                \"main_output_variable\": \"" + main_output_variable[i] + "\","
                         "                \"" + BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION + "\": 9, "
                         "                \"" + BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES + "\": { "
                         "                      \"land_surface_air__temperature\": \"" + AORC_FIELD_NAME_TEMP_2M_AG + "\","
                         "                      \"land_surface_air__pressure\": \"" + AORC_FIELD_NAME_PRESSURE_SURFACE + "\","
                         "                      \"atmosphere_air_water~vapor__relative_saturation\": \"" + AORC_FIELD_NAME_SPEC_HUMID_2M_AG + "\","
                         "                      \"land_surface_radiation~incoming~shortwave__energy_flux\": \"" + AORC_FIELD_NAME_SOLAR_SHORTWAVE + "\","
                         "                      \"land_surface_radiation~incoming~longwave__energy_flux\": \"" + AORC_FIELD_NAME_SOLAR_LONGWAVE + "\","
                         "                      \"land_surface_wind__x_component_of_velocity\": \"" + AORC_FIELD_NAME_WIND_U_10M_AG + "\","
                         "                      \"land_surface_wind__y_component_of_velocity\": \"" + AORC_FIELD_NAME_WIND_V_10M_AG + "\""
                         "                },"
                         "                \"registration_function\": \"" + registration_functions[i] + "\","
                         + variables_line +
                         "                \"uses_forcing_file\": " + (uses_forcing_file[i] ? "true" : "false") + ""
                         "            },"
                         "            \"forcing\": { \"path\": \"" + forcing_file[i] + "\"}"
                         "        }"
                         "    }"
                         "}";
        std::stringstream stream;
        stream << config_json[i];
        //cout << stream.str();
        boost::property_tree::ptree loaded_tree;
        boost::property_tree::json_parser::read_json(stream, loaded_tree);
        config_prop_ptree[i] = loaded_tree.get_child("catchments").get_child(catchment_ids[i]).get_child("bmi_c");
    }
}

void Bmi_C_Pet_IT::TearDown() {
    Test::TearDown();
}

/** Test to make sure the model initializes. */
TEST_F(Bmi_C_Pet_IT, Test_InitModel) {
    int ex_index = 0;
    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_model_type_name(formulation), model_type_name[ex_index]);
    ASSERT_EQ(get_friend_forcing_file_path(formulation), forcing_file[ex_index]);
    ASSERT_EQ(get_friend_bmi_init_config(formulation), init_config[ex_index]);
    ASSERT_EQ(get_friend_bmi_main_output_var(formulation), main_output_variable[ex_index]);
    ASSERT_EQ(get_friend_is_bmi_using_forcing_file(formulation), uses_forcing_file[ex_index]);
}

/** Test of get response. */
TEST_F(Bmi_C_Pet_IT, Test_GetResponse) {
    // Used to select the example config from what the testing Setup() function sets up.
    int ex_index = 0;
    
    // Compare within a margin of error
    double error_margin = 0.001;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(25, 3600);

    double expected = 5.19727e-08;

    ASSERT_NEAR(expected, response, error_margin);
   
}
/** Compare of BMI PET and PET output benchmark results */
TEST_F(Bmi_C_Pet_IT, Test_Cat27_Method5_CompareBenchmark) {
    // Used to select the example config from what the testing Setup() function sets up.
    int ex_index = 0;
    
    // Compare within a margin of error
    double error_margin = 0.000001;

    /* Output variables for BMI pet are:
     *
     * (0) "water_potential_evaporation_flux",
     */
    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());

    formulation.create_formulation(config_prop_ptree[ex_index]);
   
    double output_benchmark[10] = {0.000000016, 0.000000000, 0.000000088, 0.000000064, 
                                   0.000000060, 0.000000054, 0.000000000, 0.000000037, 
                                   0.000000000, 0.000000262};
    int k = 0;
    
    for (int i = 0; i < 10; i++) 
    {
        formulation.get_response(k, 3600);
    
        double val = get_friend_var_value_as_double(formulation, "water_potential_evaporation_flux");
    
        printf("Timestep index %d: response=%.9f\tbenchmark=%.9f\n", k, val, output_benchmark[i]);
    
        k += 10; //increase time step index by 10
    
        ASSERT_NEAR(val, output_benchmark[i], error_margin);
    }
}

/** Compare of BMI PET and PET output benchmark results */
TEST_F(Bmi_C_Pet_IT, Test_Cat67_Method1_CompareBenchmark) {
    // Used to select the example config from what the testing Setup() function sets up.
    int ex_index = 1;
    
    // Compare within a margin of error
    double error_margin = 0.000001;

    /* Output variables for BMI pet are:
     *
     * (0) "water_potential_evaporation_flux",
     */
    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());

    formulation.create_formulation(config_prop_ptree[ex_index]);
   
    double output_benchmark[10] = {0.000000000, 0.000000000, 0.000000081, 0.000000000, 
                                   0.000000055, 0.000000000, 0.000000000, 0.000000000, 
                                   0.000000000, 0.000000109};
    int k = 0;
    for (int i = 0; i < 10; i++) 
    {
        formulation.get_response(k, 3600);
 
        double val = get_friend_var_value_as_double(formulation, "water_potential_evaporation_flux");
 
        printf("Timestep index %d: response=%.9f\tbenchmark=%.9f\n", k, val, output_benchmark[i]);
 
        k += 10; //increase time step index by 10
 
        ASSERT_NEAR(val, output_benchmark[i], error_margin);
    }
}