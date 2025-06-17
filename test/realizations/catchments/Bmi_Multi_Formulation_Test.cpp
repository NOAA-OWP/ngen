#ifndef NGEN_BMI_MULTI_FORMULATION_TEST_CPP
#define NGEN_BMI_MULTI_FORMULATION_TEST_CPP

#include <NGenConfig.h>

// Don't bother with the rest if none of these are active (although what are we really doing here, then?)
#if NGEN_WITH_BMI_C || NGEN_WITH_BMI_FORTRAN || NGEN_WITH_PYTHON

#include "all.h"
#include "Bmi_Testing_Util.hpp"
#include <exception>
#include <map>
#include <vector>
#include "gtest/gtest.h"
#include "Bmi_Multi_Formulation.hpp"
#include "Bmi_Module_Formulation.hpp"
#include "Bmi_Fortran_Formulation.hpp"
#include "Bmi_Py_Formulation.hpp"
#include "CsvPerFeatureForcingProvider.hpp"
#include "ConfigurationException.hpp"
#include "FileChecker.h"

#if NGEN_WITH_PYTHON
#include "python/InterpreterUtil.hpp"
using namespace utils::ngenPy;
#endif // NGEN_WITH_PYTHON

using namespace realization;

class Bmi_Multi_Formulation_Test : public ::testing::Test {
private:
#if NGEN_WITH_PYTHON
    static std::shared_ptr<InterpreterUtil> interpreter;
#endif
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

    static const std::vector<std::shared_ptr<data_access::OptionalWrappedDataProvider>> &get_friend_deferred_providers(
            const Bmi_Multi_Formulation& formulation)
    {
        return formulation.deferredProviders;
    }

    static std::string get_friend_nested_module_main_output_variable(const Bmi_Multi_Formulation& formulation,
                                                                     const int nested_index) {
        return formulation.modules[nested_index]->get_bmi_main_output_var();
    }

    template <class N>
    static double get_friend_nested_var_value(const Bmi_Multi_Formulation& formulation, const int mod_index,
                                         const std::string& var_name) {
        std::shared_ptr<N> nested = std::static_pointer_cast<N>(formulation.modules[mod_index]);
        return nested->get_var_value_as_double(0, var_name);
    }

    static std::string get_friend_catchment_id(Bmi_Multi_Formulation& formulation){
        return formulation.get_catchment_id();
    }

