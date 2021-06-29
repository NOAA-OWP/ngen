#ifdef ACTIVATE_PYTHON

#include "Bmi_Py_Formulation.hpp"

using namespace realization;
using namespace models::bmi;

using namespace pybind11::literals;

void Bmi_Py_Formulation::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
    // In this case, we know interpret_parameters() will call call validate parameters, so they don't need re-validating
    protected_create_formulation(this->interpret_parameters(config, global), false);
}

void Bmi_Py_Formulation::create_formulation(geojson::PropertyMap properties) {
    protected_create_formulation(properties, true);
}

std::string Bmi_Py_Formulation::get_formulation_type() {
    return "bmi_py";
}

void Bmi_Py_Formulation::protected_create_formulation(geojson::PropertyMap properties, bool params_need_validation) {
    // TODO: see comment in Tshirt_C_Realization about how a factory might be better.
    if (params_need_validation) {
        this->validate_parameters(properties);
    }

    // Read the configured name of the model type and use it to get a reference for the Python BMI model type/class
    // TODO: document this needs to be a fully qualified name for the Python BMI model type
    std::string py_class = properties.at(CFG_REQ_PROP_KEY_MODEL_TYPE).as_string();

    // Get configured BMI config file path, if available, to provide to BMI initialize function
    // TODO: document requirements around this,
    auto config_file_property = properties.find(CFG_PROP_KEY_CONFIG_FILE);
    std::string config_file;
    if (config_file_property != properties.end()) {
        config_file = config_file_property->second.as_string();
    }

    // Get the child map of any other variables that will be manually/individually set after BMI initialize function
    // TODO: document that any listed input variable is allowed to do this
    auto it_other_vars = properties.find(CFG_PROP_KEY_OTHER_INPUT_VARS);
    if (it_other_vars != properties.end()) {
        // TODO: consider adding error message if something in list other than input variables (output and/or unrecognized)
        set_bmi_model(std::make_shared<Bmi_Py_Adapter>(Bmi_Py_Adapter(py_class, config_file, it_other_vars->second)));
    }
    else {
        set_bmi_model(std::make_shared<Bmi_Py_Adapter>(Bmi_Py_Adapter(py_class, config_file)));
    }
}

#endif //ACTIVATE_PYTHON
