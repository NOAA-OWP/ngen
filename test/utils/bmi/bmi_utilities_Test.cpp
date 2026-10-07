#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "bmi_utilities.hpp"

using models::bmi::helper::get_vector;

namespace {

// Verify each recognized type-name spelling casts to the right C++
// type and yields the expected element values when the target vector
// is `double` (chosen because every integer and floating-point width
// converts to it losslessly for the small values used here).
template <typename SRC>
void expect_roundtrip(const std::string& type_name,
                      std::initializer_list<SRC> values) {
    std::vector<SRC> src(values);
    auto out = get_vector<double>(
        type_name, src.data(), src.size());
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
        (void) get_vector<double>("nonesuch_t", buf.data(), buf.size());
        FAIL() << "expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("nonesuch_t"), std::string::npos)
            << "error message should name the unrecognized type: " << msg;
    }
}
