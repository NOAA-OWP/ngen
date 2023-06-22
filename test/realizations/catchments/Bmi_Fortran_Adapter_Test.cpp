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
#include "bmi_utilities.hpp"

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

    std::string expected_component_name = "Testing BMI Fortran Model";
    std::vector<std::string> expected_output_var_names = { "OUTPUT_VAR_1", "OUTPUT_VAR_2", "OUTPUT_VAR_3", "GRID_VAR_2", "GRID_VAR_3", "GRID_VAR_4" };
    std::vector<std::string> expected_input_var_names = { "INPUT_VAR_1", "INPUT_VAR_2", "INPUT_VAR_3", "GRID_VAR_1" };
    std::vector<std::string> expected_output_var_locations = { "node", "node", "node", "node", "node"};
    std::vector<int> expected_output_var_grids = { 0, 0, 0, 1, 2, 2 };
    std::vector<int> expected_input_var_grids = { 0, 0, 0, 1 };
    std::vector<std::string> expected_output_var_units = { "m", "m", "s", "m", "m", "m" };
    std::vector<std::string> expected_output_var_types = { "double precision", "real", "integer", "double precision", "double precision", "double precision" };
    std::vector<int> expected_output_var_item_sizes = { 8, 4, 4, 8, 8, 8};
    std::vector<std::string> expected_input_var_types = { "double precision", "real", "integer", "double precision" };
    std::vector<int> expected_input_var_item_sizes = { 8, 4, 4, 8 };
    std::vector<int> expected_input_grid_rank = {0, 0, 0, 2};
    std::vector<int> expected_output_grid_rank = {0, 0, 0, 2, 3, 3};
    std::vector<int> expected_input_grid_size = {1, 1, 1, 9};
    std::vector<int> expected_output_grid_size = {0, 0, 0, 9, 18, 18}; //FIXME correct 3D SIZE?
    std::vector<int> expected_input_var_nbytes = {8, 4, 4, 8}; //FIXME are these correct???
    std::vector<int> expected_output_var_nbytes = {8, 4, 4, 8, 8, 8}; //FIXME are these correct??? Probably not for the gridded ones...
    std::vector<int> expected_input_var_itemsize = {8, 4, 4, 8}; //FIXME are these correct???
    std::vector<int> expected_output_var_itemsize = {8, 4, 4, 8, 8, 8}; //FIXME are these correct???
    int expected_grid_shape = -1;
    double expected_grid_x = 0;
    double expected_grid_y = 0;
    double expected_grid_z = 0;
    double expected_grid_origin = -1;
    double expected_grid_spacing = -1;
    int expected_grid_node_count = 1;
    int expected_grid_edge_count = -1;
    int expected_grid_face_count = -1;

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

/** Simple test to make sure the model initializes. */
TEST_F(Bmi_Fortran_Adapter_Test, GetComponentName) {
    ASSERT_EQ( expected_component_name, adapter->GetComponentName() );
}

/** Test output variables item count can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetOutputItemCount_0_a) {
    try {
        ASSERT_EQ(adapter->GetOutputItemCount(), expected_output_var_names.size());
    }
    catch (std::exception& e) {
        printf("Exception getting output var count: %s", e.what());
        FAIL();
    }
}

/** Test output variables can be retrieved. 
 * 
 * Note  GetOutputItemCount() must be implemented for GetOutputVarNames to work.
 * 
*/
TEST_F(Bmi_Fortran_Adapter_Test, GetOutputVarNames_0_a) {
    try {
        ASSERT_EQ(adapter->GetOutputVarNames(), expected_output_var_names);
    }
    catch (std::exception& e) {
        printf("Exception getting output var names: %s", e.what());
        FAIL();
    }
}

/** Test input variables item count can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetInputItemCount_0_a) {
    try {
        ASSERT_EQ(adapter->GetInputItemCount(), expected_input_var_names.size());
    }
    catch (std::exception& e) {
        printf("Exception getting input var count: %s", e.what());
        FAIL();
    }
}

/** Test input variables can be retrieved. 
 * 
 * Note  GetInputItemCount() must be implemented for GetInputtVarNames to work.
 * 
*/
TEST_F(Bmi_Fortran_Adapter_Test, GetInputVarNames_0_a) {
    try {
        ASSERT_EQ(adapter->GetInputVarNames(), expected_input_var_names);
    }
    catch (std::exception& e) {
        printf("Exception getting input var names: %s", e.what());
        FAIL();
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
        FAIL();
    }
}

