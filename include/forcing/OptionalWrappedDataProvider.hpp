#ifndef NGEN_OPTIONALWRAPPEDPROVIDER_HPP
#define NGEN_OPTIONALWRAPPEDPROVIDER_HPP

#include <utility>

#include "DeferredWrappedProvider.hpp"

using namespace std;

namespace data_access {

    /**
     * Extension of @ref DeferredWrappedProvider, where default values can be used instead of a backing provider.
     *
     * Default Values
     * ========================
     * This type supports receiving at construction a collection of default values.  A default value can be supplied for
     * one or more of the outputs the instance is create to provide.
     *
     * When a default is available for an output, an instance can use this default as the return value for calls to
     * @ref get_value instead of proxying a value from a backing provider.
     *
     * Valid Wrapped Providers And Readiness
     * ========================
     * The potential of default values has implications on what constitutes a valid wrapped provider.  In this type's
     * parent, @ref setWrappedProvider will only set the member variable if the would-be-wrapped-provider provides
     * **all** outputs required by the instance.  For this type, that restriction is loosened, and the wrapped provider
     * need not provide all the outputs required by the instance.
     *
     * If the instance has a default value for an output, then a would-be-wrapped-provider is not required to provide
     * that output. It must still provide all outputs for which the instance does not have a default value.
     *
     * The addition of default values also introduces an interesting edge case: when default values are provided for all
     * an instance's outputs.  There are two important implications of this. First, an instance requires a valid wrapped
     * provider to provide at least one of the instance's outputs. Second, an instance does not necessarily need a
     * wrapped provider to be set in order to be ready.
     *
     * When The Default Is Used
     * ========================
     * A default value will be used any time one is available for an output and a valid cannot be obtained from a
     * wrapped provider.
     *
     * Additionally, an instance can created to override a backing provider in certain situations.  One or more "wait"
     * counts can be supplied to the constructor. These "wait" counts represent the number of times an instance should
     * wait to proxy the corresponding output's value from a wrapped provider and instead return the default value.
     */
    class OptionalWrappedDataProvider : public DeferredWrappedProvider {

    public:
        /**
         * Constructor for instance.
         *
         * Also validates that given defaults only contain keys that are in given provided outputs.
         *
         * @param providedOuts The collection of the names of outputs this instance will need to provide.
         * @param defaultVals Mapping of some or all provided output defaults, keyed by output name.
         */
        OptionalWrappedDataProvider(vector<string> providedOuts, map<string, double> defaultVals)
                : DeferredWrappedProvider(std::move(providedOuts)), defaultValues(std::move(defaultVals))
        {
            // Validate the provided map of default values to ensure there aren't any unrecognized keys, as this
            //    constraint is later relied upon (i.e., keys of defaultValues being in the providedOutputs collection)
            if (!providedOutputs.empty() && !defaultValues.empty()) {
                for (const auto &def_vals_it : defaultValues) {
                    auto name_it = find(providedOutputs.begin(), providedOutputs.end(), def_vals_it.first);
                    if (name_it == providedOutputs.end()) {
                        string msg = "Invalid default values for OptionalWrappedDataProvider: default value given for "
                                     "unknown output with name '" + def_vals_it.first + "' (expected names are ["
                                     + providedOutputs[0];
                        for (int i = 1; i < providedOutputs.size(); ++i) {
                            msg += ", " + providedOutputs[i];
                        }
                        msg += "])";
                        throw runtime_error(msg);
                    }
                }
            }
        }

