#ifdef NGEN_MPI_ACTIVE
  #include <mpi.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

#include <FeatureBuilder.hpp>
#include <FeatureVisitor.hpp>
#include "features/Features.hpp"

#include "realizations/catchment/Formulation_Manager.hpp"

#include <HY_HydroNexus.hpp>
#include <HY_Catchment.hpp>
#include <Formulation.hpp>
#include <HY_PointHydroNexus.hpp>

#include "NGenConfig.h"
#include "tshirt_params.h"

#include <FileChecker.h>
#include <boost/algorithm/string.hpp>

//new headers
#include "realizations/catchment/Catchment_Formulation.hpp"
//#include "hymod/include/Hymod.h"

#include "HY_PointHydroNexusRemoteUpstream.hpp"
#include "HY_PointHydroNexusRemoteDownstream.hpp"

std::string catchmentDataFile = "";
std::string nexusDataFile = "";
std::string REALIZATION_CONFIG_PATH = "";

//TODO this is possible, but ASSUMES realizations based on feature geom type, so not quite ready for prime time
class RealizaitonVisitor : public geojson::FeatureVisitor {

};

// TODO: Avoid having to call this in favor of having this being a core part of the creation of both the initial collections
void prepare_features(geojson::GeoJSON& nexus, geojson::GeoJSON& catchments, bool validate=false)
{
  for(auto& feature: *nexus){
    feature->set_id(feature->get_id());
    //std::cout << "Got Nexus Feature " << feature->get_id() << std::endl;
  }
  nexus->update_ids();
  //Now read the collection of catchments, iterate it and add them to the nexus collection
  //also link them by to->id
  //std::cout << "Iterating Catchment Features" << std::endl;
  for(auto& feature: *catchments){
    feature->set_id(feature->get_id());
    nexus->add_feature(feature);
    //std::cout<<"Catchment "<<feature->get_id()<<" -> Nexus "<<feature->get_property("toID").as_string()<<std::endl;
  }
  std::string linkage = "toid";
  nexus->link_features_from_property(nullptr, &linkage);

  if(validate){
    //Convience check on linkage
    for(auto& feature: *nexus){
      //feature->set_id(feature->get_property("ID").as_string());
      //std::cout << "Got Nexus Feature " << feature->get_id() << std::endl;
      if( feature->get_id().substr(0, 3) == "cat") {
        auto downstream = feature->destination_features();
        if(downstream.size() > 1) {
          std::cerr << "catchment " << feature->get_id() << " has more than one downstream connection" << std::endl;
        }
        else if(downstream.size() == 0) {
            std::cerr << "catchment " << feature->get_id() << " has NO downstream connection" << std::endl;
        }
        else {
          std::cout <<"catchment feature " << feature->get_id() << " to nexus feature " << downstream[0]->get_id() << std::endl;
        }
       }
     }//end for(feature: nexus)
   }//end if(validate)
}

std::unordered_map<std::string, std::unique_ptr<HY_HydroNexus>> nexus_realizations;
std::unordered_map<std::string, std::string> catchment_to_nexus;
std::unordered_map<std::string, std::string> nexus_to_catchment;
std::unordered_map<std::string, std::string> nexus_from_catchment;
std::unordered_map<std::string, std::vector<double>> output_map;

std::unordered_map<std::string, std::ofstream> catchment_outfiles;
std::unordered_map<std::string, std::ofstream> nexus_outfiles;

//TODO move catchment int identity to relization, and update nexus to use string id
std::unordered_map<std::string, int> catchment_id;


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

typedef Simple_Lumped_Model_Realization _hymod;
typedef realization::Tshirt_Realization _tshirt;

