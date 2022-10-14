#ifndef NGEN_CSVPERFEATUREFORCING_H
#define NGEN_CSVPERFEATUREFORCING_H

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
class CsvPerFeatureForcingProvider : public data_access::GenericDataProvider
{
    public:

    typedef struct tm time_type;


    CsvPerFeatureForcingProvider(forcing_params forcing_config):start_date_time_epoch(forcing_config.simulation_start_t),
                                           end_date_time_epoch(forcing_config.simulation_end_t),
                                           current_date_time_epoch(forcing_config.simulation_start_t),
                                           forcing_vector_index(-1)
    {
        read_csv(forcing_config.path);
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
        return start_date_time_epoch;
    }

    /**
     * @brief the exclusive ending of the period of time over which this instance can provide data for this forcing.
     *
     * @return The exclusive ending of the period of time over which this instance can provide this data.
     */
    long get_data_stop_time() override {
        return end_date_time_epoch;
    }

    /**
     * @brief the duration of one record of this forcing source
     *
     * @return The duration of one record of this forcing source
     */
    long record_duration() override {
        return time_epoch_vector[1] - time_epoch_vector[0];
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
        if (epoch_time < start_date_time_epoch) {
            throw std::out_of_range("Forcing had bad pre-start time for index query: " + std::to_string(epoch_time));
        }
        size_t i = 0;
        // 1 hour
        time_t seconds_in_time_step = 3600;
        time_t time = start_date_time_epoch;
        while (epoch_time >= time + seconds_in_time_step && time < end_date_time_epoch) {
           i++;
           time += seconds_in_time_step;
        }
        // The end_date_time_epoch is the epoch value of the BEGINNING of the last time step, not its end.
        // I.e., to make sure we cover it, we have to go another time step beyond.
        if (time >= end_date_time_epoch + 3600) {
            throw std::out_of_range("Forcing had bad beyond-end time for index query: " + std::to_string(epoch_time));
        }
        else {
            return i;
        }
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
        size_t current_index;
        long time_remaining = selector.get_duration_secs();
        auto init_time = selector.get_init_time();
        auto output_name = selector.get_variable_name();
        auto output_units = selector.get_output_units();

        try {
            current_index = get_ts_index_for_time(init_time);
        }
        catch (const std::out_of_range &e) {
            throw std::out_of_range("Forcing had bad init_time " + std::to_string(init_time) + " for value request");
        }

        std::vector<double> involved_time_step_values;

        std::vector<long> involved_time_step_seconds;
        long ts_involved_s;

        time_t first_time_step_start_epoch = start_date_time_epoch + (current_index * 3600);
        // Handle the first time step differently, since we need to do more to figure out how many seconds came from it
        // Total time step size minus the offset of the beginning, before the init time
        ts_involved_s = 3600 - (init_time - first_time_step_start_epoch);

        involved_time_step_seconds.push_back(ts_involved_s);
        involved_time_step_values.push_back(get_value_for_param_name(output_name, current_index));
        time_remaining -= ts_involved_s;
        current_index++;

        while (time_remaining > 0) {
            if(current_index >= time_epoch_vector.size())
                return involved_time_step_values[involved_time_step_values.size()-1]; //TODO: Is this the right answer? Is returning any value off the end of the range valid?
            ts_involved_s = time_remaining > 3600 ? 3600 : time_remaining;
            involved_time_step_seconds.push_back(ts_involved_s);
            involved_time_step_values.push_back(get_value_for_param_name(output_name, current_index));
            time_remaining -= ts_involved_s;
            current_index++;

        }
        double value = 0;
        for (size_t i = 0; i < involved_time_step_values.size(); ++i) {
            if (is_param_sum_over_time_step(output_name))
                value += involved_time_step_values[i] * ((double)involved_time_step_seconds[i] / 3600.0);
            else
                value += involved_time_step_values[i] * ((double)involved_time_step_seconds[i] / (double)selector.get_duration_secs());
        }

        // Convert units
        try {
            return UnitsHelper::get_converted_value(available_forcings_units[output_name], value, output_units);
        }
        catch (const std::runtime_error& e){
            #ifndef UDUNITS_QUIET
            std::cerr<<"WARN: Unit conversion unsuccessful - Returning unconverted value! (\""<<e.what()<<"\")"<<std::endl;
            #endif
            return value;
        }
    }