        /**
         * Constructor for instance including defaults and default wait counts.
         *
         * Also validates that given defaults only contain keys that are in given provided outputs.
         *
         * @param providedOuts The collection of the names of outputs this instance will need to provide.
         * @param defaultVals Mapping of some or all provided output defaults, keyed by output name.
         * @param defaultWaits Map of the number of default usages for each provided output that the instance must wait
         *                     before using values from the backing provider.
         */
        OptionalWrappedDataProvider(vector<string> providedOuts, map<string, double> defaultVals,
                                map<string, int> defaultWaits)
                : OptionalWrappedDataProvider(std::move(providedOuts), std::move(defaultVals))
        {
            // Validate that all keys/names in the defaultWaits map have corresponding key in defaultValues
            // (Note that this also depends on the delegated-to constructor to validate the keys in defaultValues)
            if (!defaultWaits.empty()) {
                for (const auto &wait_it : defaultWaits) {
                    auto def_it = defaultValues.find(wait_it.first);
                    if (def_it == defaultValues.end()) {
                        string msg = "Invalid default usage waits for OptionalWrappedDataProvider: wait count given for "
                                     "non-default output '" + wait_it.first + "' (outputs with defaults set are [";
                        def_it = defaultValues.begin();
                        msg += def_it->first;
                        def_it++;
                        while (def_it != defaultValues.end()) {
                            msg += ", " + def_it->first;
                            def_it++;
                        }
                        msg += "])";
                        throw runtime_error(msg);
                    }
                }
                // Assuming the mapping is valid, set it for the instance
                defaultUsageWaits = std::move(defaultWaits);
            }
        }

        /**
         * Convenience constructor for instance when default values are not provided.
         *
         * @param providedOutputs The collection of the names of outputs this instance will need to provide.
         */
        explicit OptionalWrappedDataProvider(vector<string> providedOutputs)
            : OptionalWrappedDataProvider(std::move(providedOutputs), map<string, double>()) { }

        /**
         * Convenience constructor for when there is only one provided output name, which does not have a default.
         *
         * @param outputName The name of the single output this instance will need to provide.
         */
        explicit OptionalWrappedDataProvider(const string& outputName) : OptionalWrappedDataProvider(vector<string>(1)) {
            providedOutputs[0] = outputName;
        }

        /**
         * Convenience constructor for instance with one output and default that must be used a minimum number of times.
         *
         * This constructor sets up the instance to provide an output and be able to supply a default value if the
         * backing provider is not set.  It also sets a minimum number of forced usages of the default value.  This will
         * result in the default being used even if the backing provider has been set, according to the implementations
         * of @ref isDefaultOverride and @ref recordUsingDefault.
         *
         * @param outputName The name of the single output this instance will need to provide.
         * @param defaultValue The default to associate with the given output.
         * @param defaultUsageWait The number of default usages that the instance must wait before using values from
         *                          the backing provider.
         */
        OptionalWrappedDataProvider(const string& outputName, double defaultValue, int defaultUsageWait)
            : OptionalWrappedDataProvider(outputName)
        {
            defaultValues[providedOutputs[0]] = defaultValue;
            if (defaultUsageWait > 0) {
                defaultUsageWaits[providedOutputs[0]] = defaultUsageWait;
            }
        }

        /**
         * Convenience constructor for when there is only one provided output name, which has a default value.
         *
         * @param outputName The name of the single output this instance will need to provide.
         * @param defaultValue The default to associate with the given output.
         */
        OptionalWrappedDataProvider(const string& outputName, double defaultValue)
            : OptionalWrappedDataProvider(outputName, defaultValue, 0) { }

        /**
         * Move constructor.
         *
         * @param provider_to_move
         */
        OptionalWrappedDataProvider(OptionalWrappedDataProvider &&provider_to_move) noexcept
                : OptionalWrappedDataProvider(provider_to_move.providedOutputs)
        {
            defaultValues = std::move(provider_to_move.defaultValues);
            defaultUsageWaits = std::move(provider_to_move.defaultUsageWaits);
            wrapped_provider = provider_to_move.wrapped_provider;
            provider_to_move.wrapped_provider = nullptr;
        }

