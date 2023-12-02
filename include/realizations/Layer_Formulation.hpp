#ifndef LAYER_FORMULATION_H
#define LAYER_FORMULATION_H

#include <memory>
#include <vector>
#include "Formulation.hpp"
#include "GenericDataProvider.hpp"
#include "GM_Object.hpp"
#include "FileStreamHandler.hpp"

namespace realization {

    class Layer_Formulation : public Formulation {
        public:
            Layer_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing, utils::StreamHandler output_stream)
                : Formulation(id), forcing(forcing), output(output_stream) { 

                };

            Layer_Formulation(std::string id) : Formulation(id){

            };

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
            virtual ~Layer_Formulation(){};

    protected:
        polygon_t bounds;
        std::shared_ptr<data_access::GenericDataProvider> forcing;
        utils::StreamHandler output;

    private:
        std::string cat_id;
    };
}
#endif // LAYER_FORMULATION_H
