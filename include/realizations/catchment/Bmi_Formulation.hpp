#ifndef NGEN_BMI_FORMULATION_H
#define NGEN_BMI_FORMULATION_H

#include <utility>
#include "Catchment_Formulation.hpp"
#include "EtCalcProperty.hpp"
#include "EtCombinationMethod.hpp"

// Define the configuration parameter names used in the realization/formulation config JSON file
// First the required:
#define BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG "init_config"
#define BMI_REALIZATION_CFG_PARAM_REQ__MAIN_OUT_VAR "main_output_variable"
#define BMI_REALIZATION_CFG_PARAM_REQ__MODEL_TYPE "model_type_name"
#define BMI_REALIZATION_CFG_PARAM_REQ__USES_FORCINGS "uses_forcing_file"

// Then the optional
#define BMI_REALIZATION_CFG_PARAM_OPT__FORCING_FILE "forcing_file"
#define BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "variables_names_map"
#define BMI_REALIZATION_CFG_PARAM_OPT__OUT_VARS "output_variables"
#define BMI_REALIZATION_CFG_PARAM_OPT__OUT_HEADER_FIELDS "output_header_fields"
#define BMI_REALIZATION_CFG_PARAM_OPT__ALLOW_EXCEED_END "allow_exceed_end_time"
#define BMI_REALIZATION_CFG_PARAM_OPT__FIXED_TIME_STEP "fixed_time_step"
#define BMI_REALIZATION_CFG_PARAM_OPT__LIB_FILE "library_file"
#define BMI_REALIZATION_CFG_PARAM_OPT__REGISTRATION_FUNC "registration_function"

// Supported Standard Names for BMI variables
// This is needed to provide a calculated potential ET value back to a BMI model
#define NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP "potential_evapotranspiration"

// Taken from the CSDMS Standard Names list
// TODO: need to add these in for anything BMI model input or output variables we need to know how to recognize
#define CSDMS_STD_NAME_RAIN_RATE "atmosphere_water__rainfall_volume_flux"

// Forward declaration to provide access to protected items in testing
class Bmi_Formulation_Test;
class Bmi_C_Formulation_Test;
class Bmi_C_Cfe_IT;

namespace realization {

    /**
     * Abstraction of a formulation with a backing model object that implements the BMI.
     *
     * @tparam M The type for the backing BMI model object.
     */
    template <class M>
    class Bmi_Formulation : public Catchment_Formulation {

    public:

        /**
         * Minimal constructor for objects initialize using the Formulation_Manager and subsequent calls to
         * ``create_formulation``.
         *
         * @param id
         * @param forcing_config
         * @param output_stream
         */
        Bmi_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream)
                : Catchment_Formulation(std::move(id), std::move(forcing_config), output_stream) { };

        virtual ~Bmi_Formulation() {};