/** Test output 1 variable type can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarType_0_b) {
    int out_var_index = 1;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    std::string expected_type = expected_output_var_types[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarType(variable_name), expected_type);
    }
    catch (std::exception& e) {
        printf("Exception getting var type: %s", e.what());
        FAIL();
    }
}

/** Test output 1 variable type can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarType_0_c) {
    int out_var_index = 2;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    std::string expected_type = expected_output_var_types[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarType(variable_name), expected_type);
    }
    catch (std::exception& e) {
        printf("Exception getting var type: %s", e.what());
        FAIL();
    }
}

/** Test input 1 variable type can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarType_1_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetInputVarNames()[out_var_index];
    std::string expected_type = expected_input_var_types[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarType(variable_name), expected_type);
    }
    catch (std::exception& e) {
        printf("Exception getting var type: %s", e.what());
        FAIL();
    }
}

/** Test input 1 variable type can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarType_1_b) {
    int out_var_index = 1;

    std::string variable_name = adapter->GetInputVarNames()[out_var_index];
    std::string expected_type = expected_input_var_types[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarType(variable_name), expected_type);
    }
    catch (std::exception& e) {
        printf("Exception getting var type: %s", e.what());
        FAIL();
    }
}

/** Test input 1 variable type can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarType_1_c) {
    int out_var_index = 2;

    std::string variable_name = adapter->GetInputVarNames()[out_var_index];
    std::string expected_type = expected_input_var_types[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarType(variable_name), expected_type);
    }
    catch (std::exception& e) {
        printf("Exception getting var type: %s", e.what());
        FAIL();
    }
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
        FAIL();
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
        FAIL();
    }
}

/** Test input 1 variable grid (id) can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarGrid_1_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetInputVarNames()[out_var_index];
    int expected_grid = expected_input_var_grids[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarGrid(variable_name), expected_grid);
    }
    catch (std::exception& e) {
        printf("Exception getting var grid id: %s", e.what());
        FAIL();
    }
}

/** Test input 2 variable grid (id) can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarGrid_1_b) {
    int out_var_index = 1;

    std::string variable_name = adapter->GetInputVarNames()[out_var_index];
    int expected_grid = expected_input_var_grids[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarGrid(variable_name), expected_grid);
    }
    catch (std::exception& e) {
        printf("Exception getting var grid id: %s", e.what());
        FAIL();
    }
}

/** Test input 3 variable grid (id) can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarGrid_2_a) {
    int out_var_index = 2;

    std::string variable_name = adapter->GetInputVarNames()[out_var_index];
    int expected_grid = expected_input_var_grids[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarGrid(variable_name), expected_grid);
    }
    catch (std::exception& e) {
        printf("Exception getting var grid id: %s", e.what());
        FAIL();
    }
}

/** Test output 5 variable grid (id) can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarGrid_2_b) {
    int out_var_index = 4;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int expected_grid = expected_output_var_grids[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarGrid(variable_name), expected_grid);
    }
    catch (std::exception& e) {
        printf("Exception getting var grid id: %s", e.what());
        FAIL();
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
        FAIL();
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
        FAIL();
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
        FAIL();
    }
}

/** Test grid size can be retrieved for output 2. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridSize_0_a) {
    int out_var_index = 1;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);

    try {
        ASSERT_EQ(adapter->GetGridSize(grd), expected_output_grid_size[out_var_index]);
    }
    catch (std::exception& e) {
        printf("Exception getting grid size: %s", e.what());
        FAIL();
    }
}

/** Test grid size can be retrieved for input 4. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridSize_0_b) {
    int in_var_index = 3;

    std::string variable_name = adapter->GetInputVarNames()[in_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);
    std::vector<int> shape = {3,3};
    adapter->SetValue("grid_1_shape", shape.data());
    //Must be int to align with fortran c_int
    int grid_shape[rank+1];
    adapter->GetGridShape(grd, grid_shape);

    // FIXME embed the expected value directly in the test and remove from "expected_input_grid_size"...
    // since the shape is dictated in this test...
    try {
        ASSERT_EQ(adapter->GetGridSize(grd), expected_input_grid_size[in_var_index]);
    }
    catch (std::exception& e) {
        printf("Exception getting grid size: %s", e.what());
        FAIL();
    }
}

/** Test grid rank can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridRank_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);

    try {
        ASSERT_EQ(adapter->GetGridRank(grd), expected_output_grid_rank[out_var_index]);
    }
    catch (std::exception& e) {
        printf("Exception getting grid rank: %s", e.what());
        FAIL();
    }
}

/**
 * Test the function for getting the grid rank for the grid of grid output variable 2.
 * */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridRank_0_b) {
    int out_var_index = 3;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);

    try {
        ASSERT_EQ(adapter->GetGridRank(grd), expected_output_grid_rank[out_var_index]);
    }
    catch (std::exception& e) {
        printf("Exception getting grid rank: %s", e.what());
        FAIL();
    }
}

