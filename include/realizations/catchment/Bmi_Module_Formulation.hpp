#ifndef NGEN_BMI_MODULE_FORMULATION_H
#define NGEN_BMI_MODULE_FORMULATION_H

#include <utility>
#include <memory>
#include "Bmi_Formulation.hpp"
#include "EtCalcProperty.hpp"
#include "EtCombinationMethod.hpp"
#include "WrappedForcingProvider.hpp"
#include "Bmi_C_Adapter.hpp"
#include <AorcForcing.hpp>
#include <DataProvider.hpp>
#include <UnitsHelper.hpp>
#include "bmi_utilities.hpp"

using data_access::MEAN;
using data_access::SUM;


// Forward declaration to provide access to protected items in testing
class Bmi_Formulation_Test;
class Bmi_Multi_Formulation_Test;
class Bmi_C_Formulation_Test;
class Bmi_Cpp_Formulation_Test;
class Bmi_C_Cfe_IT;
class Bmi_Cpp_Multi_Array_Test;

namespace realization {

    /**
     * Abstraction of a formulation with a single backing model object that implements the BMI.
     *
     * @tparam M The type for the backing BMI model object.
     */
    template <class M>
    class Bmi_Module_Formulation : public Bmi_Formulation {

    public:

        /**
         * Minimal constructor for objects initialize using the Formulation_Manager and subsequent calls to
         * ``create_formulation``.
         *
         * @param id
         * @param forcing_provider
         * @param output_stream
         */
        Bmi_Module_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing_provider, utils::StreamHandler output_stream)
                : Bmi_Formulation(std::move(id), forcing_provider, output_stream) { };

        virtual ~Bmi_Module_Formulation() {};

        /**
         * Perform (potential) ET calculation, getting input params from instance's backing model as needed.
         *
         * Function uses forcings from the "current" time step.
         *
         * @return Calculated ET or potential ET, or ``0.0`` if required parameters could not be obtained.
         */
        double calc_et() override {
            long timestep = 3600;
            time_t model_time = convert_model_time(get_bmi_model()->GetCurrentTime()) + get_bmi_model_start_time_forcing_offset_s();

            struct AORC_data raw_aorc;
            raw_aorc.TMP_2maboveground_K = forcing->get_value(BMIDataSelector(CSDMS_STD_NAME_SURFACE_TEMP, model_time, timestep, "K"), MEAN);
            raw_aorc.SPFH_2maboveground_kg_per_kg = forcing->get_value(BMIDataSelector(NGEN_STD_NAME_SPECIFIC_HUMIDITY, model_time, timestep, "kg/kg"), MEAN);
            raw_aorc.PRES_surface_Pa = forcing->get_value(BMIDataSelector(CSDMS_STD_NAME_SURFACE_AIR_PRESSURE, model_time, timestep, "Pa"), MEAN);
            raw_aorc.UGRD_10maboveground_meters_per_second = forcing->get_value(BMIDataSelector(CSDMS_STD_NAME_WIND_U_X, model_time, timestep, "m s^-1"), MEAN);
            raw_aorc.VGRD_10maboveground_meters_per_second = forcing->get_value(BMIDataSelector(CSDMS_STD_NAME_WIND_V_Y, model_time, timestep, "m s^-1"), MEAN);
            raw_aorc.DSWRF_surface_W_per_meters_squared = forcing->get_value(BMIDataSelector(CSDMS_STD_NAME_SOLAR_SHORTWAVE, model_time, timestep, "W m^-2"), SUM);
            raw_aorc.DLWRF_surface_W_per_meters_squared = forcing->get_value(BMIDataSelector(CSDMS_STD_NAME_SOLAR_LONGWAVE, model_time, timestep, "W m^-2"), SUM);
            raw_aorc.APCP_surface_kg_per_meters_squared = forcing->get_value(BMIDataSelector(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, model_time, timestep, "kg m^-2"), SUM);

            return calc_et(raw_aorc);
        }

