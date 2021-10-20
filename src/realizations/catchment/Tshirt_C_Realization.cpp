#include "Tshirt_C_Realization.hpp"
#include "Constants.h"
#include <utility>
#include "tshirt_c.h"
#include "GIUH.hpp"
#include <exception>
#include <functional>
#include <string>
#include <boost/algorithm/string/join.hpp>

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
        : Catchment_Formulation(catchment_id, std::move(std::make_unique<Forcing>(forcing_config)), output_stream), catchment_id(std::move(catchment_id)),
          giuh_cdf_ordinates(std::move(giuh_ordinates)), params(std::make_shared<tshirt_params>(params)), nash_storage(nash_storage), c_soil_params(NWM_soil_parameters()),
          groundwater_conceptual_reservoir(conceptual_reservoir()), soil_conceptual_reservoir(conceptual_reservoir()),
          c_aorc_params(aorc_forcing_data())
{
    _link_legacy_forcing();

    // initialize 0 values if necessary in Nash Cascade storage vector
    if (this->nash_storage.empty()) {
        for (int i = 0; i < params.nash_n; i++) {
            this->nash_storage.push_back(0.0);
        }
    }

    // Create this with 0 values initially
    giuh_runoff_queue_per_timestep = std::vector<double>(giuh_cdf_ordinates.size() + 1);
    for (int i = 0; i < giuh_cdf_ordinates.size() + 1; i++) {
        giuh_runoff_queue_per_timestep.push_back(0.0);
    }

    fluxes = std::vector<std::shared_ptr<tshirt_c_result_fluxes>>();

    // Convert params to struct for C-impl
    sync_c_storage_params();

    // TODO: Convert aorc to struct for C-impl

    this->init_ground_water_reservoir(groundwater_storage, storage_values_are_ratios);
    this->init_soil_reservoir(soil_storage, storage_values_are_ratios);

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
    _link_legacy_forcing();
}

Tshirt_C_Realization::Tshirt_C_Realization(
        std::string id,
        forcing_params forcing_config,
        utils::StreamHandler output_stream
) : Catchment_Formulation(std::move(id), std::move(std::make_unique<Forcing>(forcing_config)), output_stream) {
    _link_legacy_forcing();
    fluxes = std::vector<std::shared_ptr<tshirt_c_result_fluxes>>();
}

Tshirt_C_Realization::Tshirt_C_Realization(
        std::string id,
        unique_ptr<forcing::ForcingProvider> forcing_provider,
        utils::StreamHandler output_stream
) : Catchment_Formulation(std::move(id), std::move(forcing_provider), output_stream) {
    _link_legacy_forcing();

    fluxes = std::vector<std::shared_ptr<tshirt_c_result_fluxes>>();
}

Tshirt_C_Realization::~Tshirt_C_Realization()
{
    //destructor
}

/**
 * Return ``0``, as (for now) this type does not otherwise include ET within its calculations.
 *
 * @return ``0``
 */
double Tshirt_C_Realization::calc_et() {
    return 0;
}

