#ifndef NGEN_BMI_MULTI_FORMULATION_TEST_CPP
#define NGEN_BMI_MULTI_FORMULATION_TEST_CPP

// Don't bother with the rest if none of these are active (although what are we really doing here, then?)
#if NGEN_BMI_C_LIB_ACTIVE || NGEN_BMI_FORTRAN_ACTIVE || ACTIVATE_PYTHON

#include "Bmi_Testing_Util.hpp"
#include <exception>
#include <map>
#include <vector>
#include "gtest/gtest.h"
#include "Bmi_Multi_Formulation.hpp"
#include "Bmi_Module_Formulation.hpp"
#include "CsvPerFeatureForcingProvider.hpp"
#include "FileChecker.h"

#ifdef ACTIVATE_PYTHON
#include "python/InterpreterUtil.hpp"
using namespace utils::ngenPy;
#endif // ACTIVATE_PYTHON

using namespace realization;

class Bmi_Multi_Formulation_Test : public ::testing::Test {
protected:

    static std::string find_file(std::vector<std::string> dir_opts, const std::string& basename) {
        std::vector<std::string> file_opts(dir_opts.size());
        for (int i = 0; i < dir_opts.size(); ++i)
            file_opts[i] = dir_opts[i] + basename;
        return utils::FileChecker::find_first_readable(file_opts);
    }

    /*
    static void call_friend_determine_model_time_offset(Bmi_Multi_Formulation& formulation) {
        formulation.determine_model_time_offset();
    }
    */

    static std::string get_friend_bmi_main_output_var(const Bmi_Multi_Formulation& formulation) {
        return formulation.get_bmi_main_output_var();
    }

    static std::string get_friend_nested_module_main_output_variable(const Bmi_Multi_Formulation& formulation,
                                                                     const int nested_index) {
        return formulation.modules[nested_index]->get_bmi_main_output_var();
    }

    /*
    static std::vector<nested_module_ptr> get_friend_nested_formulations(Bmi_Multi_Formulation& formulation) {
        return formulation.get_bmi_model();
    }
    */

    template <class N, class M>
    static std::shared_ptr<M> get_friend_bmi_model(N& nested_formulation) {
        return nested_formulation.get_bmi_model();
    }

    static time_t get_friend_bmi_model_start_time_forcing_offset_s(Bmi_Multi_Formulation& formulation) {
        return formulation.get_bmi_model_start_time_forcing_offset_s();
    }

    static std::string get_friend_forcing_file_path(const Bmi_Multi_Formulation& formulation) {
        return formulation.get_forcing_file_path();
    }

    /*
    static time_t get_friend_forcing_start_time(Bmi_Multi_Formulation& formulation) {
        return formulation.forcing->get_forcing_output_time_begin("");
    }
    */

    static bool get_friend_is_bmi_using_forcing_file(const Bmi_Multi_Formulation& formulation) {
        return formulation.is_bmi_using_forcing_file();
    }

    static std::string get_friend_nested_module_model_type_name(Bmi_Multi_Formulation& formulation,
                                                                const int nested_index) {
        return formulation.modules[nested_index]->get_model_type_name();
    }

    /*
    static double get_friend_var_value_as_double(Bmi_Multi_Formulation& formulation, const string& var_name) {
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
    */

    void SetUp() override;

    static void SetUpTestSuite();

    void TearDown() override;

    ngen::test::Bmi_Testing_Util testUtil;
    /** The number of nested modules for each test example scenario. */
    std::vector<int> example_module_depth;
    /** Formulation config for each example, as JSON strings. */
    std::vector<std::string> config_json;
    /** Catchment id value for each example. */
    std::vector<std::string> catchment_ids;
    std::vector<std::string> example_forcing_files;
    /** Main output variables for the outter formulation in each example. */
    std::vector<std::string> main_output_variables;
    /** Collections of lib/package files names for nested modules of each example, in the order of the nested modules. */
    std::vector<std::vector<std::string>> nested_module_file_lists;
    /** Collections of model types for each example, in the order of the nested modules. */
    std::vector<std::vector<std::string>> nested_module_lists;
    /** Main output variables for the nested formulations of each example. */
    std::vector<std::vector<std::string>> nested_module_main_output_variables;
    /** Collections of model type names for each example, in the order of the nested modules. */
    std::vector<std::vector<std::string>> nested_module_type_name_lists;
    /** Collections of init config files names for nested modules of each example, in the order of the nested modules. */
    std::vector<std::vector<std::string>> nested_init_config_lists;
    /** Collections of names of the registrations config files names for nested modules of each example, in the order of the nested modules. */
    std::vector<std::vector<std::string>> nested_registration_function_lists;
    std::vector<bool> uses_forcing_file;
    std::vector<std::shared_ptr<forcing_params>> forcing_params_examples;
    std::vector<boost::property_tree::ptree> config_prop_ptree;

private:

