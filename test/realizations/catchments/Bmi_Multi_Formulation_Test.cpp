#ifndef NGEN_BMI_MULTI_FORMULATION_TEST_CPP
#define NGEN_BMI_MULTI_FORMULATION_TEST_CPP

// Don't bother with the rest if none of these are active (although what are we really doing here, then?)
#if NGEN_BMI_C_LIB_ACTIVE || NGEN_BMI_FORTRAN_ACTIVE || ACTIVATE_PYTHON

#include "Bmi_Testing_Util.cpp"
#include <map>
#include <vector>
#include "gtest/gtest.h"
#include "Bmi_Multi_Formulation.hpp"
#include "Bmi_Module_Formulation.hpp"
#include "CsvPerFeatureForcingProvider.hpp"
#include "FileChecker.h"

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

    static std::string get_friend_nested_module_forcing_file_path(const Bmi_Multi_Formulation& formulation,
                                                                  const int nested_index) {
        return formulation.modules[nested_index]->get_forcing_file_path();
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
            // Registration function is only needed for C or Fortran type modules.
            type_specific_properties += "                                \"registration_function\": \"" + nested_registration_function_lists[ex_index][nested_index] + "\",\n";
        }
        if (bmi_type == BMI_PYTHON_TYPE) {
            // Python requires the "python_type" to be set
            type_specific_properties += "                                \"" + std::string(BMI_REALIZATION_CFG_PARAM_OPT__PYTHON_TYPE_NAME) + "\": \"" + bmi_type + "\",\n";
        }

        std::string nested_index_str = std::to_string(nested_index);
        std::string input_var_alias = nested_index == 0 ? AORC_FIELD_NAME_PRECIP_RATE : nested_index_str;

        std::string uses_forcing_file_text = uses_forcing_file[ex_index] ? "true" : "false";

        return  "                        {\n"
                "                            \"name\": \"" + bmi_type + "\",\n"
                "                            \"params\": {\n"
                "                                \"model_type_name\": \"test_" + bmi_type + "_with_multi\",\n"
                "                                \"library_file\": \"" + nested_module_file_lists[ex_index][nested_index] + "\",\n"
                "                                \"forcing_file\": \"\",\n"
                "                                \"init_config\": \"" + nested_init_config_lists[ex_index][nested_index] + "\",\n"
                "                                \"allow_exceed_end_time\": true,\n"
                "                                \"main_output_variable\": \"" + nested_module_main_output_variables[ex_index][nested_index] + "\",\n"
                + type_specific_properties +
                "                                \"variables_names_map\": {\n"
                "                                    \"OUTPUT_VAR_2\": \"OUTPUT_VAR_2__" + nested_index_str + "\",\n"
                "                                    \"OUTPUT_VAR_1\": \"OUTPUT_VAR_1__" + nested_index_str + "\",\n"
                "                                    \"INPUT_VAR_2\": \"" + NGEN_STD_NAME_POTENTIAL_ET_FOR_TIME_STEP + "\",\n"
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

};

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

#ifdef NGEN_BMI_C_LIB_ACTIVE
    bool is_c_active = true;
#else
    bool is_c_active = false;
#endif

#ifdef NGEN_BMI_FORTRAN_LIB_ACTIVE
    bool is_fortran_active = true;
#else
    bool is_fortran_active = false;
#endif

#ifdef ACTIVATE_PYTHON
    bool is_python_active = true;
#else
    bool is_python_active = false;