void Tshirt_C_Realization::create_formulation(geojson::PropertyMap properties) {
    // TODO: don't particularly like the idea of "creating" the formulation, parameter constructs, etc., inside this
    //  type but outside the constructor.
    // TODO: look at creating a factory or something, rather than an instance, for doing this type of thing.

    // TODO: (if this remains and doesn't get replaced with factory) protect against this being called after calls to
    //  get_response have started being made, or else the reservoir values are going to be jacked up.
    this->validate_parameters(properties);

    catchment_id = this->get_id();

    //dt = options.at("timestep").as_natural_number();

    params = std::make_shared<tshirt_params>(tshirt_params{
            properties.at("maxsmc").as_real_number(),   //maxsmc FWRFH
            properties.at("wltsmc").as_real_number(),  //wltsmc  from fred_t-shirt.c FIXME NOT USED IN TSHIRT?!?!
            properties.at("satdk").as_real_number(),   //satdk FWRFH
            properties.at("satpsi").as_real_number(),    //satpsi    FIXME what is this and what should its value be?
            properties.at("slope").as_real_number(),   //slope
            properties.at("scaled_distribution_fn_shape_parameter").as_real_number(),      //b bexp? FWRFH
            properties.at("multiplier").as_real_number(),    //multipier  FIXMME (lksatfac)
            properties.at("alpha_fc").as_real_number(),    //aplha_fc   field_capacity_atm_press_fraction
            properties.at("Klf").as_real_number(),    //Klf lateral flow nash coefficient?
            properties.at("Kn").as_real_number(),    //Kn Kn	0.001-0.03 F Nash Cascade coeeficient
            static_cast<int>(properties.at("nash_n").as_natural_number()),      //number_lateral_flow_nash_reservoirs
            properties.at("Cgw").as_real_number(),    //fred_t-shirt gw res coeeficient (per h)
            properties.at("expon").as_real_number(),    //expon FWRFH
            properties.at("max_groundwater_storage_meters").as_real_number()   //max_gw_storage Sgwmax FWRFH
    });
    // Very important this also gets done
    sync_c_storage_params();

    init_ground_water_reservoir(properties.at("groundwater_storage_percentage").as_real_number(), true);
    init_soil_reservoir(properties.at("soil_storage_percentage").as_real_number(), true);

    nash_storage = properties.at("nash_storage").as_real_vector();

    geojson::JSONProperty giuh = properties.at("giuh");

    // Since this implementation really just cares about the ordinates, allow them to be passed directly here, or read
    // from a separate file
    if (giuh.has_key("cdf_ordinates")) {
        giuh_cdf_ordinates = giuh.at("cdf_ordinates").as_real_vector();
    }
    else {
        std::vector<std::string> missing_parameters;
        if (!giuh.has_key("giuh_path")) {
            missing_parameters.emplace_back("giuh_path");
        }
        if (!giuh.has_key("crosswalk_path")) {
            missing_parameters.emplace_back("crosswalk_path");
        }
        if (!missing_parameters.empty()) {
            std::string message = "A giuh configuration cannot be created for '" + catchment_id + "'; the following parameters are missing: ";

            for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
                message += missing_parameters[missing_parameter_index];

                if (missing_parameter_index < missing_parameters.size() - 1) {
                    message += ", ";
                }
            }

            throw std::runtime_error(message);
        }

        std::unique_ptr<giuh::GiuhJsonReader> giuh_reader = std::make_unique<giuh::GiuhJsonReader>(
                giuh.at("giuh_path").as_string(),
                giuh.at("crosswalk_path").as_string()
        );

        std::shared_ptr<giuh::giuh_kernel_impl> giuh_kernel = giuh_reader->get_giuh_kernel_for_id(catchment_id);
        giuh_kernel->set_interpolation_regularity_seconds(3600);
        // This needs to have all but the first interpolated incremental value (which is always 0 from the kernel), so
        giuh_cdf_ordinates = std::vector<double>(giuh_kernel->get_interpolated_incremental_runoff().size() - 1);
        for (int i = 0; i < giuh_cdf_ordinates.size(); ++ i) {
            giuh_cdf_ordinates[i] = giuh_kernel->get_interpolated_incremental_runoff()[i + 1];
        }
    }
    // Create this with 0 values initially
    giuh_runoff_queue_per_timestep = std::vector<double>(giuh_cdf_ordinates.size() + 1);
    for (int i = 0; i < giuh_cdf_ordinates.size() + 1; i++) {
        giuh_runoff_queue_per_timestep.push_back(0.0);
    }
}

