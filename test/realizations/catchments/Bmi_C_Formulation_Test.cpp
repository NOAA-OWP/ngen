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
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <iostream>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "FileChecker.h"
#include "Formulation_Manager.hpp"
#include <boost/date_time.hpp>

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
        return formulation.get_bmi_model();
    }

    static time_t get_friend_bmi_model_start_time_forcing_offset_s(Bmi_C_Formulation& formulation) {
        return formulation.get_bmi_model_start_time_forcing_offset_s();
    }

    static std::string get_friend_forcing_file_path(const Bmi_C_Formulation& formulation) {
        return formulation.get_forcing_file_path();
    }

    static time_t get_friend_forcing_start_time(Bmi_C_Formulation& formulation) {
        return formulation.forcing->get_data_start_time();
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

void Bmi_C_Formulation_Test::SetUp() {
    testing::Test::SetUp();

#define EX_COUNT 2

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
    registration_functions  = std::vector<std::string>(EX_COUNT);
    uses_forcing_file = std::vector<bool>(EX_COUNT);
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
    registration_functions[0] = "register_bmi";
    uses_forcing_file[0] = false;

    catchment_ids[1] = "cat-27";
    model_type_name[1] = "test_bmi_c";
    forcing_file[1] = find_file(forcing_dir_opts, "cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
    lib_file[1] = find_file(lib_dir_opts, BMI_TEST_C_LOCAL_LIB_NAME);
    init_config[1] = find_file(bmi_init_cfg_dir_opts, "test_bmi_c_config_1.txt");
    main_output_variable[1] = "OUTPUT_VAR_1";
    registration_functions[1] = "register_bmi";
    uses_forcing_file[1] = false;

    std::string variables_with_rain_rate = "                \"output_variables\": [\"OUTPUT_VAR_2\",\n"
                                           "                    \"OUTPUT_VAR_1\"],\n";

    /* Set up the derived example details */
    for (int i = 0; i < EX_COUNT; i++) {
        std::shared_ptr<forcing_params> params = std::make_shared<forcing_params>(
                forcing_params(forcing_file[i], "legacy", "2015-12-01 00:00:00", "2015-12-30 23:00:00"));
        std::string variables_line = (i == 1) ? variables_with_rain_rate : "";
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
                         "                \"" + BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION + "\": 6, "
                         "                \"" + BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES + "\": { "
                         "                      \"INPUT_VAR_2\": \"" + NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP + "\","
                         "                      \"INPUT_VAR_1\": \"" + AORC_FIELD_NAME_PRECIP_RATE + "\""
                         "                },"
                         "                \"registration_function\": \"" + registration_functions[i] + "\","
                         + variables_line +
                         "                \"uses_forcing_file\": " + (uses_forcing_file[i] ? "true" : "false") + ","
                         "                \"model_params\": { \"PARAM_VAR_1\": 42, \"PARAM_VAR_2\": 4.2, \"PARAM_VAR_3\": [4, 2]}" +
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

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_model_type_name(formulation), model_type_name[ex_index]);
    ASSERT_EQ(get_friend_forcing_file_path(formulation), forcing_file[ex_index]);
    ASSERT_EQ(get_friend_bmi_init_config(formulation), init_config[ex_index]);
    ASSERT_EQ(get_friend_bmi_main_output_var(formulation), main_output_variable[ex_index]);
    ASSERT_EQ(get_friend_is_bmi_using_forcing_file(formulation), uses_forcing_file[ex_index]);
}

/** Test to make sure we can initialize multiple model instances with dynamic loading. */
TEST_F(Bmi_C_Formulation_Test, Initialize_1_a) {
    Bmi_C_Formulation form_1(catchment_ids[0], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[0]), utils::StreamHandler());
    form_1.create_formulation(config_prop_ptree[0]);

    Bmi_C_Formulation form_2(catchment_ids[1], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[1]), utils::StreamHandler());
    form_2.create_formulation(config_prop_ptree[1]);

    std::string header_1 = form_1.get_output_header_line(",");
    std::string header_2 = form_2.get_output_header_line(",");

    ASSERT_EQ(header_1, "OUTPUT_VAR_1,OUTPUT_VAR_2");
    ASSERT_EQ(header_2, "OUTPUT_VAR_2,OUTPUT_VAR_1");
}

