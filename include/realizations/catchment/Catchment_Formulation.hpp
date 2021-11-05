#ifndef CATCHMENT_FORMULATION_H
#define CATCHMENT_FORMULATION_H

#include <memory>
#include <vector>
#include "Formulation.hpp"
#include "Et_Accountable.hpp"
#include <HY_CatchmentArea.hpp>

#include <Forcing.h> // Remove when _link_legacy_forcing() is removed!

namespace realization {

    class Catchment_Formulation : public Formulation, public HY_CatchmentArea, public Et_Accountable {
        public:
            Catchment_Formulation(std::string id, std::unique_ptr<forcing::ForcingProvider> forcing, utils::StreamHandler output_stream)
                : Formulation(id), HY_CatchmentArea(std::move(forcing), output_stream) { };

            Catchment_Formulation(std::string id) : Formulation(id){};

        /**
         * Perform in-place substitution on the given config property item, if the item and the pattern are present.
         *
         * Any and all instances of the substring ``pattern`` are replaced by ``replacement``, if ``key`` maps to a
         * present string-type property value.
         *
         * @param properties A reference to the properties config object to be altered.
         * @param key The key for the configuration property to potentially adjust.
         * @param pattern The pattern substring to search for that, when present, should be replaced.
         * @param replacement The replacement substring to potentially insert.
         */
        static void config_pattern_substitution(geojson::PropertyMap &properties, const std::string &key,
                                                const std::string &pattern, const std::string &replacement) {
            auto it = properties.find(key);
            // Do nothing and return if either the key isn't found or the associated property isn't a string
            if (it == properties.end() || it->second.get_type() != geojson::PropertyType::String) {
                return;
            }

            std::string value = it->second.as_string();
            size_t id_index = value.find(pattern);

            if (id_index != std::string::npos) {
                do {
                    value = value.replace(id_index, sizeof(pattern.c_str()) - 2, replacement);
                    id_index = value.find(pattern);
                } while (id_index != std::string::npos);

                properties.erase(key);
                properties.emplace(key, geojson::JSONProperty(key, value));
            }
        }

            /**
             * Get a header line appropriate for a file made up of entries from this type's implementation of
             * ``get_output_line_for_timestep``.
             *
             * Note that like the output generating function, this line does not include anything for time step.
             *
             * A default implementation is provided for inheritors of this type, which includes only "Total Discharge."
             *
             * @return An appropriate header line for this type.
             */
            std::string get_output_header_line(std::string delimiter) override {
                return "Total Discharge";
            }

            /**
             * Execute the backing model formulation for the given time step, where it is of the specified size, and
             * return the response output.
             *
             * Any inputs and additional parameters must be made available as instance members.
             *
             * Types should clearly document the details of their particular response output.
             *
             * @param t_index The index of the time step for which to run model calculations.
             * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
             * @return The response output of the model for this time step.
             */
            double get_response(time_step_t t_index, time_step_t t_delta) override = 0;

            const std::vector<std::string>& get_required_parameters() override = 0;

            void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override = 0;
            void create_formulation(geojson::PropertyMap properties) override = 0;
            virtual ~Catchment_Formulation(){};

    protected:
        std::string get_catchment_id() override {
            return id;
        }

        void set_catchment_id(std::string cat_id) override {
            id = cat_id;
        }

        //TODO: VERY BAD JUJU...the following two members are an ugly hack to avoid having to gut the legacy C/C++ realizations for now.
        Forcing legacy_forcing;
        // Use this a a deprecation chokepoint to get rid of Forcing when ready.
        void _link_legacy_forcing()
        {
            void* f { this->forcing.get() };
            legacy_forcing = *((Forcing *)f);
        }
    };
}
#endif // CATCHMENT_FORMULATION_H