int main(int argc, char *argv[]) {
#ifdef NGEN_MPI_ACTIVE
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);
    // Find out rank
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    // Find out size
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    utils::StreamHandler output_stream = utils::StreamHandler();
#endif


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

          if( !utils::FileChecker::file_is_readable(argv[1]) ) {
            std::cout<<"catchment data path "<<argv[1]<<" not readable"<<std::endl;
            error = true;
          }
          else{ catchmentDataFile = argv[1]; }

          if( !utils::FileChecker::file_is_readable(argv[3]) ) {
            std::cout<<"nexus data path "<<argv[3]<<" not readable"<<std::endl;
            error = true;
          }
          else { nexusDataFile = argv[3]; }

          if( !utils::FileChecker::file_is_readable(argv[5]) ) {
            std::cout<<"realization config path "<<argv[5]<<" not readable"<<std::endl;
            error = true;
          }
          else{ REALIZATION_CONFIG_PATH = argv[5]; }

          if(error) exit(-1);
          //split the subset strings into vectors
          boost::split(catchment_subset_ids, argv[2], [](char c){return c == ','; } );
          boost::split(nexus_subset_ids, argv[4], [](char c){return c == ','; } );
          //If a single id or no id is passed, the subset vector will have size 1 and be the id or the ""
          //if we get an empy string, pop it from the subset list.
          if(nexus_subset_ids.size() == 1 && nexus_subset_ids[0] == "") nexus_subset_ids.pop_back();
          if(catchment_subset_ids.size() == 1 && catchment_subset_ids[0] == "") catchment_subset_ids.pop_back();
        }

    //Read the collection of nexus
    std::cout << "Building Nexus collection" << std::endl;

    // TODO: Instead of iterating through a collection of FeatureBase objects mapping to nexi, we instead want to iterate through HY_HydroLocation objects
    geojson::GeoJSON nexus_collection = geojson::read(nexusDataFile, nexus_subset_ids);
    std::cout << "Building Catchment collection" << std::endl;

    // TODO: Instead of iterating through a collection of FeatureBase objects mapping to catchments, we instead want to iterate through HY_Catchment objects
    geojson::GeoJSON catchment_collection = geojson::read(catchmentDataFile, catchment_subset_ids);

    //The modified prepare_features() seems to work in MPI environment
    prepare_features(nexus_collection, catchment_collection, !true);
    std::cout << "Finish prepare_features()" << std::endl;

    // TODO: Have these formulations attached to the prior HY_Catchment objects
    realization::Formulation_Manager manager = realization::Formulation_Manager(REALIZATION_CONFIG_PATH);
    manager.read(catchment_collection, utils::getStdOut());

    //TODO don't really need catchment_collection once catchments are added to nexus collection
    catchment_collection.reset();
    for(auto& feature : *nexus_collection)
    {
      std::string feat_id = feature->get_id();
      //FIXME rework how we use IDs to NOT force parsing???
      //Skipping IDs that aren't "real" i.e. have a  NA id
      if (feat_id.substr(4) == "NA") continue;

      // We need a better way to identify catchments vs nexi
      if( feat_id.substr(0, 3) == "cat" ){
        catchment_outfiles[feat_id].open(feature->get_id()+"_output.csv", std::ios::trunc);

        if(feature->get_number_of_destination_features() == 1)
        {
          catchment_to_nexus[feat_id] = feature->destination_features()[0]->get_id();
        }
        else
        {
          //TODO
        }
        catchment_id[feat_id] = std::stoi(feat_id.substr(4));
      }else{
        //Create nexus realization, add to map
        int num = std::stoi( feat_id.substr(4) );
        nexus_outfiles[feat_id].open("./"+feature->get_id()+"_output.csv", std::ios::trunc);

        nexus_realizations[feat_id] = std::make_unique<HY_PointHydroNexus>(
                                      HY_PointHydroNexus(num, feat_id,
                                                         feature->get_number_of_destination_features()));
       if(feature->get_number_of_destination_features() == 1)
       {
         nexus_to_catchment[feat_id] = feature->destination_features()[0]->get_id();
       }
       else if(feature->get_number_of_origination_features() == 1)
       {
         nexus_from_catchment[feat_id] = feature->origination_features()[0]->get_id();
       }
       else{
         //TODO
       }
       output_map[feat_id] = std::vector<double>();
      }

    }

