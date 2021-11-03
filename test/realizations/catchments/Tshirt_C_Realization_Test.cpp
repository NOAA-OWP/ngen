#include "gtest/gtest.h"
#include "Tshirt_C_Realization.hpp"
#include "tshirt/include/tshirt_params.h"
#include "GIUH.hpp"
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <boost/tokenizer.hpp>
#include "FileChecker.h"

class Tshirt_C_Realization_Test : public ::testing::Test {

protected:

    typedef boost::tokenizer<boost::escaped_list_separator<char>> Tokenizer;

    Tshirt_C_Realization_Test() {
        error_bound_percent = 0.01;
        error_upper_bound_min = 0.001;
        upper_bound_factor = 1.0;
        lower_bound_factor = 1.0;

        is_giuh_ordinate_examples = false;
        is_forcing_params_examples = false;
    }

    ~Tshirt_C_Realization_Test() override {

    }

    void SetUp() override;

    void TearDown() override;

    void open_standalone_c_impl_data_stream();

    void setup_standalone_c_impl_example_case();

    /** The name of the data file for data generated from the standalone C-based implementation. */
    std::string standalone_output_data_file;
    /** The input stream of the data file for data generated from the standalone C-based implementation. */
    std::ifstream standalone_data_ingest_stream;
    /** Whether there was an input stream opened or not for the current test instance. */
    bool is_standalone_data_ingest_stream;
    std::shared_ptr<tshirt::tshirt_params> c_impl_ex_tshirt_params;

    // TODO: may need to convert these
    //std::shared_ptr<tshirt::tshirt_state> c_impl_ex_initial_state;
    //std::shared_ptr<pdm03_struct> c_impl_ex_pdm_et_data;

    // Don't think this is needed, as it is implied now.
    //double c_impl_ex_timestep_size_s;

    // TODO: probably don't need this
    /*
    **
     * The fluxes from Fred's standalone code are all in units of meters per time step.   Multiply them by the
     * "c_impl_ex_catchment_area_km2" variable to convert them into cubic meters per time step.
     */
    //double c_impl_ex_catchment_area_km2;

    double error_bound_percent;
    double error_upper_bound_min;

    double upper_bound_factor;
    double lower_bound_factor;

    std::vector<std::vector<double>> giuh_ordinate_examples;
    bool is_giuh_ordinate_examples;
    std::vector<forcing_params> forcing_params_examples;
    bool is_forcing_params_examples;

    void init_forcing_params_examples();

    void init_giuh_ordinate_examples();


};

void Tshirt_C_Realization_Test::SetUp() {

    // If changed in debugging, modify here.
    error_bound_percent = 0.01;
    error_upper_bound_min = 0.001;
    upper_bound_factor = 1.0 + error_bound_percent;
    lower_bound_factor = 1.0 - error_bound_percent;

    // By default, this is false
    is_standalone_data_ingest_stream = false;

    std::vector<std::string> path_options = {
            "test/data/model/tshirt/",
            "./test/data/model/tshirt/",
            "../test/data/model/tshirt/",
            "../../test/data/model/tshirt/",
    };

    std::vector<std::string> name_options = {"expected_results.csv"};

    // Build vector of names by building combinations of the path and basename options
    std::vector<std::string> data_file_names(path_options.size() * name_options.size());

    // Build so that all path names are tried for given basename before trying a different basename option
    for (auto & name_option : name_options) {
        for (auto & path_option : path_options) {
            std::string string_combo = path_option + name_option;
            data_file_names.push_back(string_combo);
        }
    }

    // Then go through in order and find the fist existing combination
    standalone_output_data_file = utils::FileChecker::find_first_readable(data_file_names);

    //c_impl_ex_timestep_size_s = 60 * 60;    // 60 seconds * 60 minutes => 1 hour size

    init_giuh_ordinate_examples();
    init_forcing_params_examples();
}

void Tshirt_C_Realization_Test::TearDown() {
    if (is_standalone_data_ingest_stream && standalone_data_ingest_stream.is_open()) {
        standalone_data_ingest_stream.close();
    }
}

void Tshirt_C_Realization_Test::open_standalone_c_impl_data_stream() {
    ASSERT_FALSE(standalone_output_data_file.empty());

    standalone_data_ingest_stream.open(standalone_output_data_file.c_str());

    ASSERT_TRUE(standalone_data_ingest_stream.is_open());

    is_standalone_data_ingest_stream = true;
}

