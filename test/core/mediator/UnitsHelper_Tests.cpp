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

