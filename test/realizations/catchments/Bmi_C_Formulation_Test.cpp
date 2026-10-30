#ifndef NGEN_BMI_C_FORMULATION_TEST_CPP
#define NGEN_BMI_C_FORMULATION_TEST_CPP

#ifdef NGEN_BMI_C_LIB_TESTS_ACTIVE

#ifndef BMI_TEST_C_LOCAL_LIB_NAME
#ifdef __APPLE__
    #define BMI_TEST_C_LOCAL_LIB_NAME "libtestbmicmodel.dylib"
#else
#ifdef __GNUC__
    #define BMI_TEST_C_LOCAL_LIB_NAME "libtestbmicmodel.so"
    #endif // __GNUC__
#endif // __APPLE__
#endif // BMI_TEST_C_LOCAL_LIB_NAME

#include "Bmi_Module_Formulation.hpp"
#include "Bmi_C_Formulation.hpp"
#include "utilities/bmi/mass_balance.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <cstdio>
#include <ctime>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "FileChecker.h"
#include "Formulation_Manager.hpp"
#include <boost/date_time.hpp>
#include "../../utils/bmi/MockConfig.hpp"
#include <forcing/CsvPerFeatureForcingProvider.hpp>
using ::testing::MatchesRegex;
using namespace realization;

class Bmi_C_Formulation_Test : public ::testing::Test {
protected:

    static std::string find_file(std::vector<std::string> dir_opts, const std::string& basename) {
        std::vector<std::string> file_opts(dir_opts.size());
        for (int i = 0; i < dir_opts.size(); ++i)
            file_opts[i] = dir_opts[i] + basename;
        return utils::FileChecker::find_first_readable(file_opts);
    }

    static void call_friend_determine_model_time_offset(Bmi_C_Formulation& formulation) {
        formulation.determine_model_time_offset();
    }

    static std::string get_friend_bmi_init_config(const Bmi_C_Formulation& formulation) {
        return formulation.get_bmi_init_config();
    }
    static std::string get_friend_bmi_main_output_var(const Bmi_C_Formulation& formulation) {
        return formulation.get_bmi_main_output_var();
    }

    static std::shared_ptr<models::bmi::Bmi_C_Adapter> get_friend_bmi_model(Bmi_C_Formulation& formulation) {
        return std::dynamic_pointer_cast<models::bmi::Bmi_C_Adapter>(formulation.get_bmi_model());
    }

    static time_t get_friend_bmi_model_start_time_forcing_offset_s(Bmi_C_Formulation& formulation) {
        return formulation.get_bmi_model_start_time_forcing_offset_s();
    }

    static time_t get_friend_forcing_start_time(Bmi_C_Formulation& formulation) {
        return formulation.forcing->get_data_start_time();
    }

    static std::string get_friend_model_type_name(Bmi_C_Formulation& formulation) {
        return formulation.get_model_type_name();
    }

    static bool get_friend_cache_input_variable_metadata(Bmi_C_Formulation& formulation) {
        return formulation.cache_input_variable_metadata;
    }

    static void set_friend_cache_input_variable_metadata(Bmi_C_Formulation& formulation, bool value) {
        formulation.set_cache_input_var_metadata(value);
    }

    static std::set<Bmi_Var_Details>& get_friend_known_bmi_input_vars(Bmi_C_Formulation& formulation) {
        return formulation.known_bmi_input_vars;
    }

    static std::vector<Bmi_Var_Details*>* get_friend_bmi_input_var_details(Bmi_C_Formulation& formulation) {
        return formulation.bmi_input_var_details.get();
    }

    static double get_friend_var_value_as_double(Bmi_C_Formulation& formulation, const std::string& var_name) {
        return models::bmi::GetValue<double>(*formulation.get_bmi_model(), var_name, 0);
    }

    static time_t parse_forcing_time(const std::string& date_time_str) {

        std::locale format = std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S"));

        boost::posix_time::ptime pt;
        std::istringstream is(date_time_str);
        is.imbue(format);
        is >> pt;

        boost::posix_time::ptime timet_start(boost::gregorian::date(1970,1,1));
        boost::posix_time::time_duration diff = pt - timet_start;
        return diff.ticks() / boost::posix_time::time_duration::rep_type::ticks_per_second;
    }

    void SetUp() override;

    void TearDown() override;

    double parse_from_delimited_string(const std::string& str_val, const std::string& delimiter, unsigned int index) {
        std::stringstream str_stream(str_val);

        unsigned int i = 0;
        while (str_stream.good()) {
            std::string substring;
            std::getline(str_stream, substring, delimiter.c_str()[0]);
            if (i++ == index)
                return std::stod(substring);
        }

        throw std::runtime_error("Cannot parse " + std::to_string(index) +
                                 "-th substring that exceeds number of substrings in string [" + str_val + "] (" +
                                 std::to_string(i) + ").");
    }

    std::vector<std::string> forcing_dir_opts;
    std::vector<std::string> bmi_init_cfg_dir_opts;
    std::vector<std::string> lib_dir_opts;

    std::vector<std::string> config_json;
    std::vector<std::string> catchment_ids;
    std::vector<std::string> model_type_name;
    std::vector<bool> caches_input_variable_metadata;
    std::vector<std::string> forcing_file;
    std::vector<std::string> lib_file;
    std::vector<std::string> init_config;
    std::vector<std::string> main_output_variable;
    std::vector<std::string> registration_functions;
    std::vector<bool> uses_forcing_file;
    // std::vector<bool> tries_mass_balance;
    std::vector<std::shared_ptr<forcing_params>> forcing_params_examples;
    std::vector<geojson::GeoJSON> config_properties;
    std::vector<boost::property_tree::ptree> config_prop_ptree;

};

