#ifndef CATCHMENT_FORMULATION_H
#define CATCHMENT_FORMULATION_H

#include <memory>
#include <vector>
#include "Formulation.hpp"
#include <HY_CatchmentArea.hpp>
#include "GenericDataProvider.hpp"

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
             * Get a header line appropriate for a file made up of entries from this type's implementation of
             * ``get_output_line_for_timestep``.
             *
             * Note that like the output generating function, this line does not include anything for time step.
             *
             * A default implementation is provided for inheritors of this type, which includes only "Total Discharge."
             *
             * @return An appropriate header line for this type.
             */
            virtual std::string get_output_header_line(std::string delimiter=DEFAULT_FORMULATION_OUTPUT_DELIMITER) const;

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
            virtual ~Catchment_Formulation() = default;

            /**
             * Get get a count of fields that will be included by ``get_output_header_line``.
             *
             * Note that like the output generating function, this line does not include anything for time step.
             *
             * A default implementation is provided for inheritors of this type, which includes only "Total Discharge."
             *
             * @return The number of fields included in the header output.
             */
            virtual size_t get_output_header_count() const {
                return 1;
            }

            /**
             * Release resources of the given forcing provider
             */
            void finalize();

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
