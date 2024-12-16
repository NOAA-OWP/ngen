#include "UnitsHelper.hpp"
#include <cstring>
#include <mutex>
#include "logging_utils.h"
#include "Logger.hpp"

ut_system* UnitsHelper::unit_system;
std::once_flag UnitsHelper::unit_system_inited;
std::map<std::string, std::shared_ptr<cv_converter>> UnitsHelper::converters;
std::mutex UnitsHelper::converters_mutex;

std::shared_ptr<cv_converter> UnitsHelper::get_converter(const std::string& in_units, const std::string& out_units, utEncoding in_encoding, utEncoding out_encoding ){
    if(in_units == "" || out_units == ""){
        Logger::logMsgAndThrowError("Unable to process empty units value for pairing \"" + in_units + "\" \"" + out_units + "\"");
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
            Logger::logMsgAndThrowError("Unable to parse in_units value " + in_units);
        }
        ut_unit* to = ut_parse(unit_system, out_units.c_str(), out_encoding);
        if (to == NULL)
        {
            ut_free(from);
            //Logger::logMsgAndThrowError("Unable to parse out_units value " + out_units);
        }
        cv_converter* conv = ut_get_converter(from, to);
        if (conv == NULL)
        {
            ut_free(from);
            ut_free(to);
            converters[key] = nullptr;
            Logger::logMsgAndThrowError("Unable to convert " + in_units + " to " + out_units);
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
    if(in_units == out_units){
        // Early-out optimization
        if(in_values == out_values){
            return in_values;
        } else {
            memcpy(out_values, in_values, sizeof(double)*count);
            return out_values;
        }
    }
    std::call_once(unit_system_inited, init_unit_system);
    
    auto converter = get_converter(in_units, out_units);

    cv_convert_doubles(converter.get(), in_values, count, out_values);

    return out_values;
}

