#ifndef NGEN_FORMULATION_CONSTRUCTORS_H
#define NGEN_FORMULATION_CONSTRUCTORS_H

#include "Formulation.hpp"
#include <JSONProperty.hpp>
#include <exception>

#include <boost/property_tree/ptree.hpp>

// Formulations
#include "Tshirt_Realization.hpp"
#include "Simple_Lumped_Model_Realization.hpp"

namespace realization {
    typedef std::shared_ptr<Formulation> (*constructor)(std::string, forcing_params, utils::StreamHandler);

    template<class T>
    static constructor create_formulation_constructor() {
        return [](std::string id, forcing_params forcing_config, utils::StreamHandler output_stream) -> std::shared_ptr<Formulation>{
            return std::make_shared<T>(id, forcing_config, output_stream);
        };
    };

    static std::map<std::string, constructor> formulations = {
        {"tshirt", create_formulation_constructor<Tshirt_Realization>()},
        {"simple_lumped", create_formulation_constructor<Simple_Lumped_Model_Realization>()}
    };

    static bool formulation_exists(std::string formulation_type) {
        return formulations.count(formulation_type) > 0;
    }

    static std::shared_ptr<Formulation> construct_formulation(
        std::string formulation_type,
        std::string identifier,
        forcing_params &forcing_config,
        utils::StreamHandler output_stream
    ) {
        constructor formulation_constructor = formulations.at(formulation_type);
        return formulation_constructor(identifier, forcing_config, output_stream);
    };

    static std::string get_formulation_key(boost::property_tree::ptree &tree) {
        for (auto &node : tree) {
            if (formulation_exists(node.first)) {
                return node.first;
            }
        }

        throw std::runtime_error("No valid formulation was described in the passed in tree.");
    }
}

#endif // NGEN_FORMULATION_CONSTRUCTORS_H