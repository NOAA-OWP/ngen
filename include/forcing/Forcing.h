#ifndef FORCING_H
#define FORCING_H

#include <vector>
#include <set>
#include <cmath>
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "CSV_Reader.h"
#include <ctime>
#include <time.h>
#include <memory>
#include "ForcingProvider.hpp"
#include <exception>

// Recognized Forcing Value Names (in particular for use when configuring BMI input variables)
// TODO: perhaps create way to configure a mapping of these to something different
#define AORC_FIELD_NAME_PRECIP_RATE "precip_rate"
#define AORC_FIELD_NAME_SOLAR_SHORTWAVE "DSWRF_surface"
#define AORC_FIELD_NAME_SOLAR_LONGWAVE "DLWRF_surface"
#define AORC_FIELD_NAME_PRESSURE_SURFACE "PRES_surface"
#define AORC_FIELD_NAME_TEMP_2M_AG "TMP_2maboveground"
#define AORC_FIELD_NAME_APCP_SURFACE "APCP_surface"
#define AORC_FIELD_NAME_WIND_U_10M_AG "UGRD_10maboveground"
#define AORC_FIELD_NAME_WIND_V_10M_AG "VGRD_10maboveground"
#define AORC_FIELD_NAME_SPEC_HUMID_2M_AG "SPFH_2maboveground"

// CSDMS Standard Names for several forcings
#define CSDMS_STD_NAME_RAIN_RATE "atmosphere_water__rainfall_volume_flux"
#define CSDMS_STD_NAME_SOLAR_LONGWAVE "land_surface_radiation~incoming~longwave__energy_flux"
#define CSDMS_STD_NAME_SOLAR_SHORTWAVE "land_surface_radiation~incoming~shortwave__energy_flux"
#define CSDMS_STD_NAME_SURFACE_AIR_PRESSURE "land_surface_air__pressure"
#define CSDMS_STD_NAME_HUMIDITY "atmosphere_air_water~vapor__relative_saturation"
#define CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE "atmosphere_water__liquid_equivalent_precipitation_rate"
#define CSDMS_STD_NAME_SURFACE_TEMP "land_surface_air__temperature"
#define CSDMS_STD_NAME_WIND_U_X "land_surface_wind__x_component_of_velocity"
#define CSDMS_STD_NAME_WIND_V_Y "land_surface_wind__y_component_of_velocity"

using namespace std;

/**
 * @brief forcing_params providing configuration information for forcing time period and source.
 */
struct forcing_params
{
  std::string path;
  std::string start_time;
  std::string end_time;
  std::string date_format =  "%Y-%m-%d %H:%M:%S";
  time_t start_t;
  time_t end_t;
  /*
    Constructor for forcing_params
  */
  forcing_params(std::string path, std::string start_time, std::string end_time):
    path(path), start_time(start_time), end_time(end_time)
    {
      /// \todo converting to UTC can be tricky, especially if thread safety is a concern
      /* https://stackoverflow.com/questions/530519/stdmktime-and-timezone-info */
      struct tm tm;
      strptime(this->start_time.c_str(), this->date_format.c_str() , &tm);
      //mktime returns time in local time based on system timezone
      //FIXME use timegm (not standard)? or implement timegm (see above link)
      this->start_t = timegm( &tm );

      strptime(this->end_time.c_str(), this->date_format.c_str() , &tm);
      this->end_t = timegm( &tm );
    }
};

//AORC Forcing Data Struct
struct AORC_data
{
  double APCP_surface_kg_per_meters_squared; //Total Precipitation (kg/m^2)
  double DLWRF_surface_W_per_meters_squared; //Downward Long-Wave Rad. (Flux W/m^2)
  double DSWRF_surface_W_per_meters_squared; //Downward Short-Wave Radiation (Flux W/m^2)
  double PRES_surface_Pa; //Pressure (Pa)
  double SPFH_2maboveground_kg_per_kg; //Specific Humidity (kg/kg)
  double TMP_2maboveground_K; //Temperature (K)
  double UGRD_10maboveground_meters_per_second; //U-Component of Wind (m/s)
  double VGRD_10maboveground_meters_per_second; //V-Component of Wind (m/s)
};


