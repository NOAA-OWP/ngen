#ifndef NGEN_BMI_C_FORMULATION_TEST_CPP
#define NGEN_BMI_C_FORMULATION_TEST_CPP

#ifdef NGEN_BMI_C_LIB_TESTS_ACTIVE

#ifndef BMI_CFE_LOCAL_LIB_NAME
#ifdef __APPLE__
    #define BMI_CFE_LOCAL_LIB_NAME "libcfemodel.dylib"
#else
#ifdef __GNUC__
    #define BMI_CFE_LOCAL_LIB_NAME "libcfemodel.so"
    #endif // __GNUC__
#endif // __APPLE__
#endif // BMI_CFE_LOCAL_LIB_NAME

#include "Bmi_Formulation.hpp"
#include "Bmi_C_Formulation.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "FileChecker.h"
#include "Formulation_Manager.hpp"
#include "Forcing.h"
#include <boost/date_time.hpp>

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

    static void call_friend_get_forcing_data_ts_contributions(Bmi_C_Formulation& formulation,
                                                              time_step_t t_delta, const double &model_initial_time,
                                                              const std::vector<std::string> &params,
                                                              const std::vector<std::string> &param_units,
                                                              std::vector<double> &contributions)
    {
        formulation.get_forcing_data_ts_contributions(t_delta, model_initial_time, params, param_units, contributions);
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

    static time_t get_friend_bmi_model_start_time_forcing_offset_s(const Bmi_C_Formulation& formulation) {
        return formulation.get_bmi_model_start_time_forcing_offset_s();
    }

    static double get_friend_forcing_param_value(Bmi_C_Formulation& formulation, const std::string& param_name) {
        return formulation.forcing.get_value_for_param_name(param_name);
    }

    static std::string get_friend_forcing_file_path(const Bmi_C_Formulation& formulation) {
        return formulation.get_forcing_file_path();
    }

    static time_t get_friend_forcing_start_time(Bmi_C_Formulation& formulation) {
        return formulation.forcing.get_time_epoch();
    }

    static time_t get_friend_forcing_time_step_size(Bmi_C_Formulation& formulation) {
        return formulation.forcing.get_time_step_size();
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
    std::vector<bool> uses_forcing_file;
    std::vector<std::shared_ptr<forcing_params>> forcing_params_examples;
    std::vector<geojson::GeoJSON> config_properties;
    std::vector<boost::property_tree::ptree> config_prop_ptree;

};

void Bmi_C_Formulation_Test::SetUp() {
    testing::Test::SetUp();

#define EX_COUNT 2

    forcing_dir_opts = {"./data/forcing/", "../data/forcing/", "../../data/forcing/"};
    bmi_init_cfg_dir_opts = {"./test/data/bmi/c/cfe/", "../test/data/bmi/c/cfe/", "../../test/data/bmi/c/cfe/"};
    lib_dir_opts = {"./extern/cfe/cmake_cfe_lib/", "../extern/cfe/cmake_cfe_lib/", "../../extern/cfe/cmake_cfe_lib/"};

    config_json = std::vector<std::string>(EX_COUNT);
    catchment_ids = std::vector<std::string>(EX_COUNT);
    model_type_name = std::vector<std::string>(EX_COUNT);
    forcing_file = std::vector<std::string>(EX_COUNT);
    lib_file = std::vector<std::string>(EX_COUNT);
    init_config = std::vector<std::string>(EX_COUNT);
    main_output_variable  = std::vector<std::string>(EX_COUNT);
    uses_forcing_file = std::vector<bool>(EX_COUNT);
    forcing_params_examples = std::vector<std::shared_ptr<forcing_params>>(EX_COUNT);
    config_properties = std::vector<geojson::GeoJSON>(EX_COUNT);
    config_prop_ptree = std::vector<boost::property_tree::ptree>(EX_COUNT);

    /* Set up the basic/explicit example index details in the arrays */
    catchment_ids[0] = "cat-27";
    model_type_name[0] = "bmi_c_cfe";
    forcing_file[0] = find_file(forcing_dir_opts, "cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
    lib_file[0] = find_file(lib_dir_opts, BMI_CFE_LOCAL_LIB_NAME);
    init_config[0] = find_file(bmi_init_cfg_dir_opts, "cat_27_bmi_config.txt");
    main_output_variable[0] = "Q_OUT";
    uses_forcing_file[0] = true;

    catchment_ids[1] = "cat-27";
    model_type_name[1] = "bmi_c_cfe";
    forcing_file[1] = find_file(forcing_dir_opts, "cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
    lib_file[1] = find_file(lib_dir_opts, BMI_CFE_LOCAL_LIB_NAME);
    init_config[1] = find_file(bmi_init_cfg_dir_opts, "cat_27_bmi_config.txt");
    main_output_variable[1] = "Q_OUT";
    uses_forcing_file[1] = true;

    std::string variables_with_rain_rate = "                \"output_variables\": [\"RAIN_RATE\",\n"
                                           "                    \"SCHAAKE_OUTPUT_RUNOFF\",\n"
                                           "                    \"GIUH_RUNOFF\",\n"
                                           "                    \"NASH_LATERAL_RUNOFF\",\n"
                                           "                    \"DEEP_GW_TO_CHANNEL_FLUX\",\n"
                                           "                    \"Q_OUT\"],\n";

    /* Set up the derived example details */
    for (int i = 0; i < EX_COUNT; i++) {
        std::shared_ptr<forcing_params> params = std::make_shared<forcing_params>(
                forcing_params(forcing_file[i], "2015-12-01 00:00:00", "2015-12-30 23:00:00"));
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
                         + variables_line +
                         "                \"uses_forcing_file\": " + (uses_forcing_file[i] ? "true" : "false") + ""
                         "            },"
                         "            \"forcing\": { \"path\": \"" + forcing_file[i] + "\"}"
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

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_model_type_name(formulation), model_type_name[ex_index]);
    ASSERT_EQ(get_friend_forcing_file_path(formulation), forcing_file[ex_index]);
    ASSERT_EQ(get_friend_bmi_init_config(formulation), init_config[ex_index]);
    ASSERT_EQ(get_friend_bmi_main_output_var(formulation), main_output_variable[ex_index]);
    ASSERT_EQ(get_friend_is_bmi_using_forcing_file(formulation), uses_forcing_file[ex_index]);
}

/** Test to make sure we can initialize multiple model instances with dynamic loading. */
TEST_F(Bmi_C_Formulation_Test, Initialize_1_a) {
    Bmi_C_Formulation form_1(catchment_ids[0], *forcing_params_examples[0], utils::StreamHandler());
    form_1.create_formulation(config_prop_ptree[0]);

    Bmi_C_Formulation form_2(catchment_ids[1], *forcing_params_examples[1], utils::StreamHandler());
    form_2.create_formulation(config_prop_ptree[1]);

    std::string header_1 = form_1.get_output_header_line(",");
    std::string header_2 = form_2.get_output_header_line(",");

    ASSERT_EQ(header_1, header_2);
}

/** Simple test of get response. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    // TODO: val seems to be this for now ... do something but account for error bound
    ASSERT_EQ(response, 0.19108623197892585);
}

/** Test to make sure we can execute multiple model instances with dynamic loading. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_1_a) {
    Bmi_C_Formulation form_1(catchment_ids[0], *forcing_params_examples[0], utils::StreamHandler());
    form_1.create_formulation(config_prop_ptree[0]);

    Bmi_C_Formulation form_2(catchment_ids[1], *forcing_params_examples[1], utils::StreamHandler());
    form_2.create_formulation(config_prop_ptree[1]);

    // Do these out of order
    double response_1_step_0 = form_1.get_response(0, 3600);
    double response_2_step_0 = form_2.get_response(0, 3600);
    double response_2_step_1 = form_2.get_response(0, 3600);
    ASSERT_EQ(response_1_step_0, response_2_step_0);
    double response_1_step_1 = form_1.get_response(0, 3600);
    ASSERT_EQ(response_1_step_1, response_2_step_1);
}

/** Test of get response after several iterations. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_0_b) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response;
    for (int i = 0; i < 720; i++) {
        response = formulation.get_response(i, 3600);
    }
    ASSERT_EQ(response, 0.0016562122305483094);
}

/** Simple test of output. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    std::string output = formulation.get_output_line_for_timestep(0, ",");
    ASSERT_EQ(output, "0.000000,0.000000,0.000000,0.000000,0.191086,0.191086");
}

/** Simple test of output with modified variables. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_1_a) {
    int ex_index = 1;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    std::string output = formulation.get_output_line_for_timestep(0, ",");
    ASSERT_EQ(output, "0.000000,0.000000,0.000000,0.000000,0.191086,0.191086");
}

/** Simple test of output with modified variables, picking time step when there was non-zero rain rate. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_1_b) {
    int ex_index = 1;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    int i = 0;
    while (i < 542)
        formulation.get_response(i++, 3600);
    double response = formulation.get_response(i, 3600);
    std::string output = formulation.get_output_line_for_timestep(i, ",");
    ASSERT_EQ(output, "0.007032,0.000634,0.000103,0.000294,0.001612,0.002009");
}

TEST_F(Bmi_C_Formulation_Test, determine_model_time_offset_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);

    double model_start = model_adapter->GetStartTime();
    ASSERT_EQ(model_start, 0.0);
}

TEST_F(Bmi_C_Formulation_Test, determine_model_time_offset_0_b) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);
    time_t forcing_start = get_friend_forcing_start_time(formulation);

    ASSERT_EQ(forcing_start,  parse_forcing_time("2015-12-01 00:00:00"));
}

TEST_F(Bmi_C_Formulation_Test, determine_model_time_offset_0_c) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);

    call_friend_determine_model_time_offset(formulation);

    // Note that this depends on the particular value for forcing time implied by determine_model_time_offset_0_b above
    // it also assumes the model time starts at 0
    auto expected_offset = (time_t) 1448928000;

    ASSERT_EQ(get_friend_bmi_model_start_time_forcing_offset_s(formulation), expected_offset);
}

/** Simple test for contribution when forcing and model time steps align. */
TEST_F(Bmi_C_Formulation_Test, get_forcing_data_ts_contributions_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);

    std::string param_name = "precip_rate";

    double forcing_ts_param_value = get_friend_forcing_param_value(formulation, param_name);

    double model_time = model_adapter->GetCurrentTime();
    ASSERT_EQ(model_time, 0.0);

    std::vector<std::string> param_names = {param_name};
    std::vector<std::string> param_units = {"m"};
    std::vector<double> summed_contributions = {0.0};

    ASSERT_EQ(get_friend_forcing_time_step_size(formulation), (time_t)3600);
    time_step_t t_delta = 3600;

    call_friend_get_forcing_data_ts_contributions(formulation, t_delta, model_time, param_names, param_units,
                                                  summed_contributions);
    ASSERT_EQ(summed_contributions[0], forcing_ts_param_value);
}

/** Simple test for contribution when forcing and model time steps align, skipping to time step with non-zero value. */
TEST_F(Bmi_C_Formulation_Test, get_forcing_data_ts_contributions_0_b) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);

    std::string output_line;
    int progressed_seconds = 0;

    // Skip ahead in time.
    for (int i = 0; i <= 37; ++i) {
        formulation.get_response(i, 3600);
        progressed_seconds += 3600;
    }

    std::string param_name = "precip_rate";

    double forcing_ts_param_value = get_friend_forcing_param_value(formulation, param_name);
    ASSERT_GT(forcing_ts_param_value, 0.0);

    double model_time = model_adapter->GetCurrentTime();
    ASSERT_GT(model_time, 0.0);
    ASSERT_EQ(progressed_seconds, model_adapter->convert_model_time_to_seconds(model_time));

    std::vector<std::string> param_names = {param_name};
    std::vector<std::string> param_units = {"m"};
    std::vector<double> summed_contributions = {0.0};

    ASSERT_EQ(get_friend_forcing_time_step_size(formulation), (time_t)3600);
    time_step_t t_delta = 3600;

    call_friend_get_forcing_data_ts_contributions(formulation, t_delta, model_time, param_names, param_units,
                                                  summed_contributions);
    ASSERT_EQ(summed_contributions[0], forcing_ts_param_value);
}

