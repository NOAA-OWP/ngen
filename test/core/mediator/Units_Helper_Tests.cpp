#include "gtest/gtest.h"

#include "core/mediator/Units_Helper.hpp"

class Units_Helper_Test : public ::testing::Test {

    public:
        void SetUp() override {
        }

    protected:

        Units_Helper_Test() {
        }

        ~Units_Helper_Test() {
        }

};

TEST_F(Units_Helper_Test, TestDoAConversion){
    ASSERT_NEAR(32.0, Units_Helper::get_converted_value("C", 0, "F"), 0.000000001);

}

