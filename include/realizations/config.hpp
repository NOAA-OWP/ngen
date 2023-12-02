#ifndef NGEN_REALIZATION_CONFIG_H
#define NGEN_REALIZATION_CONFIG_H

#include <boost/property_tree/ptree.hpp>
#include <string>

#include "JSONProperty.hpp"
#include "Simulation_Time.hpp"
#include "routing/Routing_Params.h"
#include "LayerData.hpp"


#include<iostream>

namespace realization{
  namespace config{

    static const std::string ROUTING_CONFIG_KEY = "t_route_config_file_with_path";
    struct Routing{
        std::shared_ptr<routing_params> params;
        Routing(const boost::property_tree::ptree& tree){
            params = std::make_shared<routing_params>(tree.get(ROUTING_CONFIG_KEY, ""));
        }
    };

    struct Time{
        std::string start_time;
        std::string end_time;
        unsigned int output_interval;

        Time(const boost::property_tree::ptree& tree){
                start_time = tree.get("start_time", std::string());
                end_time = tree.get("end_time", std::string());
                output_interval = tree.get("output_interval", 0);
        }

        simulation_time_params make_params(){
                            std::vector<std::string> missing_simulation_time_parameters;

            if (start_time.empty()){
                missing_simulation_time_parameters.push_back("start_time");
            }

            if (end_time.empty()) {
                missing_simulation_time_parameters.push_back("end_time");
            }

            if (output_interval == 0) {
                missing_simulation_time_parameters.push_back("output_interval");
            }

            if (missing_simulation_time_parameters.size() > 0) {
                std::string message = "ERROR: A simulation time parameter cannot be created; the following parameters are missing or invalid: ";

                for (int missing_parameter_index = 0; missing_parameter_index < missing_simulation_time_parameters.size(); missing_parameter_index++) {
                    message += missing_simulation_time_parameters[missing_parameter_index];

                    if (missing_parameter_index < missing_simulation_time_parameters.size() - 1) {
                        message += ", ";
                    }
                }
                
                throw std::runtime_error(message);
            }
            return simulation_time_params(
                        start_time,
                        end_time,
                        output_interval
                    );
        }
    };


    struct Formulation_Config{

        struct Formulation{
            std::string type;
            geojson::PropertyMap parameters;
            std::vector<Formulation> nested;
            Formulation():type(std::string()), parameters(geojson::PropertyMap()){}
            Formulation(std::string& type, geojson::PropertyMap params):type(std::move(type)), parameters(params){}
            Formulation(const boost::property_tree::ptree& tree){
                type = tree.get<std::string>("name");
                for (std::pair<std::string, boost::property_tree::ptree> setting : tree.get_child("params")) {
                    //if( setting.first == "modules" ) continue; //FUCK STUFF if skipping modules, we somehow
                    //end up with "modules" as an object and it fucks up calls to as_list() later with a boost assertion error
                    //that has something to do with an empty invariant...
                    //std::cout<<"FORMULATION: KEY -> "<<setting.first<<"\n";
                    parameters.emplace(
                        setting.first,
                        geojson::JSONProperty(setting.first, setting.second)
                    );
                }
                if(type=="bmi_multi"){
                    for(auto& module : tree.get_child("params.modules")){
                        nested.push_back(Formulation(module.second));
                    }
                    geojson::JSONProperty::print_property(parameters.at("modules"));
                }
                
            }

            void link_external(geojson::Feature feature){

                if(type == "bmi_multi"){
                    std::vector<geojson::JSONProperty> tmp;
                    for(auto& n : nested){
                        if(n.parameters.count("model_params")){
                            n.link_external(feature);
                        }
                        geojson::PropertyMap map = {};
                        map.emplace("name", geojson::JSONProperty("name", n.type));
                        map.emplace("params", geojson::JSONProperty("", n.parameters));
                        // std::cout<<"TESTING "<<get_propertytype_name(map.at("params").at("model_params").get_type())<<"\n";
                        // for(auto& wtf : map.at("params").at("model_params").get_values()) std::cout<<wtf.first<<": "<<get_propertytype_name(wtf.second.get_type())<<"\n";
                        tmp.push_back(geojson::JSONProperty("", map));
                    }
                    std::cout<<"UPDATING MODULES\n";
                    parameters.at("modules") = geojson::JSONProperty("modules", tmp);
                    return;
                }
                if(parameters.count("model_params") < 1 ) return;
                geojson::PropertyMap new_params = {};
                geojson::PropertyMap attr = parameters.at("model_params").get_values();
                for (decltype(auto) param : attr) {
                    std::cout<<"LINKING "<<type<<" module: "<<param.first<<"\n";
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
                                //std::cout<<"NO CLUE: "<<get_propertytype_name(parameters.at("model_params").at(param.first).get_type())<<"\n";
                                std::cout<<"MAKING TMP COPY\n";
                                geojson::JSONProperty tmp = geojson::JSONProperty(param.first, catchment_attribute);
                                std::cout<<"TMP MADE\n";
                                std::cout<<"CREATING TYPE "<<get_propertytype_name(tmp.get_type())<<"\n";
                                attr.at(param.first) = geojson::JSONProperty(param.first, catchment_attribute);
                                std::cout<<"EMPLACED!\n";
                                auto tmp2 = attr.at(param.first);
                                std::cout<<"RETRIEVED TYPE "<<get_propertytype_name(tmp2.get_type())<<"\n";
                        }
                    }
                }
                parameters.at("model_params") = geojson::JSONProperty("model_params", attr);
            }
        };

        struct Forcing{
            geojson::PropertyMap parameters;
            Forcing():parameters(geojson::PropertyMap()){};
            Forcing(const boost::property_tree::ptree& tree){
                //get forcing info
                for (auto &forcing_parameter : tree) {
                    this->parameters.emplace(
                        forcing_parameter.first,
                        geojson::JSONProperty(forcing_parameter.first, forcing_parameter.second)
                    );  
                }
            }
            bool has_key(const std::string& key) const{
                return parameters.count(key) > 0;
            }
        };

        Formulation_Config(){};
        Formulation_Config(const boost::property_tree::ptree& tree):formulation_tree(tree){
        
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

        bool has_formulation(){
            return !(formulation.type.empty() || formulation.parameters.empty());
        }

        Formulation formulation;
        boost::property_tree::ptree formulation_tree;
        Forcing forcing;
    };

    struct Layer{
        Layer():descriptor( {"surface layer", "s", 0, 3600 } ){};
        Layer(const boost::property_tree::ptree& tree):formulation(tree){
            auto name = tree.get_optional<std::string>("name");
            if(!name) missing_keys.push_back("name");
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
            
            std::cout<<"BUILDING LAYER FORMULATION: "<<formulation.has_formulation()<<"\n";
        }

        const ngen::LayerDescription& get_descriptor(){
            if(!missing_keys.empty()){
                std::string message = "ERROR: Layer cannot be created; the following parameters are missing or invalid: ";

                for (int missing_parameter_index = 0; missing_parameter_index < missing_keys.size(); missing_parameter_index++) {
                    message += missing_keys[missing_parameter_index];

                    if (missing_parameter_index < missing_keys.size() - 1) {
                        message += ", ";
                    }
                }
                
                throw std::runtime_error(message);
            }
            return descriptor;
        }

        bool has_formulation(){
            return formulation.has_formulation();
        }

        const std::string& get_domain(){ return domain; }
        Formulation_Config formulation;
        private:
        std::string domain;
        ngen::LayerDescription descriptor;
        std::vector<std::string> missing_keys;

    };
  };//end namespace config
}//end namespace realization
#endif //NGEN_REALIZATION_CONFIG_H