#include "NullForcingProvider.hpp"
#include "Logger.hpp"

#include <stdexcept>
#include <limits>

#include <all.h>

NullForcingProvider::NullForcingProvider() = default;

long NullForcingProvider::get_data_start_time()
{
    return 0;
}

long NullForcingProvider::get_data_stop_time() {
    return std::numeric_limits<long>::max();
}

long NullForcingProvider::record_duration() {
    return 1;
}

size_t NullForcingProvider::get_ts_index_for_time(const time_t &epoch_time) {
    return 0;
}

double NullForcingProvider::get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m)
{
    std::string throw_msg; throw_msg.assign("Called get_value function in NullDataProvider");
    LOG(throw_msg, LogLevel::WARNING);
    throw std::runtime_error(throw_msg);
}

std::vector<double> NullForcingProvider::get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m)
{
    std::string throw_msg; throw_msg.assign("Called get_values function in NullDataProvider");
    LOG(throw_msg, LogLevel::WARNING);
    throw std::runtime_error(throw_msg);
}

inline bool NullForcingProvider::is_property_sum_over_time_step(const std::string& name) {
    std::string throw_msg; throw_msg.assign("Got request for variable " + name + " but no such variable is provided by NullForcingProvider." + SOURCE_LOC);
    LOG(throw_msg, LogLevel::WARNING);
    throw std::runtime_error(throw_msg);

}

boost::span<const std::string> NullForcingProvider::get_available_variable_names() {
    return {};
}
