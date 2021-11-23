#ifndef NGEN_OPTIONALWRAPPEDPROVIDER_HPP
#define NGEN_OPTIONALWRAPPEDPROVIDER_HPP

#include "DeferredWrappedProvider.hpp"

using namespace std;

namespace forcing {

    /**
     * Extension of @ref DeferredWrappedProvider, where default values can be used when there is no backing provider.
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
                : DeferredWrappedProvider(providedOuts), defaultValues(defaultVals)
        {
            // Validate the provided map of defaults to make sure there aren't unrecognized keys
            if (!providedOutputs.empty() && !defaultValues.empty()) {
                for (auto def_vals_it = defaultValues.begin(); def_vals_it < defaultValues.end(); def_vals_it++) {
                    auto name_it = find(providedOutputs.begin(), providedOutputs.end(), def_vals_it->first);
                    if (name_it == providedOutputs.end()) {
                        string msg = "Invalid default values for OptionalWrappedProvider: default value given for "
                                     "unknown output with name '" + def_vals_it->first + "' (expected names are ["
                                     + providedOutputs[0];
                        for (int i = 1; i < providedOutputs.size(); ++i) {
                            msg += ", " + providedOutputs[i];
                        }
                        msg += "])";
                        thrown runtime_error(msg);
                    }
                }
            }
        }

        /**
         * Convenience constructor for instance when default values are not provided.
         *
         * @param providedOutputs The collection of the names of outputs this instance will need to provide.
         */
        explicit OptionalWrappedProvider(vector<string> providedOutputs)
            : OptionalWrappedProvider(providedOutputs, map<string, double>()) { }

        /**
         * Convenience constructor for when there is only one provided output name, which does not have a default.
         *
         * @param outputName The name of the single output this instance will need to provide.
         */
        explicit OptionalWrappedProvider(const string& outputName) : OptionalWrappedProvider(vector<string>(1)) {
            providedOutputs[0] = valueName;
        }

        /**
         * Convenience constructor for when there is only one provided output name, which has a default value.
         *
         * @param outputName The name of the single output this instance will need to provide.
         * @param defaultValue The default to associate with the given output.
         */
        OptionalWrappedProvider(const string& outputName, double defaultValue) : OptionalWrappedProvider(valueName) {
            defaultValues[providedOutputs[0]] = defaultValue;
        }

        /**
         * Get the value of a forcing property for an arbitrary time period, converting units if needed.
         *
         * This implementation can supply a default value in certain cases.  Otherwise, it defers to the wrapped,
         * backing object.
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
            // Balk if not in this instance's collection of outputs
            if (find(providedOutputs.begin(), providedOutputs.end(), output_name) == providedOutputs.end()) {
                throw runtime_error("Unknown output " + output_name + " requested from wrapped provider");
            }
            // Balk if this instance is not ready
            if (!isReadyToProvideData()) {
                throw runtime_error("Cannot get value for " + output_name
                                    + " from optional wrapped provider before it is ready");
            }
            // TODO: how do we handle forcing use of a default the first (or possibly more) time when there is a backing provider?
            // If there is a wrapped provider, try to find this output there and make a nested call
            if (isWrappedProviderSet()) {
                const vector<string> &backed_outputs = wrapped_provider->get_available_forcing_outputs();
                if (find(backed_outputs.begin(), backed_outputs.end(), output_name) != backed_outputs.end()) {
                    return wrapped_provider->get_value(output_name, init_time, duration_s, output_units);
                }
            }
            // Given the above checks and implied conditions, if we get here then there must be a default
            return defaultValues.at(output_name);
        }

        /**
         * Move constructor.
         *
         * @param provider_to_move
         */
        OptionalWrappedProvider(OptionalWrappedProvider &&provider_to_move) noexcept
        : OptionalWrappedProvider(provider_to_move.providedOutputs)
        {
            // TODO: come back to this for any other members added here
            defaultValues = move(provider_to_move.defaultValues);
            wrapped_provider = provider_to_move.wrapped_provider;
            provider_to_move.wrapped_provider = nullptr;
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

    private:

        /**
         * A collection of mapped default values for some or all of the provided outputs.
         */
        map<string, double> defaultValues;

        static bool isSuppliedByProvider(const string &valName, ForcingProvider *provider) {
            auto available = provider.get_available_forcing_outputs();
            return std::find(available.begin(), available.end(), valName) == available.end();
        }

        inline bool isSuppliedByWrappedProvider(const string &valName) {
            return wrapped_provider != nullptr && isSuppliedByProvider(valName, wrapped_provider);
        }

        inline bool isSuppliedWithDefault(const string &valName) {
            return defaultValues.find(valName) == defaultValues.end();
        }

    };

}

#endif //NGEN_OPTIONALWRAPPEDPROVIDER_HPP
