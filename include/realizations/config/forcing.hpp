#ifndef NGEN_REALIZATION_CONFIG_FORCING_H
#define NGEN_REALIZATION_CONFIG_FORCING_H

#include <boost/property_tree/ptree.hpp>

#include "JSONProperty.hpp"

namespace realization{
  namespace config{

  /**
   * @brief Structure for holding forcing configuration information
   * 
   */
  struct Forcing{
    /**
     * @brief key -> Property mapping for forcing parameters
     * 
     */
    geojson::PropertyMap parameters;

    /**
     * @brief Construct a new, empty Forcing object
     * 
     */
    Forcing():parameters(geojson::PropertyMap()){};

    /**
     * @brief Construct a new Forcing object from a property_tree
     * 
     * @param tree 
     */
    Forcing(const boost::property_tree::ptree& tree){
        //get forcing info
        for (auto &forcing_parameter : tree) {
            this->parameters.emplace(
                forcing_parameter.first,
                geojson::JSONProperty(forcing_parameter.first, forcing_parameter.second)
            );  
        }
    }

    /**
     * @brief Test if a particualr forcing parameter exists
     * 
     * @param key parameter name to test
     * @return true if the forcing properties contain key
     * @return false if the key is not in the forcing properties
     */
    bool has_key(const std::string& key) const{
        return parameters.count(key) > 0;
    }
  };


  }//end namespace config
}//end namespace realization
#endif //NGEN_REALIZATION_CONFIG_FORCING_H
