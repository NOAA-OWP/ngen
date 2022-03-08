#ifndef NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
#define NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP

#include "DataProvider.hpp"

#include <string>
#include <algorithm>

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
            //open the file
            nc_file = std::make_shared<NcFile>(input_path, NcFile::read);

            //get the listing of all variables
            auto var_set = nc_file->getVars();

            // copy the variables names into the vector for easy use
            std::for_each(var_set.begin(), var_set.end(), [&](const auto& element)
                {
                    variable_names.push_back(element.first);
                });

            // read the variable ids
            auto ids = nc_file->getVar("ids"); 
            auto id_dim_count = ids.getDimCount();

            // some sanity checks
            if ( id_dim_count > 1)
            {
                throw std::runtime_error("Provided NetCDF file has an \"ids\" variable with more than 1 dimension");       
            }

            auto id_dim = ids.getDim(0);

            if (id_dim.isNull() )
            {
                throw std::runtime_error("Provided NetCDF file has a NuLL dimension for variable  \"ids\"");
            }

            auto num_ids = id_dim.getSize();

            // allocate an array of character pointers 
            std::vector< char* > string_buffers;

            // resize to match dimension size
            string_buffers.resize(num_ids);

            // read the id strings
            ids.getVar(&string_buffers[0]);

            // initalize the map of catchment-name to offset location and free the strings allocated by the C library
            size_t loc = 0;
            for_each( string_buffers.begin(), string_buffers.end(), [&](char* str)
            {
                loc_ids.push_back(str);
                id_pos[str] = loc++;
                free(str);
            });

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

        /** return a list of ids in the current file */
        const std::vector<std::string>& get_ids() const
        {
            return loc_ids;
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
        std::vector<std::string> loc_ids;
        std::map<std::string, std::size_t> id_pos;
        int start_time;
        int stop_time;

        std::shared_ptr<NcFile> nc_file;

    };
}


#endif // NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
