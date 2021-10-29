#include <udunits2.h>
#include <mutex>

class UnitsHelper {

    public:

    static double get_converted_value(const std::string &in_units, double value, const std::string &out_units);

    private:

    // Theoretically thread-safe. //TODO: Test?
    static ut_system* unit_system;
    static std::once_flag unit_system_inited;
    static void init_unit_system(){
        unit_system = ut_read_xml(NGEN_UDUNITS2_XML_PATH);
        if (unit_system == NULL) 
        {
            throw std::runtime_error("Unable to create UDUNITS2 Unit System.");
        }
    }

    //public:

    //static double get_converted_value(const std::string &in_units, double value, const std::string &out_units);

};

/*
*/
