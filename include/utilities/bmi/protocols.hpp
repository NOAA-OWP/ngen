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
Version 0.2.1
Fix NgenBmiProtocols constructor to initialize protocols map when model is null
Fix run() to return the expected<void, ProtocolError> returned by the wrapped call

Version 0.2
Enumerate protocol types/names
The container now holds a single model pointer and passes it to each protocol
per the updated (v0.2) protocol interface
Keep protocols in a map for dynamic access by enumeration name
add operator<< for Protocol enum

Version 0.1
Container and management for abstract BMI protocols
*/
#pragma once

#include <string>
#include <vector>
#include <boost/type_index.hpp>
#include <unordered_map>
#include "Bmi_Adapter.hpp"
#include "JSONProperty.hpp"

#include "mass_balance.hpp"


namespace models{ namespace bmi{ namespace protocols{

enum class Protocol {
    MASS_BALANCE
};

auto operator<<(std::ostream& os, Protocol p) -> std::ostream&;

class NgenBmiProtocols {
    /**
     * @brief Container and management interface for BMI protocols for use in ngen
     * 
     */

    public:
        /**
         * @brief Construct a new Ngen Bmi Protocols object with a null model
         * 
         */
        NgenBmiProtocols();

        /**
         * @brief Construct a new Ngen Bmi Protocols object for use with a known model
         * 
         * @param model An initialized BMI model
         * @param properties Properties for each protocol being initialized
         */
        NgenBmiProtocols(std::shared_ptr<models::bmi::Bmi_Adapter> model, const geojson::PropertyMap& properties);
        
        /**
         * @brief Run a specific BMI protocol by name with a given context
         * 
         * @param protocol_name The name of the protocol to run
         * @param ctx The context of the current protocol run
         * 
         * @return expected<void, ProtocolError> May contain a ProtocolError if
         *         the protocol fails for any reason.
         */
        auto run(const Protocol& protocol_name, const Context& ctx) const -> expected<void, ProtocolError>; 

        private:

        /**
         * @brief All protocols managed by this container will utilize the same model
         * 
         * This reduces the amount of pointer copying and references across a large simulation
         * and it ensures that all protocols see the same model state.
         * 
         */
        std::shared_ptr<models::bmi::Bmi_Adapter> model;
        /**
         * @brief Map of protocol name to protocol instance
         * 
         */
        std::unordered_map<Protocol, std::unique_ptr<NgenBmiProtocol>> protocols;
    };

}}}
