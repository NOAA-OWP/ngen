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
Container and management for abstract BMI protocols
*/

#include "protocols.hpp"

namespace models{ namespace bmi{ namespace protocols{

NgenBmiProtocols::NgenBmiProtocols(): mass_balance(nullptr) {}

NgenBmiProtocols::NgenBmiProtocols(std::shared_ptr<models::bmi::Bmi_Adapter> model, const geojson::PropertyMap& properties)
    : mass_balance(model) {
    //initialize mass balance configurable properties
    //This is done so that initialize is called with a valid model pointer, which is shared
    //by all protocol instances
    mass_balance.initialize(properties);
}

}}} // end namespace models::bmi::protocols
