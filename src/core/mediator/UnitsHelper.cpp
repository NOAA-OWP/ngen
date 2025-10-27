#include "UnitsHelper.hpp"
#include <cstring>
#include <mutex>

ut_system* UnitsHelper::unit_system;
std::once_flag UnitsHelper::unit_system_inited;
std::map<std::string, std::shared_ptr<cv_converter>> UnitsHelper::converters;
std::mutex UnitsHelper::converters_mutex;

std::shared_ptr<cv_converter> UnitsHelper::get_converter(const std::string& in_units, const std::string& out_units, utEncoding in_encoding, utEncoding out_encoding ){
    if(in_units == "" || out_units == ""){
        throw std::runtime_error("Unable to process empty units value for pairing \"" + in_units + "\" \"" + out_units + "\"");
    }

    const std::lock_guard<std::mutex> lock(converters_mutex);

    std::string key = in_units + "|" + out_units; //Better solution? Good enough? Bother with nested maps?
    if(converters.count(key) == 1){
        if(converters[key] == nullptr){
            // same as last throw below
            throw std::runtime_error("Unable to convert " + in_units + " to " + out_units);
        }
        return converters[key];
    } else {
        ut_unit* from = ut_parse(unit_system, in_units.c_str(), in_encoding);
        if (from == NULL)
        {
            throw std::runtime_error("Unable to parse in_units value " + in_units);
        }
        ut_unit* to = ut_parse(unit_system, out_units.c_str(), out_encoding);
        if (to == NULL)
        {
            ut_free(from);
            throw std::runtime_error("Unable to parse out_units value " + out_units);

        }
        cv_converter* conv = ut_get_converter(from, to);
        if (conv == NULL)
        {
            ut_free(from);
            ut_free(to);
            converters[key] = nullptr;
            throw std::runtime_error("Unable to convert " + in_units + " to " + out_units);
        }
        auto c = std::shared_ptr<cv_converter>(
            conv,
            [from,to](cv_converter* p) {
                cv_free(p);
                ut_free(from); // Captured via closure!
                ut_free(to); // Captured via closure!
            }
        );
        converters[key] = c;

        return c;
    }
}

double UnitsHelper::get_converted_value(const std::string &in_units, const double &value, const std::string &out_units)
{
    if(in_units == out_units){
        return value; // Early-out optimization
    }
    std::call_once(unit_system_inited, init_unit_system);

    auto converter = get_converter(in_units, out_units);

    double r = cv_convert_double(converter.get(), value);
    return r;
}

double* UnitsHelper::convert_values(const std::string &in_units, double* in_values, const std::string &out_units, double* out_values, const size_t& count)
{
    auto is_noneish = [](const std::string& u)->bool {
        return u.empty() || u == "none" || u == "unitless" || u == "dimensionless" || u == "-";
    };

    // Normalize input units: map none-ish → "1"
    const std::string in_norm = is_noneish(in_units) ? std::string("1") : in_units;

    // Normalize requested units:
    //  - none-ish → "1" if input is "1"; otherwise "" (unspecified → skip conversion)
    std::string out_norm;
    if (is_noneish(out_units)) {
        out_norm = (in_norm == "1") ? std::string("1") : std::string("");
    }
    else {
        out_norm = out_units;
    }

    // Early outs (no UDUNITS parsing or converter creation)
    if (out_norm.empty() || in_norm == out_norm) {
        // Pass-through
        if (in_values == out_values) {
            return in_values;
        } else {
            std::memcpy(out_values, in_values, sizeof(double)*count);
            return out_values;
        }
    }

    std::call_once(unit_system_inited, init_unit_system);

    auto converter = get_converter(in_norm, out_norm);

    cv_convert_doubles(converter.get(), in_values, count, out_values);

    return out_values;
}
