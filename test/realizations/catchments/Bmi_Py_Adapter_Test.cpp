#ifdef NGEN_BMI_PY_TESTS_ACTIVE

#ifdef ACTIVATE_PYTHON

#include "gtest/gtest.h"
#include <vector>
#include <exception>
#include <cmath>

#include <pybind11/embed.h>
namespace py = pybind11;

#include "Bmi_Py_Adapter.hpp"

using namespace models::bmi;
using namespace utils::ngenPy;
using namespace pybind11::literals; // to bring in the `_a` literal for pybind11 keyword args functionality

typedef struct example_scenario {
    std::string bmi_init_config;
    // TODO: probably need to change to package name
    std::string module_directory;
    std::string forcing_file;
    std::string module_name;
    std::shared_ptr<Bmi_Py_Adapter> adapter;
} example_scenario;

class Bmi_Py_Adapter_Test : public ::testing::Test {
private:
    static std::shared_ptr<InterpreterUtil> interperter;
protected:

    static std::shared_ptr<py::object> friend_get_raw_model(Bmi_Py_Adapter *adapter) {
        return adapter->bmi_model;
    }

    void SetUp() override;

    void TearDown() override;

    /**
     * Do shared resource setup, as needed for the Python interpreter guard.
     *
     * This function allows for creation of things that span multiple test fixtures.  This is necessary for the guard
     * object that must remain for the embedded Python interpreter to function.
     */
    static void SetUpTestSuite();

    /**
     * Do shared resource teardown, as needed for the Python interpreter guard.
     *
     * This function allows for clean-up of things that span multiple test fixtures.  This is necessary for the guard
     * object that must remain for the embedded Python interpreter to function.
     */
    static void TearDownTestSuite();

    /**
     * Search for a file in the given directories, returning the path string of the first found, or empty string.
     *
     * Searches are done in the order of the parent directory options.
     *
     * @param parent_dir_options The possible parent directories to check for the desired file, in the order to search.
     * @param file_basename The basename of the desired file.
     * @return The path (as a string) of the first found existing file, or empty string of no existing file was found.
     */
    static std::string file_search(const std::vector<std::string> &parent_dir_options, const std::string& file_basename);

    /**
     * Search for a file in the given directories, returning path string of first found, or empty string.
     *
     * Searches are done by iterating through the basenames first.  I.e., for the current basename, iterate through all
     * parent directories, searching if they contain such file.  Return immediately if a match is found.  If none match,
     * move to the next basename.
     *
     * @param parent_dir_options The possible parent directories to check for a file, in the order to search.
     * @param file_basename The collection of possible basenames for a sought file.
     * @return The path (as a string) of the first found existing file, or empty string of no existing file was found.
     */
    static std::string file_search(const std::vector<std::string> &parent_dir_options,
                                   const std::vector<std::string> &file_basenames);

    /**
     * Search for and return the first existing example of a collection of directories, using Python to perform search.
     *
     * @param dir_options The options for the directories to check for existence, in the order to try them.
     * @return The path of the first found option which is an existing directory.
     */
    static std::string py_dir_search(const std::vector<std::string> &dir_options);

    /**
     * Find the repo root directory, starting from the current directory and working upward.
     *
     * @return The absolute path of the repo root, as a string.
     */
    static std::string py_find_repo_root();

    /**
     * Find the virtual environment site packages directory, starting from an assumed valid venv directory.
     *
     * @param venv_dir The virtual environment directory.
     * @return The absolute path of the site packages directory, as a string.
     */
    static std::string py_find_venv_site_packages_dir(const std::string &venv_dir);

    std::vector<example_scenario> examples;

    static py::object Path;

    std::vector<std::string> expected_output_var_names = { "OUTPUT_VAR_1", "OUTPUT_VAR_2", "OUTPUT_VAR_3", "GRID_VAR_2", "GRID_VAR_3" };
    std::vector<std::string> expected_input_var_names = { "INPUT_VAR_1", "INPUT_VAR_2", "GRID_VAR_1"};
    std::vector<std::string> expected_output_var_locations = { "node", "node", "node", "node" };
    std::vector<int> expected_output_var_grids = { 0, 0, 0, 1 };
    std::vector<std::string> expected_output_var_units = { "m", "m", "m", "m" };
    std::vector<std::string> expected_input_var_types = { "float64", "float64", "float64" };
    std::vector<int> expected_input_var_item_sizes = { 8, 8, 8 };
    std::vector<std::string> expected_output_var_types = { "float64", "float64", "float64", "float64" };
    std::vector<int> expected_output_var_item_sizes = { 8, 8, 8, 8 };
    std::vector<int> expected_input_grid_rank = {0, 0, 0, 2};
    std::vector<int> expected_output_grid_rank = {0, 0, 2};
    std::vector<int> expected_input_grid_size = {0, 0, 0, 9};
    std::vector<int> expected_output_grid_size = {0, 0, 9};
    std::string expected_grid_type = "scalar";
    int expected_var_nbytes = 8; //type double

};
//Make sure the interperter is instansiated and lives throught the test class
std::shared_ptr<InterpreterUtil> Bmi_Py_Adapter_Test::interperter = InterpreterUtil::getInstance();
py::object Bmi_Py_Adapter_Test::Path = InterpreterUtil::getPyModule(std::vector<std::string> {"pathlib", "Path"});