void Bmi_C_Formulation_Test::SetUp() {
    testing::Test::SetUp();

#define EX_COUNT 3

    forcing_dir_opts = {"./data/forcing/", "../data/forcing/", "../../data/forcing/"};
    bmi_init_cfg_dir_opts = {
            "./test/data/bmi/test_bmi_c/",
            "../test/data/bmi/test_bmi_c/",
            "../../test/data/bmi/test_bmi_c/"
    };
    lib_dir_opts = {
            "./extern/test_bmi_c/cmake_build/",
            "../extern/test_bmi_c/cmake_build/",
            "../../extern/test_bmi_c/cmake_build/"
    };

    config_json = std::vector<std::string>(EX_COUNT);
    catchment_ids = std::vector<std::string>(EX_COUNT);
    model_type_name = std::vector<std::string>(EX_COUNT);
    forcing_file = std::vector<std::string>(EX_COUNT);
    lib_file = std::vector<std::string>(EX_COUNT);
    init_config = std::vector<std::string>(EX_COUNT);
    main_output_variable  = std::vector<std::string>(EX_COUNT);
    caches_input_variable_metadata = std::vector<bool>(EX_COUNT);
    registration_functions  = std::vector<std::string>(EX_COUNT);
    uses_forcing_file = std::vector<bool>(EX_COUNT);
    // tries_mass_balance = std::vector<bool>(EX_COUNT);
    forcing_params_examples = std::vector<std::shared_ptr<forcing_params>>(EX_COUNT);
    config_properties = std::vector<geojson::GeoJSON>(EX_COUNT);
    config_prop_ptree = std::vector<boost::property_tree::ptree>(EX_COUNT);

    /* Set up the basic/explicit example index details in the arrays */
    catchment_ids[0] = "cat-27";
    model_type_name[0] = "test_bmi_c";
    forcing_file[0] = find_file(forcing_dir_opts, "cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
    lib_file[0] = find_file(lib_dir_opts, BMI_TEST_C_LOCAL_LIB_NAME);
    init_config[0] = find_file(bmi_init_cfg_dir_opts, "test_bmi_c_config_0.txt");
    main_output_variable[0] = "OUTPUT_VAR_1";
    caches_input_variable_metadata[0] = false;
    registration_functions[0] = "register_bmi";
    uses_forcing_file[0] = false;
    // tries_mass_balance[0] = true;

    catchment_ids[1] = "cat-27";
    model_type_name[1] = "test_bmi_c";
    forcing_file[1] = find_file(forcing_dir_opts, "cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
    lib_file[1] = find_file(lib_dir_opts, BMI_TEST_C_LOCAL_LIB_NAME);
    init_config[1] = find_file(bmi_init_cfg_dir_opts, "test_bmi_c_config_1.txt");
    main_output_variable[1] = "OUTPUT_VAR_1";
    caches_input_variable_metadata[1] = false;
    registration_functions[1] = "register_bmi";
    uses_forcing_file[1] = false;
    // tries_mass_balance[1] = false;

    catchment_ids[2] = "cat-27";
    model_type_name[2] = "test_bmi_c";
    forcing_file[2] = find_file(forcing_dir_opts, "cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
    lib_file[2] = find_file(lib_dir_opts, BMI_TEST_C_LOCAL_LIB_NAME);
    init_config[2] = find_file(bmi_init_cfg_dir_opts, "test_bmi_c_config_1.txt");
    main_output_variable[2] = "OUTPUT_VAR_1";
    caches_input_variable_metadata[2] = true;
    registration_functions[2] = "register_bmi";
    uses_forcing_file[2] = false;
    // tries_mass_balance[1] = false;

    std::string variables_with_rain_rate = "                \"output_variables\": [\"OUTPUT_VAR_2\",\n"
                                           "                    \"OUTPUT_VAR_1\"],\n";

    /* Set up the derived example details */
    for (int i = 0; i < EX_COUNT; i++) {
        std::shared_ptr<forcing_params> params = std::make_shared<forcing_params>(
                forcing_params(forcing_file[i], "legacy", "2015-12-01 00:00:00", "2015-12-30 23:00:00"));
        std::string variables_line = (i == 1) ? variables_with_rain_rate : "";

        // Add this substring with this key for even indices or if the value for it is true
        std::string cache_metadata_substr = "";
        if (i % 2 == 0 || caches_input_variable_metadata[i]) {
            cache_metadata_substr = "\"" BMI_REALIZATION_CFG_PARAM_OPT__CACHE_INPUT_VAR_METADATA "\": " ;
            cache_metadata_substr += caches_input_variable_metadata[i] ? "true," : "false,";
        }

        forcing_params_examples[i] = params;
        config_json[i] = "{"
                         "    \"global\": {},"
                         "    \"catchments\": {"
                         "        \"" + catchment_ids[i] + "\": {"
                         "            \"bmi_c\": {"
                         "                \"model_type_name\": \"" + model_type_name[i] + "\","
                         "                \"library_file\": \"" + lib_file[i] + "\","
                         "                \"init_config\": \"" + init_config[i] + "\","
                         "                \"main_output_variable\": \"" + main_output_variable[i] + "\","

                         // Add this predetermined substring from above (which could be empty)
                         + cache_metadata_substr +

                         "                \"" + BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES + "\": { "
                         "                      \"INPUT_VAR_2\": \"" + AORC_FIELD_NAME_TEMP_2M_AG + "\","
                         "                      \"INPUT_VAR_1\": \"" + AORC_FIELD_NAME_PRECIP_RATE + "\""
                         "                },"
                         "                \"registration_function\": \"" + registration_functions[i] + "\","
                         + variables_line +
                         "                \"uses_forcing_file\": " + (uses_forcing_file[i] ? "true" : "false") + ","
                         "                \"model_params\": { \"PARAM_VAR_1\": 42, \"PARAM_VAR_2\": 4.2, \"PARAM_VAR_3\": [4, 2]}" +
                        //  (tries_mass_balance[i] ? \
                        //  ",               \"mass_balance\": {\"tolerance\": 1e-12, \"fatal\":true}" \
                        // :"" )+
                         "            },"
                         "            \"forcing\": { \"path\": \"" + forcing_file[i] + "\", \"provider\": \"CsvPerFeature\"}"
                         "        }"
                         "    }"
                         "}";

        std::stringstream stream;
        stream << config_json[i];

        boost::property_tree::ptree loaded_tree;
        boost::property_tree::json_parser::read_json(stream, loaded_tree);
        config_prop_ptree[i] = loaded_tree.get_child("catchments").get_child(catchment_ids[i]).get_child("bmi_c");
    }
}

void Bmi_C_Formulation_Test::TearDown() {
    Test::TearDown();
}

/** Simple test to make sure the model initializes. */
TEST_F(Bmi_C_Formulation_Test, Initialize_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_model_type_name(formulation), model_type_name[ex_index]);
    ASSERT_EQ(get_friend_bmi_init_config(formulation), init_config[ex_index]);
    ASSERT_EQ(get_friend_bmi_main_output_var(formulation), main_output_variable[ex_index]);
}

/** Test example 0 (which should be explicitly configured) has `cache_input_variable_metadata` as `false`. */
TEST_F(Bmi_C_Formulation_Test, Initialize_0_b) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_FALSE(get_friend_cache_input_variable_metadata(formulation));
}

/** Test to make sure we can initialize multiple model instances with dynamic loading. */
TEST_F(Bmi_C_Formulation_Test, Initialize_1_a) {
    Bmi_C_Formulation form_1(catchment_ids[0], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[0]));
    form_1.create_formulation(config_prop_ptree[0]);

    Bmi_C_Formulation form_2(catchment_ids[1], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[1]));
    form_2.create_formulation(config_prop_ptree[1]);

    std::vector<std::string> header_1, header_2;
    for (const auto& f : form_1.get_output_fields()) header_1.push_back(f.output_name);
    for (const auto& f : form_2.get_output_fields()) header_2.push_back(f.output_name);

    EXPECT_THAT(header_1, ::testing::ElementsAre("OUTPUT_VAR_1", "OUTPUT_VAR_2"));
    EXPECT_THAT(header_2, ::testing::ElementsAre("OUTPUT_VAR_2", "OUTPUT_VAR_1"));
}