/**
 * @brief Forcing class providing time-series precipiation forcing data to the model.
 */
class Forcing : public forcing::ForcingProvider
{
    public:

    typedef struct tm time_type;
    
    /**
     * Get supported standard names for forcing fields.
     *
     * @return
     */
    static std::shared_ptr<std::set<std::string>> get_forcing_field_names() {
        std::shared_ptr<std::set<std::string>> field_names = std::make_shared<std::set<std::string>>();
        field_names->insert(AORC_FIELD_NAME_PRECIP_RATE);
        field_names->insert(AORC_FIELD_NAME_SOLAR_SHORTWAVE);
        field_names->insert(AORC_FIELD_NAME_SOLAR_LONGWAVE);
        field_names->insert(AORC_FIELD_NAME_PRESSURE_SURFACE);
        field_names->insert(AORC_FIELD_NAME_TEMP_2M_AG);
        field_names->insert(AORC_FIELD_NAME_APCP_SURFACE);
        field_names->insert(AORC_FIELD_NAME_WIND_U_10M_AG);
        field_names->insert(AORC_FIELD_NAME_WIND_V_10M_AG);
        field_names->insert(AORC_FIELD_NAME_SPEC_HUMID_2M_AG);
        return field_names;
    }


    /**
     * Default Constructor building an empty Forcing object
     */
    Forcing(): air_temperature_fahrenheit(0.0), catchment_id(0), forcing_file_name("")
    {

    }

    Forcing(forcing_params forcing_config):start_date_time_epoch(forcing_config.start_t),
                                           end_date_time_epoch(forcing_config.end_t),
                                           current_date_time_epoch(forcing_config.start_t),
                                           forcing_vector_index(-1)
    {
        read_forcing_aorc(forcing_config.path);
    }

    /**
     * @brief Parameterized Constuctor that builds a Forcing object and reads an input forcing CSV into a vector.
     * @param air_temperature_fahrenheit Air temperatrure in Fahrenheit
     * @param catchment_id Catchment ID
     * @param forcing_file_name Forcing file name
     * @param start_date_time Start date-time of model to select start of forcing time window of data
     * @param end_date_time End date-time of model to select end of forcing time window of data
     * /// \todo: when using catchment_id parameter, need to enforce their explicit inclusion when calling constructors
     *            instead of allowing default ID of 0.
     */
    Forcing(double air_temperature_fahrenheit, double catchment_id, string forcing_file_name, std::shared_ptr<time_type>  start_date_time, std::shared_ptr<time_type> end_date_time): air_temperature_fahrenheit(air_temperature_fahrenheit), catchment_id(0), forcing_file_name(forcing_file_name), start_date_time(start_date_time), end_date_time(end_date_time)
    {

        //Convert start and end time structs to epoch time
        start_date_time_epoch = mktime(start_date_time.get());

        end_date_time_epoch =  mktime(end_date_time.get());

        current_date_time_epoch = start_date_time_epoch;

        //Call read_forcing function
        //read_forcing(forcing_file_name);
        read_forcing_aorc(forcing_file_name);

        //Initialize forcing vector index to 0;
        forcing_vector_index = 0;
    }

    // TODO: consider using friend for this instead
    struct AORC_data get_aorc_for_index(size_t index) {
        return AORC_vector.at(index);
    }

