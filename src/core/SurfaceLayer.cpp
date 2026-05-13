#include "SurfaceLayer.hpp"

void ngen::SurfaceLayer::update_models(boost::span<double> catchment_outflows, 
                                       std::unordered_map<std::string, int> &catchment_indexes,
                                       boost::span<double> nexus_downstream_flows,
                                       std::unordered_map<std::string, int> &nexus_indexes,
                                       int current_step)
{
    long current_time_index = output_time_index;
    
    Layer::update_models(catchment_outflows, catchment_indexes, nexus_downstream_flows, nexus_indexes, current_step);

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

    // Grab time details (but only once since the output_time_index doesn' (and shouldn't) change
    std::string current_timestamp = simulation_time.get_timestamp(current_time_index);
    // Remember: above call to simulation_time.get_timestamp(current_time_index) has to be made first (see those funcs)
    time_t current_date_time_epoch = simulation_time.get_current_epoch_time();

    utils::time_marker current_time_marker(current_time_index, current_date_time_epoch, current_timestamp);

    // Once contributing catchments are updated for this timestep, dump the nexus output
    for(const auto& id : features.nexuses()) 
    {
        #if NGEN_WITH_MPI
        // Ensures only one side of the dual sided remote nexus actually does this
        if (features.is_remote_sender_nexus(id)) continue;
        #endif

        // Get the correct "requesting" id for downstream_flow
        const auto& nexus = features.nexus_at(id);
        const auto& cat_ids = nexus->get_receiving_catchments();

        if (cat_ids.size() > 1) {
            std::string error = "Nexus '" + id + "' violates dendritic hydrofabric network assumption";
            throw std::runtime_error(error);
        }

        std::string cat_id;
        if( cat_ids.size() == 1 ) {
            // With a dendritic network, there can only be a single downstream. It will consume 100%  of the available flow
            cat_id = cat_ids[0];
        }
        else {
            // This is a terminal node, SHOULDN'T be remote, so ID shouldn't matter too much
            cat_id = "terminal";
        }

        //std::cerr << "Requesting water from nexus, id = " << id << " at time = " <<current_time_index << ",  percent = 100, destination = " << cat_id << std::endl;
        double contribution_at_t = features.nexus_at(id)->get_downstream_flow(cat_id, current_time_index, 100.0);

#if NGEN_WITH_ROUTING
        int nexus_index = nexus_indexes[id];
        nexus_downstream_flows[nexus_index] += contribution_at_t;
#endif // NGEN_WITH_ROUTING

        // TODO: (later) eventually may want to use this form, if we support multiple formulations per catchment
        //nexus_outputs_mgr->receive_data_entry(form_id, id, current_time_index, current_timestamp, contribution_at_t);
        nexus_outputs_mgr->receive_data_entry(id, current_time_marker, contribution_at_t);

        //std::cout<<"\tNexus "<<id<<" has "<<contribution_at_t<<" m^3/s"<<std::endl;
    } //done nexuses
    nexus_outputs_mgr->commit_writes();
}
