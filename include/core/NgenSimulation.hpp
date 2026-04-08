#ifndef NGENSIMULATION_HPP
#define NGENSIMULATION_HPP

#include <NGenConfig.h>

#include <Simulation_Time.hpp>
#include <Layer.hpp>

namespace hy_features
{
    class HY_Features;
    class HY_Features_MPI;
}

class State_Snapshot_Saver;
class State_Snapshot_Loader;

#if NGEN_WITH_ROUTING
#include "bmi/Bmi_Py_Adapter.hpp"
#endif // NGEN_WITH_ROUTING

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

// Contains all of the dynamic state and logic to run a NextGen hydrologic simulation
class NgenSimulation
{
public:
    NgenSimulation(
                   Simulation_Time const& sim_time,
                   std::vector<std::shared_ptr<ngen::Layer>> layers,
                   std::unordered_map<std::string, int> catchment_indexes,
                   std::unordered_map<std::string, int> nexus_indexes,
                   int mpi_rank,
                   int mpi_num_procs
                   );
    NgenSimulation() = delete;

    ~NgenSimulation();

#if NGEN_WITH_MPI
    using hy_features_t = hy_features::HY_Features_MPI;
#else
    using hy_features_t = hy_features::HY_Features;
#endif

    /**
     * Run the catchment formulations for the full configured duration of the simulation
     *
     * Captures calculated runoff values in `catchment_outflows_` and
     * `nexus_downstream_flows_` for subsequent output and consumption
     * by `run_routing()`
     */
    void run_catchments();

    // Tear down of any items stored on the NgenSimulation object that could throw errors and, thus, should be kept separate from the deconstructor.
    void finalize();

    /**
     * Run t-route on the stored nexus outflow values for the full configured duration of the simulation
     */
    void run_routing(hy_features_t &features, std::string const& t_route_config_file_with_path);

    int get_nexus_index(std::string const& nexus_id) const;
    double get_nexus_outflow(int nexus_index, int timestep_index) const;

    size_t get_num_output_times() const;
    std::string get_timestamp_for_step(int step) const;

    void save_state_snapshot(std::shared_ptr<State_Snapshot_Saver> snapshot_saver);
    void load_state_snapshot(std::shared_ptr<State_Snapshot_Loader> snapshot_loader);
    /**
     * Saves a snapshot state that's intended to be run at the end of a simulation.
     * 
     * This version of saving will include T-Route BMI data and exclude the nexus outflow data stored during the catchment processing.
    */
    void save_end_of_run(std::shared_ptr<State_Snapshot_Saver> snapshot_saver);
    // Load a snapshot of the end of a previous run. This will create a T-Route python adapter if the loader finds a unit for it and the config path is not empty.
    void load_hot_start(std::shared_ptr<State_Snapshot_Loader> snapshot_loader, const std::string &t_route_config_file_with_path);

private:
    void advance_models_one_output_step();

    // Set T-route input that may require merging results from other MPI processes
    std::pair<std::vector<double>*, std::unordered_map<std::string, int>*> set_troute_inputs(
        const NgenSimulation::hy_features_t &features,
        const std::vector<double> *simulation_values,
        const std::unordered_map<std::string, int> *feature_indexes,
        const std::string id_var_name,
        const std::string value_var_name
    );

    int simulation_step_;

    std::shared_ptr<Simulation_Time> sim_time_;

    // Catchment data
    std::vector<std::shared_ptr<ngen::Layer>> layers_;

    // Routing data structured for t-route
    std::unordered_map<std::string, int> catchment_indexes_;
    std::vector<double> catchment_outflows_;
    std::unordered_map<std::string, int> nexus_indexes_;
    std::vector<double> nexus_downstream_flows_;
#if NGEN_WITH_ROUTING
    std::unique_ptr<models::bmi::Bmi_Py_Adapter> py_troute_;
#endif // NGEN_WITH_ROUTING
    void make_troute(const std::string &t_route_config_file_with_path);

    std::string unit_name() const;

    int mpi_rank_;
    int mpi_num_procs_;

    // Serialization template will be defined and instantiated in the .cpp file
    template <class Archive>
    void serialize(Archive& ar);
};

#endif