void Bmi_Py_Adapter_Test::SetUp() {

    std::string repo_root = py_find_repo_root();

    example_scenario template_ex_struct;
    // These should be safe for all examples
    template_ex_struct.module_name = "test_bmi_py.bmi_model";
    template_ex_struct.module_directory = repo_root + "/extern/";

    // Now generate the examples vector based on the above template example
    size_t num_example_scenarios = 1;
    examples = std::vector<example_scenario>(num_example_scenarios);
    for (size_t i = 0; i < num_example_scenarios; ++i) {
        examples[i] = template_ex_struct;
    }

    examples[0].forcing_file = repo_root + "/data/forcing/cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv";

    // We can handle setting the right init config and initializing the adapter in a loop
    for (int i = 0; i < examples.size(); ++i) {
        examples[i].bmi_init_config = repo_root + "/test/data/bmi/test_bmi_python/test_bmi_python_config_"
                + std::to_string(i) + ".yml";

        examples[i].adapter = std::make_shared<Bmi_Py_Adapter>(examples[i].module_name, examples[i].bmi_init_config,
                                                               examples[i].module_name, false, true,
                                                               utils::StreamHandler());
    }
}

void Bmi_Py_Adapter_Test::TearDown() {
}

void Bmi_Py_Adapter_Test::SetUpTestSuite() {
    std::string repo_root = py_find_repo_root();
    std::string module_directory = repo_root + "/extern/";

    // Add the package dir from a local virtual environment directory also, if there is one
    std::string venv_dir = py_dir_search({repo_root + "/.venv", repo_root + "/venv"});
    if (!venv_dir.empty()) {
        InterpreterUtil::addToPyPath(py_find_venv_site_packages_dir(venv_dir));
    }
    // Also add the extern dir with our test lib to Python system path
    InterpreterUtil::addToPyPath(module_directory);
}

void Bmi_Py_Adapter_Test::TearDownTestSuite() {

}

/**
 * Search for a file in the given directories, returning the path string of the first found, or empty string.
 *
 * Searches are done in the order of the parent directory options.
 *
 * @param parent_dir_options The possible parent directories to check for the desired file, in the order to search.
 * @param file_basename The basename of the desired file.
 * @return The path (as a string) of the first found existing file, or empty string of no existing file was found.
 */
std::string Bmi_Py_Adapter_Test::file_search(const vector<std::string> &parent_dir_options, const string &file_basename) {
    // Build vector of paths by building combinations of the path and basename options
    std::vector<std::string> path_possibilities(parent_dir_options.size());
    for (int i = 0; i < parent_dir_options.size(); ++i)
        path_possibilities[i] = parent_dir_options[i] + file_basename;
    return utils::FileChecker::find_first_readable(path_possibilities);
}

/**
 * Search for a file in the given directories, returning path string of first found, or empty string.
 *
 * Searches are done by iterating through the basenames first.  I.e., for the current basename, iterate through all
 * parent directories, searching if they contain such file.  Return immediately if a match is found.  If none match,
 * move to the next basename.
 *
 * @param parent_dir_options The possible parent directories to check for a file, in the order to search.
 * @param file_basename The collection of possible basenames for a sought file.
 * @return The path (as a string) of the first found existing file, or empty string of no existing file was found.
 */
std::string Bmi_Py_Adapter_Test::file_search(const vector<std::string> &parent_dir_options,
                                             const vector<std::string> &file_basenames) {
    std::string foundPathForBasename;
    for (const std::string &basename : file_basenames) {
        foundPathForBasename = file_search(parent_dir_options, basename);
        if (!foundPathForBasename.empty()) {
            return foundPathForBasename;
        }
    }
    return "";
}

/**
 * Search for and return the first existing example of a collection of directories, using Python to perform search.
 *
 * @param dir_options The options for the directories to check for existence, in the order to try them.
 * @return The path of the first found option which is an existing directory.
 */
std::string Bmi_Py_Adapter_Test::py_dir_search(const std::vector<std::string> &dir_options) {
    for (const std::string &dir : dir_options) {
        // Uses "Path", which is a class object, to init a Python pathlib.Path instance, retrieve its "is_dir" function,
        // and then call the function, which takes no arguments.
        if (py::bool_(Path(dir).attr("is_dir")()))
            return dir;
    }
    return "";
}

/**
 * Find the repo root directory, starting from the current directory and working upward.
 *
 * @return The absolute path of the repo root, as a string.
 */
std::string Bmi_Py_Adapter_Test::py_find_repo_root() {
    py::object dir = Path(".").attr("resolve")();
    while (!dir.equal(dir.attr("parent"))) {
        // If there is a child .git dir and a child .github dir, then dir is the root
        py::bool_ is_git_dir = py::bool_(dir.attr("joinpath")(".git").attr("is_dir")());
        py::bool_ is_github_dir = py::bool_(dir.attr("joinpath")(".github").attr("is_dir")());
        if (is_git_dir && is_github_dir) {
            return py::str(dir);
        }
        dir = dir.attr("parent");
    }
    throw std::runtime_error("Can't find repo root starting at " + std::string(py::str(Path(".").attr("resolve")())));
}

/**
 * Find the virtual environment site packages directory, starting from an assumed valid venv directory.
 *
 * @param venv_dir The virtual environment directory.
 * @return The absolute path of the site packages directory, as a string.
 */
