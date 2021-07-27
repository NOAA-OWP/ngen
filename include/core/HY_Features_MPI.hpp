#ifndef HY_FEATURES_MPI_H
#define HY_FEATURES_MPI_H

#include <unordered_map>

#include <HY_Catchment.hpp>
#include <HY_HydroNexus.hpp>
#include <network.hpp>
#include <Formulation_Manager.hpp>
#include <Partition_Parser.hpp>

namespace hy_features {

    class HY_Features_MPI {
      public:
      
      using Formulation_Manager = realization::Formulation_Manager;
      
        HY_Features_MPI( PartitionData partition_data,geojson::GeoJSON linked_hydro_fabric, std::shared_ptr<Formulation_Manager> formulations, int mpi_rank, int mpi_num_procs);
        
      private:
      
      std::unordered_map<std::string, std::shared_ptr<HY_Catchment>> _catchments;
      std::unordered_map<std::string, std::shared_ptr<HY_HydroNexus>> _nexuses;
      network::Network network;
      std::shared_ptr<Formulation_Manager> formulations;
      int mpi_rank;
      int mpi_num_procs;

    };
} 

#endif //HY_GRAPH_H
