#include "gtest/gtest.h"
#include "FileChecker.h"
#include "GIUH.hpp"
#include "GiuhJsonReader.h"
#include <vector>
#include <cmath>

class GIUH_Test : public ::testing::Test {

protected:

    GIUH_Test()
    {
        std::vector<std::string> abridged_choices = {"test/data/giuh/GIUH_abridged.json",
                                                     "../test/data/giuh/GIUH_abridged.json",
                                                     "../../test/data/giuh/GIUH_abridged.json",
                                                     "data/giuh/GIUH_abridged.json"};
        abridged_json_file = utils::FileChecker::find_first_readable(abridged_choices);

        std::vector<std::string> complete_choices = {"test/data/giuh/GIUH.json",
                                                     "../test/data/giuh/GIUH.json",
                                                     "../../test/data/giuh/GIUH.json",
                                                     "data/giuh/GIUH.json"};
        complete_json_file = utils::FileChecker::find_first_readable(complete_choices);

        std::vector<std::string> id_map_choices = {"../../data/crosswalk.json",
                                                   "../data/crosswalk.json",
                                                   "data/crosswalk.json",
                                                   "data/sugar_creek/crosswalk_subset.json",
                                                   "../data/sugar_creek/crosswalk_subset.json",
                                                   "../../data/sugar_creek/crosswalk_subset.json"};
        id_map_json_file = utils::FileChecker::find_first_readable(id_map_choices);
    }

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

    std::string expected_comid_0 = "9731286";
    std::string catcment_id_0 = "cat-67";
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

    std::string catchment_id_0 = "cat-67";

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

TEST_F(GIUH_Test, TestManualOrdinates0)
{
    std::vector<double> cdf_times {0, 3600, 7200, 10800, 14400};
    std::vector<double> cdf_freq {0.06, 0.51, 0.28, 0.12, 0.03};

    std::vector<double> real_cdf_freq(cdf_freq.size());

    double sum = 0;
    for (unsigned int i = 0; i < cdf_freq.size(); ++i) {
        sum += cdf_freq[i];
        real_cdf_freq[i] = sum;
    }

    giuh::giuh_kernel_impl kernel = giuh::giuh_kernel_impl("cat-67","none", cdf_times, real_cdf_freq);
    kernel.set_interpolation_regularity_seconds(3600);

    ASSERT_TRUE(true);
}

//! Test that giuh_kernel objects output using some real sample data and manually passed ordinates.
TEST_F(GIUH_Test, TestOutput1)
{
    // First make sure we have the file, looking in a few different places
    std::vector<std::string> possible_files = {"./giuh_test_samples.json", "../giuh_test_samples.json",
                                               "./test/data/giuh/giuh_test_samples.json",
                                               "../test/data/giuh/giuh_test_samples.json",
                                               "../../test/data/giuh/giuh_test_samples.json"};
    std::string sample_data_file;
    bool found_sample_data_file = false;

    for (unsigned int i = 0; i < possible_files.size(); ++i) {
        if (FILE *file = fopen(possible_files[i].c_str(), "r")) {
            fclose(file);
            found_sample_data_file = true;
            sample_data_file = possible_files[i];
        }
    }

    ASSERT_TRUE(found_sample_data_file);

    boost::property_tree::ptree json_sample_data;
    boost::property_tree::json_parser::read_json(sample_data_file, json_sample_data);

    std::vector<int> times;
    std::vector<double> runoff_inputs;
    std::vector<double> giuh_outputs;

    for (boost::property_tree::ptree::iterator pos = json_sample_data.begin(); pos != json_sample_data.end(); ++pos) {
        // TODO: might need to adjust the hour value to be explicit value plus 1, and then multiply by 60
        int hours_time = pos->second.get_child("hours").get_value<int>();
        times.push_back(hours_time * 3600);
        runoff_inputs.push_back(pos->second.get_child("directRunoff").get_value<double>());
        giuh_outputs.push_back(pos->second.get_child("giuh").get_value<double>());
    }

    std::vector<double> incremental_values {0.06, 0.51, 0.28, 0.12, 0.03};
    std::vector<double> ordinates(incremental_values.size() + 1);
    std::vector<double> ordinate_times(incremental_values.size() + 1);

    double ordinate_sum = 0.0;
    int time_sum = 0;
    ordinates.push_back(ordinate_sum);
    ordinate_times.push_back(time_sum);
    for (unsigned int i = 1; i < ordinates.size(); ++i) {
        ordinate_sum += incremental_values[i-1];
        ordinates[i] = ordinate_sum;
        time_sum += 3600;
        ordinate_times[i] = time_sum;
    }

    giuh::giuh_kernel_impl kernel("test", "test", ordinate_times, ordinates, 3600);

    double leaway = 0.000001;

    for (unsigned int i = 0; i < times.size(); ++i) {
        double kernel_output = kernel.calc_giuh_output(3600, runoff_inputs[i]);
        double diff_abs = std::fabs(kernel_output - giuh_outputs[i]);
        EXPECT_LE(diff_abs, leaway);
    }
}

TEST_F(GIUH_Test, TestOutput2) {
    // First make sure we have the file, looking in a few different places
    std::vector<std::string> possible_files = {"./giuh_test_samples.json", "../giuh_test_samples.json",
                                               "./test/data/giuh/giuh_test_samples.json",
                                               "../test/data/giuh/giuh_test_samples.json",
                                               "../../test/data/giuh/giuh_test_samples.json"};
    std::string sample_data_file;
    bool found_sample_data_file = false;

    for (unsigned int i = 0; i < possible_files.size(); ++i) {
        if (FILE *file = fopen(possible_files[i].c_str(), "r")) {
            fclose(file);
            found_sample_data_file = true;
            sample_data_file = possible_files[i];
        }
    }

    ASSERT_TRUE(found_sample_data_file);

    boost::property_tree::ptree json_sample_data;
    boost::property_tree::json_parser::read_json(sample_data_file, json_sample_data);

    std::vector<int> times;
    std::vector<double> runoff_inputs;
    std::vector<double> giuh_outputs;

    for (boost::property_tree::ptree::iterator pos = json_sample_data.begin(); pos != json_sample_data.end(); ++pos) {
        // TODO: might need to adjust the hour value to be explicit value plus 1, and then multiply by 60
        int hours_time = pos->second.get_child("hours").get_value<int>();
        times.push_back(hours_time * 3600);
        runoff_inputs.push_back(pos->second.get_child("directRunoff").get_value<double>());
        giuh_outputs.push_back(pos->second.get_child("giuh").get_value<double>());
    }

    std::vector<double> incremental_values {0.06, 0.51, 0.28, 0.12, 0.03};

    giuh::giuh_kernel_impl kernel = giuh::giuh_kernel_impl::make_from_incremental_runoffs("test", "test", 3600, incremental_values);

    double leaway = 0.000001;

    for (unsigned int i = 0; i < times.size(); ++i) {
        double kernel_output = kernel.calc_giuh_output(3600, runoff_inputs[i]);
        double diff_abs = std::fabs(kernel_output - giuh_outputs[i]);
        EXPECT_LE(diff_abs, leaway);
    }
}
