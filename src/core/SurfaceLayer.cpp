#include "SurfaceLayer.hpp"

/***
 * @brief Run one simulation timestep for each model in this layer, then gather catchment output
*/

void ngen::SurfaceLayer::update_models()
{
    long current_time_index = output_time_index;
    
    Layer::update_models();

    // On the first time step, check all the nexuses and warn user about ones have no contributing catchments
    if (current_time_index == 0) {
        for(const auto& id : features.nexuses()) {
            #if NGEN_WITH_MPI
            // When running with MPI, only be concerned with the local nexuses
            if (!features.is_remote_sender_nexus(id) && features.nexus_at(id)->get_contributing_catchments().size() == 0)
            #else
            if (features.nexus_at(id)->get_contributing_catchments().size() == 0)
            #endif
            {
                // Likely this means a flow value of 0.0, but that's dependent on the nexus class implementation
                std::cout << "WARNING: Nexus "<< id << " has no contributing catchments for flow values!" << std::endl;
            }
        }
    }

    //At this point, could make an internal routing pass, extracting flows from nexuses and routing
    //across the flowpath to the next nexus.
    //Once everything is updated for this timestep, dump the nexus output
    for(const auto& id : features.nexuses()) 
    {
        std::string current_timestamp = simulation_time.get_timestamp(current_time_index);
        
        #if NGEN_WITH_MPI
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

        //std::cerr << "Requesting water from nexus, id = " << id << " at time = " <<current_time_index << ",  percent = 100, destination = " << cat_id << std::endl;
        double contribution_at_t = features.nexus_at(id)->get_downstream_flow(cat_id, current_time_index, 100.0);
        
        if(nexus_outfiles[id].is_open()) {
        nexus_outfiles[id] << current_time_index << ", " << current_timestamp << ", " << contribution_at_t << std::endl;
        }

        #if NGEN_WITH_MPI
        }
        #endif
        //std::cout<<"\tNexus "<<id<<" has "<<contribution_at_t<<" m^3/s"<<std::endl;

        //Note: Use below if developing in-memory transfer of nexus flows to routing
        //If using below, then another single time vector would be needed to hold the timestamp
        //nexus_flows[id].push_back(contribution_at_t); 
    } //done nexuses
}