/** Test grid shape can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridShape_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 0);
    //Must be int to align with fortran c_int
    int shape[rank+1];
    adapter->GetGridShape(grd, shape);
    //shape not defined for scalars, so should be 0
    ASSERT_EQ(shape[0], 0);
}

/** Test grid shape can be retrieved for output 3. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridShape_0_b) {
    int out_var_index = 3;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);
    std::vector<int> shape = {3,4};
    adapter->SetValue("grid_1_shape", shape.data());
    //Must be int to align with fortran c_int
    int grid_shape[rank];
    adapter->GetGridShape(grd, grid_shape);
    ASSERT_EQ(shape[0], grid_shape[0]);
    ASSERT_EQ(shape[1], grid_shape[1]);
}

/** Test grid spacing can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridSpacing_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 0);
    double spacing[rank+1];
    adapter->GetGridSpacing(grd, spacing);
    //spacing undefined for scalar, so should be 0
    ASSERT_EQ(spacing[0], 0);
}

/** Test default grid spacing can be retrieved for output 4. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridSpacing_0_b) {
    int out_var_index = 3;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);
    double grid_spacing[rank];
    adapter->GetGridSpacing(grd, grid_spacing);
    //spacing not yet established for grid, so should be 0, 0
    ASSERT_EQ(grid_spacing[0], 0.0);
    ASSERT_EQ(grid_spacing[1], 0.0);
}

/** Test grid spacing can be set and retrieved for output 4. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridSpacing_0_c) {
    int out_var_index = 3;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);
    //NOTE spacing in BMI is again ij order, so this is `dy, dx`
    std::vector<double> spacing = {2.0, 2.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    adapter->SetValue("grid_1_spacing", spacing.data());
    double grid_spacing[rank];
    adapter->GetGridSpacing(grd, grid_spacing);
    ASSERT_EQ(grid_spacing[0], spacing[0]);
    ASSERT_EQ(grid_spacing[1], spacing[1]);
}

/** Test grid spacing units can be set and retrieved for output 4. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridSpacing_0_d) {
    int out_var_index = 3;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);
    //NOTE spacing in BMI is again ij order, so this is `dy, dx`
    std::vector<int> units= {1, 1}; //The enum value for `m` or `meters` in bmi_grid.f90 must be int type
    adapter->SetValue("grid_1_units", units.data());
    int grid_units[rank];
    adapter->GetValue("grid_1_units", grid_units);
    ASSERT_EQ(grid_units[0], units[0]);
    ASSERT_EQ(grid_units[1], units[1]);
}

/** Test grid origin can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridOrigin_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 0);
    double origin[rank+1];
    adapter->GetGridOrigin(grd, origin);
    //origin undefined for scalar, so should be 0
    ASSERT_EQ(origin[0], 0);
}

/** Test default grid origin can be retrieved for output 4. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridOrigin_0_b) {
    int out_var_index = 3;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);
    double origin[rank];
    adapter->GetGridOrigin(grd, origin);
    //origin not yet established for grid, should be 0,0
    ASSERT_EQ(origin[0], 0);
    ASSERT_EQ(origin[1], 0);
}

/** Test grid origin can be set and retrieved for output 4. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridOrigin_0_c) {
    int out_var_index = 3;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);
    //NOTE origin is also ij order, so this is `y0,x0`
    std::vector<double> _origin = {42.0, 42.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    adapter->SetValue("grid_1_origin", _origin.data());
    double origin[rank];
    adapter->GetGridOrigin(grd, origin);
    for(int i = 0; i < rank; i++){
        ASSERT_EQ(origin[i], _origin[i]);
    }
}

/** Test grid X can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridX_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 0);
    //single element vector to hold the result
    std::vector<double> xs = std::vector<double>(1, -1 );

    try {
        adapter->GetGridX(grd, xs.data());
        //The value passed in xs should be unchanged since GridX isn't defined for scalars
        ASSERT_EQ(xs[0], -1);
    }
    catch (std::exception& e) {
        printf("Exception getting grid X: %s", e.what());
        FAIL();
    }

}

/** Test grid X can be retrieved for output 4. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridX_0_b) {
    int out_var_index = 3;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);

    //Initialize the grid shape (y,x)
    std::vector<int> shape = {3,4};
    adapter->SetValue("grid_1_shape", shape.data());
    //Initialize the origin
    //NOTE origin is also ij order, so this is `y0,x0`
    std::vector<double> origin = {42.0, 42.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    adapter->SetValue("grid_1_origin", origin.data());
    
    //NOTE spacing in BMI is again ij order, so this is `dy, dx`
    std::vector<double> spacing = {2.0, 2.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    adapter->SetValue("grid_1_spacing", spacing.data());

    
    std::vector<double> xs = std::vector<double>(shape[1], 0.0 );

    int size = adapter->GetGridSize(grd);
  
    try {
        adapter->GetGridX(grd, xs.data());
            for(int i = 0; i < shape[1]; i++){
        ASSERT_EQ(xs[i], origin[1] + spacing[1]*i);
    }
    }
    catch (std::exception& e) {
        printf("Exception getting grid X: %s", e.what());
        FAIL();
    }

}

/** Test grid Y can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridY_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 0);
    //single element vector to hold the result
    std::vector<double> ys = std::vector<double>(1, -1 );

    try {
        adapter->GetGridY(grd, ys.data());
        //The value passed in ys should be unchanged since GridY isn't defined for scalars
        ASSERT_EQ(ys[0], -1);
    }
    catch (std::exception& e) {
        printf("Exception getting grid X: %s", e.what());
        FAIL();
    }

}

/** Test grid Y can be retrieved for output 4. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridY_0_b) {
       int out_var_index = 3;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);

    //Initialize the grid shape (y,x)
    std::vector<int> shape = {3,4};
    adapter->SetValue("grid_1_shape", shape.data());
    //Initialize the origin
    //NOTE origin is also ij order, so this is `y0,x0`
    std::vector<double> origin = {42.0, 42.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    adapter->SetValue("grid_1_origin", origin.data());
    
    //NOTE spacing in BMI is again ij order, so this is `dy, dx`
    std::vector<double> spacing = {2.0, 2.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    adapter->SetValue("grid_1_spacing", spacing.data());

    
    std::vector<double> ys = std::vector<double>(shape[0], 0.0 );

    int size = adapter->GetGridSize(grd);
  
    try {
        adapter->GetGridY(grd, ys.data());
            for(int i = 0; i < shape[0]; i++){
        ASSERT_EQ(ys[i], origin[0] + spacing[0]*i);
    }
    }
    catch (std::exception& e) {
        printf("Exception getting grid Y: %s", e.what());
        FAIL();
    }
}

/** Test grid Z can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridZ_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 0);
    //single element vector to hold the result
    std::vector<double> zs = std::vector<double>(1, -1 );

    try {
        adapter->GetGridZ(grd, zs.data());
        //The value passed in zs should be unchanged since GridZ isn't defined for scalars
        ASSERT_EQ(zs[0], -1);
    }
    catch (std::exception& e) {
        printf("Exception getting grid X: %s", e.what());
        FAIL();
    }

}

/** Test grid Z can be retrieved for output 5. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridZ_0_b) {
       int out_var_index = 4;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 3);

    //Initialize the grid shape (z,y,x)
    std::vector<int> shape = {3,4,5};
    adapter->SetValue("grid_2_shape", shape.data());
    //Initialize the origin
    //NOTE origin is also ij order, so this is `z0,y0,x0`
    std::vector<double> origin = {42.0, 42.0, 42.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    adapter->SetValue("grid_2_origin", origin.data());
    
    //NOTE spacing in BMI is again ij order, so this is `dz, dy, dx`
    std::vector<double> spacing = {2.0, 2.0, 2.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    adapter->SetValue("grid_2_spacing", spacing.data());

    
    std::vector<double> zs = std::vector<double>(shape[0], 0.0 );

    int size = adapter->GetGridSize(grd);
  
    try {
        adapter->GetGridZ(grd, zs.data());
            for(int i = 0; i < shape[0]; i++){
        ASSERT_EQ(zs[i], origin[0] + spacing[0]*i);
    }
    }
    catch (std::exception& e) {
        printf("Exception getting grid Z: %s", e.what());
        FAIL();
    }
}

/** Test grid node count can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridNodeCount_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    try {
        ASSERT_EQ(adapter->GetGridNodeCount(grd), expected_grid_node_count);
    }
    catch (std::exception& e) {
        printf("Exception getting grid node count: %s", e.what());
        FAIL();
    }
}

/** Test grid edge count can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridEdgeCount_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    EXPECT_THROW(adapter->GetGridEdgeCount(grd), std::runtime_error);
    // try {
    //     ASSERT_EQ(adapter->GetGridEdgeCount(grd), expected_grid_edge_count);
    // }
    // catch (std::exception& e) {
    //     printf("Exception getting grid edge count: %s", e.what());
    // }
}

/** Test grid face count can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridFaceCount_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    EXPECT_THROW(adapter->GetGridFaceCount(grd), std::runtime_error);
    // try {
    //     ASSERT_EQ(adapter->GetGridFaceCount(grd), expected_grid_face_count);
    // }
    // catch (std::exception& e) {
    //     printf("Exception getting grid face count: %s", e.what());
    // }
}

/** Test grid nodes per face can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridNodesPerFace_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int* nodes_per_face;
    EXPECT_THROW(adapter->GetGridNodesPerFace(grd, nodes_per_face), std::runtime_error);
    // int size = adapter->GetGridFaceCount(grd);
    // nodes_per_face = new int [size];
    // try {
    //     adapter->GetGridNodesPerFace(grd, edge_nodes);
    //     //TODO ASSERT what when working?
    // }
    // catch (std::exception& e) {
    //     printf("Exception getting grid nodes per face: %s", e.what());
    // }
    // delete [] nodes_per_face;
}

/** Test grid edge nodes can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridEdgeNodes_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int* edge_nodes;
    EXPECT_THROW(adapter->GetGridEdgeNodes(grd, edge_nodes), std::runtime_error);
    // int size = 2*adapter->GetGridEdgeCount(grd);
    // edge_nodes = new int [size];
    // try {
    //     adapter->GetGridEdgeNodes(grd, edge_nodes);
    //     //TODO ASSERT what when working?
    // }
    // catch (std::exception& e) {
    //     printf("Exception getting grid edge nodes: %s", e.what());
    // }
    // delete [] edge_nodes;
}

/** Test grid face edges can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridFaceEdges_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int* face_edges;
    EXPECT_THROW(adapter->GetGridFaceEdges(grd, face_edges), std::runtime_error);
    // int num_faces = adapter->GetGridFaceCount(grd);
    // int * nodes_per_face = new int [num_faces];
    // adapter->GetGridNodesPerFace(grd, nodes_per_face);
    // int num_face_nodes = 0;
    // for(int i = 0; i < num_faces; i++){
    //      num_face_nodes += nodes_per_face[i];
    //  }
    // 
    // face_edges = new int [num_face_nodes];
    // try {
    //     adapter->GetGridFaceEdges(grd, face_edges);
    //     //TODO ASSERT what when working?
    // }
    // catch (std::exception& e) {
    //     printf("Exception getting grid face edges: %s", e.what());
    // }
    // delete [] face_edges;
    // delete [] nodes_per_face
}

/** Test grid face nodes can be retrieved for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridFaceNodes_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);
    int* face_nodes;
    EXPECT_THROW(adapter->GetGridFaceNodes(grd, face_nodes), std::runtime_error);
    // int num_faces = adapter->GetGridFaceCount(grd);
    // int * nodes_per_face = new int [num_faces];
    // adapter->GetGridNodesPerFace(grd, nodes_per_face);
    // int num_face_nodes = 0;
    // for(int i = 0; i < num_faces; i++){
    //      num_face_nodes += nodes_per_face[i];
    //  }
    // 
    // face_nodes = new int [num_face_nodes];
    // try {
    //     adapter->GetGridFaceEdges(grd, face_nodes);
    //     //TODO ASSERT what when working?
    // }
    // catch (std::exception& e) {
    //     printf("Exception getting grid face nodes: %s", e.what());
    // }
    // delete [] face_nodes;
    // delete [] nodes_per_face;
}

/** Test grid type can be retrieved for output 1 */
TEST_F(Bmi_Fortran_Adapter_Test, GetGridType_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];
    int grd = adapter->GetVarGrid(variable_name);

    try {
        ASSERT_EQ(adapter->GetGridType(grd), "scalar");
    }
    catch (std::exception& e) {
        printf("Exception getting grid type: %s", e.what());
        FAIL();
    }
}