    template <class N>
    static std::string get_friend_nested_catchment_id(const Bmi_Multi_Formulation& formulation, const int mod_index) {
        std::shared_ptr<N> nested = std::static_pointer_cast<N>(formulation.modules[mod_index]);
        return nested->get_catchment_id();
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

    template<class M, class N>
    static std::shared_ptr<N> get_friend_bmi_adapter(const Bmi_Multi_Formulation& formulation, const int mod_index) {
        std::shared_ptr<N> nested = std::dynamic_pointer_cast<N>(std::static_pointer_cast<M>(formulation.modules[mod_index])->get_bmi_model());
        return nested;
    }

    static time_t get_friend_bmi_model_start_time_forcing_offset_s(Bmi_Multi_Formulation& formulation) {
        return formulation.get_bmi_model_start_time_forcing_offset_s();
    }

    /*
    static time_t get_friend_forcing_start_time(Bmi_Multi_Formulation& formulation) {
        return formulation.forcing->get_forcing_output_time_begin("");
    }
    */

    static std::string get_friend_nested_module_model_type_name(Bmi_Multi_Formulation& formulation,
                                                                const int nested_index) {
        return formulation.modules[nested_index]->get_model_type_name();
    }

    /*
    static double get_friend_var_value_as_double(Bmi_Multi_Formulation& formulation, const string& var_name) {
        return formulation.get_var_value_as_double(0, var_name);
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
    /** Lists for "output_variables" entries; one list per example, NOT one string per nested module */
    std::vector<std::vector<std::string>> specified_output_variables;
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
        specified_output_variables = std::vector<std::vector<std::string>>(EX_COUNT);
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

    inline std::string buildNestedC(const int ex_index, const int nested_index) {
        std::string nested_index_str = std::to_string(nested_index);
        std::string input_var_alias = determineNestedInputAliasValue(ex_index, nested_index);
        return  "                        {\n"
                "                            \"name\": \"" + std::string(BMI_C_TYPE) + "\",\n"
                "                            \"params\": {\n"
                "                                \"model_type_name\": \"" + nested_module_type_name_lists[ex_index][nested_index] + "\",\n"
                "                                \"forcing_file\": \"\",\n"
                "                                \"init_config\": \"" + nested_init_config_lists[ex_index][nested_index] + "\",\n"
                "                                \"allow_exceed_end_time\": true,\n"
                "                                \"main_output_variable\": \"" + nested_module_main_output_variables[ex_index][nested_index] + "\",\n"
                "                                \"" + BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION + "\": 6,\n"

                "                                \"library_file\": \"" + nested_module_file_lists[ex_index][nested_index] + "\",\n"
                "                                \"registration_function\": \"" + nested_registration_function_lists[ex_index][nested_index] + "\",\n"

                "                                \"variables_names_map\": {\n"
                "                                    \"OUTPUT_VAR_2\": \"OUTPUT_VAR_2__" + nested_index_str + "\",\n"
                "                                    \"OUTPUT_VAR_1\": \"OUTPUT_VAR_1__" + nested_index_str + "\",\n"
                "                                    \"INPUT_VAR_2\": \"" + CSDMS_STD_NAME_SURFACE_AIR_PRESSURE + "\",\n"
                "                                    \"INPUT_VAR_1\": \"" + input_var_alias + "\",\n"
                "                                    \"GRID_VAR_1\": \""  + input_var_alias +"\"\n"
                "                                },\n"
                "                                \"uses_forcing_file\": " + (uses_forcing_file[ex_index] ? "true" : "false") + "\n"
                "                            }\n"
                "                        }";
    }

    inline std::string buildNestedFortran(const int ex_index, const int nested_index) {
        std::string nested_index_str = std::to_string(nested_index);
        std::string input_var_alias = determineNestedInputAliasValue(ex_index, nested_index);
        return  "                        {\n"
                "                            \"name\": \"" + std::string(BMI_FORTRAN_TYPE) + "\",\n"
                "                            \"params\": {\n"
                "                                \"model_type_name\": \"" + nested_module_type_name_lists[ex_index][nested_index] + "\",\n"
                "                                \"forcing_file\": \"\",\n"
                "                                \"init_config\": \"" + nested_init_config_lists[ex_index][nested_index] + "\",\n"
                "                                \"allow_exceed_end_time\": true,\n"
                "                                \"main_output_variable\": \"" + nested_module_main_output_variables[ex_index][nested_index] + "\",\n"
                "                                \"" + BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION + "\": 6,\n"

                "                                \"library_file\": \"" + nested_module_file_lists[ex_index][nested_index] + "\",\n"
                "                                \"registration_function\": \"" + nested_registration_function_lists[ex_index][nested_index] + "\",\n"

                "                                \"variables_names_map\": {\n"
                "                                    \"OUTPUT_VAR_3\": \"OUTPUT_VAR_3__" + nested_index_str + "\",\n"
                "                                    \"INPUT_VAR_3\": \"" + std::string(CSDMS_STD_NAME_SURFACE_TEMP) + "\",\n"
                "                                    \"OUTPUT_VAR_2\": \"OUTPUT_VAR_2__" + nested_index_str + "\",\n"
                "                                    \"OUTPUT_VAR_1\": \"OUTPUT_VAR_1__" + nested_index_str + "\",\n"
                "                                    \"INPUT_VAR_2\": \"" + CSDMS_STD_NAME_SURFACE_AIR_PRESSURE + "\",\n"
                "                                    \"INPUT_VAR_1\": \"" + input_var_alias + "\",\n"
                "                                    \"GRID_VAR_1\": \"" + input_var_alias + "\",\n"
                "                                    \"GRID_VAR_2\": \"FORTRAN_Grid_Var_2__" + nested_index_str + "\",\n"
                "                                    \"GRID_VAR_3\": \"FORTRAN_Grid_Var_3__" + nested_index_str + "\"\n"
                "                                },\n"
                "                                \"uses_forcing_file\": " + (uses_forcing_file[ex_index] ? "true" : "false") + "\n"
                "                            }\n"
                "                        }";
    }

    inline std::string buildNestedPython(const int ex_index, const int nested_index) {
        std::string nested_index_str = std::to_string(nested_index);
        std::string input_var_alias = determineNestedInputAliasValue(ex_index, nested_index);
        return  "                        {\n"
                "                            \"name\": \"" + std::string(BMI_PYTHON_TYPE) + "\",\n"
                "                            \"params\": {\n"
                "                                \"model_type_name\": \"" + nested_module_type_name_lists[ex_index][nested_index] + "\",\n"
                "                                \"forcing_file\": \"\",\n"
                "                                \"init_config\": \"" + nested_init_config_lists[ex_index][nested_index] + "\",\n"
                "                                \"allow_exceed_end_time\": true,\n"
                "                                \"main_output_variable\": \"" + nested_module_main_output_variables[ex_index][nested_index] + "\",\n"
                "                                \"" + BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION + "\": 6,\n"

                "                                \"" + std::string(BMI_REALIZATION_CFG_PARAM_OPT__PYTHON_TYPE_NAME) + "\": \"" + nested_module_type_name_lists[ex_index][nested_index] + "\",\n"

                "                                \"variables_names_map\": {\n"
                "                                    \"OUTPUT_VAR_2\": \"OUTPUT_VAR_2__" + nested_index_str + "\",\n"
                "                                    \"OUTPUT_VAR_1\": \"OUTPUT_VAR_1__" + nested_index_str + "\",\n"
                "                                    \"INPUT_VAR_2\": \"" + CSDMS_STD_NAME_SURFACE_AIR_PRESSURE + "\",\n"
                "                                    \"INPUT_VAR_1\": \"" + input_var_alias + "\",\n"
                "                                    \"GRID_VAR_1\": \""  + input_var_alias +"\"\n"
                "                                },\n"
                "                                \"uses_forcing_file\": " + (uses_forcing_file[ex_index] ? "true" : "false") + "\n"
                "                            }\n"
                "                        }";
    }

    /**
     * Properly determine the mapped alias of one of the inputs, depending on the situation
     *
     * @param ex_index The index of the example config, corresponding to other index-specific saved values.
     * @param nested_index The index of the particular module within the overall example config
     * @return The appropriate input alias value for the example config being generated.
     */
    inline std::string determineNestedInputAliasValue(const int ex_index, const int nested_index) {
        // For the first two examples (i.e. not 3 or 4), have an input of all but 1st module be an output of a prior
        if (ex_index < 2 || ex_index >= 4) {
            if (nested_index == 0)  return AORC_FIELD_NAME_PRECIP_RATE;
            else                    return "OUTPUT_VAR_1__" + std::to_string(nested_index - 1);
        }
        // For the third, have the alias be a completely bogus value
        else if (ex_index == 2) {
            return "a_completely_bogus_value";
        }
        // For fourth, have alias for 1st module be an output from the 2nd module, to test lookback/deferred provided
        else if (ex_index == 3) {
            if (nested_index == 0)  return "OUTPUT_VAR_2__1";
            else                    return AORC_FIELD_NAME_PRECIP_RATE;
        }
            // This isn't a case that is expected, so ...
        else {
            throw std::runtime_error("Unexpected example index in config setup for BMI Multi Formulation tests");
        }
    }

    inline std::string buildExampleNestedModuleSubConfig(const int ex_index, const int nested_index) {
        std::string bmi_type = nested_module_lists[ex_index][nested_index];
        // Call right language-specific generator function for nested config
        if (bmi_type == BMI_C_TYPE) {
            return buildNestedC(ex_index, nested_index);
        }
        else if (bmi_type == BMI_FORTRAN_TYPE) {
            return buildNestedFortran(ex_index, nested_index);
        }
        else if (bmi_type == BMI_PYTHON_TYPE) {
            return buildNestedPython(ex_index, nested_index);
        }
        // Again, we don't quite support this yet
        else {
            throw std::runtime_error(
                    "Unsupported type '" + bmi_type + "' received for BMI Multi Formulation test example");
        }
    }

    inline std::string buildExampleOutputVariablesSubConfig(const int ex_index){
        auto list = specified_output_variables[ex_index];
        std::string s = "";
        if(list.size() == 0){
            return s;
        }
        s = ",\"output_variables\": [";
        std::string comma = "";
        for (auto item : list) {
            s += comma + "\"" + item + "\"" ;
            comma = ",";
        }
        s += "]";
        return s;
    }

    inline void buildExampleConfig(const int ex_index) {
        std::string config =
                "{\n"
                "    \"global\": {},\n"
                "    \"catchments\": {\n"
                "        \"" + catchment_ids[ex_index] + "\": {\n"
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
                + buildExampleOutputVariablesSubConfig(ex_index) + "\n"
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

    inline void initializeTestExample(const int ex_index, const std::string &cat_id,
                                      const std::vector<std::string> &nested_types, const std::vector<std::string> &output_variables) {
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
        specified_output_variables[ex_index] = output_variables;

        buildExampleConfig(ex_index);
    }


};
//Make sure the interpreter is instansiated and lives throught the test class
#if NGEN_WITH_PYTHON
std::shared_ptr<InterpreterUtil> Bmi_Multi_Formulation_Test::interpreter = InterpreterUtil::getInstance();
#endif

void Bmi_Multi_Formulation_Test::SetUpTestSuite() {
    #if NGEN_WITH_PYTHON
    std::string module_directory = "./extern/";

    // Add the extern dir with our test lib to Python system path
    InterpreterUtil::addToPyPath(module_directory);
    #endif // NGEN_WITH_PYTHON
}

void Bmi_Multi_Formulation_Test::TearDown() {
    testing::Test::TearDown();
}

void Bmi_Multi_Formulation_Test::SetUp() {
    testing::Test::SetUp();

    // Define this manually to set how many nested modules per example, and implicitly how many examples.
    // This means example_module_depth.size() example scenarios with example_module_depth[i] nested modules in each scenario.
    example_module_depth = {2, 2, 2, 2, 2, 2};

    // Initialize the members for holding required input and result test data for individual example scenarios
    setupExampleDataCollections();

    /* ********************************** First example scenario (Fortran / C) ********************************** */
    #if !NGEN_WITH_BMI_C
    throw std::runtime_error("Error: can't run multi BMI tests for scenario at index 0 without BMI C functionality active" SOURCE_LOC);
    #endif // NGEN_WITH_BMI_C

    #if !NGEN_WITH_BMI_FORTRAN
    throw std::runtime_error("Error: can't run multi BMI tests for scenario at index 0 without BMI Fortran functionality active" SOURCE_LOC);
    #endif // NGEN_WITH_BMI_FORTRAN


    initializeTestExample(0, "cat-27", {std::string(BMI_FORTRAN_TYPE), std::string(BMI_C_TYPE)}, {});

    /* ********************************** Second example scenario ********************************** */

    #if !NGEN_WITH_BMI_FORTRAN
    throw std::runtime_error("Error: can't run multi BMI tests for scenario at index 1 without BMI Fortran functionality active" SOURCE_LOC);
    #endif // NGEN_WITH_BMI_FORTRAN

    #if !NGEN_WITH_PYTHON
    throw std::runtime_error("Error: can't run multi BMI tests for scenario at index 1 without BMI Python functionality active" SOURCE_LOC);
    #endif // NGEN_WITH_PYTHON
    //This example is used to get getting output, but since we aren't initialize the test model grid just yet, need to specifiy only the variables to ask for
    //to avoid an index error if we try to get the grid data without properly intializing the grid
    //TODO This didn't seem to work: GetOutputLineForTimestep_1_a and GetOutputLineForTimestep_1_b both still try to query GRID_VAR output even though these were set
    //Not real sure what to do with that, so leaving it here for now and just hacking in the output in the test functions
    //initializeTestExample(1, "cat-27", {std::string(BMI_FORTRAN_TYPE), std::string(BMI_PYTHON_TYPE)}, {"OUTPUT_VAR_1", "OUTPUT_VAR_2", "OUTPUT_VAR_3"});
    initializeTestExample(1, "cat-27", {std::string(BMI_FORTRAN_TYPE), std::string(BMI_PYTHON_TYPE)}, {});

    initializeTestExample(2, "cat-27", {std::string(BMI_FORTRAN_TYPE), std::string(BMI_PYTHON_TYPE)}, {});

    initializeTestExample(3, "cat-27", {std::string(BMI_FORTRAN_TYPE), std::string(BMI_PYTHON_TYPE)}, {"OUTPUT_VAR_1__1", "OUTPUT_VAR_2__1", "OUTPUT_VAR_1__0", "OUTPUT_VAR_2__0", "OUTPUT_VAR_3__0", "precip_rate" });

    // Cases 4 and 5 Specifically to test output_variables failure cases...
    initializeTestExample(4, "cat-27", {std::string(BMI_FORTRAN_TYPE), std::string(BMI_PYTHON_TYPE)}, { "bogus_variable" });
    initializeTestExample(5, "cat-27", {std::string(BMI_FORTRAN_TYPE), std::string(BMI_PYTHON_TYPE)}, { "OUTPUT_VAR_1" });
   
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
}

/** Test to make sure the model config from example 0 initializes no deferred providers. */
TEST_F(Bmi_Multi_Formulation_Test, Initialize_0_b) {
    int ex_index = 0;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_deferred_providers(formulation).size(), 0);
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
}

/** Test to make sure the model config from example 1 initializes no deferred providers. */
TEST_F(Bmi_Multi_Formulation_Test, Initialize_1_b) {
    int ex_index = 1;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_deferred_providers(formulation).size(), 0);
}

