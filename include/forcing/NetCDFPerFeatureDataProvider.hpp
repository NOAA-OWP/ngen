#ifndef NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
#define NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP

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
    class NetCDFPerFeatureDataProvider : public GenericDataProvider
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

        /**
         * @brief Factory method that creates or returns an existing provider for the provided path.
         * @param input_path The path to a NetCDF file with lumped catchment forcing values.
         * @param log_s An output log stream for messages from the underlying library. If a provider object for
         * the given path already exists, this argument will be ignored.
         */
        static std::shared_ptr<NetCDFPerFeatureDataProvider> get_shared_provider(std::string input_path, time_t sim_start, time_t sim_end, utils::StreamHandler log_s, bool enable_cache = true);

        /**
         * @brief Cleanup the shared providers cache, ensuring that the files get closed.
         */
        static void cleanup_shared_providers();

        NetCDFPerFeatureDataProvider(std::string input_path, time_t sim_start, time_t sim_end,  utils::StreamHandler log_s, bool enable_cache = true);

        // Default implementation defined in the .cpp file so that
        // client code doesn't need to have the full definition of
        // NcFile visible for the compiler to implicitly generate
        // ~NetCDFPerFeatureDataProvider() = default;
        // for every file that uses this class
        ~NetCDFPerFeatureDataProvider();

        void finalize() override;

        /** Return the variables that are accessable by this data provider */
        boost::span<const std::string> get_available_variable_names() const override;

        /** return a list of ids in the current file */
        const std::vector<std::string>& get_ids() const;

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
        double get_value(const CatchmentAggrDataSelector& selector, ReSampleMethod m) override;

        virtual std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override;

        private:

        time_t sim_start_date_time_epoch;
        time_t sim_end_date_time_epoch;
        time_t sim_to_data_time_offset; // Deliberately signed--sim should never start before data, yes?

        static std::mutex shared_providers_mutex;
        static std::map<std::string, std::shared_ptr<NetCDFPerFeatureDataProvider>> shared_providers;

        std::vector<std::string> variable_names;
        std::vector<std::string> loc_ids;
        std::vector<double> time_vals;
        std::map<std::string, std::size_t> id_pos;
        double start_time;                              // the begining of the first time for which data is stored
        double stop_time;                               // the end of the last time for which data is stored
        TimeUnit time_unit;                             // the unit that time was stored as in the file
        double time_stride;                             // the amount of time between stored time values
        utils::StreamHandler log_stream;


        std::shared_ptr<netCDF::NcFile> nc_file;

        std::map<std::string,netCDF::NcVar> ncvar_cache;
        std::map<std::string,std::string> units_cache;
        boost::compute::detail::lru_cache<std::string, std::shared_ptr<std::vector<double>>> value_cache;
        bool enable_cache;
        size_t cache_slice_t_size = 1;
        size_t cache_slice_c_size = 1;

        const netCDF::NcVar& get_ncvar(const std::string& name);

        const std::string& get_ncvar_units(const std::string& name);

    };
}


#endif // NGEN_WITH_NETCDF
#endif // NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
