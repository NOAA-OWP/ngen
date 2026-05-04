#ifndef NGEN_UNITSHELPER_H
#define NGEN_UNITSHELPER_H

#include <stdexcept>
#include <string>
#include <vector>
#include "all.h"

class UnitsHelper {

    public:

    static double get_converted_value(const std::string &in_units, const double &value, const std::string &out_units);

    static double* convert_values(const std::string &in_units, double* values, const std::string &out_units, double* out_values, const size_t & count);

    struct unit_conversion_exception : public std::runtime_error {
        unit_conversion_exception(std::string message) : std::runtime_error(message) {}
        unit_conversion_exception(std::string const& message, std::string const& in_units, std::string const& out_units)
            : std::runtime_error(message)
            , provider_units(in_units)
            , to_units(out_units)
        {}

        std::string provider_model_name;
        std::string provider_var_name;
        std::string provider_units;
        std::string to_units;
        std::vector<double> unconverted_values;
    };
};

#endif //NGEN_UNITSHELPER_H