/** Simple test to make sure the model config from example 2 does not initialize because of a bad config alias. */
TEST_F(Bmi_Multi_Formulation_Test, Initialize_2_a) {
    int ex_index = 2;
    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    ASSERT_THROW(formulation.create_formulation(config_prop_ptree[ex_index]), realization::ConfigurationException);
}

/** Test to make sure the model config from example 3 initializes properly with module varible lookback configured. */
TEST_F(Bmi_Multi_Formulation_Test, Initialize_3_a) {
    int ex_index = 3;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_nested_module_model_type_name(formulation, 0), nested_module_type_name_lists[ex_index][0]);
    ASSERT_EQ(get_friend_nested_module_model_type_name(formulation, 1), nested_module_type_name_lists[ex_index][1]);
    ASSERT_EQ(get_friend_nested_module_main_output_variable(formulation, 0), nested_module_main_output_variables[ex_index][0]);
    ASSERT_EQ(get_friend_nested_module_main_output_variable(formulation, 1), nested_module_main_output_variables[ex_index][1]);
    ASSERT_EQ(get_friend_bmi_main_output_var(formulation), main_output_variables[ex_index]);
}

/** Test to make sure the model config from example 3 initializes expected number of deferred providers. */
TEST_F(Bmi_Multi_Formulation_Test, Initialize_3_b) {
    int ex_index = 3;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_GT(get_friend_deferred_providers(formulation).size(), 0);
}

