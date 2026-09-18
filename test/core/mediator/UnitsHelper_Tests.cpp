#include "gtest/gtest.h"

#include "mediator/UnitsHelper.hpp"

class UnitsHelper_Test : public ::testing::Test {

    public:
        void SetUp() override {
            init_unit_system();
        }

    protected:

        UnitsHelper_Test() {
        }

        ~UnitsHelper_Test() {
        }

};

TEST_F(UnitsHelper_Test, TestDoAConversion){
    ASSERT_NEAR(32.0, UnitsHelper::get_converted_value("degC", 0, "degF"), 0.000000001);
    ASSERT_NEAR(0.0, UnitsHelper::get_converted_value("degF", 32, "degC"), 0.000000001);

}

TEST_F(UnitsHelper_Test, TestConvertArray){
    std::vector<double> data = {1,2,3,4};
    std::vector<double> expected = {1000, 2000, 3000, 4000};
    //Call the converter, updates data in place
    UnitsHelper::convert_values("m", data.data(), "mm", data.data(), data.size());
    ASSERT_EQ( expected,  data);

}

// For coverage completeness...
TEST_F(UnitsHelper_Test, TestConvertArrayNoOp){
    std::vector<double> data = {1,2,3,4};
    std::vector<double> expected = {1, 2, 3, 4};
    //Call the converter, updates data in place
    UnitsHelper::convert_values("m", data.data(), "m", data.data(), data.size());
    ASSERT_EQ( expected,  data);
}

TEST_F(UnitsHelper_Test, TestConvertArrayDontModifyInput){
    std::vector<double> data = {1,2,3,4};
    std::vector<double> data2 = {2,4,6,8};
    std::vector<double> expected = {1000, 2000, 3000, 4000};
    //Call the converter, updates data in place
    UnitsHelper::convert_values("m", data.data(), "mm", data2.data(), data.size());
    ASSERT_EQ( expected,  data2);
    ASSERT_EQ( data.at(2), 3);
}

TEST_F(UnitsHelper_Test, TestConvertArrayDontModifyInputNoOp){
    std::vector<double> data = {1,2,3,4};
    std::vector<double> data2 = {2,4,6,8};
    std::vector<double> expected = {1, 2, 3, 4};
    //Call the converter, updates data in place
    UnitsHelper::convert_values("m", data.data(), "m", data2.data(), data.size());
    ASSERT_EQ( expected,  data2);
    ASSERT_EQ( data.at(2), 3);
}

/**
 * Reducing a pair of units is what makes is_identity() answerable.
 *
 * The none-ish spellings all mean the same thing, so they have to be collapsed before the question becomes
 * a comparison. Asked of unreduced units it would answer "a conversion is needed" for two units that both
 * mean "none", which is the whole reason the reduced form exists.
 */
TEST_F(UnitsHelper_Test, ReducingRecognizesEveryNoneishSpelling){
    for (const std::string none : {"", "none", "unitless", "dimensionless", "-"}) {
        // Neither side states units: nothing to convert, however each side spells it.
        for (const std::string other : {"", "none", "unitless", "dimensionless", "-"}) {
            ASSERT_TRUE(UnitsHelper::normalized_units(none, other).is_identity()) << none << " -> " << other;
        }
        // An unstated requirement takes whatever the provider has.
        ASSERT_TRUE(UnitsHelper::normalized_units("m", none).is_identity()) << "m -> " << none;
        // An unstated input cannot be known to satisfy a stated requirement.
        ASSERT_FALSE(UnitsHelper::normalized_units(none, "m").is_identity()) << none << " -> m";
    }
}

TEST_F(UnitsHelper_Test, IdentityAgreesWithWhatConversionActuallyDoes){
    // Same units, and convertible-but-different units.
    ASSERT_TRUE(UnitsHelper::normalized_units("m", "m").is_identity());
    ASSERT_FALSE(UnitsHelper::normalized_units("m", "mm").is_identity());

    // Where it reports identity, converting leaves the value alone.
    std::vector<double> value = {2.5};
    UnitsHelper::convert_values("m", value.data(), "none", value.data(), value.size());
    ASSERT_DOUBLE_EQ(value[0], 2.5);

    // Where it does not, converting changes it.
    value[0] = 2.5;
    UnitsHelper::convert_values("m", value.data(), "mm", value.data(), value.size());
    ASSERT_DOUBLE_EQ(value[0], 2500.0);
}
