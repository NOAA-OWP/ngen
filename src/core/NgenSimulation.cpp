#include <NgenSimulation.hpp>
#include <NGenConfig.h>
#include "ewts_ngen/logger.hpp"

#if NGEN_WITH_MPI
#include "HY_Features_MPI.hpp"
#else
#include "HY_Features.hpp"
#endif

#include "state_save_restore/State_Save_Utils.hpp"
#include "state_save_restore/State_Save_Restore.hpp"
#include "parallel_utils.h"

namespace {
    const auto NGEN_UNIT_NAME = "ngen";
    const auto TROUTE_UNIT_NAME = "troute";
}

NgenSimulation::NgenSimulation(
    Simulation_Time const& sim_time,
    std::vector<std::shared_ptr<ngen::Layer>> layers,
    std::unordered_map<std::string, int> catchment_indexes,
    std::unordered_map<std::string, int> nexus_indexes,
    int mpi_rank,
    int mpi_num_procs
                               )
    : simulation_step_(0)
    , sim_time_(std::make_shared<Simulation_Time>(sim_time))
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
        catchment_outflows_.resize(catchment_outflows_.size() + catchment_indexes_.size(), 0.0);
        nexus_downstream_flows_.resize(nexus_downstream_flows_.size() + nexus_indexes_.size(), 0.0);

        advance_models_one_output_step();

        if (simulation_step_ + 1 < num_times) {
            sim_time_->advance_timestep();
        }
    }
}

void NgenSimulation::finalize() {
#if NGEN_WITH_ROUTING
    if (this->py_troute_) {
        this->py_troute_->Finalize();
        this->py_troute_.reset();
    }
#endif // NGEN_WITH_ROUTING
}

void NgenSimulation::advance_models_one_output_step()
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
    auto next_time = sim_time_->next_timestep_epoch_time();

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

void NgenSimulation::save_state_snapshot(std::shared_ptr<State_Snapshot_Saver> snapshot_saver)
{
    // TODO: save the current nexus data
    auto unit_name = this->unit_name();
    // XXX Handle self, then recursively pass responsibility to Layers
    for (auto& layer : layers_) {
        layer->save_state_snapshot(snapshot_saver);
    }
}

void NgenSimulation::save_end_of_run(std::shared_ptr<State_Snapshot_Saver> snapshot_saver)
{
    for (auto& layer : layers_) {
        layer->save_state_snapshot(snapshot_saver);
    }
#if NGEN_WITH_ROUTING
    if (this->mpi_rank_ == 0 && this->py_troute_) {
        uint64_t serialization_size;
        this->py_troute_->SetValue(StateSaveNames::CREATE, &serialization_size);
        this->py_troute_->GetValue(StateSaveNames::SIZE, &serialization_size);
        void *troute_state = this->py_troute_->GetValuePtr(StateSaveNames::STATE);
        boost::span<const char> span(static_cast<const char*>(troute_state), serialization_size);
        snapshot_saver->save_unit(TROUTE_UNIT_NAME, span);
        this->py_troute_->SetValue(StateSaveNames::FREE, &serialization_size);
    }
#endif // NGEN_WITH_ROUTING
}

void NgenSimulation::load_state_snapshot(std::shared_ptr<State_Snapshot_Loader> snapshot_loader) {
    // TODO: load the state data related to nexus outflows
    auto unit_name = this->unit_name();
    for (auto& layer : layers_) {
        layer->load_state_snapshot(snapshot_loader);
    }
}

