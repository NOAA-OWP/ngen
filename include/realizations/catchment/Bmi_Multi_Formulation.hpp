#ifndef NGEN_BMI_MULTI_FORMULATION_HPP
#define NGEN_BMI_MULTI_FORMULATION_HPP

#include <map>
#include <vector>
#include "Bmi_Formulation.hpp"
#include "Bmi_Module_Formulation.hpp"
#include "Bmi_C_Formulation.hpp"
#include "bmi.hpp"
#include "ForcingProvider.hpp"

#define BMI_REALIZATION_CFG_PARAM_REQ__MODULES "modules"

using namespace std;

namespace realization {

    /**
     * Abstraction of a formulation with multiple backing model object that implements the BMI.
     *
     * @tparam M The type for the backing BMI model object.
     */
    class Bmi_Multi_Formulation : public Bmi_Formulation {

    public:

        /**
         * Minimal constructor for objects initialize using the Formulation_Manager and subsequent calls to
         * ``create_formulation``.
         *
         * @param id
         * @param forcing_config
         * @param output_stream
         */
        Bmi_Multi_Formulation(string id, forcing_params forcing_config, utils::StreamHandler output_stream)
                : Bmi_Formulation(move(id), move(forcing_config), output_stream) { };

        virtual ~Bmi_Multi_Formulation() {};

        void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override {
            geojson::PropertyMap options = this->interpret_parameters(config, global);
            create_multi_formulation(options, false);
        }

        void create_formulation(geojson::PropertyMap properties) override {
            create_multi_formulation(properties, true);
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
            // If not found ...
            if (availableData.empty() || availableData.find(forcing_name) == availableData.end()) {
                throw runtime_error(get_formulation_type() + " cannot get output time for unknown " + forcing_name);
            }
            return availableData[forcing_name]->get_forcing_output_time_begin(forcing_name);
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
            if (availableData.empty() || availableData.find(forcing_name) == availableData.end()) {
                throw runtime_error(get_formulation_type() + " cannot get output time for unknown " + forcing_name);
            }
            return availableData[forcing_name]->get_forcing_output_time_begin(forcing_name);
        }

        /**
         * Get the names of variables in formulation output.
         *
         * Get the names of the variables to include in the output from this formulation, which should be some ordered
         * subset of the output variables from the model.
         *
         * For this type, these will be the analogous values for the last of the contained modules.
         *
         * @return
         */
        const vector<string> &get_output_variable_names() const override;

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
        size_t get_ts_index_for_time(const time_t &epoch_time) {
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
                         const std::string &output_units)
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
         * Get whether this time step goes beyond this formulations (i.e., any of it's modules') end time.
         *
         * @param t_index The time step index in question.
         * @return Whether this time step goes beyond this formulations (i.e., any of it's modules') end time.
         */
        bool is_time_step_beyond_end_time(time_step_t t_index);

    protected:

        /**
         * Creating a multi-BMI-module formulation from NGen config.
         *
         * @param properties
         * @param needs_param_validation
         */
        void create_multi_formulation(geojson::PropertyMap properties, bool needs_param_validation);

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

        std::vector<std::shared_ptr<Bmi_Module_Formulation<bmi::Bmi>>> modules;
        /**
         * Per-module maps (order as in @ref modules) of configuration-mapped names to BMI variable names.
         */
        std::vector<std::shared_ptr<std::map<std::string, std::string>>> module_variable_maps;
        /** Index value (0-based) of the time step that will be processed by the next update of the model. */
        int next_time_step_index = 0;


    };
}

#endif //NGEN_BMI_MULTI_FORMULATION_HPP
