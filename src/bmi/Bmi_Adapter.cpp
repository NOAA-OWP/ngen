#include "bmi/Bmi_Adapter.hpp"
#include "bmi/State_Exception.hpp"
#include "utilities/FileChecker.h"
#include "utilities/logging_utils.h"
#include "Logger.hpp"

using namespace std;
std::stringstream str_stream;

namespace models {
namespace bmi {

Bmi_Adapter::Bmi_Adapter(
    std::string model_name,
    std::string bmi_init_config,
    bool has_fixed_time_step
)
    : model_name(std::move(model_name))
    , bmi_init_config(std::move(bmi_init_config))
    , bmi_model_has_fixed_time_step(has_fixed_time_step)
    , bmi_model_time_convert_factor(1.0) {
    // This replicates a lot of Initialize, but it's necessary to be able to do it separately to
    // support "initializing" on construction, given using Initialize requires use of virtual
    // functions
    errno = 0;
    if (!utils::FileChecker::file_is_readable(this->bmi_init_config)) {
        init_exception_msg = "Cannot create and initialize " + this->model_name +
                             " using unreadable file '" + this->bmi_init_config +
                             "'. Error: " + std::strerror(errno);
        Logger::logMsgAndThrowError(init_exception_msg);
    }

    str_stream << __FILE__ << ":" << __LINE__ << " mode name:" << this->model_name << std::endl;
    LOG(str_stream.str(), LogLevel::INFO); str_stream.str("");
    str_stream << __FILE__ << ":" << __LINE__ << " bmi_init_config:" << this->bmi_init_config << std::endl;
    LOG(str_stream.str(), LogLevel::INFO); str_stream.str("");
}

Bmi_Adapter::~Bmi_Adapter() = default;

double Bmi_Adapter::get_time_convert_factor() {
    double value             = 1.0;
    std::string input_units("-");
    try{
        input_units  = GetTimeUnits();
    }
    catch(std::exception &e){
        //Re-throwing any exception as a runtime_error so we don't lose
        //the error context/message.  We will lose the original exception type, though
        //When a python exception is raised from the py adapter subclass, the
        //pybind exception is lost and all we see is a generic "uncaught exception"
        //with no context.  This way we at least get the error message wrapped in
        //a runtime error.
        str_stream << "Bmi_Adapter get_time_convert_factor: Exception caught (" << e.what() << ")" << std::endl;
        LOG(str_stream.str(), LogLevel::ERROR); str_stream.str("");
        throw std::runtime_error(e.what());
    }
    std::string output_units = "s";
    return UnitsHelper::get_converted_value(input_units, value, output_units);
}

double Bmi_Adapter::convert_model_time_to_seconds(const double& model_time_val) {
    return model_time_val * bmi_model_time_convert_factor;
}

double Bmi_Adapter::convert_seconds_to_model_time(const double& seconds_val) {
    return seconds_val / bmi_model_time_convert_factor;
}

void Bmi_Adapter::Initialize() {
    // If there was previous init attempt but w/ failure exception, throw runtime error and include
    // previous message
    errno = 0;
    if (model_initialized && !init_exception_msg.empty()) {
        Logger::logMsgAndThrowError(
            "Previous " + model_name + " init attempt had exception: \n\t" + init_exception_msg
        );
    }
    // If there was previous init attempt w/ (implicitly) no exception on previous attempt, just
    // return
    else if (model_initialized) {
        return;
    } else if (!utils::FileChecker::file_is_readable(bmi_init_config)) {
        init_exception_msg = "Cannot initialize " + model_name + " using unreadable file '" +
                             bmi_init_config + "'. Error: " + std::strerror(errno);
        ;
        Logger::logMsgAndThrowError(init_exception_msg);
    } else {
        try {
            // TODO: make this same name as used with other testing (adjust name in docstring above
            // also)
            construct_and_init_backing_model();
            // Make sure this is set to 'true' after this function call finishes
            model_initialized             = true;
            bmi_model_time_convert_factor = get_time_convert_factor();
        }
        // Record the exception message before re-throwing to handle subsequent function calls
        // properly
        catch (std::exception& e) {
            // Make sure this is set to 'true' after this function call finishes
            model_initialized = true;
            throw e;
        }
    }
}

void Bmi_Adapter::Initialize(std::string config_file) {
    if (config_file != bmi_init_config && model_initialized) {
        Logger::logMsgAndThrowError(
            "Model init previously attempted; cannot change config from " + bmi_init_config +
            " to " + config_file
        );
    }
    str_stream << __FILE__ << ":" << __LINE__ << " Bmi_Adapter::Initialize: config_file = " << config_file << std::endl;
    LOG(str_stream.str(), LogLevel::INFO); str_stream.str("");

    if (config_file != bmi_init_config && !model_initialized) {
        str_stream << "Bmi_Adapter::Initialize: initialization call changes model config from " << bmi_init_config << " to " << config_file;
        LOG(str_stream.str(), LogLevel::INFO); str_stream.str("");

        bmi_init_config = config_file;
    }
    try {
        Initialize();
    } catch (models::external::State_Exception& e) {
        throw e;
    } catch (std::exception& e) {
        Logger::logMsgAndThrowError(e.what());
    }
}

bool Bmi_Adapter::isInitialized() {
    return model_initialized;
}

std::string Bmi_Adapter::get_model_name() {
    return model_name;
}

} // namespace bmi
} // namespace models
