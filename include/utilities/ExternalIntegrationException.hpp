#ifndef NGEN_EXTERNALINTEGRATIONEXCEPTION_HPP
#define NGEN_EXTERNALINTEGRATIONEXCEPTION_HPP

#include <exception>
#include <utility>

using namespace std;

namespace external {
    /**
         * Custom exception indicating a problem when integrating with some external component.
         *
         * Custom exception for indicating that the framework encountered a problem interoperating with some type of
         * external component with which it should be integrated.
         */
    class ExternalIntegrationException : public exception {

    public:

        ExternalIntegrationException(char const *const message) noexcept : ExternalIntegrationException(string(message)) {}

        ExternalIntegrationException(std::string message) noexcept : std::exception(), what_message(std::move(message)) {}

        ExternalIntegrationException(ExternalIntegrationException &exception) noexcept : ExternalIntegrationException(exception.what_message) {}

        ExternalIntegrationException(ExternalIntegrationException &&exception) noexcept
        : ExternalIntegrationException(std::move(exception.what_message)) {}

        virtual char const *what() const noexcept {
            return what_message.c_str();
        }

    private:

        std::string what_message;
    };
}

#endif //NGEN_EXTERNALINTEGRATIONEXCEPTION_HPP
