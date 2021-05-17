#ifdef NGEN_BMI_C_LIB_TESTS_ACTIVE

#include "gtest/gtest.h"
#include <vector>
#include <exception>
#include "FileChecker.h"
#include "Bmi_C_Adapter.hpp"
#include "State_Exception.hpp"

#ifndef BMI_CFE_LOCAL_LIB_NAME
#ifdef __APPLE__
#define BMI_CFE_LOCAL_LIB_NAME "libcfebmi.dylib"
#else
#ifdef __GNUC__
    #define BMI_CFE_LOCAL_LIB_NAME "libcfebmi.so"
    #endif // __GNUC__
#endif // __APPLE__
#endif // BMI_CFE_LOCAL_LIB_NAME

using namespace models::bmi;

class Bmi_C_Adapter_Test : public ::testing::Test {
protected:

    void SetUp() override;

    void TearDown() override;

    std::string file_search(const std::vector<std::string> &parent_dir_options, const std::string& file_basename);

    std::string config_file_name_0;
    std::string lib_file_name_0;
    std::string forcing_file_name_0;

    std::vector<std::string> expected_output_var_names = {
            "RAIN_RATE",
            "SCHAAKE_OUTPUT_RUNOFF",
            "GIUH_RUNOFF",
            "NASH_LATERAL_RUNOFF",
            "DEEP_GW_TO_CHANNEL_FLUX",
            "Q_OUT"
    };
    
    std::vector<std::string> expected_output_var_locations = {
            "node",
            "node",
            "node",
            "node",
            "node",
            "node"
    };

    std::vector<int> expected_output_var_grids = {
            0,
            0,
            0,
            0,
            0,
            0
    };

    std::vector<std::string> expected_output_var_units = {
            "m",
            "m",
            "m",
            "m",
            "m",
            "m"
    };

    std::vector<std::string> expected_output_var_types = {
            "double",
            "double",
            "double",
            "double",
            "double",
            "double"
    };

    int expected_grid_rank = 1;
    
    int expected_grid_size = 1;
    
    std::string expected_grid_type = "scalar";

    int expected_var_nbytes = 8; //type double

};

void Bmi_C_Adapter_Test::SetUp() {
    std::vector<std::string> config_path_options = {
            "test/data/bmi/c/cfe/",
            "./test/data/bmi/c/cfe/",
            "../test/data/bmi/c/cfe/",
            "../../test/data/bmi/c/cfe/",
    };
    std::string config_basename_0 = "cat_27_bmi_config.txt";
    config_file_name_0 = file_search(config_path_options, config_basename_0);

    std::vector<std::string> forcing_dir_opts = {"./data/forcing/", "../data/forcing/", "../../data/forcing/"};
    forcing_file_name_0 = file_search(forcing_dir_opts, "cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");

    std::vector<std::string> lib_dir_opts = {
            "./extern/alt-modular/cmake_am_libs/",
            "../extern/alt-modular/cmake_am_libs/",
            "../../extern/alt-modular/cmake_am_libs/"
    };
    lib_file_name_0 = file_search(lib_dir_opts, BMI_CFE_LOCAL_LIB_NAME);
}

void Bmi_C_Adapter_Test::TearDown() {

}

std::string
Bmi_C_Adapter_Test::file_search(const std::vector<std::string> &parent_dir_options, const std::string& file_basename) {
    // Build vector of names by building combinations of the path and basename options
    std::vector<std::string> name_combinations;

    // Build so that all path names are tried for given basename before trying a different basename option
    for (auto & path_option : parent_dir_options)
        name_combinations.push_back(path_option + file_basename);

    return utils::FileChecker::find_first_readable(name_combinations);
}

/** Simple test to make sure the model initializes. */
TEST_F(Bmi_C_Adapter_Test, Initialize_0_a) {
    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    adapter.Initialize();
    adapter.Finalize();
}

/** Test output variables can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetOutputVarNames_0_a) {
    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    try {
        ASSERT_EQ(adapter.GetOutputVarNames(), expected_output_var_names);
    }
    catch (std::exception& e) {
        printf("Exception getting output var names: %s", e.what());
    }
}

/** Test output variables item count can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetOutputItemCount_0_a) {
    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    try {
        ASSERT_EQ(adapter.GetOutputItemCount(), expected_output_var_names.size());
    }
    catch (std::exception& e) {
        printf("Exception getting output var count: %s", e.what());
    }
}

/** Test that the update function works. */
TEST_F(Bmi_C_Adapter_Test, Update_0_a) {
    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    adapter.Initialize();
    adapter.Update();
    adapter.Finalize();
}

/** Test that the update function works for the 720 time steps. */
TEST_F(Bmi_C_Adapter_Test, Update_0_b) {
    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    adapter.Initialize();
    for (int i = 0; i < 720; ++i)
        adapter.Update();

    adapter.Finalize();
}