#ifdef NGEN_MPI_ACTIVE
    // TODO For 2 catchments, need 2 focing files. For the moment, using the same forcing file
    forcing_params forcing_config(
                    "./data/forcing/cat-52_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                    "2015-12-01 00:00:00",
                    "2015-12-30 23:00:00"
                );

    double storage = 1.0;
    double max_storage = 1000.0;
    double a = 1.0;
    double b = 10.0;
    double Ks = 0.1;
    double Kq = 0.01;
    long n = 3;
    std::vector<double> Sr = {1.0, 1.0, 1.0};
    time_step_t t = 0;

    //MPI version
    //Using the if block causes variable undeclared error
    //if (world_rank == 0) {
      realization::Catchment_Formulation* A = new Simple_Lumped_Model_Realization(
            "Cat-52",
            forcing_config,
            output_stream,
            storage,
            max_storage,
            a,
            b,
            Ks,
            Kq,
            n,
            Sr,
            t);
    //} else if (world_rank == 1) {
      realization::Catchment_Formulation* B = new Simple_Lumped_Model_Realization(
            "Cat-52",
            forcing_config,
            output_stream,
            storage,
            max_storage,
            a,
            b,
            Ks,
            Kq,
            n,
            Sr,
            t);
    //}

    //Instantiate remote nexus class objects to use MPI
    //Using the if block causes variable undeclared error
    //if (world_rank == 0) {
      HY_PointHydroNexusRemoteUpstream nexus_upstream = HY_PointHydroNexusRemoteUpstream(34, "nex-34", 1);
    //} else if (world_rank == 1) {
      HY_PointHydroNexusRemoteDownstream nexus_downstream = HY_PointHydroNexusRemoteDownstream(34, "nex-34", 1);
    //}
