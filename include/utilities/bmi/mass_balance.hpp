/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
------------------------------------------------------------------------
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
------------------------------------------------------------------------
Version 0.3
Implement is_supported()
Re-align members for more better memory layout/padding
Update docstrings

Version 0.2
Conform to updated protocol interface
Removed integration and error exceptions in favor of ProtocolError

Version 0.1
Interface of the BMI mass balance protocol
*/
#pragma once


#include <string>
#include <exception>
#include <protocol.hpp>
#include <nonstd/expected.hpp>

namespace models{ namespace bmi{ namespace protocols{
    using nonstd::expected;
    /** Mass balance variable names **/
    constexpr const char* const INPUT_MASS_NAME = "ngen::mass_in";
    constexpr const char* const OUTPUT_MASS_NAME = "ngen::mass_out";
    constexpr const char* const STORED_MASS_NAME = "ngen::mass_stored";
    constexpr const char* const LEAKED_MASS_NAME = "ngen::mass_leaked";

    /** Configuration keys for defining configurable properties of the protocol */
    //The top level object key which will contain the map of configuration options
    constexpr const char* const CONFIGURATION_KEY = "mass_balance";
    //Configuration option keys
    constexpr const char* const TOLERANCE_KEY = "tolerance";
    constexpr const char* const FATAL_KEY = "fatal";
    constexpr const char* const CHECK_KEY = "check";
    constexpr const char* const FREQUENCY_KEY = "frequency";
    
    class NgenMassBalance : public NgenBmiProtocol {
        /** @brief Mass Balance protocol
         * 
         * This protocol `run()`s a simple mass balance calculation by querying the model for a
         * set of mass balance state variables and computing the basic mass balance as
         * balance = mass_in - mass_out - mass_stored - mass_leaked.  It is then checked against
         * a tolerance value to determine if the mass balance is acceptable.
         */
      public:

        /** @brief Constructor for the NgenMassBalance protocol
         * 
         * This constructor initializes the mass balance protocol with the given model and properties.
         * 
         * @param model A shared pointer to a Bmi_Adapter object which should be
         * initialized before being passed to this constructor.
         */
        NgenMassBalance(const ModelPtr& model, const Properties& properties);

        /**
         * @brief Construct a new, default Ngen Mass Balance object
         * 
         * By default, the protocol is considered unsupported and won't be checked
         */
        NgenMassBalance();

        virtual ~NgenMassBalance() override;

      private:

        /**
         * @brief Run the mass balance protocol
         * 
         * If the configured frequency is -1, the mass balance will only be checked at the end
         * of the simulation. If the frequency is greater than 0, the mass balance will be checked
         * at the specified frequency based on the current_time_step and the total_steps provided
         * in the Context.
         * 
         * Warns or errors at each check if total mass balance is not within the configured
         * acceptable tolerance.
         * 
         * @return expected<void, ProtocolError> May contain a ProtocolError if
         *         the protocol fails for any reason.  Errors of ProtocolError::PROTOCOL_WARNING
         *         severity should be logged as warnings, but not cause the simulation to fail.
         */
        auto run(const ModelPtr& model, const Context& ctx) const -> expected<void, ProtocolError> override;

        /**
         * @brief Check if the mass balance protocol is supported by the model
         * 
         * @return expected<void, ProtocolError> May contain a ProtocolError if
         *         the protocol is not supported by the model.
         */
        [[nodiscard]] auto check_support(const ModelPtr& model) -> expected<void, ProtocolError> override;

        /**
         * @brief Check the model for support and initialize the mass balance protocol from the given properties.
         * 
         * If the model does not support the mass balance protocol, an exception will be thrown, and no mass balance
         * will performed when `run()` is called.
         * 
         * A private initialize call is used since it only makes sense to check/run the protocol
         * once the model adapter is fully constructed. This should be called by the owner of the
         * NgenMassBalance instance once the model is ready.
         * 
         * @param properties Configurable key/value properties for the mass balance protocol.
         *                   If the map contains "mass_balance" object, then the following properties
         *                   are used to configure the protocol:
         *                      tolerance: double, default 1.0E-16.
         *                      check: bool, default true. Whether to perform mass balance check.
         *                      frequency: int, default 1. How often (in time steps) to check mass balance.
         *                      fatal: bool, default false. Whether to treat mass balance errors as fatal.
         *                   Otherwise, mass balance checking will be disabled (check will be false)
         * 
         * @return expected<void, ProtocolError> May contain a ProtocolError if
         *         initialization fails for any reason, since the protocol must
         *         be effectively "optional", failed initialization results in
         *         the protocol being disabled for the duration of the simulation.
         */
        auto initialize(const ModelPtr& model, const Properties& properties) -> expected<void, ProtocolError> override;

        /**
         * @brief Whether the protocol is supported by the model
         * 
         * @return true  the model exposes the required mass balance variables
         * @return false the model does not support mass balance checking via this protocol
         */
        bool is_supported() const override final;

    private:
        double tolerance;
        // How often (in time steps) to check mass balance
        int frequency;
        // Whether the protocol is supported by the model, false by default
        bool supported = false;
        // Configurable options/values
        bool check;
        bool is_fatal;
    };

}}}