    /**
     * Initialize members for holding required input, setup, and result test data for individual example scenarios.
     */
    inline void setupExampleDataCollections() {
        const int EX_COUNT = example_module_depth.size();
        config_json = std::vector<std::string>(EX_COUNT);
        catchment_ids = std::vector<std::string>(EX_COUNT);
        example_forcing_files = std::vector<std::string>(EX_COUNT);
        main_output_variables  = std::vector<std::string>(EX_COUNT);
        uses_forcing_file = std::vector<bool>(EX_COUNT);
        forcing_params_examples = std::vector<std::shared_ptr<forcing_params>>(EX_COUNT);
        config_prop_ptree = std::vector<boost::property_tree::ptree>(EX_COUNT);

        nested_module_lists = std::vector<std::vector<std::string>>(EX_COUNT);
        nested_module_type_name_lists = std::vector<std::vector<std::string>>(EX_COUNT);
        nested_module_main_output_variables = std::vector<std::vector<std::string>>(EX_COUNT);
        nested_module_file_lists = std::vector<std::vector<std::string>>(EX_COUNT);
        nested_init_config_lists = std::vector<std::vector<std::string>>(EX_COUNT);
        nested_registration_function_lists = std::vector<std::vector<std::string>>(EX_COUNT);

        for (int i = 0; i < EX_COUNT; ++i) {
            int nested_depth = example_module_depth[i];
            nested_module_lists[i] = std::vector<std::string>(nested_depth);
            nested_module_type_name_lists[i] = std::vector<std::string>(nested_depth);
            nested_module_main_output_variables[i] = std::vector<std::string>(nested_depth);
            nested_module_file_lists[i] = std::vector<std::string>(nested_depth);
            nested_init_config_lists[i] = std::vector<std::string>(nested_depth);
            nested_registration_function_lists[i] = std::vector<std::string>(nested_depth);
        }
    }

    inline std::string buildExampleNestedModuleSubConfig(const int ex_index, const int nested_index) {
        std::string bmi_type = nested_module_lists[ex_index][nested_index];

        // Certain properties must be set only for certain BMI types
        std::string type_specific_properties;
        if (bmi_type == BMI_C_TYPE || bmi_type == BMI_FORTRAN_TYPE) {
            // The library file is only needed (at least for the moment) when there is a shared lib involved
            type_specific_properties += "                                \"library_file\": \"" + nested_module_file_lists[ex_index][nested_index] + "\",\n";
            // Registration function is only needed for C or Fortran type modules.
            type_specific_properties += "                                \"registration_function\": \"" + nested_registration_function_lists[ex_index][nested_index] + "\",\n";
        }
        if (bmi_type == BMI_PYTHON_TYPE) {
            // Python requires the "python_type" to be set
            type_specific_properties += "                                \"" + std::string(BMI_REALIZATION_CFG_PARAM_OPT__PYTHON_TYPE_NAME) + "\": \"" + nested_module_type_name_lists[ex_index][nested_index] + "\",\n";
        }

        std::string nested_index_str = std::to_string(nested_index);
        std::string nested_prior_index_str = std::to_string(nested_index - 1);

        std::string type_specific_var_map;
        if (bmi_type == BMI_FORTRAN_TYPE) {
            type_specific_var_map += "                                    \"OUTPUT_VAR_3\": \"OUTPUT_VAR_3__" + nested_index_str + "\",\n";
            type_specific_var_map += "                                    \"INPUT_VAR_3\": \"" + std::string(CSDMS_STD_NAME_SURFACE_TEMP) + "\",\n";
        }

        std::string input_var_alias;
        if (nested_index == 0) {
            input_var_alias = AORC_FIELD_NAME_PRECIP_RATE;
        }
        else {
            input_var_alias = "OUTPUT_VAR_1__" + nested_prior_index_str;
        }

        std::string uses_forcing_file_text = uses_forcing_file[ex_index] ? "true" : "false";

        return  "                        {\n"
                "                            \"name\": \"" + bmi_type + "\",\n"
                "                            \"params\": {\n"
                "                                \"model_type_name\": \"" + nested_module_type_name_lists[ex_index][nested_index] + "\",\n"
                "                                \"forcing_file\": \"\",\n"
                "                                \"init_config\": \"" + nested_init_config_lists[ex_index][nested_index] + "\",\n"
                "                                \"allow_exceed_end_time\": true,\n"
                "                                \"main_output_variable\": \"" + nested_module_main_output_variables[ex_index][nested_index] + "\",\n"
                + type_specific_properties +
                "                                \"variables_names_map\": {\n"
                + type_specific_var_map +
                "                                    \"OUTPUT_VAR_2\": \"OUTPUT_VAR_2__" + nested_index_str + "\",\n"
                "                                    \"OUTPUT_VAR_1\": \"OUTPUT_VAR_1__" + nested_index_str + "\",\n"
                "                                    \"INPUT_VAR_2\": \"" + CSDMS_STD_NAME_SURFACE_AIR_PRESSURE + "\",\n"
                "                                    \"INPUT_VAR_1\": \"" + input_var_alias + "\"\n"
                "                                },\n"
                "                                \"uses_forcing_file\": " + uses_forcing_file_text + "\n"
                "                            }\n"
                "                        }";
    }