/**
 * Simple test for contribution when forcing and model time steps do not align, skipping to time step with non-zero
 * value.
 */
TEST_F(Bmi_C_Formulation_Test, get_forcing_data_ts_contributions_1_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);

    std::string output_line;
    int progressed_seconds = 0;

    // Skip ahead in time.
    int i;
    for (i = 0; i <= 37; ++i) {
        formulation.get_response(i, 3600);
        progressed_seconds += 3600;
    }

    std::string param_name = "precip_rate";

    double forcing_ts_param_value= get_friend_forcing_param_value(formulation, param_name);
    ASSERT_GT(forcing_ts_param_value, 0.0);

    double model_time = model_adapter->GetCurrentTime();
    ASSERT_GT(model_time, 0.0);
    ASSERT_EQ(progressed_seconds, model_adapter->convert_model_time_to_seconds(model_time));

    std::vector<std::string> param_names = {param_name};
    std::vector<std::string> param_units = {"m"};
    std::vector<double> summed_contributions = {0.0};

    ASSERT_EQ(get_friend_forcing_time_step_size(formulation), (time_t)3600);
    time_step_t t_delta = 1800;

    call_friend_get_forcing_data_ts_contributions(formulation, t_delta, model_time, param_names, param_units,
                                                  summed_contributions);
    double forcing_ts_param_value_2 = get_friend_forcing_param_value(formulation, param_name);

    ASSERT_EQ(summed_contributions[0], forcing_ts_param_value / 2.0);
}