void Tshirt_C_Realization::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
    // TODO: don't particularly like the idea of "creating" the formulation, parameter constructs, etc., inside this
    //  type but outside the constructor.
    // TODO: look at creating a factory or something, rather than an instance, for doing this type of thing.

    // TODO: (if this remains and doesn't get replaced with factory) protect against this being called after calls to
    //  get_response have started being made, or else the reservoir values are going to be jacked up.

    geojson::PropertyMap options = this->interpret_parameters(config, global);

    catchment_id = this->get_id();

    //dt = options.at("timestep").as_natural_number();

    params = std::make_shared<tshirt_params>(tshirt_params{
            options.at("maxsmc").as_real_number(),   //maxsmc FWRFH
            options.at("wltsmc").as_real_number(),  //wltsmc  from fred_t-shirt.c FIXME NOT USED IN TSHIRT?!?!
            options.at("satdk").as_real_number(),   //satdk FWRFH
            options.at("satpsi").as_real_number(),    //satpsi    FIXME what is this and what should its value be?
            options.at("slope").as_real_number(),   //slope
            options.at("scaled_distribution_fn_shape_parameter").as_real_number(),      //b bexp? FWRFH
            options.at("multiplier").as_real_number(),    //multipier  FIXMME (lksatfac)
            options.at("alpha_fc").as_real_number(),    //aplha_fc   field_capacity_atm_press_fraction
            options.at("Klf").as_real_number(),    //Klf lateral flow nash coefficient?
            options.at("Kn").as_real_number(),    //Kn Kn	0.001-0.03 F Nash Cascade coeeficient
            static_cast<int>(options.at("nash_n").as_natural_number()),      //number_lateral_flow_nash_reservoirs
            options.at("Cgw").as_real_number(),    //fred_t-shirt gw res coeeficient (per h)
            options.at("expon").as_real_number(),    //expon FWRFH
            options.at("max_groundwater_storage_meters").as_real_number()   //max_gw_storage Sgwmax FWRFH
    });
    // Very important this also gets done
    sync_c_storage_params();

    // TODO: might need to have this handle the ET params also

    init_ground_water_reservoir(options.at("groundwater_storage_percentage").as_real_number(), true);
    init_soil_reservoir(options.at("soil_storage_percentage").as_real_number(), true);

    nash_storage = options.at("nash_storage").as_real_vector();

    geojson::JSONProperty giuh = options.at("giuh");

    // Since this implementation really just cares about the ordinates, allow them to be passed directly here, or read
    // from a separate file
    if (giuh.has_key("cdf_ordinates")) {
        giuh_cdf_ordinates = giuh.at("cdf_ordinates").as_real_vector();
    }
    else {
        std::vector<std::string> missing_parameters;
        if (!giuh.has_key("giuh_path")) {
            missing_parameters.emplace_back("giuh_path");
        }
        if (!giuh.has_key("crosswalk_path")) {
            missing_parameters.emplace_back("crosswalk_path");
        }
        if (!missing_parameters.empty()) {
            std::string message = "A giuh configuration cannot be created for '" + catchment_id + "'; the following parameters are missing: ";

            for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
                message += missing_parameters[missing_parameter_index];

                if (missing_parameter_index < missing_parameters.size() - 1) {
                    message += ", ";
                }
            }

            throw std::runtime_error(message);
        }

        std::unique_ptr<giuh::GiuhJsonReader> giuh_reader = std::make_unique<giuh::GiuhJsonReader>(
                giuh.at("giuh_path").as_string(),
                giuh.at("crosswalk_path").as_string()
        );


        std::shared_ptr<giuh::giuh_kernel_impl> giuh_kernel = giuh_reader->get_giuh_kernel_for_id(catchment_id);
        giuh_kernel->set_interpolation_regularity_seconds(3600);
        // This needs to have all but the first interpolated incremental value (which is always 0 from the kernel), so
        giuh_cdf_ordinates = std::vector<double>(giuh_kernel->get_interpolated_incremental_runoff().size() - 1);
        for (int i = 0; i < giuh_cdf_ordinates.size(); ++ i) {
            giuh_cdf_ordinates[i] = giuh_kernel->get_interpolated_incremental_runoff()[i + 1];
        }
    }
    // Create this with 0 values initially
    giuh_runoff_queue_per_timestep = std::vector<double>(giuh_cdf_ordinates.size() + 1);
    for (int i = 0; i < giuh_cdf_ordinates.size() + 1; i++) {
        giuh_runoff_queue_per_timestep.push_back(0.0);
    }

}

std::string Tshirt_C_Realization::get_formulation_type() {
    return "tshirt_c";
}

double Tshirt_C_Realization::get_latest_flux_base_flow() {
    return fluxes.empty() ? 0.0 : fluxes.back()->flux_from_deep_gw_to_chan_m;
}

double Tshirt_C_Realization::get_latest_flux_giuh_runoff() {
    return fluxes.empty() ? 0.0 : fluxes.back()->giuh_runoff_m;
}

double Tshirt_C_Realization::get_latest_flux_lateral_flow() {
    return fluxes.empty() ? 0.0 : fluxes.back()->nash_lateral_runoff_m;
}

double Tshirt_C_Realization::get_latest_flux_surface_runoff() {
    return fluxes.empty() ? 0.0 : fluxes.back()->Schaake_output_runoff_m;
}

double Tshirt_C_Realization::get_latest_flux_total_discharge() {
    return fluxes.empty() ? 0.0 : fluxes.back()->Qout_m;
}

/**
 * Get the number of output data variables made available from the calculations for enumerated time steps.
 *
 * @return The number of output data variables made available from the calculations for enumerated time steps.
 */
