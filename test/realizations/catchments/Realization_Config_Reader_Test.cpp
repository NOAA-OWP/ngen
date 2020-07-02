#include "gtest/gtest.h"
#include <Realization_Config_Reader.hpp>
#include <Realization_Config.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class Realization_Config_Reader_Test : public ::testing::Test {

    protected:


    Realization_Config_Reader_Test() {

    }

    ~Realization_Config_Reader_Test() override {

    }

    void SetUp() override {

    }

    void TearDown() override {

    }

};

const double EPSILON = 0.0000001;

const std::string EXAMPLE = "{ "
    "\"global\": { "
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
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0, "
                "1.0 "
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
            "\"file_pattern\": \".*{{ID}}.*.csv\", "
            "\"path\": \"./data/sugar_creek/forcing/\", "
            "\"start_time\": \"2015-12-01 00:00:00\", "
            "\"end_time\": \"2015-12-30 23:00:00\" "
        "} "
    "}, "
    "\"catchments\": { "
        "\"cat-88\": { "
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
                        "1.0, "
                        "1.0, "
                        "1.0, "
                        "1.0, "
                        "1.0, "
                        "1.0, "
                        "1.0, "
                        "1.0 "
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
                    "\"path\": \"./data/sugar_creek/forcing/cat-88_2015-12-01 00_00_00_2015-12-30 23_00_00.csv\", "
                    "\"start_time\": \"2015-12-01 00:00:00\", "
                    "\"end_time\": \"2015-12-30 23:00:00\" "
                "} "
        "}, "
        "\"cat-89\": { "
            "\"simple_lumped\": { "
                "\"sr\": [ "
                    "1.0, "
                    "1.0, "
                    "1.0 "
                "], "
                "\"storage\": 1.0, "
                "\"max_storage\": 1000.0, "
                "\"a\": 1.0, "
                "\"b\": 10.0, "
                "\"Ks\": 0.1, "
                "\"Kq\": 0.01, "
                "\"n\": 3, "
                "\"t\": 0 "
            "}, "
            "\"forcing\": { "
                "\"path\": \"./data/sugar_creek/forcing/cat-89_2015-12-01 00_00_00_2015-12-30 23_00_00.csv\", "
                "\"start_time\": \"2015-12-01 00:00:00\", "
                "\"end_time\": \"2015-12-30 23:00:00\" "
            "} "
        "} "
    "} "
"}";

TEST_F(Realization_Config_Reader_Test, basic_reading) {
    std::stringstream stream;
    stream << EXAMPLE;

    realization::Realization_Config_Reader reader = realization::load_reader(stream);
    ASSERT_TRUE(reader->is_empty());
    reader->read();

    ASSERT_EQ(reader->get_size(), 2);

    ASSERT_TRUE(reader->contains("cat-88"));
    ASSERT_TRUE(reader->contains("cat-89"));

    ASSERT_NE(reader->get_global_configuration(), nullptr);
}

TEST_F(Realization_Config_Reader_Test, global) {
    std::stringstream stream;
    stream << EXAMPLE;

    realization::Realization_Config_Reader reader = realization::load_reader(stream);
    ASSERT_TRUE(reader->is_empty());

    reader->read();

    realization::Realization_Config config = reader->get("cat-87");

    std::shared_ptr<realization::Tshirt_Realization> tshirt = config->get_tshirt();

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

TEST_F(Realization_Config_Reader_Test, cat88) {
    std::stringstream stream;
    stream << EXAMPLE;

    realization::Realization_Config_Reader reader = realization::load_reader(stream);
    ASSERT_TRUE(reader->is_empty());

    reader->read();

    realization::Realization_Config config = reader->get("cat-88");

    std::shared_ptr<realization::Tshirt_Realization> tshirt = config->get_tshirt();

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

TEST_F(Realization_Config_Reader_Test, cat89) {
    std::stringstream stream;
    stream << EXAMPLE;

    realization::Realization_Config_Reader reader = realization::load_reader(stream);
    ASSERT_TRUE(reader->is_empty());

    reader->read();

    realization::Realization_Config config = reader->get("cat-89");

    std::shared_ptr<Simple_Lumped_Model_Realization> simple_lumped = config->get_simple_lumped();

    double input_flux = 10.0;
    time_step_t timestep = 3600;

    pdm03_struct pdm_et_data;
    pdm_et_data.scaled_distribution_fn_shape_parameter = 1.3;
    pdm_et_data.vegetation_adjustment = 0.99;
    pdm_et_data.model_time_step = 0.0;
    pdm_et_data.max_height_soil_moisture_storerage_tank = 400.0;
    pdm_et_data.maximum_combined_contents = pdm_et_data.max_height_soil_moisture_storerage_tank / (1.0+pdm_et_data.scaled_distribution_fn_shape_parameter);

    double response = simple_lumped->get_response(input_flux, 0, timestep, &pdm_et_data);
    
    ASSERT_LT(std::abs(response - 0.0000108374), EPSILON);
}

TEST_F(Realization_Config_Reader_Test, iterate) {
    std::stringstream stream;
    stream << EXAMPLE;

    realization::Realization_Config_Reader reader = realization::load_reader(stream);
    ASSERT_TRUE(reader->is_empty());

    reader->read();

    ASSERT_EQ(reader->get_size(), 2);

    int config_index = 0;
    for(std::pair<std::string, realization::Realization_Config> pair : *reader) {
        config_index++;

        if (config_index == 1) {
            ASSERT_EQ(pair.second->get_id(), "cat-88");
        }
        else if (config_index == 2) {
            ASSERT_EQ(pair.second->get_id(), "cat-89");
        }
    }
}