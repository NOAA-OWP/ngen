#include "gtest/gtest.h"
#include "GIUH.hpp"
#include "GiuhJsonReader.h"
#include <vector>

class GIUH_Test : public ::testing::Test {

protected:

    GIUH_Test() : abridged_json_file("../test/data/giuh/GIUH_abridged.json"),
                  complete_json_file("../test/data/giuh/GIUH.json"),
                  id_map_json_file("../data/sugar_creek/crosswalk_subset.json") {}

    ~GIUH_Test() override {

    }

    void SetUp() override;

    void TearDown() override;

    std::string abridged_json_file;
    std::string complete_json_file;
    std::string id_map_json_file;

};

void GIUH_Test::SetUp() {


}

void GIUH_Test::TearDown() {

}

//! Test that giuh_kernel objects initialize properly.
TEST_F(GIUH_Test, TestInit0)
{
    //std::string json_file = abridged_json_file;
    std::string json_file = complete_json_file;

    giuh::GiuhJsonReader reader = giuh::GiuhJsonReader(json_file, id_map_json_file);

    std::string expected_comid_0 = "9731236";
    std::string catcment_id_0 = "wat-88";
    std::string comid_0 = reader.get_associated_comid(catcment_id_0);
    ASSERT_EQ(comid_0, expected_comid_0);

    std::shared_ptr<giuh::giuh_kernel> kernel_obj = reader.get_giuh_kernel_for_id(catcment_id_0);

    ASSERT_TRUE(kernel_obj != nullptr);
}

//! Test that giuh_kernel objects output properly.
TEST_F(GIUH_Test, TestOutput0)
{
    //std::string json_file = abridged_json_file;
    std::string json_file = complete_json_file;

    giuh::GiuhJsonReader reader = giuh::GiuhJsonReader(json_file, id_map_json_file);

    std::string catchment_id_0 = "wat-88";

    std::shared_ptr<giuh::giuh_kernel> kernel_obj = reader.get_giuh_kernel_for_id(catchment_id_0);

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