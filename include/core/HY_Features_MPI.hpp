#ifndef HY_FEATURES_MPI_H
#define HY_FEATURES_MPI_H
//Only need this unit when using MPI.  It won't compile without other MPI dependent code.
#ifdef NGEN_MPI_ACTIVE

#include <unordered_map>
#include <set>

#include <HY_Catchment.hpp>
#include <HY_PointHydroNexusRemote.hpp>
#include <network.hpp>
#include <Formulation_Manager.hpp>
#include <Partition_Parser.hpp>
#include <HY_Features.hpp>

namespace hy_features {

    class HY_Features_MPI: public HY_Features {
      public:
      
      using Formulation_Manager = realization::Formulation_Manager;
      
        HY_Features_MPI(PartitionData partition_data, geojson::GeoJSON linked_hydro_fabric,
                        std::shared_ptr<Formulation_Manager> formulations, int mpi_rank, int mpi_num_procs);

        HY_Features_MPI() = delete;

        inline bool is_remote_sender_nexus(const std::string& id) {
            return _nexuses.find(id) != _nexuses.end() && _nexuses[id]->is_remote_sender();
        }

      private:
      
      std::unordered_map<std::string, std::shared_ptr<HY_Catchment>> _catchments;
      std::unordered_map<std::string, std::shared_ptr<HY_PointHydroNexusRemote>> _nexuses;

      int mpi_rank;
      int mpi_num_procs;

    };
} 
#endif //NGEN_MPI_ACTIVE
#endif //HY_FEATURES_MPI_H
