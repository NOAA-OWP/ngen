#include "NullForcingProvider.hpp"

#include <stdexcept>
#include <limits>

#include <all.h>

NullForcingProvider::NullForcingProvider() = default;

long NullForcingProvider::get_data_start_time() const
{
    return 0;
}

long NullForcingProvider::get_data_stop_time() const {
    return std::numeric_limits<long>::max();
}

long NullForcingProvider::record_duration() const {
    return 1;
}

size_t NullForcingProvider::get_ts_index_for_time(const time_t &epoch_time) const {
    return 0;
}

double NullForcingProvider::get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m)
{
    throw std::runtime_error("Called get_value function in NullDataProvider");
}

std::vector<double> NullForcingProvider::get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m)
{
    throw std::runtime_error("Called get_values function in NullDataProvider");
}

inline bool NullForcingProvider::is_property_sum_over_time_step(const std::string& name) const {
    throw std::runtime_error("Got request for variable " + name + " but no such variable is provided by NullForcingProvider." + SOURCE_LOC);
}

boost::span<const std::string> NullForcingProvider::get_available_variable_names() const {
    return {};
}
