#include "gtest/gtest.h"
#include "GIUH.hpp"
#include "GiuhJsonReader.h"
#include <vector>

class GIUH_Test : public ::testing::Test {

protected:

    GIUH_Test() : abridged_reader(giuh::GiuhJsonReader("../test/data/giuh/GIUH_abridged.json")) {}

    ~GIUH_Test() override {

    }

    void SetUp() override;

    void TearDown() override;

    giuh::GiuhJsonReader abridged_reader;

};

void GIUH_Test::SetUp() {


}

void GIUH_Test::TearDown() {

}

//! Test that giuh_kernel objects initialize properly.
TEST_F(GIUH_Test, TestInit0)
{
    std::string id_0 = "9731300";

    std::shared_ptr<giuh::giuh_kernel> kernel_obj = abridged_reader.get_giuh_kernel_for_id(id_0);

    ASSERT_TRUE(kernel_obj != nullptr);
}

//! Test that giuh_kernel objects output properly.
TEST_F(GIUH_Test, TestOutput0)
{
    std::string id_0 = "9731300";

    std::shared_ptr<giuh::giuh_kernel> kernel_obj = abridged_reader.get_giuh_kernel_for_id(id_0);

    std::vector<double> inputs = {10000, 10000, 100, 0, 100};
    std::vector<double> time_steps = {86400, 86400, 300, 300, 300};
    std::vector<double> outputs(inputs.size());

    double total_inputs = 0;
    double total_outputs = 0;

    for (unsigned int i = 0; i < inputs.size(); ++i) {
        // The most basic sanity check is that inputs don't exceed outputs
        outputs[i] = kernel_obj->calc_giuh_output(time_steps[i], inputs[i]);
        total_inputs += inputs[i];
        total_outputs += outputs[i];
        ASSERT_LE(outputs[i], total_inputs);
    }

    // TODO: add more assertions that, including those checking math more precisely

    ASSERT_TRUE(kernel_obj != nullptr);
}