#include "Realization_Config.hpp"

#include "Tshirt_Realization.hpp"
#include <Simple_Lumped_Model_Realization.hpp>
#include "tshirt_params.h"
#include <iostream>
#include <regex>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

using namespace realization;

namespace fs = boost::filesystem;

std::unique_ptr<Simple_Lumped_Model_Realization> Realization_Config_Base::get_simple_lumped() {
    std::vector<std::string> missing_parameters;
    for(std::string parameter : REQUIRED_HYMOD_PARAMETERS) {
        if (not this->has_option(parameter)) {
            missing_parameters.push_back(parameter);
        }
    }

    if (missing_parameters.size() > 0) {
        std::string message = "A Simple Lumped realization cannot be created for '" + this->id + "'; the following parameters are missing: ";

        for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
            message += missing_parameters[missing_parameter_index];

            if (missing_parameter_index < missing_parameters.size() - 1) {
                message += ", ";
            }
        }
        
        throw std::runtime_error(message);
    }

    return std::make_unique<Simple_Lumped_Model_Realization>(
        Simple_Lumped_Model_Realization(
            this->get_forcing_parameters(),
            this->options.at("storage").as_real_number(),
            this->options.at("max_storage").as_real_number(),
            this->options.at("a").as_real_number(),
            this->options.at("b").as_real_number(),
            this->options.at("Ks").as_real_number(),
            this->options.at("Kq").as_real_number(),
            this->options.at("n").as_natural_number(),
            this->options.at("sr").as_real_vector(),
            this->options.at("t").as_real_number()
        )
    );
}

std::unique_ptr<Tshirt_Realization> Realization_Config_Base::get_tshirt() {
    std::vector<std::string> missing_parameters;
    for(std::string parameter : REQUIRED_TSHIRT_PARAMETERS) {
        if (not this->has_option(parameter)) {
            missing_parameters.push_back(parameter);
        }
    }

    if (missing_parameters.size() > 0) {
        std::string message = "A TShirt realization cannot be created for '" + this->id + "'; the following parameters are missing: ";

        for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
            message += missing_parameters[missing_parameter_index];

            if (missing_parameter_index < missing_parameters.size() - 1) {
                message += ", ";
            }
        }

        throw std::runtime_error(message);
    }

    return std::make_unique<Tshirt_Realization>(        
        this->get_forcing_parameters(),
        this->options.at("soil_storage_meters").as_real_number(),
        this->options.at("groundwater_storage_meters").as_real_number(),
        this->id,
        *this->get_giuh_reader(),
        this->options.at("maxsmc").as_real_number(),
        this->options.at("wltsmc").as_real_number(),
        this->options.at("satdk").as_real_number(),
        this->options.at("satpsi").as_real_number(),
        this->options.at("slope").as_real_number(),
        this->options.at("scaled_distribution_fn_shape_parameter").as_real_number(),
        this->options.at("multiplier").as_real_number(),
        this->options.at("alpha_fc").as_real_number(),
        this->options.at("Klf").as_real_number(),
        this->options.at("Kn").as_real_number(),
        this->options.at("nash_n").as_natural_number(),
        this->options.at("Cgw").as_real_number(),
        this->options.at("expon").as_real_number(),
        this->options.at("max_groundwater_storage_meters").as_real_number(),
        this->options.at("nash_storage").as_real_vector(),
        this->options.at("timestep").as_natural_number()
    );
}

std::shared_ptr<giuh::GiuhJsonReader> Realization_Config_Base::get_giuh_reader() {
    if (not this->has_giuh()) {
        throw std::runtime_error("There is no configuration for giuh");
    }

    std::vector<std::string> missing_parameters;

    if (this->giuh.count("giuh_path") == 0) {
        missing_parameters.push_back("giuh_path");
    }

    if (this->giuh.count("crosswalk_path") == 0) {
        missing_parameters.push_back("crosswalk_path");
    }

    if (missing_parameters.size() > 0) {
        std::string message = "A giuh configuration cannot be created for '" + this->id + "'; the following parameters are missing: ";

        for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
            message += missing_parameters[missing_parameter_index];

            if (missing_parameter_index < missing_parameters.size() - 1) {
                message += ", ";
            }
        }
        
        throw std::runtime_error(message);
    }

    return std::make_shared<giuh::GiuhJsonReader>(
        this->giuh.at("giuh_path").as_string(),
        this->giuh.at("crosswalk_path").as_string()
    );
}

bool Realization_Config_Base::has_option(std::string option_name) const {
    return this->options.count(option_name) == 1;
}

geojson::JSONProperty Realization_Config_Base::get_option(std::string option_name) const {
    if(not this->has_option(option_name)) {
        std::string message = "There is no option named '" + option_name + "'";
        throw std::runtime_error(message);
    }

    return this->options.at(option_name);
}

bool Realization_Config_Base::has_giuh() {
    return not this->giuh.empty();
}

std::string Realization_Config_Base::get_id() const {
    return this->id;
}

bool Realization_Config_Base::has_forcing() {
    return not this->forcing_parameters.empty();
}

forcing_params Realization_Config_Base::get_forcing_parameters() {
    if (not this->has_forcing()) {
        throw std::runtime_error("There is no configuration for forcing");
    }

    std::vector<std::string> missing_parameters;
    
    if (this->forcing_parameters.count("path") == 0) {
        missing_parameters.push_back("path");
    }

    if (this->forcing_parameters.count("start_time") == 0) {
        missing_parameters.push_back("start_time");
    }

    if (this->forcing_parameters.count("end_time") == 0) {
        missing_parameters.push_back("end_time");
    }

    if (missing_parameters.size() > 0) {
        std::string message = "A forcing configuration cannot be created for '" + this->id + "'; the following parameters are missing: ";

        for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
            message += missing_parameters[missing_parameter_index];

            if (missing_parameter_index < missing_parameters.size() - 1) {
                message += ", ";
            }
        }
        
        throw std::runtime_error(message);
    }

    fs::path path(this->forcing_parameters.at("path").as_string());

    if (fs::exists(path)) {
        if (fs::is_regular_file(path)) {
            return forcing_params(
                this->forcing_parameters.at("path").as_string(),
                this->forcing_parameters.at("start_time").as_string(),
                this->forcing_parameters.at("end_time").as_string()
            );
        }

        std::string filepattern = this->forcing_parameters.at("file_pattern").as_string();
        int id_index = filepattern.find("{{ID}}");

        if (id_index != std::string::npos) {
            filepattern = filepattern.replace(id_index, sizeof("{{ID}}") - 1, this->id);
        }

        std::regex pattern(filepattern);
        for(auto& entry : boost::make_iterator_range(fs::directory_iterator(path), {})) {
            
            if (std::regex_match(entry.path().string().c_str(), pattern)) {
                return forcing_params(
                    entry.path().string(),
                    this->forcing_parameters.at("start_time").as_string(),
                    this->forcing_parameters.at("end_time").as_string()
                );
            }
        }

        throw std::runtime_error("Forcing data could not be found for '" + this->id + "'");
    }

    throw std::runtime_error("Forcing data could not be found for '" + this->id + "' as '" + path.string() + "'");
}