#ifdef NGEN_BMI_C_LIB_TESTS_ACTIVE

#include "gtest/gtest.h"
#include <functional>
#include <iostream>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "Bmi_Formulation.hpp"
#include "Bmi_C_Formulation.hpp"
#include "Formulation_Manager.hpp"
#include "FileChecker.h"
#include "Forcing.h"

using namespace realization;

class Bmi_Formulation_Test : public ::testing::Test {

protected:
    static std::string find_file(std::vector<std::string> dir_opts, const std::string& basename);

};

class Bmi_C_Formulation_Test : public Bmi_Formulation_Test {
protected:

    void SetUp() override;

    void TearDown() override;

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

    /**
     * Helper function to compare formulation using the external, C-based, CFE model implementing BMI, to the internal
     * CFE implementation.
     *
     * @param example_index Index of the example configs to use during creation of the model and formulation objects.
     * @param value_name A string name/description of the particular value being compared, for use in output message.
     * @param num_time_steps The number of time steps for which models should be executed and have a value compared.
     * @param error_percentage An allowed margin of error between compared results, or ``0.0`` if strict equality required.
     * @param bmi_getter A function object taking the BMI formulation and a time step, returning the value to be compared.
     * @param internal_getter A function object taking the internal Tshirt formulation and time step, returning the value to be compared.
     */
    void compare_cfe_model_values(int example_index, const std::string &value_name, int num_time_steps,
                                  double error_percentage,
                                  const std::function<double(std::shared_ptr<Bmi_C_Formulation>, int)> &bmi_getter,
                                  const std::function<double(std::shared_ptr<Tshirt_C_Realization>,
                                                           int)> &internal_getter);

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

    std::shared_ptr<realization::Tshirt_C_Realization> init_internal_cfe(const std::string& catchment_id);

    std::vector<std::string> forcing_dir_opts;
    std::vector<std::string> bmi_init_cfg_dir_opts;

    std::vector<std::string> config_json;
    std::vector<std::string> catchment_ids;
    std::vector<std::string> model_type_name;
    std::vector<std::string> forcing_file;
    std::vector<std::string> init_config;
    std::vector<std::string> main_output_variable;
    std::vector<bool> uses_forcing_file;
    std::vector<std::shared_ptr<forcing_params>> forcing_params_examples;
    std::vector<geojson::GeoJSON> config_properties;
    std::vector<boost::property_tree::ptree> config_prop_ptree;

};

