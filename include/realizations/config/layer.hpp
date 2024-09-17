#ifndef NGEN_REALIZATION_CONFIG_LAYER_H
#define NGEN_REALIZATION_CONFIG_LAYER_H

#include <boost/property_tree/ptree.hpp>

#include "LayerData.hpp"
#include "config.hpp"

namespace realization{
  namespace config{

  /**
   * @brief Layer configuration information
   * 
   */
  struct Layer{
    /**
     * @brief Construct a new default surface layer
     * 
     * Default layers are surface layers (0) with 3600 second time steps
     * 
     */
    Layer():descriptor( {"surface layer", "s", 0, 3600 } ){};

    /**
     * @brief Construct a new Layer from a property tree
     * 
     * @param tree 
     */
    Layer(const boost::property_tree::ptree& tree):formulation(tree){
        std::vector<std::string> missing_keys;
        auto name = tree.get_optional<std::string>("layer_name");
        if(!name) missing_keys.push_back("layer_name");
        auto unit = tree.get<std::string>("time_step_units", "s");

        auto id = tree.get_optional<int>("id");
        if(!id) missing_keys.push_back("id");

        auto ts = tree.get_optional<double>("time_step");
        //TODO is time_step required? e.g. need to add to missing_keys?
        auto tmp = tree.get_optional<std::string>("domain");
        if(tmp) domain = *tmp;
        if(missing_keys.empty()){
            descriptor = {*name, unit, *id, *ts};// ngen::LayerDescription(  );
        }
        else{
            std::string message = "ERROR: Layer cannot be created; the following parameters are missing or invalid: ";

            for (const auto& missing : missing_keys) {
                message += missing;
                message += ", ";
            }
            //pop off the extra ", "
            message.pop_back();
            message.pop_back();
            throw std::runtime_error(message);
        }
    }

    /**
     * @brief Get the descriptor associated with this layer configuration
     * 
     * @return const ngen::LayerDescription& 
     */
    const ngen::LayerDescription& get_descriptor(){
        return descriptor;
    }

    /**
     * @brief Determines if the layer has a valid configured formulation
     * 
     * @return true 
     * @return false 
     */
    bool has_formulation(){
        return formulation.has_formulation();
    }

    /**
     * @brief Get the domain description
     * 
     * @return const std::string& 
     */
    const std::string& get_domain(){ return domain; }

    /**
     * @brief The formulation configuration associated with the layer
     * 
     */
    Config formulation;
    private:
    std::string domain;
    ngen::LayerDescription descriptor;

  };

  }//end namespace config
}//end namespace realization
#endif //NGEN_REALIZATION_CONFIG_LAYER_H
