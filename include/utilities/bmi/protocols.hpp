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
#pragma once

#include <string>
#include <vector>
#include <boost/type_index.hpp>
#include "Bmi_Adapter.hpp"
#include "JSONProperty.hpp"

#include "mass_balance.hpp"


namespace models{ namespace bmi{ namespace protocols{

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
         * @brief Mass Balance Checker
         * 
         */
        NgenMassBalance mass_balance;
};

}}}