std::string Bmi_Py_Adapter_Test::py_find_venv_site_packages_dir(const std::string &venv_dir) {
    py::object venv_dir_path = Path(venv_dir.c_str()).attr("resolve")();
    py::list site_packages_options = py::list(venv_dir_path.attr("glob")("**/site-packages"));
    for (auto &opt : site_packages_options) {
        return py::str(opt);
    }
    return "";
}

/**
 * Simple test to make sure the model from example 0 initializes.
 */
TEST_F(Bmi_Py_Adapter_Test, Initialize_0_a) {
    size_t ex_index = 0;

    examples[ex_index].adapter->Initialize();
    examples[ex_index].adapter->Finalize();
}

/**
 * Test output variables can be retrieved.
 */
TEST_F(Bmi_Py_Adapter_Test, GetOutputVarNames_0_a) {
    size_t ex_index = 0;
    
    try {
        ASSERT_EQ(examples[ex_index].adapter->GetOutputVarNames(), expected_output_var_names);
    }
    catch (std::exception& e) {
        printf("Exception getting output var names: %s", e.what());
    }
}

/**
 * Test output variables item count can be retrieved.
 */
TEST_F(Bmi_Py_Adapter_Test, GetOutputItemCount_0_a) {
    size_t ex_index = 0;

    try {
        ASSERT_EQ(examples[ex_index].adapter->GetOutputItemCount(), expected_output_var_names.size());
    }
    catch (std::exception& e) {
        printf("Exception getting output var count: %s", e.what());
    }
}

/**
 * Test output 1 variable type can be retrieved.
 */
TEST_F(Bmi_Py_Adapter_Test, GetVarType_0_a) {
    size_t ex_index = 0;
    size_t out_var_index = 0;

    examples[ex_index].adapter->Initialize();
    std::string variable_name = examples[ex_index].adapter->GetOutputVarNames()[out_var_index];
    std::string expected_type = expected_output_var_types[out_var_index];

    try {
        ASSERT_EQ(examples[ex_index].adapter->GetVarType(variable_name), expected_type);
    }
    catch (std::exception& e) {
        printf("Exception getting var type: %s", e.what());
        throw e;
    }
}

/**
 * Test input 1 variable type can be retrieved.
 */
TEST_F(Bmi_Py_Adapter_Test, GetVarType_0_b) {
    size_t ex_index = 0;
    size_t var_index = 0;

    examples[ex_index].adapter->Initialize();
    std::string variable_name = examples[ex_index].adapter->GetOutputVarNames()[var_index];
    std::string expected_type = expected_input_var_types[var_index];

    try {
        ASSERT_EQ(examples[ex_index].adapter->GetVarType(variable_name), expected_type);
    }
    catch (std::exception& e) {
        printf("Exception getting var type: %s", e.what());
        throw e;
    }
}

/**
 * Test output 1 variable item size can be retrieved.
 */
TEST_F(Bmi_Py_Adapter_Test, GetVarItemSize_0_a) {
    size_t ex_index = 0;
    size_t var_index = 0;

    examples[ex_index].adapter->Initialize();
    std::string variable_name = expected_output_var_names[var_index];
    int expected_size = expected_output_var_item_sizes[var_index];
    int actual_size = 0;

    try {
        actual_size = examples[ex_index].adapter->GetVarItemsize(variable_name);
        ASSERT_NE(0, actual_size);
        ASSERT_EQ(expected_size, actual_size);
    }
    catch (std::exception& e) {
        printf("Exception getting var type: %s", e.what());
        throw e;
    }
}

/**
 * Test input 1 variable item size can be retrieved.
 */
TEST_F(Bmi_Py_Adapter_Test, GetVarItemSize_0_b) {
    size_t ex_index = 0;
    size_t var_index = 0;

    examples[ex_index].adapter->Initialize();
    std::string variable_name = expected_input_var_names[var_index];
    int expected_size = expected_input_var_item_sizes[var_index];
    int actual_size = 0;

    try {
        actual_size = examples[ex_index].adapter->GetVarItemsize(variable_name);
        ASSERT_NE(0, actual_size);
        ASSERT_EQ(expected_size, actual_size);
    }
    catch (std::exception& e) {
        printf("Exception getting var type: %s", e.what());
        throw e;
    }
}

/**
 * Test that getting the number of bytes works for input variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, GetVarNbytes_0_a) {
    size_t ex_index = 0;

    examples[ex_index].adapter->Initialize();
    std::shared_ptr<py::object> raw_model = friend_get_raw_model(examples[ex_index].adapter.get());
    py::array value_array = raw_model->attr("get_value_ptr")("INPUT_VAR_1");

    int expected = (int)py::int_(value_array.nbytes());

    double observed = examples[ex_index].adapter->GetVarNbytes("INPUT_VAR_1");
    examples[ex_index].adapter->Finalize();
    ASSERT_EQ(observed, expected);
}

/**
 * Test that both the get value function works for input 1.
 */
