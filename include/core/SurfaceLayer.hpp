#ifndef __NGEN_SURFACE_LAYER__
#define __NGEN_SURFACE_LAYER__

#include "Layer.hpp"

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
                long idx) :
                    Layer(desc,p_u,s_t,f,cd,idx)
        {

        }

        /***
         * @brief Run one simulation timestep for each model in this layer
        */
        void update_models(boost::span<double> catchment_outflows, 
                           std::unordered_map<std::string, int> &catchment_indexes,
                           boost::span<double> nexus_downstream_flows,
                           std::unordered_map<std::string, int> &nexus_indexes,
                           int current_step) override;
    };
}

#endif
