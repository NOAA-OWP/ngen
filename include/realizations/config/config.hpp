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
            // check if forcing is a string insted of object
            if (!tree.get_optional<std::string>("forcing")) {
                auto possible_forcing = tree.get_child_optional("forcing");
                if (possible_forcing) {
                    forcing = Forcing(*possible_forcing);
                }
            }
            // check if formulations is a string instead of object
            if (!tree.get_optional<std::string>("formulations")) {
                //get first empty key under formulations (corresponds to first json array element)
                auto possible_formulation_tree = tree.get_child_optional("formulations..");
                if(possible_formulation_tree){
                    formulation = Formulation(*possible_formulation_tree);
                }   
            }
        }
        /**
         * @brief Construct a new Config object from property tree with formulation and forcing groups as alternatives
         * 
         * @param tree JSON object with parameters for the configuration
         * @param formulation_groups Map between formulation group names and the Formulation templates. Clones of the Formulations will be stored on the Config
         * @param forcing_groups Map between the forcing group name and the Forcing templates. Clones of the Forcings will be stored on the Config.
         */
        Config(const boost::property_tree::ptree& tree,
               std::unordered_map<std::string, Formulation> &formulation_groups,
               std::unordered_map<std::string, Forcing> &forcing_groups)
                : Config(tree) {
            if (!this->has_formulation()) {
                boost::optional<std::string> formulation_group = tree.get_optional<std::string>("formulations");
                if (formulation_group) {
                    if (formulation_groups.find(*formulation_group) != formulation_groups.end()) {
                        this->formulation = formulation_groups[*formulation_group].clone();
                    }
                }
            }

            if (this->forcing.parameters.size() == 0) {
                boost::optional<std::string> forcing_group = tree.get_optional<std::string>("forcing");
                if (forcing_group) {
                    if (forcing_groups.find(*forcing_group) != forcing_groups.end()) {
                        this->forcing = forcing_groups[*forcing_group].clone();
                    }
                }
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