        /**
         * Get the value of a forcing property for an arbitrary time period, converting units if needed.
         *
         * This implementation can supply a default value in certain cases.  Otherwise, it defers to the wrapped,
         * backing object.
         *
         * An @ref out_of_range exception should be thrown if the data for the time period is not available.
         *
         * @param output_name The name of the forcing property of interest.
         * @param init_time_epoch The epoch time (in seconds) of the start of the time period.
         * @param duration_seconds The length of the time period, in seconds.
         * @param output_units The expected units of the desired output value.
         * @return The value of the forcing property for the described time period, with units converted if needed.
         * @throws out_of_range If data for the time period is not available.
         */
        double get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override
        {
            string output_name = selector.get_variable_name();
            time_t init_time = selector.get_init_time();
            const long duration_s = selector.get_duration_secs();
            const string output_units = selector.get_output_units();
            
            // Balk if not in this instance's collection of outputs
            if (find(providedOutputs.begin(), providedOutputs.end(), output_name) == providedOutputs.end()) {
                throw runtime_error("Unknown output " + output_name + " requested from wrapped provider");
            }
            // Balk if this instance is not ready
            if (!isReadyToProvideData()) {
                throw runtime_error("Cannot get value for " + output_name
                                    + " from optional wrapped provider before it is ready");
            }

            // Remember: when the instance is ready to provide data, one or both of these **must** be true:
            //      * the desired output has a default value available
            //      * the desired output is provided by the backing wrapped provider

            // If this output is not available from a wrapped provider OR we should override, then return the default
            // Remember: isDefaultOverride() is not the same as "should use default" (see function docstring)
            if (!isSuppliedByWrappedProvider(output_name) || isDefaultOverride(output_name)) {
                // Take note if/how often default gets used in case that information is needed later
                recordUsingDefault(output_name);
                return defaultValues.at(output_name);
            }
            // Otherwise (i.e., available from wrapped provider and no override), get from backing wrapped provider
            else {
                return wrapped_provider->get_value(selector,m);
            }
        }

        /**
         * Test whether an available default should override an available backing provider output value.
         *
         * This method returns whether an default value is available and, despite there being a wrapped backing provider
         * that can provide the given output, that the default value should be used instead of the backing value.
         *
         * When constructed, this instance may have been provided with a number of default usage "waits" for one or more
         * outputs; i.e., a number of times it should wait to proxy a particular output value from the backing provider
         * and instead allow a default value to override the aforementioned backing value.  This function tests whether
         * the object is in a state in which such an override applies, with respect to the given output.
         *
         * The rules for transitioning through default usage "waits" are implemented in @ref recordUsingDefault.
         *
         * Note that the intent of the function is not the same as "should use default."  As such, the function will
         * return ``false`` in cases when there is not a backing provider or the backing provider cannot provide the
         * given output, even if there is a suitable default available.
         *
         * @param output_name The name of the output in question.
         * @return Whether an available default should override an available backing provider output value.
         * @see get_value
         * @see recordUsingDefault
         */
        bool isDefaultOverride(const string &output_name) {
            // First, there must be a default, and there must be something to override
            if (!isSuppliedWithDefault(output_name) || !isSuppliedByWrappedProvider(output_name)) {
                return false;
            }
            // Must have positive wait indicator value
            const auto &wait_it = defaultUsageWaits.find(output_name);
            return wait_it != defaultUsageWaits.end() && wait_it->second > 0;
        }

        /**
         * Get whether the instance is initialized such that it can handle requests to provide data.
         *
         * For this type, this is true if either the wrapped provider member has been set, or if there are default
         * values set up for all outputs that must be provided.
         *
         * @return Whether the instance is initialized such that it can handle requests to provide data.
        */
        bool isReadyToProvideData() override {
            // When set, all values are either backed or have defaults, so instance is ready
            if (isWrappedProviderSet()) {
                return true;
            }
            // Otherwise, if there is a default for everything, we are also good
            // Since the constructor validates there are no unexpected defaults, we can just compare sizes
            else {
                return providedOutputs.size() == defaultValues.size();
            }
        }

        /**
         * Test whether the instance was supplied with default values that can be returned for this output.
         *
         * @param outputName The output in question.
         * @return Whether a default value is available for this output.
         */
        inline bool isSuppliedWithDefault(const string &outputName) {
            return defaultValues.find(outputName) != defaultValues.end();
        }

