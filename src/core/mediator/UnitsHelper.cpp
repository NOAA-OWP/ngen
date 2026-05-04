#include "UnitsHelper.hpp"
#include <cstring>
#include <mutex>
#include <string>

ut_system* UnitsHelper::unit_system;
std::once_flag UnitsHelper::unit_system_inited;
std::map<std::string, std::shared_ptr<cv_converter>> UnitsHelper::converters;
std::mutex UnitsHelper::converters_mutex;

std::shared_ptr<cv_converter> UnitsHelper::get_converter(const std::string& in_units, const std::string& out_units, utEncoding in_encoding, utEncoding out_encoding ){
    if(in_units == "") {
        unit_conversion_exception uce{"Requested conversion from empty input units string", in_units, out_units};
        throw uce;
    }

    if(out_units == "") {
        unit_conversion_exception uce{"Requested conversion to empty output units string", in_units, out_units};
        throw uce;
    }

    const std::lock_guard<std::mutex> lock(converters_mutex);

    std::string key = in_units + "|" + out_units; //Better solution? Good enough? Bother with nested maps?
    if(converters.count(key) == 1){
        if(converters[key] == nullptr){
            // Recurrence of last throw case below
            unit_conversion_exception uce{"Unable to convert as requested (repeated)", in_units, out_units};
            throw uce;
        }
        return converters[key];
    } else {
        ut_unit* from = ut_parse(unit_system, in_units.c_str(), in_encoding);
        if (from == NULL)
        {
            unit_conversion_exception uce{"Unable to parse in_units", in_units, out_units};
            throw uce;
        }
        ut_unit* to = ut_parse(unit_system, out_units.c_str(), out_encoding);
        if (to == NULL)
        {
            ut_free(from);

            unit_conversion_exception uce{"Unable to parse out_units", in_units, out_units};
            throw uce;
        }

        cv_converter* conv = ut_get_converter(from, to);
        if (conv == NULL)
        {
            ut_free(from);
            ut_free(to);
            converters[key] = nullptr;

            unit_conversion_exception uce{"Unable to convert as requested", in_units, out_units};
            throw uce;
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

static bool is_noneish(std::string const& u) {
    return u.empty() || u == "none" || u == "unitless" || u == "dimensionless" || u == "-";
}

static void normalize_units(std::string& in, std::string& out) {
    // Normalize input units: map none-ish → "1"
    if (is_noneish(in))
        in = "1";

    // Normalize requested units:
    //  - none-ish → "1" if input is "1"; otherwise "" (unspecified → skip conversion)
    if (is_noneish(out)) {
        out = (in == "1") ? std::string("1") : std::string("");
    }
}

static bool short_circuit_conversion(std::string const& in, std::string const& out) {
    return out.empty() || in == out;
}

double UnitsHelper::get_converted_value(const std::string &in_units, const double &value, const std::string &out_units)
{
    std::string in_norm = in_units;
    std::string out_norm = out_units;
    normalize_units(in_norm, out_norm);

    if (short_circuit_conversion(in_norm, out_norm)) {
        return value;
    }

    std::call_once(unit_system_inited, init_unit_system);

    try {
        auto converter = get_converter(in_norm, out_norm);
        double r = cv_convert_double(converter.get(), value);
        return r;
    } catch (unit_conversion_exception& uce) {
        uce.unconverted_values.push_back(value);
        throw;
    }
}

double* UnitsHelper::convert_values(const std::string &in_units, double* in_values, const std::string &out_units, double* out_values, const size_t& count)
{
    std::string in_norm = in_units;
    std::string out_norm = out_units;
    normalize_units(in_norm, out_norm);

    if (short_circuit_conversion(in_norm, out_norm)) {
        if (in_values == out_values) {
            return in_values;
        } else {
            std::memcpy(out_values, in_values, sizeof(double)*count);
            return out_values;
        }
    }

    std::call_once(unit_system_inited, init_unit_system);

    // Don't catch the UCE here to fill in uce.unconverted_values,
    // because the caller may be able to more efficiently std::move it
    auto converter = get_converter(in_norm, out_norm);
    cv_convert_doubles(converter.get(), in_values, count, out_values);
    return out_values;
}
