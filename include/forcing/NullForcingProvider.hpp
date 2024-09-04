#ifndef NGEN_NULLFORCING_H
#define NGEN_NULLFORCING_H

#include <vector>
#include <string>
#include "GenericDataProvider.hpp"

/**
 * @brief Forcing class that returns no variables to the simulation--use this e.g. if a BMI model provides forcing data.
 */
class NullForcingProvider : public data_access::GenericDataProvider
{
    public:

    NullForcingProvider();

    // BEGIN DataProvider interface methods

    long get_data_start_time() const override;

    long get_data_stop_time() const override;

    long record_duration() const override;

    size_t get_ts_index_for_time(const time_t &epoch_time) const override;

    double get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override;

    std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override;

    inline bool is_property_sum_over_time_step(const std::string& name) const override;

    boost::span<const std::string> get_available_variable_names() const override;
};

#endif // NGEN_NULLFORCING_H
