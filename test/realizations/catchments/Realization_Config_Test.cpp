#include "gtest/gtest.h"
#include <Realization_Config.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <cmath>

const double EPSILON = 0.0000001;

const std::string EXAMPLE = "{ "
    "\"cat-89\": { "
            "\"tshirt\": { "
                "\"maxsmc\": 0.81, "
                "\"wltsmc\": 1.0, "
                "\"satdk\": 0.48, "
                "\"satpsi\": 0.1, "
                "\"slope\": 0.58, "
                "\"scaled_distribution_fn_shape_parameter\": 1.3, "
                "\"multiplier\": 1.0, "
                "\"alpha_fc\": 1.0, "
                "\"Klf\": 0.0000672, "
                "\"Kn\": 0.1, "
                "\"nash_n\": 8, "
                "\"Cgw\": 1.08, "
                "\"expon\": 6.0, "
                "\"max_groundwater_storage_meters\": 16.0, "
                "\"nash_storage\": [ "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0, "
                "   1.0 "
                "], "
                "\"soil_storage_meters\": 1.0, "
                "\"groundwater_storage_meters\": 1.0, "
                "\"timestep\": 3600 "
            "}, "
            "\"giuh\": { "
                "\"giuh_path\": \"./test/data/giuh/GIUH.json\", "
                "\"crosswalk_path\": \"./data/sugar_creek/crosswalk_subset.json\" "
            "}, "
            "\"forcing\": { "
                "\"path\": \"./data/sugar_creek/forcing/cat-89_2015-12-01 00:00:00_2015-12-30 23:00:00.csv\", "
                "\"start_time\": \"2015-12-01 00:00:00\", "
                "\"end_time\": \"2015-12-30 23:00:00\" "
            "} "
    "} "
"}";

class Realization_Config_Test : public ::testing::Test {

    protected:


    Realization_Config_Test() {

    }

    ~Realization_Config_Test() override {

    }

    void SetUp() override {

    }

    void TearDown() override {

    }

};

TEST_F(Realization_Config_Test, basic_reading) {
    std::stringstream stream;
    stream << EXAMPLE;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    realization::Realization_Config config = NULL;

    for (auto &child : tree) {
        config = realization::get_realizationconfig(child.first, child.second);
    }

    ASSERT_EQ(config->get_id(), "cat-89");

    ASSERT_TRUE(config->has_forcing());
    ASSERT_TRUE(config->has_giuh());

    ASSERT_EQ(config->get_realization_type(), realization::Realization_Type::TSHIRT);

    ASSERT_TRUE(config->has_option("maxsmc"));
    ASSERT_EQ(config->get_option("maxsmc").as_real_number(), 0.81);

    ASSERT_TRUE(config->has_option("wltsmc"));
    ASSERT_EQ(config->get_option("wltsmc").as_real_number(), 1.0);

    ASSERT_TRUE(config->has_option("satdk"));
    ASSERT_EQ(config->get_option("satdk").as_real_number(), 0.48);

    ASSERT_TRUE(config->has_option("satpsi"));
    ASSERT_EQ(config->get_option("satpsi").as_real_number(), 0.1);

    ASSERT_TRUE(config->has_option("slope"));
    ASSERT_EQ(config->get_option("slope").as_real_number(), 0.58);

    ASSERT_TRUE(config->has_option("scaled_distribution_fn_shape_parameter"));
    ASSERT_EQ(config->get_option("scaled_distribution_fn_shape_parameter").as_real_number(), 1.3);

    ASSERT_TRUE(config->has_option("multiplier"));
    ASSERT_EQ(config->get_option("multiplier").as_real_number(), 1.0);

    ASSERT_TRUE(config->has_option("alpha_fc"));
    ASSERT_EQ(config->get_option("alpha_fc").as_real_number(), 1.0);

    ASSERT_TRUE(config->has_option("Klf"));
    ASSERT_EQ(config->get_option("Klf").as_real_number(), 0.0000672);

    ASSERT_TRUE(config->has_option("Kn"));
    ASSERT_EQ(config->get_option("Kn").as_real_number(), 0.1);

    ASSERT_TRUE(config->has_option("nash_n"));
    ASSERT_EQ(config->get_option("nash_n").as_natural_number(), 8);

    ASSERT_TRUE(config->has_option("Cgw"));
    ASSERT_EQ(config->get_option("Cgw").as_real_number(), 1.08);

    ASSERT_TRUE(config->has_option("expon"));
    ASSERT_EQ(config->get_option("expon").as_real_number(), 6.0);

    ASSERT_TRUE(config->has_option("max_groundwater_storage_meters"));
    ASSERT_EQ(config->get_option("max_groundwater_storage_meters").as_real_number(), 16.0);

    ASSERT_TRUE(config->has_option("nash_storage"));

    std::vector<double> nash_storage = config->get_option("nash_storage").as_real_vector();

    ASSERT_EQ(nash_storage.size(), 8);
    ASSERT_EQ(nash_storage[0], 1.0);
    ASSERT_EQ(nash_storage[1], 1.0);
    ASSERT_EQ(nash_storage[2], 1.0);
    ASSERT_EQ(nash_storage[3], 1.0);
    ASSERT_EQ(nash_storage[4], 1.0);
    ASSERT_EQ(nash_storage[5], 1.0);
    ASSERT_EQ(nash_storage[6], 1.0);
    ASSERT_EQ(nash_storage[7], 1.0);

    ASSERT_TRUE(config->has_option("soil_storage_meters"));
    ASSERT_EQ(config->get_option("soil_storage_meters").as_real_number(), 1.0);

    ASSERT_TRUE(config->has_option("groundwater_storage_meters"));
    ASSERT_EQ(config->get_option("groundwater_storage_meters").as_real_number(), 1.0);

    ASSERT_TRUE(config->has_option("timestep"));
    ASSERT_EQ(config->get_option("timestep").as_natural_number(), 3600);
}

TEST_F(Realization_Config_Test, forcing) {
    std::stringstream stream;
    stream << EXAMPLE;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    realization::Realization_Config config = NULL;

    for (auto &child : tree) {
        config = realization::get_realizationconfig(child.first, child.second);
    }

    forcing_params params = config->get_forcing_parameters();

    ASSERT_EQ(params.path, "./data/sugar_creek/forcing/cat-89_2015-12-01 00:00:00_2015-12-30 23:00:00.csv");
    ASSERT_EQ(params.start_time, "2015-12-01 00:00:00");
    ASSERT_EQ(params.end_time, "2015-12-30 23:00:00");
}

TEST_F(Realization_Config_Test, tshirt) {
    std::stringstream stream;
    stream << EXAMPLE;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    realization::Realization_Config config = NULL;

    for (auto &child : tree) {
        config = realization::get_realizationconfig(child.first, child.second);
    }

    std::unique_ptr<realization::Tshirt_Realization> tshirt = config->get_tshirt();

    double input_flux = 10.0;
    time_step_t timestep = 3600;

    pdm03_struct pdm_et_data;
    pdm_et_data.scaled_distribution_fn_shape_parameter = 1.3;
    pdm_et_data.vegetation_adjustment = 0.99;
    pdm_et_data.model_time_step = 0.0;
    pdm_et_data.max_height_soil_moisture_storerage_tank = 400.0;
    pdm_et_data.maximum_combined_contents = pdm_et_data.max_height_soil_moisture_storerage_tank / (1.0+pdm_et_data.scaled_distribution_fn_shape_parameter);

    double response = tshirt->get_response(input_flux, 0, timestep, &pdm_et_data);
    ASSERT_LT(std::abs(response - 0.00277778), EPSILON);
}