/** Test to make sure the model config from example 3 initializes wrapepd provider of deferred providers properly. */
TEST_F(Bmi_Multi_Formulation_Test, Initialize_3_c) {
    int ex_index = 3;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    const std::vector<std::shared_ptr<data_access::OptionalWrappedDataProvider>> deferred = get_friend_deferred_providers(
            formulation);
    for (size_t i = 0; i < deferred.size(); ++i) {
        ASSERT_TRUE(deferred[i]->isWrappedProviderSet());
    }
}

/** Test to make sure the a non-existent variable name is not allowed in `output_variables` (see issue #535). */
TEST_F(Bmi_Multi_Formulation_Test, Initialize_4_Fails) {
    int ex_index = 4;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());

    EXPECT_THROW({
        formulation.create_formulation(config_prop_ptree[ex_index]);
    }, std::runtime_error);
}

/** Test to make sure that a remapped variable name is not allowed in `output_variables` (see issue #535)
 * This is not strictly part of some spec/requirement, but is the current behavior and is here to
 * document that, and to catch any change in behavior.
*/
TEST_F(Bmi_Multi_Formulation_Test, Initialize_5_Fails) {
    int ex_index = 5;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());

    EXPECT_THROW({
        formulation.create_formulation(config_prop_ptree[ex_index]);
    }, std::runtime_error);
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
    double expected = 2.7809780039160068e-08;
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
    double expected = 2.7809780039160068e-08;
    ASSERT_EQ(expected, response);
}