        double calc_et(struct AORC_data raw_aorc) {
            // TODO: fix both not checking this, and how the way it's set currently doesn't fit with these kernels
            //if (!is_et_params_set()) {
            //    throw std::runtime_error("Can't calculate ET for BMI model without ET params being set");
            //}
            // TODO: the Et_Accountable and Et_Aware interfaces need to be overhauled, and these things need to all be
            //  moved under whatever type is used for "et_parameters"

            /*
             * ********************************************************************************************************
             * Logic based largely on et_wrapper_function() in EtWrapperFunction.hpp, and a little from et_setup() in
             * EtSetParams.hpp.
             * ********************************************************************************************************
             */

            // TODO: Putting made-up values here, but need to figure out where to actually get these from (these taken
            //  from EtSetParams.hpp, with comments from relevant lines quoted)
            double made_up_canopy_resistance_sec_per_m = 50.0; // "from plant growth model"
            double made_up_water_temperature_C = 15.5; // "from soil or lake thermal model"
            double made_up_ground_heat_flux_W_per_sq_m = -10.0; /* 0.0; */ // -40 "from soil thermal model. Negative denotes downward."
            double made_up_vegetation_height_m = 0.12; // "used for unit test of aerodynamic resistance used in Penman Monteith method."
            double made_up_zero_plane_displacement_height_m = 0.0003; /* 0.1; */  // "0.03 cm for unit testing"
            double made_up_surface_longwave_emissivity = 1.0; // "this is 1.0 for granular surfaces, maybe 0.97 for water"
            double made_up_surface_shortwave_albedo = 0.22; /* 0.0; */ // "this is a function of solar elev. angle for most surfaces."
            double made_up_surface_skin_temperature_C = 12.0; /* 20.0; */  // "from soil thermal model or vegetation model"
            double made_up_momentum_transfer_roughness_length_m = 0.0; /* 1.0; */ // zero means that default values will be used in routine.
            double made_up_heat_transfer_roughness_length_m = 0.0; /* 1.0; */ // zero means that default values will be used in routine.

            // TODO: for now, hard-code these things
            struct et::evapotranspiration_options et_options;
            et_options.yes_aorc = TRUE;
            et_options.use_energy_balance_method   = FALSE;
            et_options.use_aerodynamic_method      = FALSE;
            et_options.use_combination_method      = TRUE;
            et_options.use_priestley_taylor_method = FALSE;
            et_options.use_penman_monteith_method  = FALSE;

            struct et::evapotranspiration_forcing et_forcing;
            et_forcing.air_temperature_C = raw_aorc.TMP_2maboveground_K - TK;  // gotta convert it to C
            // TODO: is it correct to do this (copied from other code)
            et_forcing.relative_humidity_percent = -99.9; // this negative number means use specific humidity
            et_forcing.specific_humidity_2m_kg_per_kg = raw_aorc.SPFH_2maboveground_kg_per_kg;
            et_forcing.air_pressure_Pa = raw_aorc.PRES_surface_Pa;
            et_forcing.wind_speed_m_per_s = hypot(raw_aorc.UGRD_10maboveground_meters_per_second,
                                                  raw_aorc.VGRD_10maboveground_meters_per_second);

            et_forcing.canopy_resistance_sec_per_m = made_up_canopy_resistance_sec_per_m;
            et_forcing.water_temperature_C = made_up_water_temperature_C;
            et_forcing.ground_heat_flux_W_per_sq_m = made_up_ground_heat_flux_W_per_sq_m;

            struct et::evapotranspiration_params et_params;
            et_params.vegetation_height_m = made_up_vegetation_height_m;
            et_params.zero_plane_displacement_height_m = made_up_zero_plane_displacement_height_m;
            et_params.momentum_transfer_roughness_length_m = made_up_momentum_transfer_roughness_length_m;
            et_params.heat_transfer_roughness_length_m = made_up_heat_transfer_roughness_length_m;

            // AORC uses wind speeds from 10m.  Must convert to 2m.
            if (et_options.yes_aorc == TRUE) {
                // wind speed was measured at 10.0 m height, so we need to calculate the wind speed at 2.0m
                double numerator = log(2.0 / et_params.zero_plane_displacement_height_m);
                double denominator = log(10.0 / et_params.zero_plane_displacement_height_m);
                // this is the 2 m value
                et_forcing.wind_speed_m_per_s = et_forcing.wind_speed_m_per_s * numerator / denominator;
            }
            et_params.wind_speed_measurement_height_m = 2.0;

            // surface radiation parameter values that are function of land cover. Must be assigned from land cover type
            //----------------------------------------------------------------------------------------------------------
            struct et::surface_radiation_params surf_rad_params;
            // this is 1.0 for granular surfaces, maybe 0.97 for water
            surf_rad_params.surface_longwave_emissivity = made_up_surface_longwave_emissivity;
            // this is a function of solar elev. angle for most surfaces.
            surf_rad_params.surface_shortwave_albedo = made_up_surface_shortwave_albedo;

            struct et::surface_radiation_forcing  surf_rad_forcing;
            if (et_options.yes_aorc==TRUE) {
                et_params.humidity_measurement_height_m = 2.0;
                // transfer aorc forcing data into our data structure for surface radiation calculations
                surf_rad_forcing.incoming_shortwave_radiation_W_per_sq_m = raw_aorc.DSWRF_surface_W_per_meters_squared;
                surf_rad_forcing.incoming_longwave_radiation_W_per_sq_m = raw_aorc.DLWRF_surface_W_per_meters_squared;
                surf_rad_forcing.air_temperature_C = et_forcing.air_temperature_C;
                // compute relative humidity from specific humidity..
                double saturation_vapor_pressure_Pa, actual_vapor_pressure_Pa;
                saturation_vapor_pressure_Pa = et::calc_air_saturation_vapor_pressure_Pa(surf_rad_forcing.air_temperature_C);
                actual_vapor_pressure_Pa = raw_aorc.SPFH_2maboveground_kg_per_kg * raw_aorc.PRES_surface_Pa / 0.622;
                surf_rad_forcing.relative_humidity_percent = 100.0 * actual_vapor_pressure_Pa / saturation_vapor_pressure_Pa;
                // sanity check the resulting value.  Should be less than 100%.  Sometimes air can be supersaturated.
                if (100.0 < surf_rad_forcing.relative_humidity_percent) {
                    surf_rad_forcing.relative_humidity_percent = 99.0;
                }
            }
            /* Shouldn't need this for now, but might later
            else {
                et_params.humidity_measurement_height_m = set_et_params->humidity_measurement_height_m;

                // these values are needed if we don't have incoming longwave radiation measurements.
                surf_rad_forcing.incoming_shortwave_radiation_W_per_sq_m \
                     = surface_rad_forcing->incoming_shortwave_radiation_W_per_sq_m; \
                     // must come from somewhere
                surf_rad_forcing.incoming_longwave_radiation_W_per_sq_m \
                     = surface_rad_forcing->incoming_longwave_radiation_W_per_sq_m; \
                     // this huge negative value tells to calc.
                surf_rad_forcing.air_temperature_C \
                     = surface_rad_forcing->air_temperature_C;  // from some forcing data file
                surf_rad_forcing.relative_humidity_percent \
                     = surface_rad_forcing->relative_humidity_percent;  // from some forcing data file
                surf_rad_forcing.ambient_temperature_lapse_rate_deg_C_per_km \
                     = surface_rad_forcing->ambient_temperature_lapse_rate_deg_C_per_km; \
                     // ICAO standard atmosphere lapse rate
                surf_rad_forcing.cloud_cover_fraction = surface_rad_forcing->cloud_cover_fraction;  // from some forcing data file
                surf_rad_forcing.cloud_base_height_m  = surface_rad_forcing->cloud_base_height_m;   // assumed 2500 ft.
            }
             */

            // Surface radiation forcing parameter values that must come from other models
            //--------------------------------------------------------------------------------------------------------
            surf_rad_forcing.surface_skin_temperature_C = made_up_surface_skin_temperature_C;
            //Compute net_radiation et forcing value
            et_forcing.net_radiation_W_per_sq_m=et::calculate_net_radiation_W_per_sq_m(&et_options,&surf_rad_params, 
                                                                         &surf_rad_forcing);
            // TODO: and this just needs to be created with nothing set?
            struct et::intermediate_vars inter_vars;

            // TODO: there will need to be a way to establish in config which method should be used (especially
            //  potential versus actual)
            return et::combined::evapotranspiration_combination_method(&et_options,&et_params,&et_forcing,&inter_vars);
        }

        struct et::aorc_forcing_data convert_aorc_structs(struct AORC_data &aorc_data) {
            struct et::aorc_forcing_data et_forcing;
            et_forcing.precip_kg_per_m2 = (float)aorc_data.APCP_surface_kg_per_meters_squared;
            et_forcing.incoming_longwave_W_per_m2 = (float)aorc_data.DLWRF_surface_W_per_meters_squared;
            et_forcing.incoming_shortwave_W_per_m2 = (float)aorc_data.DSWRF_surface_W_per_meters_squared;
            et_forcing.surface_pressure_Pa = (float)aorc_data.PRES_surface_Pa;
            et_forcing.specific_humidity_2m_kg_per_kg = (float)aorc_data.SPFH_2maboveground_kg_per_kg;
            et_forcing.air_temperature_2m_K = (float)aorc_data.TMP_2maboveground_K;
            et_forcing.u_wind_speed_10m_m_per_s = (float)aorc_data.UGRD_10maboveground_meters_per_second;
            et_forcing.v_wind_speed_10m_m_per_s = (float)aorc_data.VGRD_10maboveground_meters_per_second;
            // TODO: these may or may not need to change in this particular function (though probably need to be set to
            //  other values elsewhere)
            et_forcing.latitude = 0.0;
            et_forcing.longitude = 0.0;
            et_forcing.time = 0.0;
            return et_forcing;
        }

