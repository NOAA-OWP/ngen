#ifndef NGEN_TEST_BMI_PRIMITIVE_TYPE_PRINTING_HPP
#define NGEN_TEST_BMI_PRIMITIVE_TYPE_PRINTING_HPP

#include <ostream>

#include "utilities/bmi_primitive_type.hpp"

namespace models {
    namespace bmi {
        /** Have googletest print a primitive_type by its enumerator's name in failures, rather than as a byte. */
        inline void PrintTo(primitive_type type, std::ostream *os)
        {
            *os << to_string(type);
        }
    }
}

#endif // NGEN_TEST_BMI_PRIMITIVE_TYPE_PRINTING_HPP
