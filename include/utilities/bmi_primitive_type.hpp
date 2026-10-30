#ifndef NGEN_BMI_PRIMITIVE_TYPE_HPP
#define NGEN_BMI_PRIMITIVE_TYPE_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

/*
 * The type of one item of a BMI variable, resolved from the name a model reports.
 *
 *   primitive_type        the item types a variable can have, independent of any language's spelling
 *   parse_bmi_type()      a type name to its primitive_type; the one table of spellings
 *   with_primitive_type() call a callable with the C++ type a primitive_type denotes
 *
 * Kept apart from bmi_utilities.hpp, which builds typed access to model variables on top of this,
 * so that Bmi_Adapter can name a primitive_type without depending on those utilities.
 */

namespace models {
    namespace bmi {
        /** @brief Thrown when a BMI type name has no C++ type behind it. */
        struct unknown_bmi_type : std::runtime_error {
            explicit unknown_bmi_type(const std::string &type_name)
                : std::runtime_error("No C++ type corresponds to the BMI type name '" + type_name + "'")
                , type_name(type_name) {}

            std::string type_name;
        };

        /**
         * @brief The type of one item of a BMI variable, independent of how any language spells it.
         *
         * BMI reports a variable's type as a name in the model's language, and some names -- Fortran's
         * "real", Python's "int" -- need the item size to settle which type they mean. The name is
         * resolved to one of these once, and the framework passes this around thereafter rather than
         * the name.
         *
         * The fixed-width enumerators mean exactly that width and representation. Two do not:
         *
         * - long_double is the platform's C++ long double, whatever that is: IEEE binary128 on aarch64
         *   Linux, x87 80-bit extended precision padded to 12 or 16 bytes on x86, or the same as double
         *   on others. It is what a model means by naming "long double".
         * - f128 is IEEE binary128, whatever C++ type holds it. It is supported only where long double
         *   is binary128; see with_primitive_type() for the path elsewhere.
         */
        enum class primitive_type : std::uint8_t {
            i8, u8, i16, u16, i32, u32, i64, u64,
            f32, f64,
            long_double,
            f128,
        };

        /**
         * @brief Whether long double is IEEE binary128, the one format with a 113-bit significand.
         *
         * x87's 80-bit type has 64, and PowerPC's double-double 106.
         */
        inline constexpr bool long_double_is_float128 = std::numeric_limits<long double>::digits == 113;

        /** @brief The enumerator's name, e.g. "i32", for messages. */
        constexpr const char *to_string(primitive_type type)
        {
            switch (type) {
                case primitive_type::i8: return "i8";
                case primitive_type::u8: return "u8";
                case primitive_type::i16: return "i16";
                case primitive_type::u16: return "u16";
                case primitive_type::i32: return "i32";
                case primitive_type::u32: return "u32";
                case primitive_type::i64: return "i64";
                case primitive_type::u64: return "u64";
                case primitive_type::f32: return "f32";
                case primitive_type::f64: return "f64";
                case primitive_type::long_double: return "long_double";
                case primitive_type::f128: return "f128";
            }
            return "?";
        }

        /** @brief The size in bytes of one item of @p type. */
        constexpr size_t size_of(primitive_type type)
        {
            switch (type) {
                case primitive_type::i8: case primitive_type::u8: return 1;
                case primitive_type::i16: case primitive_type::u16: return 2;
                case primitive_type::i32: case primitive_type::u32: case primitive_type::f32: return 4;
                case primitive_type::i64: case primitive_type::u64: case primitive_type::f64: return 8;
                case primitive_type::long_double: return sizeof(long double);
                case primitive_type::f128: return 16;
            }
            return 0;
        }