    inline void buildExampleConfig(const int ex_index) {
        int i = 0;

        std::string config =
                "{\n"
                "    \"global\": {},\n"
                "    \"catchments\": {\n"
                "        \"" + catchment_ids[i] + "\": {\n"
                "            \"formulations\": [\n"
                "                {\n"
                "                    \"name\": \"" + std::string(BMI_MULTI_TYPE) + "\",\n"
                "                    \"params\": {\n"
                "                        \"model_type_name\": \"bmi_multi_test\",\n"
                "                        \"forcing_file\": \"\",\n"
                "                        \"init_config\": \"\",\n"
                "                        \"allow_exceed_end_time\": true,\n"
                "                        \"main_output_variable\": \"" + main_output_variables[ex_index] + "\",\n"
                "                        \"modules\": [\n"
                + buildExampleNestedModuleSubConfig(ex_index, 0) + ",\n"
                + buildExampleNestedModuleSubConfig(ex_index, 1) + "\n"
                "                        ],\n"
                "                        \"uses_forcing_file\": false\n"
                "                    }\n"
                "                }\n"
                "            ],\n"
                "            \"forcing\": {\n"
                "                \"path\": \"" + example_forcing_files[ex_index] + "\",\n"
                "                \"provider\": \"CsvPerFeature\"\n"
                "            }\n"
                "        }\n"
                "    },\n"
                "    \"time\": {\n"
                "        \"start_time\": \"2012-05-01 00:00:00\",\n"
                "        \"end_time\": \"2012-05-31 23:00:00\",\n"
                "        \"output_interval\": 3600\n"
                "    }\n"
                "}";

        config_json[ex_index] = config;

        std::stringstream stream;
        stream << config_json[ex_index];

        boost::property_tree::ptree loaded_tree;
        boost::property_tree::json_parser::read_json(stream, loaded_tree);
        config_prop_ptree[ex_index] = loaded_tree.get_child("catchments").get_child(catchment_ids[ex_index]).get_child(
                "formulations").begin()->second.get_child("params");
    }

    /**
     * Find the repo root directory using Python, starting from the current directory and working upward.
     *
     * This will throw a runtime error if Python functionality is not active.
     *
     * @return The absolute path of the repo root, as a string.
     */
    static std::string py_find_repo_root() {
        #ifdef ACTIVATE_PYTHON
        py::module_ Path = InterpreterUtil::getPyModule(std::vector<std::string> {"pathlib", "Path"});
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
        #else // (i.e., if not ACTIVATE_PYTHON)
        throw std::runtime_error("Can't use Python-based test helper function 'py_find_repo_root'; Python not active!");
        #endif // ACTIVATE_PYTHON
    }

