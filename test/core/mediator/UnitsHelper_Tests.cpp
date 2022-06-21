#include "gtest/gtest.h"

#include "core/mediator/UnitsHelper.hpp"

class UnitsHelper_Test : public ::testing::Test {

    public:
        void SetUp() override {
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