        /**
         * Perform (potential) ET calculation, getting input params from instance's backing model as needed.
         *
         * @return Calculated ET or potential ET, or ``0.0`` if required parameters could not be obtained.
         */
        double calc_et() override {
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
            et_options.use_combination_method      = FALSE;
            et_options.use_priestley_taylor_method = FALSE;
            et_options.use_penman_monteith_method  = TRUE;

            struct AORC_data raw_aorc = forcing.get_AORC_data();
            // TODO: do we really actually need this, if we are converting to another forcing struct?
            // TODO: WHY ARE THERE SO MANY FORCING STRUCTS!?!
            struct et::aorc_forcing_data aorc = convert_aorc_structs(raw_aorc);

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
        const std::string &get_config_mapped_variable_name(const std::string &model_var_name) {
            // TODO: need to introduce validation elsewhere that all mapped names are valid AORC field constants.
            if (bmi_var_names_map.find(model_var_name) != bmi_var_names_map.end())
                return bmi_var_names_map[model_var_name];
            else
                return model_var_name;
        }

        virtual inline bool is_bmi_input_variable(const std::string &var_name) = 0;

        virtual inline bool is_bmi_output_variable(const std::string &var_name) = 0;

    protected:

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
        virtual void determine_model_time_offset() = 0;

        const bool &get_allow_model_exceed_end_time() const {
            return allow_model_exceed_end_time;
        }

        const string &get_bmi_init_config() const {
            return bmi_init_config;
        }

        const string &get_bmi_main_output_var() const {
            return bmi_main_output_var;
        }

        /**
         * Get the backing model object implementing the BMI.
         *
         * @return Shared pointer to the backing model object that implements the BMI.
         */
        std::shared_ptr<M> get_bmi_model() {
            return bmi_model;
        }

        const string &get_forcing_file_path() const {
            return forcing_file_path;
        }

        const time_t &get_bmi_model_start_time_forcing_offset_s() const {
            return bmi_model_start_time_forcing_offset_s;
        }

        /**
         * Get the name of the specific type of the backing model object.
         *
         * @return The name of the backing model object's type.
         */
        std::string get_model_type_name() {
            return model_type_name;
        };

        /**
         * Get the values making up the header line from get_output_header_line(), but organized as a vector of strings.
         *
         * @return The values making up the header line from get_output_header_line() organized as a vector.
         */
        const vector<std::string> &get_output_header_fields() const {
            return output_header_fields;
        }

        /**
         * Get the names of variables in formulation output.
         *
         * Get the names of the variables to include in the output from this formulation, which should be some ordered
         * subset of the output variables from the model.
         *
         * @return
         */
        const vector<std::string> &get_output_variable_names() const {
            return output_variable_names;
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
        }

        /**
         * Test whether backing model has fixed time step size.
         *
         * @return Whether backing model has fixed time step size.
         */
        bool is_bmi_model_time_step_fixed() {
            return bmi_model_time_step_fixed;
        }

        /**
         * Whether the backing model uses/reads the forcing file directly for getting input data.
         *
         * @return Whether the backing model uses/reads the forcing file directly for getting input data.
         */
        bool is_bmi_using_forcing_file() const {
            return bmi_using_forcing_file;
        }

        /**
         * Test whether the backing model object has been initialize using the BMI standard ``Initialize`` function.
         *
         * @return Whether backing model object has been initialize using the BMI standard ``Initialize`` function.
         */
        virtual bool is_model_initialized() {
            return model_initialized;
        };

        void set_allow_model_exceed_end_time(bool allow_exceed_end) {
            allow_model_exceed_end_time = allow_exceed_end;
        }

        void set_bmi_init_config(const string &init_config) {
            bmi_init_config = init_config;
        }

        void set_bmi_main_output_var(const string &main_output_var) {
            bmi_main_output_var = main_output_var;
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
         * Set BMI input variable values for the model appropriately prior to calling its `BMI `update()``.
         *
         * @param model_initial_time The model's time prior to the update, in its internal units and representation.
         * @param t_delta The size of the time step over which the formulation is going to update the model, which might
         *                be different than the model's internal time step.
         */
        virtual void set_model_inputs_prior_to_update(const double &model_initial_time, time_step_t t_delta) = 0;

        /**
         * Set the name of the specific type of the backing model object.
         *
         * @param type_name The name of the backing model object's type.
         */
        void set_model_type_name(std::string type_name) {
            model_type_name = std::move(type_name);
        }

        void set_output_header_fields(const vector<std::string> &output_headers) {
            output_header_fields = output_headers;
        }

        /**
         * Set the names of variables in formulation output.
         *
         * Set the names of the variables to include in the output from this formulation, which should be some ordered
         * subset of the output variables from the model.
         *
         * @param out_var_names the names of variables in formulation output, in the order they should appear.
         */
        void set_output_variable_names(const vector<std::string> &out_var_names) {
            output_variable_names = out_var_names;
        }

        // Unit test access
        friend class ::Bmi_Formulation_Test;
        friend class ::Bmi_C_Formulation_Test;

    private:
        /**
         * Whether model ``Update`` calls are allowed and handled in some way by the backing model for time steps after
         * the model's ``end_time``.
         */
        bool allow_model_exceed_end_time = false;
        std::string bmi_init_config;
        std::shared_ptr<M> bmi_model;
        /** Whether backing model has fixed time step size. */
        bool bmi_model_time_step_fixed = true;
        /**
         * The offset, converted to seconds, from the model's start time to the start time of the initial forcing time
         * step.
         */
        time_t bmi_model_start_time_forcing_offset_s;
        std::string bmi_main_output_var;
        /** A configured mapping of BMI model variable names to standard names for use inside the framework. */
        std::map<std::string, std::string> bmi_var_names_map;
        /** Whether the backing model uses/reads the forcing file directly for getting input data. */
        bool bmi_using_forcing_file;
        std::string forcing_file_path;
        /**
         * Output header field strings corresponding to the variables output by the realization, as defined in
         * `output_variable_names`.
         */
        std::vector<std::string> output_header_fields;
        /**
         * Names of the variables to include in the output from this realization, which should be some ordered subset of
         * the output variables from the model.
         */
        std::vector<std::string> output_variable_names;
        bool model_initialized = false;
        std::string model_type_name;

        std::vector<std::string> OPTIONAL_PARAMETERS = {
                BMI_REALIZATION_CFG_PARAM_OPT__FORCING_FILE,
                BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES,
                BMI_REALIZATION_CFG_PARAM_OPT__OUT_VARS,
                BMI_REALIZATION_CFG_PARAM_OPT__OUT_HEADER_FIELDS,
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

#endif //NGEN_BMI_FORMULATION_H
