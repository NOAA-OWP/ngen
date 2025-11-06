#include <NgenSimulation.hpp>
#include <NGenConfig.h>
#include <Logger.hpp>

#if NGEN_WITH_ROUTING
#include "bmi/Bmi_Py_Adapter.hpp"
#endif // NGEN_WITH_ROUTING

#include "parallel_utils.h"

NgenSimulation::NgenSimulation(
    std::shared_ptr<realization::Formulation_Manager> formulation_manager,
    std::vector<std::shared_ptr<ngen::Layer>> layers,
    std::unordered_map<std::string, int> catchment_indexes,
    std::unordered_map<std::string, int> nexus_indexes,
    int mpi_rank,
    int mpi_num_procs
                               )
    : catchment_formulation_manager_(formulation_manager)
    , simulation_step_(0)
    , sim_time_(std::make_shared<Simulation_Time>(*formulation_manager->Simulation_Time_Object))
    , layers_(std::move(layers))
    , catchment_indexes_(std::move(catchment_indexes))
    , nexus_indexes_(std::move(nexus_indexes))
    , mpi_rank_(mpi_rank)
    , mpi_num_procs_(mpi_num_procs)
{
    catchment_outflows_.reserve(catchment_indexes_.size() * get_num_output_times());
    nexus_downstream_flows_.reserve(nexus_indexes_.size() * get_num_output_times());
}

NgenSimulation::~NgenSimulation() = default;

void NgenSimulation::run_catchments()
{
    // Now loop some time, iterate catchments, do stuff for total number of output times
    auto num_times = get_num_output_times();

    for (; simulation_step_ < num_times; simulation_step_++) {
        // Make room for this output step's results
        catchment_outflows_.resize(catchment_outflows_.size() + catchment_indexes_.size());
        nexus_downstream_flows_.resize(nexus_downstream_flows_.size() + nexus_indexes_.size());

        advance_one_output_step();

        if (simulation_step_ + 1 < num_times) {
            sim_time_->advance_timestep();
            catchment_formulation_manager_->Simulation_Time_Object->advance_timestep();
        }
    }
}

void NgenSimulation::advance_one_output_step()
{
    // The Inner loop will advance all layers unless doing so will break one of two constraints
    // 1) A layer may not proceed ahead of the master simulation object's current time
    // 2) A layer may not proceed ahead of any layer that is computed before it
    // The do while loop ensures that all layers are tested at least once while allowing
    // layers with small time steps to be updated more than once
    // If a layer with a large time step is after a layer with a small time step the
    // layer with the large time step will wait for multiple timesteps from the preceeding
    // layer.

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
                    LOG(("Updating layer: '" + layer->get_name() + "' at output step " + std::to_string(simulation_step_)), LogLevel::DEBUG);
                }

                boost::span<double> catchment_span(catchment_outflows_.data() + (simulation_step_ * catchment_indexes_.size()),
                                                   catchment_indexes_.size());
                boost::span<double> nexus_span(nexus_downstream_flows_.data() + (simulation_step_ * nexus_indexes_.size()),
                                               nexus_indexes_.size());
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

}

int NgenSimulation::get_nexus_index(std::string const& nexus_id) const
{
    auto iter = nexus_indexes_.find(nexus_id);
    return (iter != nexus_indexes_.end()) ? iter->second : -1;
}

double NgenSimulation::get_nexus_outflow(int nexus_index, int timestep_index) const
{
    return nexus_downstream_flows_[timestep_index * nexus_indexes_.size() + nexus_index];
}

