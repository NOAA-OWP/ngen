#ifndef NGEN_UNITSHELPER_H
#define NGEN_UNITSHELPER_H

// FIXME: Workaround to handle UDUNITS2 includes with differing paths.
//        Not exactly sure why CMake can't handle this, but even with
//        verifying the search paths, the correct header can't be found.
//
//        See PR #725 for context on this issue.

#if defined(__has_include)
#  if __has_include(<udunits2/udunits2.h>)
#    include <udunits2/udunits2.h>
#  else
#    include <udunits2.h>
#  endif
#else
#  include <udunits2.h>
#endif

#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
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
