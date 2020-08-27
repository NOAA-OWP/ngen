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

    standalone_data_ingest_stream = std::ifstream(standalone_output_data_file.c_str());

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
                "/data/forcing/cat-89_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "./data/forcing/cat-89_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "../data/forcing/cat-89_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "../../data/forcing/cat-89_2015-12-01 00_00_00_2015-12-30 23_00_00.csv"
        };
        std::string path = utils::FileChecker::find_first_readable(forcing_path_opts);
        forcing_params_examples.push_back(forcing_params(path, "2015-12-01 00:00:00", "2015-12-01 23:00:00"));

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

    int result = tshirt_c_real.run_formulation_for_timestep(0.0);

    // TODO: figure out how to test for bogus/mismatched nash_n and state nash vector size (without silent error)

    // Should return 0 if mass balance check was good
    EXPECT_EQ(result, 0);
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

        // Remember, signature of tshirt_c run() expects in mm/h, which is how this comes through from source data
        // So, for now at least, no conversion is needed for the input data
        // TODO: this probably needs to be changed to work in meters per hour
        //input_storage /= 1000;

        // runoff is index 2
        double expected = std::stod(result_vector[2]);

        // Convert from mm / h to m / s
        expected /= 1000;

        // Output the line essentially
        copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));
        cout << "\n";

        tshirt_c_real.run_formulation_for_timestep(input_storage);

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

        // Remember, signature of tshirt_c run() expects in mm/h, which is how this comes through from source data
        // So, for now at least, no conversion is needed for the input data
        // TODO: this probably needs to be changed to work in meters per hour
        //input_storage /= 1000;

        // giuh runoff is index 3
        double expected = std::stod(result_vector[3]);

        // Convert from mm / h to m / s
        expected /= 1000;

        // Output the line essentially
        copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));
        cout << "\n";

        tshirt_c_real.run_formulation_for_timestep(input_storage);
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

        // Remember, signature of tshirt_c run() expects in mm/h, which is how this comes through from source data
        // So, for now at least, no conversion is needed for the input data
        // TODO: this probably needs to be changed to work in meters per hour
        //input_storage /= 1000;

        // lateral flow is index 4
        double expected = std::stod(result_vector[4]);

        // Convert from mm / h to m / s
        expected /= 1000;

        // Output the line essentially
        copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));
        cout << "\n";

        tshirt_c_real.run_formulation_for_timestep(input_storage);
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

        // Remember, signature of tshirt_c run() expects in mm/h, which is how this comes through from source data
        // So, for now at least, no conversion is needed for the input data
        // TODO: this probably needs to be changed to work in meters per hour
        //input_storage /= 1000;

        // base flow is index 5
        double expected = std::stod(result_vector[3]);

        // Convert from mm / h to m / s
        expected /= 1000;

        // Output the line essentially
        copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));
        cout << "\n";

        tshirt_c_real.run_formulation_for_timestep(input_storage);
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

/**
 * Test of actual Tshirt model execution, comparing against data generated with alternate, C-based implementation.
 */
/*
TEST_F(Tshirt_C_Realization_Test, TestRun7) {

    open_standalone_c_impl_data_stream();

    setup_standalone_c_impl_example_case();

    // Use an implementation that doesn't do any ET loss calculations
    std::unique_ptr<tshirt::tshirt_model> model = std::make_unique<tshirt::no_et_tshirt_model>(
            tshirt::no_et_tshirt_model(*c_impl_ex_tshirt_params, c_impl_ex_initial_state));

    std::vector<std::string> result_vector;
    string line;

    while (getline(standalone_data_ingest_stream, line)) {
        Tokenizer tokenizer(line);
        result_vector.assign(tokenizer.begin(), tokenizer.end());

        // vector now contains strings from one row, output to cout here
        //copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));

        //double hour = std::stod(result_vector[0]);

        double input_storage = std::stod(result_vector[1]) / 1000;
        // The fluxes from Fred's code are all in units of meters per time step.   Multiply them by the "c_impl_ex_catchment_area_km2"
        // variable to convert them into cubic meters per time step.
        //input_storage *= c_impl_ex_catchment_area_km2;

        // convert from m/h to m/s:
        //input_storage /= c_impl_ex_timestep_size_s;

        double expected_direct_runoff = std::stod(result_vector[2]) / 1000;
        //double expected_giuh_runoff = std::stod(result_vector[3]) / 1000;
        double expected_lateral_flow = std::stod(result_vector[4]) / 1000;
        double expected_base_flow = std::stod(result_vector[5]) / 1000;
        //double expected_total_discharge = std::stod(result_vector[6]) / 1000;

        model->run(c_impl_ex_timestep_size_s, input_storage, c_impl_ex_pdm_et_data);

        double lateral_flow_upper_bound =
                expected_lateral_flow != 0.0 ? expected_lateral_flow * upper_bound_factor : error_upper_bound_min;
        double lateral_flow_lower_bound = expected_lateral_flow * lower_bound_factor;
        EXPECT_LE(model->get_fluxes()->soil_lateral_flow_meters_per_second, lateral_flow_upper_bound);
        //ASSERT_LE(model.get_fluxes()->soil_lateral_flow_meters_per_second, lateral_flow_upper_bound);
        EXPECT_GE(model->get_fluxes()->soil_lateral_flow_meters_per_second, lateral_flow_lower_bound);
        //ASSERT_GE(model.get_fluxes()->soil_lateral_flow_meters_per_second, lateral_flow_lower_bound);

        double base_flow_upper_bound =
                expected_base_flow != 0.0 ? expected_base_flow * upper_bound_factor : error_upper_bound_min;
        double base_flow_lower_bound = expected_base_flow * lower_bound_factor;
        /*
        //EXPECT_LE(model.get_fluxes()->groundwater_flow_meters_per_second, base_flow_upper_bound);
        ASSERT_LE(model.get_fluxes()->groundwater_flow_meters_per_second, base_flow_upper_bound);
        //EXPECT_GE(model.get_fluxes()->groundwater_flow_meters_per_second, base_flow_lower_bound);
        ASSERT_GE(model.get_fluxes()->groundwater_flow_meters_per_second, base_flow_lower_bound);
        */

        /*
        double direct_runoff_upper_bound =
                expected_direct_runoff != 0.0 ? expected_direct_runoff * upper_bound_factor : error_upper_bound_min;
        double direct_runoff_lower_bound = expected_direct_runoff * lower_bound_factor;
        //EXPECT_LE(model.get_fluxes()->surface_runoff_meters_per_second, direct_runoff_upper_bound);
        ASSERT_LE(model.get_fluxes()->surface_runoff_meters_per_second, direct_runoff_upper_bound);
        ASSERT_GE(model.get_fluxes()->surface_runoff_meters_per_second, direct_runoff_lower_bound);
        */
/*
    }

}
*/