TEST_F(Bmi_Py_Adapter_Test, GetValue_0_a) {
    size_t ex_index = 0;

    std::string var_name = "INPUT_VAR_1";

    examples[ex_index].adapter->Initialize();

    std::shared_ptr<py::object> raw_model = friend_get_raw_model(examples[ex_index].adapter.get());
    py::array_t<double> raw_var_values = raw_model->attr("get_value_ptr")(var_name.c_str());
    auto unchecked = raw_var_values.mutable_unchecked<1>();

    double initial_var_value = unchecked(0);

    double retrieved;
    examples[ex_index].adapter->GetValue(var_name, &retrieved);
    examples[ex_index].adapter->Finalize();
    ASSERT_EQ(initial_var_value, retrieved);
}

/**
 * Test that both the get value function works for input 1 after it is explicitly set.
 */
TEST_F(Bmi_Py_Adapter_Test, GetValue_0_b) {
    size_t ex_index = 0;

    std::string var_name = "INPUT_VAR_1";
    double value = 5.0;

    examples[ex_index].adapter->Initialize();

    std::shared_ptr<py::object> raw_model = friend_get_raw_model(examples[ex_index].adapter.get());
    py::array_t<double> raw_var_values = raw_model->attr("get_value_ptr")(var_name.c_str());
    auto unchecked = raw_var_values.mutable_unchecked<1>();

    double initial_var_value = unchecked(0);
    ASSERT_NE(initial_var_value, value);

    examples[ex_index].adapter->SetValue(var_name, &value);

    double retrieved;
    examples[ex_index].adapter->GetValue(var_name, &retrieved);
    examples[ex_index].adapter->Finalize();
    ASSERT_EQ(value, retrieved);
}

/**
 * Test that both the get value function works for input 2 after it is explicitly set.
 */
TEST_F(Bmi_Py_Adapter_Test, GetValue_0_c) {
    size_t ex_index = 0;

    std::string var_name = "INPUT_VAR_2";
    double value = 10.0;

    examples[ex_index].adapter->Initialize();

    std::shared_ptr<py::object> raw_model = friend_get_raw_model(examples[ex_index].adapter.get());
    py::array_t<double> raw_var_values = raw_model->attr("get_value_ptr")(var_name.c_str());
    auto unchecked = raw_var_values.mutable_unchecked<1>();

    double initial_var_value = unchecked(0);
    ASSERT_NE(initial_var_value, value);

    examples[ex_index].adapter->SetValue(var_name, &value);

    double retrieved;
    examples[ex_index].adapter->GetValue(var_name, &retrieved);
    examples[ex_index].adapter->Finalize();
    ASSERT_EQ(value, retrieved);
}

/**
 * Test that both the get value function works for output 2.
 */
TEST_F(Bmi_Py_Adapter_Test, GetValue_0_d) {
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_2";
    double value = 10.0;

    examples[ex_index].adapter->Initialize();

    std::shared_ptr<py::object> raw_model = friend_get_raw_model(examples[ex_index].adapter.get());
    py::array_t<double> raw_var_values = raw_model->attr("get_value_ptr")(var_name.c_str());
    auto unchecked = raw_var_values.mutable_unchecked<1>();

    double initial_var_value = unchecked(0);
    ASSERT_NE(initial_var_value, value);

    //NOTE this doesn't work if get_value_ptr returns a flattened array using `array.flatten()`...its a new copy/reference
    //See implementation in extern/test_bmi_py/bmi_model.py for using `ravel()` with a check that it won't make a copy
    unchecked(0) = value;

    double retrieved;
    examples[ex_index].adapter->GetValue(var_name, &retrieved);
    examples[ex_index].adapter->Finalize();
    ASSERT_EQ(value, retrieved);
}

/**
 * Test that both the get value pointer function works for input 1.
 */
TEST_F(Bmi_Py_Adapter_Test, GetValuePtr_0_a) {
    size_t ex_index = 0;

    std::string var_name = "INPUT_VAR_1";
    double value = 5.0;

    examples[ex_index].adapter->Initialize();
    double* ptr = (double*) examples[ex_index].adapter->GetValuePtr(var_name);
    ASSERT_NE(value, *ptr);

    examples[ex_index].adapter->SetValue(var_name, &value);
    double retrieved = *ptr;
    examples[ex_index].adapter->Finalize();

    ASSERT_EQ(value, retrieved);
}

/**
 * Test the function for getting values for a particular set of indices of output variable 3.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetValueAtIndices_0_a) {
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_3";
    double retrieved[2];
    double expected[2] = {0, 2};
    int indexes[] = {0, 2};

    examples[ex_index].adapter->Initialize();

    examples[ex_index].adapter->GetValueAtIndices(var_name, retrieved, indexes, 2);

    for (int i = 0; i < 2; ++i)
        ASSERT_EQ(retrieved[i], expected[i]);
}

/**
 * Test the function for getting values for a particular set of indices of output variable 3 after an update.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetValueAtIndices_0_b) {
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_3";
    double retrieved[2];
    double expected[2] = {1, 3};
    int indexes[] = {0, 2};

    examples[ex_index].adapter->Initialize();

    // Shouldn't need to bother with other vars for this, since this var is time-step-dependent only
    examples[ex_index].adapter->Update();

    examples[ex_index].adapter->GetValueAtIndices(var_name, retrieved, indexes, 2);

    for (int i = 0; i < 2; ++i)
        ASSERT_EQ(retrieved[i], expected[i]);
}

/**
 * Test the function for setting values for a particular set of indices.
 * */
