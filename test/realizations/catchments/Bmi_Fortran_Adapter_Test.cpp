#ifdef NGEN_BMI_FORTRAN_LIB_TESTS_ACTIVE

#include "gtest/gtest.h"
#include <vector>
#include <exception>
#include <chrono>
#include <unordered_map>
#include <numeric>

#include "FileChecker.h"
#include "Bmi_Fortran_Adapter.hpp"
#include "State_Exception.hpp"

#ifndef BMI_TEST_FORTRAN_LOCAL_LIB_NAME
#ifdef __APPLE__
#define BMI_TEST_FORTRAN_LOCAL_LIB_NAME "libtestbmifortranmodel.dylib"
#else
#ifdef __GNUC__
    #define BMI_TEST_FORTRAN_LOCAL_LIB_NAME "libtestbmifortranmodel.so"
    #endif // __GNUC__
#endif // __APPLE__
#endif // BMI_TEST_FORTRAN_LOCAL_LIB_NAME

#define REGISTRATION_FUNC "register_bmi"

using namespace models::bmi;

class Bmi_Fortran_Adapter_Test : public ::testing::Test {
protected:

    void SetUp() override;

    void TearDown() override;

    std::string file_search(const std::vector<std::string> &parent_dir_options, const std::string& file_basename);

    /*
    static model_data* friend_get_model_data_struct(Bmi_Fortran_Adapter *adapter) {
        return (model_data*) adapter->bmi_model->data;
    }
     */

    std::string config_file_name_0;
    std::string lib_file_name_0;
    std::string forcing_file_name_0;
    std::string bmi_module_type_name_0;
    std::unique_ptr<Bmi_Fortran_Adapter> adapter;

    std::vector<std::string> expected_output_var_names = { "OUTPUT_VAR_1", "OUTPUT_VAR_2" };
    std::vector<std::string> expected_output_var_locations = { "node", "node" };
    std::vector<int> expected_output_var_grids = { 0, 0 };
    std::vector<std::string> expected_output_var_units = { "m", "m" };
    std::vector<std::string> expected_output_var_types = { "double", "double" };
    int expected_grid_rank = 1;
    int expected_grid_size = 1;
    std::string expected_grid_type = "scalar";
    int expected_var_nbytes = 8; //type double

};

void Bmi_Fortran_Adapter_Test::SetUp() {
    std::vector<std::string> config_path_options = {
            "test/data/bmi/test_bmi_fortran/",
            "./test/data/bmi/test_bmi_fortran/",
            "../test/data/bmi/test_bmi_fortran/",
            "../../test/data/bmi/test_bmi_fortran/",
    };
    std::string config_basename_0 = "test_bmi_fortran_config_0.txt";
    config_file_name_0 = file_search(config_path_options, config_basename_0);

    std::vector<std::string> forcing_dir_opts = {"./data/forcing/", "../data/forcing/", "../../data/forcing/"};
    forcing_file_name_0 = file_search(forcing_dir_opts, "cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");

    std::vector<std::string> lib_dir_opts = {
            "./extern/test_bmi_fortran/cmake_build/",
            "../extern/test_bmi_fortran/cmake_build/",
            "../../extern/test_bmi_fortran/cmake_build/"
    };
    lib_file_name_0 = file_search(lib_dir_opts, BMI_TEST_FORTRAN_LOCAL_LIB_NAME);
    bmi_module_type_name_0 = "test_bmi_fortran";
    adapter = std::make_unique<Bmi_Fortran_Adapter>(bmi_module_type_name_0, lib_file_name_0, config_file_name_0, 
                                              forcing_file_name_0, false, true, REGISTRATION_FUNC,
                                              utils::StreamHandler());
}

void Bmi_Fortran_Adapter_Test::TearDown() {

}

