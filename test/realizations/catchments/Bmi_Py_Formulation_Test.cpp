#ifdef NGEN_BMI_PY_TESTS_ACTIVE

#ifdef ACTIVATE_PYTHON

#include "gtest/gtest.h"
#include <boost/date_time.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <exception>
#include <pybind11/embed.h>
#include <regex>
#include <vector>
#include "Bmi_Py_Formulation.hpp"
#include "python/InterpreterUtil.hpp"
#include <CsvPerFeatureForcingProvider.hpp>

namespace py = pybind11;
using namespace pybind11::literals; // to bring in the `_a` literal for pybind11 keyword args functionality

using namespace realization;
using namespace utils::ngenPy;

typedef struct py_formulation_example_scenario {
    std::string catchment_id;
    std::string realization_config_json;
    std::shared_ptr<struct forcing_params> forcing_params;
    std::string bmi_init_config;
    boost::property_tree::ptree realization_config_properties_tree;
    std::string module_directory;
    std::string module_name;
    std::shared_ptr<Bmi_Py_Formulation> formulation;
    std::string main_output_variable;
    bool uses_forcing_file;
} py_formulation_example_scenario;

class Bmi_Py_Formulation_Test : public ::testing::Test {
private:
    static std::shared_ptr<InterpreterUtil> interperter;
protected:
    
    py::object Path;

    std::vector<py_formulation_example_scenario> examples;
    std::vector<std::string> expected_output_var_names = { "OUTPUT_VAR_1", "OUTPUT_VAR_2", "OUTPUT_VAR_3" };

    static std::string get_friend_bmi_init_config(const Bmi_Py_Formulation& formulation) {
        return formulation.get_bmi_init_config();
    }

    static std::string get_friend_bmi_main_output_var(const Bmi_Py_Formulation& formulation) {
        return formulation.get_bmi_main_output_var();
    }

    static std::shared_ptr<models::bmi::Bmi_Py_Adapter> get_friend_bmi_model(Bmi_Py_Formulation& formulation) {
        return formulation.get_bmi_model();
    }

    static time_t get_friend_bmi_model_start_time_forcing_offset_s(Bmi_Py_Formulation& formulation) {
        return formulation.get_bmi_model_start_time_forcing_offset_s();
    }

    //static double get_friend_forcing_param_value(Bmi_Py_Formulation& formulation, const std::string& param_name) {
    //    return formulation.forcing.get_value_for_param_name(param_name);
    //}

    static std::string get_friend_forcing_file_path(const Bmi_Py_Formulation& formulation) {
        return formulation.get_forcing_file_path();
    }

    static time_t get_friend_forcing_start_time(Bmi_Py_Formulation& formulation) {
        return formulation.forcing->get_data_start_time();
    }

    static bool get_friend_is_bmi_using_forcing_file(const Bmi_Py_Formulation& formulation) {
        return formulation.is_bmi_using_forcing_file();
    }

    static std::string get_friend_model_type_name(Bmi_Py_Formulation& formulation) {
        return formulation.get_model_type_name();
    }

    static double get_friend_var_value_as_double(Bmi_Py_Formulation& formulation, const string& var_name) {
        return formulation.get_var_value_as_double(var_name);
    }

    static time_t parse_forcing_time(const std::string& date_time_str) {

        std::locale format = std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S"));

        boost::posix_time::ptime pt;
        std::istringstream is(date_time_str);
        is.imbue(format);
        is >> pt;

        boost::posix_time::ptime timet_start(boost::gregorian::date(1970,1,1));
        boost::posix_time::time_duration diff = pt - timet_start;
        return diff.ticks() / boost::posix_time::time_duration::rep_type::ticks_per_second;
    }

    /**
     * Find the repo root directory, starting from the current directory and working upward.
     *
     * @return The absolute path of the repo root, as a string.
     */
    static std::string py_find_repo_root();

    /**
     * Search for and return the first existing example of a collection of directories, using Python to perform search.
     *
     * @param dir_options The options for the directories to check for existence, in the order to try them.
     * @return The path of the first found option which is an existing directory.
     */
    static std::string py_dir_search(const std::vector<std::string> &dir_options);

    /**
     * Find the virtual environment site packages directory, starting from an assumed valid venv directory.
     *
     * @param venv_dir The virtual environment directory.
     * @return The absolute path of the site packages directory, as a string.
     */
    static std::string py_find_venv_site_packages_dir(const std::string &venv_dir);

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
     * Generate the appropriate formulation/realization config property tree, saving in the example.
     * 
     * The function assumes this instance's example collection has been initialized.  It also requires that much of the
     * contents of each example have been set up, since several pieces are used to assemble the config.
     * 
     * @param ex_idx The index of the example for which to generate.
     */
    void generate_realization_config(int ex_idx);

    void SetUp() override;

    void TearDown() override;

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

};
//Make sure the interperter is instansiated and lives throught the test class
std::shared_ptr<InterpreterUtil> Bmi_Py_Formulation_Test::interperter = InterpreterUtil::getInstance();

