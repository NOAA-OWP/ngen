#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

#include <FeatureBuilder.hpp>
#include <FeatureVisitor.hpp>
#include "features/Features.hpp"

#include "realizations/catchment/Realization_Config.hpp"
#include "realizations/catchment/Realization_Config_Reader.hpp"

#include <HY_HydroNexus.hpp>
#include <HY_Catchment.hpp>
#include <Simple_Lumped_Model_Realization.hpp>
#include <Tshirt_Realization.hpp>
#include <HY_PointHydroNexus.hpp>

#include "NGenConfig.h"
#include "tshirt_params.h"

std::string catchmentRealizationFile = "./data/sugar_creek/catchment_data_subset.geojson";
std::string nexusRealizationFile = "./data/sugar_creek/nexus_data_subset.geojson";
const std::string REALIZATION_CONFIG_PATH = "./data/example_realization_config.json";

//TODO this is possible, but ASSUMES realizations based on feature geom type, so not quite ready for prime time
class RealizaitonVisitor : public geojson::FeatureVisitor {

};

void prepare_features(geojson::GeoJSON& nexus, geojson::GeoJSON& catchments, bool validate=false)
{
  for(auto& feature: *nexus){
    feature->set_id(feature->get_property("ID").as_string());
    //std::cout << "Got Nexus Feature " << feature->get_id() << std::endl;
  }
  nexus->update_ids();
  //Now read the collection of catchments, iterate it and add them to the nexus collection
  //also link them by to->id
  //std::cout << "Iterating Catchment Features" << std::endl;
  for(auto& feature: *catchments){
    feature->set_id(feature->get_property("ID").as_string());
    nexus->add_feature(feature);
    //std::cout<<"Catchment "<<feature->get_id()<<" -> Nexus "<<feature->get_property("toID").as_string()<<std::endl;
  }
  std::string linkage = "toID";
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
    std::cout << "Hello there " << ngen_VERSION_MAJOR << "."
              << ngen_VERSION_MINOR << "."
              << ngen_VERSION_PATCH << std::endl;
    std::ios::sync_with_stdio(false);


    //Read the collection of nexus
    std::cout << "Building Nexus collection" << std::endl;
    geojson::GeoJSON nexus_collection = geojson::read(nexusRealizationFile);
    std::cout << "Iterating Nexus Features" << std::endl;


    std::cout << "Building Catchment collection" << std::endl;
    geojson::GeoJSON catchment_collection = geojson::read(catchmentRealizationFile);

    prepare_features(nexus_collection, catchment_collection, !true);

    realization::Realization_Config_Reader reader = realization::load_reader(REALIZATION_CONFIG_PATH);
    reader->read();

    //TODO don't really need catchment_collection once catchments are added to nexus collection
    catchment_collection.reset();


    std::map<std::string, realization::Realization_Config> catchment_configs;

    for(auto& feature : *nexus_collection)
    {
      std::string feat_id = feature->get_id();

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

        catchment_configs[feat_id] = reader->get(feat_id);
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
    std::cout<<"Running Models"<<std::endl;

    pdm03_struct pdm_et_data = get_et_params();

    //Now loop some time, iterate catchments, do stuff for 720 hourly time steps
    for(int time_step = 0; time_step < 720; time_step++)
    {
      std::cout<<"Time step "<<time_step<<std::endl;
      for (std::pair<std::string, realization::Realization_Config> realization_config_pair : catchment_configs) {
        double response = realization_config_pair.second->get_response(0, time_step, 3600.0, &pdm_et_data);
        std::cout<<"\tCatchment "<<realization_config_pair.first<<" contributing "<<response<<" m/s to "<<catchment_to_nexus[realization_config_pair.first]<<std::endl;
        catchment_outfiles[realization_config_pair.first] << time_step <<", "<<response<<std::endl;
        response = response * boost::geometry::area(nexus_collection->get_feature(realization_config_pair.first)->geometry<geojson::multipolygon_t>());
        std::cout << "\t\tThe modified response is: " << response << std::endl;
        nexus_realizations[ catchment_to_nexus[realization_config_pair.first] ]->add_upstream_flow(response, catchment_id[realization_config_pair.first], time_step);
      }

      for(auto &nexus: nexus_realizations)
      {
        if(true || nexus.first == "nex-92"){
        //TODO this ID isn't all that important is it?  And really it should connect to
        //the downstream waterbody the way we are using it, so consider if this is needed
        //it works for now though, so keep it
        int id = catchment_id[nexus_from_catchment[nexus.first]];
        double contribution_at_t = nexus_realizations[nexus.first]->get_downstream_flow(id, time_step, 100.0);
        if(nexus_outfiles[nexus.first].is_open())
        {
          nexus_outfiles[nexus.first] << time_step <<", "<<contribution_at_t<<std::endl;
        }
        std::cout<<"\tNexus "<<nexus.first<<" has "<<contribution_at_t<<" m^3/s"<<std::endl;
        output_map[nexus.first].push_back(contribution_at_t);
      }
      }
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
}