/** Test that the update function works for the 720 time steps and gets the expected outputs. */
TEST_F(Bmi_C_Adapter_Test, Update_0_c) {
    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    adapter.Initialize();
    for (int i = 0; i < 720; ++i)
        adapter.Update();
    std::vector<double> q_out_vals = adapter.GetValue<double>("Q_OUT");
    // TODO: actually assert these values are correct
    adapter.Finalize();
}

/** Test that the update function works for the 720 time steps and gets the expected Schaake runoff. */
TEST_F(Bmi_C_Adapter_Test, Update_0_d) {
    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    adapter.Initialize();
    for (int i = 0; i < 720; ++i)
        adapter.Update();
    std::vector<double> q_out_vals = adapter.GetValue<double>("SCHAAKE_OUTPUT_RUNOFF");
    // TODO: actually assert these values are correct
    adapter.Finalize();
}

/** Test that the update function works for the 720 time steps and gets the expected GIUH runoff. */
TEST_F(Bmi_C_Adapter_Test, Update_0_e) {
    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    adapter.Initialize();
    for (int i = 0; i < 720; ++i)
        adapter.Update();
    std::vector<double> q_out_vals = adapter.GetValue<double>("GIUH_RUNOFF");
    // TODO: actually assert these values are correct
    adapter.Finalize();
}

/** Test that the update function works for the 720 time steps and gets the expected Nash lateral flow runoff. */
TEST_F(Bmi_C_Adapter_Test, Update_0_f) {
    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    adapter.Initialize();
    for (int i = 0; i < 720; ++i)
        adapter.Update();
    std::vector<double> q_out_vals = adapter.GetValue<double>("NASH_LATERAL_RUNOFF");
    // TODO: actually assert these values are correct
    adapter.Finalize();
}

/** Test that the update function works for the 720 time steps and gets the expected deep groundwater to channel flux. */
TEST_F(Bmi_C_Adapter_Test, Update_0_g) {
    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    adapter.Initialize();
    for (int i = 0; i < 720; ++i)
        adapter.Update();
    std::vector<double> q_out_vals = adapter.GetValue<double>("DEEP_GW_TO_CHANNEL_FLUX");
    // TODO: actually assert these values are correct
    adapter.Finalize();
}

/** Test that the update function works fails after exceeding end time. */
TEST_F(Bmi_C_Adapter_Test, Update_0_h) {
    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    adapter.Initialize();
    double start_time = adapter.GetStartTime();
    double end_time = adapter.GetEndTime();
    while (adapter.GetCurrentTime() < adapter.GetEndTime()) {
        adapter.Update();
    }
    bool found_except = false;
    try {
        adapter.Update();
    }
    catch (models::external::State_Exception &e) {
        found_except = true;
    }
    ASSERT_TRUE(found_except);
}