int Tshirt_C_Realization::get_output_item_count() {
    return get_output_var_names().size();
}

/**
 * Get a header line appropriate for a file made up of entries from this type's implementation of
 * ``get_output_line_for_timestep``.
 *
 * Note that like the output generating function, this line does not include anything for time step.
 *
 * @return An appropriate header line for this type.
 */
std::string Tshirt_C_Realization::get_output_header_line(std::string delimiter) {
    return boost::algorithm::join(get_output_header_fields(), delimiter);
}

/**
 * Get the values making up the header line from get_output_header_line(), but organized as a vector of strings.
 *
 * @return The values making up the header line from get_output_header_line() organized as a vector.
 */
const std::vector<std::string>& Tshirt_C_Realization::get_output_header_fields() {
    return OUTPUT_HEADER_FIELDS;
}

/**
 * Get a delimited string with all the output variable values for the given time step.
 *
 * This method is useful for preparing calculated data in a representation useful for output files, such as
 * CSV files.
 *
 * The resulting string contains only the calculated output values for the time step, and not the time step
 * index itself.
 *
 * An empty string is returned if the time step value is not in the range of valid time steps for which there
 * are calculated values for all variables.
 *
 * The default delimiter is a comma.
 *
 * @param timestep The time step for which data is desired.
 * @return A delimited string with all the output variable values for the given time step.
 */
std::string Tshirt_C_Realization::get_output_line_for_timestep(int timestep, std::string delimiter) {
    // Check if the timestep is in bounds for the fluxes vector, and handle case when it isn't
    if (timestep >= fluxes.size() || fluxes[timestep] == nullptr) {
        return "";
    }
    std::string output_str;
    tshirt_c_result_fluxes flux_for_timestep = *fluxes[timestep];
    for (const std::string& name : get_output_var_names()) {
        // Get a lambda that takes a fluxes struct and returns the right (double) member value from it from the name
        function<double(tshirt_c_result_fluxes)> get_val_func = get_output_var_flux_extraction_func(name);
        double output_var_value = get_val_func(flux_for_timestep);
        output_str += output_str.empty() ? std::to_string(output_var_value) : "," + std::to_string(output_var_value);
    }
    return output_str;
}

/**
 * Get the names of the output data variables that are available from calculations for enumerated time steps.
 *
 * @return The names of the output data variables that are available from calculations for enumerated time steps.
 */
const std::vector<std::string> &Tshirt_C_Realization::get_output_var_names() {
    return OUTPUT_VARIABLE_NAMES;
}

// TODO: don't care for this, as it could have the reference locations accidentally altered (also, raw pointer => bad)
//@robertbartel is this TODO resolved with these changes?
const std::vector<std::string>& Tshirt_C_Realization::get_required_parameters() {
    return REQUIRED_PARAMETERS;
}

/**
 * Execute the backing model formulation for the given time step, where it is of the specified size, and
 * return the total discharge.
 *
 * Any inputs and additional parameters must be made available as instance members.
 *
 * Types should clearly document the details of their particular response output.
 *
 * @param t_index The index of the time step for which to run model calculations.
 * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
 * @return The total discharge of the model for this time step.
 */
double Tshirt_C_Realization::get_response(time_step_t t_index, time_step_t t_delta_s) {
    // TODO: check that t_delta_s is of approprate size

    // TODO: add some logic for ensuring the right precip data is gathered.  This will need to include considerations
    //  for the cases when the dt is larger, smaller and equal to a specific, discrete data point available in the
    //  forcing.  It may actually belong within the forcing object.

    // TODO: it also needs to account for getting the right precip data point (i.e., t_index may not be "next")
    double precip = this->legacy_forcing.get_next_hourly_precipitation_meters_per_second();
    int response_result = run_formulation_for_timestep(precip, t_delta_s);
    // TODO: check t_index is the next expected time step to be calculated

    return fluxes.back()->Qout_m;
}

