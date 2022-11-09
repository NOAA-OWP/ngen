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
#include "Bmi_Cpp_Formulation.hpp"
#include "Bmi_C_Formulation.hpp"
#include "Bmi_Fortran_Formulation.hpp"
#include "Bmi_Multi_Formulation.hpp"
#include "Bmi_Py_Formulation.hpp"
#include <GenericDataProvider.hpp>
#include "CsvPerFeatureForcingProvider.hpp"
#ifdef NETCDF_ACTIVE
    #include "NetCDFPerFeatureDataProvider.hpp"
#endif

#ifdef NGEN_LSTM_TORCH_LIB_ACTIVE
    #include "LSTM_Realization.hpp"
#endif

namespace realization {
    typedef std::shared_ptr<Catchment_Formulation> (*constructor)(std::string, shared_ptr<data_access::GenericDataProvider>, utils::StreamHandler);

    template<class T>
    static constructor create_formulation_constructor() {
        return [](std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing_provider, utils::StreamHandler output_stream) -> std::shared_ptr<Catchment_Formulation>{
            return std::make_shared<T>(id, forcing_provider, output_stream);
        };
    };

    static std::map<std::string, constructor> formulations = {
        {"bmi_c++", create_formulation_constructor<Bmi_Cpp_Formulation>()},
#ifdef NGEN_BMI_C_LIB_ACTIVE
        {"bmi_c", create_formulation_constructor<Bmi_C_Formulation>()},
#endif // NGEN_BMI_C_LIB_ACTIVE
#ifdef NGEN_BMI_FORTRAN_ACTIVE
        {"bmi_fortran", create_formulation_constructor<Bmi_Fortran_Formulation>()},
#endif // NGEN_BMI_FORTRAN_ACTIVE
        {"bmi_multi", create_formulation_constructor<Bmi_Multi_Formulation>()},
#ifdef ACTIVATE_PYTHON
        {"bmi_python", create_formulation_constructor<Bmi_Py_Formulation>()},
#endif // ACTIVATE_PYTHON
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
        std::shared_ptr<data_access::GenericDataProvider> fp;
        if (forcing_config.provider == "CsvPerFeature" || forcing_config.provider == ""){
            fp = std::make_shared<CsvPerFeatureForcingProvider>(forcing_config);
        }
#ifdef NETCDF_ACTIVE
        else if (forcing_config.provider == "NetCDF"){
            fp = data_access::NetCDFPerFeatureDataProvider::get_shared_provider(forcing_config.path, forcing_config.simulation_start_t, forcing_config.simulation_end_t, output_stream);
        }
#endif
        else { // Some unknown string in the provider field?
            throw std::runtime_error(
                    "Invalid formulation forcing provider configuration! identifier: \"" + identifier +
                    "\", formulation_type: \"" + formulation_type +
                    "\", provider: \"" + forcing_config.provider + "\"");
        }
        return formulation_constructor(identifier, fp, output_stream);
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
