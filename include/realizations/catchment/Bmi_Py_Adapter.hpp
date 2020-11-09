#ifndef NGEN_BMI_PY_ADAPTER_H
#define NGEN_BMI_PY_ADAPTER_H

#ifdef ACTIVATE_PYTHON

#include <string>
#include "bmi.hxx"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/numpy.h"
#include "JSONProperty.hpp"

namespace py = pybind11;

namespace models {
    namespace bmi {

        // TODO: look at making this inherit from the BMI C++ interface to ensure all methods are implemented
        /**
         * An adapter class to serve as a C++ interface to the aspects of external models written in the Python
         * language that implement the BMI.
         *
         * Instantiated objects will have initialized the wrapped Python BMI modeling using the ``Initialize`` function.
         */
        class Bmi_Py_Adapter {

            // TODO: implement needed the BMI methods and translate to Python calls

        public:

            explicit Bmi_Py_Adapter(const string& type_name);

            Bmi_Py_Adapter(const string &type_name, const std::string& bmi_init_config);

            Bmi_Py_Adapter(const string &type_name, const geojson::JSONProperty& other_input_vars);

            Bmi_Py_Adapter(const string &type_name, const std::string& bmi_init_config,
                           const geojson::JSONProperty& other_input_vars);

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
            static py::array parse_other_var_val_for_setter(const geojson::JSONProperty& other_value_json);

        private:

            /** A pointer to the Python BMI model object. */
            std::shared_ptr<py::object> py_bmi_model_obj;
            /** A pointer to a string with the parent package name of the Python type referenced by ``py_bmi_type_ref``. */
            std::shared_ptr<std::string> py_bmi_type_package_name;
            /** A pointer to a reference to the Python type that provides the BMI interface. */
            std::shared_ptr<py::object> py_bmi_type_ref;
            /** A pointer to a string with the simple name of the Python type referenced by ``py_bmi_type_ref``. */
            std::shared_ptr<std::string> py_bmi_type_simple_name;

            /**
             * Initialize the reference to the Python BMI type, as stored in ``py_bmi_type_ref``, and set the member
             * storing the name of this type and its parent package as well.
             *
             * @param type_name The fully-qualified name of the reference to the Python BMI-implementing type.
             */
            inline void init_py_bmi_type(std::string type_name);
        };

    }
}

#endif //ACTIVATE_PYTHON

#endif //NGEN_BMI_PY_ADAPTER_H
