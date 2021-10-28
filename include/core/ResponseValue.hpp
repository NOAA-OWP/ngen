#ifndef RESPONSEVALUE_H
#define RESPONSEVALUE_H

#include <map>
#include <string>


namespace responsevalue {

    class ResponseValue {
        public:

            /**
             * @brief Construct a new empty ResponseValue object
             * 
             */
            ResponseValue() {}

            /**
             * @brief Destroy the ResponseValue object
             * 
             */
            virtual ~ResponseValue(){}

            virtual void set_value(const std::string& name, double value);
            virtual void set_primary(double value);
            virtual void set_primary_name(const std::string& name);
            virtual void set_input_var_units(const std::string& name, const std::string& units);
            virtual void set_output_var_units(const std::string& name, const std::string& units);

            //std::string get_var_units(std::string name);
            virtual double get_value(const std::string& name);
            virtual double get_primary();
            virtual std::string get_primary_name();
            virtual std::string get_input_var_units(const std::string& name);
            virtual std::string get_output_var_units(const std::string& name);

        protected:

            struct _ValueData {
                double value = 0.0;
                bool is_set = false;
                std::string input_units = "";
                std::string output_units = "";
                bool units_match = true; // So, std::string::== is linear time... so this might speed things up.
            };

            _ValueData data;

            virtual double handle_unit_conversion(const std::string& input_units, const std::string& output_units, double value);

        private:
            double primary = 0.0;
            std::string primary_name = "";
            std::map<std::string, _ValueData> value_data = {} ;
    };

}

#endif //RESPONSEVALUE_H