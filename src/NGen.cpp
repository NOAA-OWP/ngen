#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

#include "realizations/catchment/Formulation_Manager.hpp"
#include <Catchment_Formulation.hpp>
#include <HY_Features.hpp>

#include "NGenConfig.h"
#include "tshirt_params.h"

#include <FileChecker.h>
#include <boost/algorithm/string.hpp>

#ifdef ACTIVATE_PYTHON
#include <pybind11/embed.h>
namespace py = pybind11;
#endif // ACTIVATE_PYTHON
    
#ifdef NGEN_ROUTING_ACTIVE
#include "routing/Routing_Py_Adapter.hpp"
#endif // NGEN_ROUTING_ACTIVE

std::string catchmentDataFile = "";
std::string nexusDataFile = "";
std::string REALIZATION_CONFIG_PATH = "";

#ifdef NGEN_MPI_ACTIVE
#include <mpi.h>
#include "core/Partition_Parser.hpp"
#include <HY_Features_MPI.hpp>

std::string PARTITION_PATH = "";
int mpi_rank;
int mpi_num_procs;
#endif

std::unordered_map<std::string, std::ofstream> nexus_outfiles;

//Note: Use below if developing in-memory transfer of nexus flows to routing
//std::unordered_map<std::string, std::vector<double>> nexus_flows;

pdm03_struct get_et_params() {
  // create the struct used for ET
    pdm03_struct pdm_et_data;
    pdm_et_data.scaled_distribution_fn_shape_parameter = 1.3;
    pdm_et_data.vegetation_adjustment = 0.99;
    pdm_et_data.model_time_step = 0.0;
    pdm_et_data.max_height_soil_moisture_storerage_tank = 400.0;
    pdm_et_data.maximum_combined_contents = pdm_et_data.max_height_soil_moisture_storerage_tank /
                                            (1.0 + pdm_et_data.scaled_distribution_fn_shape_parameter);
    return pdm_et_data;
}

