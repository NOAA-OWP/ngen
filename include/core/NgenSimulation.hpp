#ifndef NGENSIMULATION_HPP
#define NGENSIMULATION_HPP

#include <Simulation_Time.hpp>
#include <realizations/catchment/Formulation_Manager.hpp>
#include <Layer.hpp>

namespace routing_py_adapter {
    class Routing_Py_Adapter;
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
                   std::shared_ptr<realization::Formulation_Manager> formulation_manager,
                   std::vector<std::shared_ptr<ngen::Layer>> layers,
                   std::unordered_map<std::string, int> catchment_indexes,
                   std::unordered_map<std::string, int> nexus_indexes,
                   int mpi_rank
                   );
    NgenSimulation() = delete;

    ~NgenSimulation();

    void run_catchments();
    void run_routing();
    void advance_one_output_step();

    int simulation_step_;

    std::shared_ptr<Simulation_Time> sim_time_;
    size_t get_num_output_times();

    // Catchment data
    std::shared_ptr<realization::Formulation_Manager> catchment_formulation_manager_;
    std::vector<std::shared_ptr<ngen::Layer>> layers_;

    // Routing data structured for t-route
    std::unordered_map<std::string, int> catchment_indexes_;
    std::vector<double> catchment_outflows_;
    std::unordered_map<std::string, int> nexus_indexes_;
    std::vector<double> nexus_downstream_flows_;

    int mpi_rank_;

#if NGEN_WITH_ROUTING
    std::unique_ptr<routing_py_adapter::Routing_Py_Adapter> router_;
#endif

    // Serialization template will be defined and instantiated in the .cpp file
    template <class Archive>
    void serialize(Archive& ar);
};

#endif
