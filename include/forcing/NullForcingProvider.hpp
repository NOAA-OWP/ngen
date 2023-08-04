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
#include "CSV_Reader.h"
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
        read_csv();
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
        // LONG_MIN is a large negative number
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
        //return time_epoch_vector[1] - time_epoch_vector[0];
        //TODO find a more general way to set it
        long timestep_size = 3600;
        return timestep_size;
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
     * @return The value of the forcing property for the described time period, with units converted if needed.
     * @throws std::out_of_range If data for the time period is not available.
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
     * Get whether a param's value is an aggregate sum over the entire time step.
     *
     * Certain params, like rain fall, are aggregated sums over an entire time step.  Others, such as pressure, are not
     * such sums and instead something else like an instantaneous reading or an average value over the time step.
     *
     * It may be the case that forcing data is needed for some discretization different than the forcing time step.
     * These values can be calculated (or at least approximated), but doing so requires knowing which values are summed
     * versus not.
     *
     * @param name The name of the forcing param for which the current value is desired.
     * @return Whether the param's value is an aggregate sum.
     */
    //TODO this function doesn't seem being used
    inline bool is_param_sum_over_time_step(const std::string& name) {
        if (name == CSDMS_STD_NAME_RAIN_VOLUME_FLUX) {
            return true;
        }
        if (name == CSDMS_STD_NAME_SOLAR_SHORTWAVE) {
            return true;
        }
        if (name == CSDMS_STD_NAME_SOLAR_LONGWAVE) {
            return true;
        }
        if (name == CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE) {
            return true;
        }
        return false;
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
        return is_param_sum_over_time_step(name);
    }

    const std::vector<std::string> &get_available_variable_names() override {
        return available_forcings;
    }

    private:

    /**
     * @brief Checks forcing vector index bounds and adjusts index if out of vector bounds
     * /// \todo: Bounds checking is based on precipitation vector. Consider potential for vectors of different sizes and indices.
     */
    inline void check_forcing_vector_index_bounds()
    {
        //Check if forcing index is less than zero and if so, set to zero.
        if (forcing_vector_index < 0)
        {
            forcing_vector_index = 0;
            /// \todo: Return appropriate warning
            std::cout << "WARNING: Forcing vector index is less than zero. Therefore, setting index to zero." << std::endl;
        }

        //Check if forcing index is greater than or equal to the size of the size of the time vector and if so, set to zero.
        else if (forcing_vector_index >= time_epoch_vector.size())
        {
            forcing_vector_index = time_epoch_vector.size() - 1;
            /// \todo: Return appropriate warning
            std::cout << "WARNING: Reached beyond the size of the forcing vector. Therefore, setting index to last value of the vector." << std::endl;
        }

        return;
    }

    /**
     * Get the current value of a forcing param identified by its name.
     *
     * @param name The name of the forcing param for which the current value is desired.
     * @param index The index of the desired forcing time step from which to obtain the value.
     * @return The particular param's value at the given forcing time step.
     */
    inline double get_value_for_param_name(const std::string& name, int index) {
        if (index >= time_epoch_vector.size() ) {
            throw std::out_of_range("Forcing had bad index " + std::to_string(index) + " for value lookup of " + name);
        }

        std::string can_name = name;
        if(data_access::WellKnownFields.count(can_name) > 0){
            auto t = data_access::WellKnownFields.find(can_name)->second;
            can_name = std::get<0>(t);
        }

        if (forcing_vectors.count(can_name) > 0) {
            return forcing_vectors[can_name].at(index);
        }
        else {
            throw std::runtime_error("Cannot get forcing value for unrecognized parameter name '" + name + "'.");
        }
    }

    /**
     * @brief Read Forcing Data from CSV
     * Reads only data within the specified model start and end date-times.
     * @param file_name Forcing file name
     */
    // No file to read
    void read_csv()
    {
                //std::string var_name = CSDMS_STD_NAME_RAIN_VOLUME_FLUX;
                available_forcings.push_back("time");
                std::string var_name = "precip";
                std::string units = "";
                auto wkf = data_access::WellKnownFields.find(var_name);
                if(wkf != data_access::WellKnownFields.end()){
                    units = units.empty() ? std::get<1>(wkf->second) : units;
                    available_forcings.push_back(var_name); // Allow lookup by non-canonical name
                    available_forcings_units[var_name] = units; // Allow lookup of units by non-canonical name
                    var_name = std::get<0>(wkf->second); // Use the CSDMS name from here on
                }
    }

    std::vector<std::string> available_forcings;
    std::unordered_map<std::string, std::string> available_forcings_units;

    /// \todo: Look into aggregation of data, relevant libraries, and storing frequency information
    std::unordered_map<std::string, std::vector<double>> forcing_vectors;

    /// \todo: Consider making epoch time the iterator
    std::vector<time_t> time_epoch_vector;     
    int forcing_vector_index;

    /// \todo: Are these used?
    double precipitation_rate_meters_per_second;
    double air_temperature_fahrenheit;

    double latitude; //latitude (degrees_north)
    double longitude; //longitude (degrees_east)
    int catchment_id;
    int day_of_year;
    std::string forcing_file_name;

    std::shared_ptr<time_type> start_date_time;
    std::shared_ptr<time_type> end_date_time;

    time_t start_date_time_epoch;
    time_t end_date_time_epoch;
    time_t current_date_time_epoch;
};

/// \todo Consider aggregating precipiation data
/// \todo Make CSV forcing a subclass
/// \todo Consider passing grid to class
/// \todo Consider following GDAL API functionality

#endif // NGEN_NULLFORCING_H
