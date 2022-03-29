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
#include "ForcingProvider.hpp"
#include <NetCDFPerFeatureDataProvider.hpp>
#include "CsvPerFeatureForcingProvider.hpp"

#ifdef NGEN_LSTM_TORCH_LIB_ACTIVE
    #include "LSTM_Realization.hpp"
#endif

namespace realization {
    /*template<class T, class DataProviderType>
    auto create_formulation_constructor() {
        return [](std::string id, std::shared_ptr<DataProviderType> forcing_provider, utils::StreamHandler output_stream) -> std::shared_ptr<Catchment_Formulation>{
            return std::make_shared<T>(id, std::move(forcing_provider), output_stream);
        };

        typedef std::shared_ptr<Catchment_Formulation> (*constructor)(std::string, shared_ptr<DataProviderType>, utils::StreamHandler);
    };*/

    /*static std::map<std::string, constructor> formulations = {
        {"bmi_c++", create_formulation_constructor<Bmi_Cpp_Formulation>()},
#ifdef NGEN_BMI_C_LIB_ACTIVE
        {"bmi_c", create_formulation_constructor<Bmi_C_Formulation, CsvPerFeatureForcingProvider>()},
#endif // NGEN_BMI_C_LIB_ACTIVE
#ifdef NGEN_BMI_FORTRAN_ACTIVE
        {"bmi_fortran", create_formulation_constructor<Bmi_Fortran_Formulation, CsvPerFeatureForcingProvider>()},
#endif // NGEN_BMI_FORTRAN_ACTIVE
        {"bmi_multi", create_formulation_constructor<Bmi_Multi_Formulation, CsvPerFeatureForcingProvider>()},
#ifdef ACTIVATE_PYTHON
        {"bmi_python", create_formulation_constructor<Bmi_Py_Formulation, CsvPerFeatureForcingProvider>()},
#endif // ACTIVATE_PYTHON
        {"tshirt", create_formulation_constructor<Tshirt_Realization, >()},
        {"tshirt_c", create_formulation_constructor<Tshirt_C_Realization>()},
        {"simple_lumped", create_formulation_constructor<Simple_Lumped_Model_Realization>()}
#ifdef NGEN_LSTM_TORCH_LIB_ACTIVE
        ,
        {"lstm", create_formulation_constructor<LSTM_Realization>()}
#endif
    }*/

    std::vector<std::string> formulations = {
#ifdef NGEN_BMI_C_LIB_ACTIVE
        "bmi_c",
#endif // NGEN_BMI_C_LIB_ACTIVE
#ifdef NGEN_BMI_FORTRAN_ACTIVE
        "bmi_fortran", 
#endif // NGEN_BMI_FORTRAN_ACTIVE
        "bmi_multi", 
#ifdef ACTIVATE_PYTHON
        "bmi_python", 
#endif // ACTIVATE_PYTHON
        "tshirt", 
        "tshirt_c",
#ifdef NGEN_LSTM_TORCH_LIB_ACTIVE
        "lstm", 
#endif
        "simple_lumped"
    };
    static bool formulation_exists(std::string formulation_type) {
        return std::find(formulations.begin(), formulations.end(), formulation_type) != formulations.end();
    }

