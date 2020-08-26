#include "Tshirt_C_Realization.hpp"
#include "Constants.h"
#include <utility>
#include "tshirt_c.h"

using namespace realization;

Tshirt_C_Realization::Tshirt_C_Realization(forcing_params forcing_config,
                                           utils::StreamHandler output_stream,
                                           double soil_storage,
                                           double groundwater_storage,
                                           bool storage_values_are_ratios,
                                           std::string catchment_id,
                                           giuh::GiuhJsonReader &giuh_json_reader,
                                           tshirt::tshirt_params params,
                                           const vector<double> &nash_storage)
        : Tshirt_C_Realization::Tshirt_C_Realization(std::move(forcing_config), output_stream, soil_storage,
                                                     groundwater_storage, storage_values_are_ratios,
                                                     std::move(catchment_id),
                                                     giuh_json_reader.extract_cumulative_frequency_ordinates(catchment_id),
                                                     params, nash_storage)
{

}

Tshirt_C_Realization::Tshirt_C_Realization(forcing_params forcing_config,
                                           utils::StreamHandler output_stream,
                                           double soil_storage,
                                           double groundwater_storage,
                                           bool storage_values_are_ratios,
                                           std::string catchment_id,
                                           std::vector<double> giuh_ordinates,
                                           tshirt::tshirt_params params,
                                           const vector<double> &nash_storage)
        : HY_CatchmentArea(std::move(forcing_config), output_stream), catchment_id(std::move(catchment_id)),
          giuh_cdf_ordinates(std::move(giuh_ordinates)), params(params), nash_storage(nash_storage), c_soil_params(NWM_soil_parameters()),
          groundwater_conceptual_reservoir(conceptual_reservoir()), soil_conceptual_reservoir(conceptual_reservoir()),
          c_aorc_params(aorc_forcing_data())
{
    fluxes = std::vector<std::shared_ptr<tshirt_c_result_fluxes>>();

    // Convert params to struct for C-impl
    c_soil_params.D = params.depth;
    c_soil_params.bb = params.b;
    c_soil_params.mult = params.multiplier;
    c_soil_params.satdk = params.satdk;
    c_soil_params.satpsi = params.satpsi;
    c_soil_params.slop = params.slope;
    c_soil_params.smcmax = params.maxsmc;
    c_soil_params.wltsmc = params.wltsmc;

    // TODO: Convert aorc to struct for C-impl

    //  Populate the groundwater conceptual reservoir data structure
    //-----------------------------------------------------------------------
    // one outlet, 0.0 threshold, nonliner and exponential as in NWM
    groundwater_conceptual_reservoir.is_exponential=TRUE;         // set this true TRUE to use the exponential form of the discharge equation
    groundwater_conceptual_reservoir.storage_max_m=16.0;            // calibrated Sugar Creek WRF-Hydro value 16.0, I assume mm.
    groundwater_conceptual_reservoir.coeff_primary=0.01;           // per h
    groundwater_conceptual_reservoir.exponent_primary=6.0;              // linear iff 1.0, non-linear iff > 1.0
    groundwater_conceptual_reservoir.storage_threshold_primary_m=0.0;     // 0.0 means no threshold applied
    groundwater_conceptual_reservoir.storage_threshold_secondary_m=0.0;   // 0.0 means no threshold applied
    groundwater_conceptual_reservoir.coeff_secondary=0.0;                 // 0.0 means that secondary outlet is not applied
    groundwater_conceptual_reservoir.exponent_secondary=1.0;              // linear

    double trigger_z_m = 0.5;   // distance from the bottom of the soil column to the center of the lowest discretization

    // calculate the activation storage ffor the secondary lateral flow outlet in the soil nonlinear reservoir.
    // following the method in the NWM/t-shirt parameter equivalence document, assuming field capacity soil
    // suction pressure = 1/3 atm= field_capacity_atm_press_fraction * atm_press_Pa.

    // equation 3 from NWM/t-shirt parameter equivalence document
    double H_water_table_m = params.alpha_fc * STANDARD_ATMOSPHERIC_PRESSURE_PASCALS / WATER_SPECIFIC_WEIGHT;


    // solve the integral given by Eqn. 5 in the parameter equivalence document.
    // this equation calculates the amount of water stored in the 2 m thick soil column when the water content
    // at the center of the bottom discretization (trigger_z_m) is at field capacity
    double Omega = H_water_table_m - trigger_z_m;
    double lower_lim = pow(Omega, (1.0 - 1.0 / c_soil_params.bb)) / (1.0 - 1.0 / c_soil_params.bb);
    double upper_lim = pow(Omega + c_soil_params.D, (1.0 - 1.0 / c_soil_params.bb)) / (1.0 - 1.0 / c_soil_params.bb);

    // initialize lateral flow function parameters
    //---------------------------------------------
    double field_capacity_storage_threshold_m =
            c_soil_params.smcmax * pow(1.0 / c_soil_params.satpsi, (-1.0 / c_soil_params.bb)) *
            (upper_lim - lower_lim);
    double lateral_flow_threshold_storage_m = field_capacity_storage_threshold_m;  // making them the same, but they don't have 2B


    // Initialize the soil conceptual reservoir data structure.  Indented here to highlight different purposes
    //-------------------------------------------------------------------------------------------------------------
    // soil conceptual reservoir first, two outlets, two thresholds, linear (exponent=1.0).
    soil_conceptual_reservoir.is_exponential = FALSE;  // set this true TRUE to use the exponential form of the discharge equation
    // this should NEVER be set to true in the soil reservoir.
    soil_conceptual_reservoir.storage_max_m = c_soil_params.smcmax * c_soil_params.D;
    //  vertical percolation parameters------------------------------------------------
    soil_conceptual_reservoir.coeff_primary = c_soil_params.satdk * c_soil_params.slop * 3600.0; // m per h
    soil_conceptual_reservoir.exponent_primary = 1.0;      // 1.0=linear
    soil_conceptual_reservoir.storage_threshold_primary_m = field_capacity_storage_threshold_m;
    // lateral flow parameters --------------------------------------------------------
    soil_conceptual_reservoir.coeff_secondary = 0.01;  // 0.0 to deactiv. else =lateral_flow_linear_reservoir_constant;   // m per h
    soil_conceptual_reservoir.exponent_secondary = 1.0;   // 1.0=linear
    soil_conceptual_reservoir.storage_threshold_secondary_m = lateral_flow_threshold_storage_m;

    // TODO: make sure these hard-coded values get into the tests
    //groundwater_conceptual_reservoir.storage_m = groundwater_conceptual_reservoir.storage_max_m * 0.5;  // INITIALIZE HALF FULL.
    //soil_conceptual_reservoir.storage_m = soil_conceptual_reservoir.storage_max_m * 0.667;  // INITIALIZE SOIL STORAGE
    groundwater_conceptual_reservoir.storage_m = init_reservoir_storage(storage_values_are_ratios,
                                                                        groundwater_storage,
                                                                        groundwater_conceptual_reservoir.storage_max_m);
    soil_conceptual_reservoir.storage_m = init_reservoir_storage(storage_values_are_ratios,
                                                                 soil_storage,
                                                                 soil_conceptual_reservoir.storage_max_m);

}