        void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override {
            geojson::PropertyMap options = this->interpret_parameters(config, global);
            inner_create_formulation(options, false);
        }

        void create_formulation(geojson::PropertyMap properties) override {
            inner_create_formulation(properties, true);
        }

        /**
         * Get the collection of forcing output property names this instance can provide.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * For this type, this is the collection of BMI output variables, plus any aliases included in the formulation
         * config's output variable mapping.
         *
         * @return The collection of forcing output property names this instance can provide.
         * @see ForcingProvider
         */
        const vector<std::string> &get_available_forcing_outputs() {
            if (is_model_initialized() && available_forcings.empty()) {
                for (const std::string &output_var_name : get_bmi_model()->GetOutputVarNames()) {
                    available_forcings.push_back(output_var_name);
                    if (bmi_var_names_map.find(output_var_name) != bmi_var_names_map.end())
                        available_forcings.push_back(bmi_var_names_map[output_var_name]);
                }
                available_forcings.emplace_back(NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP);
                available_forcings.emplace_back(CSDMS_STD_NAME_POTENTIAL_ET);
            }
            return available_forcings;
        }

        inline const vector<std::string> &get_avaliable_variable_names() override {
            return get_available_forcing_outputs();
        }

        /**
         * Get the inclusive beginning of the period of time over which this instance can provide data for this forcing.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * @return The inclusive beginning of the period of time over which this instance can provide this data.
         */
        time_t get_forcing_output_time_begin(const std::string &forcing_name) {
            // TODO: come back and implement if actually necessary for this type; for now don't use
            throw runtime_error("Bmi_Modular_Formulation does not yet implement get_forcing_output_time_begin");
        }

        /**
         * Get the inclusive beginning of the period of time over which this instance can provide data for this forcing.
         *
         * This is part of the @ref DataProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * @return The inclusive beginning of the period of time over which this instance can provide this data.
         */
        long get_data_start_time() override
        {
            return this->get_bmi_model()->GetStartTime();
        }

        /**
         * Get the exclusive ending of the period of time over which this instance can provide data for this forcing.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * @return The exclusive ending of the period of time over which this instance can provide this data.
         */
        [[deprecated]]
        time_t get_forcing_output_time_end(const std::string &output_name) {
            // TODO: come back and implement if actually necessary for this type; for now don't use
            throw runtime_error("Bmi_Module_Formulation does not yet implement get_forcing_output_time_end");
        }

        long get_data_stop_time() override {
            // TODO: come back and implement if actually necessary for this type; for now don't use
            throw runtime_error("Bmi_Module_Formulation does not yet implement get_data_stop_time");
        }

        long record_duration() override {
            throw runtime_error("Bmi_Module_Formulation does not yet implement record_duration");
        }

        /**
         * Get the current time for the backing BMI model in its native format and units.
         *
         * @return The current time for the backing BMI model in its native format and units.
         */
        const double get_model_current_time() override {
            return get_bmi_model()->GetCurrentTime();
        }

        /**
         * Get the end time for the backing BMI model in its native format and units.
         *
         * @return The end time for the backing BMI model in its native format and units.
         */
        const double get_model_end_time() override {
            return get_bmi_model()->GetEndTime();
        }

        const vector<std::string> &get_required_parameters() override {
            return REQUIRED_PARAMETERS;
        }

        /**
         * When possible, translate a variable name for a BMI model to an internally recognized name.
         *
         * Translate some BMI variable name to something recognized in some internal context for use within NGen.  Do
         * this according to the map of variable names supplied in the external formulation config.  If no mapping for
         * the given variable name was configured, return the variable name itself.
         *
         * For example, perhaps a BMI model has the input variable "RAIN_RATE."  Configuring this variable name to map
         * to "precip_rate" will allow the formulation to understand that this particular forcing field should be used
         * to set the model's "RAIN_RATE" variable.
         *
         * @param model_var_name The BMI variable name to translate so its purpose is recognized internally.
         * @return Either the internal equivalent variable name, or the provided name if there is not a mapping entry.
         */
        const std::string &get_config_mapped_variable_name(const std::string &model_var_name) override {
            // TODO: need to introduce validation elsewhere that all mapped names are valid AORC field constants.
            if (bmi_var_names_map.find(model_var_name) != bmi_var_names_map.end())
                return bmi_var_names_map[model_var_name];
            else
                return model_var_name;
        }

        /**
         * Get the index of the forcing time step that contains the given point in time.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * An @ref std::out_of_range exception should be thrown if the time is not in any time step.
         *
         * @param epoch_time The point in time, as a seconds-based epoch time.
         * @return The index of the forcing time step that contains the given point in time.
         * @throws std::out_of_range If the given point is not in any time step.
         */
        size_t get_ts_index_for_time(const time_t &epoch_time) override {
            // TODO: come back and implement if actually necessary for this type; for now don't use
            throw runtime_error("Bmi_Singular_Formulation does not yet implement get_ts_index_for_time");
        }

