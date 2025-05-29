#ifndef NGEN_WRAPPEDFORCINGPROVIDER_HPP
#define NGEN_WRAPPEDFORCINGPROVIDER_HPP

#include <memory>
#include "GenericDataProvider.hpp"

namespace data_access {

    /**
     * Simple implementation that basically wraps another instance.
     *
     * The primary purpose is to support self-referencing as a forcing provider (e.g., for ET) without introducing any
     * circular references in smart pointers.
     *
     * Instances of this type must be used carefully, as they do not contain any mechanisms for making sure they
     * actually point to a valid backing object.
     */
    class WrappedDataProvider : public GenericDataProvider {

    public:

        /**
         * Primary constructor for instances.
         *
         * @param provider Simple pointer to some existing @ref ForcingProvider object for this new instance to wrap.
         */
        explicit WrappedDataProvider(GenericDataProvider* provider) : wrapped_provider(provider) { }

        /**
         * Copy constructor.
         *
         * @param provider_to_copy
         */
        WrappedDataProvider(WrappedDataProvider &provider_to_copy)
            : wrapped_provider(provider_to_copy.wrapped_provider) { }

        /**
         * Move constructor.
         *
         * @param provider_to_move
         */
        WrappedDataProvider(WrappedDataProvider &&provider_to_move)
            : wrapped_provider(provider_to_move.wrapped_provider)
        {
            provider_to_move.wrapped_provider = nullptr;
        }

        void finalize() override {
            /*
              Do nothing here. This relies on an assumption that the
              owner of `wrapped_provider` has or will call finalize()
              on it.
            */
        }

        /**
         * @brief Get the available variable names object
         * 
         * @return const std::vector<std::string>& the names of available data variables
         */

        boost::span<const std::string> get_available_variable_names() const override {
            return wrapped_provider->get_available_variable_names();
        }

        /**
         * Get the inclusive beginning of the period of time over which this instance can provide data for this forcing.
         *
         * @return The inclusive beginning of the period of time over which this instance can provide this data.
         */
        long get_data_start_time() const override {
            return wrapped_provider->get_data_start_time();
        }

        /**
         * Get the exclusive ending of the period of time over which this instance can provide data for this forcing.
         *
         * @return The exclusive ending of the period of time over which this instance can provide this data.
         */
        long get_data_stop_time() const override {
            return wrapped_provider->get_data_stop_time();
        }

        long record_duration() const override {
            return wrapped_provider->record_duration();
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
        size_t get_ts_index_for_time(const time_t &epoch_time) const override {
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
        double get_value(const CatchmentAggrDataSelector& selector, ReSampleMethod m) override
        {
            return wrapped_provider->get_value(selector, m);
        }

        std::vector<double> get_values(const CatchmentAggrDataSelector& selector, ReSampleMethod m) override
        {
            return wrapped_provider->get_values(selector, m);
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
        bool is_property_sum_over_time_step(const std::string& name) const override {
            return wrapped_provider->is_property_sum_over_time_step(name);
        }

    protected:
        GenericDataProvider* wrapped_provider;

    };
}

#endif //NGEN_WRAPPEDFORCINGPROVIDER_HPP
