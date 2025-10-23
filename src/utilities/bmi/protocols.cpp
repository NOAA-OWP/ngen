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

Version 0.2
Implement error handling via expected<T, ProtocolError> and error_or_warning

Version 0.1
Container and management for abstract BMI protocols
*/

#include "protocols.hpp"

namespace models{ namespace bmi{ namespace protocols{

auto operator<<(std::ostream& os, Protocol p) -> std::ostream& {
    switch(p) {
    case Protocol::MASS_BALANCE: os << "MASS_BALANCE"; break;
    default: os << "UNKNOWN_PROTOCOL"; break;
    }
    return os;
}

NgenBmiProtocols::NgenBmiProtocols()
    : model(nullptr) {
        protocols[Protocol::MASS_BALANCE] = std::make_unique<NgenMassBalance>();
}

NgenBmiProtocols::NgenBmiProtocols(ModelPtr model, const geojson::PropertyMap& properties)
    : model(model) {
    //Create and initialize mass balance configurable properties
    protocols[Protocol::MASS_BALANCE] = std::make_unique<NgenMassBalance>(model, properties);
}

auto NgenBmiProtocols::run(const Protocol& protocol_name, const Context& ctx) const -> expected<void, ProtocolError> {
    // Consider using find() vs switch, especially if the number of protocols grows
    expected<void, ProtocolError> result_or_err;
    switch(protocol_name){
        case Protocol::MASS_BALANCE:
            return protocols.at(Protocol::MASS_BALANCE)->run(model, ctx)
            .or_else( NgenBmiProtocol::error_or_warning );
            break;
        default:
            std::stringstream ss;
            ss << "Error: Request for unsupported protocol: '" << protocol_name << "'.";
            return NgenBmiProtocol::error_or_warning( ProtocolError(
                Error::UNSUPPORTED_PROTOCOL,
                ss.str()
            )
            );
    }
    return {};
}

}}} // end namespace models::bmi::protocols