#endif

    /* ********************************** First example scenario ********************************** */
    catchment_ids[0] = "cat-27";
    example_forcing_files[0] = testUtil.getForcingFilePath(catchment_ids[0]);
    main_output_variables[0] = "OUTPUT_VAR_1";
    uses_forcing_file[0] = false;
    forcing_params_examples[0] = std::make_shared<forcing_params>(
            forcing_params(example_forcing_files[0], "CsvPerFeature", "2015-12-01 00:00:00", "2015-12-30 23:00:00"));

    // Try to do Fortran / C, falling back to Python if either are missing, and finally to all that is available
    if      (is_fortran_active) { nested_module_lists[0][0] = BMI_FORTRAN_TYPE; }
    else if (is_python_active)  { nested_module_lists[0][0] = BMI_PYTHON_TYPE; }
    else                        { nested_module_lists[0][0] = BMI_C_TYPE; }

    if      (is_c_active) { nested_module_lists[0][1] = BMI_C_TYPE; }
    else if (is_python_active)  { nested_module_lists[0][1] = BMI_PYTHON_TYPE; }
    else                        { nested_module_lists[0][1] = BMI_FORTRAN_TYPE; }


    std::string typeKey;

    for (int j = 0; j < nested_module_lists[0].size(); ++j) {
        typeKey = nested_module_lists[0][j];
        nested_module_type_name_lists[0][j] = testUtil.bmiFormulationConfigNames.at(typeKey);
        nested_module_file_lists[0][j] = testUtil.getModuleFilePath(typeKey);
        nested_init_config_lists[0][j] = testUtil.getBmiInitConfigFilePath(typeKey, j);
        // For any Python modules, this isn't strictly correct, but it'll be ignored.
        nested_registration_function_lists[0][j] = "register_bmi";

    }
    buildExampleConfig(0);

    /* ********************************** Second example scenario ********************************** */
    catchment_ids[1] = "cat-27";
    example_forcing_files[1] = testUtil.getForcingFilePath(catchment_ids[1]);
    main_output_variables[1] = "OUTPUT_VAR_1";
    uses_forcing_file[1] = false;
    forcing_params_examples[1] = std::make_shared<forcing_params>(
            forcing_params(example_forcing_files[1], "CsvPerFeature", "2015-12-01 00:00:00", "2015-12-30 23:00:00"));

    // Try to do Fortran / Python, falling back to C if either are missing, and finally to all that is available
    if      (is_fortran_active) { nested_module_lists[1][0] = BMI_FORTRAN_TYPE; }
    else if (is_c_active)       { nested_module_lists[1][0] = BMI_C_TYPE; }
    else                        { nested_module_lists[1][0] = BMI_PYTHON_TYPE; }

    if      (is_python_active)  { nested_module_lists[1][1] = BMI_PYTHON_TYPE; }
    else if (is_c_active)       { nested_module_lists[1][1] = BMI_C_TYPE; }
    else                        { nested_module_lists[1][1] = BMI_FORTRAN_TYPE; }

    for (int j = 0; j < nested_module_lists[1].size(); ++j) {
        typeKey = nested_module_lists[1][j];
        nested_module_type_name_lists[1][j] = testUtil.bmiFormulationConfigNames.at(typeKey);
        nested_module_file_lists[1][j] = testUtil.getModuleFilePath(typeKey);
        nested_init_config_lists[1][j] = testUtil.getBmiInitConfigFilePath(typeKey, j);
        // For any Python modules, this isn't strictly correct, but it'll be ignored.
        nested_registration_function_lists[1][j] = "register_bmi";

    }
    buildExampleConfig(1);
}

/** Simple test to make sure the model config from example 0 initializes. */
TEST_F(Bmi_Multi_Formulation_Test, Initialize_0_a) {
    int ex_index = 0;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_nested_module_model_type_name(formulation, 0), nested_module_type_name_lists[ex_index][0]);
    ASSERT_EQ(get_friend_nested_module_model_type_name(formulation, 1), nested_module_type_name_lists[ex_index][1]);
    ASSERT_EQ(get_friend_nested_module_forcing_file_path(formulation, 0), example_forcing_files[ex_index]);
    ASSERT_EQ(get_friend_nested_module_forcing_file_path(formulation, 1), example_forcing_files[ex_index]);
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
    ASSERT_EQ(get_friend_nested_module_forcing_file_path(formulation, 0), example_forcing_files[ex_index]);
    ASSERT_EQ(get_friend_nested_module_forcing_file_path(formulation, 1), example_forcing_files[ex_index]);
    ASSERT_EQ(get_friend_bmi_main_output_var(formulation), main_output_variables[ex_index]);
    ASSERT_EQ(get_friend_is_bmi_using_forcing_file(formulation), uses_forcing_file[ex_index]);
}

#endif // NGEN_BMI_C_LIB_ACTIVE || NGEN_BMI_FORTRAN_ACTIVE || ACTIVATE_PYTHON

#endif // NGEN_BMI_MULTI_FORMULATION_TEST_CPP
