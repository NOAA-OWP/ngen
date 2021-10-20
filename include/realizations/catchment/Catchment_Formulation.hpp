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
