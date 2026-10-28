#include <Formulation.hpp>

namespace realization {

    // `id` names the formulation, `feature_id` names the feature it is associated with.
    // Both start from the one string the constructor is given, because that string is the feature
    // wherever a formulation is built directly: a catchment formulation is constructed with its
    // catchment id, a domain layer with its layer id.
    //
    // Bmi_Multi_Formulation is the exception. It constructs each nested module with an id locating
    // that module within itself, so there is no feature in the string to start from; it assigns
    // the module its own feature through @ref set_feature_id.

    Formulation::Formulation(const std::string& id, std::shared_ptr<data_access::GenericDataProvider> forcing)
        : id(id)
        , feature_id(id)
        , forcing(std::move(forcing))
    { }

    Formulation::Formulation(const std::string& id)
        : Formulation(id, nullptr)
    { }

    void Formulation::finalize()
    {
        if (forcing) {
            forcing->finalize();
            forcing = nullptr;
        }
    }

    geojson::PropertyMap Formulation::interpret_parameters(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
        geojson::PropertyMap options;

        for (auto &formulation_parameter : config) {
            options.emplace(formulation_parameter.first, geojson::JSONProperty(formulation_parameter.first, formulation_parameter.second));
        }

        if (global != nullptr) {
            for(auto &global_option : *global) {
                if (options.count(global_option.first) == 0) {
                    options.emplace(global_option.first, global_option.second);
                }
            }
        }

        validate_parameters(options);

        return options;
    }

    void Formulation::validate_parameters(geojson::PropertyMap options) {
        std::vector<std::string> missing_parameters;
        std::vector<std::string> required_parameters = get_required_parameters();

        for (auto parameter : required_parameters) {
            if (options.count(parameter) == 0) {
                missing_parameters.push_back(parameter);
            }
        }

        if (missing_parameters.size() > 0) {
            std::string message = "A " + get_formulation_type() + " formulation cannot be created; the following parameters are missing: ";

            for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
                message += missing_parameters[missing_parameter_index];

                if (missing_parameter_index < missing_parameters.size() - 1) {
                    message += ", ";
                }
            }

            throw std::runtime_error(message);
        }
    }
} // namespace realization
