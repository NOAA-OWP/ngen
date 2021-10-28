#include <udunits2.h>

class UnitsHelper {

    public:

    static double get_converted_value(const std::string &in_units, double value, const std::string &out_units)
    {
        ut_system* unit_system = ut_read_xml(NGEN_UDUNITS2_XML_PATH);
        if (unit_system == NULL) 
        {
            throw std::runtime_error("Unable to create UDUNITS2 Unit System.");
        }
        ut_unit* from = ut_parse(unit_system, in_units.c_str(), UT_UTF8);
        if (from == NULL)
        {
            ut_free_system(unit_system);
            throw std::runtime_error("Unable to parse in_units value " + in_units);
        }
        ut_unit* to = ut_parse(unit_system, out_units.c_str(), UT_UTF8);
        if (to == NULL)
        {
            ut_free_system(unit_system);
            ut_free(from);
            throw std::runtime_error("Unable to parse out_units value " + out_units);
        }
        cv_converter* conv = ut_get_converter(from, to);
        if (conv == NULL)
        {
            ut_free_system(unit_system);
            ut_free(from);
            ut_free(to);
            throw std::runtime_error("Unable to convert " + in_units + " to " + out_units);
        }
        double r = cv_convert_double(conv, value);
        ut_free_system(unit_system);
        ut_free(from);
        ut_free(to);
        cv_free(conv);
        return r;
    }
};