    inline void initializeTestExample(const int ex_index, const std::string &cat_id,
                                      const std::vector<std::string> &nested_types) {
        catchment_ids[ex_index] = cat_id;
        example_forcing_files[ex_index] = testUtil.getForcingFilePath(cat_id);
        uses_forcing_file[ex_index] = false;
        // TODO: re-implement things to have these be retrieved from the testing util object
        forcing_params_examples[ex_index] = std::make_shared<forcing_params>(
                forcing_params(example_forcing_files[0], "CsvPerFeature", "2015-12-01 00:00:00", "2015-12-30 23:00:00"));

        std::string typeKey;

        for (int j = 0; j < nested_module_lists[ex_index].size(); ++j) {
            typeKey = nested_types[j];
            nested_module_lists[ex_index][j] = nested_types[j];
            nested_module_type_name_lists[ex_index][j] = testUtil.bmiFormulationConfigNames.at(typeKey);
            nested_module_file_lists[ex_index][j] = testUtil.getModuleFilePath(typeKey);
            nested_init_config_lists[ex_index][j] = testUtil.getBmiInitConfigFilePath(typeKey, j);
            // TODO: look at setting this a different way
            nested_module_main_output_variables[ex_index][j] = "OUTPUT_VAR_1";
            // For any Python modules, this isn't strictly correct, but it'll be ignored.
            nested_registration_function_lists[ex_index][j] = "register_bmi";
        }
        //main_output_variables[ex_index] = "OUTPUT_VAR_1__" + std::to_string(example_module_depth[ex_index] - 1);
        main_output_variables[ex_index] = nested_module_main_output_variables[ex_index][example_module_depth[ex_index] - 1];

        buildExampleConfig(ex_index);
    }


};

void Bmi_Multi_Formulation_Test::SetUpTestSuite() {
    #ifdef ACTIVATE_PYTHON
    std::string repo_root = py_find_repo_root();
    std::string module_directory = repo_root + "/extern/";

    // Add the extern dir with our test lib to Python system path
    InterpreterUtil::addToPyPath(module_directory);
    #endif // ACTIVATE_PYTHON
}

void Bmi_Multi_Formulation_Test::TearDown() {
    testing::Test::TearDown();
}

void Bmi_Multi_Formulation_Test::SetUp() {
    testing::Test::SetUp();

    // Define this manually to set how many nested modules per example, and implicitly how many examples.
    // This means 2 example scenarios with 2 nested modules
    example_module_depth = {2, 2};

    // Initialize the members for holding required input and result test data for individual example scenarios
    setupExampleDataCollections();

    /* ********************************** First example scenario (Fortran / C) ********************************** */
    #ifndef NGEN_BMI_C_LIB_ACTIVE
    throw std::runtime_exception("Error: can't run multi BMI tests for scenario at index 0 without BMI C functionality active");
    #endif // NGEN_BMI_C_LIB_ACTIVE

    #ifndef NGEN_BMI_FORTRAN_ACTIVE
    throw std::runtime_error("Error: can't run multi BMI tests for scenario at index 0 without BMI Fortran functionality active");
    #endif // NGEN_BMI_FORTRAN_ACTIVE


    initializeTestExample(0, "cat-27", {std::string(BMI_FORTRAN_TYPE), std::string(BMI_C_TYPE)});

    /* ********************************** Second example scenario ********************************** */

    #ifndef NGEN_BMI_FORTRAN_ACTIVE
    throw std::runtime_error("Error: can't run multi BMI tests for scenario at index 1 without BMI Fortran functionality active");
    #endif // NGEN_BMI_FORTRAN_ACTIVE

    #ifndef ACTIVATE_PYTHON
    throw std::runtime_exception("Error: can't run multi BMI tests for scenario at index 1 without BMI C functionality active");
    #endif // ACTIVATE_PYTHON

    initializeTestExample(1, "cat-27", {std::string(BMI_FORTRAN_TYPE), std::string(BMI_PYTHON_TYPE)});
}

