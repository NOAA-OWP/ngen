#ifndef __NGEN_SURFACE_LAYER__
#define __NGEN_SURFACE_LAYER__

#include "Layer.hpp"
#include "NexusOutputsMgr.hpp"

namespace ngen
{
    class SurfaceLayer : public Layer
    {
        public:
        
        SurfaceLayer(
                const LayerDescription& desc, 
                const std::vector<std::string>& p_u, 
                const Simulation_Time& s_t, 
                feature_type& f, 
                geojson::GeoJSON cd, 
                long idx,
                const std::vector<std::string>& n_u,
                const std::shared_ptr<utils::NexusOutputsMgr> &nexus_outputs_mgr) :
                    Layer(desc,p_u,s_t,f,cd,idx), 
                    nexus_ids(n_u),
                    nexus_outputs_mgr(nexus_outputs_mgr)
        {

        }

        /***
         * @brief Run one simulation timestep for each model in this layer
        */
        void update_models() override;

        private:

        std::vector<std::string> nexus_ids;
        std::shared_ptr<utils::NexusOutputsMgr> nexus_outputs_mgr;
    };
}

#endif