/** Simple test of get response. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    ASSERT_EQ(response, 00);
}

/** Simple test of initial parameter setting. */
TEST_F(Bmi_C_Formulation_Test, set_initial_parameters_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
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

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response;
    for (int i = 0; i < 39; i++) {
        response = formulation.get_response(i, 3600);
    }
    double expected = 2.7809780039160068e-08;
    ASSERT_EQ(expected, response);
}

/** Test to make sure we can execute multiple model instances with dynamic loading. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_1_a) {
    Bmi_C_Formulation form_1(catchment_ids[0], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[0]), utils::StreamHandler());
    form_1.create_formulation(config_prop_ptree[0]);

    Bmi_C_Formulation form_2(catchment_ids[1], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[1]), utils::StreamHandler());
    form_2.create_formulation(config_prop_ptree[1]);

    double response_1, response_2;
    ASSERT_NE(&response_1, &response_2);
    for (size_t i = 0; i < 720; ++i) {
        response_1 = form_1.get_response(0, 3600);
        response_2 = form_2.get_response(0, 3600);
        ASSERT_EQ(response_1, response_2);
    }
}

/** Simple test of output. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    formulation.get_response(0, 3600);
    std::string output = formulation.get_output_line_for_timestep(0, ",");
    EXPECT_THAT(output, MatchesRegex("0.000000,0.000000"));
}

/** Simple test of output with modified variables. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_1_a) {
    int ex_index = 1;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    // Notably--this test could fail if both output vars were not the same. get_output_line_for_timestep assumes
    // the return order of get_output_variable_names() is consistent but it apparently is not. In tracing this
    // test, it was actually outputing OUTPUT_VAR_2 first, while the other two comparable tests are outputting
    // OUTPUT_VAR_1 first.
    formulation.get_response(0, 3600);
    std::string output = formulation.get_output_line_for_timestep(0, ",");
    EXPECT_THAT(output, MatchesRegex("0.000000,0.000000"));
}

/** Simple test of output with modified variables, picking time step when there was non-zero rain rate. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_1_b) {
    int ex_index = 1;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    int i = 0;
    while (i < 542)
        formulation.get_response(i++, 3600);
    formulation.get_response(i, 3600);
    std::string output = formulation.get_output_line_for_timestep(i, ",");
    EXPECT_THAT(output, MatchesRegex("0.000000,0.000001"));
}

TEST_F(Bmi_C_Formulation_Test, determine_model_time_offset_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);

    double model_start = model_adapter->GetStartTime();
    ASSERT_EQ(model_start, 0.0);
}

TEST_F(Bmi_C_Formulation_Test, determine_model_time_offset_0_b) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);
    time_t forcing_start = get_friend_forcing_start_time(formulation);

    ASSERT_EQ(forcing_start,  parse_forcing_time("2015-12-01 00:00:00"));
}

TEST_F(Bmi_C_Formulation_Test, determine_model_time_offset_0_c) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], std::make_shared<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);

    call_friend_determine_model_time_offset(formulation);

    // Note that this depends on the particular value for forcing time implied by determine_model_time_offset_0_b above
    // it also assumes the model time starts at 0
    auto expected_offset = (time_t) 1448928000;

    ASSERT_EQ(get_friend_bmi_model_start_time_forcing_offset_s(formulation), expected_offset);
}

#endif  // NGEN_BMI_C_LIB_TESTS_ACTIVE

#endif // NGEN_BMI_C_FORMULATION_TEST_CPP