/** Test example 1 (which should not be explicitly configured) has `cache_input_variable_metadata` as `false`. */
TEST_F(Bmi_C_Formulation_Test, Initialize_1_b) {
    int ex_index = 1;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_FALSE(get_friend_cache_input_variable_metadata(formulation));
}

/** Simple test to make sure the model initializes. */
TEST_F(Bmi_C_Formulation_Test, Initialize_2_a) {
    int ex_index = 2;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_model_type_name(formulation), model_type_name[ex_index]);
    ASSERT_EQ(get_friend_bmi_init_config(formulation), init_config[ex_index]);
    ASSERT_EQ(get_friend_bmi_main_output_var(formulation), main_output_variable[ex_index]);
}

/** Test example 2 has `cache_input_variable_metadata` as `true`. */
TEST_F(Bmi_C_Formulation_Test, Initialize_2_b) {
    int ex_index = 2;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_TRUE(get_friend_cache_input_variable_metadata(formulation));
}

/** Simple test of get response. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    ASSERT_EQ(response, 00);
}

/** Simple test of initial parameter setting. */
TEST_F(Bmi_C_Formulation_Test, set_initial_parameters_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    static std::shared_ptr<models::bmi::Bmi_C_Adapter> bmi_c_adapter;
    bmi_c_adapter =  get_friend_bmi_model(formulation);
    int param = models::bmi::GetValue<int>(*bmi_c_adapter, "PARAM_VAR_1")[0];
    ASSERT_EQ(param, 42);
    double param2 = models::bmi::GetValue<double>(*bmi_c_adapter, "PARAM_VAR_2")[0];
    ASSERT_EQ(param2, 4.2);
    std::vector<double> param3 = models::bmi::GetValue<double>(*bmi_c_adapter, "PARAM_VAR_3");
    ASSERT_EQ(param3.size(), 2);
    ASSERT_EQ(param3[0], 4.0);
    ASSERT_EQ(param3[1], 2.0);
}

/** Test of get response after several iterations. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_0_b) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response;
    for (int i = 0; i < 39; i++) {
        response = formulation.get_response(i, 3600);
    }
    double expected = 2.7809780039160068e-08;
    ASSERT_EQ(expected, response);
}

/** Test of get response of example 0 (store metadata is `false`) to make sure no metadata stored. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_0_c) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response;
    for (int i = 0; i < 39; i++) {
        response = formulation.get_response(i, 3600);
    }

    ASSERT_EQ(get_friend_known_bmi_input_vars(formulation).size(), 0);
    ASSERT_EQ(get_friend_bmi_input_var_details(formulation), nullptr);
}

/** Test to make sure we can execute multiple model instances with dynamic loading. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_1_a) {
    Bmi_C_Formulation form_1(catchment_ids[0], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[0]));
    form_1.create_formulation(config_prop_ptree[0]);

    Bmi_C_Formulation form_2(catchment_ids[1], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[1]));
    form_2.create_formulation(config_prop_ptree[1]);

    double response_1, response_2;
    ASSERT_NE(&response_1, &response_2);
    for (size_t i = 0; i < 720; ++i) {
        response_1 = form_1.get_response(0, 3600);
        response_2 = form_2.get_response(0, 3600);
        ASSERT_EQ(response_1, response_2);
    }
}

/** Test of get response for example 2 (using stored metadata) after several iterations. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_2_b) {
    int ex_index = 2;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response;
    for (int i = 0; i < 39; i++) {
        response = formulation.get_response(i, 3600);
    }
    double expected = 2.7809780039160068e-08;
    ASSERT_EQ(expected, response);
}


/** Test of get response of example 2 (store metadata is `true`) to make sure metadata stored. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_2_c) {
    int ex_index = 2;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response;
    for (int i = 0; i < 39; i++) {
        response = formulation.get_response(i, 3600);
    }

    std::vector<Bmi_Var_Details*>* instance_metadata = get_friend_bmi_input_var_details(formulation);
    std::set<Bmi_Var_Details> global_metadata = get_friend_known_bmi_input_vars(formulation);

    ASSERT_NE(instance_metadata, nullptr);
    ASSERT_EQ(instance_metadata->size(), 2);
    ASSERT_EQ(global_metadata.size(), 2);
}

/** Test of `bmi_input_var_details` is populated correctly for example 2. */
TEST_F(Bmi_C_Formulation_Test, bmi_input_var_details_2_a) {
    int ex_index = 2;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response;
    for (int i = 0; i < 39; i++) {
        response = formulation.get_response(i, 3600);
    }

    std::vector<Bmi_Var_Details*>* input_metadata = get_friend_bmi_input_var_details(formulation);

    Bmi_Var_Details* var_metadata = input_metadata->at(0);
    ASSERT_EQ(var_metadata->get_name(), "INPUT_VAR_1");
    ASSERT_EQ(var_metadata->get_mapped_alias(), AORC_FIELD_NAME_PRECIP_RATE);
    ASSERT_EQ(var_metadata->get_units(), "m");
    ASSERT_EQ(var_metadata->get_item_size(), 8);
    ASSERT_EQ(var_metadata->get_num_items(), 1);

    var_metadata = input_metadata->at(1);
    ASSERT_EQ(var_metadata->get_name(), "INPUT_VAR_2");
    ASSERT_EQ(var_metadata->get_mapped_alias(), AORC_FIELD_NAME_TEMP_2M_AG);
    ASSERT_EQ(var_metadata->get_units(), "Pa");
    ASSERT_EQ(var_metadata->get_item_size(), 8);
    ASSERT_EQ(var_metadata->get_num_items(), 1);
}

