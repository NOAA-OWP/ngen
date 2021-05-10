#include <HY_Features.hpp>

using namespace hy_features;

HY_Features::HY_Features( geojson::GeoJSON fabric ){
  HY_Features(network::Network(fabric));
}

HY_Features::HY_Features(network::Network network):network(network){
      std::string feat_id;
      std::string feat_type;
      std::vector<std::string> origins, destinations;
      for(const auto& feat_idx : network){
        feat_id = network.get_id(feat_idx);//feature->get_id();
        feat_type = feat_id.substr(0, 3);

        if(feat_type == "cat")
        {
          origins = network.get_origination_ids(feat_id);
          destinations  = network.get_destination_ids(feat_id);

          std::shared_ptr<HY_Catchment> c = std::make_shared<HY_Catchment>(
              HY_Catchment(feat_id, origins, destinations)
            );
        }
        else if(feat_type == "nex")
        {

        }
        else
        {
          std::cerr<<"HY_Features::HY_Features unknown feature identifier type "<<feat_type<<" for feature id."<<feat_id
                   <<" Skipping feature"<<std::endl;
        }
      }
}

HY_Features::HY_Features( geojson::GeoJSON catchments, geojson::GeoJSON nexuses, std::string* link_key):
    network(catchments, link_key){

}
