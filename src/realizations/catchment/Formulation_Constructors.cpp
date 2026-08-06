#include <NGenConfig.h>

#include "Formulation.hpp"
#include <JSONProperty.hpp>
#include <exception>

#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

// Formulations
#include "Formulation_Constructors.hpp"
#include "Bmi_Cpp_Formulation.hpp"
#include "Bmi_C_Formulation.hpp"
#include "Bmi_Fortran_Formulation.hpp"
#include "Bmi_Multi_Formulation.hpp"
#include "Bmi_Py_Formulation.hpp"
#include <GenericDataProvider.hpp>
#include "CsvPerFeatureForcingProvider.hpp"
#include "NullForcingProvider.hpp"
#if NGEN_WITH_NETCDF
    #include "NetCDFPerFeatureDataProvider.hpp"
#endif

namespace realization {
    template<class T>
    static constructor create_formulation_constructor() {
        return [](std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing_provider, utils::StreamHandler output_stream) -> std::shared_ptr<Catchment_Formulation>{
            return std::make_shared<T>(id, forcing_provider, output_stream);
        };
    }

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

    std::shared_ptr<Catchment_Formulation> construct_formulation(
        std::string formulation_type,
        std::string identifier,
        forcing_params &forcing_config,
        utils::StreamHandler output_stream
    ) {
        constructor formulation_constructor = formulation_constructors.at(formulation_type);

        std::shared_ptr<data_access::GenericDataProvider> fp;
        if (forcing_config.provider == "CsvPerFeature" || forcing_config.provider == ""){
            fp = std::make_shared<CsvPerFeatureForcingProvider>(forcing_config);
        }
#if NGEN_WITH_NETCDF
        else if (forcing_config.provider == "NetCDF"){
            // Note: The stream mechanics of the formulations and formulation manager are
            // are strictly speaking indepdent of the log output stream here.  The "default" output
            // stream likely coming into this function is the null stream, but we don't want to force the forcing provider
            // to also use the null stream for any logging it may do,
            // so we use the standard output stream for the forcing provider by default.
            // TODO: this likely needs to be rethought and refactored, but for now,
            // this allows the NetCDF provider to log to standard output while still allowing
            // formulations to log to their own output streams as needed.
            std::shared_ptr<data_access::NetCDFPerFeatureDataProvider> f;
            f = data_access::NetCDFPerFeatureDataProvider::get_shared_provider(forcing_config.path, forcing_config.simulation_start_t, forcing_config.simulation_end_t, utils::getStdOut());
            // TODO: this is not _ideal_ but implements the idea.
            // refactor in the future.
            f->hint_shared_provider_id(identifier);
            fp = f;
        }
#endif
        else if (forcing_config.provider == "NullForcingProvider"){
            fp = std::make_shared<NullForcingProvider>();
        }
        else { // Some unknown string in the provider field?
            throw std::runtime_error(
                    "Invalid formulation forcing provider configuration! identifier: \"" + identifier +
                    "\", formulation_type: \"" + formulation_type +
                    "\", provider: \"" + forcing_config.provider + "\"");
        }
        return formulation_constructor(identifier, fp, output_stream);
    }

    std::map<std::string, constructor> formulation_constructors = {
        {"bmi_c++", create_formulation_constructor<Bmi_Cpp_Formulation>()},
#if NGEN_WITH_BMI_C
        {"bmi_c", create_formulation_constructor<Bmi_C_Formulation>()},
#endif // NGEN_WITH_BMI_C
#if NGEN_WITH_BMI_FORTRAN
        {"bmi_fortran", create_formulation_constructor<Bmi_Fortran_Formulation>()},
#endif // NGEN_WITH_BMI_FORTRAN
        {"bmi_multi", create_formulation_constructor<Bmi_Multi_Formulation>()},
#if NGEN_WITH_PYTHON
        {"bmi_python", create_formulation_constructor<Bmi_Py_Formulation>()},
#endif // NGEN_WITH_PYTHON
    };
}
