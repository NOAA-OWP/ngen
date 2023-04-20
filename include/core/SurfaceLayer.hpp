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
                std::shared_ptr<pdm03_struct> pdmd,
                long idx,
                const std::vector<std::string>& n_u,
                std::unordered_map<std::string, std::ofstream>& output_files) : 
                    Layer(desc,p_u,s_t,f,cd,pdmd,idx), 
                    nexus_ids(n_u), 
                    nexus_outfiles(output_files)
        {

        }

        /***
         * @breif Run one simulation timestep for each model in this layer
        */
        virtual void update_models()
        {
            long current_time_index = output_time_index;
            
            Layer::update_models();

            //At this point, could make an internal routing pass, extracting flows from nexuses and routing
            //across the flowpath to the next nexus.
            //Once everything is updated for this timestep, dump the nexus output
            for(const auto& id : features.nexuses()) 
            {
                std::string current_timestamp = simulation_time.get_timestamp(current_time_index);
                
                #ifdef NGEN_MPI_ACTIVE
                if (!features.is_remote_sender_nexus(id)) { //Ensures only one side of the dual sided remote nexus actually doing this...
                #endif

                //Get the correct "requesting" id for downstream_flow
                const auto& nexus = features.nexus_at(id);
                const auto& cat_ids = nexus->get_receiving_catchments();
                std::string cat_id;
                if( cat_ids.size() > 0 ) {
                //Assumes dendridic, e.g. only a single downstream...it will consume 100%  of the available flow
                cat_id = cat_ids[0];
                }
                else {
                //This is a terminal node, SHOULDN'T be remote, so ID shouldn't matter too much
                cat_id = "terminal";
                }

                std::cerr << "Requesting water from nexus, id = " << id << " at time = " <<current_time_index << ",  percent = 100, destination = " << cat_id << std::endl;
                double contribution_at_t = features.nexus_at(id)->get_downstream_flow(cat_id, current_time_index, 100.0);
                
                if(nexus_outfiles[id].is_open()) {
                nexus_outfiles[id] << current_time_index << ", " << current_timestamp << ", " << contribution_at_t << std::endl;
                }

                #ifdef NGEN_MPI_ACTIVE
                }
                #endif
                //std::cout<<"\tNexus "<<id<<" has "<<contribution_at_t<<" m^3/s"<<std::endl;

                //Note: Use below if developing in-memory transfer of nexus flows to routing
                //If using below, then another single time vector would be needed to hold the timestamp
                //nexus_flows[id].push_back(contribution_at_t); 
            } //done nexuses
        }

        private:

        std::vector<std::string> nexus_ids;
        std::unordered_map<std::string, std::ofstream>& nexus_outfiles;
    };
}

#endif