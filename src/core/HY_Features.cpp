#include <HY_Features.hpp>
#include <HY_PointHydroNexus.hpp>

using namespace hy_features;

HY_Features::HY_Features( geojson::GeoJSON fabric, std::shared_ptr<Formulation_Manager> formulations)
  :HY_Features(network::Network(fabric), formulations, fabric)
{
  /* If you nest the constuctor calls like this, then the network, while valid in the next constructor,
     is empty once the collection is fully instansiated...
    HY_Features(network::Network(fabric));
  */
}

HY_Features::HY_Features( geojson::GeoJSON catchments, std::string* link_key, std::shared_ptr<Formulation_Manager> formulations):
    HY_Features(network::Network(catchments, link_key), formulations, catchments){  
}

HY_Features::HY_Features(network::Network network, std::shared_ptr<Formulation_Manager> formulations, geojson::GeoJSON fabric)
  :network(network), formulations(formulations)
{
      std::string feat_id;
      std::string feat_type;
      std::vector<std::string> origins, destinations;

      for(const auto& feat_idx : network){
        feat_id = network.get_id(feat_idx);//feature->get_id();
        feat_type = feat_id.substr(0, feat_id.find(hy_features::identifiers::seperator) );

        destinations  = network.get_destination_ids(feat_id);
        if(hy_features::identifiers::isCatchment(feat_type))
        {
          //Find and prepare formulation
          auto formulation = formulations->get_formulation(feat_id);
          formulation->set_output_stream(formulations->get_output_root() + feat_id + ".csv");
          // TODO: add command line or config option to have this be omitted
          //FIXME why isn't default param working here??? get_output_header_line() fails.
          formulation->write_output("Time Step,""Time,"+formulation->get_output_header_line(",")+"\n");
          //Find upstream nexus ids
          origins = network.get_origination_ids(feat_id);

          // get the catchment layer from the hydro fabric
          const auto& cat_json_node = fabric->get_feature(feat_id);
          long lyr = cat_json_node->has_key("layer") ? cat_json_node->get_property("layer").as_natural_number() : 0;

          // add this layer to the set of layers if needed
          if (hf_layers.find(lyr) == hf_layers.end() )
          {
              hf_layers.insert(lyr);
          }

          //Create the HY_Catchment with the formulation realization
          std::shared_ptr<HY_Catchment> c = std::make_shared<HY_Catchment>(
              HY_Catchment(feat_id, origins, destinations, formulation, lyr)
            );

          _catchments.emplace(feat_id, c);
        }
        else if(hy_features::identifiers::isNexus(feat_type))
        {
            _nexuses.emplace(feat_id, std::make_unique<HY_PointHydroNexus>(
                                          HY_PointHydroNexus(feat_id, destinations) ));
        }
        else
        {
          std::cerr<<"HY_Features::HY_Features unknown feature identifier type "<<feat_type<<" for feature id."<<feat_id
                   <<" Skipping feature"<<std::endl;
        }
      }

}