void Bmi_C_Formulation_Test::SetUp() {
    Test::SetUp();

#define EX_COUNT 2

    forcing_dir_opts = {"./data/forcing/", "../data/forcing/", "../../data/forcing/"};
    bmi_init_cfg_dir_opts = {"./test/data/bmi/c/cfe/", "../test/data/bmi/c/cfe/", "../../test/data/bmi/c/cfe/"};

    config_json = std::vector<std::string>(EX_COUNT);
    catchment_ids = std::vector<std::string>(EX_COUNT);
    model_type_name = std::vector<std::string>(EX_COUNT);
    forcing_file = std::vector<std::string>(EX_COUNT);
    init_config = std::vector<std::string>(EX_COUNT);
    main_output_variable  = std::vector<std::string>(EX_COUNT);
    uses_forcing_file = std::vector<bool>(EX_COUNT);
    forcing_params_examples = std::vector<std::shared_ptr<forcing_params>>(EX_COUNT);
    config_properties = std::vector<geojson::GeoJSON>(EX_COUNT);
    config_prop_ptree = std::vector<boost::property_tree::ptree>(EX_COUNT);

    /* Set up the basic/explicit example index details in the arrays */
    catchment_ids[0] = "cat-87";
    model_type_name[0] = "bmi_c_cfe";
    forcing_file[0] = find_file(forcing_dir_opts, "cat-87_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
    init_config[0] = find_file(bmi_init_cfg_dir_opts, "cat_87_bmi_config.txt");
    main_output_variable[0] = "Q_OUT";
    uses_forcing_file[0] = true;

    catchment_ids[1] = "cat-87";
    model_type_name[1] = "bmi_c_cfe";
    forcing_file[1] = find_file(forcing_dir_opts, "cat-87_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
    init_config[1] = find_file(bmi_init_cfg_dir_opts, "cat_87_bmi_config.txt");
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
                forcing_params(forcing_file[i], "2015-12-01 00:00:00", "2015-12-01 23:00:00"));
        std::string variables_line = (i == 1) ? variables_with_rain_rate : "";
        forcing_params_examples[i] = params;
        config_json[i] = "{"
                         "    \"global\": {},"
                         "    \"catchments\": {"
                         "        \"" + catchment_ids[i] + "\": {"
                         "            \"bmi_c\": {"
                         "                \"model_type_name\": \"" + model_type_name[i] + "\","
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

/**
 * Helper function to compare formulation using the external, C-based, CFE model implementing BMI, to the internal
 * CFE implementation.
 *
 * @param example_index Index of the example configs to use during creation of the model and formulation objects.
 * @param value_name A string name/description of the particular value being compared, for use in output message.
 * @param num_time_steps The number of time steps for which models should be executed and have a value compared.
 * @param error_percentage An allowed margin of error between compared results, or ``0.0`` if strict equality required.
 * @param bmi_getter A function object taking the BMI formulation and a time step, returning the value to be compared.
 * @param internal_getter A function object taking the internal Tshirt formulation and time step, returning the value to be compared.
 */
void Bmi_C_Formulation_Test::compare_cfe_model_values(int example_index, const std::string& value_name,
                                                      int num_time_steps, double error_percentage,
                                                      const std::function<double(std::shared_ptr<Bmi_C_Formulation>,
                                                                                     int)> &bmi_getter,
                                                      const std::function<double(
                                                                  std::shared_ptr<Tshirt_C_Realization>,
                                                                  int)> &internal_getter) {
#ifdef NGEN_BMI_C_COMPARE_CFE_TESTS
    Bmi_C_Formulation bmi_obj(catchment_ids[example_index], *forcing_params_examples[example_index], utils::StreamHandler());
    std::shared_ptr<Bmi_C_Formulation> bmi_cfe = std::make_shared<Bmi_C_Formulation>(bmi_obj);
    bmi_cfe->create_formulation(config_prop_ptree[example_index]);

    std::shared_ptr<realization::Tshirt_C_Realization> internal_cfe = init_internal_cfe(catchment_ids[example_index]);

    int negative_e_pow = -6;
    double absolute_smallest_max_error = 1.0 * pow(10.0, negative_e_pow);

    std::string print_str = "Time Step %d: %s values for BMI (%f) and internal (%f) CFE models differ";
    if (error_percentage != 0.0) {
        print_str += "\n    by more than than the larger of %2.1f%% or 1.0e" + std::to_string(negative_e_pow);
    }
    print_str += " (diff: %.*e).\n";

    double allowed_error, diff, result_bmi, result_internal;

    for (int i = 0; i < num_time_steps; ++i) {
        result_bmi = bmi_getter(bmi_cfe, i);
        result_internal = internal_getter(internal_cfe, i);

        if (error_percentage == 0.0) {
            EXPECT_EQ(result_bmi, result_internal)
                                << printf(print_str.c_str(), i, value_name.c_str(), result_bmi, result_internal, diff);
        }
        else {
            diff = abs(result_bmi - result_internal);
            allowed_error = max(result_bmi, result_internal) * error_percentage;
            // Set a hard threshold also, as when values are very small we won't care about large percentages either
            allowed_error = max(allowed_error, absolute_smallest_max_error);
            EXPECT_LE(diff, allowed_error)
                                << printf(print_str.c_str(), i, value_name.c_str(), result_bmi, result_internal,
                                          error_percentage * 100, diff);
        }
    }
#else
    ASSERT_TRUE(false) << printf("Need to configure build properly for executing BMI CFE comparison tests before calling.");
#endif // NGEN_BMI_C_COMPARE_CFE_TESTS
}

std::string Bmi_Formulation_Test::find_file(std::vector<std::string> dir_opts, const std::string& basename) {
    std::vector<std::string> file_opts(dir_opts.size());
    for (int i = 0; i < dir_opts.size(); ++i)
        file_opts[i] = dir_opts[i] + basename;
    return utils::FileChecker::find_first_readable(file_opts);
}

void Bmi_C_Formulation_Test::TearDown() {
    Test::TearDown();
}

std::shared_ptr<realization::Tshirt_C_Realization>
Bmi_C_Formulation_Test::init_internal_cfe(const std::string &catchment_id)
{
    std::shared_ptr<realization::Tshirt_C_Realization> cfe_ptr = nullptr;

#ifdef NGEN_BMI_C_COMPARE_CFE_TESTS
    // TODO: consider converting to reading values from the initialization file, or otherwise keeping params in sync
    if (catchment_id != "cat-87") {
        throw std::invalid_argument("Cannot init internal CFE for catchment " + catchment_id);
    }
    // Values copied from BMI C version's init file.
    // Logic taken from internal C-based version.
    double multiplier = 1000.0;
    double satdk = 0.00000338;
    // This first part is Equation 10 in parameter equivalence document.
    // the 0.01 value is assumed_near_channel_water_table_slope
    // the 3.5 value is drainage_density_km_per_km2; this is approx. average blue line drainage density for CONUS
    double K_lf = (2.0 * 0.01 * multiplier * satdk * 2.0 * 3.5) * 3600.0;

    tshirt::tshirt_params params{0.439, 0.066, satdk, 0.355, 1.0, 4.05, multiplier, 0.33, K_lf, 0.03, 2, 0.01, 6.0,
                                 16.0};
    std::vector<double> giuh_ordinates{0.06, 0.51, 0.28, 0.12, 0.03};
    std::vector<double> nash_storage(params.nash_n);
    for (int i = 0; i < params.nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    realization::Tshirt_C_Realization internal_cfe(
            *forcing_params_examples[0],
            utils::StreamHandler(),
            0.667,
            0.5,
            true,
            "wat-87",
            giuh_ordinates,
            params,
            nash_storage);
    cfe_ptr = std::make_shared<realization::Tshirt_C_Realization>(internal_cfe);
#endif // NGEN_BMI_C_COMPARE_CFE_TESTS

    return cfe_ptr;
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

/** Simple test of get response. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    // TODO: val seems to be this for now ... do something but account for error bound
    ASSERT_EQ(response, 0.19458118700224783);
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
    // TODO: val seems to be this for now ... do something but account for error bound
    ASSERT_EQ(response, 0.0019809221085605004);
}

/** Simple test of output. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    std::string output = formulation.get_output_line_for_timestep(0, ",");
    ASSERT_EQ(output, "0.000000,0.000000,0.000000,0.000000,0.194581,0.194581");
}

/** Simple test of output with modified variables. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_1_a) {
    int ex_index = 1;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    std::string output = formulation.get_output_line_for_timestep(0, ",");
    ASSERT_EQ(output, "0.000000,0.000000,0.000000,0.000000,0.194581,0.194581");
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
    ASSERT_EQ(output, "0.003153,0.004346,0.000449,0.001001,0.000070,0.001520");
}

#ifdef NGEN_BMI_C_COMPARE_CFE_TESTS
/** Compare results of BMI CFE and internal C-based version. */
TEST_F(Bmi_C_Formulation_Test, Compare_CFEs_0_a) {
    // Used to select the example config from what the testing Setup() function sets up.
    int ex_index = 0;
    // Can either compare within a margin of error, or set to 0 to require strict equivalence.
#ifdef NGEN_BMI_C_CFE_COMPARE_STRICT
    double error_margin = 0.0;
#else
    double error_margin = 0.001;
#endif  // NGEN_BMI_C_CFE_COMPARE_STRICT

    std::function<double(std::shared_ptr<Bmi_C_Formulation>, int)> bmi_getter = [] (
            const std::shared_ptr<Bmi_C_Formulation>& bmi_cfe, int time_step)
    {
        return bmi_cfe->get_response(time_step, 3600);
    };

    std::function<double(std::shared_ptr<Tshirt_C_Realization>, int)> internal_getter = [] (
            const std::shared_ptr<Tshirt_C_Realization>& internal_cfe, int time_step)
    {
        return internal_cfe->get_response(time_step, 3600);
    };

    compare_cfe_model_values(ex_index, "Response Output", 700, error_margin, bmi_getter, internal_getter);

}

/** Compare Schaake Runoff of BMI CFE and internal C-based version. */
TEST_F(Bmi_C_Formulation_Test, Compare_CFEs_0_b) {
    // Used to select the example config from what the testing Setup() function sets up.
    int ex_index = 0;
    // Can either compare within a margin of error, or set to 0 to require strict equivalence.
#ifdef NGEN_BMI_C_CFE_COMPARE_STRICT
    double error_margin = 0.0;
#else
    double error_margin = 0.001;
#endif  // NGEN_BMI_C_CFE_COMPARE_STRICT
    // Need this for parsing, since BMI only can (easily) access other values by printing them.
    std::string delim = ",";

    /* Output variables for BMI cfe are:
     *
     * (0) "SCHAAKE_OUTPUT_RUNOFF",
     * (1) "GIUH_RUNOFF",
     * (2) "NASH_LATERAL_RUNOFF",
     * (3) "DEEP_GW_TO_CHANNEL_FLUX",
     * (4) "Q_OUT"
     */
    int csv_output_index = 0;

    std::function<double(std::shared_ptr<Bmi_C_Formulation>, int)> bmi_getter = [this, delim, csv_output_index](
            const std::shared_ptr<Bmi_C_Formulation> &bmi_cfe, int time_step)
    {
        bmi_cfe->get_response(time_step, 3600);
        return this->parse_from_delimited_string(bmi_cfe->get_output_line_for_timestep(time_step, delim), delim,
                                                 csv_output_index);
    };

    std::function<double(std::shared_ptr<Tshirt_C_Realization>, int)> internal_getter = [](
            const std::shared_ptr<Tshirt_C_Realization> &internal_cfe, int time_step)
    {
        internal_cfe->get_response(time_step, 3600);
        return internal_cfe->get_latest_flux_surface_runoff();
    };

    compare_cfe_model_values(ex_index, "Schaake Runoff", 700, error_margin, bmi_getter, internal_getter);
}

/** Compare GIUH of BMI CFE and internal C-based version. */
TEST_F(Bmi_C_Formulation_Test, Compare_CFEs_0_c) {
    // Used to select the example config from what the testing Setup() function sets up.
    int ex_index = 0;
    // Can either compare within a margin of error, or set to 0 to require strict equivalence.
#ifdef NGEN_BMI_C_CFE_COMPARE_STRICT
    double error_margin = 0.0;
#else
    double error_margin = 0.001;
#endif  // NGEN_BMI_C_CFE_COMPARE_STRICT
    // Need this for parsing, since BMI only can (easily) access other values by printing them.
    std::string delim = ",";

    /* Output variables for BMI cfe are:
     *
     * (0) "SCHAAKE_OUTPUT_RUNOFF",
     * (1) "GIUH_RUNOFF",
     * (2) "NASH_LATERAL_RUNOFF",
     * (3) "DEEP_GW_TO_CHANNEL_FLUX",
     * (4) "Q_OUT"
     */
    int csv_output_index = 1;

    std::function<double(std::shared_ptr<Bmi_C_Formulation>, int)> bmi_getter = [this, delim, csv_output_index](
            const std::shared_ptr<Bmi_C_Formulation> &bmi_cfe, int time_step)
    {
        bmi_cfe->get_response(time_step, 3600);
        return this->parse_from_delimited_string(bmi_cfe->get_output_line_for_timestep(time_step, delim), delim,
                                                 csv_output_index);
    };

    std::function<double(std::shared_ptr<Tshirt_C_Realization>, int)> internal_getter = [](
            const std::shared_ptr<Tshirt_C_Realization> &internal_cfe, int time_step)
    {
        internal_cfe->get_response(time_step, 3600);
        return internal_cfe->get_latest_flux_giuh_runoff();
    };

    compare_cfe_model_values(ex_index, "GIUH Runoff", 700, error_margin, bmi_getter, internal_getter);
}

/** Compare Nash Lateral Flow Runoff of BMI CFE and internal C-based version. */
TEST_F(Bmi_C_Formulation_Test, Compare_CFEs_0_d) {
    // Used to select the example config from what the testing Setup() function sets up.
    int ex_index = 0;
    // Can either compare within a margin of error, or set to 0 to require strict equivalence.
#ifdef NGEN_BMI_C_CFE_COMPARE_STRICT
    double error_margin = 0.0;
#else
    double error_margin = 0.001;
#endif  // NGEN_BMI_C_CFE_COMPARE_STRICT
    // Need this for parsing, since BMI only can (easily) access other values by printing them.
    std::string delim = ",";

    /* Output variables for BMI cfe are:
     *
     * "SCHAAKE_OUTPUT_RUNOFF",
     * "GIUH_RUNOFF",
     * "NASH_LATERAL_RUNOFF",
     * "DEEP_GW_TO_CHANNEL_FLUX",
     * "Q_OUT"
     */
    int csv_output_index = 2;

    std::function<double(std::shared_ptr<Bmi_C_Formulation>, int)> bmi_getter = [this, delim, csv_output_index] (
            const std::shared_ptr<Bmi_C_Formulation>& bmi_cfe, int time_step)
    {
        bmi_cfe->get_response(time_step, 3600);
        return get_friend_var_value_as_double(*bmi_cfe, "NASH_LATERAL_RUNOFF");
    };

    std::function<double(std::shared_ptr<Tshirt_C_Realization>, int)> internal_getter = [](
            const std::shared_ptr<Tshirt_C_Realization> &internal_cfe, int time_step)
    {
        internal_cfe->get_response(time_step, 3600);
        return internal_cfe->get_latest_flux_lateral_flow();
    };

    compare_cfe_model_values(ex_index, "Lateral Flow Runoff", 700, error_margin, bmi_getter, internal_getter);
}

/** Compare Deep GW to Channel Flux of BMI CFE and internal C-based version. */
TEST_F(Bmi_C_Formulation_Test, Compare_CFEs_0_e) {
    // Used to select the example config from what the testing Setup() function sets up.
    int ex_index = 0;
    // Can either compare within a margin of error, or set to 0 to require strict equivalence.
#ifdef NGEN_BMI_C_CFE_COMPARE_STRICT
    double error_margin = 0.0;
#else
    double error_margin = 0.001;
#endif  // NGEN_BMI_C_CFE_COMPARE_STRICT
    // Need this for parsing, since BMI only can (easily) access other values by printing them.
    std::string delim = ",";

    /* Output variables for BMI cfe are:
     *
     * "SCHAAKE_OUTPUT_RUNOFF",
     * "GIUH_RUNOFF",
     * "NASH_LATERAL_RUNOFF",
     * "DEEP_GW_TO_CHANNEL_FLUX",
     * "Q_OUT"
     */
    int csv_output_index = 3;

    std::function<double(std::shared_ptr<Bmi_C_Formulation>, int)> bmi_getter = [this, delim, csv_output_index] (
            const std::shared_ptr<Bmi_C_Formulation>& bmi_cfe, int time_step)
    {
        bmi_cfe->get_response(time_step, 3600);
        return get_friend_var_value_as_double(*bmi_cfe, "DEEP_GW_TO_CHANNEL_FLUX");    };

    std::function<double(std::shared_ptr<Tshirt_C_Realization>, int)> internal_getter = [](
            const std::shared_ptr<Tshirt_C_Realization> &internal_cfe, int time_step)
    {
        internal_cfe->get_response(time_step, 3600);
        return internal_cfe->get_latest_flux_base_flow();
    };

    compare_cfe_model_values(ex_index, "Deep GW to Channel Flux", 700, error_margin, bmi_getter, internal_getter);
}

#endif  // NGEN_BMI_C_COMPARE_CFE_TESTS

#endif  // NGEN_BMI_C_LIB_TESTS_ACTIVE