void NgenSimulation::load_hot_start(std::shared_ptr<State_Snapshot_Loader> snapshot_loader, const std::string &t_route_config_file_with_path) {
    for (auto& layer : layers_) {
        layer->load_hot_start(snapshot_loader);
    }
#if NGEN_WITH_ROUTING
    if (this->mpi_rank_ == 0) {
        bool config_file_set = !t_route_config_file_with_path.empty();
        bool snapshot_exists = snapshot_loader->has_unit(TROUTE_UNIT_NAME);
        if (config_file_set && snapshot_exists) {
            LOG(LogLevel::DEBUG, "Loading T-Route data from snapshot.");
            std::vector<char> troute_data;
            snapshot_loader->load_unit(TROUTE_UNIT_NAME, troute_data);
            if (py_troute_ == NULL) {
                this->make_troute(t_route_config_file_with_path);
            }
            py_troute_->set_value_unchecked(StateSaveNames::STATE, troute_data.data(), troute_data.size());
            double rt; // unused by the BMI but needed for messaging
            py_troute_->SetValue(StateSaveNames::RESET, &rt);
        } else if (!config_file_set && !snapshot_exists) {
            LOG(LogLevel::DEBUG, "No data set for loading T-Route.");
        } else if (config_file_set && !snapshot_exists) {
            LOG(LogLevel::WARNING, "A T-Route config file was provided but the load data does not contain T-Route data. T-Route will be run as a cold start.");
        } else if (!config_file_set && snapshot_exists) {
            LOG(LogLevel::WARNING, "A T-Route hot start snapshot exists but no config file was provided. T-Route will not be loaded or run,");
        }
    }
#endif // NGEN_WITH_ROUTING
}


void NgenSimulation::make_troute(const std::string &t_route_config_file_with_path) {
#if NGEN_WITH_ROUTING
    this->py_troute_ = std::make_unique<models::bmi::Bmi_Py_Adapter>(
        "T-Route",
        t_route_config_file_with_path,
        "troute_nwm_bmi.troute_bmi.BmiTroute",
        true
    );
#endif // NGEN_WITH_ROUTING
}