/** Test that Schaake Runoff output variable values can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetValue_0_a_0) {
    int out_var_index = 1;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];

    adapter.Initialize();
    // Do the first few time steps
    for (int i = 0; i < 40; i++)
        adapter.Update();
    std::vector<double> values = adapter.GetValue<double>(variable_name);
    ASSERT_GT(values.size(), 0);
    adapter.Finalize();
}

/** Test that GIUH Runoff output variable values can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetValue_0_a_1) {
    int out_var_index = 2;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];

    adapter.Initialize();
    // Do the first few time steps
    for (int i = 0; i < 40; i++)
        adapter.Update();
    std::vector<double> values = adapter.GetValue<double>(variable_name);
    ASSERT_GT(values.size(), 0);
    adapter.Finalize();
}

/** Test that Nash Lateral Flow output variable values can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetValue_0_a_2) {
    int out_var_index = 3;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];

    adapter.Initialize();
    // Do the first few time steps
    for (int i = 0; i < 40; i++)
        adapter.Update();
    std::vector<double> values = adapter.GetValue<double>(variable_name);
    ASSERT_GT(values.size(), 0);
    adapter.Finalize();
}

/** Test that Deep Groundwater to Channel Flux output variable values can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetValue_0_a_3) {
    int out_var_index = 4;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];

    adapter.Initialize();
    // Do the first few time steps
    for (int i = 0; i < 40; i++)
        adapter.Update();
    std::vector<double> values = adapter.GetValue<double>(variable_name);
    ASSERT_GT(values.size(), 0);
    adapter.Finalize();
}

/** Test that Total Flux (Q_OUT) output variable values can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetValue_0_a_4) {
    int out_var_index = 5;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];

    adapter.Initialize();
    // Do the first few time steps
    for (int i = 0; i < 40; i++)
        adapter.Update();
    std::vector<double> values = adapter.GetValue<double>(variable_name);
    ASSERT_GT(values.size(), 0);
    adapter.Finalize();
}

/** Test GIUH_RUNOFF variable grid (id) can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetVarGrid_0_a) {
    int out_var_index =2;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];
    int expected_grid = expected_output_var_grids[out_var_index];
    
    try {
        ASSERT_EQ(adapter.GetVarGrid(variable_name), expected_grid);
    }
    catch (std::exception& e) {
        printf("Exception getting var grid id: %s", e.what());
    }
}

/** Test DEEP_GW_TO_CHANNEL_FLUX variable grid (id) can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetVarGrid_0_b) {
    int out_var_index = 4;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];
    int expected_grid = expected_output_var_grids[out_var_index];
    
    try {
        ASSERT_EQ(adapter.GetVarGrid(variable_name), expected_grid);
    }
    catch (std::exception& e) {
        printf("Exception getting var grid id: %s", e.what());
    }
}

/** Test SCHAKKE_OUTPUT_RUNOFF variable location can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetVarLocation_0_a) {
    int out_var_index = 1;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];
    std::string expected_location = expected_output_var_locations[out_var_index];
    
    try {
        ASSERT_EQ(adapter.GetVarLocation(variable_name), expected_location);
    }
    catch (std::exception& e) {
        printf("Exception getting var location: %s", e.what());
    }
}

/** Test Q_OUT variable location can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetVarLocation_0_b) {
    int out_var_index = 5;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];
    std::string expected_location = expected_output_var_locations[out_var_index];
    
    try {
        ASSERT_EQ(adapter.GetVarLocation(variable_name), expected_location);
    }
    catch (std::exception& e) {
        printf("Exception getting var location: %s", e.what());
    }
}

/** Test NASH_LATERAL_RUNOFF variable units can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetVarUnits_0_a) {
    int out_var_index =3;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];
    std::string expected_units = expected_output_var_units[out_var_index];
    
    try {
        ASSERT_EQ(adapter.GetVarUnits(variable_name), expected_units);
    }
    catch (std::exception& e) {
        printf("Exception getting var units: %s", e.what());
    }
}

/** Test RAIN_RATE variable type can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetVarType_0_a) {
    int out_var_index =0;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];
    std::string expected_type = expected_output_var_types[out_var_index];
    
    try {
        ASSERT_EQ(adapter.GetVarType(variable_name), expected_type);
    }
    catch (std::exception& e) {
        printf("Exception getting var type: %s", e.what());
    }
}


/** Test SCHAKKE_OUTPUT_RUNOFF variable nbytes can be retrieved. */
TEST_F(Bmi_C_Adapter_Test, GetVarNbytes_0_a) {
    int out_var_index =1;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());
    
    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];

    try {
        ASSERT_EQ(adapter.GetVarNbytes(variable_name), expected_var_nbytes);
    }
    catch (std::exception& e) {
        printf("Exception getting var nbytes: %s", e.what());
    }
}
/** Test grid type can be retrieved. Based off NASH_LATERAL_RUNOFF grid id */
TEST_F(Bmi_C_Adapter_Test, GetGridType_0_a) {
    int out_var_index =3;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];
    int grd = adapter.GetVarGrid(variable_name);
    
    try {
        ASSERT_EQ(adapter.GetGridType(grd), expected_grid_type);
    }
    catch (std::exception& e) {
        printf("Exception getting grid type: %s", e.what());
    }
}

/** Test grid rank can be retrieved. Based off GUIH_RUNOFF grid id */
TEST_F(Bmi_C_Adapter_Test, GetGridRank_0_a) {
    int out_var_index =2;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];
    int grd = adapter.GetVarGrid(variable_name);
    
    try {
        ASSERT_EQ(adapter.GetGridRank(grd), expected_grid_rank);
    }
    catch (std::exception& e) {
        printf("Exception getting grid rank: %s", e.what());
    }
}

/** Test grid size can be retrieved. Based off Q_OUT grid id */
TEST_F(Bmi_C_Adapter_Test, GetGridSize_0_a) {
    int out_var_index =5;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetOutputVarNames()[out_var_index];
    int grd = adapter.GetVarGrid(variable_name);
    
    try {
        ASSERT_EQ(adapter.GetGridSize(grd), expected_grid_size);
    }
    catch (std::exception& e) {
        printf("Exception getting grid size: %s", e.what());
    }
}

/* Commenting out, since RAIN_RATE is not longer input variable; leaving in place to use as future template. */
/*
TEST_F(Bmi_C_Adapter_Test, GetValue_0_a_5) {
    int input_var_index = 1;

    Bmi_C_Adapter adapter(lib_file_name_0, config_file_name_0, forcing_file_name_0, true, false, true, utils::StreamHandler());

    std::string variable_name = adapter.GetInputVarNames()[input_var_index];

    adapter.Initialize();
    // Do the first few time steps
    for (int i = 0; i < 40; i++)
        adapter.Update();
    std::vector<double> values = adapter.GetValue<double>(variable_name);
    ASSERT_GT(values.size(), 0);
    adapter.Finalize();
}
 */

#endif  // NGEN_BMI_C_LIB_TESTS_ACTIVE