int main(int argc, char *argv[]) {
    std::cout << "Hello there " << ngen_VERSION_MAJOR << "."
              << ngen_VERSION_MINOR << "."
              << ngen_VERSION_PATCH << std::endl;
    std::ios::sync_with_stdio(false);


    //Pull a few "options" form the cli input, this is a temporary solution to CLI parsing!
    //Use "positional args"
    //arg 0 is program name
    //arg 1 is catchment_data file path
    //arg 2 is catchment subset ids, comma seperated string of ids (no spaces!), "" for all
    //arg 3 is nexus_data file path
    //arg 4 is nexus subset ids, comma seperated string of ids (no spaces!), "" for all
    //arg 5 is realization config path
    //arg 7 is the partion file path

    std::vector<string> catchment_subset_ids;
    std::vector<string> nexus_subset_ids;

    if( argc < 6) {
        std::cout << "Missing required args:" << std::endl;
        std::cout << argv[0] << " <catchment_data_path> <catchment subset ids> <nexus_data_path> <nexus subset ids>"
                  << " <realization_config_path>" << std::endl;
        if(argc > 3) {
            std::cout << std::endl << "Note:" << std::endl
                      << "Arguments for <catchment subset ids> and <nexus subset ids> must be given." << std::endl
                      << "Use empty string (\"\") as explicit argument when no subset is needed." << std::endl;
        }
        exit(-1);
    }
    else {
      bool error = false;

      catchmentDataFile = argv[1];
      nexusDataFile = argv[3];

  #ifdef NGEN_MPI_ACTIVE
      //initalize mpi
      MPI_Init(NULL, NULL);
      MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
      MPI_Comm_size(MPI_COMM_WORLD, &mpi_num_procs);
      catchmentDataFile += "." + std::to_string(mpi_rank);
      nexusDataFile += "." + std::to_string(mpi_rank);
  #endif

      if( !utils::FileChecker::file_is_readable(catchmentDataFile) ) {
        std::cout<<"catchment data path "<<catchmentDataFile<<" not readable"<<std::endl;
        error = true;
      }

      if( !utils::FileChecker::file_is_readable(nexusDataFile) ) {
        std::cout<<"nexus data path "<<nexusDataFile<<" not readable"<<std::endl;
        error = true;
      }

      if( !utils::FileChecker::file_is_readable(argv[5]) ) {
        std::cout<<"realization config path "<<argv[5]<<" not readable"<<std::endl;
        error = true;
      }
      else { REALIZATION_CONFIG_PATH = argv[5]; }

  #ifdef NGEN_MPI_ACTIVE
      if ( argc >= 7 ) {
        if ( !utils::FileChecker::file_is_readable(argv[6]) ) {
          std::cout<<"partion path "<<argv[6]<<" not readable"<<std::endl;
          error = true;
        }
        else { PARTITION_PATH = argv[6]; }
      }
      else {
        std::cout << "Missing required arguement partition file path.";
      }
  #endif

      if(error) exit(-1);

      //split the subset strings into vectors
      boost::split(catchment_subset_ids, argv[2], [](char c){return c == ','; } );
      boost::split(nexus_subset_ids, argv[4], [](char c){return c == ','; } );
      //If a single id or no id is passed, the subset vector will have size 1 and be the id or the ""
      //if we get an empy string, pop it from the subset list.
      if(nexus_subset_ids.size() == 1 && nexus_subset_ids[0] == "") nexus_subset_ids.pop_back();
      if(catchment_subset_ids.size() == 1 && catchment_subset_ids[0] == "") catchment_subset_ids.pop_back();
    } // end else if (argc < 6)

  #ifdef ACTIVATE_PYTHON
    // Start Python interpreter and keep it alive
    py::scoped_interpreter guard{};
  #endif // ACTIVATE_PYTHON

    //Read the collection of nexus
    std::cout << "Building Nexus collection" << std::endl;
    
  #ifdef NGEN_MPI_ACTIVE
    Partitions_Parser partition_parser(PARTITION_PATH);
    partition_parser.parse_partition_file();
    
    auto& partitions = partition_parser.partition_ranks;  
    auto& local_data = partitions[std::to_string(mpi_rank)];   
    nexus_subset_ids = local_data.nex_ids;
    catchment_subset_ids = local_data.cat_ids; 
  #endif

    // TODO: Instead of iterating through a collection of FeatureBase objects mapping to nexi, we instead want to iterate through HY_HydroLocation objects
    geojson::GeoJSON nexus_collection = geojson::read(nexusDataFile, nexus_subset_ids);
    std::cout << "Building Catchment collection" << std::endl;

    // TODO: Instead of iterating through a collection of FeatureBase objects mapping to catchments, we instead want to iterate through HY_Catchment objects
    geojson::GeoJSON catchment_collection = geojson::read(catchmentDataFile, catchment_subset_ids);
    
    for(auto& feature: *catchment_collection)
    {
        //feature->set_id(feature->get_property("ID").as_string());
        nexus_collection->add_feature(feature);
        //std::cout<<"Catchment "<<feature->get_id()<<" -> Nexus "<<feature->get_property("toID").as_string()<<std::endl;
    }

    std::shared_ptr<realization::Formulation_Manager> manager = std::make_shared<realization::Formulation_Manager>(REALIZATION_CONFIG_PATH);
    manager->read(catchment_collection, utils::getStdOut());
    std::string link_key = "toid";
  #ifdef NGEN_MPI_ACTIVE
    nexus_collection->link_features_from_property(nullptr, &link_key);
    hy_features::HY_Features_MPI features = hy_features::HY_Features_MPI(local_data, nexus_collection, manager, mpi_rank, mpi_num_procs);
  #else
    hy_features::HY_Features features = hy_features::HY_Features(catchment_collection, &link_key, manager);
  #endif

    //validate dendridic connections
    features.validate_dendridic();
    //TODO don't really need catchment_collection once catchments are added to nexus collection
    //Still using  catchments for geometry at the moment, fix this later
    //catchment_collection.reset();
    nexus_collection.reset();

    //Still hacking nexus output  for the moment
    for(const auto& id : features.nexuses()) {
  #ifdef NGEN_MPI_ACTIVE
      //if (local_data.nexus_at(id).is_remote_sender() == false )
      if (!features.is_remote_sender_nexus(id)) {
        nexus_outfiles[id].open("./"+id+"_output.csv", std::ios::trunc);
      }
  #else 
      nexus_outfiles[id].open("./"+id+"_output.csv", std::ios::trunc);
  #endif
    }

    std::cout<<"Running Models"<<std::endl;

    std::shared_ptr<pdm03_struct> pdm_et_data = std::make_shared<pdm03_struct>(get_et_params());

    //Now loop some time, iterate catchments, do stuff for total number of output times
    for(int output_time_index = 0; output_time_index < manager->Simulation_Time_Object->get_total_output_times(); output_time_index++) {
      //std::cout<<"Output Time Index: "<<output_time_index<<std::endl;
      if(output_time_index%100 == 0) std::cout<<"Running timestep "<<output_time_index<<std::endl;
      std::string current_timestamp = manager->Simulation_Time_Object->get_timestamp(output_time_index);
      for(const auto& id : features.catchments()) {
        //std::cout<<"Running cat "<<id<<std::endl;
        auto r = features.catchment_at(id);
        //TODO redesign to avoid this cast
        auto r_c = dynamic_pointer_cast<realization::Catchment_Formulation>(r);
        r_c->set_et_params(pdm_et_data);
        double response = r_c->get_response(output_time_index, 3600.0);
        std::string output = std::to_string(output_time_index)+","+current_timestamp+","+
                             r_c->get_output_line_for_timestep(output_time_index)+"\n";
        r_c->write_output(output);
        //TODO put this somewhere else.  For now, just trying to ensure we get m^3/s into nexus output
        response *= (catchment_collection->get_feature(id)->get_property("areasqkm").as_real_number() * 1000000);
        //update the nexus with this flow
        for(auto& nexus : features.destination_nexuses(id)) {
          //TODO in a DENDRIDIC network, only one destination nexus per catchment
          //If there is more than one, some form of catchment partitioning will be required.
          //for now, only contribute to the first one in the list
          nexus->add_upstream_flow(response, id, output_time_index);
	        break;
        }
      } //done catchments
      //At this point, could make an internal routing pass, extracting flows from nexuses and routing
      //across the flowpath to the next nexus.
      //Once everything is updated for this timestep, dump the nexus output
      for(const auto& id : features.nexuses()) {
  #ifdef NGEN_MPI_ACTIVE
        if (!features.is_remote_sender_nexus(id)) { //Ensures only one side of the dual sided remote nexus actually doing this...
  #endif
          //Get the correct "requesting" id for downstream_flow
	        const auto& nexus = features.nexus_at(id);
          const auto& cat_ids = nexus->get_receiving_catchments();
          std::string cat_id;
          if( cat_ids.size() > 0 ) {
            //Assumes dendridic, e.g. only a single downstream...it will consume 100%  of the available flow
            cat_id = cat_ids[0];
          }
          else {
            //This is a terminal node, SHOULDN'T be remote, so ID shouldn't matter too much
            cat_id = "terminal";
          }
          double contribution_at_t = features.nexus_at(id)->get_downstream_flow(cat_id, output_time_index, 100.0);
          if(nexus_outfiles[id].is_open()) {
            nexus_outfiles[id] << output_time_index << ", " << current_timestamp << ", " << contribution_at_t << std::endl;
          }
  #ifdef NGEN_MPI_ACTIVE
        }
  #endif
        //std::cout<<"\tNexus "<<id<<" has "<<contribution_at_t<<" m^3/s"<<std::endl;

        //Note: Use below if developing in-memory transfer of nexus flows to routing
        //If using below, then another single time vector would be needed to hold the timestamp
        //nexus_flows[id].push_back(contribution_at_t); 
      } //done nexuses
    } //done time
    std::cout<<"Finished "<<manager->Simulation_Time_Object->get_total_output_times()<<" timesteps."<<std::endl;


  #ifdef NGEN_ROUTING_ACTIVE
    
    //Syncronization here. MPI barrier. If rank == 0, do routing

    if(manager->get_using_routing()) {
      std::cout<<"Using Routing"<<std::endl;

      std::string t_route_connection_path = manager->get_t_route_connection_path();
      
      std::string input_path = manager->get_input_path();
   
      int number_of_timesteps = manager->Simulation_Time_Object->get_total_output_times();

      int delta_time = manager->Simulation_Time_Object->get_output_interval_seconds();
 
      routing_py_adapter::Routing_Py_Adapter routing_py_adapter1(t_route_connection_path, input_path, catchment_subset_ids, number_of_timesteps, delta_time);
    }
    else {
      std::cout<<"Not Using Routing"<<std::endl;
    }

  #endif // NGEN_ROUTING_ACTIVE

  #ifdef NGEN_MPI_ACTIVE
    MPI_Finalize();
  #endif
}