std::string
Bmi_Fortran_Adapter_Test::file_search(const std::vector<std::string> &parent_dir_options, const std::string& file_basename) {
    // Build vector of names by building combinations of the path and basename options
    std::vector<std::string> name_combinations;

    // Build so that all path names are tried for given basename before trying a different basename option
    for (auto & path_option : parent_dir_options)
        name_combinations.push_back(path_option + file_basename);

    return utils::FileChecker::find_first_readable(name_combinations);
}

/** Simple test to make sure the model initializes. */
TEST_F(Bmi_Fortran_Adapter_Test, Initialize_0_a) {
    adapter->Initialize();
    adapter->Finalize();
}

/** Test output variables can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetOutputVarNames_0_a) {
    try {
        ASSERT_EQ(adapter->GetOutputVarNames(), expected_output_var_names);
    }
    catch (std::exception& e) {
        printf("Exception getting output var names: %s", e.what());
    }
}

/** Test output variables item count can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetOutputItemCount_0_a) {
    try {
        ASSERT_EQ(adapter->GetOutputItemCount(), expected_output_var_names.size());
    }
    catch (std::exception& e) {
        printf("Exception getting output var count: %s", e.what());
    }
}

/** Test that both the get value function works for input 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_0_a) {
    adapter->Initialize();
    double value = 5.0;
    adapter->SetValue("INPUT_VAR_1", &value);
    double retrieved = adapter->GetValue<double>("INPUT_VAR_1")[0];
    adapter->Finalize();
    ASSERT_EQ(value, retrieved);
}

/** Test that both the get value function works for input 2. */
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_0_b) {
    adapter->Initialize();
    double value = 6.0;
    adapter->SetValue("INPUT_VAR_2", &value);
    double retrieved = adapter->GetValue<double>("INPUT_VAR_2")[0];
    adapter->Finalize();
    ASSERT_EQ(value, retrieved);
}

/** Test that both the get value function works for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_0_c) {
    adapter->Initialize();
    double value1 = 7.0;
    double value2 = 8.0;
    double expectedOutput1 = 0; // TODO: fix this
    adapter->SetValue("INPUT_VAR_1", &value1);
    adapter->SetValue("INPUT_VAR_2", &value2);
    adapter->Update();
    double retrieved = adapter->GetValue<double>("OUTPUT_VAR_1")[0];
    adapter->Finalize();
    ASSERT_EQ(expectedOutput1, retrieved);
}

/** Profile the update function and GetValues functions */
TEST_F(Bmi_Fortran_Adapter_Test, Profile)
{
    int num_ouput_items = adapter->GetOutputItemCount();
    int num_input_items = adapter->GetInputItemCount();

    std::vector<std::string> output_names = adapter->GetOutputVarNames();
    std::vector<std::string> input_names = adapter->GetInputVarNames();

    std::unordered_map<std::string, std::vector<long> > saved_times;


    using time_point = std::chrono::time_point<std::chrono::steady_clock>;
    auto to_micros = [](const time_point& s, const time_point& e){ return std::chrono::duration_cast<std::chrono::microseconds>(e - s).count();};

    adapter->Initialize();
    // Do the first few time steps
    for (int i = 0; i < 720; i++)
    {
        // record time for Update
        auto s1 = std::chrono::steady_clock::now();
        adapter->Update();
        auto e1 = std::chrono::steady_clock::now();
        saved_times["Update"].push_back(to_micros(s1,e1));

        // record time to get each output variable
        for(std::string name : output_names)
        {
            auto s2 = std::chrono::steady_clock::now();
            std::vector<double> values = adapter->GetValue<double>(name);
            auto e2 = std::chrono::steady_clock::now();
            saved_times["Get " + name].push_back(to_micros(s2,e2));
        }

        // record time to get each input variable
        for(std::string name : input_names)
        {
            auto s3 = std::chrono::steady_clock::now();
            std::vector<double> values = adapter->GetValue<double>(name);
            auto e3 = std::chrono::steady_clock::now();
            saved_times["Get " + name].push_back(to_micros(s3,e3));
        }

    }

    // calcuate average time for each record

    for ( auto& kv : saved_times )
    {
        auto key = kv.first;
        auto& v = kv.second;
        long sum = 0;

        sum = std::accumulate(v.begin(), v.end(), sum );
        double average = double(sum) / v.size();

        std::cout << "Average time for " << key << " = " << average << "µs\n";
    }

    adapter->Finalize();
}

