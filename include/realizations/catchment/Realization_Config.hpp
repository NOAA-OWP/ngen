#ifndef REALIZATION_CONFIG_H
#define REALIZATION_CONFIG_H

#include <memory>
#include <string>
#include <map>
#include <exception>

#include "JSONProperty.hpp"
#include "Tshirt_Realization.hpp"
#include <Simple_Lumped_Model_Realization.hpp>
#include "tshirt_params.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>

namespace realization {
    class Realization_Config_Base;

    typedef std::shared_ptr<Realization_Config_Base> Realization_Config;


    enum Realization_Type {
        TSHIRT,
        SIMPLE_LUMPED,
        NONE
    };

    const std::string REQUIRED_TSHIRT_PARAMETERS[] = {
        "maxsmc",
        "wltsmc",
        "satdk",
        "satpsi",
        "slope",
        "scaled_distribution_fn_shape_parameter",
        "multiplier",
        "alpha_fc",
        "Klf",
        "Kn",
        "nash_n",
        "Cgw",
        "expon",
        "max_groundwater_storage_meters",
        "nash_storage",
        "soil_storage_percentage",
        "groundwater_storage_percentage",
        "timestep"
    };

    const std::string REQUIRED_HYMOD_PARAMETERS[] = {
        "sr",
        "storage",
        "max_storage",
        "a",
        "b",
        "Ks",
        "Kq",
        "n",
        "t"
    };

    static std::string get_realization_type_name(Realization_Type realization_type) {
        switch(realization_type) {
            case SIMPLE_LUMPED:
                return "simple_lumped";
            case TSHIRT:
                return "tshirt";
            default:
                return "none";
        }
    }

    static Realization_Type get_realization_type(std::string type_name) {
        std::string lowercase = boost::to_lower_copy(type_name);
        
        if (lowercase == "simple_lumped") {
            return Realization_Type::SIMPLE_LUMPED;
        }
        else if (lowercase == "tshirt") {
            return Realization_Type::TSHIRT;
        }

        return Realization_Type::NONE;
    }

    class Realization_Config_Base {
        public:            
            Realization_Config_Base(std::string id, boost::property_tree::ptree &config, Realization_Config global = nullptr) : id(id) {
                for (auto &child : config) {
                    std::string key = boost::to_lower_copy(child.first);

                    if (realization::get_realization_type(child.first) != Realization_Type::NONE) {
                        this->realization_type = realization::get_realization_type(key);

                        for (auto &realization_parameter : child.second) {
                            options.emplace(realization_parameter.first, geojson::JSONProperty(realization_parameter.first, realization_parameter.second));
                        }
                    }
                    else if(key == "giuh") {
                        for (auto &giuh_parameter : child.second) {
                            giuh.emplace(giuh_parameter.first, geojson::JSONProperty(giuh_parameter.first, giuh_parameter.second));
                        }
                    }
                    else if (key == "forcing") {
                        for (auto &forcing_parameter : child.second) {
                            forcing_parameters.emplace(forcing_parameter.first, geojson::JSONProperty(forcing_parameter.first, forcing_parameter.second));
                        }
                    }
                }

                if (global != nullptr) {
                    if(not this->has_forcing() && global->has_forcing()) {
                        this->forcing_parameters = global->forcing_parameters;
                    }

                    if (this->realization_type == Realization_Type::NONE && global->realization_type != Realization_Type::NONE) {
                        this->options = global->options;
                        this->realization_type = global->realization_type;
                    }
                }
            }

            Realization_Config assign_id(std::string new_id) {
                return std::make_shared<Realization_Config_Base>(Realization_Config_Base(
                    new_id,
                    this
                ));
            }

            Realization_Type get_realization_type() {
                return this->realization_type;
            }

            std::shared_ptr<HY_CatchmentRealization> get_realization();

            std::shared_ptr<Simple_Lumped_Model_Realization> get_simple_lumped();

            std::shared_ptr<Tshirt_Realization> get_tshirt();

            geojson::JSONProperty get_option(std::string option_name) const;

            forcing_params get_forcing_parameters();

            bool has_option(std::string option_name) const;

            bool has_giuh();

            bool has_forcing();

            std::string get_id() const;

            double get_response(double input_flux, time_step_t t, time_step_t dt, void* et_params);

        private:
            Realization_Config_Base(std::string new_id, Realization_Config_Base *original) {
                this->id = new_id;
                this->options = original->options;
                this->giuh = original->giuh;
                this->forcing_parameters = original->forcing_parameters;
                this->realization_type = original->realization_type;
            }

            std::shared_ptr<giuh::GiuhJsonReader> get_giuh_reader();
            geojson::PropertyMap options;
            Realization_Type realization_type = Realization_Type::NONE;
            geojson::PropertyMap forcing_parameters;
            geojson::PropertyMap giuh;
            std::string id;
            std::shared_ptr<HY_CatchmentRealization> realization = NULL;
    };

    static Realization_Config get_realizationconfig(std::string id, boost::property_tree::ptree &config, Realization_Config global = NULL) {
        return std::make_shared<Realization_Config_Base>(Realization_Config_Base(id, config, global));
    }
}

#endif // REALIZATION_CONFIG_H