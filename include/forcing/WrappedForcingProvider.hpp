#ifndef NGEN_WRAPPEDFORCINGPROVIDER_HPP
#define NGEN_WRAPPEDFORCINGPROVIDER_HPP

#include <memory>
#include "ForcingProvider.hpp"

namespace forcing {

    /**
     * Simple implementation that basically wraps another instance.
     *
     * The primary purpose is to support self-referencing as a forcing provider (e.g., for ET) without introducing any
     * circular references in smart pointers.
     *
     * Instances of this type must be used carefully, as they do not contain any mechanisms for making sure they
     * actually point to a valid backing object.
     */
    class WrappedForcingProvider : public ForcingProvider {

    public:

        /**
         * Primary constructor for instances.
         *
         * @param provider Simple pointer to some existing @ref ForcingProvider object for this new instance to wrap.
         */
        explicit WrappedForcingProvider(ForcingProvider* provider) : wrapped_provider(provider) { }

        /**
         * Copy constructor.
         *
         * @param provider_to_copy
         */
        WrappedForcingProvider(WrappedForcingProvider &provider_to_copy)
            : wrapped_provider(provider_to_copy.wrapped_provider) { }

        /**
         * Move constructor.
         *
         * @param provider_to_move
         */
        WrappedForcingProvider(WrappedForcingProvider &&provider_to_move)
            : wrapped_provider(provider_to_move.wrapped_provider)
        {
            provider_to_move.wrapped_provider = nullptr;
        }

        const std::vector<std::string> &get_available_forcing_outputs() override {
            return wrapped_provider->get_available_forcing_outputs();
        }

        /**
         * Get the inclusive beginning of the period of time over which this instance can provide data for this forcing.
         *
         * @return The inclusive beginning of the period of time over which this instance can provide this data.
         */
        time_t get_forcing_output_time_begin(const std::string &output_name) override {
            return wrapped_provider->get_forcing_output_time_begin(output_name);
        }

        /**
         * Get the exclusive ending of the period of time over which this instance can provide data for this forcing.
         *
         * @return The exclusive ending of the period of time over which this instance can provide this data.
         */
        time_t get_forcing_output_time_end(const std::string &output_name) override {
            return wrapped_provider->get_forcing_output_time_end(output_name);
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
            return wrapped_provider->get_ts_index_for_time(epoch_time);
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
            return wrapped_provider->get_value(output_name, init_time, duration_s, output_units);
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
        bool is_property_sum_over_time_step(const std::string& name) override {
            return wrapped_provider->is_property_sum_over_time_step(name);
        }

    private:
        ForcingProvider* wrapped_provider;

    };
}

#endif //NGEN_WRAPPEDFORCINGPROVIDER_HPP
