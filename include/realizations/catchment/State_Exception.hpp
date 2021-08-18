#ifndef NGEN_BMI_MODEL_STATE_EXCEPTION_H
#define NGEN_BMI_MODEL_STATE_EXCEPTION_H

#include <exception>
#include <utility>

using namespace std;

namespace models {
    namespace external {

        /**
         * Custom exception indicating bad external model state.
         *
         * Custom exception for indicating that an external model has, or at least potentially has, entered into an
         * invalid state.
         *
         */
        class State_Exception : public std::exception {

        public:

            State_Exception(char const *const message) noexcept: std::exception(), what_message(message) {}

            State_Exception(std::string message) noexcept
                    : std::exception(), what_message(std::move(message)) {}

            State_Exception(State_Exception &exception) noexcept: State_Exception(exception.what_message) {}

            State_Exception(State_Exception &&exception) noexcept
                    : State_Exception(std::move(exception.what_message)) {}

            virtual char const *what() const noexcept {
                return what_message.c_str();
            }

        private:

            std::string what_message;
        };
    }
}

#endif //NGEN_BMI_MODEL_STATE_EXCEPTION_H
