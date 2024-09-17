#pragma once

#include <NGenConfig.h>

#if NGEN_WITH_NETCDF

#include "GenericDataProvider.hpp"
#include "DataProviderSelectors.hpp"

#include <string>
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <exception>
#include <mutex>
#include "assert.h"
#include <iomanip>
#include <boost/compute/detail/lru_cache.hpp>

#include <UnitsHelper.hpp>
#include <StreamHandler.hpp>

#include "AorcForcing.hpp"

namespace netCDF {
    class NcVar;
    class NcFile;
}

namespace data_access
{
    class NetCDFMeshPointsDataProvider : public MeshPointsDataProvider
    {
        public:

        enum TimeUnit
        {
            TIME_HOURS,
            TIME_MINUTES,
            TIME_SECONDS,
            TIME_MILLISECONDS,
            TIME_MICROSECONDS,
            TIME_NANOSECONDS
        };

        using time_point_type = std::chrono::time_point<std::chrono::system_clock>;

        NetCDFMeshPointsDataProvider(std::string input_path,
                                     time_point_type sim_start,
                                     time_point_type sim_end);

        // Default implementation defined in the .cpp file so that
        // client code doesn't need to have the full definition of
        // NcFile visible for the compiler to implicitly generate
        // ~NetCDFMeshPointsDataProvider() = default;
        // for every file that uses this class
        ~NetCDFMeshPointsDataProvider();

        void finalize() override;

        /** Return the variables that are accessable by this data provider */
        boost::span<const std::string> get_available_variable_names() const override;

        /** Return the first valid time for which data from the request variable  can be requested */
        long get_data_start_time() const override;

        /** Return the last valid time for which data from the requested variable can be requested */
        long get_data_stop_time() const override;

        long record_duration() const override;

        /**
         * Get the index of the data time step that contains the given point in time.
         *
         * An @ref std::out_of_range exception should be thrown if the time is not in any time step.
         *
         * @param epoch_time The point in time, as a seconds-based epoch time.
         * @return The index of the forcing time step that contains the given point in time.
         * @throws std::out_of_range If the given point is not in any time step.
         */
        size_t get_ts_index_for_time(const time_t &epoch_time) const override;

        /**
         * Get the value of a forcing property for an arbitrary time period, converting units if needed.
         *
         * An @ref std::out_of_range exception should be thrown if the data for the time period is not available.
         *
         * @param selector Data required to establish what subset of the stored data should be accessed
         * @param m How data is to be resampled if there is a mismatch in data alignment or repeat rate
         * @return The value of the forcing property for the described time period, with units converted if needed.
         * @throws std::out_of_range If data for the time period is not available.
         */
        double get_value(const selection_type& selector, ReSampleMethod m) override;

        // The interface that DataProvider really should have
        virtual void get_values(const selection_type& selector, boost::span<double> data);

        // And an implementation of the usual version using it
        std::vector<data_type> get_values(const selection_type& selector, data_access::ReSampleMethod) override
        {
            std::vector<data_type> output(selected_points_count(selector));
            get_values(selector, output);
            return output;
        }

        private:

        size_t mesh_size(std::string const& variable_name);

        size_t selected_points_count(const selection_type& selector)
        {
            auto* points = boost::get<std::vector<int>>(&selector.points);
            size_t size = points ? points->size() : this->mesh_size(selector.variable_name);
            return size;
        }

        time_point_type sim_start_date_time_epoch;
        time_point_type sim_end_date_time_epoch;
        std::chrono::seconds sim_to_data_time_offset; // Deliberately signed--sim should never start before data, yes?

        std::vector<std::string> variable_names;
        std::vector<time_point_type> time_vals;
        std::chrono::seconds time_stride;                             // the amount of time between stored time values

        std::shared_ptr<netCDF::NcFile> nc_file;

        struct metadata_cache_entry;
        std::map<std::string, metadata_cache_entry> ncvar_cache;
    };
}


#endif // NGEN_WITH_NETCDF
