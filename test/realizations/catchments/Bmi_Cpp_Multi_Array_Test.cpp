#ifndef NGEN_Bmi_Cpp_Multi_Array_Test_CPP
#define NGEN_Bmi_Cpp_Multi_Array_Test_CPP

#ifdef NGEN_BMI_CPP_LIB_TESTS_ACTIVE

#include "Bmi_Testing_Util.hpp"
#include <exception>
#include <map>
#include <vector>
#include "gtest/gtest.h"
#include "Bmi_Multi_Formulation.hpp"
#include "Bmi_Module_Formulation.hpp"
#include "CsvPerFeatureForcingProvider.hpp"
#include "ConfigurationException.hpp"
#include "FileChecker.h"
#include "Formulation_Manager.hpp"

using namespace realization;

class Bmi_Cpp_Multi_Array_Test : public ::testing::Test {
protected:

    static std::string find_file(std::vector<std::string> dir_opts, const std::string& basename) {
        std::vector<std::string> file_opts(dir_opts.size());
        for (int i = 0; i < dir_opts.size(); ++i)
            file_opts[i] = dir_opts[i] + basename;
        return utils::FileChecker::find_first_readable(file_opts);
    }

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
        return nested->get_var_value_as_double(var_name);
    }

    template <class N, class M>
    static std::vector<double> get_friend_nested_var_values(const Bmi_Multi_Formulation& formulation, const int mod_index,
                                         const std::string& var_name) {
        std::shared_ptr<M> module_formulation = std::static_pointer_cast<M>(formulation.modules[mod_index]);
        std::shared_ptr<N> nested = std::static_pointer_cast<N>( module_formulation->get_bmi_model());
        //return (*nested).template GetValue<double>(var_name);
        return models::bmi::GetValue<double>(*nested.get(), var_name);
    }

    template <class N, class M>
    static std::shared_ptr<M> get_friend_bmi_model(N& nested_formulation) {
        return nested_formulation.get_bmi_model();
    }

    template <class N, class M>
    static std::shared_ptr<M> get_friend_nested_bmi_model(const Bmi_Multi_Formulation& formulation, const int mod_index) {
        std::shared_ptr<N> nested_formulation = std::static_pointer_cast<N>(formulation.modules[mod_index]);
        return nested_formulation.get_bmi_model();
    }

    static time_t get_friend_bmi_model_start_time_forcing_offset_s(Bmi_Multi_Formulation& formulation) {
        return formulation.get_bmi_model_start_time_forcing_offset_s();
    }

    static std::string get_friend_forcing_file_path(const Bmi_Multi_Formulation& formulation) {
        return formulation.get_forcing_file_path();
    }

    static bool get_friend_is_bmi_using_forcing_file(const Bmi_Multi_Formulation& formulation) {
        return formulation.is_bmi_using_forcing_file();
    }

    static std::string get_friend_nested_module_model_type_name(Bmi_Multi_Formulation& formulation,
                                                                const int nested_index) {
        return formulation.modules[nested_index]->get_model_type_name();
    }

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

    inline std::string buildNested(const int ex_index, const int nested_index) {
        std::string nested_index_str = std::to_string(nested_index);
        std::string input_var_alias = determineNestedInputAliasValue(ex_index, nested_index);
        std::string array_alias = nested_index == 1 ? "OUTPUT_VAR_3" : AORC_FIELD_NAME_PRECIP_RATE;
        return  "                        {\n"
                "                            \"name\": \"" + std::string(BMI_CPP_TYPE) + "\",\n"
                "                            \"params\": {\n"
                "                                \"model_type_name\": \"" + nested_module_type_name_lists[ex_index][nested_index] + "\",\n"
                "                                \"forcing_file\": \"\",\n"
                "                                \"init_config\": \"" + nested_init_config_lists[ex_index][nested_index] + "\",\n"
                "                                \"allow_exceed_end_time\": true,\n"
                "                                \"output_bbox\": [ "
                "                                  0, "
                "                                  0, "
                "                                  3, "
                "                                  2, "
                "                                  4, "
                "                                  2, "
                "                                  1 "
                "                                ], \n"
                "                                \"main_output_variable\": \"" + nested_module_main_output_variables[ex_index][nested_index] + "\",\n"
                "                                \"" + BMI_REALIZATION_CFG_PARAM_OPT__OUTPUT_PRECISION + "\": 6,\n"

                "                                \"library_file\": \"" + nested_module_file_lists[ex_index][nested_index] + "\",\n"
                "                                \"registration_function\": \"" + nested_registration_function_lists[ex_index][nested_index] + "\",\n"

                "                                \"variables_names_map\": {\n"
                "                                    \"OUTPUT_VAR_2\": \"OUTPUT_VAR_2__" + nested_index_str + "\",\n"
                "                                    \"OUTPUT_VAR_1\": \"OUTPUT_VAR_1__" + nested_index_str + "\",\n"
                "                                    \"INPUT_VAR_2\": \"" + CSDMS_STD_NAME_SURFACE_AIR_PRESSURE + "\",\n"
                "                                    \"INPUT_VAR_1\": \"" + input_var_alias + "\",\n"
                "                                    \"INPUT_VAR_3\": \"" + array_alias + "\"\n"
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
        if (ex_index < 2) {
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
                "                        \"output_bbox\": [ "
                "                          0, "
                "                          0, "
                "                          3, "
                "                          2, "
                "                          4, "
                "                          2, "
                "                          1 "
                "                        ], \n"
                "                        \"main_output_variable\": \"" + main_output_variables[ex_index] + "\",\n"
                "                        \"modules\": [\n"
                + buildNested(ex_index, 0) + ",\n"
                + buildNested(ex_index, 1) + "\n"
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

void Bmi_Cpp_Multi_Array_Test::SetUpTestSuite() {

}

void Bmi_Cpp_Multi_Array_Test::TearDown() {
    testing::Test::TearDown();
}

void Bmi_Cpp_Multi_Array_Test::SetUp() {
    testing::Test::SetUp();

    // Define this manually to set how many nested modules per example, and implicitly how many examples.
    // This means 1 example scenarios with 2 nested modules
    example_module_depth = {2};

    // Initialize the members for holding required input and result test data for individual example scenarios
    setupExampleDataCollections();

    initializeTestExample(0, "cat-27", {std::string(BMI_CPP_TYPE), std::string(BMI_CPP_TYPE)});
}

/** Simple test to make sure the model config from example 0 initializes. */
TEST_F(Bmi_Cpp_Multi_Array_Test, Initialize_0_a) {
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

/** Test to make sure the model config from example 0 initializes no deferred providers. */
TEST_F(Bmi_Cpp_Multi_Array_Test, Initialize_0_b) {
    int ex_index = 0;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    ASSERT_EQ(get_friend_deferred_providers(formulation).size(), 0);
}

/**
 * Simple test of get response in example 0.
 */
TEST_F(Bmi_Cpp_Multi_Array_Test, GetResponse_0_a) {
    int ex_index = 0;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    ASSERT_EQ(response, 00);
}

/**
 * Test of get response in example 0 after several iterations.
 */
TEST_F(Bmi_Cpp_Multi_Array_Test, GetResponse_0_b) {
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

/** Test to ensure value array passes from one module into the next */
TEST_F(Bmi_Cpp_Multi_Array_Test, Pass_Bmi_Array_0) {
    int ex_index = 0;

    Bmi_Multi_Formulation formulation(catchment_ids[ex_index], std::make_unique<CsvPerFeatureForcingProvider>(*forcing_params_examples[ex_index]), utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    formulation.get_response(0, 3600);
    
    std::vector<double> expected = {1000, 2000, 3000};
    auto data = get_friend_nested_var_values<models::bmi::Bmi_Cpp_Adapter, Bmi_Cpp_Formulation>(formulation, 1, "INPUT_VAR_3");
    ASSERT_EQ(data,  expected);
}

/**
 * Simple test of output for example 0.
 */
TEST_F(Bmi_Cpp_Multi_Array_Test, GetOutputLineForTimestep_0_a) {
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
TEST_F(Bmi_Cpp_Multi_Array_Test, GetOutputLineForTimestep_0_b) {
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


#endif // NGEN_BMI_CPP_LIB_TESTS_ACTIVE
#endif // NGEN_Bmi_Cpp_Multi_Array_Test_CPP
