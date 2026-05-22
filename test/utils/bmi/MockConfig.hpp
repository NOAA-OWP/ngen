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
*/
#pragma once
#include <boost/property_tree/ptree.hpp>
#include "protocols.hpp"
#include "JSONProperty.hpp"

static const auto noneConfig = std::map<std::string, geojson::JSONProperty> {
        {"none", geojson::JSONProperty("none", true)}
};

static models::bmi::protocols::Context make_context(int current_time_step, int total_steps, const std::string& timestamp, const std::string& id) {
    return models::bmi::protocols::Context{
        current_time_step,
        total_steps,
        timestamp,
        id
    };
}

class MassBalanceMock {
    public:

        MassBalanceMock( bool fatal = false, double tolerance = 1e-12, int frequency = 1, bool check = true)
            : properties() {
            boost::property_tree::ptree config;
            config.put("check", check);
            config.put("tolerance", tolerance);
            config.put("fatal", fatal);
            config.put("frequency", frequency);
            properties.add_child("mass_balance", config);
        }

         MassBalanceMock( bool fatal, const char* tolerance){
            boost::property_tree::ptree config;
            config.put("check", true);
            config.put("tolerance", tolerance);
            config.put("fatal", fatal);
            config.put("frequency", 1);
            properties.add_child("mass_balance", config);
         }

        const boost::property_tree::ptree& get() const {
            return properties.get_child("mass_balance");
        }

        const geojson::PropertyMap as_json_property() const {
            auto props = geojson::JSONProperty("mass_balance", properties);
            // geojson::JSONProperty::print_property(props, 1);
            return props.get_values();
        }

    private:
        boost::property_tree::ptree properties;
};