/**
 * Simple test of get response in example 3, which uses a deferred provider.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetResponse_3_a) {
    int ex_index = 3;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    ASSERT_EQ(response, 00);
}

/**
 * Test of get response in example 3, which uses a deferred provider, after several iterations.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetResponse_3_b) {
    int ex_index = 3;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response;
    for (int i = 0; i < 39; i++) {
        response = formulation.get_response(i, 3600);
    }
    double expected = 2.7809780039160068e-08;
    ASSERT_EQ(expected, response);
}

/**
 * Test of get response in example 3, which uses a deferred provider, checking the values for several iterations.
 */
    TEST_F(Bmi_Multi_Formulation_Test, GetResponse_3_c) {

/* Note that a runtime check in SetUp() prevents this from executing when it can't, but
   this needs to be here to prevent compile-time errors if either of these flags is not
   enabled. */
#if NGEN_WITH_PYTHON && NGEN_WITH_BMI_FORTRAN

        int ex_index = 3;

        Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
        formulation.create_formulation(config_prop_ptree[ex_index]);

        double response, mod_0_input_1, mod_1_output_2;
        for (int i = 0; i < 39; i++) {
            // Note that we need to get this before the "current" get_response ...
            mod_1_output_2 = get_friend_nested_var_value<Bmi_Py_Formulation>(formulation, 1, "OUTPUT_VAR_2");

            response = formulation.get_response(i, 3600);
            // But we need to get this after the "current" get_response ...
            mod_0_input_1 = get_friend_nested_var_value<Bmi_Fortran_Formulation>(formulation, 0, "INPUT_VAR_1");
            // ... and also, this won't work for the 0th index
            if (i != 0) {
                EXPECT_EQ(mod_0_input_1, mod_1_output_2);
            }
        }

#endif // NGEN_WITH_PYTHON && NGEN_WITH_BMI_FORTRAN

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
    ASSERT_EQ(output, "0.000001,199280.000000");
}

