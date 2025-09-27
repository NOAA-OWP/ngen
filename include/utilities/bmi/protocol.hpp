/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
------------------------------------------------------------------------
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
------------------------------------------------------------------------

Version 0.2
Enumerate protocol error types and add ProtocolError exception class
Implement error handling via expected<T, ProtocolError> and error_or_warning
Removed model member and required model reference in run(), check_support(), and initialize()
Minor refactoring and style changes

Version 0.1
Virtual interface for BMI protocols
*/

#pragma once

#include "Bmi_Adapter.hpp"
#include "JSONProperty.hpp"
#include <nonstd/expected.hpp>

namespace models{ namespace bmi{ namespace protocols{
using nonstd::expected;
using nonstd::make_unexpected;

enum class Error{
    UNITIALIZED_MODEL,
    UNSUPPORTED_PROTOCOL,
    INTEGRATION_ERROR,
    PROTOCOL_ERROR,
    PROTOCOL_WARNING
};

class ProtocolError: public std::exception {
  public:
    ProtocolError () = delete;
    ProtocolError(Error err, const std::string& message="") : err(std::move(err)), message(std::move(message)) {}
    ProtocolError(const ProtocolError& other) = default;
    ProtocolError(ProtocolError&& other) noexcept = default;
    ProtocolError& operator=(const ProtocolError& other) = default;
    ProtocolError& operator=(ProtocolError&& other) noexcept = default;
    ~ProtocolError() = default;

    auto to_string() const -> std::string {
        switch (err) {
            case Error::UNITIALIZED_MODEL: return "Error(Uninitialized Model)::" + message;
            case Error::UNSUPPORTED_PROTOCOL: return "Warning(Unsupported Protocol)::" + message;
            case Error::INTEGRATION_ERROR: return "Error(Integration)::" + message;
            case Error::PROTOCOL_ERROR: return "Error(Protocol)::" + message;
            case Error::PROTOCOL_WARNING: return "Warning(Protocol)::" + message;
            default: return "Unknown Error: " + message;
        }
    }

  auto error_code() const -> const Error&  { return err; }
  auto get_message() const -> const std::string&  { return message; }

  char const *what() const noexcept override {
      message = to_string();
      return message.c_str();
  }

  private:
    Error err;
    mutable std::string message;
};

struct Context{
    const int current_time_step;
    const int total_steps;
    const std::string& timestamp;
    const std::string& id;
};

using ModelPtr = std::shared_ptr<models::bmi::Bmi_Adapter>;
using Properties = geojson::PropertyMap;

class NgenBmiProtocol{
  /**
   * @brief Abstract interface for a generic BMI protocol
   * 
   */

  public:

    /**
     * @brief Construct a new Ngen Bmi Protocol object
     * 
     * By default, the protocol is considered unsupported.
     * Subclasses are responsible for implementing the check_support() method,
     * and ensuring that is_supported is properly set based on the protocol's
     * requirements.
     * 
     */
    NgenBmiProtocol() : is_supported(false) {}

    virtual ~NgenBmiProtocol() = default;

  protected:
    /**
     * @brief Handle a ProtocolError by either throwing it or logging it as a warning
     * 
     * @param err The ProtocolError to handle
     * @return expected<void, ProtocolError> Returns an empty expected if the error was logged as a warning,
     *         otherwise throws the ProtocolError.
     * 
     * @throws ProtocolError if the error is of type PROTOCOL_ERROR
     */
    static auto error_or_warning(const ProtocolError& err) -> expected<void, ProtocolError> {
        // Log warnings, but throw errors
        switch(err.error_code()){
            case Error::PROTOCOL_ERROR:
                throw err;
                break;
            case Error::INTEGRATION_ERROR:
            case Error::UNITIALIZED_MODEL:
            case Error::UNSUPPORTED_PROTOCOL:
            case Error::PROTOCOL_WARNING:
                std::cerr << err.to_string() << std::endl;
                return make_unexpected<ProtocolError>( ProtocolError(std::move(err) ) );
            default:
                throw err;
        }
        assert (false && "Unreachable code reached in error_or_warning");
    }

    /**
     * @brief Run the BMI protocol against the given model
     * 
     * Execute the logic of the protocol with the provided context and model.
     * It is the caller's responsibility to ensure that the model provided is
     * consistent with the model provided to the object's initialize() and
     * check_support() methods, hence the protected nature of this function.
     * 
     * @param ctx Contextual information for the protocol run
     * @param model A shared pointer to a Bmi_Adapter object which should be
     * initialized before being passed to this method.
     * 
     * @return expected<void, ProtocolError> May contain a ProtocolError if
     *         the protocol fails for any reason.  Errors of ProtocolError::PROTOCOL_WARNING
     *         severity should be logged as warnings, but not cause the simulation to fail.
     */
    [[nodiscard]] virtual auto run(const ModelPtr& model, const Context& ctx) const -> expected<void, ProtocolError> = 0;

    /**
     * @brief Check if the BMI protocol is supported by the model
     * 
     * It is the caller's responsibility to ensure that the model provided is
     * consistent with the model provided to the object's initialize() and
     * run() methods, hence the protected nature of this function.
     * 
     * @param model A shared pointer to a Bmi_Adapter object which should be
     * initialized before being passed to this method.
     * 
     * @return expected<void, ProtocolError> May contain a ProtocolError if
     *         the protocol is not supported by the model.
     */
    [[nodiscard]] virtual expected<void, ProtocolError> check_support(const ModelPtr& model) = 0;

    /**
     * @brief Initialize the BMI protocol from a set of key/value properties
     * 
     * It is the caller's responsibility to ensure that the model provided is
     * consistent with the model provided to the object's run() and
     * check_support() methods, hence the protected nature of this function.
     * 
     * @param properties key/value pairs for initializing the protocol
     * @param model A shared pointer to a Bmi_Adapter object which should be
     * initialized before being passed to this method.
     * 
     * @return expected<void, ProtocolError> May contain a ProtocolError if
     *         initialization fails for any reason, since the protocol must
     *         be effectively "optional", failed initialization results in
     *         the protocol being disabled for the duration of the simulation.
     */
    virtual auto initialize(const ModelPtr& model, const Properties& properties) -> expected<void, ProtocolError> = 0;

    /**
     * @brief Whether the protocol is supported by the model
     * 
     */
    bool is_supported = false;

    /**
     * @brief Friend class for managing one or more protocols
     * 
     * This allows the NgenBmiProtocols container class to access the protected `run()`
     * method. This allows the container to ensure consistent application of the
     * protocol with a particular bmi model instance throughout the lifecycle of a given
     * protocol.
     * 
     */
    friend class NgenBmiProtocols;
}; 

}}}