TEST_F(Bmi_Py_Adapter_Test, SetValueAtIndices_0_a) {
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_3";
    double retrieved[2];
    double expected[2] = {1, 3};
    int indexes[] = {0, 2};

    examples[ex_index].adapter->Initialize();

    // Shouldn't need to bother with other vars for this, since this var is time-step-dependent only
    examples[ex_index].adapter->Update();

    examples[ex_index].adapter->GetValueAtIndices(var_name, retrieved, indexes, 2);

    for (int i = 0; i < 2; ++i)
        ASSERT_EQ(retrieved[i], expected[i]);

    double newVals[] = {7, 12};

    examples[ex_index].adapter->SetValueAtIndices(var_name, indexes, 2, newVals);

    examples[ex_index].adapter->GetValueAtIndices(var_name, retrieved, indexes, 2);
    for (int i = 0; i < 2; ++i)
        ASSERT_EQ(retrieved[i], newVals[i]);

    // Still shouldn't need to bother with other vars for this, since this var is time-step-dependent only
    examples[ex_index].adapter->Update();
    examples[ex_index].adapter->GetValueAtIndices(var_name, retrieved, indexes, 2);
    for (int i = 0; i < 2; ++i)
        ASSERT_EQ(retrieved[i], newVals[i]+1);

    double skipped;
    int singleItem = 1;
    examples[ex_index].adapter->GetValueAtIndices(var_name, &skipped, &singleItem, 1);
    ASSERT_EQ(skipped, 3);
}

/**
 * Test the function for getting start time.
 */
TEST_F(Bmi_Py_Adapter_Test, GetStartTime_0_a) {
    size_t ex_index = 0;

    examples[ex_index].adapter->Initialize();
    int time = examples[ex_index].adapter->GetStartTime();
    examples[ex_index].adapter->Finalize();
    ASSERT_EQ(0, time);
}

/**
 * Test the function for getting time step size.
 */
TEST_F(Bmi_Py_Adapter_Test, GetTimeStep_0_a) {
    size_t ex_index = 0;

    examples[ex_index].adapter->Initialize();
    int time_step = examples[ex_index].adapter->GetTimeStep();
    examples[ex_index].adapter->Finalize();
    ASSERT_EQ(3600, time_step);
}

/**
 * Test the function for getting model time units.
 */
TEST_F(Bmi_Py_Adapter_Test, GetTimeUnits_0_a) {
    size_t ex_index = 0;

    examples[ex_index].adapter->Initialize();
    std::string time_units = examples[ex_index].adapter->GetTimeUnits();
    examples[ex_index].adapter->Finalize();
    ASSERT_EQ("seconds", time_units);
}

/**
 * Test the function for getting model end time, assuming (as in used config) 720 time steps.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetEndTime_0_a) {
    size_t ex_index = 0;

    examples[ex_index].adapter->Initialize();
    py::object np = py::module_::import("numpy");


    double max_float = py::float_(np.attr("finfo")(py::float_(0.0)).attr("max"));

    //double expected = examples[ex_index].adapter->GetStartTime() + (720 * examples[ex_index].adapter->GetTimeStep());
    double end_time = examples[ex_index].adapter->GetEndTime();
    examples[ex_index].adapter->Finalize();
    ASSERT_EQ(max_float, end_time);
}

/**
 * Test that getting the current model time works after initialization.
 */
TEST_F(Bmi_Py_Adapter_Test, GetCurrentTime_0_a) {
    size_t ex_index = 0;

    examples[ex_index].adapter->Initialize();
    int start_time = examples[ex_index].adapter->GetStartTime();
    int current_time = examples[ex_index].adapter->GetCurrentTime();
    examples[ex_index].adapter->Finalize();
    ASSERT_EQ(start_time, current_time);
}

/**
 * Test that getting the current model time works after an update.
 */
TEST_F(Bmi_Py_Adapter_Test, GetCurrentTime_0_b) {
    size_t ex_index = 0;

    examples[ex_index].adapter->Initialize();
    int expected_time = examples[ex_index].adapter->GetStartTime() + examples[ex_index].adapter->GetTimeStep();
    double value = 10.0;
    examples[ex_index].adapter->SetValue("INPUT_VAR_1", &value);
    examples[ex_index].adapter->SetValue("INPUT_VAR_2", &value);
    examples[ex_index].adapter->Update();
    int current_time = examples[ex_index].adapter->GetCurrentTime();
    examples[ex_index].adapter->Finalize();
    ASSERT_EQ(expected_time, current_time);
}

/**
 * Test that the set value function works for input 1.
 */
TEST_F(Bmi_Py_Adapter_Test, SetValue_0_a) {
    size_t ex_index = 0;

    std::string var_name = "INPUT_VAR_1";
    double value = 5.0;


    examples[ex_index].adapter->Initialize();

    std::shared_ptr<py::object> raw_model = friend_get_raw_model(examples[ex_index].adapter.get());
    py::array_t<double> raw_var_values = raw_model->attr("get_value_ptr")(var_name.c_str());
    auto unchecked = raw_var_values.mutable_unchecked<1>();

    double initial_var_value = unchecked(0);

    ASSERT_NE(value, initial_var_value);

    examples[ex_index].adapter->SetValue(var_name, &value);

    //double actual_stored_value = *((double*) raw_var_values.request(false).ptr);
    //This only works if the BMI model `set_value` implementation is guaranteed to not assign a new reference...
    double actual_stored_value = unchecked(0);

    ASSERT_EQ(value, actual_stored_value);
}