/** Test that both the get value function works for output 2. */
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_0_d) {
    adapter->Initialize();
    double value1 = 7.0;
    double value2 = 8.0;
    double expectedOutput2 = 0; // TODO: fix this
    adapter->SetValue("INPUT_VAR_1", &value1);
    adapter->SetValue("INPUT_VAR_2", &value2);
    adapter->Update();
    double retrieved = adapter->GetValue<double>("OUTPUT_VAR_2")[0];
    adapter->Finalize();
    ASSERT_EQ(expectedOutput2, retrieved);
}

/** Test the function for getting start time. */
TEST_F(Bmi_Fortran_Adapter_Test, GetStartTime_0_a) {
    adapter->Initialize();
    ASSERT_EQ(0, adapter->GetStartTime());
    adapter->Finalize();
}

/** Test the function for getting time step size. */
TEST_F(Bmi_Fortran_Adapter_Test, GetTimeStep_0_a) {
    adapter->Initialize();
    ASSERT_EQ(3600, adapter->GetTimeStep());
    adapter->Finalize();
}

/** Test the function for getting model time units. */
TEST_F(Bmi_Fortran_Adapter_Test, GetTimeUnits_0_a) {
    adapter->Initialize();
    ASSERT_EQ("s", adapter->GetTimeUnits());
    adapter->Finalize();
}

/** Test the function for getting model end time, assuming (as in used config) 720 time steps. */
TEST_F(Bmi_Fortran_Adapter_Test, GetEndTime_0_a) {
    adapter->Initialize();
    double expected = adapter->GetStartTime() + (720.0 * adapter->GetTimeStep());
    ASSERT_EQ(expected, adapter->GetEndTime());
    adapter->Finalize();
}

/** Test that getting the current model time works after initialization. */
TEST_F(Bmi_Fortran_Adapter_Test, GetCurrentTime_0_a) {
    adapter->Initialize();
    ASSERT_EQ(adapter->GetStartTime(), adapter->GetCurrentTime());
    adapter->Finalize();
}

/** Test that getting the current model time works after an update. */
TEST_F(Bmi_Fortran_Adapter_Test, GetCurrentTime_0_b) {
    adapter->Initialize();
    double expected_time = adapter->GetStartTime() + adapter->GetTimeStep();
    double value = 10.0;
    adapter->SetValue("INPUT_VAR_1", &value);
    adapter->SetValue("INPUT_VAR_2", &value);
    adapter->Update();
    ASSERT_EQ(expected_time, adapter->GetCurrentTime());
    adapter->Finalize();
}

/** Test that both the set value function works for input 1. */
TEST_F(Bmi_Fortran_Adapter_Test, SetValue_0_a) {
    adapter->Initialize();
    double value = 5.0;
    adapter->SetValue("INPUT_VAR_1", &value);
    double retrieved = adapter->GetValue<double>("INPUT_VAR_1")[0];
    adapter->Finalize();
    ASSERT_EQ(value, retrieved);
}

/** Test that both the set value function works for input 2. */
TEST_F(Bmi_Fortran_Adapter_Test, SetValue_0_b) {
    adapter->Initialize();
    double value = 6.0;
    adapter->SetValue("INPUT_VAR_2", &value);
    double retrieved = adapter->GetValue<double>("INPUT_VAR_2")[0];
    adapter->Finalize();
    ASSERT_EQ(value, retrieved);
}

