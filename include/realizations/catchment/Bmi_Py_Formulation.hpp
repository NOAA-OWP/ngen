#ifndef NGEN_BMI_PY_FORMULATION_H
#define NGEN_BMI_PY_FORMULATION_H

#ifdef ACTIVATE_PYTHON

#include <memory>
#include <string>
#include "Bmi_Formulation.hpp"
#include "Bmi_Py_Adapter.hpp"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/numpy.h"

#define CFG_PROP_KEY_CONFIG_FILE "config_file"
#define CFG_PROP_KEY_OTHER_INPUT_VARS "other_variables"

#define CFG_REQ_PROP_KEY_MODEL_TYPE "model_type"

namespace realization {

    class Bmi_Py_Formulation : public Bmi_Formulation<models::bmi::Bmi_Py_Adapter> {

    public:
        Bmi_Py_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream);

        virtual ~Bmi_Py_Formulation();

        void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override;

        void create_formulation(geojson::PropertyMap properties) override;

        std::string get_formulation_type() override;

    protected:

        void protected_create_formulation(geojson::PropertyMap properties, bool params_need_validation);

    private:

        std::vector<std::string> REQUIRED_PARAMETERS = {
                CFG_REQ_PROP_KEY_MODEL_TYPE
        };

    };

}

#endif //ACTIVATE_PYTHON

#endif //NGEN_BMI_PY_FORMULATION_H
