#ifndef NGEN_BMI_PY_FORMULATION_H
#define NGEN_BMI_PY_FORMULATION_H

#ifdef ACTIVATE_PYTHON

#include <memory>
#include <string>
#include "Bmi_Formulation.hpp"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/numpy.h"

#define CFG_PROP_KEY_CONFIG_FILE "config_file"
#define CFG_PROP_KEY_MODEL_PYTHON_INIT_ARGS "model_python_init"
#define CFG_PROP_KEY_OTHER_INPUT_VARS "other_variables"

#define CFG_REQ_PROP_KEY_MODEL_TYPE "model_type"

namespace realization {

    class Bmi_Py_Formulation : public Bmi_Formulation<pybind11::object> {

    public:
        Bmi_Py_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream);

        virtual ~Bmi_Py_Formulation();

        void create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global = nullptr) override;

        void create_formulation(geojson::PropertyMap properties) override;

        std::string get_formulation_type() override;

        /**
         * Get the name of the parent package for the Python BMI model type.
         *
         * @return The name of the parent package for the Python BMI model type.
         */
        std::string get_bmi_type_package() const;

        /**
         * Get the simple name of the Python BMI model type.
         *
         * @return The simple name of the Python BMI model type.
         */
        std::string get_bmi_type_simple_name() const;

        /**
         * Parse variable value(s) from within "other_vars" property of formulation config, to a numpy array suitable for
         * passing to the BMI model via the ``set_value`` function.
         *
         * @param other_value_json JSON node containing variable/parameter value(s) needing to be passed to a BMI model.
         * @return A bound Python numpy array to containing values to pass to a BMI model object via ``set_value``.
         */
        static pybind11::array parse_other_var_val_for_setter(const geojson::JSONProperty& other_value_json);

    protected:

        void protected_create_formulation(geojson::PropertyMap properties, bool params_need_validation);

    private:

        std::vector<std::string> REQUIRED_PARAMETERS = {
                CFG_REQ_PROP_KEY_MODEL_TYPE
        };

        /** A pointer to a reference to the Python type that provides the BMI interface. */
        std::shared_ptr<pybind11::object> py_bmi_type_ref;
        /** A pointer to a string with the parent package name of the Python type referenced by ``py_bmi_type_ref``. */
        std::shared_ptr<std::string> py_bmi_type_package_name;
        /** A pointer to a string with the simple name of the Python type referenced by ``py_bmi_type_ref``. */
        std::shared_ptr<std::string> py_bmi_type_simple_name;

        /**
         * Create the internal BMI model object from the given config properties.
         *
         * Note that this function is intended to be called only as part of ``protected_create_formulation``.
         *
         * @param properties Externally provided config properties for the formulation
         */
        inline void create_bmi_model_object(geojson::PropertyMap &properties);

        /**
         * Initialize the reference to the Python BMI type, as stored in ``py_bmi_type_ref``, and set the member storing the
         * name of this type and its parent package as well.
         *
         * @param type_name The fully-qualified name of the reference to the Python BMI-implementing type.
         */
        inline void init_py_bmi_type(std::string type_name);

    };

}

#endif //ACTIVATE_PYTHON

#endif //NGEN_BMI_PY_FORMULATION_H