/**
 * Test that the set value function works for input 2.
 */
TEST_F(Bmi_Py_Adapter_Test, SetValue_0_b) {
    size_t ex_index = 0;

    std::string var_name = "INPUT_VAR_2";
    double value = 7.0;


    examples[ex_index].adapter->Initialize();

    std::shared_ptr<py::object> raw_model = friend_get_raw_model(examples[ex_index].adapter.get());
    py::array_t<double> raw_var_values = raw_model->attr("get_value_ptr")(var_name.c_str());
    auto unchecked = raw_var_values.mutable_unchecked<1>();

    double initial_var_value = unchecked(0);

    ASSERT_NE(value, initial_var_value);

    examples[ex_index].adapter->SetValue(var_name, &value);

    //double actual_stored_value = *((double*) raw_var_values.request(false).ptr);
    double actual_stored_value = unchecked(0);

    ASSERT_EQ(value, actual_stored_value);
}

/**
 * Test that the set value function works for grid input 1 using pointer data.
 */
TEST_F(Bmi_Py_Adapter_Test, SetValue_0_c) {
    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_1";
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};


    examples[ex_index].adapter->Initialize();
    //Initialize the grid shape
    std::vector<int> shape = {2,3};
    examples[ex_index].adapter->SetValue("grid_1_shape", shape.data());
    //Get the initial values
    std::shared_ptr<py::object> raw_model = friend_get_raw_model(examples[ex_index].adapter.get());
    py::array_t<double> raw_var_values = raw_model->attr("get_value_ptr")(var_name.c_str());
    auto unchecked = raw_var_values.mutable_unchecked<1>();
    
    std::vector<double> initial_values(values.size());
    for(int i = 0; i < initial_values.size(); i++){
        initial_values[i] = unchecked(i);
    }

    EXPECT_NE(values, initial_values);

    examples[ex_index].adapter->SetValue(var_name, values.data());

    //double actual_stored_value = *((double*) raw_var_values.request(false).ptr);
    std::vector<double> actual_stored_values(values.size());
    for(int i = 0; i < actual_stored_values.size(); i++){
        actual_stored_values[i] = unchecked(i);
    }

    EXPECT_EQ(values, actual_stored_values);
}

/**
 * Test that the set value function works for grid input 1 using vector data.
 */
TEST_F(Bmi_Py_Adapter_Test, SetValue_0_d) {
    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_1";
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};


    examples[ex_index].adapter->Initialize();
    //Initialize the grid shape
    std::vector<int> shape = {2,3};
    examples[ex_index].adapter->SetValue("grid_1_shape", shape.data());
    //Get the initial values
    std::shared_ptr<py::object> raw_model = friend_get_raw_model(examples[ex_index].adapter.get());
    py::array_t<double> raw_var_values = raw_model->attr("get_value_ptr")(var_name.c_str());
    auto unchecked = raw_var_values.mutable_unchecked<1>();
    
    std::vector<double> initial_values(values.size());
    for(int i = 0; i < initial_values.size(); i++){
        initial_values[i] = unchecked(i);
    }

    EXPECT_NE(values, initial_values);

    examples[ex_index].adapter->set_value(var_name, values);

    //double actual_stored_value = *((double*) raw_var_values.request(false).ptr);
    std::vector<double> actual_stored_values(values.size());
    for(int i = 0; i < actual_stored_values.size(); i++){
        actual_stored_values[i] = unchecked(i);
    }

    EXPECT_EQ(values, actual_stored_values);
}