TEST_F(Bmi_Fortran_Adapter_Test, GetVarItemsize_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarItemsize(variable_name), expected_output_var_itemsize[out_var_index]);
    }
    catch (std::exception& e) {
        printf("Exception getting var nbytes: %s", e.what());
        FAIL();
    }
}

/** Test output 2 variable itemsize can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarItemsize_0_b) {
    int out_var_index = 1;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarItemsize(variable_name), expected_output_var_itemsize[out_var_index]);
    }
    catch (std::exception& e) {
        printf("Exception getting var itemsize: %s", e.what());
        FAIL();
    }
}

/** Test output 3 variable itemsize can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarItemsize_0_c) {
    int out_var_index = 2;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarItemsize(variable_name), expected_output_var_itemsize[out_var_index]);
    }
    catch (std::exception& e) {
        printf("Exception getting var itemsize: %s", e.what());
        FAIL();
    }
}

/** Test output 1 variable nbytes can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarNbytes_0_a) {
    int out_var_index = 0;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarNbytes(variable_name), expected_output_var_nbytes[out_var_index]);
    }
    catch (std::exception& e) {
        printf("Exception getting var nbytes: %s", e.what());
        FAIL();
    }
}

/** Test output 2 variable nbytes can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarNbytes_0_b) {
    int out_var_index = 1;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarNbytes(variable_name), expected_output_var_nbytes[out_var_index]);
    }
    catch (std::exception& e) {
        printf("Exception getting var nbytes: %s", e.what());
        FAIL();
    }
}

/** Test output 3 variable nbytes can be retrieved. */
TEST_F(Bmi_Fortran_Adapter_Test, GetVarNbytes_0_c) {
    int out_var_index = 2;

    std::string variable_name = adapter->GetOutputVarNames()[out_var_index];

    try {
        ASSERT_EQ(adapter->GetVarNbytes(variable_name), expected_output_var_nbytes[out_var_index]);
    }
    catch (std::exception& e) {
        printf("Exception getting var nbytes: %s", e.what());
        FAIL();
    }
}

