#include "UnitsHelper.hpp"

ut_system* UnitsHelper::unit_system;
std::once_flag UnitsHelper::unit_system_inited;

double UnitsHelper::get_converted_value(const std::string &in_units, double value, const std::string &out_units)
{
    if(in_units == out_units){
        return value; // Early-out optimization
    }
    if(in_units == "" || out_units == ""){
        throw std::runtime_error("Unable to process empty units value for pairing \"" + in_units + "\" \"" + out_units + "\"");
    }
    std::call_once(unit_system_inited, init_unit_system);
    ut_unit* from = ut_parse(unit_system, in_units.c_str(), UT_UTF8);
    if (from == NULL)
    {
        throw std::runtime_error("Unable to parse in_units value " + in_units);
    }
    ut_unit* to = ut_parse(unit_system, out_units.c_str(), UT_UTF8);
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
    double r = cv_convert_double(conv, value);
    ut_free(from);
    ut_free(to);
    cv_free(conv);
    return r;
}