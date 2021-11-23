#ifndef NGEN_DEFERREDWRAPPEDPROVIDER_HPP
#define NGEN_DEFERREDWRAPPEDPROVIDER_HPP

#include <string>
#include <utility>
#include <vector>
#include "WrappedForcingProvider.hpp"

using namespace std;

namespace forcing {

    /**
     * A specialized @WrappedForcingProvider that is created without first knowing the backing source it wraps.
     *
     * This type wraps another @ref ForcingProvider, similarly to its parent.  This is "optimistic," however, in that it
     * is constructed without the backing data source it will wrap.  It only requires the data value names it expects to
     * eventually be able to provide.
     *
     * This type allows for deferring reconciling of whether a provider for some data property is available.  This is
     * useful for situations when a required provider is not current known, but is expected, and there will be ample
     * time to validate the expectation and make the necessary associations before the data must be provided.
     *
     * Note that this type does not alter the behavior of inherited functions, except for
     * @ref get_available_forcing_outputs.  It will only list outputs guaranteed to be provideable by it, but it still
     * defers to the wrapped instance completely for other functions.  This means the wrapped type will control behavior
     * of cases when some unknown value name is given.
     */
    class DeferredWrappedProvider : public WrappedForcingProvider {
    public:

        /**
         * Constructor for instance.
         *
         * @param providedValues The collection of the names of values this instance will need to provide.
         */
        explicit DeferredWrappedProvider(vector<string> providedValues) : WrappedForcingProvider(nullptr), providedValues(std::move(providedValues)) { }

        /**
         * Convenience constructor for when there is only one provided property name.
         *
         * @param valueName The name of the single value this instance will need to provide.
         */
        explicit DeferredWrappedProvider(const string& valueName) : DeferredWrappedProvider(vector<string>(1)) {
            providedValues[0] = valueName;
        }

        /**
         * Move constructor.
         *
         * @param provider_to_move
         */
        DeferredWrappedProvider(DeferredWrappedProvider &&provider_to_move) noexcept
            : DeferredWrappedProvider(provider_to_move.providedValues)
        {
            wrapped_provider = provider_to_move.wrapped_provider;
            provider_to_move.wrapped_provider = nullptr;
        }

        /**
         * Get the names of the outputs for which this instance is (or will be) able to provide values.
         *
         * Note that this function behaves the same regardless of whether the wrapped provider has been set.
         *
         * @return The names of the outputs for which this instance is (or will be) able to provide values.
         */
        const std::vector<std::string> &get_available_forcing_outputs() override {
            return providedValues;
        }

        /**
         * Get the message string for the last call to @ref setWrappedProvider.
         *
         * The value of the backing member for this function will be empty if the last call to @ref setWrappedProvider
         * returned ``true`` (or has not been called yet).  If @ref setWrappedProvider it returned ``false``, then an
         * info message will be saved in the member returned by this function, explaining more detail on the failure.
         *
         * @return The message string for the last call to @ref setWrappedProvider.
         */
        inline const string &getSetMessage() {
            return setMessage;
        }

        /**
         * Get whether the instance is initialized such that it can handle requests to provide data.
         *
         * For this type, this is equivalent to whether the wrapped provider member has been set.
         *
         * @return Whether the instance is initialized such that it can handle requests to provide data.
         */
        virtual bool isReadyToProvideData() {
            return isWrappedProviderSet();
        }

        /**
      * Get whether the backing provider this instance wraps has been set yet.
      *
      * @return Whether the backing provider this instance wraps has been set yet.
      */
        inline bool isWrappedProviderSet() {
            return wrapped_provider != nullptr;
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
         * - an arg that is actually a null pointer
         * - an arg that points to a provider that does not itself provide all the required values this must provide
         * - any arg if there has already been a valid provider set
         *
         * @param provider A pointer for the wrapped provider.
         * @return Whether @ref wrapped_provider was set to the given arg.
         */
        virtual bool setWrappedProvider(ForcingProvider *provider) {
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

            // Confirm this will provide everything needed
            const vector<string> &available = provider->get_available_forcing_outputs();
            for (const string &requiredName : providedValues) {
                if (std::find(available.begin(), available.end(), requiredName) == available.end()) {
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
        /** The collection of names of the values this type can/will be able to provide from its wrapped source. */
        vector<string> providedValues;

        /**
         * A message providing information from the last @ref setWrappedProvider call.
         *
         * The value will be empty if the function returned ``true``.  If it returned ``false``, this will have an
         * info message set explaining more detail on the failure.
         */
        string setMessage;
    };

}

#endif //NGEN_DEFERREDWRAPPEDPROVIDER_HPP
