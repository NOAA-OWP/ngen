#ifndef NGEN_BMI_PY_FORMULATION_H
#define NGEN_BMI_PY_FORMULATION_H

#ifdef ACTIVATE_PYTHON

#include <memory>
#include <string>
#include "Bmi_Module_Formulation.hpp"
#include "Bmi_Py_Adapter.hpp"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/numpy.h"

namespace realization {

    class Bmi_Py_Formulation : public Bmi_Module_Formulation<models::bmi::Bmi_Py_Adapter> {

    // TODO: Type is not fully implemented.  Complete before using.

    public:
        Bmi_Py_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream);

        virtual ~Bmi_Py_Formulation();


        std::string get_formulation_type() override;

        bool is_bmi_input_variable(const string &var_name) override;

        bool is_bmi_output_variable(const string &var_name) override;

    };

}

#endif //ACTIVATE_PYTHON

#endif //NGEN_BMI_PY_FORMULATION_H