void Tshirt_C_Realization_Test::setup_standalone_c_impl_example_case() {

    // Note using NGen value instead of Fred's value of 0.0 (which will cancel out things like Klf)
    //double mult = 1.0;
    // Valid range for this is 10-10000
    // TODO: considering tinkering with the value for this in multiple samples
    // Also called 'lksatfac'
    double mult = 1000.0;

    double maxsmc = 0.439;
    //double wltsmc = 0.055;
    double wltsmc = 0.066;
    double satdk = 3.38e-06;
    double satpsi = 0.355;
    // High slop (0.0-1.0) will lead to almost no lateral flow (consider < 0.05)
    // TODO: considering tinkering with the value for this in multiple samples
    double slop = 1.0;
    double bb = 4.05;
    double alpha_fc = 0.33;
    double expon = 6.0;

    // From Freds code; (approx) the average blue line drainage density for CONUS
    double drainage_density_km_per_km2 = 3.5;

    double assumed_near_channel_water_table_slope = 0.01;

    // NGen has 0.0000672
    // see lines 326-329 in Fred's code
    double Klf = 2.0 * assumed_near_channel_water_table_slope * mult * satdk * 2.0 * drainage_density_km_per_km2;
    Klf *= TSHIRT_C_FIXED_TIMESTEP_SIZE_S;     // Use this to convert Klf from m/s to m/timestep (m/h)

    // NGen has 1.08
    double Cgw = 0.01;   // NGen has 1.08

    // Note that NGen uses 8
    int nash_n = 2;

    // Note that NGen uses 0.1
    double Kn = 0.03;

    double max_gw_storage = 16.0;

    //Define tshirt params
    //{maxsmc, wltsmc, satdk, satpsi, slope, b, multiplier, aplha_fx, klf, kn, nash_n, Cgw, expon, max_gw_storage}
    c_impl_ex_tshirt_params = std::make_shared<tshirt::tshirt_params>(tshirt::tshirt_params{
            maxsmc,   //maxsmc FWRFH
            wltsmc,    //wltsmc
            satdk,   //satdk FWRFH
            satpsi,    //satpsi
            slop,   //slope
            bb,      //b bexp? FWRFH
            mult,    //multipier  FIXMME what should this value be
            alpha_fc,    //aplha_fc
            Klf,    //Klf F
            Kn,    //Kn Kn	0.001-0.03 F
            nash_n,      //nash_n
            Cgw,    //Cgw C? FWRFH
            expon,    //expon FWRFH
            max_gw_storage  //max_gw_storage Sgwmax FWRFH
    });
}

void Tshirt_C_Realization_Test::init_giuh_ordinate_examples() {
    if (!is_giuh_ordinate_examples) {
        std::vector<double> giuh_ordinates_1(5);
        giuh_ordinates_1[0] = 0.06;  // note these sum to 1.0.  If we have N ordinates, we need a queue sized N+1 to perform
        giuh_ordinates_1[1] = 0.51;  // the convolution.
        giuh_ordinates_1[2] = 0.28;
        giuh_ordinates_1[3] = 0.12;
        giuh_ordinates_1[4] = 0.03;
        giuh_ordinate_examples.push_back(giuh_ordinates_1);

        is_giuh_ordinate_examples = true;
    }
}

void Tshirt_C_Realization_Test::init_forcing_params_examples() {
    if (!is_forcing_params_examples) {
        std::vector<std::string> forcing_path_opts = {
                "test/data/forcing/cat-89_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "./test/data/forcing/cat-89_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "../test/data/forcing/cat-89_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "../../test/data/forcing/cat-89_2015-12-01 00_00_00_2015-12-30 23_00_00.csv"
        };
        std::string path = utils::FileChecker::find_first_readable(forcing_path_opts);
        forcing_params_examples.push_back(forcing_params(path, "legacy", "2015-12-01 00:00:00", "2015-12-01 23:00:00"));

        is_forcing_params_examples = true;
    }
}

