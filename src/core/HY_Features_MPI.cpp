#include <HY_Features_MPI.hpp>
#include <HY_PointHydroNexusRemote.hpp>

#ifdef NGEN_MPI_ACTIVE

using namespace hy_features;

HY_Features_MPI::HY_Features_MPI( PartitionData partition_data, geojson::GeoJSON linked_hydro_fabric, std::shared_ptr<Formulation_Manager> formulations, int mpi_rank, int mpi_num_procs) :
      network(linked_hydro_fabric), formulations(formulations), mpi_rank(mpi_rank), mpi_num_procs(mpi_num_procs)
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

      for(const auto& feat_idx : network){
        feat_id = network.get_id(feat_idx);//feature->get_id();
        feat_type = feat_id.substr(0, 3);

        destinations  = network.get_destination_ids(feat_id);
        //Find upstream ids
        origins = network.get_origination_ids(feat_id);
        if(feat_type == "cat" || feat_type == "agg")
        {
          //Find and prepare formulation
          auto formulation = formulations->get_formulation(feat_id);
          formulation->set_output_stream(formulations->get_output_root() + feat_id + ".csv");
          // TODO: add command line or config option to have this be omitted
          //FIXME why isn't default param working here??? get_output_header_line() fails.
          formulation->write_output("Time Step,""Time,"+formulation->get_output_header_line(",")+"\n");
          
          // get the catchment level from the hydro fabric
          const auto& cat_json_node = linked_hydro_fabric->get_feature(feat_id);
          long lv = cat_json_node->has_key("level") ? cat_json_node->get_property("level").as_natural_number() : 0;

          // add this level to the set of levels if needed
          if (hf_levels.find(lv) == hf_levels.end() )
          {
              hf_levels.insert(lv);
          }

          //Create the HY_Catchment with the formulation realization
          std::shared_ptr<HY_Catchment> c = std::make_shared<HY_Catchment>(
              HY_Catchment(feat_id, origins, destinations, formulation, lv)
            );

          _catchments.emplace(feat_id, c);
        }
        else if(feat_type == "nex" || feat_type == "tnx")
        {   //origins only contains LOCAL origin features (catchments) as read from
            //the geojson/partition subset.  We need to make sure `origins` passed to remote nexus
            //contain IDS of ALL upstream features, including those in remote partitions
            //The same applies to destinations as well.
            //Find all remote catchments related to this feature
            for(auto& catchment_direction : remote_connection_direction[feat_id])
            { //Determine how it is related to the nexus.  This helps determine which side is a "sender"
              //and which side is "receiver"
              if(catchment_direction.second == "nex-to-dest_cat"){
                destinations.push_back(catchment_direction.first);
              }else if( catchment_direction.second == "orig_cat-to-nex" )
              {
                origins.push_back(catchment_direction.first);
              }
            }
            _nexuses.emplace(feat_id, std::make_unique<HY_PointHydroNexusRemote>(feat_id, destinations, origins, remote_connections[feat_id]) );
        }
        else
        {
          std::cerr<<"HY_Features::HY_Features unknown feature identifier type "<<feat_type<<" for feature id."<<feat_id
                   <<" Skipping feature"<<std::endl;
        }
      }	
}
#endif //NGEN_MPI_ACTIVE
