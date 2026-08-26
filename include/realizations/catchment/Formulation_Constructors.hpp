#ifndef NGEN_FORMULATION_CONSTRUCTORS_H
#define NGEN_FORMULATION_CONSTRUCTORS_H

#include <NGenConfig.h>

#include "Formulation.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

// Formulations
#include "Bmi_Formulation.hpp"
#include <GenericDataProvider.hpp>
#include "CsvPerFeatureForcingProvider.hpp"
#include "NullForcingProvider.hpp"
#if NGEN_WITH_NETCDF
    #include "NetCDFPerFeatureDataProvider.hpp"
#endif

namespace realization {
    using constructor = std::shared_ptr<Catchment_Formulation> (*)(std::string, std::shared_ptr<data_access::GenericDataProvider>, utils::StreamHandler);

    extern std::map<std::string, constructor> formulation_constructors;

    static std::string valid_formulation_keys(){
        std::string keys = "";
        for(const auto& kv : formulation_constructors){
            keys.append(kv.first+" ");
        }
        return keys;
    }

    static bool formulation_exists(std::string formulation_type) {
        return formulation_constructors.count(formulation_type) > 0;
    }

    std::shared_ptr<Catchment_Formulation> construct_formulation(
        std::string formulation_type,
        std::string identifier,
        forcing_params &forcing_config,
        utils::StreamHandler output_stream);
}

#endif // NGEN_FORMULATION_CONSTRUCTORS_H
