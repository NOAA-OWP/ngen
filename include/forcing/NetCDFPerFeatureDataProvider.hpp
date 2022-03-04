#ifndef NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
#define NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP

#include "DataProvider.hpp"

#include <string>
#include <netcdf>

using namespace netCDF;
using namespace netCDF::exceptions;

namespace data_access
{
    class Catchment_Id
    {
        public:

        Catchment_Id(std::string id) : id_str(id) {}
        Catchment_Id(const char* id) : id_str(id) {}

        operator std::string() const { return id_str; }

        private:

        std::string id_str;
    };

    class NetCDFPerFeatureDataProvider : public DataProvider<double, Catchment_Id>
    {
        
        public:

        NetCDFPerFeatureDataProvider(std::string input_path)
        {
            nc_file = std::make_shared<NcFile>(input_path, NcFile::read);
        }

        NetCDFPerFeatureDataProvider(const char* input_path) : 
            NetCDFPerFeatureDataProvider(std::string(input_path))
        {

        }

        /** Return the variables that are accessable by this data provider */

        const std::vector<std::string> get_avaliable_variable_names()
        {
            return variable_names;
        }

        /** Return the first valid time for which data from the request variable  can be requested */

        int get_data_start_time(std::string var)
        {
            return start_time;
        }

        /** Return the last valid time for which data from the requested variable can be requested */

        int get_data_stop_time(std::string var)
        {
            return stop_time;
        }

        /**
         * Get the index of the data time step that contains the given point in time.
         *
         * An @ref std::out_of_range exception should be thrown if the time is not in any time step.
         *
         * @param epoch_time The point in time, as a seconds-based epoch time.
         * @return The index of the forcing time step that contains the given point in time.
         * @throws std::out_of_range If the given point is not in any time step.
         */
        size_t get_ts_index_for_time(const time_t &epoch_time)
        {
            return 0;
        }

        /**
         * Get the value of a forcing property for an arbitrary time period, converting units if needed.
         *
         * An @ref std::out_of_range exception should be thrown if the data for the time period is not available.
         *
         * @param selector Data required to establish what subset of the stored data should be accessed
         * @param variable_name The name of the data property of interest.
         * @param init_time_epoch The epoch time (in seconds) of the start of the time period.
         * @param duration_seconds The length of the time period, in seconds.
         * @param output_units The expected units of the desired output value.
         * @return The value of the forcing property for the described time period, with units converted if needed.
         * @throws std::out_of_range If data for the time period is not available.
         */
        double get_value(Catchment_Id selector, const std::string &variable_name, const time_t &init_time, const long &duration_s,
                                 const std::string &output_units)
        {
            return 0.0;
        }

        private:

        std::vector<std::string> variable_names;
        int start_time;
        int stop_time;

        std::shared_ptr<NcFile> nc_file;

    };
}


#endif // NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
