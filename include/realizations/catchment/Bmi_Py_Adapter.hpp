#ifndef NGEN_BMI_PY_ADAPTER_H
#define NGEN_BMI_PY_ADAPTER_H

#ifdef ACTIVATE_PYTHON

#include <exception>
#include <memory>
#include <string>
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/numpy.h"
#include "JSONProperty.hpp"
#include "StreamHandler.hpp"
#include "boost/algorithm/string.hpp"
#include "Bmi_Adapter.hpp"

namespace py = pybind11;

using namespace std;

namespace models {
    namespace bmi {

        // TODO: look at making this inherit from the BMI C++ interface to ensure all methods are implemented
        /**
         * An adapter class to serve as a C++ interface to the aspects of external models written in the Python
         * language that implement the BMI.
         */
        class Bmi_Py_Adapter : public Bmi_Adapter<py::object> {

            // TODO: implement needed the BMI methods and translate to Python calls

        public:

            /*
            explicit Bmi_Py_Adapter(const string& type_name, utils::StreamHandler output);

            Bmi_Py_Adapter(const string &type_name, std::string bmi_init_config, bool allow_exceed_end,
                           bool has_fixed_time_step, utils::StreamHandler output);

            Bmi_Py_Adapter(const string &type_name, bool allow_exceed_end, bool has_fixed_time_step,
                           const geojson::JSONProperty& other_input_vars, utils::StreamHandler output);
            */

            Bmi_Py_Adapter(const string &type_name, std::string bmi_init_config, bool allow_exceed_end,
                           bool has_fixed_time_step, const geojson::JSONProperty& other_input_vars,
                           utils::StreamHandler output);

            Bmi_Py_Adapter(const string &type_name, std::string  bmi_init_config, std::string forcing_file_path,
                           bool allow_exceed_end, bool has_fixed_time_step,
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
             * The number of input variables the model can use from other models implementing a BMI.
             *
             * @return The number of input variables the model can use from other models implementing a BMI.
             */
            int get_input_item_count();

            /**
             * Gets a collection of names for the variables the model can use from other models implementing a BMI.
             *
             * @return A vector of names for the variables the model can use from other models implementing a BMI.
             */
            std::vector<std::string> get_input_var_names();

            /**
             * Parse variable value(s) from within "other_vars" property of formulation config, to a numpy array suitable for
             * passing to the BMI model via the ``set_value`` function.
             *
             * @param other_value_json JSON node containing variable/parameter value(s) needing to be passed to a BMI model.
             * @return A bound Python numpy array to containing values to pass to a BMI model object via ``set_value``.
             */
            static py::array parse_other_var_val_for_setter(const geojson::JSONProperty& other_value_json);

            int GetInputItemCount() override;

            vector<std::string> GetInputVarNames() override;

            int GetOutputItemCount() override;

            vector<std::string> GetOutputVarNames() override;

            void Update() override;

            void UpdateUntil(double time) override;

            double GetCurrentTime() override;

            double GetEndTime() override;

            double GetStartTime() override;

            string GetVarType(std::string name) override;

            string GetVarUnits(std::string name) override;

            int GetVarGrid(std::string name) override;

            int GetVarItemsize(std::string name) override;

            int GetVarNbytes(std::string name) override;

            string GetVarLocation(std::string name) override;

            void GetValue(std::string name, void *dest) override;

            template<typename T>
            void copy_from_numpy_array(const py::array_t<float, py::array::c_style>& np_array, T* dest) {
                auto direct_array_access = np_array.unchecked<1>();
                for (int i = 0; i < np_array.size(); i++) {
                    dest[i] = (T)(direct_array_access[i]);
                }
            }

        protected:
            std::string model_name = "BMI Python model";

            /**
             * Construct the backing BMI model object, then call its BMI-native ``Initialize()`` function.
             *
             * Implementations should return immediately without taking any further action if ``model_initialized`` is
             * already ``true``.
             *
             * The call to the BMI native ``Initialize(string)`` should pass the value stored in ``bmi_init_config``.
             */
            void construct_and_init_backing_model() override {
                if (model_initialized)
                    return;
                try {
                    separate_package_and_simple_name();
                    // This is a class object for the BMI module Python class
                    py::object bmi_py_class = py::module_::import(py_bmi_type_package_name->c_str()).attr(
                            py_bmi_type_simple_name->c_str());
                    // This is the actual backing model object
                    bmi_model = make_shared<py::object>(bmi_py_class());
                    bmi_model->attr("Initialize")(bmi_init_config);
                }
                    // Record the exception message before re-throwing to handle subsequent function calls properly
                catch (exception& e) {
                    init_exception_msg = string(e.what());
                    // Make sure this is non-empty to be consistent with the above logic
                    if (init_exception_msg.empty()) {
                        init_exception_msg = "Unknown Python model initialization exception.";
                    }
                    throw e;
                }
            }


        private:

            /** Fully qualified Python type name for backing module. */
            string bmi_py_type_name;
            /** A binding to the Python numpy package/module. */
            py::module_ np;
            /** A pointer to a string with the parent package name of the Python type referenced by ``py_bmi_type_ref``. */
            shared_ptr<string> py_bmi_type_package_name;
            /** A pointer to a string with the simple name of the Python type referenced by ``py_bmi_type_ref``. */
            shared_ptr<string> py_bmi_type_simple_name;

            inline void separate_package_and_simple_name() {
                if (!model_initialized) {
                    vector<string> split_name;
                    string delimiter = ".";

                    size_t pos = 0;
                    string token;
                    while ((pos = bmi_py_type_name.find(delimiter)) != string::npos) {
                        token = bmi_py_type_name.substr(0, pos);
                        split_name.emplace_back(token);
                        bmi_py_type_name.erase(0, pos + delimiter.length());
                    }

                    py_bmi_type_simple_name = make_shared<string>(split_name.back());
                    split_name.pop_back();
                    py_bmi_type_package_name = make_shared<string>(boost::algorithm::join(split_name, delimiter));
                }
            }
        };

    }
}

#endif //ACTIVATE_PYTHON

#endif //NGEN_BMI_PY_ADAPTER_H