/** Test that both the get value function works for input 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_0_a) {
    adapter->Initialize();
    double value = 5.0;
    adapter->SetValue("INPUT_VAR_1", &value);
    double retrieved = GetValue<double>(*adapter, "INPUT_VAR_1")[0];
    adapter->Finalize();
    ASSERT_EQ(value, retrieved);
}

/** Test that both the get value function works for input 2. */
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_0_b) {
    adapter->Initialize();
    float value = 6.0;
    adapter->SetValue("INPUT_VAR_2", &value);
    double retrieved = GetValue<float>(*adapter, "INPUT_VAR_2")[0];
    adapter->Finalize();
    ASSERT_EQ(value, retrieved);
}
/** Test that both the get value function works for input 3. */
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_0_c) {
    adapter->Initialize();
    int value = 7;
    adapter->SetValue("INPUT_VAR_3", &value);
    double retrieved = GetValue<int>(*adapter, "INPUT_VAR_3")[0];
    adapter->Finalize();
    ASSERT_EQ(value, retrieved);
}

/** Test that gridded data can be set for grid var 1*/
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_1_a) {
    adapter->Initialize();
    std::string variable_name = "GRID_VAR_1";
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);

    //Initialize the grid shape (y,x)
    std::vector<int> shape = {2,3};
    adapter->SetValue("grid_1_shape", shape.data());
    /*
    *    pass a 2 row, 3 column flattened data into the model
    *        1 2 3
    *        4 5 6
    *   flattened as 1 2 3 4 5 6 (row major)
    *   (column major would be 1 4 2 5 3 6)
    */
    std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    adapter->SetValue(variable_name, data.data());
    std::vector<double> retrieved = GetValue<double>(*adapter, variable_name);
    adapter->Finalize();
    ASSERT_EQ(data, retrieved);
}