/** Test of `bmi_input_var_details` values come from same static object in example 2. */
TEST_F(Bmi_C_Formulation_Test, bmi_input_var_details_2_b) {
    int ex_index = 2;

    Bmi_C_Formulation formulation_1(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation_1.create_formulation(config_prop_ptree[ex_index]);

    Bmi_C_Formulation formulation_2(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation_2.create_formulation(config_prop_ptree[ex_index]);

    // Advance only one to start with
    formulation_1.get_response(0, 3600);

    std::vector<Bmi_Var_Details*>* instance_metadata_1 = get_friend_bmi_input_var_details(formulation_1);
    std::vector<Bmi_Var_Details*>* instance_metadata_2 = get_friend_bmi_input_var_details(formulation_2);

    // So, only the first of the instances has actually advanced and populated metadata objects
    ASSERT_NE(instance_metadata_1, nullptr);
    ASSERT_EQ(instance_metadata_2, nullptr);
    ASSERT_EQ(instance_metadata_1->size(), 2);

    // But the static member should be populated
    std::set<Bmi_Var_Details> global_metadata_1 = get_friend_known_bmi_input_vars(formulation_1);
    std::set<Bmi_Var_Details> global_metadata_2 = get_friend_known_bmi_input_vars(formulation_2);
    ASSERT_EQ(global_metadata_1.size(), 2);
    ASSERT_EQ(global_metadata_1.size(), global_metadata_2.size());

    // Now advance formulation 2, and see that it has two var metadata objects
    formulation_2.get_response(0, 3600);
    instance_metadata_2 = get_friend_bmi_input_var_details(formulation_2);
    ASSERT_NE(instance_metadata_2, nullptr);
    ASSERT_EQ(instance_metadata_2->size(), 2);

    // But the static metadata collections won't have grown ...
    ASSERT_EQ(global_metadata_1.size(), 2);
    ASSERT_EQ(global_metadata_1.size(), global_metadata_2.size());

    // And the instance metadata pointers across the two instances (with different collection objects) will be the same
    ASSERT_NE(&instance_metadata_1, &instance_metadata_2);
    for (size_t i = 0; i < instance_metadata_1->size(); i++) {
        Bmi_Var_Details* inst_1_obj_ptr = instance_metadata_1->at(i);
        Bmi_Var_Details* inst_2_obj_ptr = instance_metadata_2->at(i);
        ASSERT_EQ(inst_1_obj_ptr, inst_2_obj_ptr);
    }
}

/** Test of `bmi_input_var_details` values are same objects for two objects with same config based on example 2. */
TEST_F(Bmi_C_Formulation_Test, bmi_input_var_details_2_c) {
    int ex_index = 2;

    Bmi_C_Formulation formulation_1(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation_1.create_formulation(config_prop_ptree[ex_index]);

    Bmi_C_Formulation formulation_2(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation_2.create_formulation(config_prop_ptree[ex_index]);

    for (int i = 0; i < 10; i++) {
        formulation_1.get_response(i, 3600);
        formulation_2.get_response(i, 3600);
    }

    std::vector<Bmi_Var_Details*>* instance_metadata_1 = get_friend_bmi_input_var_details(formulation_1);
    std::vector<Bmi_Var_Details*>* instance_metadata_2 = get_friend_bmi_input_var_details(formulation_2);

    std::set<Bmi_Var_Details> global_metadata_1 = get_friend_known_bmi_input_vars(formulation_1);
    std::set<Bmi_Var_Details> global_metadata_2 = get_friend_known_bmi_input_vars(formulation_2);


    ASSERT_EQ(instance_metadata_1->size(), 2);
    ASSERT_EQ(instance_metadata_1->size(), instance_metadata_2->size());
    ASSERT_EQ(global_metadata_1.size(), 2);
    ASSERT_EQ(global_metadata_1.size(), global_metadata_2.size());

    for (size_t i = 0; i < instance_metadata_1->size(); i++) {
        ASSERT_EQ(instance_metadata_1->at(i), instance_metadata_2->at(i));
    }
}

/**
 * Test that caching input variable metadata yields responses identical to the default refetch path.
 *
 * Two formulations are built from the same example 2 config, differing only in whether
 * `cache_input_variable_metadata` is enabled.  This guards against behavioral divergence between the two
 * execution paths of `set_model_inputs_prior_to_update` (i.e., `do_bmi_sets_from_stored_metadata` versus
 * `do_bmi_sets_with_full_refetch`), including any unintended side effect from how `Bmi_Var_Details`
 * instances are constructed in either path.
 */
TEST_F(Bmi_C_Formulation_Test, cache_matches_refetch_2_a) {
    int ex_index = 2;

    // Cached formulation directly from the example 2 config (which enables caching).
    Bmi_C_Formulation cached(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    cached.create_formulation(config_prop_ptree[ex_index]);
    ASSERT_TRUE(get_friend_cache_input_variable_metadata(cached));

    // Same config, but with caching disabled so the full-refetch path is exercised instead.  The flag is
    // overridden before the first response, since metadata is initialized lazily on that first call.
    Bmi_C_Formulation refetch(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    refetch.create_formulation(config_prop_ptree[ex_index]);
    set_friend_cache_input_variable_metadata(refetch, false);
    ASSERT_FALSE(get_friend_cache_input_variable_metadata(refetch));

    for (int i = 0; i < 39; i++) {
        ASSERT_EQ(cached.get_response(i, 3600), refetch.get_response(i, 3600));
    }
}

/** Simple test of output. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    formulation.get_response(0, 3600);
    std::vector<double> output = formulation.get_output_values_for_timestep(0);
    EXPECT_THAT(output, ::testing::Pointwise(::testing::DoubleNear(1e-15), std::vector<double>{0.0, 571.60003662109375}));
}

/** Simple test of output with modified variables. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_1_a) {
    int ex_index = 1;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    // ex_index=1 configures output_variables as [OUTPUT_VAR_2, OUTPUT_VAR_1], so the values come out in
    // that order (OUTPUT_VAR_2's value first) — the same order asserted for header_2 in Initialize_1_a.

    formulation.get_response(0, 3600);
    std::vector<double> output = formulation.get_output_values_for_timestep(0);
    EXPECT_THAT(output, ::testing::Pointwise(::testing::DoubleNear(1e-15), std::vector<double>{571.60003662109375, 0.0}));
}

/** Simple test of output with modified variables, picking time step when there was non-zero rain rate. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_1_b) {
    int ex_index = 1;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    int i = 0;
    while (i < 542)
        formulation.get_response(i++, 3600);
    formulation.get_response(i, 3600);
    std::vector<double> output = formulation.get_output_values_for_timestep(i);
    EXPECT_THAT(output, ::testing::Pointwise(::testing::DoubleNear(1e-15), std::vector<double>{580.79998779296875, 1.1124674593096233e-06}));
}

/** Simple test of output, picking time step when there was non-zero rain rate. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_2_b) {
    int ex_index = 2;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);

    int i = 0;
    while (i < 542)
        formulation.get_response(i++, 3600);
    formulation.get_response(i, 3600);

    // ex_index=2 does not configure output_variables, so values come out in the model's default
    // order (OUTPUT_VAR_1's value first), unlike ex_index=1 above.
    std::vector<double> output = formulation.get_output_values_for_timestep(i);
    EXPECT_THAT(output, ::testing::Pointwise(::testing::DoubleNear(1e-15), std::vector<double>{1.1124674593096233e-06, 580.79998779296875}));
}

TEST_F(Bmi_C_Formulation_Test, determine_model_time_offset_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);

    double model_start = model_adapter->GetStartTime();
    ASSERT_EQ(model_start, 0.0);
}

TEST_F(Bmi_C_Formulation_Test, determine_model_time_offset_0_b) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);
    time_t forcing_start = get_friend_forcing_start_time(formulation);

    ASSERT_EQ(forcing_start,  parse_forcing_time("2015-12-01 00:00:00"));
}

TEST_F(Bmi_C_Formulation_Test, determine_model_time_offset_0_c) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);

    call_friend_determine_model_time_offset(formulation);

    // Note that this depends on the particular value for forcing time implied by determine_model_time_offset_0_b above
    // it also assumes the model time starts at 0
    auto expected_offset = (time_t) 1448928000;

    ASSERT_EQ(get_friend_bmi_model_start_time_forcing_offset_s(formulation), expected_offset);
}

using models::bmi::protocols::INPUT_MASS_NAME;
using models::bmi::protocols::OUTPUT_MASS_NAME;
using models::bmi::protocols::STORED_MASS_NAME;
using models::bmi::protocols::LEAKED_MASS_NAME;
using models::bmi::protocols::ProtocolError;

TEST_F(Bmi_C_Formulation_Test, check_mass_balance) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    auto ptree = config_prop_ptree[ex_index];
    // mass balance failure will throw an exception
    ptree.put_child("mass_balance", MassBalanceMock(true).get());
    formulation.create_formulation(ptree);
    formulation.check_mass_balance(0, 2, "t0");
    formulation.get_response(1, 3600);
    formulation.check_mass_balance(1, 2, "t1");
}

TEST_F(Bmi_C_Formulation_Test, check_mass_balance_warns) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    auto ptree = config_prop_ptree[ex_index];
    ptree.put_child("mass_balance", MassBalanceMock(false).get());
    formulation.create_formulation(ptree);
    //formulation.check_mass_balance(0, 1, "t0");
    formulation.get_response(1, 3600);
    double mass_error = 100;
    get_friend_bmi_model(formulation)->SetValue(STORED_MASS_NAME, &mass_error); // Force a mass balance error
    //ASSERT_THROW(formulation.check_mass_balance(0, 1, "t0"), MassBalanceWarning);
    testing::internal::CaptureStderr();
    formulation.check_mass_balance(0, 1, "t0");
    std::string output = testing::internal::GetCapturedStderr();
    std::cerr << output;
    EXPECT_THAT(output, testing::HasSubstr("Warning(Protocol)::mass_balance:"));
}

TEST_F(Bmi_C_Formulation_Test, check_mass_balance_stored_fails) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    auto ptree = config_prop_ptree[ex_index];
    ptree.put_child("mass_balance", MassBalanceMock(true).get());
    formulation.create_formulation(ptree);
    //formulation.check_mass_balance(0, 1, "t0");
    formulation.get_response(1, 3600);
    double mass_error = 100;
    get_friend_bmi_model(formulation)->SetValue(STORED_MASS_NAME, &mass_error); // Force a mass balance error
    //formulation.check_mass_balance(0, 1, "t0");
    ASSERT_THROW(formulation.check_mass_balance(0, 1, "t0"), ProtocolError);
    try{
        formulation.check_mass_balance(0, 1, "t0");
    }
    catch (ProtocolError& e) {
        std::cerr << e.to_string() << std::endl;
        EXPECT_THAT(e.to_string(), MatchesRegex("Error\\(Protocol\\)::mass_balance:.*"));
    }
}

TEST_F(Bmi_C_Formulation_Test, check_mass_balance_in_fails_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    auto ptree = config_prop_ptree[ex_index];
    ptree.put_child("mass_balance", MassBalanceMock(true, 1e-5).get());
    formulation.create_formulation(ptree);
    formulation.get_response(1, 3600);
    double mass_error = 2;
    get_friend_bmi_model(formulation)->SetValue(INPUT_MASS_NAME, &mass_error); // Force a mass balance error
    ASSERT_THROW(formulation.check_mass_balance(0, 1, "t0"), ProtocolError);
    try{
        formulation.check_mass_balance(0, 1, "t0");
    }
    catch (ProtocolError& e) {
        std::cerr << e.to_string() << std::endl;
        EXPECT_THAT(e.to_string(), MatchesRegex("Error\\(Protocol\\)::mass_balance:.*"));
    }
}

TEST_F(Bmi_C_Formulation_Test, check_mass_balance_out_fails) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    auto ptree = config_prop_ptree[ex_index];
    ptree.put_child("mass_balance", MassBalanceMock(true).get());
    formulation.create_formulation(ptree);
    formulation.get_response(1, 3600);
    double mass_error = 2;
    get_friend_bmi_model(formulation)->SetValue(OUTPUT_MASS_NAME, &mass_error); // Force a mass balance error
    ASSERT_THROW(formulation.check_mass_balance(0, 1, "t0"), ProtocolError);
    try{
        formulation.check_mass_balance(0, 1, "t0");
    }
    catch (ProtocolError& e) {
        std::cerr << e.to_string() << std::endl;
        EXPECT_THAT(e.to_string(), MatchesRegex("Error\\(Protocol\\)::mass_balance:.*"));
    }
}

TEST_F(Bmi_C_Formulation_Test, check_mass_balance_leaked_fails) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    auto ptree = config_prop_ptree[ex_index];
    ptree.put_child("mass_balance", MassBalanceMock(true).get());
    formulation.create_formulation(ptree);
    formulation.get_response(1, 3600);
    double mass_error = 2;
    get_friend_bmi_model(formulation)->SetValue(LEAKED_MASS_NAME, &mass_error); // Force a mass balance error
    ASSERT_THROW(formulation.check_mass_balance(0, 1, "t0"), ProtocolError);
    try{
        formulation.check_mass_balance(0, 1, "t0");
    }
    catch (ProtocolError& e) {
        std::cerr << e.to_string() << std::endl;
        EXPECT_THAT(e.to_string(), MatchesRegex("Error\\(Protocol\\)::mass_balance:.*"));
    }
}

TEST_F(Bmi_C_Formulation_Test, check_mass_balance_tolerance) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    auto ptree = config_prop_ptree[ex_index];
    ptree.put_child("mass_balance", MassBalanceMock(true, 1e-5).get());
    formulation.create_formulation(ptree);
    //formulation.check_mass_balance(0, "t0");
    formulation.get_response(1, 3600);
    double mass_error;
    get_friend_bmi_model(formulation)->GetValue(INPUT_MASS_NAME, &mass_error);
    mass_error += 1e-4; // Force a mass balance error not within tolerance
    get_friend_bmi_model(formulation)->SetValue(INPUT_MASS_NAME, &mass_error); // Force a mass balance error
    ASSERT_THROW(formulation.check_mass_balance(0, 1, "t0"), ProtocolError);
    try{
        formulation.check_mass_balance(0, 1, "t0");
    }
    catch (ProtocolError& e) {
        std::cerr << e.to_string() << std::endl;
        EXPECT_THAT(e.to_string(), MatchesRegex("Error\\(Protocol\\)::mass_balance:.*"));
    }
}

TEST_F(Bmi_C_Formulation_Test, check_mass_balance_tolerance_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    auto ptree = config_prop_ptree[ex_index];
    ptree.put_child("mass_balance", MassBalanceMock(true, 1e-5).get());
    formulation.create_formulation(ptree);
    //formulation.check_mass_balance(0, 1, "t0");
    formulation.get_response(1, 3600);
    double mass_error;
    get_friend_bmi_model(formulation)->GetValue(INPUT_MASS_NAME, &mass_error);
    mass_error += 1e-6; // Force a mass balance error within tolerance
    get_friend_bmi_model(formulation)->SetValue(INPUT_MASS_NAME, &mass_error); // Force a mass balance error
    formulation.check_mass_balance(0, 1, "t0"); // Should not throw an error
}

TEST_F(Bmi_C_Formulation_Test, check_mass_balance_off) {
    int ex_index = 1;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    formulation.create_formulation(config_prop_ptree[ex_index]);
    formulation.check_mass_balance(0, 2, "t0");
    formulation.get_response(1, 3600);
    formulation.check_mass_balance(1, 2, "t1");
}

TEST_F(Bmi_C_Formulation_Test, check_mass_balance_missing) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    auto ptree = config_prop_ptree[ex_index];
 
    std::string catchment_path = "catchments." + catchment_ids[ex_index] + ".bmi_c";
    ptree.erase("mass_balance");
    formulation.create_formulation(ptree);
    formulation.check_mass_balance(0, 2, "t0");
    formulation.get_response(1, 3600);
    double mass_error = 100;
    double more_mass_error = 99;
    get_friend_bmi_model(formulation)->SetValue(OUTPUT_MASS_NAME, &mass_error); // Force a mass balance error
    get_friend_bmi_model(formulation)->SetValue(INPUT_MASS_NAME, &more_mass_error); // Force a mass balance error
    formulation.check_mass_balance(1, 2, "t1");

}

TEST_F(Bmi_C_Formulation_Test, check_mass_balance_frequency) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    auto ptree = config_prop_ptree[ex_index];
    ptree.put_child("mass_balance", MassBalanceMock(true, 1e-5, 2).get());
    formulation.create_formulation(ptree);
    double mass_error;
    mass_error += 10; // Force a mass balance error above tolerance
    get_friend_bmi_model(formulation)->SetValue(OUTPUT_MASS_NAME, &mass_error); //
    // Context contract: current_time_step is drawn from [0, total_steps - 1].
    // For a 3-step run with frequency=2, indices 0 and 2 fire (0 % 2 == 0,
    // 2 % 2 == 0) and index 1 is skipped.
    //Check initial mass balance -- should error which indicates it was properly checked
    //per frequency setting
    ASSERT_THROW(formulation.check_mass_balance(0, 3, "t0"), ProtocolError);
    // Call mass balance check again, this should NOT error, since the actual check
    // should be skipped due to the frequency setting
    formulation.check_mass_balance(1, 3, "t1");
    // Check mass balance again, this SHOULD error since the previous mass balance
    // will propagate, and it should now be checked based on the frequency
    ASSERT_THROW(formulation.check_mass_balance(2, 3, "t2"), ProtocolError);
}

TEST_F(Bmi_C_Formulation_Test, check_mass_balance_frequency_1) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]));
    auto ptree = config_prop_ptree[ex_index];
    ptree.put_child("mass_balance", MassBalanceMock(true, 1e-5, -1).get());
    formulation.create_formulation(ptree);
    double mass_error;
    mass_error += 10; // Force a mass balance error above tolerance
    get_friend_bmi_model(formulation)->SetValue(OUTPUT_MASS_NAME, &mass_error); //
    // Context contract: current_time_step is drawn from [0, total_steps - 1].
    // For a 3-step run, indices 0 and 1 must skip the check (frequency=-1
    // only fires at the end) and index 2 must fire, since
    // total_steps - 1 == 2 is the last in-range index.
    //Check initial mass balance -- should NOT error
    formulation.check_mass_balance(0, 3, "t0");
    // Call mass balance check again, this should NOT error, since the actual check
    // should be skipped due to the frequency setting
    formulation.check_mass_balance(1, 3, "t1");
    // Check mass balance again, this SHOULD error since this is the last
    // timestep (current_time_step == total_steps - 1) and the violation
    // should now be observable.
    ASSERT_THROW(formulation.check_mass_balance(2, 3, "t2"), ProtocolError);
}

/**
 * Restoring a state that carries the model clock must not shift forcing reads.
 *
 * test_bmi_c serializes its current time, and OUTPUT_VAR_2 echoes the hourly
 * temperature forcing, so a misaligned forcing read shows up directly in the
 * response. A first-half run (00:00 -> 12:00) saves after its last step; restored
 * runs are compared against an uninterrupted 00:00 -> 24:00 run at the same times.
 *
 * Only forcing alignment is checked. Whether the restored state belongs at the
 * new run's start time is a separate question these tests don't cover.
 */
class Bmi_C_Formulation_Restore_Clock_Test : public Bmi_C_Formulation_Test {
protected:
    static constexpr const char* DAY_START = "2015-12-01 00:00:00";
    static constexpr const char* MIDDAY    = "2015-12-01 12:00:00";
    static constexpr const char* DAY_END   = "2015-12-02 00:00:00";
    static constexpr realization::Formulation::time_step_t DT = 3600;

    void SetUp() override {
        Bmi_C_Formulation_Test::SetUp();
        checkpoint = ::testing::TempDir() + "bmi_c_restore_clock_" +
                     ::testing::UnitTest::GetInstance()->current_test_info()->name() + ".ckpt";
        std::remove(checkpoint.c_str());
    }

    void TearDown() override {
        std::remove(checkpoint.c_str());
        Bmi_C_Formulation_Test::TearDown();
    }

    static std::string format_time(time_t t) {
        std::tm tm{};
        gmtime_r(&t, &tm);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        return buf;
    }

    /** test_bmi_c over [start, end) with optional "serialization" and "model_params" blocks (JSON object text).
     *  model_params are applied via SetValue before the restore runs. */
    std::unique_ptr<Bmi_C_Formulation> make_formulation(const std::string& start, const std::string& end,
                                                        const std::string& serialization = "",
                                                        const std::string& model_params = "") {
        std::string json =
            "{"
            "  \"model_type_name\": \"test_bmi_c\","
            "  \"library_file\": \"" + lib_file[0] + "\","
            "  \"init_config\": \"" + init_config[0] + "\","
            "  \"main_output_variable\": \"OUTPUT_VAR_2\","
            "  \"registration_function\": \"register_bmi\","
            "  \"uses_forcing_file\": false,"
            "  \"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": {"
            "      \"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG "\","
            "      \"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "  }" +
            (serialization.empty() ? std::string() : ", \"serialization\": " + serialization) +
            (model_params.empty() ? std::string() : ", \"model_params\": " + model_params) +
            "}";
        std::stringstream stream(json);
        boost::property_tree::ptree tree;
        boost::property_tree::json_parser::read_json(stream, tree);
        auto provider = std::make_shared<CsvPerFeatureForcingProvider>(
            forcing_params(forcing_file[0], "legacy", start, end));
        auto formulation = std::make_unique<Bmi_C_Formulation>(catchment_ids[0], provider);
        formulation->create_formulation(tree);
        return formulation;
    }

    /** Responses of an uninterrupted DAY_START -> DAY_END run, keyed by step start time. */
    std::map<time_t, double> reference_run() {
        auto formulation = make_formulation(DAY_START, DAY_END);
        std::map<time_t, double> responses;
        const time_t start = parse_forcing_time(DAY_START);
        for (int i = 0; i < 24; ++i) {
            responses[start + i * DT] = formulation->get_response(i, DT);
        }
        return responses;
    }

    /** DAY_START -> MIDDAY, saving every `frequency` steps (-1: only after the last step).
     *  Records are stamped with each step's output time, as Layer does. Returns each step's response. */
    std::vector<double> save_first_half(int frequency = -1) {
        auto formulation = make_formulation(DAY_START, MIDDAY,
            "{\"path\": \"" + checkpoint + "\", \"save\": {\"frequency\": " + std::to_string(frequency) +
            ", \"fatal\": true}}");
        const time_t start = parse_forcing_time(DAY_START);
        std::vector<double> responses;
        for (int i = 0; i < 12; ++i) {
            responses.push_back(formulation->get_response(i, DT));
            formulation->checkpoint_state(i, 12, format_time(start + i * DT));
        }
        return responses;
    }

    /** Restore by "latest", or by an explicit timestamp when one is given. */
    std::string restore_block(bool fatal = true, const std::string& timestamp = "") const {
        const std::string select =
            timestamp.empty() ? "\"step\": \"latest\"" : "\"timestamp\": \"" + timestamp + "\"";
        return "{\"path\": \"" + checkpoint + "\", \"restore\": {" + select + ", \"fatal\": " +
               (fatal ? "true" : "false") + "}}";
    }

    /** Same conversion as Bmi_C_Formulation::convert_model_time, which is protected. */
    static time_t model_time_seconds(Bmi_C_Formulation& formulation, double model_time) {
        return (time_t) get_friend_bmi_model(formulation)->convert_model_time_to_seconds(model_time);
    }

    /** Forcing reads for the first step map to the run's start time. */
    void expect_forcing_anchored_to_start(Bmi_C_Formulation& formulation, const std::string& start) {
        EXPECT_EQ(model_time_seconds(formulation, formulation.get_model_current_time()) +
                      get_friend_bmi_model_start_time_forcing_offset_s(formulation),
                  parse_forcing_time(start));
    }

    /** Restore into [start, end) and compare every step against the reference run. */
    void expect_restored_run_matches_reference(const std::string& start, const std::string& end, int steps) {
        const auto reference = reference_run();
        save_first_half();

        auto restored = make_formulation(start, end, restore_block());
        auto model = get_friend_bmi_model(*restored);
        ASSERT_NE(model->GetCurrentTime(), model->GetStartTime())
            << "restore did not move the model clock, so this test would not exercise re-anchoring";
        expect_forcing_anchored_to_start(*restored, start);

        const time_t t0 = parse_forcing_time(start);
        for (int i = 0; i < steps; ++i) {
            EXPECT_DOUBLE_EQ(restored->get_response(i, DT), reference.at(t0 + i * DT))
                << "step " << i << " (" << format_time(t0 + i * DT) << ")";
        }
    }

    std::string checkpoint;
};

/** Resume where the saving run stopped (start == the restored state's time). */
TEST_F(Bmi_C_Formulation_Restore_Clock_Test, RestoredClock_StartAtSavePoint) {
    expect_restored_run_matches_reference(MIDDAY, DAY_END, 12);
}

/** Start before the restored state's time; forcings must follow the new run's clock. */
TEST_F(Bmi_C_Formulation_Restore_Clock_Test, RestoredClock_StartBeforeSavePoint) {
    expect_restored_run_matches_reference(DAY_START, MIDDAY, 12);
}

/** Start after the restored state's time; forcings must follow the new run's clock. */
TEST_F(Bmi_C_Formulation_Restore_Clock_Test, RestoredClock_StartAfterSavePoint) {
    expect_restored_run_matches_reference("2015-12-01 14:00:00", DAY_END, 10);
}

/** The documented resume recipe: start at the saving run's end time and restore its last output time. */
TEST_F(Bmi_C_Formulation_Restore_Clock_Test, ExplicitTimestamp_ResumeFromLastOutput) {
    const auto reference = reference_run();
    const double saved_output = save_first_half().back();

    // The saving run's last output row is 11:00 (end_time - output_interval).
    auto restored = make_formulation(MIDDAY, DAY_END, restore_block(/*fatal*/ true, "2015-12-01 11:00:00"));
    ASSERT_EQ(get_friend_var_value_as_double(*restored, "OUTPUT_VAR_2"), saved_output);
    expect_forcing_anchored_to_start(*restored, MIDDAY);

    const time_t t0 = parse_forcing_time(MIDDAY);
    for (int i = 0; i < 12; ++i) {
        EXPECT_DOUBLE_EQ(restored->get_response(i, DT), reference.at(t0 + i * DT))
            << "step " << i << " (" << format_time(t0 + i * DT) << ")";
    }
}

/** A record's timestamp is its step's output time: restoring T gives the state shown in output row T. */
TEST_F(Bmi_C_Formulation_Restore_Clock_Test, ExplicitTimestamp_SelectsStateFromThatOutputRow) {
    const auto responses = save_first_half(/*frequency*/ 1);
    ASSERT_NE(responses[4], responses[5]) << "adjacent steps must differ for this check to mean anything";

    // Output row 05:00 is the step at index 5; continuing from it starts at 06:00.
    const std::string six = "2015-12-01 06:00:00";
    auto restored = make_formulation(six, DAY_END, restore_block(/*fatal*/ true, "2015-12-01 05:00:00"));
    EXPECT_EQ(get_friend_var_value_as_double(*restored, "OUTPUT_VAR_2"), responses[5]);
    expect_forcing_anchored_to_start(*restored, six);
}

/** The consistency guarantee: with fatal, a formulation that has no record at the requested time stops
 *  the run at creation instead of starting unrestored. */
TEST_F(Bmi_C_Formulation_Restore_Clock_Test, ExplicitTimestamp_MissingRecordIsFatal) {
    save_first_half();
    EXPECT_THROW(make_formulation(MIDDAY, DAY_END, restore_block(/*fatal*/ true, "2015-12-01 13:00:00")),
                 ProtocolError);
}

/** Models that restore state but keep their own clock (`test::restore_clock` = 0, set before the restore). */
class Bmi_C_Formulation_Restore_Kept_Clock_Test : public Bmi_C_Formulation_Restore_Clock_Test {
protected:
    void expect_kept_clock_run_matches_reference(const std::string& start, const std::string& end, int steps) {
        const auto reference = reference_run();
        const double saved_output = save_first_half().back();

        auto restored = make_formulation(start, end, restore_block(), "{\"test::restore_clock\": 0}");
        auto model = get_friend_bmi_model(*restored);
        ASSERT_EQ(get_friend_var_value_as_double(*restored, "OUTPUT_VAR_2"), saved_output)
            << "state was not restored, so this test would not exercise a restore";
        ASSERT_EQ(model->GetCurrentTime(), model->GetStartTime()) << "model clock should not be restored";
        EXPECT_EQ(get_friend_bmi_model_start_time_forcing_offset_s(*restored),
                  parse_forcing_time(start) - model_time_seconds(*restored, model->GetStartTime()));
        expect_forcing_anchored_to_start(*restored, start);

        const time_t t0 = parse_forcing_time(start);
        for (int i = 0; i < steps; ++i) {
            EXPECT_DOUBLE_EQ(restored->get_response(i, DT), reference.at(t0 + i * DT))
                << "step " << i << " (" << format_time(t0 + i * DT) << ")";
        }
    }
};

/** Resume where the saving run stopped; the model keeps its own clock. */
TEST_F(Bmi_C_Formulation_Restore_Kept_Clock_Test, KeptClock_StartAtSavePoint) {
    expect_kept_clock_run_matches_reference(MIDDAY, DAY_END, 12);
}

/** Start away from the restored state's time; the model keeps its own clock. */
TEST_F(Bmi_C_Formulation_Restore_Kept_Clock_Test, KeptClock_StartAfterSavePoint) {
    expect_kept_clock_run_matches_reference("2015-12-01 14:00:00", DAY_END, 10);
}

/** A configured restore that finds no record leaves the clock and the forcing offset alone. */
TEST_F(Bmi_C_Formulation_Restore_Clock_Test, NothingRestored_OffsetUnchanged) {
    const auto reference = reference_run();

    auto formulation = make_formulation(MIDDAY, DAY_END, restore_block(/*fatal*/ false));
    auto model = get_friend_bmi_model(*formulation);
    ASSERT_EQ(model->GetCurrentTime(), model->GetStartTime());
    EXPECT_EQ(get_friend_bmi_model_start_time_forcing_offset_s(*formulation),
              parse_forcing_time(MIDDAY) - model_time_seconds(*formulation, model->GetStartTime()));
    expect_forcing_anchored_to_start(*formulation, MIDDAY);

    const time_t t0 = parse_forcing_time(MIDDAY);
    for (int i = 0; i < 12; ++i) {
        EXPECT_DOUBLE_EQ(formulation->get_response(i, DT), reference.at(t0 + i * DT))
            << "step " << i << " (" << format_time(t0 + i * DT) << ")";
    }
}

#endif  // NGEN_BMI_C_LIB_TESTS_ACTIVE

#endif // NGEN_BMI_C_FORMULATION_TEST_CPP