        /**
         * @brief Get the 1D values of a forcing property for an arbitrary time period, converting units if needed.
         * 
         * @param output_name The name of the forcing property of interest.
         * @param init_time The epoch time (in seconds) of the start of the time period.
         * @param duration_s The length of the time period, in seconds.
         * @param output_units The expected units of the desired output value.
         * @return std::vector<double> The 1D values of the forcing property for the described time period, with units converted if needed.
         * @throws std::out_of_range If data for the time period is not available.
         * @throws std::runtime_error output_name is not one of the available outputs of this provider instance.
         */
        std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m=SUM) override
        {
            std::string output_name = selector.get_variable_name();
            time_t init_time = selector.get_init_time();
            long duration_s = selector.get_duration_secs();
            std::string output_units = selector.get_output_units();

            // First make sure this is an available output
            const std::vector<std::string> forcing_outputs = get_available_forcing_outputs();
            if (std::find(forcing_outputs.begin(), forcing_outputs.end(), output_name) == forcing_outputs.end()) {
                throw runtime_error(get_formulation_type() + " received invalid output forcing name " + output_name);
            }
            // TODO: do this, or something better, later; right now, just assume anything using this as a provider is
            //  consistent with times
            /*
            if (last_model_response_delta == 0 && last_model_response_start_time == 0) {
                throw runtime_error(get_formulation_type() + " does not properly set output time validity ranges "
                                                             "needed to provide outputs as forcings");
            }
            */

            // check if output is available from BMI
            std::string bmi_var_name;
            get_bmi_output_var_name(output_name, bmi_var_name);

            if( !bmi_var_name.empty() )
            {
                auto model = get_bmi_model().get();
                //Get vector of double values for variable
                //The return type of the vector here dependent on what
                //needs to use it.  For other BMI moudles, that is runtime dependent
                //on the type of the requesting module 
                auto values = models::bmi::GetValue<double>(*model, bmi_var_name);

                // Convert units
                std::string native_units = get_bmi_model()->GetVarUnits(bmi_var_name);
                try {
                    UnitsHelper::convert_values(native_units, values.data(), output_units, values.data(), values.size());
                    return values;
                }
                catch (const std::runtime_error& e){
                    std::cerr<<"Unit conversion error: "<<std::endl<<e.what()<<std::endl<<"Returning unconverted value!"<<std::endl;
                    return values;
                }
            }
            //Fall back to any internal providers as a last resort.
            return check_internal_providers<double>(output_name);
        }

        /**
         * Get the value of a forcing property for an arbitrary time period, converting units if needed.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * An @ref std::out_of_range exception should be thrown if the data for the time period is not available.
         *
         * @param output_name The name of the forcing property of interest.
         * @param init_time_epoch The epoch time (in seconds) of the start of the time period.
         * @param duration_seconds The length of the time period, in seconds.
         * @param output_units The expected units of the desired output value.
         * @return The value of the forcing property for the described time period, with units converted if needed.
         * @throws std::out_of_range If data for the time period is not available.
         * @see ForcingProvider::get_value
         */
        double get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override
        {
            std::string output_name = selector.get_variable_name();
            time_t init_time = selector.get_init_time();
            long duration_s = selector.get_duration_secs();
            std::string output_units = selector.get_output_units();

            // First make sure this is an available output
            const std::vector<std::string> forcing_outputs = get_available_forcing_outputs();
            if (std::find(forcing_outputs.begin(), forcing_outputs.end(), output_name) == forcing_outputs.end()) {
                throw runtime_error(get_formulation_type() + " received invalid output forcing name " + output_name);
            }
            // TODO: do this, or something better, later; right now, just assume anything using this as a provider is
            //  consistent with times
            /*
            if (last_model_response_delta == 0 && last_model_response_start_time == 0) {
                throw runtime_error(get_formulation_type() + " does not properly set output time validity ranges "
                                                             "needed to provide outputs as forcings");
            }
            */

            // check if output is available from BMI
            std::string bmi_var_name;
            get_bmi_output_var_name(output_name, bmi_var_name);
            
            if( !bmi_var_name.empty() )
            {
                //Get forcing value from BMI variable
                double value = get_var_value_as_double(bmi_var_name);

                // Convert units
                std::string native_units = get_bmi_model()->GetVarUnits(bmi_var_name);
                try {
                    return UnitsHelper::get_converted_value(native_units, value, output_units);
                }
                catch (const std::runtime_error& e){
                    std::cerr<<"Unit conversion error: "<<std::endl<<e.what()<<std::endl<<"Returning unconverted value!"<<std::endl;
                    return value;
                }
            }
            //Fall back to any internal providers as a last resort.
            return check_internal_providers<double>(output_name)[0];
        }

        bool is_bmi_input_variable(const std::string &var_name) override {
           return is_var_name_in_collection(get_bmi_input_variables(), var_name);
        }

        bool is_bmi_output_variable(const std::string &var_name) override {
            return is_var_name_in_collection(get_bmi_output_variables(), var_name);
        }

        inline bool is_var_name_in_collection(const std::vector<std::string> &all_names, const std::string &var_name) {
            return std::count(all_names.begin(), all_names.end(), var_name) > 0;
        }

        /**
         * Get whether a property's per-time-step values are each an aggregate sum over the entire time step.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * Certain properties, like rain fall, are aggregated sums over an entire time step.  Others, such as pressure,
         * are not such sums and instead something else like an instantaneous reading or an average value.
         *
         * It may be the case that forcing data is needed for some discretization different than the forcing time step.
         * This aspect must be known in such cases to perform the appropriate value interpolation.
         *
         * For instances of this type, all output forcings fall under this category.
         *
         * @param name The name of the forcing property for which the current value is desired.
         * @return Whether the property's value is an aggregate sum, which is always ``true`` for this type.
         */
        bool is_property_sum_over_time_step(const std::string& name) override {
            // TODO: verify with some kind of proof that "always true" is appropriate
            return true;
        }

        const vector<string> get_bmi_input_variables() override {
            return get_bmi_model()->GetInputVarNames();
        }

        const vector<string> get_bmi_output_variables() override {
            return get_bmi_model()->GetOutputVarNames();
        }

    protected:

        /**
         * @brief Get correct BMI variable name, which may be the output or something mapped to this output.
         *
         * @param name
         * @param bmi_var_name
         */
        inline void get_bmi_output_var_name(const std::string &name, std::string &bmi_var_name)
        {
            //check standard output names first
            std::vector<std::string> output_names = get_bmi_model()->GetOutputVarNames();
            if (std::find(output_names.begin(), output_names.end(), name) != output_names.end()) {
                bmi_var_name = name;
            }
            else
            {
                //check mapped names
                std::string mapped_name;
                for (auto & iter : bmi_var_names_map) {
                    if (iter.second == name) {
                        mapped_name = iter.first;
                        break;
                    }
                }
                //ensure mapped name maps to an output variable, see GH #393 =)
                if (std::find(output_names.begin(), output_names.end(), mapped_name) != output_names.end()){
                    bmi_var_name = mapped_name;
                }
                //else not an output variable
            }
        }

        /**
         * @brief Check for implementation of internal calculators/data for a given requsted output_name
         *
         * @tparam T the type expected to be returned for the value of @p output_name
         * @param output_name
         * @return T
         * @throws std::runtime_error If no known value or function for @p output_name
         */
        template <typename T>
        std::vector<T> check_internal_providers(std::string output_name){
            // Only use the internal et_calc() if this formulation (or possibly multi-formulation)
            // does not know how to supply potential et
            if (output_name == NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP || output_name == CSDMS_STD_NAME_POTENTIAL_ET) {
                return std::vector<T>( 1, calc_et() );
            }
            //Note, when called via get_value, this is unlikely to throw since a pre-check on available names is done
            //in that function.
            throw runtime_error(get_formulation_type() + " received invalid output forcing name " + output_name);
        }

        /**
         * Construct model and its shared pointer, potentially supplying input variable values from config.
         *
         * Construct a model (and a shared pointer to it), checking whether additional input variable values are present
         * in the configuration properties and need to be used during model construction.
         *
         * @param properties Configuration properties for the formulation, potentially containing values for input
         *                   variables
         * @return A shared pointer to a newly constructed model object
         */
        virtual std::shared_ptr<M> construct_model(const geojson::PropertyMap& properties) = 0;

        /**
         * Determine and set the offset time of the model in seconds, compared to forcing data.
         *
         * BMI models frequently have their model start time be set to 0.  As such, to know what the forcing time is
         * compared to the model time, an offset value is needed.  This becomes important in situations when the size of
         * the time steps for forcing data versus model execution are not equal.  This method will determine and set
         * this value.
         */
        void determine_model_time_offset() {
            set_bmi_model_start_time_forcing_offset_s(
                    // TODO: Look at making this epoch start configurable instead of from forcing
                    forcing->get_data_start_time() - convert_model_time(get_bmi_model()->GetStartTime()));
        }

        /**
         * Get whether a model may perform updates beyond its ``end_time``.
         *
         * Get whether model ``Update`` calls are allowed and handled in some way by the backing model for time steps
         * after the model's ``end_time``.   Implementations of this type should use this function to safeguard against
         * entering either an invalid or otherwise undesired state as a result of attempting to process a model beyond
         * its available data.
         *
         * As mentioned, even for models that are capable of validly handling processing beyond end time, it may be
         * desired that they do not for some reason (e.g., the way they account for the lack of input data leads to
         * valid but incorrect results for a specific application).  Because of this, whether models are allowed to
         * process beyond their end time is configuration-based.
         *
         * @return Whether a model may perform updates beyond its ``end_time``.
         */
        const bool &get_allow_model_exceed_end_time() const override {
            return allow_model_exceed_end_time;
        }

        const string &get_bmi_init_config() const {
            return bmi_init_config;
        }

        /**
         * Get the backing model object implementing the BMI.
         *
         * @return Shared pointer to the backing model object that implements the BMI.
         */
        std::shared_ptr<M> get_bmi_model() {
            return bmi_model;
        }

        const string &get_forcing_file_path() const override {
            return forcing_file_path;
        }

        const time_t &get_bmi_model_start_time_forcing_offset_s() override {
            return bmi_model_start_time_forcing_offset_s;
        }

        /**
         * Get value for some BMI model variable.
         *
         * This function assumes that the given variable, while returned by the model within an array per the BMI spec,
         * is actual a single, scalar value.  Thus, it returns what is at index 0 of the array reference.
         *
         * @param index
         * @param var_name
         * @return
         */
        virtual double get_var_value_as_double(const std::string& var_name) = 0;

        /**
         * Get value for some BMI model variable at a specific index.
         *
         * Function gets the value for a provided variable, returned from the backing model as an array, and returns the
         * specific value at the desired index cast as a double type.
         *
         * The function makes several assumptions:
         *
         *     1. `index` is within array bounds
         *     2. `var_name` is in the set of valid variable names for the model
         *     3. the type for output variable allows the value to be cast to a `double` appropriately
         *
         * It falls to user (functions) of this function to ensure these assumptions hold before invoking.
         *
         * @param index
         * @param var_name
         * @return
         */
        virtual double get_var_value_as_double(const int& index, const std::string& var_name) = 0;

        /**
         * Universal logic applied when creating a BMI-backed formulation from NGen config.
         *
         * This performs all the necessary steps to initialize this formulation from provided configuration
         * properties. It is written in such a way that it can be used in appropriately crafted nested calls from both
         * public `create_formulation` implementations, thus allowing the primary formulation initialization logic to
         * be centralized and not duplicated.
         *
         * @param properties
         * @param needs_param_validation
         */
        void inner_create_formulation(geojson::PropertyMap properties, bool needs_param_validation) {
            if (needs_param_validation) {
                validate_parameters(properties);
            }
            // Required parameters first
            set_bmi_init_config(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG).as_string());
            set_bmi_main_output_var(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MAIN_OUT_VAR).as_string());
            set_model_type_name(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MODEL_TYPE).as_string());
            set_bmi_using_forcing_file(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__USES_FORCINGS).as_boolean());

            // Then optional ...

            // Note that this must be present if bmi_using_forcing_file is true
            if (properties.find(BMI_REALIZATION_CFG_PARAM_OPT__FORCING_FILE) != properties.end())
                set_forcing_file_path(properties.at(BMI_REALIZATION_CFG_PARAM_OPT__FORCING_FILE).as_string());
            else if (is_bmi_using_forcing_file())
                throw std::runtime_error("Can't create BMI formulation: using_forcing_file true but no file path set");

            if (properties.find(BMI_REALIZATION_CFG_PARAM_OPT__ALLOW_EXCEED_END) != properties.end()) {
                set_allow_model_exceed_end_time(
                        properties.at(BMI_REALIZATION_CFG_PARAM_OPT__ALLOW_EXCEED_END).as_boolean());
            }
            if (properties.find(BMI_REALIZATION_CFG_PARAM_OPT__FIXED_TIME_STEP) != properties.end()) {
                set_bmi_model_time_step_fixed(
                        properties.at(BMI_REALIZATION_CFG_PARAM_OPT__FIXED_TIME_STEP).as_boolean());
            }

            auto std_names_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES);
            if (std_names_it != properties.end()) {
                geojson::PropertyMap names_map = std_names_it->second.get_values();
                for (auto& names_it : names_map) {
                    bmi_var_names_map.insert(
                            std::pair<std::string, std::string>(names_it.first, names_it.second.as_string()));
                }
            }

            // Do this next, since after checking whether other input variables are present in the properties, we can
            // now construct the adapter and init the model
            set_bmi_model(construct_model(properties));
            
            //Check if any parameter values need to be set on the BMI model,
            //and set them before it is run
            set_initial_bmi_parameters(properties);
            
            // Make sure that this is able to interpret model time and convert to real time, since BMI model time is
            // usually starting at 0 and just counting up
            determine_model_time_offset();

            // Output variable subset and order, if present
            auto out_var_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OUT_VARS);
            if (out_var_it != properties.end()) {
                std::vector<geojson::JSONProperty> out_vars_json_list = out_var_it->second.as_list();
                std::vector<std::string> out_vars(out_vars_json_list.size());
                for (int i = 0; i < out_vars_json_list.size(); ++i) {
                    out_vars[i] = out_vars_json_list[i].as_string();
                }
                set_output_variable_names(out_vars);
            }
                // Otherwise, just take what literally is provided by the model
            else {
                set_output_variable_names(get_bmi_model()->GetOutputVarNames());
            }

            // Output header fields, if present
            auto out_headers_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OUT_HEADER_FIELDS);
            if (out_headers_it != properties.end()) {
                std::vector<geojson::JSONProperty> out_headers_json_list = out_var_it->second.as_list();
                std::vector<std::string> out_headers(out_headers_json_list.size());
                for (int i = 0; i < out_headers_json_list.size(); ++i) {
                    out_headers[i] = out_headers_json_list[i].as_string();
                }
                set_output_header_fields(out_headers);
            }
            else {
                set_output_header_fields(get_output_variable_names());
            }
            // Create a reference to this for ET by using a WrappedForcingProvider
            std::shared_ptr<data_access::GenericDataProvider> self = std::make_shared<data_access::WrappedForcingProvider>(this);
            input_forcing_providers[NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP] = self;
            input_forcing_providers[CSDMS_STD_NAME_POTENTIAL_ET] = self;

            // Output precision, if present
            auto out_precision_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION);
            if (out_precision_it != properties.end()) {
                set_output_precision(properties.at(BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION).as_natural_number());
            }

            // Finally, make sure this is set
            model_initialized = get_bmi_model()->is_model_initialized();
        }

        /**
         * @brief Check configuration properties for `model_params` and attempt to set them in the bmi model.
         * 
         * This checks for a key named `model_params` in the parsed properties, and for each property
         * it will attempt to call `SetValue` using the property's key as the BMI variable
         * and the property's value as the value to set.
         * 
         * This function should only be called once @p bmi_model is properly constructed.
         * If @p bmi_model is a nullptr, this function becomes a no-op.
         * 
         */
        void set_initial_bmi_parameters(geojson::PropertyMap properties){
            auto model = get_bmi_model();
            if( model == nullptr ) return;
            //Now that the model is ready, we can set some intial parameters passed in the config
            auto model_params = properties.find("model_params");
            
            if (model_params != properties.end() ){
                
                geojson::PropertyMap params = model_params->second.get_values();
                //Declare/init the possible vectors here
                //reuse them for each loop iteration, make sure to clear them
                std::vector<long> long_vec;
                std::vector<double> double_vec;
                //not_supported
                //std::vector<std::string> str_vec;
                //std::vector<bool> bool_vec;
                std::shared_ptr<void> value_ptr;
                for (auto& param : params) {
                    //Get some basic BMI info for this param
                    int varItemSize = get_bmi_model()->GetVarItemsize(param.first);
                    int totalBytes = get_bmi_model()->GetVarNbytes(param.first);
                
                    //Figure out the c++ type to convert data to
                    std::string type = get_bmi_model()->get_analogous_cxx_type(get_bmi_model()->GetVarType(param.first),
                                                                           varItemSize);
                    //TODO might consider refactoring as_vector and get_values_as_type
                    //(and by extension, as_c_array) into the JSONProperty class
                    //then instead of the PropertyVariant visitor filling vectors
                    //it could fill the c-like array and avoid another copy.
                     switch( param.second.get_type() ){
                        case geojson::PropertyType::Natural:
                            param.second.as_vector(long_vec);
                            value_ptr = get_values_as_type(type, long_vec.begin(), long_vec.end());
                            std::cout<<"NAT VALUE: "<<long_vec[0]<<std::endl;
                            break;
                        case geojson::PropertyType::Real:
                            param.second.as_vector(double_vec);
                            value_ptr = get_values_as_type(type, double_vec.begin(), double_vec.end());
                            std::cout<<"REAL VALUE: "<<double_vec[0]<<std::endl;
                            break;
                        /* Not currently supporting string parameter values
                        case geojson::PropertyType::String:
                            param.second.as_vector(str_vec);
                            value_ptr = get_values_as_type(type, long_vec.begin(), long_vec.end());
                        */
                        /* Not currently supporting native bool (true/false) parameter values (use int 0/1)
                        case geojson::PropertyType::Boolean:
                            param.second.as_vector(bool_vec);
                            //data_ptr = bool_vec.data();
                        */
                        case geojson::PropertyType::List:
                            //In this case, only supporting numeric lists
                            //will retrieve as double (longs will get casted)
                            //TODO consider some additional introspection/optimization for this?
                            param.second.as_vector(double_vec);
                            if(double_vec.size() == 0){
                                std::cout<<"Cannot pass non-numeric lists as a BMI parameter, skipping "<<param.first<<std::endl;
                                continue;
                            }
                            value_ptr = get_values_as_type(type, double_vec.begin(), double_vec.end());
                            break;
                        default:
                            std::cout<<"Cannot pass parameter of type "<<geojson::get_propertytype_name(param.second.get_type())<<" as a BMI parameter, skipping "<<param.first<<std::endl;
                            continue;
                    }
                    try{
                        
                        // Finally, use the value obtained to set the model param 
                        get_bmi_model()->SetValue(param.first, value_ptr.get());
                    }
                    catch (const std::exception &e)
                    {
                        std::cout<<"Exception setting parameter value: "<< e.what()<<std::endl;
                        std::cout<<"Skipping parameter: "<<param.first<<std::endl;
                    }
                    catch (...)
                    {
                        std::cout<<"Unknown Exception setting parameter value: "<<std::endl;
                        std::cout<<"Skipping parameter: "<<param.first<<std::endl;
                    }
                    long_vec.clear();
                    double_vec.clear();
                    //Not supported
                    //str_vec.clear();
                    //bool_vec.clear();
                }

            }
            //TODO use SetValue(name, vector<t>) overloads???
            //implment the overloads in each adapter
            //ensure proper type is prepared before setting value
        }

        /**
         * Test whether backing model has fixed time step size.
         *
         * @return Whether backing model has fixed time step size.
         */
        bool is_bmi_model_time_step_fixed() override {
            return bmi_model_time_step_fixed;
        }

        /**
         * Whether the backing model uses/reads the forcing file directly for getting input data.
         *
         * @return Whether the backing model uses/reads the forcing file directly for getting input data.
         */
        bool is_bmi_using_forcing_file() const override {
            return bmi_using_forcing_file;
        }

        /**
         * Test whether the backing model object has been initialize using the BMI standard ``Initialize`` function.
         *
         * @return Whether backing model object has been initialize using the BMI standard ``Initialize`` function.
         */
        bool is_model_initialized() override {
            return model_initialized;
        };

        void set_allow_model_exceed_end_time(bool allow_exceed_end) {
            allow_model_exceed_end_time = allow_exceed_end;
        }

        void set_bmi_init_config(const string &init_config) {
            bmi_init_config = init_config;
        }

        /**
         * Set the backing model object implementing the BMI.
         *
         * @param model Shared pointer to the BMI model.
         */
        void set_bmi_model(std::shared_ptr<M> model) {
            bmi_model = model;
        }

        void set_bmi_model_start_time_forcing_offset_s(const time_t &offset_s) {
            bmi_model_start_time_forcing_offset_s = offset_s;
        }

        void set_bmi_model_time_step_fixed(bool is_fix_time_step) {
            bmi_model_time_step_fixed = is_fix_time_step;
        }

        /**
         * Set whether the backing model uses/reads the forcing file directly for getting input data.
         *
         * @param uses_forcing_file Whether the backing model uses/reads forcing file directly for getting input data.
         */
        void set_bmi_using_forcing_file(bool uses_forcing_file) {
            bmi_using_forcing_file = uses_forcing_file;
        }

        void set_forcing_file_path(const string &forcing_path) {
            forcing_file_path = forcing_path;
        }

        /**
         * Set whether the backing model object has been initialize using the BMI standard ``Initialize`` function.
         *
         * @param is_initialized Whether model object has been initialize using the BMI standard ``Initialize``.
         */
        virtual void set_model_initialized(bool is_initialized) {
            model_initialized = is_initialized;
        }

        /**
         * @brief Template function for copying iterator range into contiguous array.
         * 
         * This function will iterate the range and cast the iterator value to type T
         * and copy that value into a C-like array of contiguous, dynamically allocated memory.
         * This array is stored in a smart pointer with a custom array deleter.
         * 
         * @tparam T 
         * @tparam Iterator 
         * @param begin 
         * @param end 
         * @return std::shared_ptr<T> 
         */
        template <typename T, typename Iterator>
        std::shared_ptr<T> as_c_array(Iterator begin, Iterator end){
            //Make a shared pointer large enough to hold all elements
            //This is a CONTIGUOUS array of type T
            //Must provide a custom deleter to delete the array
            std::shared_ptr<T> ptr( new T[std::distance(begin, end)], [](T *p) { delete[] p; } );
            Iterator it = begin;
            int i = 0;
            while(it != end){
                //Be safe and cast the input to the desired type
                ptr.get()[i] = static_cast<T>(*it);
                ++it;
                ++i;
            }
            return ptr;
        }

        /**
         * @brief Gets values in iterator range, casted based on @p type then returned as typeless (void) pointer.  
         * 
         * @tparam Iterator 
         * @param type 
         * @param begin 
         * @param end 
         * @return std::shared_ptr<void> 
         */
        template <typename Iterator>
        std::shared_ptr<void> get_values_as_type(std::string type, Iterator begin, Iterator end)
        {
            //Use std::vector range constructor to ensure contiguous storage of values
            //Return the pointer to the contiguous storage
            if (type == "double" || type == "double precision")
                return as_c_array<double>(begin, end);
           
            if (type == "float" || type == "real")
                return as_c_array<float>(begin, end);

            if (type == "short" || type == "short int" || type == "signed short" || type == "signed short int")
                return as_c_array<short>(begin, end);

            if (type == "unsigned short" || type == "unsigned short int")
                return as_c_array<unsigned short>(begin, end);

            if (type == "int" || type == "signed" || type == "signed int" || type == "integer")
                return as_c_array<int>(begin, end);

            if (type == "unsigned" || type == "unsigned int")
                return as_c_array<unsigned int>(begin, end);

            if (type == "long" || type == "long int" || type == "signed long" || type == "signed long int")
                return as_c_array<long>(begin, end);

            if (type == "unsigned long" || type == "unsigned long int")
                return as_c_array<unsigned long>(begin, end);

            if (type == "long long" || type == "long long int" || type == "signed long long" || type == "signed long long int")
                return as_c_array<long long>(begin, end);

            if (type == "unsigned long long" || type == "unsigned long long int")
                return as_c_array<unsigned long long>(begin, end);

            throw std::runtime_error("Unable to get values of iterable as type" + type + " from " + get_model_type_name() +
                " : no logic for converting values to variable's type.");
        }

        // TODO: need to modify this to support arrays properly, since in general that's what BMI modules deal with
        template<typename T>
        std::shared_ptr<void> get_value_as_type(std::string type, T value)
        {
            if (type == "double" || type == "double precision")
                return std::make_shared<double>( static_cast<double>(value) );

            if (type == "float" || type == "real")
                return std::make_shared<float>( static_cast<float>(value) );

            if (type == "short" || type == "short int" || type == "signed short" || type == "signed short int")
                return std::make_shared<short>( static_cast<short>(value) );

            if (type == "unsigned short" || type == "unsigned short int")
                return std::make_shared<unsigned short>( static_cast<unsigned short>(value) );

            if (type == "int" || type == "signed" || type == "signed int" || type == "integer")
                return std::make_shared<int>( static_cast<int>(value) );

            if (type == "unsigned" || type == "unsigned int")
                return std::make_shared<unsigned int>( static_cast<unsigned int>(value) );

            if (type == "long" || type == "long int" || type == "signed long" || type == "signed long int")
                return std::make_shared<long>( static_cast<long>(value) );

            if (type == "unsigned long" || type == "unsigned long int")
                return std::make_shared<unsigned long>( static_cast<unsigned long>(value) );

            if (type == "long long" || type == "long long int" || type == "signed long long" || type == "signed long long int")
                return std::make_shared<long long>( static_cast<long long>(value) );

            if (type == "unsigned long long" || type == "unsigned long long int")
                return std::make_shared<unsigned long long>( static_cast<unsigned long long>(value) );

            throw std::runtime_error("Unable to get value of variable as type" + type + " from " + get_model_type_name() +
                " : no logic for converting value to variable's type.");
        }

        /**
         * Set BMI input variable values for the model appropriately prior to calling its `BMI `update()``.
         *
         * @param model_initial_time The model's time prior to the update, in its internal units and representation.
         * @param t_delta The size of the time step over which the formulation is going to update the model, which might
         *                be different than the model's internal time step.
         */
        void set_model_inputs_prior_to_update(const double &model_init_time, time_step_t t_delta) {
            std::vector<std::string> in_var_names = get_bmi_model()->GetInputVarNames();
            time_t model_epoch_time = convert_model_time(model_init_time) + get_bmi_model_start_time_forcing_offset_s();

            for (std::string & var_name : in_var_names) {
                data_access::GenericDataProvider *provider;
                std::string var_map_alias = get_config_mapped_variable_name(var_name);
                if (input_forcing_providers.find(var_map_alias) != input_forcing_providers.end()) {
                    provider = input_forcing_providers[var_map_alias].get();
                }
                else if (var_map_alias != var_name && input_forcing_providers.find(var_name) != input_forcing_providers.end()) {
                    provider = input_forcing_providers[var_name].get();
                }
                else {
                    provider = forcing.get();
                }
                // TODO: probably need to actually allow this by default and warn, but have config option to activate
                //  this type of behavior
                // TODO: account for arrays later
                int varItemSize = get_bmi_model()->GetVarItemsize(var_name);
                std::shared_ptr<void> value_ptr;
                // Finally, use the value obtained to set the model input
                std::string type = get_bmi_model()->get_analogous_cxx_type(get_bmi_model()->GetVarType(var_name),
                                                                           varItemSize);
                if (varItemSize != get_bmi_model()->GetVarNbytes(var_name)) {
                    //more than a single value needed for var_name
                    auto values = provider->get_values(CatchmentAggrDataSelector("",var_map_alias, model_epoch_time, t_delta,
                                                   get_bmi_model()->GetVarUnits(var_name)));
                    //need to marshal data types to the reciever as well
                    //this could be done a little more elegantly if the provider interface were
                    //"type aware", but for now, this will do (but requires yet another copy)
                    value_ptr = get_values_as_type( type, values.begin(), values.end() );

                } else {
                    //scalar value
                    double value = provider->get_value(CatchmentAggrDataSelector("",var_map_alias, model_epoch_time, t_delta,
                                                   get_bmi_model()->GetVarUnits(var_name)));
                    value_ptr = get_value_as_type(type, value);      
                }
                get_bmi_model()->SetValue(var_name, value_ptr.get());
            }
        }

        /** The delta of the last model update execution (typically, this is time step size). */
        time_step_t last_model_response_delta = 0;
        /** The epoch time of the model at the beginning of its last update. */
        time_t last_model_response_start_time = 0;
        std::map<std::string, std::shared_ptr<data_access::GenericDataProvider>> input_forcing_providers;

        // Access for multi-BMI
        friend class Bmi_Multi_Formulation;

        // Unit test access
        friend class ::Bmi_Formulation_Test;
        friend class ::Bmi_C_Formulation_Test;
        friend class ::Bmi_Multi_Formulation_Test;
        friend class ::Bmi_Cpp_Formulation_Test;
        friend class ::Bmi_Cpp_Multi_Array_Test;

    private:
        /**
         * Whether model ``Update`` calls are allowed and handled in some way by the backing model for time steps after
         * the model's ``end_time``.
         */
        bool allow_model_exceed_end_time = false;
        /** The set of available "forcings" (output variables, plus their mapped aliases) that the model can provide. */
        std::vector<std::string> available_forcings;
        std::string bmi_init_config;
        std::shared_ptr<M> bmi_model;
        /** Whether backing model has fixed time step size. */
        bool bmi_model_time_step_fixed = true;
        /**
         * The offset, converted to seconds, from the model's start time to the start time of the initial forcing time
         * step.
         */
        time_t bmi_model_start_time_forcing_offset_s;
        /** A configured mapping of BMI model variable names to standard names for use inside the framework. */
        std::map<std::string, std::string> bmi_var_names_map;
        /** Whether the backing model uses/reads the forcing file directly for getting input data. */
        bool bmi_using_forcing_file;
        std::string forcing_file_path;
        bool model_initialized = false;

        std::vector<std::string> OPTIONAL_PARAMETERS = {
                BMI_REALIZATION_CFG_PARAM_OPT__FORCING_FILE,
                BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES,
                BMI_REALIZATION_CFG_PARAM_OPT__OUT_VARS,
                BMI_REALIZATION_CFG_PARAM_OPT__OUT_HEADER_FIELDS,
                BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION,
                BMI_REALIZATION_CFG_PARAM_OPT__ALLOW_EXCEED_END,
                BMI_REALIZATION_CFG_PARAM_OPT__FIXED_TIME_STEP,
                BMI_REALIZATION_CFG_PARAM_OPT__LIB_FILE
        };
        std::vector<std::string> REQUIRED_PARAMETERS = {
                BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG,
                BMI_REALIZATION_CFG_PARAM_REQ__MAIN_OUT_VAR,
                BMI_REALIZATION_CFG_PARAM_REQ__MODEL_TYPE,
                BMI_REALIZATION_CFG_PARAM_REQ__USES_FORCINGS
        };

    };