/**
 * Simple test of output for example 1.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetOutputLineForTimestep_1_a) {
/* Note that a runtime check in SetUp() prevents this from executing when it can't, but
   this needs to be here to prevent compile-time errors if this flag is not enabled. */
#if NGEN_WITH_PYTHON
    int ex_index = 1;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    //Init the test grids #FIXME do this better...highly volitile if example changes...
    //This only works if the python module is at index 1 in the multi BMI layers, and would need repeated
    //if for whatever reason there were more than one of the python test modles in the stack (which is possible...)
    //This is really just a tempoary work around to get the basic funcationality of grid support in place...
    std::shared_ptr<models::bmi::Bmi_Py_Adapter> model_adapter = get_friend_bmi_adapter<Bmi_Py_Formulation, models::bmi::Bmi_Py_Adapter>(formulation, 1);
    std::vector<int> shape = {2,3};
    model_adapter->SetValue("grid_1_shape", shape.data());
    formulation.get_response(0, 3600);
    std::string output = formulation.get_output_line_for_timestep(0, ",");
    //FIXME the last two outputs are the first value from the GRID_VAR in the python module...couldn't get the output variables
    //configured in the example realization generation to not query those, so hacked in here.  See comment above about not worrying about
    //initializing/using the grid vars in this test, and try to find a better way in the future.
    ASSERT_EQ(output, "0.000000,200620.000000,1.000000,2.000000,3.000000");
