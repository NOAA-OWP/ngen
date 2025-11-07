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

    void run_catchments();
    void run_routing(hy_features_t &features, std::string const& t_route_config_file_with_path);

    int get_nexus_index(std::string const& nexus_id) const;
    double get_nexus_outflow(int nexus_index, int timestep_index) const;

    size_t get_num_output_times() const;
    std::string get_timestamp_for_step(int step) const;

private:
    void advance_models_one_output_step();

    int simulation_step_;

    std::shared_ptr<Simulation_Time> sim_time_;

    // Catchment data
    std::vector<std::shared_ptr<ngen::Layer>> layers_;

    // Routing data structured for t-route
    std::unordered_map<std::string, int> catchment_indexes_;
    std::vector<double> catchment_outflows_;
    std::unordered_map<std::string, int> nexus_indexes_;
    std::vector<double> nexus_downstream_flows_;

    int mpi_rank_;
    int mpi_num_procs_;

    // Serialization template will be defined and instantiated in the .cpp file
    template <class Archive>
    void serialize(Archive& ar);
};

#endif
