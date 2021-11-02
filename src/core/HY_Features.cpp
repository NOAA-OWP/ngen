#include <HY_Features.hpp>
#include <HY_PointHydroNexus.hpp>

using namespace hy_features;

HY_Features::HY_Features( geojson::GeoJSON fabric, std::shared_ptr<Formulation_Manager> formulations)
  :HY_Features(network::Network(fabric), formulations)
{
  /* If you nest the constuctor calls like this, then the network, while valid in the next constructor,
     is empty once the collection is fully instansiated...
    HY_Features(network::Network(fabric));
  */
}

HY_Features::HY_Features(network::Network network, std::shared_ptr<Formulation_Manager> formulations)
  :network(network), formulations(formulations)
{
      std::string feat_id;
      std::string feat_type;
      std::vector<std::string> origins, destinations;

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
        else if(feat_type == "nex" || feat_type == "tnx")
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

HY_Features::HY_Features( geojson::GeoJSON catchments, std::string* link_key, std::shared_ptr<Formulation_Manager> formulations):
    HY_Features(network::Network(catchments, link_key), formulations){

}
