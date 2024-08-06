#ifndef NGEN_REALIZATION_CONFIG_FORMULATION_H
#define NGEN_REALIZATION_CONFIG_FORMULATION_H

#include <NGenConfig.h>
#include <boost/property_tree/ptree.hpp>
#include <string>

#include "JSONProperty.hpp"

#if NGEN_WITH_MPI
#include <mpi.h>
#endif

namespace realization{
  namespace config{

  struct Formulation{
    std::string type;
    //Formulation parameters, object as a PropertyMap
    geojson::PropertyMap parameters;
    //List of nested formulations (used for multi bmi representations)
    std::vector<Formulation> nested;

    /**
     * @brief Construct a new default Formulation object
     * 
     * Default objects have an "" type and and empty property map.
     */
    Formulation() = default;

    /**
     * @brief Construct a new Formulation object
     * 
     * @param type formulation type represented
     * @param params formulation parameter mapping
     */
    Formulation(std::string type, geojson::PropertyMap params):type(std::move(type)), parameters(params){}

    /**
     * @brief Construct a new Formulation object from a boost property tree
     * 
     * The tree should have a "name" key corresponding to the formulation type
     * as well as a "params" key to build the property map from
     * 
     * @param tree property tree to build Formulation from
     */
    Formulation(const boost::property_tree::ptree& tree){
        type = tree.get<std::string>("name");
        for (std::pair<std::string, boost::property_tree::ptree> setting : tree.get_child("params")) {
            //Construct the geoJSON PropertyMap from each key, value pair in  "params"
            parameters.emplace(
                setting.first,
                geojson::JSONProperty(setting.first, setting.second)
            );
        }
        if(type=="bmi_multi"){
            for(auto& module : tree.get_child("params.modules")){
                //Create the nested formulations in order of definition
                nested.push_back(Formulation(module.second));
            }
        // If running MPI job, output with only one processor
        #if NGEN_WITH_MPI
        int mpi_rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        if (mpi_rank == 0)
        #endif
        {
            geojson::JSONProperty::print_property(parameters.at("modules"));
        }
      }
    }

    /**
     * @brief Link formulation parameters to hydrofabric data held in feature
     * 
     * @param feature Hydrofabric feature with properties to assign to formulation
     *                model params
     */
    void link_external(geojson::Feature feature){

        if(type == "bmi_multi"){
            std::vector<geojson::JSONProperty> tmp;
            for(auto& n : nested){
                //Iterate and link any nested modules with this feature
                if(n.parameters.count("model_params")){
                    n.link_external(feature);
                }
                //Need a temporary map to hold the updated formulation properties in
                geojson::PropertyMap map = {};
                map.emplace("name", geojson::JSONProperty("name", n.type));
                map.emplace("params", geojson::JSONProperty("", n.parameters));
                tmp.push_back(geojson::JSONProperty("", map));
            }
            //Reset the bmi_multi modules with the now linked module definitions
            parameters.at("modules") = geojson::JSONProperty("modules", tmp);
            return;
        }
        //Short circut
        if(parameters.count("model_params") < 1 ) return;
        //Have some model params, check to see if any should be linked to the hyrdofabric feature
        geojson::PropertyMap attr = parameters.at("model_params").get_values();
        for (decltype(auto) param : attr) {
            // Check for type to short-circuit. If param.second is not an object, `.has_key()` will throw
            if (param.second.get_type() != geojson::PropertyType::Object || !param.second.has_key("source")) {
                attr.emplace(param.first, param.second);
                continue;
            }
        
            decltype(auto) param_source = param.second.at("source");
            decltype(auto) param_source_name = param_source.as_string();
            if (param_source_name != "hydrofabric") {
                // TODO: temporary until the logic for alternative sources is designed
                throw std::logic_error("ERROR: 'model_params' source `" + param_source_name + "` not currently supported. Only `hydrofabric` is supported.");
            }

            // Property name in the feature properties is either
            // the value of key "from", or has the same name as
            // the expected model parameter key
            decltype(auto) param_name = param.second.has_key("from")
                ? param.second.at("from").as_string()
                : param.first;

            if (feature->has_property(param_name)) {
                auto catchment_attribute = feature->get_property(param_name);

                // Use param.first in the `.emplace` calls instead of param_name since
                // the expected name is given by the key of the model_params values.
                switch (catchment_attribute.get_type()) {
                    case geojson::PropertyType::List:
                    case geojson::PropertyType::Object:
                        // TODO: Should list/object values be passed to model parameters?
                        //       Typically, feature properties *should* be scalars.
                        std::cerr << "WARNING: property type " << static_cast<int>(catchment_attribute.get_type()) << " not allowed as model parameter. "
                                    << "Must be one of: Natural (int), Real (double), Boolean, or String" << '\n';
                        break;
                    default:
                        attr.at(param.first) = geojson::JSONProperty(param.first, catchment_attribute);
                }
            }
        }
        parameters.at("model_params") = geojson::JSONProperty("model_params", attr);
    }
  };

  }//end namespace config
}//end namespace realization
#endif //NGEN_REALIZATION_CONFIG_FORMULATION_H
