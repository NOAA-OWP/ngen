#include "SurfaceLayer.hpp"

void ngen::SurfaceLayer::update_models(boost::span<double> catchment_outflows, 
                                       std::unordered_map<std::string, int> &catchment_indexes,
                                       boost::span<double> nexus_downstream_flows,
                                       std::unordered_map<std::string, int> &nexus_indexes,
                                       int current_step)
{
    long current_time_index = output_time_index;
    
    Layer::update_models(catchment_outflows, catchment_indexes, nexus_downstream_flows, nexus_indexes, current_step);

    // Once contributing catchments are updated for this timestep, dump the nexus output
    for(const auto& id : features.nexuses()) 
    {
        std::string current_timestamp = simulation_time.get_timestamp(current_time_index);
        
        #if NGEN_WITH_MPI
        // Ensures only one side of the dual sided remote nexus actually does this
        if (features.is_remote_sender_nexus(id)) continue;
        #endif

        // Get the correct "requesting" id for downstream_flow
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

        //std::cerr << "Requesting water from nexus, id = " << id << " at time = " <<current_time_index << ",  percent = 100, destination = " << cat_id << std::endl;
        double contribution_at_t = features.nexus_at(id)->get_downstream_flow(cat_id, current_time_index, 100.0);

#if NGEN_WITH_ROUTING
        int nexus_index = nexus_indexes[id];
        nexus_downstream_flows[nexus_index] = contribution_at_t;
#endif // NGEN_WITH_ROUTING

        //std::cout<<"\tNexus "<<id<<" has "<<contribution_at_t<<" m^3/s"<<std::endl;
    } //done nexuses
}
