#ifndef NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
#define NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP

#include <NGenConfig.h>

#if NGEN_WITH_NETCDF

#include "GenericDataProvider.hpp"
#include "DataProviderSelectors.hpp"
#include "AorcForcing.hpp"

#include "utilities/multithreading.hpp"

#include <assert.h>

#include <algorithm>
#include <atomic>
#include <ctime>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace netCDF {
    class NcVar;
    class NcFile;
}

namespace data_access
{
    class NetCDFPerFeatureDataProvider : public GenericDataProvider
    {
        
        public:

        enum class TimeUnit
        {
            Days,
            Hours,
            Minutes,
            Seconds,
            Milliseconds,
            Microseconds,
            Nanoseconds
        };

        /**
         * @brief Time metadata for netcdf time units.
         */
        struct TimeInfo
        {
            TimeUnit unit;                  //the unit raw time values are stored in
            double scale_factor;            //multiplier converting a raw value to seconds
            std::optional<std::time_t> epoch_start_time;   //reference epoch, in seconds since the Unix epoch, if specified
        };

        /**
         * @brief Interpret the name of a time unit as a time unit and scale factor.
         *
         * Takes a bare unit, such as the part of CF "<unit> since <date>" time
         * units before " since ", and returns no reference epoch.
         *
         * @param units_str the unit's name (e.g. "ns", "hours")
         * @return the parsed @ref TimeInfo, or std::nullopt if @p units_str is unrecognized
         */
        static std::optional<TimeInfo> interpret_time_units(const std::string& units_str);

        /**
         * @brief Parse a reference date: that of CF "<unit> since <date>" time
         * units, or an `epoch_start` attribute.
         *
         * Accepts "YYYY-MM-DD", or "MM/DD/YYYY" or "MM/DD/YY" (with years 69 to 99
         * in the 1900s and 00 to 68 in the 2000s), optionally followed by a space or 'T' and
         * "hh:mm", "hh:mm:ss" or "hh:mm:ss.000" (any fraction must be zero), and
         * then optionally "Z", "UTC" or an offset from UTC ("+hh:mm", "-hhmm", "+hh")
         * of less than 24 hours, which is applied. Month, day and hour may have a
         * single digit.
         *
         * @param date_str the reference date text
         * @return seconds since the Unix epoch
         * @throws std::runtime_error if @p date_str isn't such a date
         */
        static std::time_t parse_epoch(const std::string& date_str);

        /**
         * @brief Factory method that creates or returns an existing provider for the provided path.
         * @param input_path The path to a NetCDF file with lumped catchment forcing values.
         */
        static std::shared_ptr<NetCDFPerFeatureDataProvider> get_shared_provider(std::string input_path, time_t sim_start, time_t sim_end);

        /**
         * @brief Tell provider an id it is expected to provide.
         */
        void hint_shared_provider_id(const std::string& id);

        /**
         * @brief Cleanup the shared providers cache, ensuring that the files get closed.
         */
        static void cleanup_shared_providers();

        NetCDFPerFeatureDataProvider(std::string input_path, time_t sim_start, time_t sim_end);
        NetCDFPerFeatureDataProvider() = delete;
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

        // Key is the index of a page's first time step (c_idx)
        using cache_key_type = std::size_t;
        using page_buffer_type = std::shared_ptr<std::vector<double>>;

        using shared_cache = multithreading::shared_cache<cache_key_type, std::vector<double>>;

        private:
        // What the provider knows about a variable in the file; defined in
        // the .cpp file, so that client code doesn't need NcVar's definition
        struct VariableInfo;

        page_buffer_type load_page(VariableInfo const& var, std::size_t page_c_idx);

        time_t sim_start_date_time_epoch;
        time_t sim_end_date_time_epoch;
        time_t sim_to_data_time_offset; // Deliberately signed--sim should never start before data, yes?

        static std::mutex shared_providers_mutex;
        static std::map<std::string, std::shared_ptr<NetCDFPerFeatureDataProvider>> shared_providers;

        std::mutex hinted_ids_mutex;
        std::atomic_flag hinted_ids_done;
        std::set<std::string> hinted_ids;