// Simple test to make sure the run function executes and that the inherent mass-balance check returned by run is good.
TEST_F(Tshirt_C_Realization_Test, TestRun0) {
    int example_index = 0;

    tshirt::tshirt_params params{1000.0, 1.0, 10.0, 0.1, 0.01, 3, 1.0, 1.0, 1.0, 1.0, 2, 1.0, 1.0, 100.0};

    std::vector<double> nash_storage(params.nash_n);
    for (int i = 0; i < params.nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            0.667,
            0.5,
            true,
            "wat-88",
            giuh_ordinates,
            params,
            nash_storage);

    int result = tshirt_c_real.run_formulation_for_timestep(0.0, 3600);

    // TODO: figure out how to test for bogus/mismatched nash_n and state nash vector size (without silent error)

    // Should return 0 if mass balance check was good
    EXPECT_EQ(result, 0);
}

/** Test function for getting values for time step in formatted output string, for first and last time step. */
TEST_F(Tshirt_C_Realization_Test, TestGetOutputLineForTimestep1a) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> result_vector;
    string line;

    int timestep = 0;

    while (getline(standalone_data_ingest_stream, line)) {
        Tokenizer tokenizer(line);
        result_vector.assign(tokenizer.begin(), tokenizer.end());

        double input_storage = std::stod(result_vector[1]) / 1000 / 3600;

        // Output the line essentially
        //copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));
        //cout << "\n";

        tshirt_c_real.run_formulation_for_timestep(input_storage, 3600);
        timestep++;
    }

    std::string actual_first = tshirt_c_real.get_output_line_for_timestep(0);
    std::string actual_last = tshirt_c_real.get_output_line_for_timestep(timestep - 1);
    std::string outside_bounds = tshirt_c_real.get_output_line_for_timestep(timestep);

    EXPECT_EQ(actual_first, "0.010000,0.001523,0.000091,0.000000,0.191106,0.191197");
    ASSERT_EQ(actual_last, "0.000000,0.000000,0.000000,0.000003,0.000233,0.000236");
}

/** Test function for getting values for time step in formatted output string handles out-of-bounds case as expected. */
TEST_F(Tshirt_C_Realization_Test, TestGetOutputLineForTimestep1b) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> result_vector;
    string line;

    int timestep = 0;

    while (getline(standalone_data_ingest_stream, line)) {
        Tokenizer tokenizer(line);
        result_vector.assign(tokenizer.begin(), tokenizer.end());
        double input_storage = std::stod(result_vector[1]) / 1000;
        tshirt_c_real.run_formulation_for_timestep(input_storage, 3600);
        timestep++;
    }

    std::string actual_last = tshirt_c_real.get_output_line_for_timestep(timestep - 1);
    std::string outside_bounds = tshirt_c_real.get_output_line_for_timestep(timestep);

    EXPECT_EQ(actual_last, "0.000000,0.000000,0.000000,0.000049,0.000336,0.000385");
    ASSERT_EQ(outside_bounds, "");
}

/**
 * Test function for getting values for time step in formatted output string, for first and last time step, when reading
 * from forcing data.
 */
TEST_F(Tshirt_C_Realization_Test, TestGetOutputLineForTimestep2a) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> result_vector;
    string line;

    for (int timestep = 0; timestep < 5; ++timestep) {
        tshirt_c_real.get_response(timestep, 3600);
    }

    std::string actual_first = tshirt_c_real.get_output_line_for_timestep(0);
    std::string actual_last = tshirt_c_real.get_output_line_for_timestep(4);

    EXPECT_EQ(actual_first, "0.000000,0.000000,0.000000,0.000000,0.191086,0.191086");
    ASSERT_EQ(actual_last, "0.000000,0.000000,0.000000,0.000242,0.145356,0.145598");
}

/**
 * Test function for getting values for time step in formatted output string handles out-of-bounds case as expected,
 * when reading from forcing data.
 */
TEST_F(Tshirt_C_Realization_Test, TestGetOutputLineForTimestep2b) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> result_vector;
    string line;

    for (int timestep = 0; timestep < 5; ++timestep) {
        tshirt_c_real.get_response(timestep, 3600);
    }

    std::string actual_last = tshirt_c_real.get_output_line_for_timestep(4);
    std::string outside_bounds = tshirt_c_real.get_output_line_for_timestep(5);

    EXPECT_EQ(actual_last, "0.000000,0.000000,0.000000,0.000242,0.145356,0.145598");
    ASSERT_EQ(outside_bounds, "");
}

