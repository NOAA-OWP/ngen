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

      // loop through the partiton data remote arrays and make a map of catchment location maps
      for( int i = 0; i < partition_data.remote_nexi.size(); ++i )
      {
        remote_connections[partition_data.remote_nexi[i]][partition_data.remote_catchments[i]] = partition_data.remote_mpi_ranks[i];
      }

      for(const auto& feat_idx : network){
        feat_id = network.get_id(feat_idx);//feature->get_id();
        feat_type = feat_id.substr(0, 3);

        destinations  = network.get_destination_ids(feat_id);
        if(feat_type == "cat")
        {
          //Find and prepare formulation
          auto formulation = formulations->get_formulation(feat_id);
          formulation->set_output_stream(feat_id+".csv");
          // TODO: add command line or config option to have this be omitted
          //FIXME why isn't default param working here??? get_output_header_line() fails.
          formulation->write_output("Time Step,""Time,"+formulation->get_output_header_line(",")+"\n");
          //Find upstream nexus ids
          origins = network.get_origination_ids(feat_id);
          //Create the HY_Catchment with the formulation realization
          std::shared_ptr<HY_Catchment> c = std::make_shared<HY_Catchment>(
              HY_Catchment(feat_id, origins, destinations, formulation)
            );

          _catchments.emplace(feat_id, c);
        }
        else if(feat_type == "nex")
        {
            
            
            _nexuses.emplace(feat_id, std::make_unique<HY_PointHydroNexusRemote>(feat_id, destinations, remote_connections[feat_id]) );
        }
        else
        {
          std::cerr<<"HY_Features::HY_Features unknown feature identifier type "<<feat_type<<" for feature id."<<feat_id
                   <<" Skipping feature"<<std::endl;
        }
      }	
}
#endif //NGEN_MPI_ACTIVE