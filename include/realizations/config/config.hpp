#ifndef NGEN_REALIZATION_CONFIG_H
#define NGEN_REALIZATION_CONFIG_H

#include <boost/property_tree/ptree.hpp>

#include "formulation.hpp"
#include "forcing.hpp"

namespace realization{
  namespace config{

    /**
     * @brief Structure representing the configuration for a general Formulation.
     * 
     */
    struct Config{
        
        /**
         * @brief Construct a new Config object
         *
         */
        Config() = default;

        /**
         * @brief Construct a new Config object from a property tree
         * 
         * @param tree 
         */
        Config(const boost::property_tree::ptree& tree){
        
            auto possible_forcing = tree.get_child_optional("forcing");

            if (possible_forcing) {
                forcing = Forcing(*possible_forcing);
            }
            //get first empty key under formulations (corresponds to first json array element)
            auto possible_formulation_tree = tree.get_child_optional("formulations..");
            if(possible_formulation_tree){
                formulation = Formulation(*possible_formulation_tree);
            }
        }

        /**
         * @brief Determine if the config has a formulation
         * 
         * @return true if the formulation name/type is set or if model parameters are present
         * @return false if either the type or parameters are empty
         */
        bool has_formulation(){
            return !(formulation.type.empty() || formulation.parameters.empty());
        }

        Formulation formulation;
        Forcing forcing;
    };

    
  }//end namespace config
}//end namespace realization
#endif //NGEN_REALIZATION_CONFIG_H
