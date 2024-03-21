#include "Bmi_Module_Formulation.hpp"

namespace realization {
        void Bmi_Module_Formulation::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
            geojson::PropertyMap options = this->interpret_parameters(config, global);
            inner_create_formulation(options, false);
        }

        void Bmi_Module_Formulation::create_formulation(geojson::PropertyMap properties) {
            inner_create_formulation(properties, true);
        }

        boost::span<const std::string> Bmi_Module_Formulation::get_available_variable_names() {
            if (is_model_initialized() && available_forcings.empty()) {
                for (const std::string &output_var_name : get_bmi_model()->GetOutputVarNames()) {
                    available_forcings.push_back(output_var_name);
                    if (bmi_var_names_map.find(output_var_name) != bmi_var_names_map.end())
                        available_forcings.push_back(bmi_var_names_map[output_var_name]);
                }
            }
            return available_forcings;
        }

        std::string Bmi_Module_Formulation::get_output_line_for_timestep(int timestep, std::string delimiter) {
            // TODO: something must be added to store values if more than the current time step is wanted
            // TODO: if such a thing is added, it should probably be configurable to turn it off
            if (timestep != (next_time_step_index - 1)) {
                throw std::invalid_argument("Only current time step valid when getting output for BMI C++ formulation");
            }
            std::string output_str;

            for (const std::string& name : get_output_variable_names()) {
                output_str += (output_str.empty() ? "" : ",") + std::to_string(get_var_value_as_double(name));
            }
            return output_str;
        }

        time_t Bmi_Module_Formulation::get_variable_time_begin(const std::string &variable_name) {
            // TODO: come back and implement if actually necessary for this type; for now don't use
            throw std::runtime_error("Bmi_Modular_Formulation does not yet implement get_variable_time_begin");
        }

        long Bmi_Module_Formulation::get_data_start_time()
        {
            return this->get_bmi_model()->GetStartTime();
        }

        long Bmi_Module_Formulation::get_data_stop_time() {
            // TODO: come back and implement if actually necessary for this type; for now don't use
            throw std::runtime_error("Bmi_Module_Formulation does not yet implement get_data_stop_time");
        }

        long Bmi_Module_Formulation::record_duration() {
            throw std::runtime_error("Bmi_Module_Formulation does not yet implement record_duration");
        }

        const double Bmi_Module_Formulation::get_model_current_time() {
            return get_bmi_model()->GetCurrentTime();
        }

        const double Bmi_Module_Formulation::get_model_end_time() {
            return get_bmi_model()->GetEndTime();
        }

        const std::vector<std::string>& Bmi_Module_Formulation::get_required_parameters() {
            return REQUIRED_PARAMETERS;
        }

        const std::string& Bmi_Module_Formulation::get_config_mapped_variable_name(const std::string &model_var_name) {
            // TODO: need to introduce validation elsewhere that all mapped names are valid AORC field constants.
            if (bmi_var_names_map.find(model_var_name) != bmi_var_names_map.end())
                return bmi_var_names_map[model_var_name];
            else
                return model_var_name;
        }

        size_t Bmi_Module_Formulation::get_ts_index_for_time(const time_t &epoch_time) {
            // TODO: come back and implement if actually necessary for this type; for now don't use
            throw std::runtime_error("Bmi_Singular_Formulation does not yet implement get_ts_index_for_time");
        }

        std::vector<double> Bmi_Module_Formulation::get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m)
        {
            std::string output_name = selector.get_variable_name();
            time_t init_time = selector.get_init_time();
            long duration_s = selector.get_duration_secs();
            std::string output_units = selector.get_output_units();

            // First make sure this is an available output
            auto forcing_outputs = get_available_variable_names();
            if (std::find(forcing_outputs.begin(), forcing_outputs.end(), output_name) == forcing_outputs.end()) {
                throw std::runtime_error(get_formulation_type() + " received invalid output forcing name " + output_name);
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
                    #ifndef UDUNITS_QUIET
                    logging::warning((std::string("WARN: Unit conversion unsuccessful - Returning unconverted value! (\"")+e.what()+"\")\n").c_str());
                    #endif
                    return values;
                }
            }
            //This is unlikely (impossible?) to throw since a pre-check on available names is done above. Assert instead?
            throw std::runtime_error(get_formulation_type() + " received invalid output forcing name " + output_name);
        }

        double Bmi_Module_Formulation::get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m)
        {
            std::string output_name = selector.get_variable_name();
            time_t init_time = selector.get_init_time();
            long duration_s = selector.get_duration_secs();
            std::string output_units = selector.get_output_units();

            // First make sure this is an available output
            auto forcing_outputs = get_available_variable_names();
            if (std::find(forcing_outputs.begin(), forcing_outputs.end(), output_name) == forcing_outputs.end()) {
                throw std::runtime_error(get_formulation_type() + " received invalid output forcing name " + output_name);
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
                    #ifndef UDUNITS_QUIET
                    std::cerr<<"WARN: Unit conversion unsuccessful - Returning unconverted value! (\""<<e.what()<<"\")"<<std::endl;
                    #endif
                    return value;
                }
            }

            //This is unlikely (impossible?) to throw since a pre-check on available names is done above. Assert instead?
            throw std::runtime_error(get_formulation_type() + " received invalid output forcing name " + output_name);
        }


        static bool is_var_name_in_collection(const std::vector<std::string> &all_names, const std::string &var_name) {
            return std::count(all_names.begin(), all_names.end(), var_name) > 0;
        }

        bool Bmi_Module_Formulation::is_bmi_input_variable(const std::string &var_name) {
           return is_var_name_in_collection(get_bmi_input_variables(), var_name);
        }

        bool Bmi_Module_Formulation::is_bmi_output_variable(const std::string &var_name) {
            return is_var_name_in_collection(get_bmi_output_variables(), var_name);
        }

        bool Bmi_Module_Formulation::is_property_sum_over_time_step(const std::string& name) {
            // TODO: verify with some kind of proof that "always true" is appropriate
            return true;
        }

        const std::vector<std::string> Bmi_Module_Formulation::get_bmi_input_variables() {
            return get_bmi_model()->GetInputVarNames();
        }

        const std::vector<std::string> Bmi_Module_Formulation::get_bmi_output_variables() {
            return get_bmi_model()->GetOutputVarNames();
        }
}
