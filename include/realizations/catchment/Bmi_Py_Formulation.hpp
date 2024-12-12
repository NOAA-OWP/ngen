#ifndef NGEN_BMI_PY_FORMULATION_H
#define NGEN_BMI_PY_FORMULATION_H

#include <NGenConfig.h>

#if NGEN_WITH_PYTHON

#include <memory>
#include <string>
#include "Bmi_Module_Formulation.hpp"
#include "Bmi_Py_Adapter.hpp"
#include "GenericDataProvider.hpp"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/numpy.h"

// Forward declaration to provide access to protected items in testing
class Bmi_Py_Formulation_Test;

namespace realization {

    class Bmi_Py_Formulation : public Bmi_Module_Formulation {

    public:

        Bmi_Py_Formulation(std::string id, std::shared_ptr<data_access::GenericDataProvider> forcing, utils::StreamHandler output_stream);

        const std::vector<std::string> get_bmi_input_variables() const override;

        const std::vector<std::string> get_bmi_output_variables() const override;

        std::string get_formulation_type() const override;

        bool is_bmi_input_variable(const std::string &var_name) const override;

        bool is_bmi_output_variable(const std::string &var_name) const override;

    protected:

        std::shared_ptr<models::bmi::Bmi_Adapter> construct_model(const geojson::PropertyMap &properties) override;

        time_t convert_model_time(const double &model_time) const override;

        double get_var_value_as_double(const int &index, const std::string &var_name) override;

        /**
         * Test whether backing model has run BMI ``Initialize``.
         *
         * Test whether the backing model object has been initialize using the BMI standard ``Initialize`` function.
         *
         * This overrides the super class implementation and checks the model directly.  As such, the associated setter
         * does not serve any purpose.
         *
         * @return Whether backing model object has been initialize using the BMI standard ``Initialize`` function.
         */
        bool is_model_initialized() const override;

        // Unit test access
        friend class ::Bmi_Formulation_Test;
        friend class ::Bmi_Py_Formulation_Test;
        friend class ::Bmi_Multi_Formulation_Test;
    };

}

#endif //NGEN_WITH_PYTHON

#endif //NGEN_BMI_PY_FORMULATION_H
