#include <iostream>
#include <string>
#include <unordered_map>

#include <FeatureBuilder.hpp>
#include <FeatureVisitor.hpp>

#include <HY_HydroNexus.hpp>
#include <HY_Catchment.hpp>
#include <Simple_Lumped_Model_Realization.hpp>
#include <Tshirt_Realization.hpp>
#include <HY_PointHydroNexus.hpp>

#include "NGenConfig.h"
#include "tshirt_params.h"

std::string catchmentRealizationFile = "../data/sugar_creek/catchment_data_subset.geojson";
std::string nexusRealizationFile = "../data/sugar_creek/nexus_data_subset.geojson";

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

std::unordered_map<std::string, std::unique_ptr<HY_CatchmentRealization>>  catchment_realizations;
std::unordered_map<std::string, std::unique_ptr<HY_HydroNexus>> nexus_realizations;
std::unordered_map<std::string, std::string> catchment_to_nexus;
std::unordered_map<std::string, std::string> nexus_to_catchment;
std::unordered_map<std::string, std::string> nexus_from_catchment;
std::unordered_map<std::string, std::vector<double>> output_map;

//TODO move catchment int identity to relization, and update nexus to use string id
std::unordered_map<std::string, int> catchment_id;
std::unordered_map<std::string, std::string> forcing_paths {
  {"cat-88", "../data/sugar_creek/forcing/cat-88_2015-12-01 00:00:00_2015-12-30 23:00:00.csv"},
  {"cat-89", "../data/sugar_creek/forcing/cat-89_2015-12-01 00:00:00_2015-12-30 23:00:00.csv"},
  {"cat-92", "../data/sugar_creek/forcing/cat-92_2015-12-01 00:00:00_2015-12-30 23:00:00.csv"},
  {"cat-87", "../data/sugar_creek/forcing/cat-87_2015-12-01 00:00:00_2015-12-30 23:00:00.csv"}
};
// create the struct used for ET
pdm03_struct pdm_et_data;

//Define tshirt params
//{maxsmc, wltsmc, satdk, satpsi, slope, b, multiplier, aplha_fx, klf, kn, nash_n, Cgw, expon, max_gw_storage}
tshirt::tshirt_params tshirt_params{
  0.81,   //maxsmc FWRFH
  1.0,    //wltsmc  FIXME NOT USED IN TSHIRT?!?!
  0.48,   //satdk FWRFH
  0.1,    //satpsi    FIXME what is this and what should its value be?
  0.58,   //slope FWRFH
  1.3,      //b bexp? FWRFH
  1.0,    //multipier  FIXMME what should this value be
  1.0,    //aplha_fc   FIXME what should this value be
  0.0000672,    //Klf F
  0.1,    //Kn Kn	0.001-0.03 F
  8,      //nash_n     FIXME is 8 a good number for the cascade?
  1.08,    //Cgw C? FWRFH
  6.0,    //expon FWRFH
  16.0   //max_gw_storage Sgwmax FWRFH
};

//FIXME get real values for GIUH/Catchments
std::vector<double> cdf_times {0, 300, 600, 900, 1200};//, 1500, 1800, 2100, 2400, 2700};
std::vector<double> cdf_freq {0.00, 0.38, 0.59, 0.03, 0.0};

// Now doing this via json reader
//giuh::giuh_kernel giuh_k("cat-88", cdf_times, cdf_freq);
//unique_ptr<giuh::giuh_kernel> giuh_example = make_unique<giuh::giuh_kernel>(giuh_k);

