#include "bmi/AbstractCLibBmiAdapter.hpp"

#include "utilities/FileChecker.h"
#include "utilities/ExternalIntegrationException.hpp"

namespace models {
namespace bmi {

AbstractCLibBmiAdapter::AbstractCLibBmiAdapter(
    const std::string& type_name,
    std::string library_file_path,
    std::string bmi_init_config,
    std::string forcing_file_path,
    bool allow_exceed_end,
    bool has_fixed_time_step,
    std::string registration_func,
    utils::StreamHandler output
)
    : Bmi_Adapter(
          type_name,
          std::move(bmi_init_config),
          std::move(forcing_file_path),
          allow_exceed_end,
          has_fixed_time_step,
          output
      )
    , bmi_lib_file(std::move(library_file_path))
    , bmi_registration_function(std::move(registration_func)){};

AbstractCLibBmiAdapter::AbstractCLibBmiAdapter(AbstractCLibBmiAdapter&& adapter) noexcept = default;

AbstractCLibBmiAdapter::~AbstractCLibBmiAdapter() {
    finalizeForLibAbstraction();
}

void AbstractCLibBmiAdapter::Finalize() {
    finalizeForLibAbstraction();
}

void AbstractCLibBmiAdapter::dynamic_library_load() {
    if (bmi_registration_function.empty()) {
        this->init_exception_msg = "Can't init " + this->model_name +
                                   "; empty name given for library's registration function.";
        throw std::runtime_error(this->init_exception_msg);
    }
    if (dyn_lib_handle != nullptr) {
        this->output.put(
            "WARNING: ignoring attempt to reload dynamic shared library '" + bmi_lib_file +
            "' for " + this->model_name
        );
        return;
    }
    if (!utils::FileChecker::file_is_readable(bmi_lib_file)) {
        // Try alternative extension...
        size_t idx = bmi_lib_file.rfind(".");
        if (idx == std::string::npos) {
            idx = bmi_lib_file.length() - 1;
        }
        std::string alt_bmi_lib_file;
        if (bmi_lib_file.length() == 0) {
            this->init_exception_msg =
                "Can't init " + this->model_name + "; library file path is empty";
            throw std::runtime_error(this->init_exception_msg);
        }
        if (bmi_lib_file.substr(idx) == ".so") {
            alt_bmi_lib_file = bmi_lib_file.substr(0, idx) + ".dylib";
        } else if (bmi_lib_file.substr(idx) == ".dylib") {
            alt_bmi_lib_file = bmi_lib_file.substr(0, idx) + ".so";
        } else {
// Try appending instead of replacing...
#ifdef __APPLE__
            alt_bmi_lib_file = bmi_lib_file + ".dylib";
#else
#ifdef __GNUC__
            alt_bmi_lib_file = bmi_lib_file + ".so";
#endif // __GNUC__
#endif // __APPLE__
        }
        // TODO: Try looking in e.g. /usr/lib, /usr/local/lib, $LD_LIBRARY_PATH... try pre-pending
        // "lib"...
        if (utils::FileChecker::file_is_readable(alt_bmi_lib_file)) {
            bmi_lib_file = alt_bmi_lib_file;
        } else {
            this->init_exception_msg = "Can't init " + this->model_name +
                                       "; unreadable shared library file '" + bmi_lib_file + "'";
            throw std::runtime_error(this->init_exception_msg);
        }
    }

    // Call first to ensure any previous error is cleared before trying to load the symbol
    dlerror();
    // Load up the necessary library dynamically
    dyn_lib_handle = dlopen(bmi_lib_file.c_str(), RTLD_NOW | RTLD_LOCAL);
    // Now call again to see if there was an error (if there was, this will not be null)
    char* err_message = dlerror();
    if (dyn_lib_handle == nullptr && err_message != nullptr) {
        this->init_exception_msg =
            "Cannot load shared lib '" + bmi_lib_file + "' for model " + this->model_name;
        if (err_message != nullptr) {
            this->init_exception_msg += " (" + std::string(err_message) + ")";
        }
        throw ::external::ExternalIntegrationException(this->init_exception_msg);
    }
}

void* AbstractCLibBmiAdapter::dynamic_load_symbol(
    const std::string& symbol_name,
    bool is_null_valid
) {
    if (dyn_lib_handle == nullptr) {
        throw std::runtime_error(
            "Cannot load symbol '" + symbol_name +
            "' without handle to shared library (bmi_lib_file = '" + bmi_lib_file + "')"
        );
    }
    // Call first to ensure any previous error is cleared before trying to load the symbol
    dlerror();
    void* symbol = dlsym(dyn_lib_handle, symbol_name.c_str());
    // Now call again to see if there was an error (if there was, this will not be null)
    char* err_message = dlerror();
    if (symbol == nullptr && (err_message != nullptr || !is_null_valid)) {
        this->init_exception_msg =
            "Cannot load shared lib symbol '" + symbol_name + "' for model " + this->model_name;
        if (err_message != nullptr) {
            this->init_exception_msg += " (" + std::string(err_message) + ")";
        }
        throw ::external::ExternalIntegrationException(this->init_exception_msg);
    }
    return symbol;
}

void* AbstractCLibBmiAdapter::dynamic_load_symbol(const std::string& symbol_name) {
    return dynamic_load_symbol(symbol_name, false);
}

const std::string& AbstractCLibBmiAdapter::get_bmi_registration_function() {
    return bmi_registration_function;
}

const void* AbstractCLibBmiAdapter::get_dyn_lib_handle() {
    return dyn_lib_handle;
}

void AbstractCLibBmiAdapter::finalizeForLibAbstraction() {
    //  close the dynamically loaded library
    if (dyn_lib_handle != nullptr) {
        dlclose(dyn_lib_handle);
    }
}

} // namespace bmi
} // namespace models