Tshirt_C_Realization::Tshirt_C_Realization(forcing_params forcing_config,
                                           utils::StreamHandler output_stream,
                                           double soil_storage,
                                           double groundwater_storage,
                                           bool storage_values_are_ratios,
                                           std::string catchment_id,
                                           giuh::GiuhJsonReader &giuh_json_reader,
                                           double maxsmc,
                                           double wltsmc,
                                           double satdk,
                                           double satpsi,
                                           double slope,
                                           double b,
                                           double multiplier,
                                           double alpha_fc,
                                           double Klf,
                                           double Kn,
                                           int nash_n,
                                           double Cgw,
                                           double expon,
                                           double max_gw_storage,
                                           const vector<double> &nash_storage)
           : Tshirt_C_Realization::Tshirt_C_Realization(std::move(forcing_config), output_stream, soil_storage,
                                                        groundwater_storage, storage_values_are_ratios,
                                                        std::move(catchment_id), giuh_json_reader,
                                                        tshirt::tshirt_params(maxsmc, wltsmc, satdk, satpsi, slope, b,
                                                                              multiplier, alpha_fc, Klf, Kn, nash_n,
                                                                              Cgw, expon, max_gw_storage),
                                                        nash_storage)
{

}

Tshirt_C_Realization::~Tshirt_C_Realization()
{
    //destructor
}

double Tshirt_C_Realization::get_latest_flux_surface_runoff() {
    return fluxes.empty() ? 0.0 : fluxes.back()->Schaake_output_runoff_m;
}

double Tshirt_C_Realization::get_response(double input_flux, time_step_t t, time_step_t dt, void* et_params) {
    // TODO: check that dt is of approprate size

    int response_result = run_formulation_for_timestep(input_flux);

    // TODO: check time_step_t is the next expected time step to be calculated

    return fluxes.back()->Qout_m;
}

int Tshirt_C_Realization::run_formulation_for_timestep(double input_flux) {
    std::vector<double> input_flux_in_vector{input_flux};
    return run_formulation_for_timesteps(input_flux_in_vector);
}

int Tshirt_C_Realization::run_formulation_for_timesteps(std::vector<double> input_fluxes) {
    int num_timesteps = (int) input_fluxes.size();

    // FIXME: verify this needs to be independent like this
    // FIXME: also make sure it shouldn't be parameterized somewhere, rather than hard-coded
    double assumed_near_channel_water_table_slope = 0.01;

    double* giuh_ordinates = &giuh_cdf_ordinates[0];

    //aorc_forcing_data empty_forcing[num_timesteps];
    aorc_forcing_data empty_forcing[1];

    double* input_as_array = &input_fluxes[0];

    tshirt_c_result_fluxes output_fluxes_as_array[num_timesteps];

    // use this to sanity check the fluxes got added as expected to array
    int num_added_fluxes = 0;

    int result = run(c_soil_params,
                     groundwater_conceptual_reservoir,
                     soil_conceptual_reservoir,
                     num_timesteps,
                     giuh_ordinates,
                     (int)giuh_cdf_ordinates.size(),
                     params.alpha_fc,
                     assumed_near_channel_water_table_slope,
                     params.Cschaake,
                     params.Klf,
                     params.Kn,
                     params.nash_n,
                     FALSE,
                     &empty_forcing[0],
                     &input_as_array[0],
                     num_added_fluxes,
                     &output_fluxes_as_array[0]);

    // Move fluxes over to member data structure
    for (int i = 0; i < num_added_fluxes && i < num_timesteps; i++) {
        fluxes.push_back(std::make_shared<tshirt_c_result_fluxes>(output_fluxes_as_array[i]));
    }

    return result;
}

double Tshirt_C_Realization::init_reservoir_storage(bool is_ratio, double amount, double max_amount) {
    // Negative amounts are always ignored and just considered emtpy
    if (amount < 0.0) {
        return 0.0;
    }
    // When not a ratio (and positive), just return the literal amount
    if (!is_ratio) {
        return amount;
    }
    // When between 0 and 1, return the simple ratio computation
    if (amount <= 1.0) {
        return max_amount * amount;
    }
        // Otherwise, just return the literal amount, and assume the is_ratio value was invalid
        // TODO: is this the best way to handle this?
    else {
        return amount;
    }
}
