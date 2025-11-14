#include <gtest/gtest.h>

#include <NgenSimulation.hpp>

//! Test that an instance of NgenSimulation can be created.
TEST(NgenSimulation_Test, Construction)
{
    simulation_time_params time_params("2025-10-15 08:00:00", "2025-11-13 15:00:00", 3600);
    Simulation_Time sim_time(time_params);
    std::vector<std::shared_ptr<ngen::Layer>> layers;
    std::unordered_map<std::string, int> catchment_indexes;
    std::unordered_map<std::string, int> nexus_indexes;
    int mpi_rank = 0;
    int mpi_num_procs = 1;

    std::unique_ptr<NgenSimulation> simulation{
        new NgenSimulation(
                           sim_time,
                           layers,
                           catchment_indexes,
                           nexus_indexes,
                           mpi_rank,
                           mpi_num_procs
                           )};
}
