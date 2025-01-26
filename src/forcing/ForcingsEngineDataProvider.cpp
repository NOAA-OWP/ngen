#include <forcing/ForcingsEngineDataProvider.hpp>
#include <utilities/python/InterpreterUtil.hpp>

#include <ctime> // timegm
#include <iomanip> // std::get_time
#include "Logger.hpp"

namespace data_access {
namespace detail {

// Initialize instance storage
ForcingsEngineStorage ForcingsEngineStorage::instances{};

time_t parse_time(const std::string& time, const std::string& fmt)
{
    std::tm tm_ = {};
    std::stringstream tmstr{time};
    tmstr >> std::get_time(&tm_, fmt.c_str());

    // Note: `timegm` is available for Linux and macOS via time.h, but not Windows.
    return timegm(&tm_);
}

void assert_forcings_engine_requirements()
{
    // Check that the python module is installed.
    {
        auto interpreter_ = utils::ngenPy::InterpreterUtil::getInstance();
        try {
            auto mod = interpreter_->getModule(forcings_engine_python_module);
            auto cls = mod.attr(forcings_engine_python_class).cast<py::object>();
        } catch(std::exception& e) {
            Logger::logMsgAndThrowError(
                "Failed to initialize ForcingsEngine: ForcingsEngine python module is not installed or is not properly configured. (" + std::string{e.what()} + ")"
            );
        }
    }

    // Check that the WGRIB2 environment variable is defined
    {
        const auto* wgrib2_exec = std::getenv("WGRIB2");
        if (wgrib2_exec == nullptr) {
            Logger::logMsgAndThrowError("Failed to initialize ForcingsEngine: $WGRIB2 is not defined");
        }
    }
}

} // namespace detail
} // namespace data_access