    const std::vector<std::string> &get_available_forcing_outputs() override {
        if (available_forcings.empty()) {
            for (std::string forcing_name : *(Forcing::get_forcing_field_names())) {
                available_forcings.push_back(forcing_name);
            }
            available_forcings.push_back(CSDMS_STD_NAME_SOLAR_LONGWAVE);
            available_forcings.push_back(CSDMS_STD_NAME_SOLAR_SHORTWAVE);
            available_forcings.push_back(CSDMS_STD_NAME_SURFACE_AIR_PRESSURE);
            available_forcings.push_back(CSDMS_STD_NAME_HUMIDITY);
            available_forcings.push_back(CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE);
            available_forcings.push_back(CSDMS_STD_NAME_SURFACE_TEMP);
            available_forcings.push_back(CSDMS_STD_NAME_WIND_U_X);
            available_forcings.push_back(CSDMS_STD_NAME_WIND_V_Y);
        }
        return available_forcings;
    }

    /**
     * Placeholder implementation for function to return the given param adjusted to be in the supplied units.
     *
     * At present this just returns the param's internal value.
     *
     * @param name The name of the desired parameter.
     * @param units_str A string represented the units for conversion, using standard abbreviations.
     * @return For now, just the param value, but in the future, the value converted.
     */
    inline double get_converted_value_for_param_in_units(const std::string& name, const std::string& units_str,
                                                         int index)
    {
        // TODO: this is just a placeholder implementation that needs to be replaced with real convertion logic
        return get_value_for_param_name(name, index);
    }

    /**
     * Placeholder implementation for function to return the given param adjusted to be in the supplied units.
     *
     * At present this just returns the param's internal value.
     *
     * @param name The name of the desired parameter.
     * @param units_str A string represented the units for conversion, using standard abbreviations.
     * @return For now, just the param value, but in the future, the value converted.
     */
    inline double get_converted_value_for_param_in_units(const std::string& name, const std::string& units_str) {
        check_forcing_vector_index_bounds();
        return get_converted_value_for_param_in_units(name, units_str, forcing_vector_index);
    }

    /**
     * Get the inclusive beginning of the period of time over which this instance can provide data for this forcing.
     *
     * @return The inclusive beginning of the period of time over which this instance can provide this data.
     */
    time_t get_forcing_output_time_begin(const std::string &output_name) override {
        return start_date_time_epoch;
    }

    /**
     * Get the exclusive ending of the period of time over which this instance can provide data for this forcing.
     *
     * @return The exclusive ending of the period of time over which this instance can provide this data.
     */
    time_t get_forcing_output_time_end(const std::string &output_name) override {
        return end_date_time_epoch;
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
     * @param output_name The name of the forcing property of interest.
     * @param init_time_epoch The epoch time (in seconds) of the start of the time period.
     * @param duration_seconds The length of the time period, in seconds.
     * @param output_units The expected units of the desired output value.
     * @return The value of the forcing property for the described time period, with units converted if needed.
     * @throws std::out_of_range If data for the time period is not available.
     */
    double get_value(const std::string &output_name, const time_t &init_time, const long &duration_s,
                     const std::string &output_units) override
    {
        size_t current_index;
        long time_remaining = duration_s;
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
            ts_involved_s = time_remaining > 3600 ? 3600 : time_remaining;
            involved_time_step_seconds.push_back(ts_involved_s);
            involved_time_step_values.push_back(get_value_for_param_name(output_name, current_index));
            time_remaining -= ts_involved_s;
            current_index++;
        }
        double value;
        for (size_t i = 0; i < involved_time_step_values.size(); ++i) {
            if (is_param_sum_over_time_step(output_name))
                value += involved_time_step_values[i] * ((double)involved_time_step_seconds[i] / 3600.0);
            else
                value += involved_time_step_values[i] * ((double)involved_time_step_seconds[i] / (double)duration_s);
        }
        return value;
    }

    /**
     * Get the current value of a forcing param identified by its name.
     *
     * @param name The name of the forcing param for which the current value is desired.
     * @return The particular param's current value.
     */
    inline double get_value_for_param_name(const std::string& name) {
        check_forcing_vector_index_bounds();
        return get_value_for_param_name(name, forcing_vector_index);
    }