#endif

    std::cout<<"Running Models"<<std::endl;

    std::shared_ptr<pdm03_struct> pdm_et_data = std::make_shared<pdm03_struct>(get_et_params());

    //Now loop some time, iterate catchments, do stuff for total number of output times
    for(int output_time_index = 0; output_time_index < manager.Simulation_Time_Object->get_total_output_times(); output_time_index++)
    {
      std::cout<<"Output Time Index: "<<output_time_index<<std::endl;

      std::string current_timestamp = manager.Simulation_Time_Object->get_timestamp(output_time_index);

#ifndef NGEN_MPI_ACTIVE
      for (std::pair<std::string, std::shared_ptr<realization::Formulation>> formulation_pair : manager ) {
        formulation_pair.second->set_et_params(pdm_et_data);
        //get the catchment response
        double response = formulation_pair.second->get_response(output_time_index, 3600.0);
        //dump the output
        std::cout<<"\tCatchment "<<formulation_pair.first<<" contributing "<<response<<" m/s to "<<catchment_to_nexus[formulation_pair.first]<<std::endl;
        // If the timestep is 0, also write the header line to the file
        // TODO: add command line or config option to have this be omitted

        if (output_time_index == 0) {
            // Append "Time Step" to header string provided by formulation, since we'll also add time step to output
            std::string header_str = formulation_pair.second->get_output_header_line();
            catchment_outfiles[formulation_pair.first] << "Time Step," << "Time," << header_str <<std::endl;
        }
        std::string output_str = formulation_pair.second->get_output_line_for_timestep(output_time_index);
        catchment_outfiles[formulation_pair.first] << output_time_index << "," << current_timestamp << "," << output_str << std::endl;
        response = response * boost::geometry::area(nexus_collection->get_feature(formulation_pair.first)->geometry<geojson::multipolygon_t>());
        std::cout << "\t\tThe modified response is: " << response << std::endl;
        //update the nexus with this flow
        nexus_realizations[ catchment_to_nexus[formulation_pair.first] ]->add_upstream_flow(response, catchment_id[formulation_pair.first], output_time_index);
      }
#endif

#ifdef NGEN_MPI_ACTIVE
      // MPI version
      if (world_rank == 0)
        A->set_et_params(pdm_et_data);
      else if (world_rank == 1)
        B->set_et_params(pdm_et_data);

      //MPI version
      long time_step = (long) output_time_index;
      long cat_A = 52;
      long cat_B = 52;
      long delta_t = 3600;

      ofstream cat_outfile;
      if (world_rank == 0) {
        double response = A->get_response(time_step, delta_t);
        nexus_upstream.add_upstream_flow(response, cat_A, time_step);
        std::cout << time_step << ", The response for Cat-A  is: " << response << std::endl;

        cat_outfile.open ("cat-A_output.csv", ios::out | ios::app);
        if (output_time_index == 0) {
          cat_outfile << "Time Step," << "Time," << "flow" <<std::endl;
        }
          cat_outfile << output_time_index << "," << current_timestamp << "," << response << std::endl;
        
      } else if (world_rank == 1) {
        nexus_downstream.add_upstream_flow(0.0, cat_B, time_step);
        double upstream_flow = nexus_downstream.get_upstream_flow_value();
        long upstream_catchment_id = nexus_downstream.get_catchment_id();
        time_step = nexus_downstream.get_time_step();
        std::cout << "received: " << time_step << " " << upstream_catchment_id << " " << upstream_flow << std::endl;

        //Make cat_B do some work
        double response = B->get_response(time_step, 3600.0);
        std::cout << time_step << ", The response for Cat-B  is: " << response << std::endl;

        cat_outfile.open ("cat-B_output.csv", ios::out | ios::app);
        if (output_time_index == 0) {
          cat_outfile << "Time Step," << "Time," << "flow," << "upstream_flow" <<std::endl;
        }
          cat_outfile << output_time_index << "," << current_timestamp << "," << response << "," << upstream_flow << std::endl;
      }
#endif

#ifndef NGEN_MPI_ACTIVE
      for(auto &nexus: nexus_realizations)
      {
        //TODO this ID isn't all that important is it?  And really it should connect to
        //the downstream waterbody the way we are using it, so consider if this is needed
        //it works for now though, so keep it
        int id = catchment_id[nexus_from_catchment[nexus.first]];
        std::cout<<"nexusID: "<<id<<std::endl;
        double contribution_at_t = nexus_realizations[nexus.first]->get_downstream_flow(id, output_time_index, 100.0);
        if(nexus_outfiles[nexus.first].is_open())
        {
          nexus_outfiles[nexus.first] << output_time_index << ", " << current_timestamp << ", " << contribution_at_t << std::endl;
        }
        std::cout<<"\tNexus "<<nexus.first<<" has "<<contribution_at_t<<" m^3/s"<<std::endl;
        output_map[nexus.first].push_back(contribution_at_t);
      }
#endif

#ifdef NGEN_MPI_ACTIVE
      MPI_Barrier(MPI_COMM_WORLD);
      if (output_time_index == manager.Simulation_Time_Object->get_total_output_times())
        cat_outfile.close();
#endif
    }

    /*
        The basic driving algorithm looks something like this:

        Iteration 1: Only read catchments and nexus
        Read configuration;
        From configuration, read inputs
        inputs: id, realization enum, input file (geojson), forcing provider (enum)

        loop:
          id -> read geojson -> lookup by id -> construct catchment realization and nexus
        TODO catchment realizations will need to construct forcing objects (implement forcing provider interface) upon constructuion
        One INDEPDENT timestep
        run catchment A -> forcing.get(time, dt, <grid?> ) -> run_model(time, dt, forcing)
        run catchment B

        Iteration 2: Read waterbodies, build linkage between nexus/waterbody, apply routing, multiple timesteps

    */

#ifdef NGEN_MPI_ACTIVE
    if (world_rank == 0)
      delete A;
    else if (world_rank == 1)
      delete B;

    MPI_Finalize();
#endif
}
