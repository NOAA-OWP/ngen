#ifndef NGEN_NULLFORCING_H
#define NGEN_NULLFORCING_H

#include <vector>
#include <string>
#include <stdexcept>
#include <limits>
#include "GenericDataProvider.hpp"

/**
 * @brief Forcing class that returns no variables to the simulation--use this e.g. if a BMI model provides forcing data.
 */
class NullForcingProvider : public data_access::GenericDataProvider
{
    public:

    NullForcingProvider(){}

    // BEGIN DataProvider interface methods

    long get_data_start_time() override {
        return 0;
    }

    long get_data_stop_time() override {
        return LONG_MAX;
    }

    long record_duration() override {
        return 1;
    }

    size_t get_ts_index_for_time(const time_t &epoch_time) override {
        return 0;
    }

    double get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override
    {
        throw std::runtime_error("Called get_value function in NullDataProvider");
    }

    virtual std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override
    {
        throw std::runtime_error("Called get_values function in NullDataProvider");
    }

    inline bool is_property_sum_over_time_step(const std::string& name) override {
        throw std::runtime_error("Got request for variable " + name + " but no such variable is provided by NullForcingProvider." + SOURCE_LOC);
    }

    boost::span<const std::string> get_available_variable_names() override {
        return available_forcings;
    }

    private:
    
    std::vector<std::string> available_forcings;

};

#endif // NGEN_NULLFORCING_H
