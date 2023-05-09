#include <udunits2.h>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include "all.h"

#ifndef NGEN_UNITSHELPER_H
#define NGEN_UNITSHELPER_H

class UnitsHelper {

    public:

    static double get_converted_value(const std::string &in_units, const double &value, const std::string &out_units);

    static double* convert_values(const std::string &in_units, double* values, const std::string &out_units, double* out_values, const size_t & count);

    private:
     
    // Theoretically thread-safe. //TODO: Test?
    static ut_system* unit_system;

    static std::map<std::string, std::shared_ptr<cv_converter>> converters;
    static std::mutex converters_mutex;

    static std::once_flag unit_system_inited;
    static void init_unit_system(){
        #ifdef NGEN_UDUNITS2_XML_PATH
        unit_system = ut_read_xml(NGEN_UDUNITS2_XML_PATH);
        #else
        unit_system = ut_read_xml(NULL);
        #endif
        if (unit_system == NULL) 
        {
            throw std::runtime_error("Unable to create UDUNITS2 Unit System." SOURCE_LOC);
        }
        #ifndef UDUNITS_QUIET
        ut_set_error_message_handler(ut_ignore);
        #endif
    }

    static std::shared_ptr<cv_converter> get_converter(const std::string& in_units, const std::string& out_units, utEncoding in_encoding = UT_UTF8, utEncoding out_encoding = UT_UTF8 );

};

#endif //NGEN_UNITSHELPER_H
