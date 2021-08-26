#ifndef NGEN_FORMULATION_CONSTRUCTORS_H
#define NGEN_FORMULATION_CONSTRUCTORS_H

#include "Formulation.hpp"
#include <JSONProperty.hpp>
#include <exception>

#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

// Formulations
#include "Tshirt_Realization.hpp"
#include "Tshirt_C_Realization.hpp"
#include "Simple_Lumped_Model_Realization.hpp"
#include "Bmi_C_Formulation.hpp"
#include "Bmi_Fortran_Formulation.hpp"

#ifdef NGEN_LSTM_TORCH_LIB_ACTIVE
    #include "LSTM_Realization.hpp"
#endif

namespace realization {
    typedef std::shared_ptr<Catchment_Formulation> (*constructor)(std::string, forcing_params, utils::StreamHandler);

    template<class T>
    static constructor create_formulation_constructor() {
        return [](std::string id, forcing_params forcing_config, utils::StreamHandler output_stream) -> std::shared_ptr<Catchment_Formulation>{
            return std::make_shared<T>(id, forcing_config, output_stream);
        };
    };

    static std::map<std::string, constructor> formulations = {
#ifdef NGEN_BMI_C_LIB_ACTIVE
        {"bmi_c", create_formulation_constructor<Bmi_C_Formulation>()},
#endif // NGEN_BMI_C_LIB_ACTIVE
#ifdef ACTIVATE_FORTRAN
        {"bmi_fortran", create_formulation_constructor<Bmi_Fortran_Formulation>()},
#endif // ACTIVATE_FORTRAN
        {"tshirt", create_formulation_constructor<Tshirt_Realization>()},
        {"tshirt_c", create_formulation_constructor<Tshirt_C_Realization>()},
        {"simple_lumped", create_formulation_constructor<Simple_Lumped_Model_Realization>()}
#ifdef NGEN_LSTM_TORCH_LIB_ACTIVE
        ,
        {"lstm", create_formulation_constructor<LSTM_Realization>()}
#endif
    };

    static bool formulation_exists(std::string formulation_type) {
        return formulations.count(formulation_type) > 0;
    }

    static std::shared_ptr<Catchment_Formulation> construct_formulation(
        std::string formulation_type,
        std::string identifier,
        forcing_params &forcing_config,
        utils::StreamHandler output_stream
    ) {
        constructor formulation_constructor = formulations.at(formulation_type);
        return formulation_constructor(identifier, forcing_config, output_stream);
    };

    static std::string get_formulation_key(const boost::property_tree::ptree &tree) {
        /*for (auto &node : tree) {
            if (formulation_exists(node.first)) {
                return node.first;
            }
        }*/
        boost::optional<std::string> key = tree.get_optional<std::string>("name");
        if(key && formulation_exists(*key)){
          return *key;
        }

        throw std::runtime_error("No valid formulation for " + *key + " was described in the passed in tree.");
    }
}

#endif // NGEN_FORMULATION_CONSTRUCTORS_H
