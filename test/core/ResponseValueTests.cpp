#include "gtest/gtest.h"

#include "ResponseValue.hpp"

using namespace responsevalue;

class ResponseValue_Test : public ::testing::Test {

    public:
        std::shared_ptr<ResponseValue> rvo;

        void SetUp() override {
            rvo = std::make_shared<ResponseValue>();
        }

    protected:

        ResponseValue_Test() {
        }

        ~ResponseValue_Test() {
        }

};

TEST_F(ResponseValue_Test, TestGetSetValues){
    ASSERT_EQ(rvo->get_value("the-answer"), 0);
    rvo->set_value("the-answer", 42.0);
    rvo->set_value("beeblebrox-number-of-heads", 2.0);
    ASSERT_EQ(rvo->get_value("the-answer"), 42.0);
    ASSERT_EQ(rvo->get_value("beeblebrox-number-of-heads"), 2.0);
}

TEST_F(ResponseValue_Test, TestPrimaryValue)
{
    ASSERT_EQ( rvo->get_primary(), 0.0 );
    rvo->set_primary(42.0);
    ASSERT_EQ( rvo->get_primary(), 42.0 );
    rvo->set_primary_name("the-answer");
    ASSERT_EQ( rvo->get_primary(), 0.0);

    ASSERT_EQ( rvo->get_value(""), 42.0);

    rvo->set_primary(42.0);
    ASSERT_EQ( rvo->get_primary(), 42.0 );
}

TEST_F(ResponseValue_Test, TestSetUnits){
    rvo->set_value("the-answer", 42.0);
    ASSERT_EQ( rvo->get_input_var_units("the-answer"), "");
    rvo->set_input_var_units("the-answer", "unknown");
    ASSERT_EQ( rvo->get_input_var_units("the-answer"), "unknown" );
    rvo->set_output_var_units("the-answer", "how-many-roads-must-a-man-walk-down");
    ASSERT_EQ( rvo->get_output_var_units("the-answer"), "how-many-roads-must-a-man-walk-down" );
    ASSERT_THROW( rvo->get_value("the-answer"), std::exception );
}
