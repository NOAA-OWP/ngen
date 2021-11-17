#ifndef NGEN_BMI_MULTI_FORMULATION_HPP
#define NGEN_BMI_MULTI_FORMULATION_HPP

#include <map>
#include <vector>
#include "Bmi_Formulation.hpp"
#include "Bmi_Module_Formulation.hpp"
#include "bmi.hpp"
#include "ForcingProvider.hpp"

#define BMI_REALIZATION_CFG_PARAM_REQ__MODULES "modules"
#define DEFAULT_ET_FORMULATION_INDEX 0

using namespace std;

class Bmi_Multi_Formulation_Test;

namespace realization {

    /**
     * Abstraction of a formulation with multiple backing model object that implements the BMI.
     */
    class Bmi_Multi_Formulation : public Bmi_Formulation {

    public:

        typedef Bmi_Formulation nested_module_type;
        typedef std::shared_ptr<nested_module_type> nested_module_ptr;

        /**
         * Minimal constructor for objects initialize using the Formulation_Manager and subsequent calls to
         * ``create_formulation``.
         *
         * @param id
         * @param forcing_config
         * @param output_stream
         */
        Bmi_Multi_Formulation(string id, std::unique_ptr<forcing::ForcingProvider> forcing_provider, utils::StreamHandler output_stream)
                : Bmi_Formulation(move(id), move(forcing_provider), output_stream) { };

        virtual ~Bmi_Multi_Formulation() {};

        /**
         * Perform ET (or PET) calculations.
         *
         * This function defers to the analogous function of the appropriate nested submodule, using
         * @ref determine_et_formulation_index to determine which one that is.
         *
         * @return The ET calculation from the appropriate module from this instance's collection of BMI nested modules.
         */
        double calc_et() override {
            return modules[determine_et_formulation_index()]->calc_et();
        }

        /**
         * Convert a time value from the model to an epoch time in seconds.
         *
         * Model time values are typically (though not always) 0-based totals count upward as time progresses.  The
         * units are not necessarily seconds.  This performs the necessary lookup and conversion for such units, and
         * then shifts the value appropriately for epoch time representation.
         *
         * For this type, this function will behave in the same manner as the analogous function of the current
         * "primary" nested formulation, which is found in the instance's ordered collection of nested module
         * formulations at the index returned by @ref get_index_for_primary_module.
         *
         * @param model_time The time value in a model's representation that is to be converted.
         * @return The equivalent epoch time.
         */
        time_t convert_model_time(const double &model_time) override {
            return convert_model_time(model_time, get_index_for_primary_module());
        }

        /**
         * Convert a time value from the model to an epoch time in seconds.
         *
         * Model time values are typically (though not always) 0-based totals count upward as time progresses.  The
         * units are not necessarily seconds.  This performs the necessary lookup and conversion for such units, and
         * then shifts the value appropriately for epoch time representation.
         *
         * For this type, this function will behave in the same manner as the analogous function of nested formulation
         * found at the provided index within the instance's ordered collection of nested module formulations.
         *
         * @param model_time The time value in a model's representation that is to be converted.
         * @return The equivalent epoch time.
         */
        inline time_t convert_model_time(const double &model_time, int module_index) {
            return modules[module_index]->convert_model_time(model_time);
        }

        void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override {
            geojson::PropertyMap options = this->interpret_parameters(config, global);
            create_multi_formulation(options, false);
        }

        void create_formulation(geojson::PropertyMap properties) override {
            create_multi_formulation(properties, true);
        }