void NgenSimulation::run_routing(NgenSimulation::hy_features_t &features)
{
#if NGEN_WITH_ROUTING
    std::vector<double> *routing_nexus_downflows = &nexus_downstream_flows_;
    std::unordered_map<std::string, int> *routing_nexus_indexes = &nexus_indexes_;

#if NGEN_WITH_MPI
    std::vector<double> all_nexus_downflows;
    std::unordered_map<std::string, int> all_nexus_indexes;

    if (mpi_num_procs_ > 1) {
        int number_of_timesteps = catchment_formulation_manager_->Simulation_Time_Object->get_total_output_times();
        std::vector<std::string> local_nexus_ids;
        for (const auto& nexus : nexus_indexes_) {
            local_nexus_ids.push_back(nexus.first);
        }
        // MPI_Gather all nexus IDs into a single vector
        std::vector<std::string> all_nexus_ids = parallel::gather_strings(local_nexus_ids, mpi_rank_, mpi_num_procs_);
        if (mpi_rank_ == 0) {
            // filter to only the unique IDs
            std::sort(all_nexus_ids.begin(), all_nexus_ids.end());
            all_nexus_ids.erase(
                std::unique(all_nexus_ids.begin(), all_nexus_ids.end()),
                all_nexus_ids.end()
            );
        }
        // MPI_Broadcast so all processes share the nexus IDs
        all_nexus_ids = std::move(parallel::broadcast_strings(all_nexus_ids, mpi_rank_, mpi_num_procs_));

        // MPI_Reduce to collect the results from processes
        if (mpi_rank_ == 0) {
            all_nexus_downflows.resize(number_of_timesteps * all_nexus_ids.size(), 0.0);
        }
        std::vector<double> local_buffer(number_of_timesteps);
        std::vector<double> receive_buffer(number_of_timesteps, 0.0);
        for (int i = 0; i < all_nexus_ids.size(); ++i) {
            std::string nexus_id = all_nexus_ids[i];
            if (nexus_indexes_.find(nexus_id) != nexus_indexes_.end() && !features.is_remote_sender_nexus(nexus_id)) {
                // if this process has the id and receives/records data, copy the values to the buffer
                int nexus_index = nexus_indexes_[nexus_id];
                for (int step = 0; step < number_of_timesteps; ++step) {
                    int offset = step * nexus_indexes_.size() + nexus_index;
                    local_buffer[step] = nexus_downstream_flows_[offset];
                }
            } else {
                // if this process does not have the id, fill with 0 to make sure it doesn't affect reduce sum
                std::fill(local_buffer.begin(), local_buffer.end(), 0.0);
            }
            MPI_Reduce(local_buffer.data(), receive_buffer.data(), number_of_timesteps, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
            if (mpi_rank_ == 0) {
                // copy reduce values to a combined downflows vector
                all_nexus_indexes[nexus_id] = i;
                for (int step = 0; step < number_of_timesteps; ++step) {
                    int offset = step * all_nexus_ids.size() + i;
                    all_nexus_downflows[offset] = receive_buffer[step];
                    receive_buffer[step] = 0.0;
                }
            }
        }

        if (mpi_rank_ == 0) {
            // update root's local data for running t-route below
            routing_nexus_indexes = &all_nexus_indexes;
            routing_nexus_downflows = &all_nexus_downflows;
        }
    }
#endif // NGEN_WITH_MPI

    if (mpi_rank_ == 0) { // Run t-route from single process
        if (catchment_formulation_manager_->get_using_routing()) {
            LOG(LogLevel::INFO, "Running T-Route on nexus outflows.");

            // Note: Currently, delta_time is set in the t-route yaml configuration file, and the
            // number_of_timesteps is determined from the total number of nexus outputs in t-route.
            // It is recommended to still pass these values to the routing_py_adapter object in
            // case a future implmentation needs these two values from the ngen framework.
            int number_of_timesteps = catchment_formulation_manager_->Simulation_Time_Object->get_total_output_times();

            int delta_time = catchment_formulation_manager_->Simulation_Time_Object->get_output_interval_seconds();

            std::string t_route_config_file_with_path =
                catchment_formulation_manager_->get_t_route_config_file_with_path();
            // model for routing
            models::bmi::Bmi_Py_Adapter py_troute("T-Route", t_route_config_file_with_path, "troute_nwm_bmi.troute_bmi.BmiTroute", true);

            // tell BMI to resize nexus containers
            int64_t nexus_count = routing_nexus_indexes->size();
            py_troute.SetValue("land_surface_water_source__volume_flow_rate__count", &nexus_count);
            py_troute.SetValue("land_surface_water_source__id__count", &nexus_count);
            // set up nexus id indexes
            std::vector<int> nexus_df_index(nexus_count);
            for (const auto& key_value : *routing_nexus_indexes) {
                int id_index = key_value.second;

                // Convert string ID into numbers for T-route index
                int id_as_int = -1;
                size_t sep_index = key_value.first.find(hy_features::identifiers::seperator);
                if (sep_index != std::string::npos) {
                    std::string numbers = key_value.first.substr(sep_index + hy_features::identifiers::seperator.length());
                    id_as_int = std::stoi(numbers);
                }
                if (id_as_int == -1) {
                    std::string error_msg = "Cannot convert the nexus ID to an integer: " + key_value.first;
                    LOG(LogLevel::FATAL, error_msg);
                    throw std::runtime_error(error_msg);
                }
                nexus_df_index[id_index] = id_as_int;
            }
            py_troute.SetValue("land_surface_water_source__id", nexus_df_index.data());
            for (int i = 0; i < number_of_timesteps; ++i) {
                py_troute.SetValue("land_surface_water_source__volume_flow_rate",
                                   routing_nexus_downflows->data() + (i * nexus_count));
                py_troute.Update();
            }
            // Finalize will write the output file
            py_troute.Finalize();
        }
    }
#endif // NGEN_WITH_ROUTING
}

size_t NgenSimulation::get_num_output_times() const
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
