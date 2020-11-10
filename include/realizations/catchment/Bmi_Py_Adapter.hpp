#ifndef NGEN_BMI_PY_ADAPTER_H
#define NGEN_BMI_PY_ADAPTER_H

#ifdef ACTIVATE_PYTHON

#include <exception>
#include <memory>
#include <string>
#include "bmi.hxx"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/numpy.h"
#include "JSONProperty.hpp"
#include "StreamHandler.hpp"

namespace py = pybind11;

namespace models {
    namespace bmi {

        // TODO: look at making this inherit from the BMI C++ interface to ensure all methods are implemented
        /**
         * An adapter class to serve as a C++ interface to the aspects of external models written in the Python
         * language that implement the BMI.
         */
        class Bmi_Py_Adapter {

            // TODO: implement needed the BMI methods and translate to Python calls

        public:

            explicit Bmi_Py_Adapter(const string& type_name, utils::StreamHandler output);

            Bmi_Py_Adapter(const string &type_name, std::string bmi_init_config, utils::StreamHandler output);

            Bmi_Py_Adapter(const string &type_name, const geojson::JSONProperty& other_input_vars,
                           utils::StreamHandler output);

            Bmi_Py_Adapter(const string &type_name, const std::string& bmi_init_config,
                           const geojson::JSONProperty& other_input_vars, utils::StreamHandler output);

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
             * Initialize the wrapped BMI model object using the value from the `bmi_init_config` member variable and
             * the object's ``Initialize`` function.
             *
             * If no attempt to initialize the model has yet been made (i.e., ``model_initialized`` is ``false`` when
             * this function is called), then ``model_initialized`` is set to ``true`` and initialization is attempted
             * for the model object. If initialization fails, an exception will be raised, with it's type and message
             * saved as part of this object's state.
             *
             * If an attempt to initialize the model has already been made (i.e., ``model_initialized`` is ``true``),
             * this function will either simply return or will throw a runtime_error, with the message listing the
             * type and message of the exception from the earlier attempt.
             */
            void Initialize();

            /**
             * Initialize the wrapped BMI model object using the given config file and the object's ``Initialize``
             * function.
             *
             * If the given file is not the same as what is in `bmi_init_config`` and the model object has not already
             * been initialized, this function will produce a warning message about the difference, then subsequently
             * update `bmi_init_config`` to the given file.  It will then proceed with initialization.
             *
             * However, if the given file is not the same as what is in `bmi_init_config``, but the model has already
             * been initialized, a runtime_error exception is thrown.
             *
             * This otherwise operates using the logic of ``Initialize()``.
             *
             * @param config_file
             * @see Initialize()
             * @throws runtime_error If already initialized but using a different file than the passed argument.
             */
            void Initialize(const std::string& config_file);

            /**
             * Parse variable value(s) from within "other_vars" property of formulation config, to a numpy array suitable for
             * passing to the BMI model via the ``set_value`` function.
             *
             * @param other_value_json JSON node containing variable/parameter value(s) needing to be passed to a BMI model.
             * @return A bound Python numpy array to containing values to pass to a BMI model object via ``set_value``.
             */
            static py::array parse_other_var_val_for_setter(const geojson::JSONProperty& other_value_json);

        private:

            /** Path (as a string) to the BMI config file for initializing the Python model object (empty if none). */
            std::string bmi_init_config;
            std::string init_exception_msg;
            /** A pointer to the Python BMI model object. */
            std::shared_ptr<py::object> py_bmi_model_obj;
            /** A pointer to a string with the parent package name of the Python type referenced by ``py_bmi_type_ref``. */
            std::shared_ptr<std::string> py_bmi_type_package_name;
            /** A pointer to a reference to the Python type that provides the BMI interface. */
            std::shared_ptr<py::object> py_bmi_type_ref;
            /** A pointer to a string with the simple name of the Python type referenced by ``py_bmi_type_ref``. */
            std::shared_ptr<std::string> py_bmi_type_simple_name;
            /** Whether the model object has been initialized yet, which is always initially ``false``. */
            bool model_initialized = false;
            utils::StreamHandler output;

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