/** Test function for getting number of output variables for realization type. */
TEST_F(Tshirt_C_Realization_Test, TestGetOutputItemCount1a) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    ASSERT_EQ(tshirt_c_real.get_output_item_count(), tshirt_c_real.get_output_var_names().size());
}

/** Test header fields are reasonable (same size collection) for output variables. */
TEST_F(Tshirt_C_Realization_Test, TestGetOutputHeaderFields1a) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    ASSERT_EQ(tshirt_c_real.get_output_header_fields().size(), tshirt_c_real.get_output_var_names().size());
}

/** Test function for getting the output variable names for realization type. */
TEST_F(Tshirt_C_Realization_Test, TestGetOutputVariableNames1a) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> var_names = {
            OUT_VAR_RAINFALL,
            OUT_VAR_SURFACE_RUNOFF,
            OUT_VAR_GIUH_RUNOFF,
            OUT_VAR_LATERAL_FLOW,
            OUT_VAR_BASE_FLOW,
            OUT_VAR_TOTAL_DISCHARGE
    };

    ASSERT_EQ(tshirt_c_real.get_output_var_names(), var_names);
}

/** Test values getting function for surface runoff. */
TEST_F(Tshirt_C_Realization_Test, TestGetValue1a) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> result_vector;
    std::vector<double> values_vector;
    string line;

    while (getline(standalone_data_ingest_stream, line)) {
        Tokenizer tokenizer(line);
        result_vector.assign(tokenizer.begin(), tokenizer.end());
        double input_storage = std::stod(result_vector[1]);
        tshirt_c_real.run_formulation_for_timestep(input_storage, 3600);
        values_vector.emplace_back(tshirt_c_real.get_latest_flux_surface_runoff());
    }

    ASSERT_EQ(tshirt_c_real.get_value(OUT_VAR_SURFACE_RUNOFF), values_vector);
}

/** Test values getting function for total discharge. */
TEST_F(Tshirt_C_Realization_Test, TestGetValue1b) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> result_vector;
    std::vector<double> values_vector;
    string line;

    while (getline(standalone_data_ingest_stream, line)) {
        Tokenizer tokenizer(line);
        result_vector.assign(tokenizer.begin(), tokenizer.end());
        double input_storage = std::stod(result_vector[1]);
        tshirt_c_real.run_formulation_for_timestep(input_storage, 3600);
        values_vector.emplace_back(tshirt_c_real.get_latest_flux_total_discharge());
    }

    ASSERT_EQ(tshirt_c_real.get_value(OUT_VAR_TOTAL_DISCHARGE), values_vector);
}

/** Test direct surface runoff flux calculations for first example, within bounds. */
TEST_F(Tshirt_C_Realization_Test, TestSurfaceRunoffCalc1a) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> result_vector;
    string line;

    while (getline(standalone_data_ingest_stream, line)) {
        Tokenizer tokenizer(line);
        result_vector.assign(tokenizer.begin(), tokenizer.end());

        double input_storage = std::stod(result_vector[1]);
        // The fluxes from Fred's code are all in units of meters per time step.   Multiply them by the "c_impl_ex_catchment_area_km2"
        // variable to convert them into cubic meters per time step.
        //input_storage *= c_impl_ex_catchment_area_km2;

        // However, the data starts in mm/h, so first convert mm to m ...
        input_storage /= 1000;
        // ... then convert m / h to m / s
        input_storage /= 3600;

        // runoff is index 2
        double expected = std::stod(result_vector[2]);

        // Also convert expected from mm / h to m / h
        expected /= 1000;

        // Output the line essentially
        copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));
        cout << "\n";

        tshirt_c_real.run_formulation_for_timestep(input_storage, 3600);

        double actual = tshirt_c_real.get_latest_flux_surface_runoff();

        // Note that, for non-zero values, having to work within a reasonable upper and lower bounds to allow for
        // precision and rounding errors both on the calculation and sample-data-recording side.
        if (expected == 0.0) {
            EXPECT_EQ(actual, expected);
        }
        else {
            double upper_bound = expected * upper_bound_factor;
            double lower_bound = expected * lower_bound_factor;
            EXPECT_LE(actual, upper_bound);
            EXPECT_GE(actual, lower_bound);
        }

    }
}