/** Test that gridded data can be set and retrieved for grid var 3 in (x,y,z) order*/
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_1_b) {
    adapter->Initialize();
    std::string variable_name = "GRID_VAR_1";
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);
    //Initialize the output grid shape (z,y,x)
    std::vector<int> shape2 = {2,2,3};
    adapter->SetValue("grid_2_shape", shape2.data());
    //Initialize the grid shape (y,x)
    std::vector<int> shape = {2,3};
    adapter->SetValue("grid_1_shape", shape.data());
    /*
    *    pass a 2 row, 3 column flattened data into the model
    *        1 2 3
    *        4 5 6
    *   flattened as 1 2 3 4 5 6 (row major)
    */
    std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    adapter->SetValue(variable_name, data.data());

    /*
    *  Now, update the model one time step, which feeds this grid into grid var 3 and test the 
    *  output of that variable.
    * 
    *  The test model applies this 2D grid to the 3D grid, with +10 in the Z0 dimension
    *  and + 100 in the Z1 dimension, so the resulting grid should look like
    * 
    *  Z0 =      x0   x1  x2
    *       y0: [11   12  13]
    *       y1: [14   15  16]
    * 
    *  Z1 =      x0   x1  x2
    *       y0: [101   102  103]
    *       y1: [104   105  106]
    * 
    *  a row-major flattening of this looks like (last index changes fastest):
    * 
    *       assuming the index is (z,y,x):
    *           
    *           [ 11, 12, 13, 14, 15, 16, 101, 102, 103, 104, 105, 106 ]
    * 
    *       assuming the index is (z,x,y):
    * 
    *           [ 11, 14, 12, 15, 13, 16, 101, 104, 102, 105, 103, 106 ]
    * 
    *       assuming the index is (y,x,z):
    * 
    *           [ 11, 101, 12, 102, 13, 103, 14, 104, 15, 105, 16, 106 ]
    *  
    *       assuming the index is (y,z,x):
    * 
    *           [ 11, 12, 13, 101, 102, 103, 14, 15, 16, 104, 105, 106 ]
    *  
    *       assuming the index is (x,y,z):
    *  
    *           [ 11, 101, 14, 104, 12, 102, 15, 105, 13, 103, 16, 106 ]
    *  
    *       assuming the index is (x,z,y):
    *       
    *           [ 11, 14, 101, 104, 12, 15, 102, 105, 13, 16, 103, 106 ]
    *
    *  a column-major flattening of this looks like (first index changes fastest):
    *  
    *       assuming the index is (z,y,x):
    * 
    *           [ 11, 101, 14, 104, 12, 102, 15, 105, 13, 103, 16, 106 ]
    *  
    *       assuming the index is (z,x,y):
    *  
    *           [ 11, 101, 12, 102, 13, 103, 14, 104, 15, 105, 16, 106 ]
    *  
    *       assuming the index is (y,x,z):
    *  
    *           [ 11, 14, 12, 15, 13, 16, 101, 104, 102, 105, 103, 106 ]
    * 
    *       assuming the index is (y,z,x):
    *           
    *           [ 11, 14, 101, 104, 12, 15, 102, 105, 13, 16, 103, 106 ]
    *  
    *       assuming the index is (x,y,z):
    * 
    *           [ 11, 12, 13, 14, 15, 16, 101, 102, 103, 104, 105, 106 ]
    *  
    *       assuming the index is (x,z,y)
    *  
    *       [ 11, 12, 13, 101, 102, 103, 14, 15, 16, 104, 105, 106 ]
    * 
    *  Indexing the returned array in 3 dimensions using row-major conventions approriately would be (assuming y,x,z indexing)
    *  
    *  data[i][j][k] = flattened_data[ depth*(j+cols*i) + k ]
    *  
    *  where depth is the size of the 3rd dimension (z) and cols is the size of the second second dimension (x)
    *  i = row iterator (first dimension)
    *  j = column iterator (second dimension)
    *  k = depth iterator (thrid dimension)
    *  
    *  For example, using the grid described above, extracting the linear data into a y,x,z 3D array would look like this
    * 
    *  data[i][j][k] = flattened_data[ 2*(j+3*i) + k ]
    * 
    *  for( i = 0; i < 3; i++){
    *      for( j = 0; j < 2; j++){
    *          for( k = 0; k < 2; k++){
    *               data[i][j][k] = flattened_data[ depth*(j+cols*i) + k ]
    *               data[i][j][k] = flattened_data[  2*3*i + 2*j + k ]
    *           }   
    *      }
    *  }
    * 
    *  indexing with column major conventions would look like (assuming y,x,z indexing)
    *  
    *  data[i][j][k] = flattened_data[ rows*cols*k + rows*j + i ]
    * 
    *  where rows is the size of the first dimension, and the rest of the of the variables are as before.
    * 
    *  Using our example grid, this looks the following
    * 
    *  data[i][j][k] = flattened_data[ 2*3*k + 2*j + i ]
    * 
    *  for( i = 0; i < 3; i++){
    *      for( j = 0; j < 2; j++){
    *          for( k = 0; k < 2; k++){
    *               data[i][j][k] = flattened_data[ rows*cols*k + rows*j + i  ]
    *               data[i][j][k] = arr[  2*3*k + 2*j + i  ]
    *           }   
    *      }
    *  }  
    *   
    */
    adapter->Update();
    std::vector<double> retrieved = GetValue<double>(*adapter, "GRID_VAR_3");
    adapter->Finalize();
    std::vector<double> expected = {11, 12, 13, 14, 15, 16, 101, 102, 103, 104, 105, 106};

    int rows = shape2[1]; // 2
    int cols = shape2[2]; // 3
    int depth = shape2[1]; //2
    int i, j, k, r_idx, iter = 0;

    for( i = 0; i < rows; i++){ //rows (y)
        for( j = 0; j < cols; j++){ //cols (x) 
            for( k = 0; k < depth; k++){ //depth (z)
                r_idx =  depth*(j+cols*i) + k;
                ASSERT_EQ( retrieved[ r_idx ], expected[iter]);
                iter++;
            }
        }
    }
}

