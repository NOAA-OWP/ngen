#ifndef CATCHMENT_FORMULATION_H
#define CATCHMENT_FORMULATION_H

#include <memory>

#include "Formulation.hpp"
#include <HY_CatchmentArea.hpp>

namespace realization {

    class Catchment_Formulation : public Formulation, public HY_CatchmentArea {
        public:
            Catchment_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream) : Formulation(id), HY_CatchmentArea(forcing_config, output_stream) {};

            Catchment_Formulation(std::string id) : Formulation(id){};

            virtual double get_response(double input_flux, time_step_t t, time_step_t dt, void* et_params) = 0;

            virtual std::string* get_required_parameters() = 0;
            
            virtual void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) = 0;

            virtual ~Catchment_Formulation(){};
    };
}
#endif // CATCHMENT_FORMULATION_H