/** Test GIUH runoff calculations. */
TEST_F(Tshirt_C_Realization_Test, TestGiuhRunoffCalc1a) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> result_vector;
    string line;

    while (getline(standalone_data_ingest_stream, line)) {
        Tokenizer tokenizer(line);
        result_vector.assign(tokenizer.begin(), tokenizer.end());

        double input_storage = std::stod(result_vector[1]);
        // The fluxes from Fred's code are all in units of meters per time step.   Multiply them by the "c_impl_ex_catchment_area_km2"
        // variable to convert them into cubic meters per time step.
        //input_storage *= c_impl_ex_catchment_area_km2;

        // However, the data starts in mm/h, so first convert mm to m ...
        input_storage /= 1000;
        // ... then convert m / h to m / s
        input_storage /= 3600;

        // giuh runoff is index 3
        double expected = std::stod(result_vector[3]);

        // Also convert expected from mm / h to m / h
        expected /= 1000;

        // Output the line essentially
        copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));
        cout << "\n";

        tshirt_c_real.run_formulation_for_timestep(input_storage, 3600);
        double actual = tshirt_c_real.get_latest_flux_giuh_runoff();

        // Note that, for non-zero values, having to work within a reasonable upper and lower bounds to allow for
        // precision and rounding errors both on the calculation and sample-data-recording side.
        if (expected == 0.0) {
            // Might be acceptable if actual value is less than 10^-9
            //EXPECT_LT(actual - expected, 0.000000001);
            // ... but for now, do this
            EXPECT_EQ(actual, expected);
        }
        else {
            double upper_bound = expected * upper_bound_factor;
            double lower_bound = expected * lower_bound_factor;
            // TODO: apply this logic to the other tests also.
            // Keep in mind that the sample data only has so much precision (for brevity/formatting purposes).  As such,
            // upper and lower bounds for this test also need to be limited by what is representable.
            // This is the smallest value that can be written to test data with current formatting (then divided by
            // 1000, since in millimeters)
            double representable_bound_limit = 0.000001 / 1000;
            double rep_upper_bound = expected + representable_bound_limit;
            double rep_lower_bound = expected - representable_bound_limit;
            upper_bound = max(upper_bound, rep_upper_bound);
            lower_bound = min(lower_bound, rep_lower_bound);

            EXPECT_LE(actual, upper_bound);
            EXPECT_GE(actual, lower_bound);
        }
    }
}

/** Test soil lateral flow calculations. */
TEST_F(Tshirt_C_Realization_Test, TestLateralFlowCalc1a) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> result_vector;
    string line;

    while (getline(standalone_data_ingest_stream, line)) {
        Tokenizer tokenizer(line);
        result_vector.assign(tokenizer.begin(), tokenizer.end());

        double input_storage = std::stod(result_vector[1]);
        // The fluxes from Fred's code are all in units of meters per time step.   Multiply them by the "c_impl_ex_catchment_area_km2"
        // variable to convert them into cubic meters per time step.
        //input_storage *= c_impl_ex_catchment_area_km2;

        // However, the data starts in mm/h, so first convert mm to m ...
        input_storage /= 1000;
        // ... then convert m / h to m / s
        input_storage /= 3600;

        // lateral flow is index 4
        double expected = std::stod(result_vector[4]);

        // Also convert expected from mm / h to m / h
        expected /= 1000;

        // Output the line essentially
        copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));
        cout << "\n";

        tshirt_c_real.run_formulation_for_timestep(input_storage, 3600);
        double actual = tshirt_c_real.get_latest_flux_lateral_flow();

        // Note that, for non-zero values, having to work within a reasonable upper and lower bounds to allow for
        // precision and rounding errors both on the calculation and sample-data-recording side.
        if (expected == 0.0) {
            // Might be acceptable if actual value is less than 10^-9
            //EXPECT_LT(actual - expected, 0.000000001);
            // ... but for now, do this
            EXPECT_EQ(actual, expected);
        }
        else {
            double upper_bound = expected * upper_bound_factor;
            double lower_bound = expected * lower_bound_factor;
            EXPECT_LE(actual, upper_bound);
            EXPECT_GE(actual, lower_bound);
        }
    }
}

