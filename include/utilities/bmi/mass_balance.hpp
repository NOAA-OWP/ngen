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

Version 0.1
Interface of the BMI mass balance protocol
*/
#pragma once

#include <string>
#include <exception>
#include "protocol.hpp"

namespace models{ namespace bmi{ namespace protocols{

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
        NgenMassBalance(std::shared_ptr<models::bmi::Bmi_Adapter> model=nullptr) :
          NgenBmiProtocol(model), check(false), is_fatal(false), 
          tolerance(1.0E-16), frequency(1){}

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
         * throws: MassBalanceError if the mass balance is not within the configured
         *         acceptable tolerance and the protocol is configured to be fatal.
         */
        void run(const Context& ctx) const override;

        virtual ~NgenMassBalance() override {};

      private:
        /**
         * @brief Check if the mass balance protocol is supported by the model
         * 
         * throws: MassBalanceIntegrationError if the mass balance protocol is not supported
         */
        void check_support() override;

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
         */
        void initialize(const geojson::PropertyMap& properties) override;

        // Configurable options/values
        bool check;
        bool is_fatal;
        double tolerance;
        // How often (in time steps) to check mass balance
        int frequency; 

        /**
         * @brief Friend class for checking/managing support and initialization
         * 
         * This allows the NgenBmiProtocols container class to access private members,
         * particularly the check_support() and initialize() methods.
         * 
         */
        friend class NgenBmiProtocols;

    };

    class MassBalanceIntegration : public std::exception {
        /**
         * @brief Exception thrown when there is an error in mass balance integration
         * 
         * This indicates that the BMI model isn't capable of supporting the mass balance protocol.
         */

    public:

        MassBalanceIntegration(char const *const message) noexcept : MassBalanceIntegration(std::string(message)) {}

        MassBalanceIntegration(std::string message) noexcept : std::exception(), what_message(std::move(message)) {}

        MassBalanceIntegration(MassBalanceIntegration &exception) noexcept : MassBalanceIntegration(exception.what_message) {}

        MassBalanceIntegration(MassBalanceIntegration &&exception) noexcept
        : MassBalanceIntegration(std::move(exception.what_message)) {}

        char const *what() const noexcept override {
            return what_message.c_str();
        }

    private:

        std::string what_message;
    };

    class MassBalanceError : public std::exception {
        /**
         * @brief Exception thrown when a mass balance error occurs
         * 
         * This indicates that a mass balance error has occurred within the model.
         */

    public:

        MassBalanceError(char const *const message) noexcept : MassBalanceError(std::string(message)) {}

        MassBalanceError(std::string message) noexcept : std::exception(), what_message(std::move(message)) {}

        MassBalanceError(MassBalanceError &exception) noexcept : MassBalanceError(exception.what_message) {}

        MassBalanceError(MassBalanceError &&exception) noexcept
        : MassBalanceError(std::move(exception.what_message)) {}

        char const *what() const noexcept override {
            return what_message.c_str();
        }

    private:

        std::string what_message;
    };

}}}