void Bmi_Py_Formulation_Test::SetUp() {
    Path = InterpreterUtil::getPyModule(std::vector<std::string> {"pathlib", "Path"});
    
    std::string repo_root = py_find_repo_root();
    std::string forcing_data_dir = repo_root + "/data/forcing/";

    py_formulation_example_scenario template_ex_struct;
    // These should be safe for all examples
    template_ex_struct.module_name = "test_bmi_py.bmi_model";
    template_ex_struct.module_directory = repo_root + "/extern/";
    template_ex_struct.main_output_variable = "OUTPUT_VAR_1";
    template_ex_struct.uses_forcing_file = false;

#define BMI_PY_FORM_TEST_EX_COUNT 1
    // Now generate the examples vector based on the above template example, setting it to appropriate num of examples
    examples = std::vector<py_formulation_example_scenario>(BMI_PY_FORM_TEST_EX_COUNT);
    for (size_t i = 0; i < examples.size(); ++i) {
        examples[i] = template_ex_struct;
    }

    // Set up a few things individually that are distinct to each example
    examples[0].catchment_id = "cat-27";

    // We can handle setting the rest up in a loop
    std::string forcing_file;
    for (size_t i = 0; i < examples.size(); ++i) {
        examples[i].bmi_init_config = repo_root + "/test/data/bmi/test_bmi_python/test_bmi_python_config_"
                                      + std::to_string(i) + ".yml";
        forcing_file = forcing_data_dir + examples[i].catchment_id + "_2015-12-01 00_00_00_2015-12-30 23_00_00.csv";
        examples[i].forcing_params = std::make_shared<forcing_params>(forcing_file, "legacy", "2015-12-01 00:00:00",
                                                                      "2015-12-30 23:00:00");
        generate_realization_config((int)i);

        examples[i].formulation = std::make_shared<Bmi_Py_Formulation>(examples[i].catchment_id,
                                                                       std::make_unique<CsvPerFeatureForcingProvider>(*examples[i].forcing_params),
                                                                       utils::StreamHandler());
        examples[i].formulation->create_formulation(examples[i].realization_config_properties_tree);
    }

}