    static std::shared_ptr<Catchment_Formulation> construct_formulation(
        std::string formulation_type,
        std::string identifier,
        forcing_params &forcing_config,
        utils::StreamHandler output_stream
    ) {
        //constructor formulation_constructor = formulations.at(formulation_type);
        
        if (formulation_type == "tshirt" || formulation_type == "tshirt_c"  || formulation_type == "lstm" // These formulations are still using the legacy interface!
            || forcing_config.provider == "" || forcing_config.provider == "legacy") // Permit legacy Forcing class with BMI formulations and simple_lumped--don't break old configs
        {
            std::unique_ptr<Forcing> fp;
            fp = std::make_unique<Forcing>(forcing_config);
            #ifdef NGEN_BMI_C_LIB_ACTIVE
            if (formulation_type == "bmi_c" )
                return std::make_shared<Bmi_C_Formulation>(identifier, std::move(fp), output_stream);
            #endif
            #ifdef NGEN_BMI_FORTRAN_ACTIVE
            if (formulation_type == "bmi_fortran" )
                return std::make_shared<Bmi_Fortran_Formulation>(identifier, std::move(fp), output_stream);
            #endif
            if (formulation_type == "bmi_multi" )
                return std::make_shared<Bmi_Multi_Formulation>(identifier, std::move(fp), output_stream);
            #ifdef ACTIVATE_PYTHON
            if (formulation_type == "bmi_python" )
                return std::make_shared<Bmi_Py_Formulation>(identifier, std::move(fp), output_stream);
            #endif
            #ifdef NGEN_LSTM_TORCH_LIB_ACTIV
            if (formulation_type == "lstm" )
                return std::make_shared<LSTM_Realization>(identifier, std::move(fp), output_stream);
            #endif
            if (formulation_type == "tshirt" )
                 return std::make_shared<Tshirt_Realization>(identifier, std::move(fp), output_stream);
            if (formulation_type == "tshirt_c" )
                return std::make_shared<Tshirt_C_Realization>(identifier, std::move(fp), output_stream);
            if (formulation_type == "simple_lumped" )
                return std::make_shared<Simple_Lumped_Model_Realization>(identifier, std::move(fp), output_stream);
            throw std::runtime_error("Could not find a matching constructor");
        }
        else if (forcing_config.provider == "CsvPerFeature")
        {
            std::unique_ptr<CsvPerFeatureForcingProvider> fp;
            fp = std::make_unique<CsvPerFeatureForcingProvider>(forcing_config);
            #ifdef NGEN_BMI_C_LIB_ACTIVE
            if (formulation_type == "bmi_c" )
                return std::make_shared<Bmi_C_Formulation>(identifier, std::move(fp), output_stream);
            #endif
            #ifdef NGEN_BMI_FORTRAN_ACTIVE
            if (formulation_type == "bmi_fortran" )
                return std::make_shared<Bmi_Fortran_Formulation>(identifier, std::move(fp), output_stream);
            #endif
            if (formulation_type == "bmi_multi" )
                return std::make_shared<Bmi_Multi_Formulation>(identifier, std::move(fp), output_stream);
            #ifdef ACTIVATE_PYTHON
            if (formulation_type == "bmi_python" )
                return std::make_shared<Bmi_Py_Formulation>(identifier, std::move(fp), output_stream);
            #endif
            throw std::runtime_error("Could not find a matching constructor");
        }
        else if (forcing_config.provider == "NetCDFPerFeature")
        {
            std::shared_ptr<data_access::NetCDFPerFeatureDataProvider> fp;
            fp = std::make_unique<data_access::NetCDFPerFeatureDataProvider>(forcing_config.path.c_str(), output_stream);
            #ifdef NGEN_BMI_C_LIB_ACTIVE
            if (formulation_type == "bmi_c" )
                return std::make_shared<Bmi_C_Formulation>(identifier, std::move(fp), output_stream);
            #endif
            #ifdef NGEN_BMI_FORTRAN_ACTIVE
            if (formulation_type == "bmi_fortran" )
                return std::make_shared<Bmi_Fortran_Formulation>(identifier, std::move(fp), output_stream);
            #endif
            if (formulation_type == "bmi_multi" )
                return std::make_shared<Bmi_Multi_Formulation>(identifier, std::move(fp), output_stream);
            #ifdef ACTIVATE_PYTHON
            if (formulation_type == "bmi_python" )
                return std::make_shared<Bmi_Py_Formulation>(identifier, std::move(fp), output_stream);
            #endif
            if (formulation_type == "simple_lumped" )
                return std::make_shared<Simple_Lumped_Model_Realization>(identifier, std::move(fp), output_stream);
            throw std::runtime_error("Could not find a matching constructor");
        }
        else { // Some unknown string in the provider field?
            throw std::runtime_error(
                    "Invalid formulation forcing provider configuration! identifier: \"" + identifier +
                    "\", formulation_type: \"" + formulation_type +
                    "\", provider: \"" + forcing_config.provider + "\"");
        }
        
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
