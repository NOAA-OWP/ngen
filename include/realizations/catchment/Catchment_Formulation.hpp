#ifndef CATCHMENT_FORMULATION_H
#define CATCHMENT_FORMULATION_H

#include <memory>
#include <vector>
#include "Formulation.hpp"
#include <HY_CatchmentArea.hpp>
#include "GenericDataProvider.hpp"
#include "utilities/output/CatchmentOutputsMgr.hpp"   // utils::OutputField

#include "Logger.hpp"

#define DEFAULT_FORMULATION_OUTPUT_DELIMITER ","

namespace realization {

    class Catchment_Formulation : public Formulation, public HY_CatchmentArea {
        public:
            Catchment_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing, utils::StreamHandler output_stream);
            Catchment_Formulation(std::string id);

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
                                                    const std::string &pattern, const std::string &replacement);

            /**
             * Get this formulation's output fields -- name plus metadata (units, ...) -- in the same
             * order as the values returned by @ref get_output_values_for_timestep.
             *
             * The name is the output header field (configured, or defaulting to the output variable
             * name); the units come from the same source that produces each value. So each field is
             * positionally aligned with, and describes, the corresponding value.
             *
             * @return The ordered output fields.
             */
            virtual std::vector<utils::OutputField> get_output_fields() const = 0;

            /**
             * Get the output values for the given time step, positionally aligned with @ref get_output_fields.
             *
             * Values are returned as raw doubles, with no formatting or precision applied by the formulation.
             *
             * @param timestep The time step for which values are desired.
             * @return The output values for @p timestep, one per output column.
             */
            virtual std::vector<double> get_output_values_for_timestep(int timestep) = 0;

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
            virtual ~Catchment_Formulation() = default;

            /**
             * Release resources of the given forcing provider
             */
            void finalize();

    protected:
            const std::string& get_catchment_id() const override {
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