#endif // NGEN_WITH_PYTHON
}

/**
 * Simple test of output for example 1 with modified variables, picking time step when there was non-zero rain rate.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetOutputLineForTimestep_1_b) {
/* Note that a runtime check in SetUp() prevents this from executing when it can't, but
   this needs to be here to prevent compile-time errors if this flag is not enabled. */
#if NGEN_WITH_PYTHON
    int ex_index = 1;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    //Init the test grids #FIXME do this better...highly volitile if example changes...
    //This only works if the python module is at index 1 in the multi BMI layers, and would need repeated
    //if for whatever reason there were more than one of the python test modles in the stack (which is possible...)
    //This is really just a tempoary work around to get the basic funcationality of grid support in place...
    std::shared_ptr<models::bmi::Bmi_Py_Adapter> model_adapter = get_friend_bmi_adapter<Bmi_Py_Formulation, models::bmi::Bmi_Py_Adapter>(formulation, 1);
    std::vector<int> shape = {2,3};
    model_adapter->SetValue("grid_1_shape", shape.data());
    int i = 0;
    while (i < 542)
        formulation.get_response(i++, 3600);
    formulation.get_response(i, 3600);
    std::string output = formulation.get_output_line_for_timestep(i, ",");
    //FIXME the last two outputs are the first value from the GRID_VAR in the python module...couldn't get the output variables
    //configured in the example realization generation to not query those, so hacked in here.  See comment above about not worrying about
    //initializing/using the grid vars in this test, and try to find a better way in the future.
    ASSERT_EQ(output, "0.000001,199280.000000,543.000000,2.000001,3.000001");
#endif // NGEN_WITH_PYTHON
}

/**
 * Test of output for example 3 with output_variables from multiple BMI modules, picking time step when there was non-zero rain rate.
 */
TEST_F(Bmi_Multi_Formulation_Test, GetOutputLineForTimestep_3_a) {
    int ex_index = 3;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    int i = 0;
    while (i < 542)
        formulation.get_response(i++, 3600);
    formulation.get_response(i, 3600);
    std::string output = formulation.get_output_line_for_timestep(i, ",");
    ASSERT_EQ(output, "0.000001112,199280.000000000,199240.000000000,199280.000000000,0.000000000,0.000001001");
}

/**
 * Test if Catchment Ids of submodules correctly trim any suffix
 */
TEST_F(Bmi_Multi_Formulation_Test, GetIdAndCatchmentId) {
    int ex_index = 3;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    ASSERT_EQ(formulation.get_id(), "cat-27");
    ASSERT_EQ(get_friend_catchment_id(formulation), "cat-27");
    #if NGEN_WITH_BMI_FORTRAN
    ASSERT_EQ(get_friend_nested_catchment_id<Bmi_Fortran_Formulation>(formulation, 0), "cat-27");
    #endif
    //ASSERT_EQ(formulation.get_catchment_id(), "id");
}

TEST_F(Bmi_Multi_Formulation_Test, GetAvailableVariableNames) {
    int ex_index = 1;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
    
    const auto actual_names =  formulation.get_available_variable_names();
    const auto expected_names = {
        "OUTPUT_VAR_1__0",
        "OUTPUT_VAR_2__0",
        "OUTPUT_VAR_3__0",
        "FORTRAN_Grid_Var_2__0",
        "FORTRAN_Grid_Var_3__0",
        "GRID_VAR_4",
        "OUTPUT_VAR_1__1",
        "OUTPUT_VAR_2__1",
        "OUTPUT_VAR_3",
        "GRID_VAR_2",
        "GRID_VAR_3"
    };

    for (const auto& expected : expected_names) {
        EXPECT_NE(
            std::find(actual_names.begin(), actual_names.end(), expected),
            actual_names.end()
        );
    }
}
#endif // NGEN_WITH_BMI_C || NGEN_WITH_BMI_FORTRAN || NGEN_WITH_PYTHON

#endif // NGEN_BMI_MULTI_FORMULATION_TEST_CPP
