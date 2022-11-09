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

    std::vector<std::string> expected_output_var_names = { "OUTPUT_VAR_1", "OUTPUT_VAR_2", "OUTPUT_VAR_3" };
    std::vector<std::string> expected_input_var_names = { "INPUT_VAR_1", "INPUT_VAR_2"};
    std::vector<std::string> expected_output_var_locations = { "node", "node", "node" };
    std::vector<int> expected_output_var_grids = { 0, 0, 0 };
    std::vector<std::string> expected_output_var_units = { "m", "m", "m" };
    std::vector<std::string> expected_input_var_types = { "float64", "float64" };
    std::vector<int> expected_input_var_item_sizes = { 8, 8 };
    std::vector<std::string> expected_output_var_types = { "float64", "float64", "float64" };
    std::vector<int> expected_output_var_item_sizes = { 8, 8, 8 };
    int expected_grid_rank = 1;
    int expected_grid_size = 1;
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
 * Test the function for getting the grid rank for the grid of output variable 1.
 * */
TEST_F(Bmi_Py_Adapter_Test, GetGridRank_0_a) {
    size_t ex_index = 0;

    std::string var_name = "OUTPUT_VAR_1";
    examples[ex_index].adapter->Initialize();
    int grid_id = examples[ex_index].adapter->GetVarGrid(var_name);
    int grid_rank = examples[ex_index].adapter->GetGridRank(grid_id);

    ASSERT_EQ(grid_rank, expected_grid_rank);
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

    ASSERT_EQ(grid_size, expected_grid_size);
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
    int grid_shape[10];
    // For now, expect an exception
    ASSERT_THROW(
        {
            try {
                examples[ex_index].adapter->GetGridShape(grid_id, grid_shape);
            }
            catch (const py::error_already_set &e) {
                std::string msg = e.what();
                std::string expected_msg_start = "NotImplementedError";
                ASSERT_TRUE(msg.rfind(expected_msg_start, 0) == 0);
                throw e;
            }
        },
        py::error_already_set);

    //ASSERT_EQ(grid_shape, expected_grid_size);
}

/**
 * Test the function for getting the grid spacing for the grid of output variable 1.
 * */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridSpacing_0_a) {
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
 * Test the function for getting the grid origin for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridOrigin_0_a) {
    // TODO: requires model support
    ASSERT_TRUE(false);
}

/**
 * Test the function for getting the location of grid nodes in the first dimension for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridX_0_a) {
    // TODO: requires model support
    ASSERT_TRUE(false);
}

/**
 * Test the function for getting the location of grid nodes in the second dimension for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridY_0_a) {
    // TODO: requires model support
    ASSERT_TRUE(false);
}

/**
 * Test the function for getting the location of grid nodes in the third dimension for the grid of output variable 1.
 */
TEST_F(Bmi_Py_Adapter_Test, DISABLED_GetGridZ_0_a) {
    // TODO: requires model support
    ASSERT_TRUE(false);
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
