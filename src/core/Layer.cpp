#include <Layer.hpp>
#include <Catchment_Formulation.hpp>
#include <CatchmentOutputsMgr.hpp>

#if NGEN_WITH_MPI
#include "HY_Features_MPI.hpp"
#else
#include "HY_Features.hpp"
#endif

// Out-of-line so the shared_ptr members are destroyed where their (possibly forward-declared) types
// are complete.
ngen::Layer::~Layer() = default;

void ngen::Layer::update_models(boost::span<double> catchment_outflows,
                                std::unordered_map<std::string, int> const & catchment_indexes,
                                boost::span<double> nexus_downstream_flows,
                                std::unordered_map<std::string, int> const& nexus_indexes,
                                int current_step)
{
    //std::cout<<"Output Time Index: "<<output_time_index<<std::endl;
    if(output_time_index%1000 == 0) std::cout<<"Running timestep " << output_time_index <<std::endl;
    std::string current_timestamp = simulation_time.get_timestamp(output_time_index);
    // Catchment output (if enabled) is pushed to this layer's output manager, which owns the
    // sinks and decides formatting/aggregation. Build the time marker once for all catchments
    // in this timestep (mirrors SurfaceLayer).
    utils::time_marker current_time_marker(
        output_time_index, simulation_time.get_current_epoch_time(), current_timestamp);
    for(const auto& id : processing_units) {
        int sub_time = output_time_index;
        //std::cout<<"Running cat "<<id<<std::endl;
        auto r = features.catchment_at(id);
        //TODO redesign to avoid this cast
        auto r_c = std::dynamic_pointer_cast<realization::Catchment_Formulation>(r);
        double response(0.0);
        try{
            response = r_c->get_response(output_time_index, simulation_time.get_output_interval_seconds());
            // Check mass balance if able
            r_c->check_mass_balance(output_time_index, simulation_time.get_total_output_times(), current_timestamp);
        }
        catch(models::external::State_Exception& e){
            std::string msg = e.what();
            msg = msg+" at timestep "+std::to_string(output_time_index)
                +" ("+current_timestamp+")"
                +" at feature id "+id;
            throw models::external::State_Exception(msg);
        }
        catch(std::exception& e){
            std::string msg = e.what();
            msg = msg+" at timestep "+std::to_string(output_time_index)
                +" ("+current_timestamp+")"
                +" at feature id "+id;
            throw std::runtime_error(msg);
        }
#if NGEN_WITH_ROUTING && NGEN_WITH_ROUTING_TROUTE_BMI
        int results_index = catchment_indexes.at(id);
        // XXX: This is currently accumulating in meters of depth, which may not be desirable
        catchment_outflows[results_index] += response;
#endif // NGEN_WITH_ROUTING && NGEN_WITH_ROUTING_TROUTE_BMI
        if (catchment_output_mgr) {
            catchment_output_mgr->receive_data_entry(
                id, current_time_marker, r_c->get_output_values_for_timestep(output_time_index));
        }
        //TODO put this somewhere else.  For now, just trying to ensure we get m^3/s into nexus output
        double area;
        try {
            area = catchment_data->get_feature(id)->get_property("areasqkm").as_real_number();
        }
        catch(std::invalid_argument &e) {
            area = catchment_data->get_feature(id)->get_property("area_sqkm").as_real_number();
        }
        double response_m_s = response * (area * 1000000);
        //TODO put this somewhere else as well, for now, an implicit assumption is that a module's get_response returns
        //m/timestep
        //since we are operating on a 1 hour (3600s) dt, we need to scale the output appropriately
        //so no response is m^2/hr...m^2/hr * 1hr/3600s = m^3/hr
        double response_m_h = response_m_s / 3600.0;
        //update the nexus with this flow
        for(auto& nexus : features.destination_nexuses(id)) {
            //TODO in a DENDRITIC network, only one destination nexus per catchment
            //If there is more than one, some form of catchment partitioning will be required.
            //for now, only contribute to the first one in the list
            if(nexus == nullptr){
                throw std::runtime_error("Invalid (null) nexus instantiation downstream of '"+id+"'");
            }
            nexus->add_upstream_flow(response_m_h, id, output_time_index);
            /*std::cerr << "Add water to nexus ID = " << nexus->get_id() << " from catchment ID = " << id << " value = "
              << response << ", ID = " << id << ", time-index = " << output_time_index << std::endl; */
            break;
        }
                
    } //done catchments

    ++output_time_index;
    if ( output_time_index < simulation_time.get_total_output_times() ) {
        simulation_time.advance_timestep();
    }
}
