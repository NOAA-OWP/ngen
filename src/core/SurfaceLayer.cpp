#include "SurfaceLayer.hpp"

/***
 * @brief Run one simulation timestep for each model in this layer, then gather catchment output
*/

void ngen::SurfaceLayer::update_models()
{
    long current_time_index = output_time_index;
    
    Layer::update_models();

    //At this point, could make an internal routing pass, extracting flows from nexuses and routing
    //across the flowpath to the next nexus.

    // Grab time details (but only once since the output_time_index doesn' (and shouldn't) change
    std::string current_timestamp = simulation_time.get_timestamp(current_time_index);
    // Remember: above call to simulation_time.get_timestamp(current_time_index) has to be made first (see those funcs)
    time_t current_date_time_epoch = simulation_time.get_current_epoch_time();

    //Once everything is updated for this timestep, dump the nexus output
    for(const auto& id : features.nexuses()) 
    {
        #if NGEN_WITH_MPI
        if (features.is_remote_sender_nexus(id)) {
            //Ensures only one side of the dual sided remote nexus actually doing this...
            return;
        }
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
        // TODO: (later) eventually may want to use this form, if we support multiple formulations per catchment
        //nexus_outputs_mgr->receive_data_entry(form_id, id, current_time_index, current_timestamp, contribution_at_t);
        nexus_outputs_mgr->receive_data_entry(id, current_time_index, current_date_time_epoch,
                                              current_timestamp, contribution_at_t);

    } //done nexuses
    nexus_outputs_mgr->commit_writes();
}
