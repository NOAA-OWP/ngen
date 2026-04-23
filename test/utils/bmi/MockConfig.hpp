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

/**
 * @brief Config builder for the save-side serialization protocol.
 *
 * Produces a `serialization` block with a shared top-level `path` and a
 * `save` sub-block.
 */
class SerializationMock {
    public:

        SerializationMock(const std::string& path, bool fatal = true, int frequency = 1, bool check = true)
            : properties() {
            boost::property_tree::ptree top;
            top.put("path", path);
            boost::property_tree::ptree save;
            save.put("check", check);
            save.put("fatal", fatal);
            save.put("frequency", frequency);
            top.add_child("save", save);
            properties.add_child("serialization", top);
        }

        // Variant producing a config with no "path" at the top-level — lets
        // tests exercise the "path required but missing" branch.
        static SerializationMock without_path(bool fatal = true, int frequency = 1, bool check = true) {
            SerializationMock m{};
            boost::property_tree::ptree top;
            boost::property_tree::ptree save;
            save.put("check", check);
            save.put("fatal", fatal);
            save.put("frequency", frequency);
            top.add_child("save", save);
            m.properties.add_child("serialization", top);
            return m;
        }

        const boost::property_tree::ptree& get() const {
            return properties.get_child("serialization");
        }

        const geojson::PropertyMap as_json_property() const {
            auto props = geojson::JSONProperty("serialization", properties);
            return props.get_values();
        }

    private:
        SerializationMock() = default;
        boost::property_tree::ptree properties;
};

/**
 * @brief Config builder for the restore-side deserialization protocol.
 *
 * Produces a `serialization` block with a shared top-level `path` and a
 * `restore` sub-block. Save config (for integration tests that drive both
 * sides from one config) can be grafted on with `with_save()`.
 */
class DeserializationMock {
    public:
        DeserializationMock(const std::string& path,
                            const std::string& step = "latest",
                            bool fatal = true,
                            bool check = true)
            : properties() {
            boost::property_tree::ptree top;
            top.put("path", path);
            boost::property_tree::ptree restore;
            restore.put("check", check);
            restore.put("fatal", fatal);
            restore.put("step", step);
            top.add_child("restore", restore);
            properties.add_child("serialization", top);
        }

        // Build a restore config keyed by timestamp instead of step.
        static DeserializationMock by_timestamp(const std::string& path,
                                                const std::string& timestamp,
                                                bool fatal = true,
                                                bool check = true) {
            DeserializationMock m{};
            boost::property_tree::ptree top;
            top.put("path", path);
            boost::property_tree::ptree restore;
            restore.put("check", check);
            restore.put("fatal", fatal);
            restore.put("timestamp", timestamp);
            top.add_child("restore", restore);
            m.properties.add_child("serialization", top);
            return m;
        }

        // Build a restore config that specifies BOTH step and timestamp,
        // so tests can exercise the precedence rule (timestamp wins).
        static DeserializationMock both(const std::string& path,
                                        const std::string& step,
                                        const std::string& timestamp,
                                        bool fatal = true,
                                        bool check = true) {
            DeserializationMock m{};
            boost::property_tree::ptree top;
            top.put("path", path);
            boost::property_tree::ptree restore;
            restore.put("check", check);
            restore.put("fatal", fatal);
            restore.put("step", step);
            restore.put("timestamp", timestamp);
            top.add_child("restore", restore);
            m.properties.add_child("serialization", top);
            return m;
        }

        DeserializationMock& with_save(int frequency = 1, bool check = true, bool fatal = true) {
            boost::property_tree::ptree save;
            save.put("check", check);
            save.put("fatal", fatal);
            save.put("frequency", frequency);
            properties.get_child("serialization").add_child("save", save);
            return *this;
        }

        const boost::property_tree::ptree& get() const {
            return properties.get_child("serialization");
        }

        const geojson::PropertyMap as_json_property() const {
            auto props = geojson::JSONProperty("serialization", properties);
            return props.get_values();
        }

    private:
        DeserializationMock() = default;
        boost::property_tree::ptree properties;
};