/** Test base flow calculations. */
TEST_F(Tshirt_C_Realization_Test, TestBaseFlowCalc1a) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> result_vector;
    string line;

    while (getline(standalone_data_ingest_stream, line)) {
        Tokenizer tokenizer(line);
        result_vector.assign(tokenizer.begin(), tokenizer.end());

        double input_storage = std::stod(result_vector[1]);
        // The fluxes from Fred's code are all in units of meters per time step.   Multiply them by the "c_impl_ex_catchment_area_km2"
        // variable to convert them into cubic meters per time step.
        //input_storage *= c_impl_ex_catchment_area_km2;

        // However, the data starts in mm/h, so first convert mm to m ...
        input_storage /= 1000;
        // ... then convert m / h to m / s
        input_storage /= 3600;

        // base flow is index 5
        double expected = std::stod(result_vector[5]);

        // Also convert expected from mm / h to m / h
        expected /= 1000;

        // Output the line essentially
        copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));
        cout << "\n";

        tshirt_c_real.run_formulation_for_timestep(input_storage, 3600);
        double actual = tshirt_c_real.get_latest_flux_base_flow();

        // Note that, for non-zero values, having to work within a reasonable upper and lower bounds to allow for
        // precision and rounding errors both on the calculation and sample-data-recording side.
        if (expected == 0.0) {
            // Might be acceptable if actual value is less than 10^-9
            //EXPECT_LT(actual - expected, 0.000000001);
            // ... but for now, do this
            EXPECT_EQ(actual, expected);
        }
        else {
            double upper_bound = expected * upper_bound_factor;
            double lower_bound = expected * lower_bound_factor;
            EXPECT_LE(actual, upper_bound);
            EXPECT_GE(actual, lower_bound);
        }
    }
}

/** Test total discharge output calculations. */
TEST_F(Tshirt_C_Realization_Test, TestTotalDischargeOutputCalc1a) {
    int example_index = 0;

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // init gw res as half full for test
    double gw_storage_ratio = 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage_ratio = 0.667;

    std::vector<double> nash_storage(c_impl_ex_tshirt_params->nash_n);
    for (int i = 0; i < c_impl_ex_tshirt_params->nash_n; i++) {
        nash_storage[i] = 0.0;
    }

    std::vector<double> giuh_ordinates = giuh_ordinate_examples[example_index];

    realization::Tshirt_C_Realization tshirt_c_real(
            forcing_params_examples[example_index],
            utils::StreamHandler(),
            soil_storage_ratio,
            gw_storage_ratio,
            true,
            "wat-88",
            giuh_ordinates,
            *c_impl_ex_tshirt_params,
            nash_storage);

    std::vector<std::string> result_vector;
    string line;

    while (getline(standalone_data_ingest_stream, line)) {
        Tokenizer tokenizer(line);
        result_vector.assign(tokenizer.begin(), tokenizer.end());

        double input_storage = std::stod(result_vector[1]);
        // The fluxes from Fred's code are all in units of meters per time step.   Multiply them by the "c_impl_ex_catchment_area_km2"
        // variable to convert them into cubic meters per time step.
        //input_storage *= c_impl_ex_catchment_area_km2;

        // However, the data starts in mm/h, so first convert mm to m ...
        input_storage /= 1000;
        // ... then convert m / h to m / s
        input_storage /= 3600;

        // output is index 5
        double expected = std::stod(result_vector[6]);

        // Also convert expected from mm / h to m / h
        expected /= 1000;

        // Output the line essentially
        copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));
        cout << "\n";

        tshirt_c_real.run_formulation_for_timestep(input_storage, 3600);
        double actual = tshirt_c_real.get_latest_flux_total_discharge();

        // Note that, for non-zero values, having to work within a reasonable upper and lower bounds to allow for
        // precision and rounding errors both on the calculation and sample-data-recording side.
        if (expected == 0.0) {
            // Might be acceptable if actual value is less than 10^-9
            //EXPECT_LT(actual - expected, 0.000000001);
            // ... but for now, do this
            EXPECT_EQ(actual, expected);
        }
        else {
            double upper_bound = expected * upper_bound_factor;
            double lower_bound = expected * lower_bound_factor;
            EXPECT_LE(actual, upper_bound);
            EXPECT_GE(actual, lower_bound);
        }
    }
}
