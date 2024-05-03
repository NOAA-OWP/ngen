#include <forcing/ForcingsEngineDataProvider.hpp>
#include <utilities/python/InterpreterUtil.hpp>

void data_access::assert_forcings_engine_requirements()
{
    // Check that the python module is installed.
    {
        auto interpreter_ = utils::ngenPy::InterpreterUtil::getInstance();
        try {
            auto mod = interpreter_->getModule("NextGen_Forcings_Engine");
            auto cls = mod.attr("BMIForcingsEngine").cast<py::object>();
        } catch(std::exception& e) {
            throw std::runtime_error{
                "Failed to initialize ForcingsEngine: ForcingsEngine python module is not installed or is not properly configured. (" + std::string{e.what()} + ")"
            };
        }
    }

    // Check that the WGRIB2 environment variable is defined
    {
        const auto* wgrib2_exec = std::getenv("WGRIB2");
        if (wgrib2_exec == nullptr) {
            throw std::runtime_error{"Failed to initialize ForcingsEngine: $WGRIB2 is not defined"};
        }
    }
}
