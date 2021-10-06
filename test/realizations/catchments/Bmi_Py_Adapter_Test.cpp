#ifdef NGEN_BMI_PY_TESTS_ACTIVE

#ifdef ACTIVATE_PYTHON

#include "gtest/gtest.h"
#include <vector>
#include <exception>

#include <pybind11/embed.h>
namespace py = pybind11;

#include "Bmi_Py_Adapter.hpp"

using namespace models::bmi;

typedef struct example_scenario {
    std::string bmi_init_config;
    // TODO: probably need to change to package name
    std::string module_directory;
    std::string forcing_file;
    std::string module_name;
    std::shared_ptr<Bmi_Py_Adapter> adapter;
} example_scenario;

class Bmi_Py_Adapter_Test : public ::testing::Test {

protected:

    // This is required for the Python interpreter and must be kept alive
    static std::shared_ptr<py::scoped_interpreter> guard_ptr;

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
    std::string py_dir_search(const std::vector<std::string> &dir_options);

    /**
     * Find the repo root directory, starting from the current directory and working upward.
     *
     * @return The absolute path of the repo root, as a string.
     */
    std::string py_find_repo_root();

    /**
     * Find the virtual environment site packages directory, starting from an assumed valid venv directory.
     *
     * @param venv_dir The virtual environment directory.
     * @return The absolute path of the site packages directory, as a string.
     */
    std::string py_find_venv_site_packages_dir(const std::string &venv_dir);

    std::vector<example_scenario> examples;

    py::object Path;

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

std::shared_ptr<py::scoped_interpreter> Bmi_Py_Adapter_Test::guard_ptr = nullptr;

void Bmi_Py_Adapter_Test::SetUp() {

    Path = py::module_::import("pathlib").attr("Path");

    std::string repo_root = py_find_repo_root();

    example_scenario template_ex_struct;
    // These should be safe for all examples
    template_ex_struct.module_name = "test_bmi_py.bmi_model";
    //template_ex_struct.module_name = "test_bmi_py";
    template_ex_struct.module_directory = repo_root + "/extern/";

    // Now generate the examples vector based on the above template example
    size_t num_example_scenarios = 1;
    examples = std::vector<example_scenario>(num_example_scenarios);
    for (size_t i = 0; i < num_example_scenarios; ++i) {
        examples[i] = template_ex_struct;
    }

    examples[0].forcing_file = repo_root + "/data/forcing/cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv";

    py::module_ sys = py::module_::import("sys");
    sys.attr("path").attr("insert")(0, template_ex_struct.module_directory);

    std::string venv_dir = py_dir_search({repo_root + "/.venv", repo_root + "/venv"});
    if (!venv_dir.empty()) {
        std::string venv_site_packages_dir = py_find_venv_site_packages_dir(venv_dir);
        sys.attr("path").attr("insert")(1, venv_site_packages_dir);
    }

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
    std::string teardown = "got_here";
}

void Bmi_Py_Adapter_Test::SetUpTestSuite() {
    // Start Python interpreter and keep it alive
    guard_ptr = std::make_shared<py::scoped_interpreter>();
}

void Bmi_Py_Adapter_Test::TearDownTestSuite() {
    guard_ptr.reset();
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

#endif // ACTIVATE_PYTHON

#endif // NGEN_BMI_PY_TESTS_ACTIVE
