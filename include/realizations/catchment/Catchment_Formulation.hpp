#ifndef CATCHMENT_FORMULATION_H
#define CATCHMENT_FORMULATION_H

#include <memory>
#include <vector>
#include "Formulation.hpp"
#include <HY_CatchmentArea.hpp>
#include "GenericDataProvider.hpp"

#define DEFAULT_FORMULATION_OUTPUT_DELIMITER ","

namespace realization {

    class Catchment_Formulation : public Formulation, public HY_CatchmentArea {
        public:
            Catchment_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing, utils::StreamHandler output_stream)
                : Formulation(id)
                , HY_CatchmentArea(output_stream)
                , forcing(forcing)
        {
                    // Assume the catchment ID is equal to or embedded in the formulation `id`
                    size_t idx = id.find(".");
                    set_catchment_id( idx == std::string::npos ? id : id.substr(0, idx) );
        }

            Catchment_Formulation(std::string id) : Formulation(id){
                    // Assume the catchment ID is equal to or embedded in the formulation `id`
                    size_t idx = id.find(".");
                    set_catchment_id( idx == std::string::npos ? id : id.substr(0, idx) );
            };

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
            virtual std::string get_output_header_line(std::string delimiter=DEFAULT_FORMULATION_OUTPUT_DELIMITER) const {
                return "Total Discharge";
            }

            /**
             * Get a formatted line of output values for the given time step as a delimited string.
             *
             * This method is useful for preparing calculated data in a representation useful for output files, such as
             * CSV files.
             *
             * The resulting string will contain calculated values for applicable output variables for the particular
             * formulation, as determined for the given time step.  However, the string will not contain any
             * representation of the time step itself.
             *
             * An empty string is returned if the time step value is not in the range of valid time steps for which there
             * are calculated values for all variables.
             *
             * @param timestep The time step for which data is desired.
             * @param delimiter The value delimiter for the string.
             * @return A delimited string with all the output variable values for the given time step.
             */
            virtual std::string get_output_line_for_timestep(int timestep,
                                                             std::string delimiter = DEFAULT_FORMULATION_OUTPUT_DELIMITER) = 0;

            /**
             * Execute the backing model formulation for the given time step, where it is of the specified size, and
             * return the response output.
             *
             * Any inputs and additional parameters must be made available as instance members.
             *
             * Types should clearly document the details of their particular response output.
             *
             * @param t_index The index of the time step for which to run model calculations.
             * @param t_delta The duration, in seconds, of the time step for which to run model calculations.
             * @return The response output of the model for this time step.
             */
            virtual double get_response(time_step_t t_index, time_step_t t_delta) override = 0;

            const std::vector<std::string>& get_required_parameters() const override = 0;

            void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override = 0;
            void create_formulation(geojson::PropertyMap properties) override = 0;
            virtual ~Catchment_Formulation(){};

        /**
         * Release resources of the given forcing provider
         */
        void finalize()
        {
            if (forcing) {
                forcing->finalize();
                forcing = nullptr;
            }
        }

    protected:
        std::string get_catchment_id() const override {
            return this->cat_id;
        }

        void set_catchment_id(std::string cat_id) override {
            this->cat_id = cat_id;
        }

        std::shared_ptr<data_access::GenericDataProvider> forcing;

    private:
        std::string cat_id;
    };
}
#endif // CATCHMENT_FORMULATION_H