    virtual std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override
    {
        return std::vector<double>(1, get_value(selector, m));
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
    inline bool is_property_sum_over_time_step(const std::string& name) override {
        return is_param_sum_over_time_step(name);
    }

    const std::vector<std::string> &get_avaliable_variable_names() override {
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
    void read_csv(std::string file_name)
    {
        int time_col_index = 0;
        //std::map<std::string, int> col_indices;
        std::vector<std::vector<double>*> local_valvec_index = {};

        //Call CSVReader constuctor
        CSVReader reader(file_name);

        //Get the data from CSV File
        std::vector<std::vector<std::string> > data_list = reader.getData();

        // Process the header (first) row..
        int col_num = 0;
        for (const auto& col_head : data_list[0]){
            //std::cerr << s << std::endl;
            if(col_head == "Time" || col_head == "time"){
                time_col_index = col_num;
                local_valvec_index.push_back(nullptr); // make sure the column indices line up!
            } else {
                std::string var_name = col_head;
                std::string units = "";

                //TODO: parse units in parens and/or square brackets?

                auto wkf = data_access::WellKnownFields.find(var_name);
                if(wkf != data_access::WellKnownFields.end()){
                    units = units.empty() ? std::get<1>(wkf->second) : units;
                    available_forcings.push_back(var_name); // Allow lookup by non-canonical name
                    available_forcings_units[var_name] = units; // Allow lookup of units by non-canonical name
                    var_name = std::get<0>(wkf->second); // Use the CSDMS name from here on
                }

                forcing_vectors[var_name] = {};
                local_valvec_index.push_back(&(forcing_vectors[var_name]));
                available_forcings.push_back(var_name);
                available_forcings_units[var_name] = units;
            }
            col_num++;
        }

        time_t current_row_date_time_epoch;
        //Iterate through CSV starting on the second row
        int i = 1;
        for (i = 1; i < data_list.size(); i++)
        {
            //Row vector
            std::vector<std::string>& vec = data_list[i];

            struct tm current_row_date_time_utc = tm();
            std::string time_str = vec[time_col_index];
            //TODO: Support more time string formats? This is basically ISO8601 but not complete, support TZ?
            strptime(time_str.c_str(), "%Y-%m-%d %H:%M:%S", &current_row_date_time_utc);

            //Convert current row date-time UTC to epoch time
            current_row_date_time_epoch = timegm(&current_row_date_time_utc);

            //TODO: I am not sure this is a concern of this object. If forcing is retrieved that doesn't cover the
            //needed time period, isn't that the requester's concern? (Methods exist to check this...)
            //Ensure that forcing data covers the entire model period. Otherwise, throw an error.
            if (i == 1 && start_date_time_epoch < current_row_date_time_epoch)
            {
                struct tm start_date_tm;
                gmtime_r(&start_date_time_epoch, &start_date_tm);
                
                char tm_buff[128];
                strftime(tm_buff, 128, "%Y-%m-%d %H:%M:%S", &start_date_tm);
                throw std::runtime_error("Error: Forcing data " + file_name + " begins after the model start time:" + std::string(tm_buff) + " < " + time_str);
            }

            
            if (start_date_time_epoch <= current_row_date_time_epoch && current_row_date_time_epoch <= end_date_time_epoch)
            {
                time_epoch_vector.push_back(current_row_date_time_epoch);

                int c = -1;
                for (auto& s : vec){
                    c++;
                    if(c == time_col_index)
                        continue;
                    boost::algorithm::trim(s);
                    local_valvec_index[c]->push_back(boost::lexical_cast<double>(s)); // This is supposed to update the vector in the map...
                }

            }

        }
        if (i <= 1 || current_row_date_time_epoch < end_date_time_epoch)
        {
            /// \todo TODO: Return appropriate error
            std::cout << "WARNING: Forcing data ends before the model end time." << std::endl;
            //throw std::runtime_error("Error: Forcing data ends before the model end time.");
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

#endif // NGEN_CSVPERFEATUREFORCING_H