/** Test that gridded data can be set and retrieved for grid var 4 in (z,y,x) order*/
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_1_c) {
    adapter->Initialize();
    std::string variable_name = "GRID_VAR_1";
    int grd = adapter->GetVarGrid(variable_name);
    int rank = adapter->GetGridRank(grd);
    ASSERT_EQ(rank, 2);
    //Initialize the output grid shape (z,y,x)
    std::vector<int> shape2 = {2,2,3};
    adapter->SetValue("grid_2_shape", shape2.data());
    //Initialize the grid shape (y,x)
    std::vector<int> shape = {2,3};
    adapter->SetValue("grid_1_shape", shape.data());
    /*
    *    pass a 2 row, 3 column flattened data into the model
    *        1 2 3
    *        4 5 6
    *   flattened as 1 2 3 4 5 6 (row major)
    */
    std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    adapter->SetValue(variable_name, data.data());

    //Now, update the model one time step, which feeds this grid into grid var 3 and test the 
    //output of that variable

    adapter->Update();
    std::vector<double> retrieved = GetValue<double>(*adapter, "GRID_VAR_4");
    adapter->Finalize();
    std::vector<double> expected = {11, 101, 14, 104, 12, 102, 15, 105, 13, 103, 16, 106};

    int rows = shape2[1]; // 2
    int cols = shape2[2]; // 3
    int depth = shape2[1]; //2
    int i, j, k, r_idx, iter = 0;

    for( i = 0; i < rows; i++){ //rows (y)
        for( j = 0; j < cols; j++){ //cols (x) 
            for( k = 0; k < depth; k++){ //depth (z)
                r_idx =  depth*(j+cols*i) + k;
                ASSERT_EQ( retrieved[ r_idx ], expected[iter]);
                iter++;
            }
        }
    }
}

/** Test that both the set value function works for input 1. */
TEST_F(Bmi_Fortran_Adapter_Test, SetValue_0_a) {
    adapter->Initialize();
    double value = 5.0;
    adapter->SetValue("INPUT_VAR_1", &value);
    double retrieved = GetValue<double>(*adapter, "INPUT_VAR_1")[0];
    adapter->Finalize();
    ASSERT_EQ(value, retrieved);
}

/** Test that both the set value function works for input 2. */
TEST_F(Bmi_Fortran_Adapter_Test, SetValue_0_b) {
    adapter->Initialize();
    float value = 6.0;
    adapter->SetValue("INPUT_VAR_2", &value);
    double retrieved = GetValue<float>(*adapter, "INPUT_VAR_2")[0];
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
    double retrieved = GetValue<double>(*adapter, "INPUT_VAR_1")[0];
    adapter->Finalize();
    ASSERT_EQ(value_2, retrieved);
}

/** Test that getting the current model time works after initialization. */
TEST_F(Bmi_Fortran_Adapter_Test, GetCurrentTime_0_a) {
    adapter->Initialize();
    ASSERT_EQ(adapter->GetStartTime(), adapter->GetCurrentTime());
    adapter->Finalize();
}

/** Test the function for getting start time. */
TEST_F(Bmi_Fortran_Adapter_Test, GetStartTime_0_a) {
    adapter->Initialize();
    ASSERT_EQ(0, adapter->GetStartTime());
    adapter->Finalize();
}

/** Test the function for getting model end time, assuming (as in used config) 720 time steps. */
TEST_F(Bmi_Fortran_Adapter_Test, GetEndTime_0_a) {
    adapter->Initialize();
    double expected = adapter->GetStartTime() + (720.0 * adapter->GetTimeStep());
    ASSERT_EQ(expected, adapter->GetEndTime());
    adapter->Finalize();
}

/** Test the function for getting model time units. */
TEST_F(Bmi_Fortran_Adapter_Test, GetTimeUnits_0_a) {
    adapter->Initialize();
    ASSERT_EQ("s", adapter->GetTimeUnits());
    adapter->Finalize();
}

/** Test the function for getting time step size. */
TEST_F(Bmi_Fortran_Adapter_Test, GetTimeStep_0_a) {
    adapter->Initialize();
    ASSERT_EQ(3600, adapter->GetTimeStep());
    adapter->Finalize();
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
    ASSERT_EQ(value_1, GetValue<double>(*adapter, "OUTPUT_VAR_1")[0]);
    adapter->Finalize();
}

