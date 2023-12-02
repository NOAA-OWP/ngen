#ifndef __NGEN_DOMAIN_LAYER__
#define __NGEN_DOMAIN_LAYER__

#include "Catchment_Formulation.hpp"
#include "Layer.hpp"

namespace ngen
{
    class DomainLayer : public Layer
    {
        public:
        // DomainLayer() = delete;
        DomainLayer(
                const LayerDescription& desc,
                const Simulation_Time& s_t,
                feature_type& features,
                long idx,
                std::shared_ptr<realization::Catchment_Formulation> formulation):
                    //description(desc), features(features), output_time_index(0), simulation_time(s_t), formulation(formulation)
                    Layer(desc, s_t, features, idx), formulation(formulation)
        {
            formulation->write_output("Time Step,""Time,"+formulation->get_output_header_line(",")+"\n");
        }

        /***
         * @brief Run one simulation timestep for each model in this layer
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
        // LayerDescription description;
        // Simulation_Time simulation_time;
        // long output_time_index;
        std::shared_ptr<realization::Catchment_Formulation> formulation;
    };
}

#endif