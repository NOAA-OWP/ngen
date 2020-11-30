#include "gtest/gtest.h"
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "Bmi_C_Formulation.hpp"
#include "Formulation_Manager.hpp"
#include "FileChecker.h"
#include "Forcing.h"

using namespace realization;

class Bmi_C_Formulation_Test : public ::testing::Test {
protected:

    void SetUp() override;

    void TearDown() override;

    static std::string find_file(std::vector<std::string> dir_opts, const std::string& basename);

    std::vector<std::string> forcing_dir_opts;
    std::vector<std::string> bmi_init_cfg_dir_opts;

#define EX_COUNT 1

    std::vector<std::string> config_json;
    std::vector<std::string> catchment_ids;
    std::vector<std::string> model_type_name;
    std::vector<std::string> forcing_file;
    std::vector<std::string> init_config;
    std::vector<std::string> main_output_variable;
    std::vector<bool> uses_forcing_file;
    std::vector<std::shared_ptr<forcing_params>> forcing_params_examples;
    std::vector<geojson::GeoJSON> config_properties;
    std::vector<boost::property_tree::ptree> config_prop_ptree;

};

void Bmi_C_Formulation_Test::SetUp() {
    Test::SetUp();

    forcing_dir_opts = {"./data/forcing/", "../data/forcing/", "../../data/forcing/"};
    bmi_init_cfg_dir_opts = {"./test/data/bmi/c/cfe/", "../test/data/bmi/c/cfe/", "../../test/data/bmi/c/cfe/"};

    config_json = std::vector<std::string>(EX_COUNT);
    catchment_ids = std::vector<std::string>(EX_COUNT);
    model_type_name = std::vector<std::string>(EX_COUNT);
    forcing_file = std::vector<std::string>(EX_COUNT);
    init_config = std::vector<std::string>(EX_COUNT);
    main_output_variable  = std::vector<std::string>(EX_COUNT);
    uses_forcing_file = std::vector<bool>(EX_COUNT);
    forcing_params_examples = std::vector<std::shared_ptr<forcing_params>>(EX_COUNT);
    config_properties = std::vector<geojson::GeoJSON>(EX_COUNT);
    config_prop_ptree = std::vector<boost::property_tree::ptree>(EX_COUNT);

    /* Set up the basic/explicit example index details in the arrays */
    catchment_ids[0] = "cat-87";
    model_type_name[0] = "bmi_c_cfe";
    forcing_file[0] = find_file(forcing_dir_opts, "cat-87_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
    init_config[0] = find_file(bmi_init_cfg_dir_opts, "cat_87_bmi_config.txt");
    main_output_variable[0] = "Q_OUT";
    uses_forcing_file[0] = true;

    /* Set up the derived example details */
    for (int i = 0; i < EX_COUNT; i++) {
        std::shared_ptr<forcing_params> params = std::make_shared<forcing_params>(
                forcing_params(forcing_file[i], "2015-12-01 00:00:00", "2015-12-01 23:00:00"));
        forcing_params_examples[i] = params;
        config_json[i] = "{"
                         "    \"global\": {},"
                         "    \"catchments\": {"
                         "        \"" + catchment_ids[i] + "\": {"
                         "            \"bmi_c\": {"
                         "                \"model_type_name\": \"" + model_type_name[i] + "\","
                         "                \"forcing_file\": \"" + forcing_file[i] + "\","
                         "                \"init_config\": \"" + init_config[i] + "\","
                         "                \"main_output_variable\": \"" + main_output_variable[i] + "\","
                         "                \"uses_forcing_file\": " + (uses_forcing_file[i] ? "true" : "false") + ""
                         "            },"
                         "            \"forcing\": { \"path\": \"" + forcing_file[i] + "\"}"
                         "        }"
                         "    }"
                         "}";

        std::stringstream stream;
        stream << config_json[i];

        boost::property_tree::ptree loaded_tree;
        boost::property_tree::json_parser::read_json(stream, loaded_tree);
        config_prop_ptree[i] = loaded_tree.get_child("catchments").get_child(catchment_ids[i]).get_child("bmi_c");
    }
}

std::string Bmi_C_Formulation_Test::find_file(std::vector<std::string> dir_opts, const std::string& basename) {
    std::vector<std::string> file_opts(dir_opts.size());
    for (int i = 0; i < dir_opts.size(); ++i)
        file_opts[i] = dir_opts[i] + basename;
    return utils::FileChecker::find_first_readable(file_opts);
}

void Bmi_C_Formulation_Test::TearDown() {
    Test::TearDown();
}

/** Simple test to make sure the model initializes. */
TEST_F(Bmi_C_Formulation_Test, Initialize_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);
}

/** Simple test of get response. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    // TODO: val seems to be this for now ... do something but account for error bound
    ASSERT_EQ(response, 0.19108473440217114);
}

/** Test of get response after several iterations. */
TEST_F(Bmi_C_Formulation_Test, GetResponse_0_b) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response;
    for (int i = 0; i < 720; i++) {
        response = formulation.get_response(i, 3600);
    }
    // TODO: val seems to be this for now ... do something but account for error bound
    ASSERT_EQ(response, 0.00067750933548268489);
}

/** Simple test of output. */
TEST_F(Bmi_C_Formulation_Test, GetOutputLineForTimestep_0_a) {
    int ex_index = 0;

    Bmi_C_Formulation formulation(catchment_ids[ex_index], *forcing_params_examples[ex_index], utils::StreamHandler());
    formulation.create_formulation(config_prop_ptree[ex_index]);

    double response = formulation.get_response(0, 3600);
    std::string output = formulation.get_output_line_for_timestep(0, ",");
    // TODO: verify the results are correct
    ASSERT_EQ(output, "0.000000,0.000000,0.000000,0.191085,0.191085");

}