    /**
     * Get the current value of a forcing param identified by its name.
     *
     * @param name The name of the forcing param for which the current value is desired.
     * @param name The applicable forcing time step index for which the value is desired.
     * @return The particular param's value at the given forcing time step.
     */
    inline double get_value_for_param_name(const std::string& name, int index) {
        if (index >= precipitation_rate_meters_per_second_vector.size() || index >= AORC_vector.size()) {
            throw std::out_of_range("Forcing had bad index " + std::to_string(index) + " for value lookup of " + name);
        }

        if (name == AORC_FIELD_NAME_PRECIP_RATE) {
            return precipitation_rate_meters_per_second_vector.at(index);
        }
        if (name == AORC_FIELD_NAME_SOLAR_SHORTWAVE || name == CSDMS_STD_NAME_SOLAR_SHORTWAVE) {
            return AORC_vector.at(index).DSWRF_surface_W_per_meters_squared;
        }
        if (name == AORC_FIELD_NAME_SOLAR_LONGWAVE || name == CSDMS_STD_NAME_SOLAR_LONGWAVE) {
            return AORC_vector.at(index).DLWRF_surface_W_per_meters_squared;
        }
        if (name == AORC_FIELD_NAME_PRESSURE_SURFACE || name == CSDMS_STD_NAME_SURFACE_AIR_PRESSURE) {
            return AORC_vector.at(index).PRES_surface_Pa;
        }
        if (name == AORC_FIELD_NAME_TEMP_2M_AG || name == CSDMS_STD_NAME_SURFACE_TEMP) {
            return AORC_vector.at(index).TMP_2maboveground_K;
        }
        if (name == AORC_FIELD_NAME_APCP_SURFACE || name == CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE) {
            return AORC_vector.at(index).APCP_surface_kg_per_meters_squared;
        }
        if (name == AORC_FIELD_NAME_WIND_U_10M_AG || name == CSDMS_STD_NAME_WIND_U_X) {
            return AORC_vector.at(index).UGRD_10maboveground_meters_per_second;
        }
        if (name == AORC_FIELD_NAME_WIND_V_10M_AG || name == CSDMS_STD_NAME_WIND_V_Y) {
            return AORC_vector.at(index).VGRD_10maboveground_meters_per_second;
        }
        if (name == AORC_FIELD_NAME_SPEC_HUMID_2M_AG || name == CSDMS_STD_NAME_HUMIDITY) {
            return AORC_vector.at(index).SPFH_2maboveground_kg_per_kg;
        }
        else {
            throw std::runtime_error("Cannot get forcing value for unrecognized parameter name '" + name + "'.");
        }
    }

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
            cout << "WARNING: Forcing vector index is less than zero. Therefore, setting index to zero." << endl;
        }

        //Check if forcing index is greater than or equal to the size of the size of the precipiation vector and if so, set to zero.
        else if (forcing_vector_index >= precipitation_rate_meters_per_second_vector.size())
        {
            forcing_vector_index = precipitation_rate_meters_per_second_vector.size() - 1;
            /// \todo: Return appropriate warning
            cout << "WARNING: Reached beyond the size of the forcing vector. Therefore, setting index to last value of the vector." << endl;
        }

        return;
    }

    /**
     * @brief Gets current hourly precipitation in meters per second
     * Precipitation frequency is assumed to be hourly for now.
     * /// \todo: Add input for dt (delta time) for different frequencies in the data than the model frequency.
     * @return the current hourly precipitation in meters per second
     */
    double get_current_hourly_precipitation_meters_per_second()
    {
        check_forcing_vector_index_bounds();

        return precipitation_rate_meters_per_second_vector.at(forcing_vector_index);
    }

    /**
     * @brief Gets next hourly precipitation in meters per second
     * Increments pointer in forcing vector by one timestep
     * Precipitation frequency is assumed to be hourly for now.
     * /// \todo: Add input for dt (delta time) for different frequencies in the data than the model frequency.
     * /// \todo: Reconsider incrementing the forcing_vector_index because other functions rely on this, and it
     *            could have side effects
     * @return the current hourly precipitation in meters per second after pointer is incremented by one timestep
     */
    double get_next_hourly_precipitation_meters_per_second()
    {
        //Check forcing vector bounds before incrementing forcing index
        //\todo size() is unsigned, using -1 for initial offset isn't a good way, hacking for now.
        if (forcing_vector_index == -1 || forcing_vector_index < precipitation_rate_meters_per_second_vector.size() - 1){
            //Increment forcing index
            forcing_vector_index = forcing_vector_index + 1;
          }
        else{
            /// \todo: Return appropriate warning
            cout << "WARNING: Reached beyond the size of the forcing precipitation vector. Therefore, returning the last precipitation value of the vector." << endl;
          }
        return get_current_hourly_precipitation_meters_per_second();
    }

    /**
     * @brief Gets day of year integer
     * @return day of year integer
     */
    int get_day_of_year()
    {
        int current_day_of_year;

        struct tm *current_date_time;

        check_forcing_vector_index_bounds();

        current_date_time_epoch = time_epoch_vector.at(forcing_vector_index);

        /// \todo: Sort out using local versus UTC time
        current_date_time = localtime(&current_date_time_epoch);

        current_day_of_year = current_date_time->tm_yday;

        return current_day_of_year;
    }

    /**
     * @brief Accessor to time epoch
     * @return current_date_time_epoch
     */
    time_t get_time_epoch()
    {
        check_forcing_vector_index_bounds();

        return current_date_time_epoch = time_epoch_vector.at(forcing_vector_index);
    };

    /**
     * Get the time step size, based on epoch vector, assuming the last ts is equal to the next to last.
     *
     * @return
     */
    time_t get_time_step_size() {
        check_forcing_vector_index_bounds();
        // When at the last index, make an assumption the length is the same as the next-to-last
        if (time_epoch_vector.size() - 1 == forcing_vector_index)
            return time_epoch_vector.at(forcing_vector_index) - time_epoch_vector.at(forcing_vector_index - 1);
        else
            return time_epoch_vector.at(forcing_vector_index + 1) - time_epoch_vector.at(forcing_vector_index);
    }

    /**
     * @brief Accessor to AORC data struct
     * @return AORC_data
     */

    AORC_data get_AORC_data()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index);
    };

    /**
     * @brief Accessor to AORC APCP_surface_kg_per_meters_squared
     * @return APCP_surface_kg_per_meters_squared
     */
    double get_AORC_APCP_surface_kg_per_meters_squared()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).APCP_surface_kg_per_meters_squared;
    };

    /**
     * @brief Accessor to AORC DLWRF_surface_W_per_meters_squared
     * @return DLWRF_surface_W_per_meters_squared
     */
    double get_AORC_DLWRF_surface_W_per_meters_squared()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).DLWRF_surface_W_per_meters_squared;
    };

    /**
     * @brief Accessor to AORC DSWRF_surface_W_per_meters_squared
     * @return DSWRF_surface_W_per_meters_squared
     */
    double get_AORC_DSWRF_surface_W_per_meters_squared()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).DSWRF_surface_W_per_meters_squared;
    };

    /**
     * @brief Accessor to AORC PRES_surface_Pa
     * @return PRES_surface_Pa
     */
    double get_AORC_PRES_surface_Pa()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).PRES_surface_Pa;
    };

    /**
     * @brief Accessor to AORC SPFH_2maboveground_kg_per_kg
     * @return SPFH_2maboveground_kg_per_kg
     */
    double get_AORC_SPFH_2maboveground_kg_per_kg()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).SPFH_2maboveground_kg_per_kg;
    };

    /**
     * @brief Accessor to AORC TMP_2maboveground_K
     * @return TMP_2maboveground_K
     */
    double get_AORC_TMP_2maboveground_K()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).TMP_2maboveground_K;
    };

    /**
     * @brief Accessor to AORC UGRD_10maboveground_meters_per_second
     * @return UGRD_10maboveground_meters_per_second
     */
    double get_AORC_UGRD_10maboveground_meters_per_second()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).UGRD_10maboveground_meters_per_second;
    };

    /**
     * @brief Accessor to AORC VGRD_10maboveground_meters_per_second
     * @return VGRD_10maboveground_meters_per_second
     */
    double get_AORC_VGRD_10maboveground_meters_per_second()
    {
        check_forcing_vector_index_bounds();

        return AORC_vector.at(forcing_vector_index).VGRD_10maboveground_meters_per_second;
    };

    bool is_aorc_forcing() {
        return is_aorc_data;
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
        if (name == AORC_FIELD_NAME_PRECIP_RATE) {
            return true;
        }
        if (name == AORC_FIELD_NAME_SOLAR_SHORTWAVE) {
            return true;
        }
        if (name == AORC_FIELD_NAME_SOLAR_LONGWAVE) {
            return true;
        }
        if (name == AORC_FIELD_NAME_APCP_SURFACE) {
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

    private:

    /**
     * @brief Read Forcing Data from CSV
     * Reads only data within the specified model start and end date-times and adds to precipiation vector
     * @param file_name Forcing file name
     */
    void read_forcing(string file_name)
    {
        //Call CSVReader constuctor
        CSVReader reader(file_name);

	//Get the data from CSV File
	std::vector<std::vector<std::string> > data_list = reader.getData();

        //Iterate through CSV starting on the third row
        for (int i = 2; i < data_list.size(); i++)
        {
                //Row vector
                std::vector<std::string>& vec = data_list[i];

                //Declare pointer to struct for the current row date-time utc
                struct tm *current_row_date_time_utc;

                //Allocate memory to struct for the current row date-time utc
                current_row_date_time_utc = new tm();

                //Year
                string year_str = vec[0];
                int year = stoi(year_str);
                current_row_date_time_utc->tm_year = year - 1900;

                //Month
                string month_str = vec[1];
                int month = stoi(month_str);
                current_row_date_time_utc->tm_mon = month - 1;

                //Day
                string day_str = vec[2];
                int day = stoi(day_str);
                current_row_date_time_utc->tm_mday = day;

                //Hour
                string hour_str = vec[3];
                int hour = stoi(hour_str);
                current_row_date_time_utc->tm_hour = hour;

                //Convert current row date-time utc to epoch time
                time_t current_row_date_time_epoch = timegm(current_row_date_time_utc);

                //If the current row date-time is within the model date-time range, then add precipitation to vector
                if (start_date_time_epoch <= current_row_date_time_epoch && current_row_date_time_epoch <= end_date_time_epoch)
                {
                    //Precipitation
                    string precip_str = vec[5];

                    //Convert from string to double and from mm/hr to m/s
                    double precip = boost::lexical_cast<double>(precip_str) / (1000 * 3600);

                    //Add precip to vector
                    precipitation_rate_meters_per_second_vector.push_back(precip);
                }

                //Free memory from struct
                delete current_row_date_time_utc;
        }
    }

    /**
     * @brief Read Forcing Data from AORC CSV
     * Reads only data within the specified model start and end date-times and adds to precipiation vector
     * @param file_name Forcing file name
     */
    void read_forcing_aorc(string file_name)
    {
        is_aorc_data = true;
        //Call CSVReader constuctor
        CSVReader reader(file_name);

	//Get the data from CSV File
	std::vector<std::vector<std::string> > data_list = reader.getData();

        //Iterate through CSV starting on the second row
        for (int i = 1; i < data_list.size(); i++)
        {
                //Row vector
                std::vector<std::string>& vec = data_list[i];

                //Declare struct for the current row date-time 
                struct tm current_row_date_time_utc;

                //Allocate memory to struct for the current row date-time
                current_row_date_time_utc = tm();

                //Grab time string from first column
                string time_str = vec[0];

                //Convert time string to time struct
                strptime(time_str.c_str(), "%Y-%m-%d %H:%M:%S", &current_row_date_time_utc);

                //Convert current row date-time UTC to epoch time
                time_t current_row_date_time_epoch = timegm(&current_row_date_time_utc);

                //Ensure that forcing data covers the entire model period. Otherwise, throw an error.
                if (i == 1 && start_date_time_epoch < current_row_date_time_epoch)
                    /// \todo TODO: Return appropriate error
                    //cout << "WARNING: Forcing data begins after the model start time." << endl;
                    throw std::runtime_error("Error: Forcing data begins after the model start time.");


                else if (i == data_list.size() - 1 && current_row_date_time_epoch < end_date_time_epoch)
                    /// \todo TODO: Return appropriate error
                    cout << "WARNING: Forcing data ends before the model end time." << endl;
                    //throw std::runtime_error("Error: Forcing data ends before the model end time.");

                //If the current row date-time is within the model date-time range, then add precipitation to vector
                if (start_date_time_epoch <= current_row_date_time_epoch && current_row_date_time_epoch <= end_date_time_epoch)
                {
                    //Grab data from columns
                    string APCP_surface_str = vec[1];
                    string DLWRF_surface_str = vec[2];
                    string DSWRF_surface_str = vec[3];
                    string PRES_surface_str = vec[4];
                    string SPFH_2maboveground_str = vec[5];
                    string TMP_2maboveground_str = vec[6];
                    string UGRD_10maboveground_str = vec[7];
                    string VGRD_10maboveground_str = vec[8];
                    string precip_rate_str = vec[9];

                    //Declare AORC struct
                    AORC_data AORC;

                    //Convert from strings to doubles and add to AORC struct
                    AORC.APCP_surface_kg_per_meters_squared = boost::lexical_cast<double>(APCP_surface_str);
                    AORC.DLWRF_surface_W_per_meters_squared = boost::lexical_cast<double>(DLWRF_surface_str);
                    AORC.DSWRF_surface_W_per_meters_squared = boost::lexical_cast<double>(DSWRF_surface_str);
                    AORC.PRES_surface_Pa = boost::lexical_cast<double>(PRES_surface_str);
                    AORC.SPFH_2maboveground_kg_per_kg = boost::lexical_cast<double>(SPFH_2maboveground_str);
                    AORC.TMP_2maboveground_K = boost::lexical_cast<double>(TMP_2maboveground_str);
                    AORC.UGRD_10maboveground_meters_per_second = boost::lexical_cast<double>(UGRD_10maboveground_str);
                    AORC.VGRD_10maboveground_meters_per_second = boost::lexical_cast<double>(VGRD_10maboveground_str);


                    //Add AORC struct to AORC vector
                    AORC_vector.push_back(AORC);
            
                    //Convert precip_rate from string to double
                    double precip_rate = boost::lexical_cast<double>(precip_rate_str);

                    //Add data to vectors
                    precipitation_rate_meters_per_second_vector.push_back(precip_rate);
                    time_epoch_vector.push_back(current_row_date_time_epoch);
                }

                //Free memory from struct
                //delete current_row_date_time_utc;
        }
    }

    std::vector<std::string> available_forcings;
    vector<AORC_data> AORC_vector;
    /**
     * Whether this forcing instance has read in full AORC data or just ``precipitation_rate_meters_per_second_vector``.
     */
    bool is_aorc_data = false;

    /// \todo: Look into aggregation of data, relevant libraries, and storing frequency information
    vector<double> precipitation_rate_meters_per_second_vector;

    /// \todo: Consider making epoch time the iterator
    vector<time_t> time_epoch_vector;     
    int forcing_vector_index;
    double precipitation_rate_meters_per_second;
    double air_temperature_fahrenheit;
    double latitude; //latitude (degrees_north)
    double longitude; //longitude (degrees_east)
    int catchment_id;
    int day_of_year;
    string forcing_file_name;

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

#endif // FORCING_H