        /**
         * @brief The primitive_type of the C++ arithmetic type @tparam T.
         *
         * Integer types map by signedness and size, so that a C spelling such as "long" resolves to the
         * width it has on this platform.
         */
        template <typename T>
        constexpr primitive_type primitive_type_of()
        {
            if constexpr (std::is_same_v<T, long double>) {
                return primitive_type::long_double;
            } else if constexpr (std::is_same_v<T, double>) {
                return primitive_type::f64;
            } else if constexpr (std::is_same_v<T, float>) {
                return primitive_type::f32;
            } else {
                static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>, "No primitive_type for this type");
                static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
                              "No primitive_type of this integer's width");
                constexpr bool is_signed = std::is_signed_v<T>;
                if constexpr (sizeof(T) == 1) return is_signed ? primitive_type::i8 : primitive_type::u8;
                else if constexpr (sizeof(T) == 2) return is_signed ? primitive_type::i16 : primitive_type::u16;
                else if constexpr (sizeof(T) == 4) return is_signed ? primitive_type::i32 : primitive_type::u32;
                else return is_signed ? primitive_type::i64 : primitive_type::u64;
            }
        }

        /** @brief Thrown when a primitive_type has no C++ type on this platform. */
        struct unsupported_primitive_type : std::runtime_error {
            explicit unsupported_primitive_type(primitive_type type)
                : std::runtime_error(std::string("No C++ type for primitive type ") + to_string(type) +
                                     " on this platform")
                , type(type) {}

            primitive_type type;
        };

        /**
         * @brief Call @p f with the C++ type that @p type denotes.
         *
         * Every operation on a variable has to pick the matching C++ type before it can do anything, and
         * this is where that happens: @p f is a callable with a template parameter, invoked as
         * `f.template operator()<T>()`, and this picks the T.
         *
         * @code
         * // "Give me this value as whatever the variable declares." The target type is only known at run
         * // time, so the cast cannot be written directly: capture the value, and let this supply the type.
         * std::shared_ptr<void> as_declared_type(primitive_type type, double value)
         * {
         *     return with_primitive_type(type, [&]<typename T>() -> std::shared_ptr<void> {
         *         return std::make_shared<T>(static_cast<T>(value));
         *     });
         * }
         * @endcode
         *
         * Two things there are easy to get wrong.
         *
         * The lambda takes no arguments. `value` arrives by capture, which leaves the parameter list free
         * for the one thing this function supplies -- `f.template operator()<T>()` passes a type and
         * nothing else. That is what lets one dispatch serve callers whose data is a scalar, an iterator
         * pair, or a pointer and a count: each seals its own data into the closure.
         *
         * The `-> std::shared_ptr<void>` is load-bearing. `make_shared<T>` yields a `shared_ptr<T>`, a
         * different type in every branch, and the branches have to agree on one return type. Converting
         * there is the only place T is still known. Omitting it is a compile error rather than a silent
         * conversion -- as is a body that does not compile for every type below, even one no caller ever
         * asks for, since all of them are instantiated.
         *
         * @param type The primitive type to dispatch on.
         * @param f A callable templated on the type to use.
         * @return Whatever @p f returns.
         * @throw unsupported_primitive_type If @p type has no C++ type on this platform.
         */
        template <typename F>
        auto with_primitive_type(primitive_type type, F &&f)
        {
            switch (type) {
                case primitive_type::i8: return f.template operator()<std::int8_t>();
                case primitive_type::u8: return f.template operator()<std::uint8_t>();
                case primitive_type::i16: return f.template operator()<std::int16_t>();
                case primitive_type::u16: return f.template operator()<std::uint16_t>();
                case primitive_type::i32: return f.template operator()<std::int32_t>();
                case primitive_type::u32: return f.template operator()<std::uint32_t>();
                case primitive_type::i64: return f.template operator()<std::int64_t>();
                case primitive_type::u64: return f.template operator()<std::uint64_t>();
                case primitive_type::f32: return f.template operator()<float>();
                case primitive_type::f64: return f.template operator()<double>();
                case primitive_type::long_double: return f.template operator()<long double>();
                case primitive_type::f128:
                    // Where long double is not binary128, GCC and Clang offer __float128 on x86-64, and C++23
                    // std::float128_t where <stdfloat> defines it. Supporting f128 there means dispatching to
                    // that type here, which every callable then has to compile for, as for long double.
                    if constexpr (long_double_is_float128) {
                        return f.template operator()<long double>();
                    } else {
                        break;
                    }
            }
            throw unsupported_primitive_type(type);
        }

        /**
         * @brief The primitive_type a BMI type name denotes.
         *
         * This is the one table of type spellings. Several may denote one type -- "real" and "float",
         * "integer" and "int" -- and C spellings resolve to the width they have on this platform. A name
         * whose width depends on more than the name, as Fortran's "real" under -fdefault-real-8 does, is
         * for an adapter's Bmi_Adapter::get_analogous_cxx_type() to settle first.
         *
         * @param type The BMI type name.
         * @return The primitive_type it denotes.
         * @throw unknown_bmi_type If @p type is not a name this knows.
         */
        inline primitive_type parse_bmi_type(const std::string &type)
        {
            if (type == "double" || type == "double precision")
                return primitive_type::f64;
            if (type == "long double")
                return primitive_type::long_double;
            if (type == "float" || type == "real")
                return primitive_type::f32;

            if (type == "int8" || type == "int8_t")
                return primitive_type::i8;
            if (type == "uint8" || type == "uint8_t")
                return primitive_type::u8;
            if (type == "int16" || type == "int16_t")
                return primitive_type::i16;
            if (type == "uint16" || type == "uint16_t")
                return primitive_type::u16;
            if (type == "int32" || type == "int32_t")
                return primitive_type::i32;
            if (type == "uint32" || type == "uint32_t")
                return primitive_type::u32;
            if (type == "int64" || type == "int64_t")
                return primitive_type::i64;
            if (type == "uint64" || type == "uint64_t")
                return primitive_type::u64;

            if (type == "short" || type == "short int" || type == "signed short" || type == "signed short int")
                return primitive_type_of<short>();
            if (type == "unsigned short" || type == "unsigned short int")
                return primitive_type_of<unsigned short>();
            if (type == "int" || type == "signed" || type == "signed int" || type == "integer")
                return primitive_type_of<int>();
            if (type == "unsigned" || type == "unsigned int")
                return primitive_type_of<unsigned int>();
            if (type == "long" || type == "long int" || type == "signed long" || type == "signed long int")
                return primitive_type_of<long>();
            if (type == "unsigned long" || type == "unsigned long int")
                return primitive_type_of<unsigned long>();
            if (type == "long long" || type == "long long int" || type == "signed long long" || type == "signed long long int")
                return primitive_type_of<long long>();
            if (type == "unsigned long long" || type == "unsigned long long int")
                return primitive_type_of<unsigned long long>();

            throw unknown_bmi_type(type);
        }
    }
}

#endif //NGEN_BMI_PRIMITIVE_TYPE_HPP