void Bmi_Py_Formulation_Test::SetUpTestSuite() {
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

void Bmi_Py_Formulation_Test::TearDown() {

}

void Bmi_Py_Formulation_Test::TearDownTestSuite() {

}

/**
 * Generate the appropriate formulation/realization config property tree, saving in the example.
 *
 * The function assumes this instance's example collection has been initialized.  It also requires that much of the
 * contents of each example have been set up, since several pieces are used to assemble the config.
 *
 * @param example_index The index of the example for which to generate.
 */
void Bmi_Py_Formulation_Test::generate_realization_config(int ex_idx) {

    //std::string variables_line = (i == 1) ? variables_with_rain_rate : "";
    std::string variables_line = "";

    examples[ex_idx].realization_config_json = "{"
              "    \"global\": {},"
              "    \"catchments\": {"
              "        \"" + examples[ex_idx].catchment_id + "\": {"
              "            \"bmi_python\": {"
              "                \"model_type_name\": \"" + examples[ex_idx].module_name + "\","
              "                \"" + BMI_REALIZATION_CFG_PARAM_OPT__PYTHON_TYPE_NAME + "\": \"" + examples[ex_idx].module_name + "\","
              "                \"forcing_file\": \"" + examples[ex_idx].forcing_params->path + "\","
              "                \"init_config\": \"" + examples[ex_idx].bmi_init_config + "\","
              "                \"main_output_variable\": \"" + examples[ex_idx].main_output_variable + "\","
              "                \"" + BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION + "\": 6, "
              "                \"" + BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES + "\": { "
              "                      \"INPUT_VAR_2\": \"" + NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP + "\","
              "                      \"INPUT_VAR_1\": \"" + AORC_FIELD_NAME_PRECIP_RATE + "\""
              "                },"
              + variables_line +
              "                \"uses_forcing_file\": " + (examples[ex_idx].uses_forcing_file ? "true" : "false") + ""
              "            },"
              "            \"forcing\": { \"path\": \"" + examples[ex_idx].forcing_params->path + "\"}"
              "        }"
              "    }"
              "}";

    std::stringstream stream;
    stream << examples[ex_idx].realization_config_json;

    boost::property_tree::ptree loaded_tree;
    boost::property_tree::json_parser::read_json(stream, loaded_tree);
    examples[ex_idx].realization_config_properties_tree = loaded_tree.get_child("catchments").get_child(
            examples[ex_idx].catchment_id).get_child("bmi_python");
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
std::string Bmi_Py_Formulation_Test::file_search(const vector<std::string> &parent_dir_options, const string &file_basename) {
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
std::string Bmi_Py_Formulation_Test::file_search(const vector<std::string> &parent_dir_options,
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
std::string Bmi_Py_Formulation_Test::py_dir_search(const std::vector<std::string> &dir_options) {
    py::object Path = InterpreterUtil::getPyModule(std::vector<std::string> {"pathlib", "Path"});
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
std::string Bmi_Py_Formulation_Test::py_find_repo_root() {
    py::object Path = InterpreterUtil::getPyModule(std::vector<std::string> {"pathlib", "Path"});
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
std::string Bmi_Py_Formulation_Test::py_find_venv_site_packages_dir(const std::string &venv_dir) {
    py::object Path = InterpreterUtil::getPyModule(std::vector<std::string> {"pathlib", "Path"});
    py::object venv_dir_path = Path(venv_dir.c_str()).attr("resolve")();
    py::list site_packages_options = py::list(venv_dir_path.attr("glob")("**/site-packages"));
    for (auto &opt : site_packages_options) {
        return py::str(opt);
    }
    return "";
}

/**
 * Simple test to make sure the model initializes.
 */
TEST_F(Bmi_Py_Formulation_Test, Initialize_0_a) {
    int ex_index = 0;

    ASSERT_EQ(get_friend_model_type_name(*examples[ex_index].formulation), examples[ex_index].module_name);
    ASSERT_EQ(get_friend_forcing_file_path(*examples[ex_index].formulation), examples[ex_index].forcing_params->path);
    ASSERT_EQ(get_friend_bmi_init_config(*examples[ex_index].formulation), examples[ex_index].bmi_init_config);
    ASSERT_EQ(get_friend_bmi_main_output_var(*examples[ex_index].formulation), examples[ex_index].main_output_variable);
    ASSERT_EQ(get_friend_is_bmi_using_forcing_file(*examples[ex_index].formulation), examples[ex_index].uses_forcing_file);
}

/**
 * Simple test of get response.
 */
TEST_F(Bmi_Py_Formulation_Test, get_response_0_a) {
    int ex_index = 0;

    double response = examples[ex_index].formulation->get_response(0, 3600);
    ASSERT_EQ(response, 00);
}

/**
 * Test of get_response after several iterations.
 */
TEST_F(Bmi_Py_Formulation_Test, get_response_0_b) {
    int ex_index = 0;

    double response;
    for (int i = 0; i < 39; i++) {
        response = examples[ex_index].formulation->get_response(i, 3600);
    }
    double expected = 2.7809780039160068e-08;
    ASSERT_EQ(expected, response);
}

/**
 * Test to make sure we can execute multiple model instances.
 */
TEST_F(Bmi_Py_Formulation_Test, DISABLED_GetResponse_1_a) {
    // TODO: implement
    ASSERT_TRUE(false);
}

/**
 * Simple test of output.
 */
TEST_F(Bmi_Py_Formulation_Test, GetOutputLineForTimestep_0_a) {
    int ex_index = 0;

    double response = examples[ex_index].formulation->get_response(0, 3600);
    std::string output = examples[ex_index].formulation->get_output_line_for_timestep(0, ",");
    ASSERT_EQ(output, "0.000000,0.000000,1.000000");
}

/**
 * Simple test of output, picking time step when there was non-zero rain rate.
 */
TEST_F(Bmi_Py_Formulation_Test, GetOutputLineForTimestep_0_b) {
    int ex_index = 0;

    int i = 0;
    while (i < 542)
        examples[ex_index].formulation->get_response(i++, 3600);

    double response = examples[ex_index].formulation->get_response(543, 3600);
    std::string output = examples[ex_index].formulation->get_output_line_for_timestep(543, ",");
    std::regex expected ("0.000001,(-?)0.000000,544.000000");
    ASSERT_TRUE(std::regex_match(output, expected));
}

/**
 * Simple test of determine_model_time_offset.
 */
TEST_F(Bmi_Py_Formulation_Test, determine_model_time_offset_0_a) {
    int ex_index = 0;

    std::shared_ptr<models::bmi::Bmi_Py_Adapter> model_adapter = get_friend_bmi_model(*examples[ex_index].formulation);

    double model_start = model_adapter->GetStartTime();
    ASSERT_EQ(model_start, 0.0);

    time_t forcing_start = get_friend_forcing_start_time(*examples[ex_index].formulation);

    ASSERT_EQ(forcing_start, parse_forcing_time("2015-12-01 00:00:00"));
}

/**
 * Test of get_var_value_as_double.
 */
TEST_F(Bmi_Py_Formulation_Test, get_var_value_as_double_0_a) {
    int ex_index = 0;

    std::shared_ptr<models::bmi::Bmi_Py_Adapter> model_adapter = get_friend_bmi_model(*examples[ex_index].formulation);

    double value = 4;

    model_adapter->SetValue("INPUT_VAR_2", &value);

    double retrieved = get_friend_var_value_as_double(*examples[ex_index].formulation, "INPUT_VAR_2");

    ASSERT_EQ(value, retrieved);
}

#endif // ACTIVATE_PYTHON

#endif // NGEN_BMI_PY_TESTS_ACTIVE
