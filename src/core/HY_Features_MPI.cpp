#include <HY_Features_MPI.hpp>
#include <HY_PointHydroNexusRemote.hpp>
#include <network.hpp>

#ifdef NGEN_MPI_ACTIVE

using namespace hy_features;

HY_Features_MPI::HY_Features_MPI( PartitionData partition_data, geojson::GeoJSON linked_hydro_fabric, std::shared_ptr<Formulation_Manager> formulations, int mpi_rank, int mpi_num_procs) :
    HY_Features(network::Network(linked_hydro_fabric), formulations, linked_hydro_fabric), mpi_rank(mpi_rank), mpi_num_procs(mpi_num_procs)
{ 
      std::string feat_id;
      std::string feat_type;
      std::vector<std::string> origins, destinations;
      
      std::unordered_map<std::string, HY_PointHydroNexusRemote::catcment_location_map_t> remote_connections;
      using DirectionMap = std::unordered_map<std::string, std::map<std::string, std::string> >;
      DirectionMap remote_connection_direction;

      // loop through the partiton data remote arrays and make a map of catchment location maps
      for( int i = 0; i < partition_data.remote_connections.size(); ++i )
      {
        std::tuple<int, std::string, std::string, std::string> remote_tuple = (partition_data.remote_connections)[i];
        int remote_mpi_ranks = std::get<0>(remote_tuple);
        std::string remote_nexi = std::get<1>(remote_tuple);
        std::string remote_catchments = std::get<2>(remote_tuple);
        remote_connections[remote_nexi][remote_catchments] = remote_mpi_ranks;
        remote_connection_direction[remote_nexi][remote_catchments] = std::get<3>(remote_tuple);
      }

      //catchment and nexus objects already exist for the partitioned data
      //we simply need to replace the nexuses which require remote interactions
      //with the approriate remote nexus
      //origins only contains LOCAL origin features (catchments) as read from
      //the geojson/partition subset.  We need to make sure `origins` passed to remote nexus
      //contain IDS of ALL upstream features, including those in remote partitions
      //The same applies to destinations as well.
      //Find all remote catchments related to each remote feature
      for(const auto& kv : remote_connection_direction)
      { 
        auto feat_id = kv.first;
        for(const auto& catchment_direction : kv.second ){
          destinations  = network.get_destination_ids(feat_id);
          origins = network.get_origination_ids(feat_id);
          //Determine how it is related to the nexus.  This helps determine which side is a "sender"
          //and which side is "receiver"
          if(catchment_direction.second == "nex-to-dest_cat"){
            destinations.push_back(catchment_direction.first);
          }else if( catchment_direction.second == "orig_cat-to-nex" )
          {
            origins.push_back(catchment_direction.first);
          }
          _nexuses[feat_id] = std::make_unique<HY_PointHydroNexusRemote>(feat_id, destinations, origins, remote_connections[feat_id]) ;
        }
      }
      
}
#endif //NGEN_MPI_ACTIVE