/**
 * Test the function for getting the grid for output variable 1.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetVarGrid_0_a) {
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid = examples[ex_index].adapter->GetVarGrid(var_name);

    ASSERT_EQ(grid, expected_output_var_grids[0]);
}

/**
 * Test the function for getting the grid for grid variable 1.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetVarGrid_0_b) {
    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid = examples[ex_index].adapter->GetVarGrid(var_name);

    ASSERT_EQ(grid, expected_output_var_grids[3]);
}

/**
 * Test the function for getting the grid rank for the grid of output variable 1.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridRank_0_a) {
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int grid_rank = examples[ex_index].adapter->GetGridRank(grid_id);

    ASSERT_EQ(grid_rank, expected_output_grid_rank[0]);
}

/**
 * Test the function for getting the grid rank for the grid of grid output variable 2.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridRank_0_b) {
    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_2";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int grid_rank = examples[ex_index].adapter->GetGridRank(grid_id);

    ASSERT_EQ(grid_rank, expected_output_grid_rank[2]);
}

/**
 * Test the function for getting the grid size for the grid of output variable 1.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridSize_0_a) {
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int grid_size = examples[ex_index].adapter->GetGridSize(grid_id);

    ASSERT_EQ(grid_size, expected_output_grid_size[0]);
}

/**
 * Test the function for getting the grid size for the grid of grid variable 2.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridSize_0_b) {
    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_2";
    examples[ex_index].adapter->Initialize();
    std::vector<int> shape = {3,4};
    examples[ex_index].adapter->SetValue("grid_1_shape", shape.data());
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int grid_size = examples[ex_index].adapter->GetGridSize(grid_id);
    
    ASSERT_EQ(grid_size, shape[0]*shape[1]);
}

/**
 * Test the function for getting the grid shape for the grid of output variable 1.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridShape_0_a) {
    // TODO: requires model support

    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);

    ASSERT_EQ(rank, 0);
    int grid_shape[rank+1];

    examples[ex_index].adapter->GetGridShape(grid_id, grid_shape);
    ASSERT_EQ(grid_shape[0], 0);
}

/**
 * Test the function for getting the grid shape for the grid of grid variable 2.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridShape_0_b) {
    // TODO: requires model support

    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_2";
    examples[ex_index].adapter->Initialize();
    //NOTE shape for BMI is number of nodes in ij (column-major) order, so this would create a grid of 
    // 3x2 cells/faces (2 rows, 3 columns) defined by the corner coordinates
    std::vector<int> shape = {3,4};
    examples[ex_index].adapter->SetValue("grid_1_shape", shape.data());
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);

    ASSERT_EQ(rank, 2);
    int grid_shape[rank];

    examples[ex_index].adapter->GetGridShape(grid_id, grid_shape);
    ASSERT_EQ(grid_shape[0], shape[0]);
    ASSERT_EQ(grid_shape[1], shape[1]);
}

/**
 * Test the function for getting the grid spacing for the grid of output variable 1.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridSpacing_0_a) {

    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);

    ASSERT_EQ(rank, 0);
    double grid_spacing[rank+1];

    examples[ex_index].adapter->GetGridSpacing(grid_id, grid_spacing);
    ASSERT_EQ(grid_spacing[0], 0);
}

/**
 * Test the function for getting the default grid spacing for grid var 1.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridSpacing_0_b) {

    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);

    ASSERT_EQ(rank, 2);
    double grid_spacing[rank];

    examples[ex_index].adapter->GetGridSpacing(grid_id, grid_spacing);
    for(int i = 0; i < rank; i++){
        ASSERT_EQ(grid_spacing[i], 0);
    }
}

/**
 * Test the function for setting then getting the grid spacing for grid var 1.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridSpacing_0_c) {

    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);
    //NOTE spacing in BMI is again ij order, so this is `dy, dx`
    std::vector<double> spacing = {2.0, 2.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    examples[ex_index].adapter->SetValue("grid_1_spacing", spacing.data());
    
    ASSERT_EQ(rank, 2);
    double grid_spacing[rank];

    examples[ex_index].adapter->GetGridSpacing(grid_id, grid_spacing);
    for(int i = 0; i < rank; i++){
        ASSERT_EQ(grid_spacing[i], 2.0);
    }
}

/**
 * Test the function for setting then getting the grid spacing units for grid var 1.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridSpacing_0_d) {

    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);
    //NOTE spacing in BMI is again ij order, so this is `dy, dx`
    std::vector<short> units = {1,1}; //The enum value for `m` or `meters` in bmi_grid.py
    examples[ex_index].adapter->SetValue("grid_1_units", units.data());

    short grid_units[rank];
    examples[ex_index].adapter->GetValue("grid_1_units", grid_units);
    for(int i = 0; i < rank; i++){
        ASSERT_EQ(grid_units[i], units[i]);
    }
}

/**
 * Test the function for getting the grid origin for the grid of grid var 2.
 */
TEST_F(Bmi_Py_Adapter_Test, GetGridOrigin_0_a) {
    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_2";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);

    ASSERT_EQ(rank, 2);
    double grid_origin[rank];

    examples[ex_index].adapter->GetGridSpacing(grid_id, grid_origin);
    ASSERT_EQ(grid_origin[0], 0);
    ASSERT_EQ(grid_origin[1], 0);
}