function<double(tshirt_c_result_fluxes)>
Tshirt_C_Realization::get_output_var_flux_extraction_func(const std::string& var_name) {
    // TODO: think about making this a lazily initialized member map
    if (var_name == OUT_VAR_BASE_FLOW) {
        return [](tshirt_c_result_fluxes flux) { return flux.flux_from_deep_gw_to_chan_m;};
    }
    else if (var_name == OUT_VAR_GIUH_RUNOFF) {
        return [](tshirt_c_result_fluxes flux) { return flux.giuh_runoff_m;};
    }
    else if (var_name == OUT_VAR_LATERAL_FLOW) {
        return [](tshirt_c_result_fluxes flux) { return flux.nash_lateral_runoff_m;};
    }
    else if (var_name == OUT_VAR_RAINFALL) {
        return [](tshirt_c_result_fluxes flux) { return flux.timestep_rainfall_input_m;};
    }
    else if (var_name == OUT_VAR_SURFACE_RUNOFF) {
        return [](tshirt_c_result_fluxes flux) { return flux.Schaake_output_runoff_m;};
    }
    else if (var_name == OUT_VAR_TOTAL_DISCHARGE) {
        return [](tshirt_c_result_fluxes flux) { return flux.Qout_m;};
    }
    else {
        throw std::invalid_argument("Cannot get values for unrecognized variable name " + var_name);
    }
}

/**
 * Get a copy of the values for the given output variable at all available time steps.
 *
 * @param name
 * @return A vector containing copies of the output value of the variable, indexed by time step.
 */
std::vector<double> Tshirt_C_Realization::get_value(const std::string& name) {
    // Generate a lambda that takes a fluxes struct and returns the right (double) member value from it from the name
    function<double(tshirt_c_result_fluxes)> get_val_func = get_output_var_flux_extraction_func(name);
    // Then, assuming we don't bail, use the lambda to build the result array
    std::vector<double> outputs = std::vector<double>(fluxes.size());
    for (int i = 0; i < fluxes.size(); ++i) {
        outputs[i] = get_val_func(*fluxes[i]);
    }
    return outputs;
}

/**
 * Run model formulation calculations for the next time step using the given input flux value in meters per second.
 *
 * The backing model works on a collection of time steps by receiving an associated collection of input fluxes.  This
 * is implemented by putting this input flux in a single-value vector and using it as the arg to a nested call to
 * ``run_formulation_for_timesteps``, returning that result code.
 *
 * @param input_flux Input flux (typically expected to be just precipitation) in meters per second.
 * @param t_delta_s The size of the time step in seconds
 * @return The result code from the execution of the model time step calculations.
 */
int Tshirt_C_Realization::run_formulation_for_timestep(double input_flux, time_step_t t_delta_s) {
    std::vector<double> input_flux_in_vector{input_flux};
    return run_formulation_for_timesteps({input_flux}, {t_delta_s});
}

/**
 * Run model formulation calculations for a series of time steps using the given collection of input flux values
 * in meters per second.
 *
 * @param input_fluxes Ordered, per-time-step input flux (typically expected to be just precipitation) in meters
 * per second.
 * @param t_delta_s The sizes of each of the time steps in seconds
 * @return The result code from the execution of the model time step calculations.
 */
int Tshirt_C_Realization::run_formulation_for_timesteps(std::vector<double> input_fluxes,
                                                        std::vector<time_step_t> t_deltas_s) {
    int num_timesteps = (int) input_fluxes.size();

    // FIXME: verify this needs to be independent like this
    // FIXME: also make sure it shouldn't be parameterized somewhere, rather than hard-coded
    double assumed_near_channel_water_table_slope = 0.01;

    // TODO: need some kind of guarantee that the vectors won't be resized and current buffer arrays won't be changed or
    //  removed (before the below "run" call finishes)
    double* giuh_ordinates = &giuh_cdf_ordinates[0];
    double* giuh_runoff_queue = &giuh_runoff_queue_per_timestep[0];

    //aorc_forcing_data empty_forcing[num_timesteps];
    aorc_forcing_data empty_forcing[1];

    // Since the input_fluxes param values are in meters per second, this will need to do some conversions to what gets
    // passed to Tshirt_C's run(), which expects meters per time step.
    std::vector<double> input_meters_per_time_step(input_fluxes.size());
    for (int i = 0; i < input_fluxes.size(); ++i) {
        input_meters_per_time_step[i] = input_fluxes[i] * t_deltas_s[i];
    }
    double* input_as_array = &input_meters_per_time_step[0];

    tshirt_c_result_fluxes output_fluxes_as_array[num_timesteps];

    // use this to sanity check the fluxes got added as expected to array
    int num_added_fluxes = 0;

    int result = run(c_soil_params,
                     groundwater_conceptual_reservoir,
                     soil_conceptual_reservoir,
                     num_timesteps,
                     giuh_ordinates,
                     (int)giuh_cdf_ordinates.size(),
                     giuh_runoff_queue,
                     params->alpha_fc,
                     assumed_near_channel_water_table_slope,
                     params->Cschaake,
                     params->Klf,
                     params->Kn,
                     params->nash_n,
                     &nash_storage[0],
                     FALSE,
                     empty_forcing,
                     input_as_array,
                     num_added_fluxes,
                     output_fluxes_as_array);

    // Move fluxes over to member data structure
    for (int i = 0; i < num_added_fluxes && i < num_timesteps; i++) {
        fluxes.push_back(std::make_shared<tshirt_c_result_fluxes>(output_fluxes_as_array[i]));
    }

    return result;
}