        /**
         * Set the wrapped provider, if the given arg is valid for doing so.
         *
         * For a valid provider pointer argument, set the wrapped provider to it, clear the info message and return
         * ``true``.  For an invalid arg, set the info message member to provide details on why it is invalid.  Then
         * return ``false`` without setting the wrapped provider.
         *
         * There are several invalid argument cases:
         *
         * - a null pointer, except when all provided outputs have a default available
         * - a pointer to a provider that cannot provide all the ***required*** outputs this instance must proxy
         *     - only values that do not have a default available are considered required
         * - any arg if there has already been a valid provider set
         *
         * @param provider A pointer for the wrapped provider.
         * @return Whether @ref wrapped_provider was set to the given arg.
         */
        bool setWrappedProvider(
            GenericDataProvider *provider) override {
            // Disallow re-setting the provider if already validly set
            // TODO: consider if this is still appropriate, given backing providers can be valid without providing
            //  everything (because of defaults), and so a new provider may supply more or less of what is needed
            if (isWrappedProviderSet()) {
                setMessage = "Cannot change wrapped provider after a valid provide has already been set";
                return false;
            }

            // Confirm this will provide anything
            if (provider == nullptr) {
                setMessage = "Cannot set provider as null, as this cannot provide the values this type must proxy";
                return false;
            }

            // Check this provides everything needed, accounting for defaults (and also tallying the outputs provided)
            unsigned short providedByProviderCount = 0;
            for (const string &requiredName : providedOutputs) {
                // If supplied by the provider, increment our count and continue to the next required output name
                if (isSuppliedByProvider(requiredName, provider)) {
                    ++providedByProviderCount;
                    continue;
                }
                // If something isn't supplied by the provider (i.e., the "else"), there has to be a default, or we bail
                else if (!isSuppliedWithDefault(requiredName)) {
                    setMessage = "Given provider does not provide the required " + requiredName;
                    return false;
                }
            }

            // Also make sure it provides at least 1 relevant output (possible no outputs were "needed" due to defaults)
            if (providedByProviderCount == 0) {
                setMessage = "Given provider does not provide any required outputs (all satisfied via defaults)";
                return false;
            }

            // If this is good, set things and return true
            wrapped_provider = provider;
            setMessage.clear();
            return true;
        }

    protected:

        /**
         * Record that there was an instance of a default value being used and manage the default usage "waits."
         *
         * When constructed, this instance may have been provided with a number of default usage "waits" for one or more
         * outputs; i.e., a number of times it should wait to proxy a particular output value from the backing provider
         * and instead allow a default value to override the aforementioned backing value.  This function, which should
         * be called by @ref get_value any time a default is used, performs any modifications to the object's state that
         * are necessary when a default is used, in particular with respect to managing "waits."
         *
         * In this base implementation, a recorded default usage does not count against the required number of "waits"
         * for an output unless/until there is a wrapped backing provider set that can provide the given output.
         * Otherwise, positive wait counts for this output be reduced by 1.  This allows negative wait counts to be used
         * to represent that some output should always have the default used, even if the backing provider could supply
         * a value for it.
         *
         * @param output_name The name of the output for which a default was used.
         */
        virtual void recordUsingDefault(const string &output_name) {
            // Don't bother doing anything if there aren't waits assigned for this output
            // Also, in this implementation, don't count usages until there is a backing provider that can provide this
            auto waits_it = defaultUsageWaits.find(output_name);
            if (waits_it != defaultUsageWaits.end() && isSuppliedByWrappedProvider(output_name)) {
                // If this was not the last required default usage, simply decrement the wait count
                if (waits_it->second > 1) {
                    waits_it->second -= 1;
                }
                // If this was the last required default usage wait, clear this entry from the collection
                // Also, right now we are only supporting positive counts, so remove anything else as well
                else {
                    defaultUsageWaits.erase(waits_it);
                }
            }
        }

    private:

        /**
         * A collection of mapped default values for some or all of the provided outputs.
         */
        map<string, double> defaultValues;
        /**
         * The number of times a default value should still be used before beginning to proxy a backing provider value.
         *
         * Note than all elements should have values greater than zero.  Any elements that would have their value
         * reduced to zero should instead be removed.
         */
        map<string, int> defaultUsageWaits;

        static bool isSuppliedByProvider(const string &outputName, GenericDataProvider *provider) {
            const vector<string> &available = provider->get_avaliable_variable_names();
            return find(available.begin(), available.end(), outputName) != available.end();
        }

        inline bool isSuppliedByWrappedProvider(const string &outputName) {
            return wrapped_provider != nullptr && isSuppliedByProvider(outputName, wrapped_provider);
        }

    };

}

#endif //NGEN_OPTIONALWRAPPEDPROVIDER_HPP