/**
 * Simple test for contribution when forcing and model time steps do not align, skipping to time step with non-zero
 * value, and spanning data from multiple forcing time steps.
 */
TEST_F(Bmi_C_Formulation_Test, get_forcing_data_ts_contributions_1_b) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    std::shared_ptr<models::bmi::Bmi_C_Adapter> model_adapter = get_friend_bmi_model(formulation);

    std::string output_line;
    int progressed_seconds = 0;

    // Skip ahead in time.
    int i;
    for (i = 0; i <= 37; ++i) {
        formulation.get_response(i, 3600);
        progressed_seconds += 3600;
    }

    std::string param_name = "precip_rate";

    double forcing_ts_param_value= get_friend_forcing_param_value(formulation, param_name);
    ASSERT_GT(forcing_ts_param_value, 0.0);

    double model_time = model_adapter->GetCurrentTime();
    ASSERT_GT(model_time, 0.0);
    ASSERT_EQ(progressed_seconds, model_adapter->convert_model_time_to_seconds(model_time));

    std::vector<std::string> param_names = {param_name};
    std::vector<std::string> param_units = {"m"};
    std::vector<double> summed_contributions = {0.0};

    ASSERT_EQ(get_friend_forcing_time_step_size(formulation), (time_t)3600);
    time_step_t t_delta = 3600 + 1800;

    call_friend_get_forcing_data_ts_contributions(formulation, t_delta, model_time, param_names, param_units,
                                                  summed_contributions);
    double forcing_ts_param_value_2 = get_friend_forcing_param_value(formulation, param_name);

    // Assert that these are actually values from two different forcing time steps.
    ASSERT_NE(forcing_ts_param_value, forcing_ts_param_value_2);
    // Assert that the tested function is actually getting contributions in the appropriate proportions from two
    // forcing time steps
    ASSERT_EQ(summed_contributions[0], forcing_ts_param_value + forcing_ts_param_value_2 / 2.0);
}


#endif  // NGEN_BMI_C_LIB_TESTS_ACTIVE

#endif // NGEN_BMI_C_FORMULATION_TEST_CPP