/**
 * Test the function for getting the default grid origin for grid var 2.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridOrigin_0_b) {

    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_2";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);

    ASSERT_EQ(rank, 2);
    double grid_origin[rank];

    examples[ex_index].adapter->GetGridOrigin(grid_id, grid_origin);
    for(int i = 0; i < rank; i++){
        ASSERT_EQ(grid_origin[i], 0);
    }
}

/**
 * Test the function for setting then getting the grid origin for grid var 2.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridOrigin_0_c) {

    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_2";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);
    //NOTE origin is also ij order, so this is `y0,x0`
    std::vector<double> origin = {42.0, 42.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    examples[ex_index].adapter->SetValue("grid_1_origin", origin.data());
    
    ASSERT_EQ(rank, 2);
    double grid_origin[rank];

    examples[ex_index].adapter->GetGridOrigin(grid_id, grid_origin);
    for(int i = 0; i < rank; i++){
        ASSERT_EQ(grid_origin[i], 42.0);
    }
}

/**
 * Test the function for getting the location of grid nodes in the first dimension for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, GetGridX_0_a) {

    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_2";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);
    //Initialize the grid shape (y,x)
    std::vector<int> shape = {3,4};
    examples[ex_index].adapter->SetValue("grid_1_shape", shape.data());
    //Initialize the origin
    //NOTE origin is also ij order, so this is `y0,x0`
    std::vector<double> origin = {42.0, 42.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    examples[ex_index].adapter->SetValue("grid_1_origin", origin.data());
    
    //NOTE spacing in BMI is again ij order, so this is `dy, dx`
    std::vector<double> spacing = {2.0, 2.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    examples[ex_index].adapter->SetValue("grid_1_spacing", spacing.data());

    ASSERT_EQ(rank, 2);
    std::vector<double> xs = std::vector<double>(shape[1], 0.0 );
    examples[ex_index].adapter->GetGridX(grid_id, xs.data());

    for(int i = 0; i < shape[1]; i++){
        ASSERT_EQ(xs[i], origin[1] + spacing[1]*i);
    }
}

/**
 * Test the function for getting the location of grid nodes in the second dimension for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, GetGridY_0_a) {
    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_2";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);
    //Initialize the grid shape (y,x)
    std::vector<int> shape = {3,4};
    examples[ex_index].adapter->SetValue("grid_1_shape", shape.data());
    //Initialize the origin
    //NOTE origin is also ij order, so this is `y0,x0`
    std::vector<double> origin = {42.0, 42.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    examples[ex_index].adapter->SetValue("grid_1_origin", origin.data());
    
    //NOTE spacing in BMI is again ij order, so this is `dy, dx`
    std::vector<double> spacing = {2.0, 2.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    examples[ex_index].adapter->SetValue("grid_1_spacing", spacing.data());

    ASSERT_EQ(rank, 2);
    std::vector<double> ys = std::vector<double>(shape[0], 0.0 );
    examples[ex_index].adapter->GetGridY(grid_id, ys.data());

    for(int i = 0; i < shape[0]; i++){
        ASSERT_EQ(ys[i], origin[0] + spacing[0]*i);
    }
}

/**
 * Test the function for getting the location of grid nodes in the third dimension for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, GetGridZ_0_a) {
    size_t ex_index = 0;

    std::string var_name = "GRID_VAR_3";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int rank = examples[ex_index].adapter->GetGridRank(grid_id);
    //Initialize the grid shape (z,y,x)
    std::vector<int> shape = {3,4,5};
    examples[ex_index].adapter->SetValue("grid_2_shape", shape.data());
    //Initialize the origin
    //NOTE origin is also ij order, so this is `z0, y0,x0`
    std::vector<double> origin = {42.0, 42.0, 42.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    examples[ex_index].adapter->SetValue("grid_2_origin", origin.data());
    
    //NOTE spacing in BMI is again ij order, so this is `dz, dy, dx`
    std::vector<double> spacing = {2.0, 2.0, 2.0}; //TODO if this isn't double, the results are wacky...but it doesn't crash...
    examples[ex_index].adapter->SetValue("grid_2_spacing", spacing.data());

    ASSERT_EQ(rank, 3);
    std::vector<double> zs = std::vector<double>(shape[0], 0.0 );

    examples[ex_index].adapter->GetGridZ(grid_id, zs.data());

    for(int i = 0; i < shape[0]; i++){
        ASSERT_EQ(zs[i], origin[0] + spacing[0]*i);
    }
}

/**
 * Test the function for getting the grid node count for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridNodeCount_0_a) {
    // TODO: requires model support
    /*
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int *grid_spacing = examples[ex_index].adapter->GetGridSpacing(grid_id);

    ASSERT_EQ(grid_size, expected_grid_size);
    */
    ASSERT_TRUE(false);
}

/**
 * Test the function for getting the grid edge count for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridEdgeCount_0_a) {
    // TODO: requires model support
    /*
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int *grid_spacing = examples[ex_index].adapter->GetGridSpacing(grid_id);

    ASSERT_EQ(grid_size, expected_grid_size);
    */
    ASSERT_TRUE(false);
}

/**
 * Test the function for getting the grid face count for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridFaceCount_0_a) {
    // TODO: requires model support
    /*
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int *grid_spacing = examples[ex_index].adapter->GetGridSpacing(grid_id);

    ASSERT_EQ(grid_size, expected_grid_size);
    */
    ASSERT_TRUE(false);
}

/**
 * Test the function for getting the grid edge nodes for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridEdgeNodes_0_a) {
    // TODO: requires model support
    /*
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int *grid_spacing = examples[ex_index].adapter->GetGridSpacing(grid_id);

    ASSERT_EQ(grid_size, expected_grid_size);
    */
    ASSERT_TRUE(false);
}

/**
 * Test the function for getting the grid face edges for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridFaceEdges_0_a) {
    // TODO: requires model support
    /*
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int *grid_spacing = examples[ex_index].adapter->GetGridSpacing(grid_id);

    ASSERT_EQ(grid_size, expected_grid_size);
    */
    ASSERT_TRUE(false);
}

/**
 * Test the function for getting the grid face nodes for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridFaceNodes_0_a) {
    // TODO: requires model support
    /*
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int *grid_spacing = examples[ex_index].adapter->GetGridSpacing(grid_id);

    ASSERT_EQ(grid_size, expected_grid_size);
    */
    ASSERT_TRUE(false);
}

/**
 * Test the function for getting the grid nodes per face for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridNodesPerFace_0_a) {
    // TODO: requires model support
    /*
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int *grid_spacing = examples[ex_index].adapter->GetGridSpacing(grid_id);

    ASSERT_EQ(grid_size, expected_grid_size);
    */
    ASSERT_TRUE(false);
}

#endif // ACTIVATE_PYTHON

#endif // NGEN_BMI_PY_TESTS_ACTIVE
