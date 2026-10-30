#include <array>
#include <cstdint>
#include <list>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <boost/type_index.hpp>
#include <gtest/gtest.h>

#include "bmi_utilities.hpp"

namespace {

// Reads a buffer of the named BMI type back as T. This is what GetValue() does once it has fetched
// the bytes, with the model standing up the buffer; here the buffer is supplied directly so the
// mapping can be exercised on its own.
template <typename T>
std::vector<T> read_as(const std::string& type, const void* data, size_t num_items) {
    return models::bmi::with_bmi_type(type, [&]<typename S>() -> std::vector<T> {
        return models::bmi::helper::make_vector<T>(static_cast<const S*>(data), num_items);
    });
}

// Verify each recognized type-name spelling casts to the right C++
// type and yields the expected element values when the target vector
// is `double` (chosen because every integer and floating-point width
// converts to it losslessly for the small values used here).
template <typename SRC>
void expect_roundtrip(const std::string& type_name,
                      std::initializer_list<SRC> values) {
    std::vector<SRC> src(values);
    auto out = read_as<double>(type_name, src.data(), src.size());
    ASSERT_EQ(out.size(), src.size()) << "spelling: " << type_name;
    for (size_t i = 0; i < src.size(); ++i) {
        EXPECT_EQ(out[i], static_cast<double>(src[i]))
            << "spelling: " << type_name << " index " << i;
    }
}

} // namespace

// ---------------------------------------------------------------------
// Floating-point spellings.
// ---------------------------------------------------------------------

TEST(BmiUtilitiesDispatch, float_variants) {
    expect_roundtrip<float>("float", {1.5f, -2.25f, 3.125f});
    expect_roundtrip<double>("double", {1.5, -2.25, 3.125});
    expect_roundtrip<long double>("long double", {1.5L, -2.25L, 3.125L});
}

// ---------------------------------------------------------------------
// stdint integer spellings. Each width tested with
// and without the `_t` suffix, both signed and unsigned.
// ---------------------------------------------------------------------

TEST(BmiUtilitiesDispatch, int8_stdint) {
    expect_roundtrip<int8_t>("int8", {int8_t{-3}, int8_t{7}, int8_t{-128}});
    expect_roundtrip<int8_t>("int8_t", {int8_t{-3}, int8_t{7}, int8_t{-128}});
}

TEST(BmiUtilitiesDispatch, uint8_stdint) {
    expect_roundtrip<uint8_t>("uint8", {uint8_t{0}, uint8_t{200}, uint8_t{255}});
    expect_roundtrip<uint8_t>("uint8_t", {uint8_t{0}, uint8_t{200}, uint8_t{255}});
}

TEST(BmiUtilitiesDispatch, int16_stdint) {
    expect_roundtrip<int16_t>("int16", {int16_t{-1}, int16_t{32000}, int16_t{-32000}});
    expect_roundtrip<int16_t>("int16_t", {int16_t{-1}, int16_t{32000}, int16_t{-32000}});
}

TEST(BmiUtilitiesDispatch, uint16_stdint) {
    expect_roundtrip<uint16_t>("uint16", {uint16_t{0}, uint16_t{40000}, uint16_t{65535}});
    expect_roundtrip<uint16_t>("uint16_t", {uint16_t{0}, uint16_t{40000}, uint16_t{65535}});
}

TEST(BmiUtilitiesDispatch, int32_stdint) {
    expect_roundtrip<int32_t>("int32", {int32_t{-1}, int32_t{1'000'000}, int32_t{-1'000'000}});
    expect_roundtrip<int32_t>("int32_t", {int32_t{-1}, int32_t{1'000'000}, int32_t{-1'000'000}});
}

