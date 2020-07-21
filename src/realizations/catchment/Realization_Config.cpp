#include "Realization_Config.hpp"

#include "Tshirt_Realization.hpp"
#include <Simple_Lumped_Model_Realization.hpp>
#include "tshirt_params.h"
#include <iostream>
#include <regex>
#include <dirent.h>

using namespace realization;

std::shared_ptr<Simple_Lumped_Model_Realization> Realization_Config_Base::get_simple_lumped() {
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

    double seconds_to_day = 3600.0/86400.0;

    double storage = this->get_option("storage").as_real_number();
    double max_storage = this->get_option("max_storage").as_real_number();
    double a = this->get_option("a").as_real_number();
    double b = this->get_option("b").as_real_number();
    double Ks = this->get_option("Ks").as_real_number() * seconds_to_day; //Implicitly connected to time used for DAILY dt need to account for hourly dt
    double Kq = this->get_option("Kq").as_real_number() * seconds_to_day; //Implicitly connected to time used for DAILY dt need to account for hourly dt
    long n = this->get_option("n").as_natural_number();
    double t = this->get_option("t").as_real_number();

    std::vector<double> sr_tmp = {1.0, 1.0, 1.0};
    return std::make_shared<Simple_Lumped_Model_Realization>(
        Simple_Lumped_Model_Realization(
            this->get_forcing_parameters(),
            storage,
            max_storage,
            a,
            b,
            Ks,
            Kq,
            n,
            sr_tmp,
            t
        )
    );
}

std::shared_ptr<Tshirt_Realization> Realization_Config_Base::get_tshirt() {
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

    tshirt::tshirt_params tshirt_params{
        this->get_option("maxsmc").as_real_number(),   //maxsmc FWRFH
        this->get_option("wltsmc").as_real_number(),  //wltsmc  from fred_t-shirt.c FIXME NOT USED IN TSHIRT?!?!
        this->get_option("satdk").as_real_number(),   //satdk FWRFH
        this->get_option("satpsi").as_real_number(),    //satpsi    FIXME what is this and what should its value be?
        this->get_option("slope").as_real_number(),   //slope
        this->get_option("scaled_distribution_fn_shape_parameter").as_real_number(),      //b bexp? FWRFH
        this->get_option("multiplier").as_real_number(),    //multipier  FIXMME (lksatfac)
        this->get_option("alpha_fc").as_real_number(),    //aplha_fc   field_capacity_atm_press_fraction
        this->get_option("Klf").as_real_number(),    //Klf lateral flow nash coefficient?
        this->get_option("Kn").as_real_number(),    //Kn Kn	0.001-0.03 F Nash Cascade coeeficient
        static_cast<int>(this->get_option("nash_n").as_natural_number()),      //number_lateral_flow_nash_reservoirs
        this->get_option("Cgw").as_real_number(),    //fred_t-shirt gw res coeeficient (per h)
        this->get_option("expon").as_real_number(),    //expon FWRFH
        this->get_option("max_groundwater_storage_meters").as_real_number()   //max_gw_storage Sgwmax FWRFH
    };

    double soil_storage_meters = tshirt_params.max_soil_storage_meters * this->get_option("soil_storage_percentage").as_real_number();
    double ground_water_storage = tshirt_params.max_groundwater_storage_meters * this->get_option("groundwater_storage_percentage").as_real_number();
    return std::make_shared<Tshirt_Realization>(        
            this->get_forcing_parameters(),
            soil_storage_meters, //soil_storage_meters
            ground_water_storage, //groundwater_storage_meters
            this->id, //used to cross-reference the COMID, need to look up the catchments GIUH data
            *this->get_giuh_reader(),     //used to actually lookup GIUH data and create a giuh_kernel obj for catchment
            tshirt_params,
            this->get_option("nash_storage").as_real_vector(),
            this->get_option("timestep").as_natural_number()
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

    std::string path = this->forcing_parameters.at("path").as_string();

    if (this->forcing_parameters.count("file_pattern") == 0) {
        return forcing_params(
            path,
            this->forcing_parameters.at("start_time").as_string(),
            this->forcing_parameters.at("end_time").as_string()
        );
    }

    // Since we are given a pattern, we need to identify the directory and pull out anything that matches the pattern
    if (path.compare(path.size() - 1, 1, "/") != 0) {
        path += "/";
    }

    std::string filepattern = this->forcing_parameters.at("file_pattern").as_string();

    int id_index = filepattern.find("{{ID}}");

    // If an index for '{{ID}}' was found, we can count on that being where the id for this realization can be found.
    //     For instance, if we have a pattern of '.*{{ID}}_14_15.csv' and this is named 'cat-87',
    //     this will match on 'stuff_example_cat-87_14_15.csv'
    if (id_index != std::string::npos) {
        filepattern = filepattern.replace(id_index, sizeof("{{ID}}") - 1, this->id);
    }

    // Create a regular expression used to identify proper file names
    std::regex pattern(filepattern);

    // A stream providing the functions necessary for evaluating a directory: 
    //    https://www.gnu.org/software/libc/manual/html_node/Opening-a-Directory.html#Opening-a-Directory
    DIR *directory = nullptr;

    // structure representing the member of a directory: https://www.gnu.org/software/libc/manual/html_node/Directory-Entries.html
    struct dirent *entry = nullptr;

    // Attempt to open the directory for evaluation
    directory = opendir(path.c_str());

    // If the directory could be found, we can go ahead and iterate
    if (directory != nullptr) {
        while ((entry = readdir(directory))) {
            // If the entry is a regular file or symlink AND the name matches the pattern, 
            //    we can consider this ready to be interpretted as valid forcing data (even if it isn't)
            if ((entry->d_type == DT_REG or entry->d_type == DT_LNK) and std::regex_match(entry->d_name, pattern)) {
                return forcing_params(
                    path + entry->d_name,
                    this->forcing_parameters.at("start_time").as_string(),
                    this->forcing_parameters.at("end_time").as_string()
                );
            }
        }
    }
    else {
        // The directory wasn't found; forcing data cannot be retrieved
        throw std::runtime_error("No directory for forcing data was found at: " + path);
    }

    closedir(directory);

    throw std::runtime_error("Forcing data could not be found for '" + this->id + "'");
}

double Realization_Config_Base::get_response(double input_flux, time_step_t t, time_step_t dt, void* et_params) {
    return this->get_realization()->get_response(input_flux, t, dt, et_params);
}

std::shared_ptr<HY_CatchmentRealization> Realization_Config_Base::get_realization() {
    if (this->realization == NULL) {
        switch (this->realization_type)
        {
            case Realization_Type::TSHIRT:
                this->realization = this->get_tshirt();
                return this->realization;
            case Realization_Type::SIMPLE_LUMPED:
                this->realization = this->get_simple_lumped();
                return this->realization;
            default:
                throw std::runtime_error("There is not a model connected for '" + get_realization_type_name(this->get_realization_type()) + "'");
        }
    }

    return this->realization;
}