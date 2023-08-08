#ifndef NGEN_NULLFORCING_H
#define NGEN_NULLFORCING_H

#include <vector>
#include <set>
#include <cmath>
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <ctime>
#include <time.h>
#include <memory>
#include "AorcForcing.hpp"
#include "GenericDataProvider.hpp"
#include "DataProviderSelectors.hpp"
#include <exception>
#include <UnitsHelper.hpp>

/**
 * @brief Forcing class providing time-series precipiation forcing data to the model.
 */
class NullForcingProvider : public data_access::GenericDataProvider
{
    public:

    typedef struct tm time_type;


    NullForcingProvider(forcing_params forcing_config):start_date_time_epoch(forcing_config.simulation_start_t),
                                           end_date_time_epoch(forcing_config.simulation_end_t),
                                           current_date_time_epoch(forcing_config.simulation_start_t),
                                           forcing_vector_index(-1)
    {
        set_available_forcings();
    }

    // BEGIN DataProvider interface methods

    /**
     * @brief the inclusive beginning of the period of time over which this instance can provide data for this forcing.
     *
     * @return The inclusive beginning of the period of time over which this instance can provide this data.
     */
    long get_data_start_time() override {
        //FIXME: Trace this back and you will find that it is the simulation start time, not having anything to do with the forcing at all.
        // Apparently this "worked", but at a minimum the description above is false.
        //return start_date_time_epoch;
        // LONG_MIN is a large negative number, so we return 0 as the starting time
        return 0;
    }

    /**
     * @brief the exclusive ending of the period of time over which this instance can provide data for this forcing.
     *
     * @return The exclusive ending of the period of time over which this instance can provide this data.
     */
    long get_data_stop_time() override {
        //return end_date_time_epoch;
        return LONG_MAX;
    }

    /**
     * @brief the duration of one record of this forcing source
     *
     * @return The duration of one record of this forcing source
     */
    long record_duration() override {
        return 1;
    }

    /**
     * Get the index of the forcing time step that contains the given point in time.
     *
     * An @ref std::out_of_range exception should be thrown if the time is not in any time step.
     *
     * @param epoch_time The point in time, as a seconds-based epoch time.
     * @return The index of the forcing time step that contains the given point in time.
     * @throws std::out_of_range If the given point is not in any time step.
     */
    size_t get_ts_index_for_time(const time_t &epoch_time) override {
        return 0;
    }

    /**
     * Get the value of a forcing property for an arbitrary time period, converting units if needed.
     *
     * An @ref std::out_of_range exception should be thrown if the data for the time period is not available.
     *
     * @param selector Object storing information about the data to be queried
     * @param m methode to resample data if needed
     * @throws std::runtime_error as this provider does not provide forcing value/values
     */
    double get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override
    {
        throw std::runtime_error("Called get_value function in NullDataProvider");
    }

    virtual std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override
    {
        throw std::runtime_error("Called get_values function in NullDataProvider");
    }

    /**
     * Get whether a property's per-time-step values are each an aggregate sum over the entire time step.
     *
     * Certain properties, like rain fall, are aggregated sums over an entire time step.  Others, such as pressure,
     * are not such sums and instead something else like an instantaneous reading or an average value.
     *
     * It may be the case that forcing data is needed for some discretization different than the forcing time step.
     * This aspect must be known in such cases to perform the appropriate value interpolation.
     *
     * @param name The name of the forcing property for which the current value is desired.
     * @return Whether the property's value is an aggregate sum.
     */
    //TODO this one used in Bmi_Module_Formulation.hpp and Bmi_Multi_Formulation.hpp
    inline bool is_property_sum_over_time_step(const std::string& name) override {
        throw std::runtime_error("Got request for variable " + name + " but no such variable is provided by NullForcingProvider." + SOURCE_LOC);
    }

    const std::vector<std::string> &get_available_variable_names() override {
        return available_forcings;
    }

    private:


    /**
     * @brief set_available_forcings so that Formulation_Manager has a non-empty variable name to access simulation time
     */
    void set_available_forcings()
    {
        available_forcings.push_back("dummy");
    }

    std::vector<std::string> available_forcings;

    int forcing_vector_index;

    time_t start_date_time_epoch;
    time_t end_date_time_epoch;
    time_t current_date_time_epoch;
};

#endif // NGEN_NULLFORCING_H
