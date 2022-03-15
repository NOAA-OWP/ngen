#ifndef NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
#define NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP

#include "DataProvider.hpp"

#include <string>
#include <algorithm>
#include <sstream>
#include <exception>

#include <UnitsHelper.hpp>

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

        enum TimeUnit
        {
            TIME_HOURS,
            TIME_MINUETS,
            TIME_SECONDS,
            TIME_MILLISECONDS,
            TIME_MICROSECONDS,
            TIME_NANOSECONDS
        };

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
                
                //free(str);
            });

            // correct string release
            ids.freeString(num_ids,&string_buffers[0]);

            // now get the size of the time dimension
            auto num_times = nc_file->getDim("time").getSize();

            // allocate storage for the raw time array
            std::vector<double> raw_time;
            raw_time.resize(num_times);

            // get the time variable
            auto time_var = nc_file->getVar("Time");

            // read from the first catchment row to get the recorded times
            std::vector<size_t> start;
            start.push_back(0);
            start.push_back(0);
            std::vector<size_t> count;
            count.push_back(1);
            count.push_back(num_times);
            time_var.getVar(start, count, &raw_time[0]);

            // read the meta data to get the time_unit
            auto time_unit_att = time_var.getAtt("units");

            // if time att is not encoded 
            // TODO determine how this should be handled
            std::string time_unit_str;

            if ( time_unit_att.isNull() )
            {
                time_unit = TIME_SECONDS;
            }
            else
            {  
                time_unit_att.getValues(time_unit_str);
            }

            if ( time_unit_str == "h" || time_unit_str == "hours ")
                time_unit = TIME_HOURS;
            else if ( time_unit_str == "m" || time_unit_str == "minuets" )
                    time_unit = TIME_MINUETS;
            else if ( time_unit_str ==  "s" || time_unit_str == "seconds" )
                    time_unit = TIME_SECONDS;
            else if ( time_unit_str ==  "ms" || time_unit_str == "miliseconds" )
                    time_unit = TIME_SECONDS;
            else if ( time_unit_str ==  "us" || time_unit_str == "microseconds" )
                    time_unit = TIME_SECONDS;
            else if ( time_unit_str ==  "ns" || time_unit_str == "nanoseconds" )
                    time_unit = TIME_SECONDS;
            
            double time_scale_factor;

            switch (time_unit)
            {
                case TIME_HOURS:
                    time_scale_factor = 3600;
                break;

                case TIME_MINUETS:
                    time_scale_factor = 60;
                break;

                case TIME_SECONDS:
                    time_scale_factor = 1;
                break;

                case TIME_MILLISECONDS:
                    time_scale_factor = .001;
                break;

                case TIME_MICROSECONDS:
                    time_scale_factor = .000001;
                break;

                case TIME_NANOSECONDS:
                    time_scale_factor = .000000001;
                break;
            }

            // read the meta data to get the epoc start
            auto epoc_att = time_var.getAtt("epoc_start");
            std::string epoc_start_str;

            if ( epoc_att.isNull() )
            {
                epoc_start_str = "01/01/1970 00:00:00";
            }
            else
            {  
                epoc_att.getValues(epoc_start_str);
            }

            std::tm tm{};
            std::stringstream s(epoc_start_str);
            s >> std::get_time(&tm, "%F %R");
            std::time_t epoc_start_time = mktime(&tm);

            // scale the time to account for time units and epoc_start
            // TODO make sure this happens with a FMA instruction
            time_vals.resize(raw_time.size());
            std::transform(raw_time.begin(), raw_time.end(), time_vals.begin(), 
                [&](const auto& n){return n * time_scale_factor + epoc_start_time; });
                

            // determine the stride of the time array
            time_stride = time_vals[1] - time_vals[0];

            // determine start_time and stop_time;
            start_time = time_vals[0];
            stop_time = time_vals.back() + time_stride;


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

        long get_data_start_time()
        {
            return start_time;
        }

        /** Return the last valid time for which data from the requested variable can be requested */

        long get_data_stop_time()
        {
            return stop_time;
        }

        long record_duration()
        {
            return time_stride;
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
            if (start_time <= epoch_time && epoch_time < stop_time)
            {
                double offset = epoch_time - start_time;
                offset /= time_stride;
                return size_t(offset);
            }
            else
            {
                std::stringstream ss;
                ss << "The value " << epoch_time << "was not in the range [" << start_time << "," << stop_time << ")\n";
                throw std::out_of_range(ss.str().c_str());
            }
        }

        /**
         * Get the value of a forcing property for an arbitrary time period, converting units if needed.
         *
         * An @ref std::out_of_range exception should be thrown if the data for the time period is not available.
         *
         * @param selector Data required to establish what subset of the stored data should be accessed
         * @param variable_name The name of the data property of interest.
         * @param init_time The epoch time (in seconds) of the start of the time period.
         * @param duration_seconds The length of the time period, in seconds.
         * @param output_units The expected units of the desired output value.
         * @param m How data is to be resampled if there is a mismatch in data alignment or repeat rate
         * @return The value of the forcing property for the described time period, with units converted if needed.
         * @throws std::out_of_range If data for the time period is not available.
         */
        double get_value(Catchment_Id selector, const std::string &variable_name, const time_t &init_time, const long &duration_s,
                                 const std::string &output_units, ReSampleMethod m)
        {
            auto stop_time = init_time + duration_s;
            
            auto idx1 = get_ts_index_for_time(init_time);
            auto idx2 = get_ts_index_for_time(stop_time);

            auto stride = idx2 - idx1;

            std::vector<std::size_t> start, count;

            auto cat_pos = id_pos[selector];

            start.push_back(cat_pos);
            start.push_back(idx1);

            count.push_back(1);

            double t1 = time_vals[idx1];
            double t2 = time_vals[idx2];

            double rvalue = 0.0;

            auto ncvar = nc_file->getVar(variable_name);

            std::string native_units;
            
            try
            {
                auto units_att = ncvar.getAtt("units");
                if ( units_att.isNull() )
                {
                    native_units = "unknown";      
                }
                else
                {
                    units_att.getValues(native_units);
                }
            }
            catch(...)
            {
                native_units = "unknown";
            }

            // case where less than one time step is requested
            if ( idx1 == idx2)
            {
                count.push_back(1);

                ncvar.getVar(start,count,&rvalue);

                switch(m)
                {
                    case SUM:
                    rvalue *= (stride / time_stride);
                    break;
                }
            }
            // case where exactly 2 time steps are involved
            else if ( idx1 + 1 == idx2)
            {
                count.push_back(2);
                double raw_value[2];
                ncvar.getVar(start,count,raw_value);

                double a = (t2 - init_time) / time_stride;
                double b = (stop_time - t2) / time_stride;

                rvalue = (raw_value[0] * a) + (raw_value[1] * b);

                switch(m)
                {
                    case SUM:   // we allready have the sum so do nothing
                        ;
                    break;

                    case MEAN:  
                        // This is getting a length weighted mean
                        // the data values where allready scaled for where there was only partial use of a data value
                        // so we just need to do a final scale to account for the differnce between time_stride and duration_s
                        rvalue *= (time_stride / duration_s);
                    break;

                    default:
                        ;
                }
             }
            // general case
            else
            {
                auto read_len = idx2 - idx1 + 1;
                count.push_back(read_len);

                std::vector<double> raw_values;
                raw_values.resize(read_len);

                ncvar.getVar(start,count,&raw_values[0]);

                rvalue = 0.0;

                for( size_t i = 0; i < raw_values.size(); ++i )
                {
                    if (i == 0 ) // the first data value may not be fully in the time window
                    {
                        double a = 1.0 - ( (t1 - init_time) / time_stride );

                        rvalue += (a * raw_values[i]);
                    }
                    else if ( i == raw_values.size() - 1) // likewise the last data value may not be fully in the window
                    {
                        double b = (stop_time - t2) / time_stride;

                        rvalue += (b * raw_values[i]);
                    }
                    else // for all other values just accumulate
                    {
                        rvalue += raw_values[i];
                    }
                }

                // account for the resampling methods
                switch(m)
                {
                    case SUM:   // we allready have the sum so do nothing
                        ;
                    break;

                    case MEAN:  
                        // This is getting a length weighted mean
                        // the data values where allready scaled for where there was only partial use of a data value
                        // so we just need to do a final scale to account for the differnce between time_stride and duration_s
                        rvalue *= (time_stride / duration_s);
                    break;

                    default:
                        ;
                }
            }

            try 
            {
                return UnitsHelper::get_converted_value(native_units, rvalue, output_units);
            }
            catch (const std::runtime_error& e)
            {
                std::cerr<<"Unit conversion error: "<<std::endl<<e.what()<<std::endl<<"Returning unconverted value!"<<std::endl;
                return rvalue;
            }

            return rvalue;
        }

        private:

        std::vector<std::string> variable_names;
        std::vector<std::string> loc_ids;
        std::vector<double> time_vals;
        std::map<std::string, std::size_t> id_pos;
        double start_time;                              // the begining of the first time for which data is stored
        double stop_time;                               // the end of the last time for which data is stored
        TimeUnit time_unit;                             // the unit that time was stored as in the file
        double time_stride;                             // the amount of time between stored time values


        std::shared_ptr<NcFile> nc_file;

    };
}


#endif // NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
