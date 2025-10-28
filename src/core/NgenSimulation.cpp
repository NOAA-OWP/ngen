#include <NgenSimulation.hpp>
#include <NGenConfig.h>
#include <Logger.hpp>

#if NGEN_WITH_ROUTING
#include "routing/Routing_Py_Adapter.hpp"
#endif // NGEN_WITH_ROUTING

NgenSimulation::NgenSimulation(
    std::shared_ptr<realization::Formulation_Manager> formulation_manager,
    std::vector<std::shared_ptr<ngen::Layer>> layers,
    std::unordered_map<std::string, int> catchment_indexes,
    std::unordered_map<std::string, int> nexus_indexes,
    int mpi_rank
                               )
    : catchment_formulation_manager_(formulation_manager)
    , simulation_step_(0)
    , sim_time_(std::make_shared<Simulation_Time>(*formulation_manager->Simulation_Time_Object))
    , layers_(std::move(layers))
    , catchment_indexes_(std::move(catchment_indexes))
    , nexus_indexes_(std::move(nexus_indexes))
    , mpi_rank_(mpi_rank)
{
    catchment_outflows_.reserve(catchment_indexes_.size() * get_num_output_times());
    nexus_downstream_flows_.reserve(nexus_indexes_.size() * get_num_output_times());
}

NgenSimulation::~NgenSimulation() = default;

void NgenSimulation::run_catchments()
{
    std::stringstream ss;

    // Now loop some time, iterate catchments, do stuff for total number of output times
    auto num_times = get_num_output_times();

    for (; simulation_step_ < num_times; simulation_step_++) {
        // The Inner loop will advance all layers unless doing so will break one of two constraints
        // 1) A layer may not proceed ahead of the master simulation object's current time
        // 2) A layer may not proceed ahead of any layer that is computed before it
        // The do while loop ensures that all layers are tested at least once while allowing
        // layers with small time steps to be updated more than once
        // If a layer with a large time step is after a layer with a small time step the
        // layer with the large time step will wait for multiple timesteps from the preceeding
        // layer.

        // Make room for this output step's results
        catchment_outflows_.resize(catchment_outflows_.size() + catchment_indexes_.size());
        nexus_downstream_flows_.resize(nexus_downstream_flows_.size() + nexus_indexes_.size());

        // this is the time that layers are trying to reach (or get as close as possible)
        auto next_time = catchment_formulation_manager_->Simulation_Time_Object->next_timestep_epoch_time();
        auto next_time_private = sim_time_->next_timestep_epoch_time();

        // this is the time that the layer above the current layer is at
        auto prev_layer_time = next_time;

        // this is the time that the least advanced layer is at
        auto layer_min_next_time = next_time;
        do {
            for (auto& layer : layers_) {
                auto layer_next_time = layer->next_timestep_epoch_time();

                // only advance if you would not pass the master next time and the previous layer
                // next time
                if (layer_next_time <= next_time && layer_next_time <= prev_layer_time) {
                    if (simulation_step_ % 100 == 0) {
                        ss << "Updating layer: '" << layer->get_name() << "' at output step " << simulation_step_;
                        LOG(ss.str(), LogLevel::DEBUG);
                        ss.str("");
                    }
#if NGEN_WITH_ROUTING
                    boost::span<double> catchment_span(catchment_outflows_.data() + (simulation_step_ * catchment_indexes_.size()),
                                                       catchment_indexes_.size());
                    boost::span<double> nexus_span(nexus_downstream_flows.data() + (simulation_step_ * nexus_indexes_.size()),
                                                   nexus_indexes_.size());
#else
                    boost::span<double> catchment_span;
                    boost::span<double> nexus_span;
#endif
                    layer->update_models(
                        catchment_span,
                        catchment_indexes_,
                        nexus_span,
                        nexus_indexes_,
                        simulation_step_
                    ); // assume update_models() calls time->advance_timestep()
                    prev_layer_time = layer_next_time;
                } else {
                    layer_min_next_time = prev_layer_time = layer->current_timestep_epoch_time();
                }

                if (layer_min_next_time > layer_next_time) {
                    layer_min_next_time = layer_next_time;
                }
            } // done layers
        } while (layer_min_next_time < next_time); // rerun the loop until the last layer would pass the master next time

        if (simulation_step_ + 1 < num_times) {
            sim_time_->advance_timestep();
            catchment_formulation_manager_->Simulation_Time_Object->advance_timestep();
        }
    }
}


void NgenSimulation::advance_one_output_step()
{

}

void NgenSimulation::run_routing()
{
#if NGEN_WITH_ROUTING
    // Run t-route from single process
    if (mpi_rank != 0)
        return;

    // Note: Currently, delta_time is set in the t-route yaml configuration file, and the
    // number_of_timesteps is determined from the total number of nexus outputs in t-route.
    // It is recommended to still pass these values to the routing_py_adapter object in
    // case a future implmentation needs these two values from the ngen framework.
    int number_of_timesteps = get_num_output_times();

    int delta_time = sim_time_->get_output_interval_seconds();

    router_->route(number_of_timesteps, delta_time);
#endif
}

size_t NgenSimulation::get_num_output_times()
{
    return sim_time_->get_total_output_times();
}

template <class Archive>
void NgenSimulation::serialize(Archive& ar) {
    /* Handle `catchment_formulation_manager` specially in the
     * overall checkpoint/restart logic, so that we can subset
     * individual catchments and do other tricky things */
    //ar & catchment_formulation_manager

    ar & simulation_step_;
    ar & sim_time_;

    // Layers will be reconstructed, but their internal time keeping needs to be serialized

    // Nexus and catchment indexes could be re-generated, but only if
    // the set of catchments remains consistent
    ar & catchment_indexes_;
    ar & catchment_outflows_;
    ar & nexus_indexes_;
    ar & nexus_downstream_flows_;
}