void Tshirt_C_Realization::init_ground_water_reservoir(double storage, bool storage_values_are_ratios) {
    //  Populate the groundwater conceptual reservoir data structure
    //-----------------------------------------------------------------------
    // one outlet, 0.0 threshold, nonlinear and exponential as in NWM
    groundwater_conceptual_reservoir.is_exponential=TRUE;         // set this true TRUE to use the exponential form of the discharge equation
    groundwater_conceptual_reservoir.storage_max_m=params->max_groundwater_storage_meters;

    groundwater_conceptual_reservoir.coeff_primary=params->Cgw;           // per h
    groundwater_conceptual_reservoir.exponent_primary=params->expon;       // linear iff 1.0, non-linear iff > 1.0
    groundwater_conceptual_reservoir.storage_threshold_primary_m=0.0;     // 0.0 means no threshold applied

    groundwater_conceptual_reservoir.storage_threshold_secondary_m=0.0;   // 0.0 means no threshold applied
    groundwater_conceptual_reservoir.coeff_secondary=0.0;                 // 0.0 means that secondary outlet is not applied
    groundwater_conceptual_reservoir.exponent_secondary=1.0;              // linear

    groundwater_conceptual_reservoir.storage_m = init_reservoir_storage(storage_values_are_ratios,
                                                                        storage,
                                                                        groundwater_conceptual_reservoir.storage_max_m);
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

void Tshirt_C_Realization::init_soil_reservoir(double storage, bool storage_values_are_ratios) {

    double trigger_z_m = 0.5;   // distance from bottom of soil column to the center of the lowest discretization

    // calculate the activation storage for the secondary lateral flow outlet in the soil nonlinear reservoir.
    // following the method in the NWM/t-shirt parameter equivalence document, assuming field capacity soil
    // suction pressure = 1/3 atm= field_capacity_atm_press_fraction * atm_press_Pa.

    // equation 3 from NWM/t-shirt parameter equivalence document
    // This may need to be changed as follows later, but for now, use the constant value
    //double H_water_table_m = params->alpha_fc * forcing.get_AORC_PRES_surface_Pa() / WATER_SPECIFIC_WEIGHT;
    double H_water_table_m = params->alpha_fc * STANDARD_ATMOSPHERIC_PRESSURE_PASCALS / WATER_SPECIFIC_WEIGHT;


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
    // TODO: should this get parameterized somehow?
    soil_conceptual_reservoir.coeff_primary = c_soil_params.satdk * c_soil_params.slop * 3600.0; // m per h
    soil_conceptual_reservoir.exponent_primary = 1.0;      // 1.0=linear
    soil_conceptual_reservoir.storage_threshold_primary_m = field_capacity_storage_threshold_m;
    // lateral flow parameters --------------------------------------------------------
    soil_conceptual_reservoir.coeff_secondary = params->Klf;  // 0.0 to deactiv. else =lateral_flow_linear_reservoir_constant;   // m per h
    soil_conceptual_reservoir.exponent_secondary = 1.0;   // 1.0=linear
    soil_conceptual_reservoir.storage_threshold_secondary_m = lateral_flow_threshold_storage_m;

    soil_conceptual_reservoir.storage_m = init_reservoir_storage(storage_values_are_ratios,
                                                                 storage,
                                                                 soil_conceptual_reservoir.storage_max_m);

}

void Tshirt_C_Realization::sync_c_storage_params() {
    // Convert params to struct for C-impl
    c_soil_params.D = params->depth;
    c_soil_params.bb = params->b;
    c_soil_params.mult = params->multiplier;
    c_soil_params.satdk = params->satdk;
    c_soil_params.satpsi = params->satpsi;
    c_soil_params.slop = params->slope;
    c_soil_params.smcmax = params->maxsmc;
    c_soil_params.wltsmc = params->wltsmc;
}
