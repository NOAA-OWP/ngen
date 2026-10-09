#ifndef NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP
#define NGEN_NETCDF_PER_FEATURE_DATAPROVIDER_HPP

#include <NGenConfig.h>

#if NGEN_WITH_NETCDF

#include "GenericDataProvider.hpp"
#include "DataProviderSelectors.hpp"

#include <atomic>
#include <string>
#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <set>
#include <sstream>
#include <exception>
#include <shared_mutex>
#include "assert.h"
#include <iomanip>
#include <optional>
#include <ctime>
#include <boost/compute/detail/lru_cache.hpp>

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
         * @brief Time metadata for netcdf time units.
         */
        struct TimeInfo
        {
            TimeUnit unit;                  //the unit raw time values are stored in
            double scale_factor;            //multiplier converting a raw value to seconds
            std::optional<std::time_t> epoch_start_time;   //reference epoch, in seconds since the Unix epoch, if specified
        };

        /**
         * @brief Interpret a NetCDF time `units` string as a time unit and scale factor.
         *
         * Handles both bare units and the CF form "<unit> since <date>", in which case
         * the parsed reference epoch is returned in the result.
         *
         * @param units_str the raw `units` attribute value (e.g. "ns", "hours")
         * @return the parsed @ref TimeInfo, or std::nullopt if @p units_str is unrecognized
         */
        static std::optional<TimeInfo> interpret_time_units(const std::string& units_str);

        /**
         * @brief Parse a reference-epoch timestamp into seconds since the Unix epoch.
         *
         * @param epoch_str the timestamp text (e.g. "01/01/1970 00:00:00")
         * @param format a std::get_time / strptime style format string
         * @return seconds since the Unix epoch
         */
        static std::time_t parse_epoch(const std::string& epoch_str, const std::string& format);

        /**
         * @brief Factory method that creates or returns an existing provider for the provided path.
         * @param input_path The path to a NetCDF file with lumped catchment forcing values.
         * @param log_s An output log stream for messages from the underlying library. If a provider object for
         * the given path already exists, this argument will be ignored.
         */
        static std::shared_ptr<NetCDFPerFeatureDataProvider> get_shared_provider(std::string input_path, time_t sim_start, time_t sim_end, utils::StreamHandler log_s);

        /**
         * @brief Tell provider an id it is expected to provide.
         */
        void hint_shared_provider_id(const std::string& id);

        /**
         * @brief Cleanup the shared providers cache, ensuring that the files get closed.
         */
        static void cleanup_shared_providers();

        NetCDFPerFeatureDataProvider(std::string input_path, time_t sim_start, time_t sim_end,  utils::StreamHandler log_s);
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
        using cache_key_type = int;
        using cache_buffer_type = std::shared_ptr<std::vector<double>>;

        // value is a pointer to the
        // cached data; if the pointer is null, another thread has
        // started filling it in, but is not done yet
        struct cache_slot {
            cache_slot() = default;

            cache_slot(cache_slot const&) = delete;
            cache_slot(cache_slot &&) = delete;

            // Get back a non-nullptr buffer pointer that can safely
            // be read from. If it's not already set, this may block
            // on another thread that will fill said pointer
            cache_buffer_type get();

            // Fill in this slot with the provided buffer pointer and
            // safely unblock any threads waiting for it. Must be
            // called with a non-null pointer, on a slot that is
            // EMPTY, or in ERROR to retry a failed fill
            void fill(cache_buffer_type buffer_ptr, bool immediate_use);

        private:
            cache_buffer_type ptr_ = nullptr;
            // ERROR marks a failed fill, which fill() may retry
            enum STATE : char { EMPTY=0, FILLED=1, HOT=2, ERROR=3 };
            std::atomic<int> state_ = STATE::EMPTY;
        };

        struct shared_cache {
            // Get back a cache slot matching the @arg key. Exactly
            // one thread will return 'true' the first time `key` is
            // passed
            //
            // Keys are expected to increase monotonically as the
            // simulation advances, so a key is not looked up again
            // after it is evicted. If it is, it is inserted again,
            // and this returns 'true' again
            //
            // If this call inserts a slot and @arg eviction_floor is
            // set, first evict all slots whose time index is below
            // it. Callers must not pass a floor above the time index
            // of any slot another thread may still be using, which
            // includes the key being looked up. Eviction is handled
            // here to take advantage of the locking on the internal
            // structure necessary for insertion.
            std::pair<cache_slot&, bool> find_or_insert(cache_key_type key, std::optional<int> eviction_floor);

            // The keys of all slots currently in the cache
            std::set<cache_key_type> keys() const;

        private:
            std::map<cache_key_type, cache_slot> cache;
            mutable std::shared_mutex mutex;
        };

        private:
        cache_buffer_type fill_slot(int page_c_idx, netCDF::NcVar const& ncvar, cache_slot& slot, bool immediate_use);

        time_t sim_start_date_time_epoch;
        time_t sim_end_date_time_epoch;
        time_t sim_to_data_time_offset; // Deliberately signed--sim should never start before data, yes?

        static std::mutex shared_providers_mutex;
        static std::map<std::string, std::shared_ptr<NetCDFPerFeatureDataProvider>> shared_providers;

        std::mutex hinted_ids_mutex;
        std::atomic_flag hinted_ids_done;
        std::set<std::string> hinted_ids;

        std::vector<std::string> variable_names;
        std::vector<std::string> loc_ids;
        std::vector<double> time_vals;
        std::map<std::string, std::size_t> id_pos;      // map from cat-id to position in vec of nc var values; accounts for chunking
        std::vector<std::pair<size_t, size_t>> chunks;  // a chunk is the start and length of a span in the "catchment-id" dim of a nc variable
        double start_time;                              // the begining of the first time for which data is stored
        double stop_time;                               // the end of the last time for which data is stored
        TimeUnit time_unit;                             // the unit that time was stored as in the file
        double time_stride;                             // the amount of time between stored time values
        utils::StreamHandler log_stream;
        std::string file_path;

        std::mutex netcdf_library_mutex;
        std::shared_ptr<netCDF::NcFile> nc_file;

        std::map<std::string, std::pair<std::string, netCDF::NcVar>> ncvar_cache;
        std::map<std::string,std::string> units_cache;

        // One cache per variable, keyed by the variable's name in the
        // file. Populated by the constructor and not modified after, so
        // threads can look up a variable's cache without locking.
        std::map<std::string, shared_cache> value_caches;

        // number of time slices per cache entry
        // this is a tunable parameter; your mileage may vary
        // NOTE: it would be nice if this were divisible by 2 and 4
        size_t cache_slice_t_size = 24;
        size_t cache_slice_c_size = 1;

        std::pair<std::string, netCDF::NcVar> const& get_ncvar(const std::string& name) const;

        const std::string& get_ncvar_units(const std::string& name);

        void test_data_is_readable();

        /**
         * @brief Read the time unit, scale factor and reference epoch from a `Time` variable.
         *
         * Reads the `units` and `epoch_start` attributes and resolves them via
         * @ref interpret_time_units and @ref parse_epoch, warning and falling back to
         * sensible defaults (seconds, Unix epoch) when an attribute is absent or
         * unrecognized.
         *
         * @param time_var the file's `Time` variable
         * @return the resolved time metadata
         */
        TimeInfo get_time_metadata(const netCDF::NcVar& time_var);

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
