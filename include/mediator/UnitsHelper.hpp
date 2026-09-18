#ifndef NGEN_UNITSHELPER_H
#define NGEN_UNITSHELPER_H

#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include "all.h"

void init_unit_system();

class UnitsHelper {

    public:

    /**
     * A pair of normalized units
     *
     * This struct encapsulates the noneish semantics of unit strings, 
     * normalizing various representations of "no units" for consistent handling in conversion checks.
     */
    struct normalized_units {
        /** Reduce @p in_units and @p out_units to the form the questions below are asked of. */
        normalized_units(std::string in_units, std::string out_units);

        /**
         * Whether converting between these would leave every value exactly as it is.
         *
         * True in two cases:
         * 1. The output units are empty (no units stated).
         * 2. The input and output units are identical.
         */
        bool is_identity() const { return out.empty() || in == out; }

        std::string in;
        std::string out;
    };

    static double get_converted_value(const normalized_units &units, const double &value);

    static double get_converted_value(const std::string &in_units, const double &value, const std::string &out_units) {
        return get_converted_value(normalized_units(in_units, out_units), value);
    }

    static double* convert_values(const normalized_units &units, double* values, double* out_values, const size_t & count);

    static double* convert_values(const std::string &in_units, double* values, const std::string &out_units, double* out_values, const size_t & count) {
        return convert_values(normalized_units(in_units, out_units), values, out_values, count);
    }

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

    /** Record the observation of a failed unit conversion attempt, and report whether it's new */
    static bool record_unit_conversion_fault(unit_conversion_exception const& uce, std::string const& requester_name, std::string const& requester_variable);

    private:

    struct unit_error_log_key {
        std::string requester_name;
        std::string requester_variable;
        std::string provider_name;
        std::string provider_variable;
        std::string failure_message;

        bool operator<(unit_error_log_key const& rhs) const {
            return std::tie(requester_name, requester_variable, provider_name, provider_variable, failure_message)
                 < std::tie(rhs.requester_name, rhs.requester_variable, rhs.provider_name, rhs.provider_variable, rhs.failure_message);
        }

        bool operator==(unit_error_log_key const& rhs) const {
            return std::tie(requester_name, requester_variable, provider_name, provider_variable, failure_message)
                == std::tie(rhs.requester_name, rhs.requester_variable, rhs.provider_name, rhs.provider_variable, rhs.failure_message);
        }
    };

    static std::set<unit_error_log_key> unit_errors_reported;
};

#endif //NGEN_UNITSHELPER_H
