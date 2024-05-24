#ifndef NGEN_REALIZATION_CONFIG_ROUTING_H
#define NGEN_REALIZATION_CONFIG_ROUTING_H

#include <boost/property_tree/ptree.hpp>

#include "routing/Routing_Params.h"

namespace realization{
  namespace config{

    static const std::string ROUTING_CONFIG_KEY = "t_route_config_file_with_path";
    struct Routing{
        std::shared_ptr<routing_params> params;
        Routing(const boost::property_tree::ptree& tree){
            params = std::make_shared<routing_params>(tree.get(ROUTING_CONFIG_KEY, ""));
        }
    };

  }//end namespace config
}//end namespace realization
#endif //NGEN_REALIZATION_CONFIG_ROUTING_H
