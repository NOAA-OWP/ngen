#include "SurfaceLayer.hpp"
#include "utilities/logging_utils.h"

/***
 * @brief Run one simulation timestep for each model in this layer, then gather catchment output
*/

void ngen::SurfaceLayer::update_models()
{
    long current_time_index = output_time_index;
    
    Layer::update_models();

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

        //logging::warning(("Requesting water from nexus, id = " + id + " at time = " + std::to_string(current_time_index) + ",  percent = 100, destination = " + cat_id + "\n").c_str());
        double contribution_at_t = features.nexus_at(id)->get_downstream_flow(cat_id, current_time_index, 100.0);
        
        if(nexus_outfiles[id].is_open()) {
        nexus_outfiles[id] << current_time_index << ", " << current_timestamp << ", " << contribution_at_t << std::endl;
        }

        #if NGEN_WITH_MPI
        }
        #endif
        //contribution_at_t out of scope unless for serial build
        //logging::info(("\tNexus "+id+" has "+std::to_string(contribution_at_t)+" m^3/s"+"\n").c_str());

        //Note: Use below if developing in-memory transfer of nexus flows to routing
        //If using below, then another single time vector would be needed to hold the timestamp
        //nexus_flows[id].push_back(contribution_at_t); 
    } //done nexuses
}
