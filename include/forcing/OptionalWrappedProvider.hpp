#ifndef NGEN_OPTIONALWRAPPEDPROVIDER_HPP
#define NGEN_OPTIONALWRAPPEDPROVIDER_HPP

#include <utility>

#include "DeferredWrappedProvider.hpp"

using namespace std;

namespace forcing {

    /**
     * Extension of @ref DeferredWrappedProvider, where default values can be used instead of a backing provider.
     *
     * This type allows for default values for provided outputs to be supplied during construction.  These can then be
     * provided instead of values proxied from a backing provider.  Instances can, therefore, either be ready to provide
     * even before/without a backing provider is set, or accept as valid a backing provider that does not supply all of
     * its required outputs.
     *
     * Additionally, this type can be initialized to override a backing provider in some situations.  A collection of
     * "wait" count values can be given at construction, with these mapped to provided outputs for which defaults are
     * also supplied. These "wait" counts represent the number of times the instance should wait to proxy the
     * corresponding output's value from a backing provider.  In such cases, the available default value is used
     * instead.  It is also possible to set this "wait" count to a negative number to indicate that a default should
     * always be used for a certain output, even if a backing provider could provide it.
     */
    class OptionalWrappedProvider : public DeferredWrappedProvider {

    public:
        /**
         * Constructor for instance.
         *
         * Also validates that given defaults only contain keys that are in given provided outputs.
         *
         * @param providedOuts The collection of the names of outputs this instance will need to provide.
         * @param defaultVals Mapping of some or all provided output defaults, keyed by output name.
         */
        OptionalWrappedProvider(vector<string> providedOuts, map<string, double> defaultVals)
                : DeferredWrappedProvider(move(providedOuts)), defaultValues(move(defaultVals))
        {
            // Validate the provided map of defaults to make sure there aren't unrecognized keys
            if (!providedOutputs.empty() && !defaultValues.empty()) {
                for (const auto &def_vals_it : defaultValues) {
                    auto name_it = find(providedOutputs.begin(), providedOutputs.end(), def_vals_it.first);
                    if (name_it == providedOutputs.end()) {
                        string msg = "Invalid default values for OptionalWrappedProvider: default value given for "
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
        OptionalWrappedProvider(vector<string> providedOuts, map<string, double> defaultVals,
                                map<string, int> defaultWaits)
                : OptionalWrappedProvider(move(providedOuts), move(defaultVals))
        {
            if (!defaultWaits.empty()) {
                // Make sure all the keys/names in the mapping of waits are recognized
                for (const auto &wait_it : defaultWaits) {
                    auto def_it = defaultValues.find(wait_it.first);
                    if (def_it == defaultValues.end()) {
                        string msg = "Invalid default usage waits for OptionalWrappedProvider: wait count given for "
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
                defaultUsageWaits = move(defaultWaits);
            }
        }

        /**
         * Convenience constructor for instance when default values are not provided.
         *
         * @param providedOutputs The collection of the names of outputs this instance will need to provide.
         */
        explicit OptionalWrappedProvider(vector<string> providedOutputs)
            : OptionalWrappedProvider(move(providedOutputs), map<string, double>()) { }

        /**
         * Convenience constructor for when there is only one provided output name, which does not have a default.
         *
         * @param outputName The name of the single output this instance will need to provide.
         */
        explicit OptionalWrappedProvider(const string& outputName) : OptionalWrappedProvider(vector<string>(1)) {
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
        OptionalWrappedProvider(const string& outputName, double defaultValue, int defaultUsageWait)
            : OptionalWrappedProvider(outputName)
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
        OptionalWrappedProvider(const string& outputName, double defaultValue)
            : OptionalWrappedProvider(outputName, defaultValue, 0) { }

        /**
         * Move constructor.
         *
         * @param provider_to_move
         */
        OptionalWrappedProvider(OptionalWrappedProvider &&provider_to_move) noexcept
                : OptionalWrappedProvider(provider_to_move.providedOutputs)
        {
            defaultValues = move(provider_to_move.defaultValues);
            defaultUsageWaits = move(provider_to_move.defaultUsageWaits);
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
        double get_value(const string &output_name, const time_t &init_time, const long &duration_s,
                         const string &output_units) override
        {
            // Balk if not in this instance's collection of outputs
            if (find(providedOutputs.begin(), providedOutputs.end(), output_name) == providedOutputs.end()) {
                throw runtime_error("Unknown output " + output_name + " requested from wrapped provider");
            }
            // Balk if this instance is not ready
            if (!isReadyToProvideData()) {
                throw runtime_error("Cannot get value for " + output_name
                                    + " from optional wrapped provider before it is ready");
            }
            // If there is a backing provider and default should not override, try to get output from backing provider
            if (isWrappedProviderSet() && !isDefaultOverride(output_name)) {
                const vector<string> &backed_outputs = wrapped_provider->get_available_forcing_outputs();
                if (find(backed_outputs.begin(), backed_outputs.end(), output_name) != backed_outputs.end()) {
                    return wrapped_provider->get_value(output_name, init_time, duration_s, output_units);
                }
            }
            // Take note if/how often default gets used in case that information is needed later
            recordUsingDefault(output_name);
            return defaultValues.at(output_name);
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
            return isSuppliedWithDefault(output_name)                                  // Must be a default
                   && isSuppliedByWrappedProvider(output_name)                         // Must be something to override
                   && defaultUsageWaits.find(output_name) != defaultUsageWaits.end();  // Must have a wait indicator
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
        bool setWrappedProvider(ForcingProvider *provider) override {
            // Disallow re-setting the provider if already validly set
            if (isWrappedProviderSet()) {
                setMessage = "Cannot change wrapped provider after a valid provide has already been set";
                return false;
            }

            // Confirm this will provide anything
            if (provider == nullptr) {
                setMessage = "Cannot set provider as null, as this cannot provide the values this type must proxy";
                return false;
            }

            // Confirm this will provide everything needed, though allow for defaults when available
            for (const string &requiredName : providedOutputs) {
                // When not provided by this provider and there is not a default value set up ...
                if (!isSuppliedByProvider(requiredName, provider) && !isSuppliedWithDefault(requiredName)) {
                    setMessage = "Given provider does not provide the required " + requiredName;
                    return false;
                }
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
                // A value of 0 should be inferred and cleared from the collection, so ...
                if (waits_it->second == 1) {
                    defaultUsageWaits.erase(waits_it);
                }
                else if (waits_it->second > 1) {
                    waits_it->second -= 1;
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

        static bool isSuppliedByProvider(const string &outputName, ForcingProvider *provider) {
            const vector<string> &available = provider->get_available_forcing_outputs();
            return find(available.begin(), available.end(), outputName) != available.end();
        }

        inline bool isSuppliedByWrappedProvider(const string &outputName) {
            return wrapped_provider != nullptr && isSuppliedByProvider(outputName, wrapped_provider);
        }

        inline bool isSuppliedWithDefault(const string &outputName) {
            return defaultValues.find(outputName) != defaultValues.end();
        }

    };

}

#endif //NGEN_OPTIONALWRAPPEDPROVIDER_HPP
