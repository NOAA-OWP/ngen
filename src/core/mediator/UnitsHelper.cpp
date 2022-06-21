#include "UnitsHelper.hpp"
#include <cstring>

ut_system* UnitsHelper::unit_system;
std::once_flag UnitsHelper::unit_system_inited;

cv_converter* UnitsHelper::get_converter(const std::string &in_units, const std::string& out_units, ut_unit*& to, ut_unit*& from)
{
    if(in_units == "" || out_units == ""){
        throw std::runtime_error("Unable to process empty units value for pairing \"" + in_units + "\" \"" + out_units + "\"");
    }
    from = ut_parse(unit_system, in_units.c_str(), UT_UTF8);
    if (from == NULL)
    {
        throw std::runtime_error("Unable to parse in_units value " + in_units);
    }
    to = ut_parse(unit_system, out_units.c_str(), UT_UTF8);
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
        throw std::runtime_error("Unable to convert " + in_units + " to " + out_units);
    }
    return conv;
}

double UnitsHelper::get_converted_value(const std::string &in_units, const double &value, const std::string &out_units)
{
    if(in_units == out_units){
        return value; // Early-out optimization
    }
    std::call_once(unit_system_inited, init_unit_system);
    ut_unit* to = NULL;
    ut_unit* from = NULL;
    cv_converter* conv = get_converter(in_units, out_units, to, from);
    double r = cv_convert_double(conv, value);
    ut_free(from);
    ut_free(to);
    cv_free(conv);
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
    ut_unit* to = NULL;
    ut_unit* from = NULL;
    cv_converter* conv = get_converter(in_units, out_units, to, from);
    
    cv_convert_doubles(conv, in_values, count, out_values);
    ut_free(from);
    ut_free(to);
    cv_free(conv);
    return out_values;
}
