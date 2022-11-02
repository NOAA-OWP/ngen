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

namespace hy_features {

    class HY_Features_MPI {
      public:
      
      using Formulation_Manager = realization::Formulation_Manager;
      
        HY_Features_MPI(PartitionData partition_data, geojson::GeoJSON linked_hydro_fabric,
                        std::shared_ptr<Formulation_Manager> formulations, int mpi_rank, int mpi_num_procs);

        std::shared_ptr<HY_CatchmentRealization> catchment_at(std::string id) {
            return (_catchments.find(id) != _catchments.end()) ? _catchments[id]->realization : nullptr;
        }

        inline auto catchments() {
            return network.filter("cat");
        }

        inline bool is_remote_sender_nexus(const std::string& id) {
            return _nexuses.find(id) != _nexuses.end() && _nexuses[id]->is_remote_sender();
        }
        
        inline auto catchments(long lv) {
            return network.filter("cat");
        }

        inline const auto& levels() {
            return hf_levels;
        }

        inline bool is_remote_sender_nexus(std::string id) {


        inline std::vector<std::shared_ptr<HY_HydroNexus>> destination_nexuses(const std::string& id) {
            std::vector<std::shared_ptr<HY_HydroNexus>> downstream;
            if (_catchments.find(id) != _catchments.end()) {
                for(const auto& nex_id : _catchments[id]->get_outflow_nexuses()) {
                    downstream.push_back(_nexuses[nex_id]);
                }
            }
            return downstream;
        }

        std::shared_ptr<HY_HydroNexus> nexus_at(const std::string& id) {
            return (_nexuses.find(id) != _nexuses.end()) ? _nexuses[id] : nullptr;
        }

        inline auto nexuses() {
            return network.filter("nex");
        }

        void validate_dendritic() {
            for(const auto& id : catchments()) {
                auto downstream = network.get_destination_ids(id);
                if(downstream.size() > 1) {
                    std::cerr << "Catchment " << id << " has more than one downstream connection." << std::endl;
                    std::cerr << "Downstreams are: ";
                    for(const auto& id : downstream){
                        std::cerr <<id<<" ";
                    }
                    std::cerr << std::endl;
                    assert( false );
                }
                else if (downstream.size() == 0) {
                    std::cerr << "Catchment " << id << " has 0 downstream connections, must have 1." << std::endl;
                    assert( false );
                }
            }
            std::cout<<"Catchment topology is dendritic."<<std::endl;
        }

      private:
      
      std::unordered_map<std::string, std::shared_ptr<HY_Catchment>> _catchments;
      std::unordered_map<std::string, std::shared_ptr<HY_PointHydroNexusRemote>> _nexuses;
      network::Network network;
      std::shared_ptr<Formulation_Manager> formulations;
      std::set<long> hf_levels;
      int mpi_rank;
      int mpi_num_procs;

    };
} 
#endif //NGEN_MPI_ACTIVE
#endif //HY_FEATURES_MPI_H
