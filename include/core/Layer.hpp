#ifndef __NGEN_LAYER__
#define __NGEN_LAYER__

#include "LayerData.hpp"
#include "Simulation_Time.hpp"
#include <HY_Features.hpp>

namespace ngen
{

    class Layer
    {
        #ifdef NGEN_MPI_ACTIVE
            typedef hy_features::HY_Features_MPI feature_type;
        #else
            typedef hy_features::HY_Features feature_type;
        #endif
        
        public:

        Layer(const LayerDescription& desc, const std::vector<std::string>& p_u, const Simulation_Time& s_t, feature_type& f) :
            description(desc),
            processing_units(p_u),
            simulation_time(s_t),
            features(f)
        {

        }

        time_t next_timestep_epoch_time() { return simulation_time.next_timestep_epoch_time(); }

        void update_models()
        {
            auto idx = simulation_time.next_timestep_index();
            auto step = simulation_time.get_output_interval_seconds();
            
            for( std::string& unit : processing_units)
            {
                auto r = features.catchment_at(id);
                //TODO redesign to avoid this cast
                auto r_c = dynamic_pointer_cast<realization::Catchment_Formulation>(r);
                r_c->set_et_params(pdm_et_data)
                double response = r_c->get_response(ts1, m_data.time_step)
            }
        }

        

        private:

        LayerDescription description;
        std::vector<std::string> processing_units;
        Simulation_Time simulation_time;
        feature_type& features;       

    };
}

#endif