        /**
         * Determine the index of the correct sub-module formulation from which ET values should be obtain.
         *
         * This is primarily used to determine the correct module in the multi-module collection for getting the ET
         * value when calls to @ref calc_et are made on this "outer" multi-module formulation.
         *
         * The function first attempts to find the nested module that makes available either
         * ``NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP`` or ``CSDMS_STD_NAME_POTENTIAL_ET`` available, if any does.  It
         * then provides the index of this module.
         *
         * Otherwise, the function iterates through the nested modules in order performing a search for one that
         * provides a value with either the name defined in ``NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP`` or defined in
         * ``CSDMS_STD_NAME_POTENTIAL_ET``.  It return the index of the first having either.
         *
         * If neither means finds the appropriate module, at default value ``DEFAULT_ET_FORMULATION_INDEX`` is returned.
         *
         * @return The index of correct sub-module formulation from which ET values should be obtain.
         */
        size_t determine_et_formulation_index() {
            std::shared_ptr<forcing::ForcingProvider> et_provider = nullptr;
            if (availableData.find(NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP) != availableData.end()) {
                et_provider = availableData[NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP];
            }
            else if (availableData.find(CSDMS_STD_NAME_POTENTIAL_ET) != availableData.end()) {
                et_provider = availableData[CSDMS_STD_NAME_POTENTIAL_ET];
            }
            // The pointers in availableData and modules should be the same, based on create_multi_formulation()
            // So, if an et_provider was found from availableData, we can compare to determine the right index.
            if (et_provider != nullptr) {
                for (size_t i = 0; i < modules.size(); ++i) {
                    if (et_provider == modules[i]) {
                        return i;
                    }
                }
            }
            // Otherwise, check the modules directly for the standard ET output variable names.
            for (size_t i = 0; i < modules.size(); ++i) {
                std::vector<std::string> values = modules[i]->get_available_forcing_outputs();
                if (std::find(values.begin(), values.end(), NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP) != values.end()) {
                    return i;
                }
                if (std::find(values.begin(), values.end(), CSDMS_STD_NAME_POTENTIAL_ET) != values.end()) {
                    return i;
                }
            }
            return DEFAULT_ET_FORMULATION_INDEX;
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
        const bool &get_allow_model_exceed_end_time() const override;

        /**
         * Get the collection of forcing output property names this instance can provide.
         *
         * For this type, this is the collection of the names/aliases of the BMI output variables for nested modules;
         * i.e., the config-mapped alias for the variable when set in the realization config, or just the name when no
         * alias was included in the configuration.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * @return The collection of forcing output property names this instance can provide.
         * @see ForcingProvider
         */
        const vector<std::string> &get_available_forcing_outputs() override;

        /**
        * Get the input variables of the first nested BMI model.
        *
        * @return The input variables of the first nested BMI model.
        */
        const vector<string> get_bmi_input_variables() override {
            return modules[0]->get_bmi_input_variables();
        }

        const time_t &get_bmi_model_start_time_forcing_offset_s() override;

        /**
         * Get the output variables of the last nested BMI model.
         *
         * @return The output variables of the last nested BMI model.
         */
        const vector<string> get_bmi_output_variables() override {
            return modules.back()->get_bmi_output_variables();
        }

        /**
         * When possible, translate a variable name for a BMI model to an internally recognized name.
         *
         * Because of the implementation of this type, this function can only translate variable names for input or
         * output variables of either the first or last nested BMI module.  In cases when this is not possible, it will
         * return the original parameter.
         *
         * The function will only check input variable names for the first module and output variable names for the
         * last module.  Further, it will always check the first module first, returning if it finds a translation.
         * Only then will it check the last module.  This can be controlled by using the overloaded function
         * @ref get_config_mapped_variable_name(string, bool, bool).
         *
         * To perform a similar translation between modules, see the overloaded function
         * @ref get_config_mapped_variable_name(string, shared_ptr, shared_ptr).
         *
         * @param model_var_name The BMI variable name to translate so its purpose is recognized internally.
         * @return Either the translated equivalent variable name, or the provided name if there is not a mapping entry.
         * @see get_config_mapped_variable_name(string, bool, bool)
         * @see get_config_mapped_variable_name(string, shared_ptr, shared_ptr)
         */
        const string &get_config_mapped_variable_name(const string &model_var_name) override;

        /**
         * When possible, translate a variable name for the first or last BMI model to an internally recognized name.
         *
         * Because of the implementation of this type, this function can only translate variable names for input or
         * output variables of either the first or last nested BMI module.  In cases when this is not possible, it will
         * return the original parameter.
         *
         * The function can only check input variable names for the first module and output variable names for the
         * last module.  Parameters control whether each is actually checked.  When both are ``true``, it will always
         * check the first module first, returning if it finds a translation without checking the last.
         *
         * @param model_var_name The BMI variable name to translate so its purpose is recognized internally.
         * @param check_first Whether an input variable mapping for the first module should sought.
         * @param check_last Whether an output variable mapping for the last module should sought.
         * @return Either the translated equivalent variable name, or the provided name if there is not a mapping entry.
         */
        const string &get_config_mapped_variable_name(const string &model_var_name, bool check_first, bool check_last);

        /**
         * When possible, translate the name of an output variable for one BMI model to an input variable for another.
         *
         * This function behaves similarly to @ref get_config_mapped_variable_name(string), except that is performs the
         * translation between modules (rather than between a module and the framework).  As such, it is designed for
         * translation between two sequential models, although this is not a requirement for valid execution.
         *
         * The function will first request the mapping for the parameter name from the outputting module, which will either
         * return a mapped name or the original param.  It will check if the returned value is one of the advertised BMI input
         * variable names of the inputting module; if so, it returns that name.  Otherwise, it proceeds.
         *
         * The function then iterates through all the BMI input variable names for the inputting module.  If it finds any that
         * maps to either the original parameter or the mapped name from the outputting module, it returns it.
         *
         * If neither of those find a mapping, then the original parameter is returned.
         *
         * Note that if this is not an output variable name of the outputting module, the function treats this as a no-mapping
         * condition and returns the parameter.
         *
         * @param output_var_name The output variable to be translated.
         * @param out_module The module having the output variable.
         * @param in_module The module needing a translation of ``output_var_name`` to one of its input variable names.
         * @return Either the translated equivalent variable name, or the provided name if there is not a mapping entry.
         */
        const string &get_config_mapped_variable_name(const string &output_var_name,
                                                      const shared_ptr<Bmi_Formulation>& out_module,
                                                      const shared_ptr<Bmi_Formulation>& in_module);

        const string &get_forcing_file_path() const override;

        /**
         * Get the inclusive beginning of the period of time over which this instance can provide data for this forcing.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * @return The inclusive beginning of the period of time over which this instance can provide this data.
         */
        time_t get_forcing_output_time_begin(const std::string &forcing_name) override {
            std::string var_name = forcing_name;
            if(var_name == "*" || var_name == ""){
                // when unspecified, assume all data is available for the same range.
                // Find one that successfully returns...
                for(std::map<std::string,std::shared_ptr<forcing::ForcingProvider>>::iterator iter = availableData.begin(); iter != availableData.end(); ++iter)
                {
                    var_name = iter->first;
                    //TODO: Find a probably more performant way than trial and exception here.
                    try {
                        time_t rv = availableData[var_name]->get_forcing_output_time_begin(var_name);
                        return rv;
                    }
                    catch (...){
                        continue;
                    }
                    break;
                }
            }
            // If not found ...
            if (availableData.empty() || availableData.find(var_name) == availableData.end()) {
                throw runtime_error(get_formulation_type() + " cannot get output time for unknown \"" + forcing_name + "\"");
            }
            return availableData[var_name]->get_forcing_output_time_begin(var_name);
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
        time_t get_forcing_output_time_end(const std::string &forcing_name) override {
            // If not found ...
            std::string var_name = forcing_name;
            if(var_name == "*" || var_name == ""){
                // when unspecified, assume all data is available for the same range.
                // Find one that successfully returns...
                for(std::map<std::string,std::shared_ptr<forcing::ForcingProvider>>::iterator iter = availableData.begin(); iter != availableData.end(); ++iter)
                {
                    var_name = iter->first;
                    //TODO: Find a probably more performant way than trial and exception here.
                    try {
                        time_t rv = availableData[var_name]->get_forcing_output_time_end(var_name);
                        return rv;
                    }
                    catch (...){
                        continue;
                    }
                    break;
                }
            }
            // If not found ...
            if (availableData.empty() || availableData.find(var_name) == availableData.end()) {
                throw runtime_error(get_formulation_type() + " cannot get output time for unknown \"" + forcing_name + "\"");
            }
            return availableData[var_name]->get_forcing_output_time_end(var_name);
        }

        string get_formulation_type() override {
            return "bmi_multi";
        }

        /**
         * Get the current time for the primary nested BMI model in its native format and units.
         *
         * @return The current time for the primary nested BMI model in its native format and units.
         */
        const double get_model_current_time() override {
            return modules[get_index_for_primary_module()]->get_model_current_time();
        }

        /**
         * Get the end time for the primary nested BMI model in its native format and units.
         *
         * @return The end time for the primary nested BMI model in its native format and units.
         */
        const double get_model_end_time() override {
            return modules[get_index_for_primary_module()]->get_model_end_time();
        }

        string get_output_line_for_timestep(int timestep, std::string delimiter) override;

        double get_response(time_step_t t_index, time_step_t t_delta) override;

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
            throw runtime_error("Bmi_Multi_Formulation does not yet implement get_ts_index_for_time");
        }

        /**
         * Get the value of a forcing property for an arbitrary time period, converting units if needed.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * For this type, the @ref availableData map contains available properties and either the external forcing
         * provider or the internal nested module that provides that output property.  That member is set during
         * creation, within the @ref create_multi_formulation function. This function implementation simply defers to
         * the function of the same name in the appropriate nested forcing provider.
         *
         * An @ref std::out_of_range exception should be thrown if the data for the time period is not available.
         *
         * @param output_name The name of the forcing property of interest.
         * @param init_time_epoch The epoch time (in seconds) of the start of the time period.
         * @param duration_seconds The length of the time period, in seconds.
         * @param output_units The expected units of the desired output value.
         * @return The value of the forcing property for the described time period, with units converted if needed.
         * @throws std::out_of_range If data for the time period is not available.
         */
        double get_value(const std::string &output_name, const time_t &init_time, const long &duration_s,
                         const std::string &output_units) override
        {
            // If not found ...
            if (availableData.empty() || availableData.find(output_name) == availableData.end()) {
                throw runtime_error(get_formulation_type() + " cannot get output value for unknown " + output_name);
            }
            return availableData[output_name]->get_value(output_name, init_time, duration_s, output_units);
        }

        bool is_bmi_input_variable(const string &var_name) override;

        /**
         * Test whether all backing models have fixed time step size.
         *
         * @return Whether all backing models has fixed time step size.
         */
        bool is_bmi_model_time_step_fixed() override;

        bool is_bmi_output_variable(const string &var_name) override;

        bool is_bmi_using_forcing_file() const override;

        /**
         * Test whether all backing models have been initialize using the BMI standard ``Initialize`` function.
         *
         * @return Whether all backing models have been initialize using the BMI standard ``Initialize`` function.
         */
        bool is_model_initialized() override;

        /**
         * Get whether a property's per-time-step values are each an aggregate sum over the entire time step.
         *
         * Certain properties, like rain fall, are aggregated sums over an entire time step.  Others, such as pressure,
         * are not such sums and instead something else like an instantaneous reading or an average value.
         *
         * It may be the case that forcing data is needed for some discretization different than the forcing time step.
         * This aspect must be known in such cases to perform the appropriate value interpolation.
         *
         * For instances of this type, all output forcings fall under this category.
         *
         * This is part of the @ref ForcingProvider interface.  This interface must be implemented for items of this
         * type to be usable as "forcing" providers for situations when some other object needs to receive as an input
         * (i.e., one of its forcings) a data property output from this object.
         *
         * @param name The name of the forcing property for which the current value is desired.
         * @return Whether the property's value is an aggregate sum.
         */
        bool is_property_sum_over_time_step(const string &name) override {
            if (availableData.empty() || availableData.find(name) == availableData.end()) {
                throw runtime_error(
                        get_formulation_type() + " cannot get whether unknown property " + name + " is summation");
            }
            return availableData[name]->is_property_sum_over_time_step(name);
        }

        /**
         * Get whether this time step goes beyond this formulations (i.e., any of it's modules') end time.
         *
         * @param t_index The time step index in question.
         * @return Whether this time step goes beyond this formulations (i.e., any of it's modules') end time.
         */
        bool is_time_step_beyond_end_time(time_step_t t_index);

        /**
         * Get the index of the primary module.
         *
         * @return The index of the primary module.
         */
        inline int get_index_for_primary_module() {
            return primary_module_index;
        }

        /**
         * Set the index of the primary module.
         *
         * Note that this function does not alter the state of the class, or produce an error, if the index is out of
         * range.
         *
         * @param index The index for the module.
         */
        inline void set_index_for_primary_module(int index) {
            if (index < modules.size()) {
                primary_module_index = index;
            }
        }

    protected:

        /**
         * Creating a multi-BMI-module formulation from NGen config.
         *
         * @param properties
         * @param needs_param_validation
         */
        void create_multi_formulation(geojson::PropertyMap properties, bool needs_param_validation);

        template<class T>
        double get_module_var_value_as_double(const std::string &var_name, std::shared_ptr<Bmi_Formulation> mod) {
            std::shared_ptr<T> module = std::static_pointer_cast<T>(mod);
            return module->get_var_value_as_double(var_name);
        }

        /**
         * Initialize a nested formulation from the given properties and update multi formulation metadata.
         *
         * This function creates a new formulation, processes the mapping of BMI variables, and adds outputs to the outer
         * module's provideable data items.
         *
         * Note that it is VERY IMPORTANT that ``properties`` argument`` is provided by value, as this copy is
         * potentially updated to perform per-feature pattern substitution for certain property element values.
         *
         * @tparam T The particular type for the nested formulation object.
         * @param mod_index The index for the new formulation in this instance's collection of nested formulations.
         * @param identifier The id of for the represented feature.
         * @param properties A COPY of the nested module config properties for the nested formulation of interest.
         * @return
         */
        template<class T>
        std::shared_ptr<T> init_nested_module(int mod_index, std::string identifier, geojson::PropertyMap properties) {
            std::unique_ptr<forcing::ForcingProvider> wfp = std::make_unique<forcing::WrappedForcingProvider>(this);
            std::shared_ptr<T> mod = std::make_shared<T>(identifier, std::move(wfp), output);

            // Since this is a nested formulation, support usage of the '{{id}}' syntax for init config file paths.
            Catchment_Formulation::config_pattern_substitution(properties, BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG,
                                                               "{{id}}", id);

            // Call create_formulation to perform the rest of the typical initialization steps for the formulation.
            mod->create_formulation(properties);

            // Set this up for placing in the module_variable_maps member variable
            std::shared_ptr<std::map<std::string, std::string>> var_aliases;
            var_aliases = std::make_shared<std::map<std::string, std::string>>(std::map<std::string, std::string>());
            for (const std::string &var_name : mod->get_bmi_input_variables()) {
                std::string framework_alias = mod->get_config_mapped_variable_name(var_name);
                (*var_aliases)[framework_alias] = var_name;
                // If framework_name is not in collection from which we have available data sources ...
                if (availableData.count(framework_alias) != 1) {
                    throw std::runtime_error(
                            "Multi BMI cannot be created with module " + mod->get_model_type_name() + " with input " +
                            "variable " + framework_alias +
                            (var_name == framework_alias ? "" : " (an alias of BMI variable " + var_name + ")") +
                            " when there is no previously-enabled source for this input.");
                }
                else {
                    mod->input_forcing_providers[var_name] = availableData[framework_alias];
                    mod->input_forcing_providers[framework_alias] = availableData[framework_alias];
                }
            }

            // Also add the output variable aliases
            for (const std::string &var_name : mod->get_bmi_output_variables()) {
                std::string framework_alias = mod->get_config_mapped_variable_name(var_name);
                (*var_aliases)[framework_alias] = var_name;
                if (availableData.count(framework_alias) > 0) {
                    throw std::runtime_error(
                            "Multi BMI cannot be created with module " + mod->get_model_type_name() +
                            " with output variable " + framework_alias +
                            (var_name == framework_alias ? "" : " (an alias of BMI variable " + var_name + ")") +
                            " because a previous module is using this output variable name/alias.");
                }
                availableData[framework_alias] = mod;
            }
            module_variable_maps[mod_index] = var_aliases;
            return mod;
        }

        /**
         * A mapping of data properties to their providers.
         *
         * Keys for the data properties are unique, name-like identifiers.  These could be a BMI output variable name
         * for a module, or a configuration-mapped alias of such a variable.  The intent is for any module needing any
         * input data to also have either an input variable name or input variable name mapping identical to one of
         * these keys (and though ordering is important at a higher level, it is not handled directly by this member).
         */
        std::map<std::string, std::shared_ptr<forcing::ForcingProvider>> availableData;

    private:

        /** The set of available "forcings" (output variables, plus their mapped aliases) this instance can provide. */
        std::vector<std::string> available_forcings;
        /**
         * Whether the @ref Bmi_Formulation::output_variable_names value is just the analogous value from this
         * instance's final nested module.
         */
        bool is_out_vars_from_last_mod = false;
        /** The nested BMI modules composing this multi-module formulation, in their order of execution. */
        std::vector<nested_module_ptr> modules;
        std::vector<std::string> module_types;
        /**
         * Per-module maps (ordered as in @ref modules) of configuration-mapped names to BMI variable names.
         */
        // TODO: confirm that we actually need this for something
        std::vector<std::shared_ptr<std::map<std::string, std::string>>> module_variable_maps;
        /** Index value (0-based) of the time step that will be processed by the next update of the model. */
        int next_time_step_index = 0;
        /** The index of the "primary" nested module, used when functionality is deferred to a particular module's behavior. */
        int primary_module_index = -1;

        friend Bmi_Multi_Formulation_Test;

    };
}

#endif //NGEN_BMI_MULTI_FORMULATION_HPP