        std::vector<std::string> variable_names;
        std::string catchment_dim_name;                 // the dimension of "ids"
        std::vector<std::string> loc_ids;
        std::vector<double> time_vals;
        std::map<std::string, std::size_t> id_pos;      // map from cat-id to position in vec of nc var values; accounts for chunking
        std::vector<std::pair<size_t, size_t>> chunks;  // a chunk is the start and length of a span in the "catchment-id" dim of a nc variable
        double start_time;                              // the begining of the first time for which data is stored
        double stop_time;                               // the end of the last time for which data is stored
        TimeUnit time_unit;                             // the unit that time was stored as in the file
        double time_stride;                             // the amount of time between stored time values
        std::string file_path;

        // Its deleter takes the NetCDF library lock, since destroying
        // an NcFile closes it if it is still open
        std::shared_ptr<netCDF::NcFile> nc_file;

        // By the name requested, which may be the CSDMS name: what's
        // needed to serve reads of the variable from the file
        std::map<std::string, VariableInfo> ncvar_cache;

        // CSDMS names that more than one variable in the file has, with
        // those variables' names; reads by these names are errors
        std::map<std::string, std::vector<std::string>> ambiguous_names;

        // One cache per variable, keyed by the variable's name in the
        // file. Reads by any name that ncvar_cache resolves to the
        // variable, such as its CSDMS name, share its pages.
        //
        // Populated by the constructor and not modified after, so
        // threads can look up a variable's cache without locking.
        //
        // Separate caches keep lookups and evicting inserts for one
        // variable from contending with readers of the others.
        std::map<std::string, shared_cache> value_caches;

        // The buffers of pages a thread has already looked up, so its
        // repeated reads of a page skip the shared cache's lock and a
        // shared_ptr copy: by the variable's name in the file, as for
        // value_caches, then by the same page key as that variable's
        // shared cache.
        using thread_cache_type = std::map<std::string, std::map<cache_key_type, page_buffer_type>>;

        // Each thread's cache for this provider
        multithreading::per_thread<thread_cache_type> thread_caches;

        // number of time slices per cache entry
        // this is a tunable parameter; your mileage may vary
        // NOTE: it would be nice if this were divisible by 2 and 4
        size_t cache_slice_t_size = 24;
        size_t cache_slice_c_size = 1;

        VariableInfo const& get_ncvar(const std::string& name) const;

        void test_data_is_readable();

        /**
         * @brief Record each of the file's variables in ncvar_cache, by its
         * name and, for a well-known field, its CSDMS name, with its units.
         * Called by the constructor, holding the NetCDF library lock.
         */
        void read_variable_metadata();

        /**
         * @brief Read the catchment ids, and the position of each in the file.
         * Called by the constructor, holding the NetCDF library lock.
         */
        void read_ids();

        /**
         * @brief Read the times of the file's values, and work out the time
         * range and step they cover.
         * Called by the constructor, holding the NetCDF library lock.
         */
        void read_times();

        /**
         * @brief Read the time unit, scale factor and reference epoch from a `Time` variable.
         *
         * Takes the `units` attribute from the variable's VariableInfo, splitting
         * CF "<unit> since <date>" units, and resolves the unit via
         * @ref interpret_time_units, and the reference
         * date, from the units or else the `epoch_start` attribute, via
         * @ref parse_epoch. Warns and falls back to sensible defaults
         * (seconds, Unix epoch) when an attribute is absent or empty.
         *
         * @param time_var the VariableInfo of the file's `Time` variable
         * @return the resolved time metadata
         * @throws std::runtime_error if `units` isn't a recognized time unit, or the
         * reference date can't be parsed
         */
        TimeInfo get_time_metadata(const VariableInfo& time_var);

        /**
         * @brief Attempts to align the internal cache size with the chunking parameters of
         * the underlying netCDF variables.
         * If no chunking is present, the default cache size is used.
         *
         * This can have some significant performance implications.
         * both in terms of memory use and speed of access.
         * If the variables are chunked too large, then the cache will hold
         * a lot of data in memory.  If they are too small, then we end up
         * doing a lot of small reads from the netCDF file.
         */
        void align_cache_with_chunks();

        /**
         * @brief If applicable, update chunk spans.
         * Note, no hint_shared_provider_id() calls should be made afterwards.
         * Note, additional calls have no effect.
         */
        void maybe_update_chunks_with_hints();
    };
}


#endif // NGEN_WITH_NETCDF
#endif // NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
