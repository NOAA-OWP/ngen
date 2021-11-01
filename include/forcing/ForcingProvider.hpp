#ifndef NGEN_FORCINGPROVIDER_HPP
#define NGEN_FORCINGPROVIDER_HPP

#include <map>
#include <vector>

namespace forcing {

    /**
     * An abstraction interface for types that provides forcing data.
     *
     * Data may be pre-provided from some external source, internally calculated by the implementing type, or some
     * combination of both.
     */
    class ForcingProvider {
        /*
         * TODO: later add this to docstring, and then implement these functions:
         *
         * Functions are available for getting whether is can be expected for any valid time periods
         * to have data for all valid forcings, and whether a valid time period for some forcing ``F`` is guaranteed to
         * remain valid for the life of the object.
         */

    public:

        virtual ~ForcingProvider() = default;

        virtual const std::vector<std::string> &get_available_forcing_outputs() = 0;

        /**
         * Get the inclusive beginning of the period of time over which this instance can provide data for this forcing.
         *
         * @return The inclusive beginning of the period of time over which this instance can provide this data.
         */
        virtual time_t get_forcing_output_time_begin(const std::string &output_name) = 0;

        /**
         * Get the exclusive ending of the period of time over which this instance can provide data for this forcing.
         *
         * @return The exclusive ending of the period of time over which this instance can provide this data.
         */
        virtual time_t get_forcing_output_time_end(const std::string &output_name) = 0;

        /**
         * Get the index of the forcing time step that contains the given point in time.
         *
         * An @ref std::out_of_range exception should be thrown if the time is not in any time step.
         *
         * @param epoch_time The point in time, as a seconds-based epoch time.
         * @return The index of the forcing time step that contains the given point in time.
         * @throws std::out_of_range If the given point is not in any time step.
         */
        virtual size_t get_ts_index_for_time(const time_t &epoch_time) = 0;

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
        virtual double get_value(const std::string &output_name, const time_t &init_time, const long &duration_s,
                                 const std::string &output_units) = 0;

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
        virtual bool is_property_sum_over_time_step(const std::string& name) = 0;

    };
}

#endif //NGEN_FORCINGPROVIDER_HPP
