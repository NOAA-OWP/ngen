#ifndef __NGEN_DOMAIN_LAYER__
#define __NGEN_DOMAIN_LAYER__

#include "Catchment_Formulation.hpp"
#include "Layer.hpp"

namespace ngen
{
    class DomainLayer : public Layer
    {
        public:
        DomainLayer() = delete;
        /**
         * @brief Construct a new Domain Layer object
         * 
         * @param desc LayerDescription for the domain layer
         * @param s_t Simulation_Time object associated with the domain layer
         * @param features collection of HY_Features associated with the domain
         * @param idx index of the layer
         * @param formulation Formulation associated with the domain
         */
        DomainLayer(
                const LayerDescription& desc,
                const Simulation_Time& s_t,
                feature_type& features,
                long idx,
                std::shared_ptr<realization::Catchment_Formulation> formulation):
                    Layer(desc, s_t, features, idx), formulation(formulation)
        {
            formulation->write_output("Time Step,""Time,"+formulation->get_output_header_line(",")+"\n");
        }

        /***
         * @brief Run one simulation timestep for this model associated with the domain
        */
        void update_models() override{
            formulation->get_response(output_time_index, simulation_time.get_output_interval_seconds());
            std::string current_timestamp = simulation_time.get_timestamp(output_time_index);
            std::string output = std::to_string(output_time_index)+","+current_timestamp+","+
            formulation->get_output_line_for_timestep(output_time_index)+"\n";
            formulation->write_output(output);
            ++output_time_index;
            if ( output_time_index < simulation_time.get_total_output_times() )
            {
               simulation_time.advance_timestep();
            }
    
        }

        private:
        std::shared_ptr<realization::Catchment_Formulation> formulation;
    };
}

#endif