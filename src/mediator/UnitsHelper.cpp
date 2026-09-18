#include "UnitsHelper.hpp"

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

#include <cstring>
#include <map>
#include <memory>
#include <mutex>

// Theoretically thread-safe. //TODO: Test?
static ut_system* unit_system = nullptr;

static thread_local std::map<std::string, std::shared_ptr<cv_converter>> converters;
static std::mutex converters_mutex;

void init_unit_system(){
    // Allow for multiple-initialization from unit tests
    if (unit_system != nullptr)
        return;

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

static std::shared_ptr<cv_converter> make_converter(const std::string& in_units, const std::string& out_units, utEncoding in_encoding = UT_UTF8, utEncoding out_encoding = UT_UTF8) {
        // Don't trust that UDUNITS2 is thread-safe
        std::unique_lock insertion_lock(converters_mutex);

        ut_unit* from = ut_parse(unit_system, in_units.c_str(), in_encoding);
        if (from == NULL)
        {
            UnitsHelper::unit_conversion_exception uce{"Unable to parse in_units", in_units, out_units};
            throw uce;
        }
        ut_unit* to = ut_parse(unit_system, out_units.c_str(), out_encoding);
        if (to == NULL)
        {
            ut_free(from);

            UnitsHelper::unit_conversion_exception uce{"Unable to parse out_units", in_units, out_units};
            throw uce;
        }

        cv_converter* conv = ut_get_converter(from, to);
        if (conv == NULL)
        {
            ut_free(from);
            ut_free(to);

            UnitsHelper::unit_conversion_exception uce{"Unable to convert as requested", in_units, out_units};
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
        return c;
}

static std::shared_ptr<cv_converter> get_converter(const std::string& in_units, const std::string& out_units, utEncoding in_encoding = UT_UTF8, utEncoding out_encoding = UT_UTF8 ){
    if(in_units == "") {
        UnitsHelper::unit_conversion_exception uce{"Requested conversion from empty input units string", in_units, out_units};
        throw uce;
    }

    if(out_units == "") {
        UnitsHelper::unit_conversion_exception uce{"Requested conversion to empty output units string", in_units, out_units};
        throw uce;
    }

    std::string key = in_units + "|" + out_units; //Better solution? Good enough? Bother with nested maps?

    auto iter = converters.find(key);
    if(iter != converters.end()){
      if(iter->second == nullptr){
          // Recurrence of last throw case below
          UnitsHelper::unit_conversion_exception uce{"Unable to convert as requested (repeated)", in_units, out_units};
          throw uce;
      }
      return iter->second;
    }

    try {
      auto c = make_converter(in_units, out_units, in_encoding, out_encoding);
      converters[key] = c;
      return c;
    } catch (UnitsHelper::unit_conversion_exception &) {
      converters[key] = nullptr;
      throw;
    }
}

static bool is_noneish(std::string const& u) {
    return u.empty() || u == "none" || u == "unitless" || u == "dimensionless" || u == "-";
}

UnitsHelper::normalized_units::normalized_units(std::string in_units, std::string out_units)
    : in(std::move(in_units)), out(std::move(out_units))
{
    if (is_noneish(in))
        in = "1";
    if (is_noneish(out))
        out = (in == "1") ? std::string("1") : std::string("");
}

double UnitsHelper::get_converted_value(const normalized_units &units, const double &value)
{
    if (units.is_identity()) {
        return value;
    }

    try {
        auto converter = get_converter(units.in, units.out);
        double r = cv_convert_double(converter.get(), value);
        return r;
    } catch (unit_conversion_exception& uce) {
        uce.unconverted_values.push_back(value);
        throw;
    }
}

double* UnitsHelper::convert_values(const normalized_units &units, double* in_values, double* out_values, const size_t& count)
{
    if (units.is_identity()) {
        if (in_values == out_values) {
            return in_values;
        } else {
            std::memcpy(out_values, in_values, sizeof(double)*count);
            return out_values;
        }
    }

    // Don't catch the UCE here to fill in uce.unconverted_values,
    // because the caller may be able to more efficiently std::move it
    auto converter = get_converter(units.in, units.out);
    cv_convert_doubles(converter.get(), in_values, count, out_values);
    return out_values;
}

static std::mutex errors_mutex;
std::set<UnitsHelper::unit_error_log_key> UnitsHelper::unit_errors_reported;

bool UnitsHelper::record_unit_conversion_fault(unit_conversion_exception const& uce, std::string const& requester_name, std::string const& requester_variable)
{
    unit_error_log_key key{requester_name, requester_variable, uce.provider_model_name, uce.provider_var_name, uce.what()};

    std::lock_guard lock(errors_mutex);
    auto ret = unit_errors_reported.insert(key);
    bool new_error = ret.second;
    return new_error;
}
