#include "Bmi_Module_Formulation.hpp"
#include "utilities/logging_utils.h"
#include <UnitsHelper.hpp>

namespace realization {
        void Bmi_Module_Formulation::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
            geojson::PropertyMap options = this->interpret_parameters(config, global);
            inner_create_formulation(options, false);
        }

        void Bmi_Module_Formulation::create_formulation(geojson::PropertyMap properties) {
            inner_create_formulation(properties, true);
        }

        boost::span<const std::string> Bmi_Module_Formulation::get_available_variable_names() const {
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
                output_str += (output_str.empty() ? "" : ",") + std::to_string(get_var_value_as_double(0, name));
            }
            return output_str;
        }

        double Bmi_Module_Formulation::get_response(time_step_t t_index, time_step_t t_delta) {
            if (get_bmi_model() == nullptr) {
                throw std::runtime_error("Trying to process response of improperly created BMI formulation of type '" + get_formulation_type() + "'.");
            }
            if (t_index < 0) {
                throw std::invalid_argument("Getting response of negative time step in BMI formulation of type '" + get_formulation_type() + "' is not allowed.");
            }
            // Use (next_time_step_index - 1) so that second call with current time step index still works
            if (t_index < (next_time_step_index - 1)) {
                // TODO: consider whether we should (optionally) store and return historic values
                throw std::invalid_argument("Getting response of previous time step in BMI formulation of type '" + get_formulation_type() + "' is not allowed.");
            }

            // The time step delta size, expressed in the units internally used by the model
            double t_delta_model_units;
            if (next_time_step_index <= t_index) {
                t_delta_model_units = get_bmi_model()->convert_seconds_to_model_time((double)t_delta);
                double model_time = get_bmi_model()->GetCurrentTime();
                // Also, before running, make sure this doesn't cause a problem with model end_time
                if (!get_allow_model_exceed_end_time()) {
                    int total_time_steps_to_process = abs((int)t_index - next_time_step_index) + 1;
                    if (get_bmi_model()->GetEndTime() < (model_time + (t_delta_model_units * total_time_steps_to_process))) {
                        throw std::invalid_argument("Cannot process BMI formulation of type '" + get_formulation_type() + "' to get response of future time step "
                                                    "that exceeds model end time.");
                    }
                }
            }

            while (next_time_step_index <= t_index) {
                double model_initial_time = get_bmi_model()->GetCurrentTime();
                set_model_inputs_prior_to_update(model_initial_time, t_delta);
                if (t_delta_model_units == get_bmi_model()->GetTimeStep())
                    get_bmi_model()->Update();
                else
                    get_bmi_model()->UpdateUntil(model_initial_time + t_delta_model_units);
                // TODO: again, consider whether we should store any historic response, ts_delta, or other var values
                next_time_step_index++;
            }
            return get_var_value_as_double(0, get_bmi_main_output_var());
        }

        time_t Bmi_Module_Formulation::get_variable_time_begin(const std::string &variable_name) {
            // TODO: come back and implement if actually necessary for this type; for now don't use
            throw std::runtime_error("Bmi_Modular_Formulation does not yet implement get_variable_time_begin");
        }

        long Bmi_Module_Formulation::get_data_start_time() const
        {
            return this->get_bmi_model()->GetStartTime();
        }

        long Bmi_Module_Formulation::get_data_stop_time() const {
            // TODO: come back and implement if actually necessary for this type; for now don't use
            throw std::runtime_error("Bmi_Module_Formulation does not yet implement get_data_stop_time");
        }

        long Bmi_Module_Formulation::record_duration() const {
            throw std::runtime_error("Bmi_Module_Formulation does not yet implement record_duration");
        }

        const double Bmi_Module_Formulation::get_model_current_time() const {
            return get_bmi_model()->GetCurrentTime();
        }

        const double Bmi_Module_Formulation::get_model_end_time() const {
            return get_bmi_model()->GetEndTime();
        }

        const std::vector<std::string>& Bmi_Module_Formulation::get_required_parameters() const {
            return REQUIRED_PARAMETERS;
        }

        const std::string& Bmi_Module_Formulation::get_config_mapped_variable_name(const std::string &model_var_name) const {
            // TODO: need to introduce validation elsewhere that all mapped names are valid AORC field constants.
            if (bmi_var_names_map.find(model_var_name) != bmi_var_names_map.end())
                return bmi_var_names_map.at(model_var_name);
            else
                return model_var_name;
        }

        size_t Bmi_Module_Formulation::get_ts_index_for_time(const time_t &epoch_time) const {
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
                double value = get_var_value_as_double(0, bmi_var_name);

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

        bool Bmi_Module_Formulation::is_bmi_input_variable(const std::string &var_name) const {
           return is_var_name_in_collection(get_bmi_input_variables(), var_name);
        }

        bool Bmi_Module_Formulation::is_bmi_output_variable(const std::string &var_name) const {
            return is_var_name_in_collection(get_bmi_output_variables(), var_name);
        }

        bool Bmi_Module_Formulation::is_property_sum_over_time_step(const std::string& name) const {
            // TODO: verify with some kind of proof that "always true" is appropriate
            return true;
        }

        const std::vector<std::string> Bmi_Module_Formulation::get_bmi_input_variables() const {
            return get_bmi_model()->GetInputVarNames();
        }

        const std::vector<std::string> Bmi_Module_Formulation::get_bmi_output_variables() const {
            return get_bmi_model()->GetOutputVarNames();
        }

        void Bmi_Module_Formulation::get_bmi_output_var_name(const std::string &name, std::string &bmi_var_name)
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

        void Bmi_Module_Formulation::determine_model_time_offset() {
            set_bmi_model_start_time_forcing_offset_s(
                    // TODO: Look at making this epoch start configurable instead of from forcing
                    forcing->get_data_start_time() - convert_model_time(get_bmi_model()->GetStartTime()));
        }

        const bool& Bmi_Module_Formulation::get_allow_model_exceed_end_time() const {
            return allow_model_exceed_end_time;
        }

        const std::string& Bmi_Module_Formulation::get_bmi_init_config() const {
            return bmi_init_config;
        }

        std::shared_ptr<models::bmi::Bmi_Adapter> Bmi_Module_Formulation::get_bmi_model() const {
            return bmi_model;
        }

        const time_t& Bmi_Module_Formulation::get_bmi_model_start_time_forcing_offset_s() const {
            return bmi_model_start_time_forcing_offset_s;
        }

        void Bmi_Module_Formulation::inner_create_formulation(geojson::PropertyMap properties, bool needs_param_validation) {
            if (needs_param_validation) {
                validate_parameters(properties);
            }
            // Required parameters first
            set_bmi_init_config(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__INIT_CONFIG).as_string());
            set_bmi_main_output_var(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MAIN_OUT_VAR).as_string());
            set_model_type_name(properties.at(BMI_REALIZATION_CFG_PARAM_REQ__MODEL_TYPE).as_string());

            // Then optional ...

            auto uses_forcings_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__USES_FORCINGS);
            if (uses_forcings_it != properties.end() && uses_forcings_it->second.as_boolean()) {
                throw std::runtime_error("The '" BMI_REALIZATION_CFG_PARAM_OPT__USES_FORCINGS "' parameter was removed and cannot be set");
            }

            auto forcing_file_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__FORCING_FILE);
            if (forcing_file_it != properties.end() && forcing_file_it->second.as_string() != "") {
                throw std::runtime_error("The '" BMI_REALIZATION_CFG_PARAM_OPT__FORCING_FILE "' parameter was removed and cannot be set " + forcing_file_it->second.as_string());
            }

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

            // Output precision, if present
            auto out_precision_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION);
            if (out_precision_it != properties.end()) {
                set_output_precision(properties.at(BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION).as_natural_number());
            }

            // Finally, make sure this is set
            model_initialized = get_bmi_model()->is_model_initialized();

            // Get output variable names
            if (model_initialized) {
                for (const std::string &output_var_name : get_bmi_model()->GetOutputVarNames()) {
                    available_forcings.push_back(output_var_name);
                    if (bmi_var_names_map.find(output_var_name) != bmi_var_names_map.end())
                        available_forcings.push_back(bmi_var_names_map[output_var_name]);
                }
            }
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

            throw std::runtime_error("Unable to get values of iterable as type" + type +
                " : no logic for converting values to variable's type.");
        }


        void Bmi_Module_Formulation::set_initial_bmi_parameters(geojson::PropertyMap properties) {
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
                            break;
                        case geojson::PropertyType::Real:
                            param.second.as_vector(double_vec);
                            value_ptr = get_values_as_type(type, double_vec.begin(), double_vec.end());
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
                                logging::warning(("Cannot pass non-numeric lists as a BMI parameter, skipping "+param.first+"\n").c_str());
                                continue;
                            }
                            value_ptr = get_values_as_type(type, double_vec.begin(), double_vec.end());
                            break;
                        default:
                            logging::warning(("Cannot pass parameter of type "+geojson::get_propertytype_name(param.second.get_type())+" as a BMI parameter, skipping "+param.first+"\n").c_str());
                            continue;
                    }
                    try{

                        // Finally, use the value obtained to set the model param
                        get_bmi_model()->SetValue(param.first, value_ptr.get());
                    }
                    catch (const std::exception &e)
                    {
                        logging::warning((std::string("Exception setting parameter value: ")+e.what()).c_str());
                        logging::warning(("Skipping parameter: "+param.first+"\n").c_str());
                    }
                    catch (...)
                    {
                        logging::warning((std::string("Unknown Exception setting parameter value: \n")).c_str());
                        logging::warning(("Skipping parameter: "+param.first+"\n").c_str());
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
        bool Bmi_Module_Formulation::is_bmi_model_time_step_fixed() const {
            return bmi_model_time_step_fixed;
        }

        bool Bmi_Module_Formulation::is_model_initialized() const {
            return model_initialized;
        }

        void Bmi_Module_Formulation::set_allow_model_exceed_end_time(bool allow_exceed_end) {
            allow_model_exceed_end_time = allow_exceed_end;
        }

        void Bmi_Module_Formulation::set_bmi_init_config(const std::string &init_config) {
            bmi_init_config = init_config;
        }
        void Bmi_Module_Formulation::set_bmi_model(std::shared_ptr<models::bmi::Bmi_Adapter> model) {
            bmi_model = model;
        }

        void Bmi_Module_Formulation::set_bmi_model_start_time_forcing_offset_s(const time_t &offset_s) {
            bmi_model_start_time_forcing_offset_s = offset_s;
        }

        void Bmi_Module_Formulation::set_bmi_model_time_step_fixed(bool is_fix_time_step) {
            bmi_model_time_step_fixed = is_fix_time_step;
        }

        void Bmi_Module_Formulation::set_model_initialized(bool is_initialized) {
            model_initialized = is_initialized;
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

            throw std::runtime_error("Unable to get value of variable as type '" + type +
                "': no logic for converting value to variable's type.");
        }

        void Bmi_Module_Formulation::set_model_inputs_prior_to_update(const double &model_init_time, time_step_t t_delta) {
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
                int nbytes = get_bmi_model()->GetVarNbytes(var_name);
                int varItemSize = get_bmi_model()->GetVarItemsize(var_name);
                int numItems = nbytes / varItemSize;
                assert(nbytes % varItemSize == 0);

                std::shared_ptr<void> value_ptr;
                // Finally, use the value obtained to set the model input
                std::string type = get_bmi_model()->get_analogous_cxx_type(get_bmi_model()->GetVarType(var_name),
                                                                           varItemSize);
                if (numItems != 1) {
                    //more than a single value needed for var_name
                    auto values = provider->get_values(CatchmentAggrDataSelector(this->get_catchment_id(),var_map_alias, model_epoch_time, t_delta,
                                                   get_bmi_model()->GetVarUnits(var_name)));
                    //need to marshal data types to the receiver as well
                    //this could be done a little more elegantly if the provider interface were
                    //"type aware", but for now, this will do (but requires yet another copy)
                    if(values.size() == 1){
                        //FIXME this isn't generic broadcasting, but works for scalar implementations
                        #ifndef NGEN_QUIET
                        std::cerr << "WARN: broadcasting variable '" << var_name << "' from scalar to expected array\n";
                        #endif
                        values.resize(numItems, values[0]);
                    } else if (values.size() != numItems) {
                        throw std::runtime_error("Mismatch in item count for variable '" + var_name + "': model expects " +
                                                 std::to_string(numItems) + ", provider returned " + std::to_string(values.size()) +
                                                 " items\n");
                    }
                    value_ptr = get_values_as_type( type, values.begin(), values.end() );

                } else {
                    //scalar value
                    double value = provider->get_value(CatchmentAggrDataSelector(this->get_catchment_id(),var_map_alias, model_epoch_time, t_delta,
                                                   get_bmi_model()->GetVarUnits(var_name)));
                    value_ptr = get_value_as_type(type, value);
                }
                get_bmi_model()->SetValue(var_name, value_ptr.get());
            }
        }
}
