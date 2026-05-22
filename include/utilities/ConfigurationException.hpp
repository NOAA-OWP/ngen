#ifndef NGEN_CONFIGURATIONEXCEPTION_HPP
#define NGEN_CONFIGURATIONEXCEPTION_HPP

#include <exception>
#include <string>
#include <utility>


namespace realization {
    /**
     * Custom exception indicating a problem when integrating with the provided realization configuration.
     */
    class ConfigurationException : public std::exception {

    public:

        ConfigurationException(char const *const message) noexcept : ConfigurationException(std::string(message)) {}

        ConfigurationException(std::string message) noexcept : std::exception(), what_message(std::move(message)) {}

        ConfigurationException(ConfigurationException &exception) noexcept : ConfigurationException(exception.what_message) {}

        ConfigurationException(ConfigurationException &&exception) noexcept
                : ConfigurationException(std::move(exception.what_message)) {}

        char const *what() const noexcept override {
            return what_message.c_str();
        }

    private:

        std::string what_message;
    };
}

#endif //NGEN_CONFIGURATIONEXCEPTION_HPP