/*
    template<class M>
    void Bmi_Formulation<M>::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
        geojson::PropertyMap options = this->interpret_parameters(config, global);
        inner_create_formulation(options, false);
    }

    template<class M>
    void Bmi_Formulation<M>::create_formulation(geojson::PropertyMap properties) {
        inner_create_formulation(properties, true);
    }

    template<class M>
    void Bmi_Formulation<M>::inner_create_formulation(geojson::PropertyMap properties, bool needs_param_validation) {
        if (needs_param_validation) {
            validate_parameters(properties);
        }
        // Required parameters first
        set_bmi_init_config(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG).as_string());
        set_bmi_main_output_var(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MAIN_OUT_VAR).as_string());
        set_forcing_file_path(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__FORCING_FILE).as_string());
        set_model_type_name(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MODEL_TYPE).as_string());
        set_bmi_using_forcing_file(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__USES_FORCINGS).as_boolean());

        // Then optional ...

        if (properties.find(BMI_REALIZATION_CFG_PARAM_OPT__ALLOW_EXCEED_END) != properties.end()) {
            set_allow_model_exceed_end_time(
                    properties.at(BMI_REALIZATION_CFG_PARAM_OPT__ALLOW_EXCEED_END).as_boolean());
        }

        // Do this next, since after checking whether other input variables are present in the properties, we can
        // now construct the adapter and init the model
        set_bmi_model(construct_model(properties));

        // Output variable subset and order, if present
        auto out_var_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OUT_VARS);
        if (out_var_it != properties.end()) {
            std::vector<geojson::JSONProperty> out_vars_json_list = out_var_it->second.as_list();
            std::vector<std::string> out_vars(out_vars_json_list.size());
            for (int i = 0; i < out_vars_json_list.size(); ++i) {
                out_vars[i] = out_vars_json_list[i].as_string();
            }
            set_output_variable_names(out_vars);
        }
            // Otherwise, just take what literally is provided by the model
        else {
            set_output_variable_names(get_bmi_model()->GetOutputVarNames());
        }

        // Output header fields, if present
        auto out_headers_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OUT_HEADER_FIELDS);
        if (out_headers_it != properties.end()) {
            std::vector<geojson::JSONProperty> out_headers_json_list = out_var_it->second.as_list();
            std::vector<std::string> out_headers(out_headers_json_list.size());
            for (int i = 0; i < out_headers_json_list.size(); ++i) {
                out_headers[i] = out_headers_json_list[i].as_string();
            }
            set_output_header_fields(out_headers);
        }
        else {
            set_output_header_fields(get_output_variable_names());
        }
    }
    */

}

#endif //NGEN_BMI_MODULE_FORMULATION_H