typedef Simple_Lumped_Model_Realization _hymod;
typedef realization::Tshirt_Realization _tshirt;
int main(int argc, char *argv[]) {
    std::cout << "Hello there " << ngen_VERSION_MAJOR << "."
              << ngen_VERSION_MINOR << "."
              << ngen_VERSION_PATCH << std::endl;

    std::string start_time = "2015-12-01 00:00:00";
    std::string end_time = "2015-12-30 23:00:00";


    //Read the collection of nexus
    std::cout << "Building Nexus collection" << std::endl;
    geojson::GeoJSON nexus_collection = geojson::read(nexusRealizationFile);
    std::cout << "Iterating Nexus Features" << std::endl;


    std::cout << "Building Catchment collection" << std::endl;
    geojson::GeoJSON catchment_collection = geojson::read(catchmentRealizationFile);

    prepare_features(nexus_collection, catchment_collection, !true);

    //TODO don't really need catchment_collection once catchments are added to nexus collection
    catchment_collection.reset();

    pdm_et_data.scaled_distribution_fn_shape_parameter = 1.3;
    pdm_et_data.vegetation_adjustment = 0.99;
    pdm_et_data.model_time_step = 0.0;
    pdm_et_data.max_height_soil_moisture_storerage_tank = 400.0;
    pdm_et_data.maximum_combined_contents = pdm_et_data.max_height_soil_moisture_storerage_tank /
                                            (1.0 + pdm_et_data.scaled_distribution_fn_shape_parameter);

    //Hymod default params
    double storage = 1.0;
    double max_storage = 1000.0;
    double a = 1.0;
    double b = 10.0;
    double Ks = 0.1;
    double Kq = 0.01;
    long n = 3;
    double t = 0;
    time_step_t dt = 3600; //tshirt time step

    // TODO: parameterize these values, rather than hard-code
    // TODO: current mapping file shows "wat-*" rather than "cat-*" for some reason ... figure out if this has further implications
    std::string example_catchment_id = "wat-88";
    // For now expect in working directory
    std::string giuh_json_file_path = "./GIUH.json";
    std::string comid_mapping_json_file_path = "./crosswalk.json";
    giuh::GiuhJsonReader giuh_json_reader(giuh_json_file_path, comid_mapping_json_file_path);

    // Fall back to testing values if either of the above hard-coded paths doesn't work.
    if (!giuh_json_reader.is_data_json_file_readable() || !giuh_json_reader.is_id_map_json_file_readable()) {
        giuh_json_file_path = "../test/data/giuh/GIUH.json";
        comid_mapping_json_file_path = "../data/sugar_creek/crosswalk_subset.json";
        giuh_json_reader = giuh::GiuhJsonReader(giuh_json_file_path, comid_mapping_json_file_path);
    }

    for(auto& feature : *nexus_collection)
    {
      std::string feat_id = feature->get_id();

      if( feat_id.substr(0, 3) == "cat" ){
        //Create catchment realization, add to map
        forcing_params forcing_p(forcing_paths[feat_id], start_time, end_time);
        if (feature->get_property("realization").as_string() == "hymod") {
            //Create the hymod instance
            std::vector<double> sr_tmp = {1.0, 1.0, 1.0};
            catchment_realizations[feature->get_id()] = std::make_unique<_hymod>( _hymod(forcing_p, storage, max_storage, a, b, Ks, Kq, n, sr_tmp, t) );
        }
        else if(feature->get_property("realization").as_string() == "tshirt") {
          //Create the tshirt instance
          vector<double> nash_storage = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

          catchment_realizations[feature->get_id()] = std::make_unique<_tshirt>(forcing_p,
                 1.0, //soil_storage_meters
                 1.0, //groundwater_storage_meters
                 example_catchment_id, //used to cross-reference the COMID, need to look up the catchments GIUH data
                 giuh_json_reader,     //used to actually lookup GIUH data and create a giuh_kernel obj for catchment
                 tshirt_params, nash_storage, dt);

        }
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
    //Now loop some time, iterate catchments, do stuff for 720 hourly time steps
    for(int time_step = 0; time_step < 100; time_step++)
    {
      std::cout<<"Time step "<<time_step<<std::endl;
      for(auto &catchment: catchment_realizations)
      {
        if(catchment.first == "cat-88" || catchment.first == "cat-89")
        {
        //Get response for an hour (3600 seconds) time step
        double response = catchment.second->get_response(0, time_step, 3600.0, &pdm_et_data);

        std::cout<<"\tCatchment "<<catchment.first<<" contributing "<<response<<" m/s to "<<catchment_to_nexus[catchment.first]<<std::endl;

        nexus_realizations[ catchment_to_nexus[catchment.first] ]->add_upstream_flow(response, catchment_id[catchment.first], time_step);
      }
      }
      for(auto &nexus: nexus_realizations)
      {
        if(nexus.first == "nex-92"){
        //TODO this ID isn't all that important is it?  And really it should connect to
        //the downstream waterbody the way we are using it, so consider if this is needed
        //it works for now though, so keep it
        int id = catchment_id[nexus_from_catchment[nexus.first]];
        double contribution_at_t = nexus_realizations[nexus.first]->get_downstream_flow(id, time_step, 100.0);
        std::cout<<"\tNexus "<<nexus.first<<" has "<<contribution_at_t<<std::endl;
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