std::string NgenSimulation::unit_name() const {
#if NGEN_WITH_MPI
    return "ngen_" + std::to_string(this->mpi_rank_);
#else
    return "ngen_0";
#endif // NGEN_WITH_MPI
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

void NgenSimulation::set_troute_inputs(
    const std::vector<double> *simulation_values,
    const std::unordered_map<std::string, int> *feature_indexes,
    const std::string id_var_name,
    const std::string value_var_name,
    const NgenSimulation::hy_features_t &features
) {
#if NGEN_WITH_MPI
    std::vector<double> all_values;
    std::unordered_map<std::string, int> all_indexes;
    if (this->mpi_num_procs_ > 1) {
        size_t number_of_timesteps = sim_time_->get_total_output_times();
        // create a list of local IDs
        std::vector<std::string> local_ids;
        for (const auto& id_pair : *feature_indexes) {
            local_ids.push_back(id_pair.first);
        }
        // MPI_Gather all IDs into a single vector
        std::vector<std::string> all_ids = parallel::gather_strings(local_ids, mpi_rank_, mpi_num_procs_);
        if (mpi_rank_ == 0) {
            // filter to only the unique IDs
            std::sort(all_ids.begin(), all_ids.end());
            all_ids.erase(
                std::unique(all_ids.begin(), all_ids.end()),
                all_ids.end()
            );
        }
        // MPI_Broadcast so all processes share the IDs
        all_ids = std::move(parallel::broadcast_strings(all_ids, mpi_rank_, mpi_num_procs_));

        // MPI_Reduce to collect the results from processes
        if (this->mpi_rank_ == 0) {
            all_values.resize(number_of_timesteps * all_ids.size(), 0.0);
        }
        std::vector<double> local_buffer(number_of_timesteps);
        std::vector<double> receive_buffer(number_of_timesteps, 0.0);
        for (int i = 0; i < all_ids.size(); ++i) {
            std::string current_id = all_ids[i];
            if (feature_indexes->find(current_id) != feature_indexes->end() && !features.is_remote_sender_nexus(current_id)) {
                // if this process has the id and receives/records data, copy the values to the buffer
                int id_index = feature_indexes->at(current_id);
                for (int step = 0; step < number_of_timesteps; ++step) {
                    int offset = step * feature_indexes->size() + id_index;
                    local_buffer[step] = simulation_values->at(offset);
                }
            } else {
                // if this process does not have the id, fill with 0 to make sure it doesn't affect reduce sum
                std::fill(local_buffer.begin(), local_buffer.end(), 0.0);
            }
            MPI_Reduce(local_buffer.data(), receive_buffer.data(), number_of_timesteps, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
            if (this->mpi_rank_ == 0) {
                // copy reduce values to a combined values vector
                all_indexes[current_id] = i;
                for (int step = 0; step < number_of_timesteps; ++step) {
                    int offset = step * all_ids.size() + i;
                    all_values[offset] = receive_buffer[step];
                    receive_buffer[step] = 0.0;
                }
            }
        }

        if (this->mpi_rank_ == 0) {
            // change rank 0's indexes and values to the MPI merged results for setting below
            simulation_values = &all_values;
            feature_indexes = &all_indexes;
        }
    }
#endif // NGEN_WITH_MPI
#if NGEN_WITH_ROUTING
    if (this->mpi_rank_ == 0) {
        // set up nexus id indexes
        std::vector<int> df_index(feature_indexes->size());
        for (const auto& key_value : *feature_indexes) {
            int id_index = key_value.second;

            // Convert string ID into numbers for T-route index
            int id_as_int = -1;
            size_t sep_index = key_value.first.find(hy_features::identifiers::separator);
            if (sep_index != std::string::npos) {
                std::string numbers = key_value.first.substr(sep_index + hy_features::identifiers::separator.length());
                id_as_int = std::stoi(numbers);
            }
            if (id_as_int == -1) {
                std::string error_msg = "Cannot convert the ID to an integer: " + key_value.first;
                LOG(LogLevel::FATAL, error_msg);
                throw std::runtime_error(error_msg);
            }
            df_index[id_index] = id_as_int;
        }
        // use unchecked messaging to allow the BMI to change its container size
        py_troute_->set_value_unchecked(id_var_name, df_index.data(), df_index.size());
        py_troute_->set_value_unchecked(value_var_name, simulation_values->data(), simulation_values->size());
    }
#endif // NGEN_WITH_ROUTING
}

void NgenSimulation::run_routing(NgenSimulation::hy_features_t &features, std::string const& t_route_config_file_with_path)
{
#if NGEN_WITH_ROUTING
    size_t number_of_timesteps = sim_time_->get_total_output_times();
    if (nexus_downstream_flows_.size() != number_of_timesteps * nexus_indexes_.size()) {
        std::string msg = "Routing input data in NgenSimulation::nexus_downstream_flows_ does not reflect a full-duration run";
        LOG(msg, LogLevel::FATAL);
        throw std::runtime_error(msg);
    }
    if (mpi_rank_ == 0) { // Run t-route from single process
        LOG(LogLevel::INFO, "Running T-Route on simulation outputs.");

        // Note: Currently, delta_time is set in the t-route yaml configuration file, and the
        // number_of_timesteps is determined from the total number of nexus outputs in t-route.
        // It is recommended to still pass these values to the routing_py_adapter object in
        // case a future implementation needs these two values from the ngen framework.
        int delta_time = sim_time_->get_output_interval_seconds();

        // model for routing
        if (this->py_troute_ == NULL) {
            this->make_troute(t_route_config_file_with_path);
        }
        this->py_troute_->set_value_unchecked("ngen_dt", &delta_time, 1);
    }
    // set the inputs from catchment and nexus results
    this->set_troute_inputs(
        &this->nexus_downstream_flows_,
        &this->nexus_indexes_,
        "land_surface_water_source__id",
        "land_surface_water_source__volume_flow_rate",
        features
    );
    this->set_troute_inputs(
        &this->catchment_outflows_,
        &this->catchment_indexes_,
        "catchment_water_source__id",
        "catchment_water_source__volume_flow_rate",
        features
    );
    if (this->mpi_rank_ == 0)
        this->py_troute_->Update();
#endif // NGEN_WITH_ROUTING
}

size_t NgenSimulation::get_num_output_times() const
{
    return sim_time_->get_total_output_times();
}

std::string NgenSimulation::get_timestamp_for_step(int step) const
{
    return sim_time_->get_timestamp(step);
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