/** Simple test to make sure the model config from example 0 initializes. */
TEST_F(Bmi_Multi_Formulation_Test, Initialize_0_a) {
    int ex_index = 0;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_nested_module_model_type_name(formulation, 0), nested_module_type_name_lists[ex_index][0]);
    ASSERT_EQ(get_friend_nested_module_model_type_name(formulation, 1), nested_module_type_name_lists[ex_index][1]);
    ASSERT_EQ(get_friend_nested_module_main_output_variable(formulation, 0), nested_module_main_output_variables[ex_index][0]);
    ASSERT_EQ(get_friend_nested_module_main_output_variable(formulation, 1), nested_module_main_output_variables[ex_index][1]);
    ASSERT_EQ(get_friend_bmi_main_output_var(formulation), main_output_variables[ex_index]);
    ASSERT_EQ(get_friend_is_bmi_using_forcing_file(formulation), uses_forcing_file[ex_index]);
}

/** Simple test to make sure the model config from example 1 initializes. */
TEST_F(Bmi_Multi_Formulation_Test, Initialize_1_a) {
    int ex_index = 1;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_nested_module_model_type_name(formulation, 0), nested_module_type_name_lists[ex_index][0]);
    ASSERT_EQ(get_friend_nested_module_model_type_name(formulation, 1), nested_module_type_name_lists[ex_index][1]);
    ASSERT_EQ(get_friend_nested_module_main_output_variable(formulation, 0), nested_module_main_output_variables[ex_index][0]);
    ASSERT_EQ(get_friend_nested_module_main_output_variable(formulation, 1), nested_module_main_output_variables[ex_index][1]);
    ASSERT_EQ(get_friend_bmi_main_output_var(formulation), main_output_variables[ex_index]);
    ASSERT_EQ(get_friend_is_bmi_using_forcing_file(formulation), uses_forcing_file[ex_index]);
}

/**
 * Simple test of get response in example 0.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetResponse_0_a) {
    int ex_index = 0;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    ASSERT_EQ(response, 00);
}

/**
 * Test of get response in example 0 after several iterations.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetResponse_0_b) {
    int ex_index = 0;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response;
    for (int i = 0; i < 39; i++) {
        response = formulation.get_response(i, 3600);
    }
    double expected = 4.866464273262429e-08;
    ASSERT_EQ(expected, response);
}

/**
 * Simple test of get response in example 1.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetResponse_1_a) {
    int ex_index = 1;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    ASSERT_EQ(response, 00);
}

/**
 * Test of get response in example 1 after several iterations.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetResponse_1_b) {
    int ex_index = 1;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response;
    for (int i = 0; i < 39; i++) {
        response = formulation.get_response(i, 3600);
    }
    double expected = 4.866464273262429e-08;
    ASSERT_EQ(expected, response);
}

/**
 * Simple test of output for example 0.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetOutputLineForTimestep_0_a) {
    int ex_index = 0;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    formulation.get_response(0, 3600);
    std::string output = formulation.get_output_line_for_timestep(0, ",");
    ASSERT_EQ(output, "0.000000,200620.000000");
}

/**
 * Simple test of output for example 0 with modified variables, picking time step when there was non-zero rain rate.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetOutputLineForTimestep_0_b) {
    int ex_index = 0;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    int i = 0;
    while (i < 542)
        formulation.get_response(i++, 3600);
    formulation.get_response(i, 3600);
    std::string output = formulation.get_output_line_for_timestep(i, ",");
    ASSERT_EQ(output, "0.000002,199280.000000");
}

/**
 * Simple test of output for example 1.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetOutputLineForTimestep_1_a) {
    int ex_index = 1;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    formulation.get_response(0, 3600);
    std::string output = formulation.get_output_line_for_timestep(0, ",");
    ASSERT_EQ(output, "0.000000,200620.000000,1.000000");
}

/**
 * Simple test of output for example 1 with modified variables, picking time step when there was non-zero rain rate.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetOutputLineForTimestep_1_b) {
    int ex_index = 1;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    int i = 0;
    while (i < 542)
        formulation.get_response(i++, 3600);
    formulation.get_response(i, 3600);
    std::string output = formulation.get_output_line_for_timestep(i, ",");
    ASSERT_EQ(output, "0.000002,199280.000000,543.000000");
}

#endif // NGEN_BMI_C_LIB_ACTIVE || NGEN_BMI_FORTRAN_ACTIVE || ACTIVATE_PYTHON

#endif // NGEN_BMI_MULTI_FORMULATION_TEST_CPP
