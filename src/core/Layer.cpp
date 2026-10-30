#include <Layer.hpp>

#include <optional>

#include <boost/type_index.hpp>
#include <Formulation.hpp>
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
    // The time duration passed to the model for this step is the output interval in seconds.
    // TODO output_interval is really not the best thing here, but it is what we have for now...
    const long model_step_seconds = simulation_time.get_output_interval_seconds();

    for(const auto& id : processing_units) {
        //std::cout<<"Running cat "<<id<<std::endl;
        auto realization = features.catchment_at(id);
        auto formulation = realization->get_formulation();
        HY_CatchmentRealization::Response response;
        try{
            response = realization->get_response(output_time_index, model_step_seconds);
            // Check mass balance if able
            formulation->check_mass_balance(output_time_index, simulation_time.get_total_output_times(), current_timestamp);
            // Checkpoint state via the serialization protocol (no-op when unconfigured)
            formulation->checkpoint_state(output_time_index, simulation_time.get_total_output_times(), current_timestamp);
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
        std::optional<double> depth = response.in(HY_CatchmentRealization::Quantity::depth);
        if (!depth) {
            throw std::runtime_error(
                "Catchment '" + id + "' is realized as " +
                boost::typeindex::type_id_runtime(*realization).pretty_name() +
                ", which does not express its value as a depth.");
        }
        std::atomic_ref(catchment_outflows[results_index]) += *depth;

#endif // NGEN_WITH_ROUTING && NGEN_WITH_ROUTING_TROUTE_BMI
        if (catchment_output_mgr) {
            catchment_output_mgr->receive_data_entry(
                id, current_time_marker,
                formulation->get_output_values_for_timestep(output_time_index));
        }
        // Get the volumetric flow rate from the realization's response
        std::optional<double> flow_m3_s =
            response.in(HY_CatchmentRealization::Quantity::volumetric_flow);
        if (!flow_m3_s) {
            throw std::runtime_error(
                "Catchment '" + id + "' is realized as " +
                boost::typeindex::type_id_runtime(*realization).pretty_name() +
                ", which does not express its value as a volumetric flow.");
        }
        //update the nexus with this flow
        for(auto& nexus : features.destination_nexuses(id)) {
            //TODO in a DENDRITIC network, only one destination nexus per catchment
            //If there is more than one, some form of catchment partitioning will be required.
            //for now, only contribute to the first one in the list
            if(nexus == nullptr){
                throw std::runtime_error("Invalid (null) nexus instantiation downstream of '"+id+"'");
            }
            nexus->add_upstream_flow(*flow_m3_s, id, output_time_index);
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