/** Test that the set value function works for input 1 for multiple calls. */
TEST_F(Bmi_Fortran_Adapter_Test, SetValue_0_c) {
    adapter->Initialize();
    double value_1 = 7.0;
    adapter->SetValue("INPUT_VAR_1", &value_1);
    double value_2 = 10.0;
    adapter->SetValue("INPUT_VAR_1", &value_2);
    double retrieved = adapter->GetValue<double>("INPUT_VAR_1")[0];
    adapter->Finalize();
    ASSERT_EQ(value_2, retrieved);
}

/** Test that the update function works for a single update. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_0_a) {
    adapter->Initialize();
    double value_1 = 7.0;
    double value_2 = 10.0;
    adapter->SetValue("INPUT_VAR_1", &value_1);
    adapter->SetValue("INPUT_VAR_2", &value_2);
    adapter->Update();
    adapter->Finalize();
}

/** Test that the update function works for a single update and produces the expected value for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_0_b) {
    adapter->Initialize();
    double value_1 = 7.0;
    double value_2 = 10.0;
    adapter->SetValue("INPUT_VAR_1", &value_1);
    adapter->SetValue("INPUT_VAR_2", &value_2);
    adapter->Update();
    ASSERT_EQ(value_1, adapter->GetValue<double>("OUTPUT_VAR_1")[0]);
    adapter->Finalize();
}

/** Test that the update function works for a single update and produces the expected value for output 2. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_0_c) {
    adapter->Initialize();
    double value_1 = 7.0;
    double value_2 = 10.0;
    adapter->SetValue("INPUT_VAR_1", &value_1);
    adapter->SetValue("INPUT_VAR_2", &value_2);
    adapter->Update();
    ASSERT_EQ(value_2 * 2, adapter->GetValue<double>("OUTPUT_VAR_2")[0]);
    adapter->Finalize();
}

/** Test that the update function works for the 720 time steps and gets the expected output 1 values. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_1_a) {
    size_t num_steps = 720;
    std::vector<double> expected(num_steps);
    std::vector<double> out_1_vals(num_steps);
    adapter->Initialize();
    double value;
    for (size_t i = 0; i < num_steps; ++i) {
        value = 2.0 * i;
        expected[i] = value;
        adapter->SetValue("INPUT_VAR_1", &value);
        adapter->SetValue("INPUT_VAR_2", &value);
        adapter->Update();
        out_1_vals[i] = adapter->GetValue<double>("OUTPUT_VAR_1")[0];
    }
    adapter->Finalize();
    ASSERT_EQ(expected, out_1_vals);
}

/** Test that the update function works for the 720 time steps and gets the expected output 2 values. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_1_b) {
    size_t num_steps = 720;
    std::vector<double> expected(num_steps);
    std::vector<double> out_2_vals(num_steps);
    adapter->Initialize();
    double value;
    for (size_t i = 0; i < num_steps; ++i) {
        value = 2.0 * i;
        expected[i] = value * 2;
        adapter->SetValue("INPUT_VAR_1", &value);
        adapter->SetValue("INPUT_VAR_2", &value);
        adapter->Update();
        out_2_vals[i] = adapter->GetValue<double>("OUTPUT_VAR_2")[0];
    }
    adapter->Finalize();
    ASSERT_EQ(expected, out_2_vals);
}

/** Test that the update_until function works for a single update and produces the expected value for output 2. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_until_0_a) {
    adapter->Initialize();
    double value_1 = 7.0;
    double value_2 = 10.0;
    adapter->SetValue("INPUT_VAR_1", &value_1);
    adapter->SetValue("INPUT_VAR_2", &value_2);
    // This basically is a step of 1.5 times the normal length
    double time = 5400 + adapter->GetCurrentTime();
    adapter->UpdateUntil(time);
    // Normally and update would produce out_2 as 2 * input_2, but here must further multiply by 1.5 for the longer time
    ASSERT_EQ(value_2 * 2.0 * 1.5, adapter->GetValue<double>("OUTPUT_VAR_2")[0]);
    adapter->Finalize();
}

/** Test that the update_until function works and advances the model time. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_until_0_b) {
    adapter->Initialize();
    double value_1 = 7.0;
    double value_2 = 10.0;
    adapter->SetValue("INPUT_VAR_1", &value_1);
    adapter->SetValue("INPUT_VAR_2", &value_2);
    // This basically is a step of 1.5 times the normal length
    double time = 5400 + adapter->GetCurrentTime();
    adapter->UpdateUntil(time);
    ASSERT_EQ(time, adapter->GetCurrentTime());
    adapter->Finalize();
}

/** Test output 1 variable grid (id) can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarGrid_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int expected_grid = expected_output_var_grids[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarGrid(variable_name), expected_grid);
    }
    catch (std::exception& e) {
        printf("Exception getting var grid id: %s", e.what());
    }
}

/** Test output 2 variable grid (id) can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarGrid_0_b) {
    int out_var_index = 1;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int expected_grid = expected_output_var_grids[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarGrid(variable_name), expected_grid);
    }
    catch (std::exception& e) {
        printf("Exception getting var grid id: %s", e.what());
    }
}

/** Test output 1 variable location can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarLocation_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    std::string expected_location = expected_output_var_locations[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarLocation(variable_name), expected_location);
    }
    catch (std::exception& e) {
        printf("Exception getting var location: %s", e.what());
    }
}

/** Test output 2 variable location can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarLocation_0_b) {
    int out_var_index = 1;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    std::string expected_location = expected_output_var_locations[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarLocation(variable_name), expected_location);
    }
    catch (std::exception& e) {
        printf("Exception getting var location: %s", e.what());
    }
}

/** Test output 1 variable units can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarUnits_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    std::string expected_units = expected_output_var_units[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarUnits(variable_name), expected_units);
    }
    catch (std::exception& e) {
        printf("Exception getting var units: %s", e.what());
    }
}

/** Test output 1 variable type can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarType_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    std::string expected_type = expected_output_var_types[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarType(variable_name), expected_type);
    }
    catch (std::exception& e) {
        printf("Exception getting var type: %s", e.what());
    }
}


/** Test output 1 variable nbytes can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarNbytes_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarNbytes(variable_name), expected_var_nbytes);
    }
    catch (std::exception& e) {
        printf("Exception getting var nbytes: %s", e.what());
    }
}
// Test model GetVarGrid() function is disabled currently.  Suggest disabling test until fully implemented.
/** Test grid type can be retrieved for output 1 */
TEST_F(Bmi_Fortran_Adapter_Test, DISABLED_GetGridType_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);

    try {
        ASSERT_EQ(adapter->GetGridType(grd), expected_grid_type);
    }
    catch (std::exception& e) {
        printf("Exception getting grid type: %s", e.what());
    }
}

// CFE GetVarGrid() function is disabled currently.  Suggest disabling test until pure testing BMI module is developed.
/** Test grid rank can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, DISABLED_GetGridRank_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);

    try {
        ASSERT_EQ(adapter->GetGridRank(grd), expected_grid_rank);
    }
    catch (std::exception& e) {
        printf("Exception getting grid rank: %s", e.what());
    }
}

// CFE GetVarGrid() function is disabled currently.  Suggest disabling test until pure testing BMI module is developed.
/** Test grid size can be retrieved for output 2. */
TEST_F(Bmi_Fortran_Adapter_Test, DISABLED_GetGridSize_0_a) {
    int out_var_index = 1;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);

    try {
        ASSERT_EQ(adapter->GetGridSize(grd), expected_grid_size);
    }
    catch (std::exception& e) {
        printf("Exception getting grid size: %s", e.what());
    }
}

#endif  // NGEN_BMI_FORTRAN_LIB_TESTS_ACTIVE