/** Test that the update function works for a single update and produces the expected value for output 2. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_0_c) {
    adapter->Initialize();
    double value_1 = 7.0;
    float value_2 = 10.0;
    adapter->SetValue("INPUT_VAR_1", &value_1);
    adapter->SetValue("INPUT_VAR_2", &value_2);
    adapter->Update();
    ASSERT_EQ(value_2 * 2, GetValue<float>(*adapter, "OUTPUT_VAR_2")[0]);
    adapter->Finalize();
}

/** Test that the update function works for the 720 time steps and gets the expected output 1 values. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_1_a) {
    size_t num_steps = 1;//720;
    std::vector<double> expected(num_steps);
    std::vector<double> out_1_vals(num_steps);
    adapter->Initialize();
    double value;
    float value2;
    for (size_t i = 0; i < num_steps; ++i) {
        value = 2.0 * i;
        value2 = 2.0 * i;
        expected[i] = value;
        adapter->SetValue("INPUT_VAR_1", &value);
        adapter->SetValue("INPUT_VAR_2", &value2);
        adapter->Update();
        out_1_vals[i] = GetValue<double>(*adapter, "OUTPUT_VAR_1")[0];
    }
    adapter->Finalize();
    ASSERT_EQ(expected, out_1_vals);
}

/** Test that the update function works for the 720 time steps and gets the expected output 2 values. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_1_b) {
    size_t num_steps = 1;//720;
    std::vector<float> expected(num_steps);
    std::vector<float> out_2_vals(num_steps);
    adapter->Initialize();
    double value;
    float value2;
    for (size_t i = 0; i < num_steps; ++i) {
        value = 2.0 * i;
        value2 = 2.0 * i;
        expected[i] = value * 2;
        adapter->SetValue("INPUT_VAR_1", &value);
        adapter->SetValue("INPUT_VAR_2", &value2);
        adapter->Update();
        out_2_vals[i] = GetValue<float>(*adapter, "OUTPUT_VAR_2")[0];
    }
    adapter->Finalize();
    ASSERT_EQ(expected, out_2_vals);
}

/** Test that the update_until function works for a single update and produces the expected value for output 2. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_until_0_a) {
    adapter->Initialize();
    double value_1 = 7.0;
    float value_2 = 10.0;
    adapter->SetValue("INPUT_VAR_1", &value_1);
    adapter->SetValue("INPUT_VAR_2", &value_2);
    // This basically is a step of 1.5 times the normal length
    double time = 5400 + adapter->GetCurrentTime();
    adapter->UpdateUntil(time);
    // Normally and update would produce out_2 as 2 * input_2, but here must further multiply by 1.5 for the longer time
    ASSERT_EQ(value_2 * 2.0 * 1.5, GetValue<float>(*adapter, "OUTPUT_VAR_2")[0]);
    adapter->Finalize();
}

/** Test that the update_until function works and advances the model time. */
TEST_F(Bmi_Fortran_Adapter_Test, Update_until_0_b) {
    adapter->Initialize();
    double value_1 = 7.0;
    float value_2 = 10.0;
    adapter->SetValue("INPUT_VAR_1", &value_1);
    adapter->SetValue("INPUT_VAR_2", &value_2);
    // This basically is a step of 1.5 times the normal length
    double time = 5400 + adapter->GetCurrentTime();
    adapter->UpdateUntil(time);
    ASSERT_EQ(time, adapter->GetCurrentTime());
    adapter->Finalize();
}

/* Tests  dependent on Update() */
/** Test that both the get value function works for output 1. */
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_0_d) {
    adapter->Initialize();
    double value1 = 7.0;
    float value2 = 8.0;
    double expectedOutput1 = 7.0;
    adapter->SetValue("INPUT_VAR_1", &value1);
    adapter->SetValue("INPUT_VAR_2", &value2);
    adapter->Update();
    double retrieved = GetValue<double>(*adapter, "OUTPUT_VAR_1")[0];
    adapter->Finalize();
    ASSERT_EQ(expectedOutput1, retrieved);
}

/** Test that both the get value function works for output 2. */
TEST_F(Bmi_Fortran_Adapter_Test, GetValue_0_e) {
    adapter->Initialize();
    double value1 = 7.0;
    float value2 = 8.0;
    double expectedOutput2 = 16.0; // TODO: fix this
    adapter->SetValue("INPUT_VAR_1", &value1);
    adapter->SetValue("INPUT_VAR_2", &value2);
    adapter->Update();
    double retrieved = GetValue<float>(*adapter, "OUTPUT_VAR_2")[0];
    adapter->Finalize();
    ASSERT_EQ(expectedOutput2, retrieved);
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
    //init the grid shapes to prevent errors getting gridded var outputs
    std::vector<int> shape = {2,3};
    adapter->SetValue("grid_1_shape", shape.data());
    //set some initial value for grid var 1
    double data = 0.0;
    adapter->SetValue("GRID_VAR_1", &data);
    shape = {2,2,3};
    adapter->SetValue("grid_2_shape", shape.data());
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
            std::vector<double> values = GetValue<double>(*adapter, name);
            auto e2 = std::chrono::steady_clock::now();
            saved_times["Get " + name].push_back(to_micros(s2,e2));
        }

        // record time to get each input variable
        for(std::string name : input_names)
        {
            auto s3 = std::chrono::steady_clock::now();
            std::vector<double> values = GetValue<double>(*adapter, name);
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

#endif  // NGEN_BMI_FORTRAN_LIB_TESTS_ACTIVE