TEST(BmiUtilitiesDispatch, uint32_stdint) {
    expect_roundtrip<uint32_t>("uint32", {uint32_t{0}, uint32_t{3'000'000'000u}});
    expect_roundtrip<uint32_t>("uint32_t", {uint32_t{0}, uint32_t{3'000'000'000u}});
}

TEST(BmiUtilitiesDispatch, int64_stdint) {
    // Value range chosen to survive a lossless round-trip through double.
    expect_roundtrip<int64_t>("int64", {int64_t{-1}, int64_t{1LL << 40}, int64_t{-(1LL << 40)}});
    expect_roundtrip<int64_t>("int64_t", {int64_t{-1}, int64_t{1LL << 40}, int64_t{-(1LL << 40)}});
}

TEST(BmiUtilitiesDispatch, uint64_stdint) {
    expect_roundtrip<uint64_t>("uint64", {uint64_t{0}, uint64_t{1ULL << 40}});
    expect_roundtrip<uint64_t>("uint64_t", {uint64_t{0}, uint64_t{1ULL << 40}});
}

// ---------------------------------------------------------------------
// C keyword spellings — kept for adapters that forward the model's raw
// C type spelling (Bmi_C_Adapter, Bmi_Cpp_Adapter).
// ---------------------------------------------------------------------

TEST(BmiUtilitiesDispatch, short_keyword_variants) {
    expect_roundtrip<short>("short", {short{-1}, short{7}});
    expect_roundtrip<short>("short int", {short{-1}, short{7}});
    expect_roundtrip<short>("signed short", {short{-1}, short{7}});
    expect_roundtrip<short>("signed short int", {short{-1}, short{7}});
    expect_roundtrip<unsigned short>("unsigned short", {static_cast<unsigned short>(0), static_cast<unsigned short>(50000)});
    expect_roundtrip<unsigned short>("unsigned short int", {static_cast<unsigned short>(0), static_cast<unsigned short>(50000)});
}

TEST(BmiUtilitiesDispatch, int_keyword_variants) {
    expect_roundtrip<int>("int", {-1, 7, 1'000'000});
    expect_roundtrip<int>("signed", {-1, 7});
    expect_roundtrip<int>("signed int", {-1, 7});
    expect_roundtrip<unsigned int>("unsigned", {0u, 3'000'000'000u});
    expect_roundtrip<unsigned int>("unsigned int", {0u, 3'000'000'000u});
}

TEST(BmiUtilitiesDispatch, long_keyword_variants) {
    expect_roundtrip<long>("long", {-1L, 7L});
    expect_roundtrip<long>("long int", {-1L, 7L});
    expect_roundtrip<long>("signed long", {-1L, 7L});
    expect_roundtrip<long>("signed long int", {-1L, 7L});
    expect_roundtrip<unsigned long>("unsigned long", {0UL, 100UL});
    expect_roundtrip<unsigned long>("unsigned long int", {0UL, 100UL});
}

TEST(BmiUtilitiesDispatch, long_long_keyword_variants) {
    expect_roundtrip<long long>("long long", {-1LL, 7LL});
    expect_roundtrip<long long>("long long int", {-1LL, 7LL});
    expect_roundtrip<long long>("signed long long", {-1LL, 7LL});
    expect_roundtrip<long long>("signed long long int", {-1LL, 7LL});
    expect_roundtrip<unsigned long long>("unsigned long long", {0ULL, 100ULL});
    expect_roundtrip<unsigned long long>("unsigned long long int", {0ULL, 100ULL});
}

// ---------------------------------------------------------------------
// Negative case: unrecognized spelling throws and names the offending
// type in the error message.
// ---------------------------------------------------------------------

TEST(BmiUtilitiesDispatch, unknown_type_throws) {
    std::array<double, 1> buf{0.0};
    try {
        (void) read_as<double>("nonesuch_t", buf.data(), buf.size());
        FAIL() << "expected models::bmi::unknown_bmi_type";
    } catch (const models::bmi::unknown_bmi_type& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("nonesuch_t"), std::string::npos)
            << "error message should name the unrecognized type: " << msg;
    }
}

// ---------------------------------------------------------------------
// The table serves more than the read-back path above. Every caller
// supplies its own body and its own data; the table only picks the type.
// ---------------------------------------------------------------------

/**
 * as_c_array() gathers any range into a contiguous array of the requested type.
 *
 * It is what get_values_as_type() reduces to once the table has picked a type, and the only caller
 * that allocates. The source here is a list, which is deliberately not contiguous: producing an
 * array a BMI model can be handed is the whole point.
 */
TEST(BmiUtilitiesDispatch, as_c_array_gathers_a_range_into_a_contiguous_array) {
    const std::list<double> source{1.5, 2.5, 3.5};

    const auto as_double = models::bmi::as_c_array<double>(source.begin(), source.end());
    EXPECT_DOUBLE_EQ(as_double.get()[0], 1.5);
    EXPECT_DOUBLE_EQ(as_double.get()[1], 2.5);
    EXPECT_DOUBLE_EQ(as_double.get()[2], 3.5);

    // Each element is cast on the way in, so a narrowing target truncates rather than being refused.
    const auto as_int = models::bmi::as_c_array<int>(source.begin(), source.end());
    EXPECT_EQ(as_int.get()[0], 1);
    EXPECT_EQ(as_int.get()[1], 2);
    EXPECT_EQ(as_int.get()[2], 3);

    // An empty range still yields a usable pointer that the deleter can free.
    const std::list<double> empty;
    EXPECT_NE(models::bmi::as_c_array<double>(empty.begin(), empty.end()).get(), nullptr);
}

TEST(BmiUtilitiesDispatch, one_table_serves_every_shape) {
    // A single value, cast into the named type.
    auto one = models::bmi::get_value_as_type("float", 2.5);
    ASSERT_FLOAT_EQ(*static_cast<float*>(one.get()), 2.5f);

    // An iterator range, cast into a contiguous array of the named type.
    std::vector<double> source{1.5, 2.5, 3.5};
    auto many = models::bmi::get_values_as_type("int32_t", source.begin(), source.end());
    ASSERT_EQ(static_cast<std::int32_t*>(many.get())[0], 1);
    ASSERT_EQ(static_cast<std::int32_t*>(many.get())[2], 3);

    // And bytes read back as something else, which is what read_as() above does.
    const float raw[2] = {1.5f, 3.5f};
    auto back = read_as<double>("real", raw, 2);
    ASSERT_DOUBLE_EQ(back[1], 3.5);
}

/**
 * Both type-name vocabularies resolve.
 *
 * The C and C++ adapters report the model's own type name, and the Fortran adapter's
 * get_primitive_type falls back to the table for names it does not resolve by width, so the
 * table takes the Fortran spellings as well as the C and C++ ones.
 */
TEST(BmiUtilitiesDispatch, both_type_name_vocabularies_resolve) {
    auto cxx_type_of = [](const std::string& type) {
        return models::bmi::with_bmi_type(type, []<typename T>() { return boost::typeindex::type_id<T>(); });
    };

    EXPECT_EQ(cxx_type_of("real"), cxx_type_of("float"));
    EXPECT_EQ(cxx_type_of("double precision"), cxx_type_of("double"));
    EXPECT_EQ(cxx_type_of("integer"), cxx_type_of("int"));
    EXPECT_EQ(cxx_type_of("int32_t"), cxx_type_of("int32"));
    EXPECT_EQ(cxx_type_of("signed long int"), cxx_type_of("long"));
}

/** Every name the table carries is reachable from the functions built on it, not just read_as. */
TEST(BmiUtilitiesDispatch, every_name_is_reachable_through_the_callers) {
    for (const std::string type : {"double", "double precision", "long double", "float", "real",
                                   "short", "unsigned short", "int", "integer", "unsigned",
                                   "long", "unsigned long", "long long", "unsigned long long",
                                   "int8", "int8_t", "uint8", "uint8_t", "int16", "int16_t",
                                   "uint16", "uint16_t", "int32", "int32_t", "uint32", "uint32_t",
                                   "int64", "int64_t", "uint64", "uint64_t"}) {
        EXPECT_NO_THROW(models::bmi::get_value_as_type(type, 1.0)) << type;

        std::vector<double> one{1.0};
        EXPECT_NO_THROW(models::bmi::get_values_as_type(type, one.begin(), one.end())) << type;
    }
}

/** An unrecognized spelling is refused by those callers too, not only by read_as. */
TEST(BmiUtilitiesDispatch, unknown_type_throws_from_every_caller) {
    EXPECT_THROW(models::bmi::get_value_as_type("nonesuch_t", 1.0), models::bmi::unknown_bmi_type);

    std::vector<double> one{1.0};
    EXPECT_THROW(models::bmi::get_values_as_type("nonesuch_t", one.begin(), one.end()),
                 models::bmi::unknown_bmi_type);
}

/**
 * require_item_size() refuses a type name that disagrees with the size the model reports.
 *
 * The two are reported separately and nothing in BMI makes them agree, so a buffer sized from one and
 * addressed as the other runs off the end.
 */
TEST(BmiUtilitiesDispatch, require_item_size_refuses_a_disagreeing_pair) {
    EXPECT_NO_THROW(models::bmi::require_item_size("double", sizeof(double), "variable 'X'"));
    EXPECT_NO_THROW(models::bmi::require_item_size("int32_t", 4, "variable 'X'"));
    EXPECT_NO_THROW(models::bmi::require_item_size("real", sizeof(float), "variable 'X'"));

    try {
        models::bmi::require_item_size("int", sizeof(int) + 1, "variable 'X'");
        FAIL() << "expected a disagreeing item size to be refused";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("variable 'X'"), std::string::npos)
            << "message should name what was being resolved: " << msg;
        EXPECT_NE(msg.find(std::to_string(sizeof(int) + 1)), std::string::npos)
            << "message should report the size the model gave: " << msg;
    }

    // A name with no type behind it is still that, not a complaint about its size.
    EXPECT_THROW(models::bmi::require_item_size("nonesuch_t", 4, "variable 'X'"),
                 models::bmi::unknown_bmi_type);
}

/** The conversion is a cast: narrowing follows the usual rules rather than being refused here. */
TEST(BmiUtilitiesDispatch, values_are_cast_into_the_named_type) {
    auto truncated = models::bmi::get_value_as_type("int", 2.9);
    EXPECT_EQ(*static_cast<int*>(truncated.get()), 2);

    auto widened = models::bmi::get_value_as_type("double", 2.5f);
    EXPECT_DOUBLE_EQ(*static_cast<double*>(widened.get()), 2.5);
}

// ---------------------------------------------------------------------
// primitive_type: the resolved form a type name is passed around in.
// ---------------------------------------------------------------------

namespace {
const std::vector<models::bmi::primitive_type> all_primitive_types = {
    models::bmi::primitive_type::i8, models::bmi::primitive_type::u8,
    models::bmi::primitive_type::i16, models::bmi::primitive_type::u16,
    models::bmi::primitive_type::i32, models::bmi::primitive_type::u32,
    models::bmi::primitive_type::i64, models::bmi::primitive_type::u64,
    models::bmi::primitive_type::f32, models::bmi::primitive_type::f64,
    models::bmi::primitive_type::long_double, models::bmi::primitive_type::f128,
};
}

/** Every primitive type with a C++ type here dispatches to one of its size and kind; f128 may have none. */
TEST(BmiUtilitiesPrimitiveType, dispatches_to_a_type_of_its_size) {
    using models::bmi::primitive_type;
    for (const primitive_type type : all_primitive_types) {
        if (type == primitive_type::f128 && !models::bmi::long_double_is_float128) {
            EXPECT_THROW(models::bmi::with_primitive_type(type, []<typename T>() { return sizeof(T); }),
                         models::bmi::unsupported_primitive_type);
            continue;
        }
        const auto [size, is_float, is_signed] = models::bmi::with_primitive_type(type, []<typename T>() {
            return std::tuple<size_t, bool, bool>{sizeof(T), std::is_floating_point_v<T>, std::is_signed_v<T>};
        });
        EXPECT_EQ(size, models::bmi::size_of(type)) << models::bmi::to_string(type);
        const bool expect_float = type == primitive_type::f32 || type == primitive_type::f64 ||
                                  type == primitive_type::long_double || type == primitive_type::f128;
        EXPECT_EQ(is_float, expect_float) << models::bmi::to_string(type);
        const std::string name = models::bmi::to_string(type);
        EXPECT_EQ(is_signed, name[0] != 'u') << name;
    }
}

/** C spellings resolve to the width they have on this platform; fixed-width spellings to exactly that. */
TEST(BmiUtilitiesPrimitiveType, parses_spellings_to_their_width) {
    using models::bmi::parse_bmi_type;
    using models::bmi::primitive_type;
    EXPECT_EQ(models::bmi::size_of(parse_bmi_type("long")), sizeof(long));
    EXPECT_EQ(models::bmi::size_of(parse_bmi_type("unsigned short int")), sizeof(unsigned short));
    EXPECT_EQ(parse_bmi_type("integer"), models::bmi::primitive_type_of<int>());
    EXPECT_EQ(parse_bmi_type("int64_t"), primitive_type::i64);
    EXPECT_EQ(parse_bmi_type("uint8"), primitive_type::u8);
    EXPECT_EQ(parse_bmi_type("double precision"), primitive_type::f64);
    EXPECT_EQ(parse_bmi_type("real"), primitive_type::f32);
    EXPECT_EQ(parse_bmi_type("long double"), primitive_type::long_double);
    EXPECT_THROW(parse_bmi_type("nonesuch_t"), models::bmi::unknown_bmi_type);
}

/** primitive_type_of agrees with the fixed-width typedefs, whichever C++ type each is here. */
TEST(BmiUtilitiesPrimitiveType, primitive_type_of_fixed_width_types) {
    using models::bmi::primitive_type;
    using models::bmi::primitive_type_of;
    EXPECT_EQ(primitive_type_of<std::int8_t>(), primitive_type::i8);
    EXPECT_EQ(primitive_type_of<std::uint16_t>(), primitive_type::u16);
    EXPECT_EQ(primitive_type_of<std::int32_t>(), primitive_type::i32);
    EXPECT_EQ(primitive_type_of<std::uint64_t>(), primitive_type::u64);
    EXPECT_EQ(primitive_type_of<long long>(), primitive_type::i64);
    EXPECT_EQ(primitive_type_of<float>(), primitive_type::f32);
    EXPECT_EQ(primitive_type_of<long double>(), primitive_type::long_double);
}
