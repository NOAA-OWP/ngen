#ifndef NGEN_DEFERREDWRAPPEDPROVIDER_HPP
#define NGEN_DEFERREDWRAPPEDPROVIDER_HPP

#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include "WrappedDataProvider.hpp"

namespace data_access {

    /**
     * A specialized @WrappedDataProvider that is created without first knowing the backing source it wraps.
     *
     * This type wraps another @ref GenericDataProvider, similarly to its parent.  This is "optimistic," however, in
     * that it is constructed without the backing data source it will wrap.  It only requires the data output names it
     * expects to eventually be able to provide.
     *
     * This type allows for deferring reconciling of whether a provider for some data output is available.  This is
     * useful for situations when a required provider is not currently known, but is expected, and there will be ample
     * time to validate the expectation and make the necessary associations before the data must be provided.
     *
     * The runtime behavior of inherited functions depends on the type of the wrapped object.  I.e., nested calls to the
     * analogous function of the wrapped provider object are made, with those results returned. The one exception to
     * this is @ref get_available_forcing_outputs.  A particular effect of this is that the type of the wrapped provider
     * object will dictate how an instance behaves in cases when an unrecognized output name is supplied as a parameter
     * to @ref get_value.
     *
     * As noted above, the @ref get_available_forcing_outputs virtual function is overridden here.  Instead of directly
     * returning results from a nested call to the wrapped provider, this type's implementation ensures only the outputs
     * set as provideable in this instance (i.e., the outer wrapper) are returned.
     */
    class DeferredWrappedProvider : public WrappedDataProvider 
    {
    public:

        /**
         * Constructor for instance.
         *
         * @param providedOutputs The collection of the names of outputs this instance will need to provide.
         */
        explicit DeferredWrappedProvider(std::vector<std::string> providedOutputs) : WrappedDataProvider(nullptr), providedOutputs(std::move(providedOutputs)) { }

        /**
         * Convenience constructor for when there is only one provided output name.
         *
         * @param outputName The name of the single output this instance will need to provide.
         */
        explicit DeferredWrappedProvider(const std::string& outputName) : DeferredWrappedProvider(std::vector<std::string>(1)) {
            providedOutputs[0] = outputName;
        }

        /**
         * Move constructor.
         *
         * @param provider_to_move
         */
        DeferredWrappedProvider(DeferredWrappedProvider &&provider_to_move) noexcept
            : DeferredWrappedProvider(provider_to_move.providedOutputs)
        {
            wrapped_provider = provider_to_move.wrapped_provider;
            provider_to_move.wrapped_provider = nullptr;
        }

        /**
         * Get the names of the outputs that this instance is (or will be) able to provide.
         *
         * Note that this function behaves the same regardless of whether the wrapped provider has been set.
         *
         * @return The names of the outputs for which this instance is (or will be) able to provide values.
         */

        boost::span<const std::string> get_available_variable_names() const override {
            return providedOutputs;
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
        inline const std::string &getSetMessage() {
            return setMessage;
        }

        /**
         * Get whether the instance is initialized such that it can handle requests to provide data.
         *
         * For this type, this is equivalent to whether the wrapped provider member has been set.
         *
         * Note that readiness is subject only to the result of @ref isWrappedProviderSet, which for this type (and
         * generally) does not reflect the readiness state of the inner, wrapped provider. This effectively assumes it
         * is ready prior to or immediately upon being set.
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
         * - an arg that points to a provider that does not itself provide all the required outputs this must provide
         * - any arg if there has already been a valid provider set
         *
         * @param provider A pointer for the wrapped provider.
         * @return Whether @ref wrapped_provider was set to the given arg.
         */
        virtual bool setWrappedProvider(GenericDataProvider *provider) {
            // Disallow re-setting the provider if already validly set
            if (isWrappedProviderSet()) {
                setMessage = "Cannot change wrapped provider after a valid provider has already been set";
                return false;
            }

            // Confirm this will provide anything
            if (provider == nullptr) {
                setMessage = "Cannot set provider as null, as this cannot provide the outputs this type must proxy";
                return false;
            }

            // Confirm this will provide everything needed
            const auto available = provider->get_available_variable_names();
            for (const std::string &requiredName : providedOutputs) {
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
        /** The collection of names of the outputs this type can/will be able to provide from its wrapped source. */
        std::vector<std::string> providedOutputs;

        /**
         * A message providing information from the last @ref setWrappedProvider call.
         *
         * The value will be empty if the function returned ``true``.  If it returned ``false``, this will have an
         * info message set explaining more detail on the failure.
         */
        std::string setMessage;
    };

}

#endif //NGEN_DEFERREDWRAPPEDPROVIDER_HPP
