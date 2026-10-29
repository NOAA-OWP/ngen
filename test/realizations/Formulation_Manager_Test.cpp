#include "Bmi_Testing_Util.hpp"
#include "Bmi_Cpp_Formulation.hpp"
#include "DataProvider.hpp"
#include "DataProviderSelectors.hpp"
#include "FileChecker.h"
#include "gtest/gtest.h"
#include <Formulation_Manager.hpp>
#include <Catchment_Formulation.hpp>

#include <features/Features.hpp>
#include <JSONGeometry.hpp>
#include <JSONProperty.hpp>
#include <HY_Features.hpp>
#include "realizations/config/catchment_output.hpp"

#include <iostream>
#include <memory>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sys/stat.h>
#include <optional>

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>

#include <AorcForcing.hpp>
#include <Bmi_Formulation.hpp>

class Formulation_Manager_Test : public ::testing::Test {

    protected:


    Formulation_Manager_Test() {

    }

    ~Formulation_Manager_Test() override {

    }

    void SetUp() override {

    }

    void TearDown() override {

    }

    void add_feature(std::string id)
    {
      geojson::three_dimensional_coordinates three_dimensions {
          {
              {1.0, 2.0},
              {3.0, 4.0},
              {5.0, 6.0}
          },
          {
              {7.0, 8.0},
              {9.0, 10.0},
              {11.0, 12.0}
          }
      };
      std::vector<double> bounding_box{1.0, 2.0};
      geojson::PropertyMap properties{
          //{"prop_0", geojson::JSONProperty("prop_0", 0)},
          //{"prop_1", geojson::JSONProperty("prop_1", "1")},
          //{"prop_2", geojson::JSONProperty("prop_2", false)},
          //{"prop_3", geojson::JSONProperty("prop_3", 2.0)}
      };

      geojson::Feature feature = std::make_shared<geojson::PolygonFeature>(geojson::PolygonFeature(
        geojson::polygon(three_dimensions),
        id,
        properties
        //bounding_box
      ));

      fabric->add_feature(feature);
    }

    void add_feature(const std::string& id, geojson::PropertyMap properties)
    {
      geojson::three_dimensional_coordinates three_dimensions {
          {
              {1.0, 2.0},
              {3.0, 4.0},
              {5.0, 6.0}
          },
          {
              {7.0, 8.0},
              {9.0, 10.0},
              {11.0, 12.0}
          }
      };

      geojson::Feature feature = std::make_shared<geojson::PolygonFeature>(geojson::PolygonFeature(
        geojson::polygon(three_dimensions),
        id,
        properties
      ));

      fabric->add_feature(feature);
    }

    std::vector<std::string> path_options = {
            "",
            "../",
            "../../",
            "./test/",
            "../test/",
            "../../test/"

    };

    void replace_paths(std::string& input, const std::string& pattern, const std::string& replacement)
    {
        std::vector<std::string> v{path_options.size()};
        for(unsigned int i = 0; i < path_options.size(); i++)
            v[i] = path_options[i] + replacement;
        
        const std::string dir = utils::FileChecker::find_first_readable(v);
        if (dir == "") {
            // std::cerr << "Can't find any of:\n";
            // for (const auto& s : v)
            //   std::cerr << "  - " << s << '\n';
            return;
        }

        boost::replace_all(input, pattern, dir);
    }

    std::string fix_paths(std::string json)
    {
        std::vector<std::string> forcing_paths = {
                "./data/forcing/cat-52_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "./data/forcing/cat-67_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "./data/forcing/cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                "./data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv"
        };
        std::vector<std::string> v = {};
        for(unsigned int i = 0; i < path_options.size(); i++){
                v.push_back( path_options[i] + "data/forcing" );
        }
        std::string dir = utils::FileChecker::find_first_readable(v);
        if(dir != ""){
                std::string remove = "\"./data/forcing/\"";
                std::string replace = "\""+dir+"\"";
                //std::cerr<<"TRYING TO REPLACE DIRECTORY... "<<remove<<" -> "<<replace<<std::endl;
                boost::replace_all(json, remove , replace);
        }
      
        //BMI_C_INIT_DIR_PATH
        replace_paths(json, "{{BMI_C_INIT_DIR_PATH}}", "data/bmi/test_bmi_c");
        //BMI_CPP_INIT_DIR_PATH
        replace_paths(json, "{{BMI_CPP_INIT_DIR_PATH}}", "data/bmi/test_bmi_cpp");
        //EXTERN_DIR_PATH
        replace_paths(json, "{{EXTERN_LIB_DIR_PATH}}", "extern/test_bmi_cpp/cmake_build/");
        
        for (unsigned int i = 0; i < forcing_paths.size(); i++) {
          if(json.find(forcing_paths[i]) == std::string::npos){
            continue;
          }
          std::vector<std::string> v = {};
          for (unsigned int j = 0; j < path_options.size(); j++) {
            v.push_back(path_options[j] + forcing_paths[i]);
          }
          std::string right_path = utils::FileChecker::find_first_readable(v);
          if(right_path != forcing_paths[i]){
            std::cerr<<"Replacing "<<forcing_paths[i]<<" with "<<right_path<<std::endl;
            boost::replace_all(json, forcing_paths[i] , right_path);
          }
        }
        return json;
    }

    /**
     * Parse a realization config from a stream and build its simulation time parameters.
     *
     * Reads the JSON in @p stream into @p realization_config (which can then be used to
     * construct a @c Formulation_Manager) and derives the simulation time parameters from
     * the config's required "time" section.
     *
     * @param stream Stream holding the (path-fixed) realization config JSON.
     * @param realization_config Property tree populated with the parsed config (output parameter).
     * @return The simulation time parameters parsed from the config's "time" section.
     * @throws std::runtime_error If the config has no "time" section.
     */
    simulation_time_params get_time_from_load_realization_config(std::stringstream& stream,
                                                                 boost::property_tree::ptree& realization_config)
    {
        boost::property_tree::json_parser::read_json(stream, realization_config);

        boost::optional<boost::property_tree::ptree&> possible_simulation_time =
            realization_config.get_child_optional("time");
        if (!possible_simulation_time) {
            throw std::runtime_error("ERROR: No simulation time period defined.");
        }

        return realization::config::Time(*possible_simulation_time).make_params();
    }

    geojson::GeoJSON fabric = std::make_shared<geojson::FeatureCollection>();

};

/**
 * A child class for the Formulation_Manager used specifically to test stock behavior in
 * get_forcing_params
 */
class ForcingLookupManager : public Formulation_Manager {
    public:
        ForcingLookupManager() : Formulation_Manager(boost::property_tree::ptree{}) {}
        using Formulation_Manager::get_forcing_params;
};

/**
 * A structured test used to maintain test consistency for Formulation_Manager::get_forcing_params
 */
class FormulationManagerForcingTest : public ::testing::Test {
public:
    static std::string get_provider_name() {
        return "CsvPerFeature";
    }
protected:
    /**
     * A DTO used to handle input parameters to FormulationManagerForcingTest::make_properties
     *
     * `path` is not set by default because it needs to be based on instance specific details
     */
    struct MakePropertiesPropertyArguments {
        std::string pattern = R"({{id}}\.csv)";
        std::optional<std::string> path;
    };

    /**
     * Put all resources into place for testing
     */
    void SetUp() override {
        // This will be used to create the temporary directory name. It MUST end in 'XXXXXX'
        const std::string basename = "ngen-get_forcing_params-XXXXXX";
        std::string pattern = (
            std::filesystem::temp_directory_path() / basename
        ).string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');

        // Create a temporary directory to hold data used for these tests
        const char* directory = ::mkdtemp(buffer.data());
        ASSERT_NE(directory, nullptr);
        this->root_ = directory;
    }

    /**
     * Remove resources that were created for testing
     */
    void TearDown() override {
        if (!this->root_.empty()) {
            std::error_code error;
            std::filesystem::remove_all(this->root_, error);
            EXPECT_FALSE(error) << error.message();
        }
    }

    /**
     * Create a regular file with very basic data in it with the given name off of the test
     * directory's root
     * @param name The name of the file to create in the test directory
     */
    void create_file(const std::string& name) const {
        std::ofstream file(this->root_ / name);
        ASSERT_TRUE(file.is_open());
        file << "forcing data\n";
        file.close();
        ASSERT_TRUE(file.good());
    }

    /**
     * Create some basic properties for tests
     * @param args A keyword argument DTO for setting values
     * @return A new propertymap to feed into the functions being tested
     */
    geojson::PropertyMap make_properties(
        MakePropertiesPropertyArguments args = {R"({{id}}\.csv)"}
    ) const {
        return {
            {
                "path",
                {"path", args.path.value_or(this->root_.string())}
            },
            {
                "provider",
                {"provider", FormulationManagerForcingTest::get_provider_name()}
            },
            {
                "file_pattern",
                {"file_pattern", args.pattern}
            }
        };
    }

    /**
     * Call the 'get_forcing_params' function
     * @param forcing_prop_map Optional forcing properties to use to search - uses the default for
     * testing if not provided
     * @param identifier Optional location identifier to use to search - uses the default for
     * testing if not provided
     * @return Found forcing parameters
     */
    forcing_params lookup(
        const geojson::PropertyMap& forcing_prop_map,
        const std::string& identifier = "cat-27"
    ) {
        return this->manager_.get_forcing_params(forcing_prop_map, identifier, this->time_);
    }

    std::filesystem::path root_;
    ForcingLookupManager manager_;
    simulation_time_params time_{
        "2015-12-01 00:00:00",
        "2015-12-02 00:00:00",
        3600
    };
};

/**
 * Make sure Formulation_Manager::get_forcing_params runs without error
 * if there is not a provided file_pattern
 *
 * No search is performed if there's no pattern, so there shouldn't be much if anything to fail on
 */
TEST_F(FormulationManagerForcingTest, WithoutPatternPassesThroughPathProviderAndTimes) {
    auto config = this->make_properties();
    config.erase("file_pattern");

    // With no pattern, even a nonexistent path must be passed through unchanged.
    const auto path = (this->root_ / "not-created.csv").string();
    config.at("path") = {"path", path};

    // "Find" the forcing_params. This should work no problem with the given parameters
    const auto result = this->lookup(config);
    EXPECT_EQ(
        result.path,
        path
    );
    EXPECT_EQ(
        result.provider,
        FormulationManagerForcingTest::get_provider_name()
    );
    EXPECT_EQ(
        result.start_time,
        this->time_.start_time
    );
    EXPECT_EQ(
        result.end_time,
        this->time_.end_time
    );
}

/**
 * Prove that Formulation_Manager::get_forcing_params functions without error if neither a path
 * nor file_pattern are provided
 */
TEST_F(FormulationManagerForcingTest, WithoutPatternAllowsMissingPathAndProvider) {
    // Lookup forcing_params based on nothing but the identifier (implicit)
    const auto result = this->lookup({});
    EXPECT_TRUE(result.path.empty());
    EXPECT_TRUE(result.provider.empty());
    EXPECT_EQ(
        result.start_time,
        this->time_.start_time
    );
    EXPECT_EQ(
        result.end_time,
        this->time_.end_time
    );
}

/**
 * Test to make sure that files may be found by Formulation_Manager::get_forcing_params
 * regardless of whether the root path ends with a slash or not
 */
TEST_F(FormulationManagerForcingTest, FindsRegularFileWithOrWithoutTrailingSlash) {
    // Create several files to search for
    //  Only 1 file should be found: cat-27.csv
    //  The default id is 'cat-27' and the default file pattern is '{{id}}\.csv' (becoming 'cat-27\.csv')
    //      - cat-270.csv should be skipped because it doesn't match the pattern
    //      - cat-27.csv should match because it matches the pattern
    //      - cat-27.csv.backup should be skipped because cat-27.csv is found first
    this->create_file("cat-270.csv");
    this->create_file("cat-27.csv");
    this->create_file("cat-27.csv.backup");

    const std::vector<std::string> suffixes = {"", "/"};
    for (const std::string& suffix : suffixes) {
        SCOPED_TRACE("Searching for when the suffix of the root path is: " + suffix);

        geojson::PropertyMap config = make_properties({
            .path = this->root_.string() + suffix
        });

        const forcing_params result = this->lookup(config);
        EXPECT_EQ(
            result.path,
            (this->root_ / "cat-27.csv").string()
        );
        EXPECT_EQ(
            result.provider,
            FormulationManagerForcingTest::get_provider_name()
        );
        EXPECT_EQ(
            result.start_time,
            this->time_.start_time
        );
        EXPECT_EQ(
            result.end_time,
            this->time_.end_time
        );
    }
}

/**
 * Ensure that Formulation_Manager::get_forcing_params can find the file even if the path doesn't
 * hold a space for a token
 */
TEST_F(FormulationManagerForcingTest, PatternWithoutTokenStillUsesRegex) {
    this->create_file("weather_2015.csv");
    const forcing_params found_parameters = this->lookup(
        make_properties({.pattern = R"(weather_[0-9]+\.csv)"})
    );
    EXPECT_EQ(
        found_parameters.path,
        (this->root_ / "weather_2015.csv").string()
    );
}

/**
 * Ensure that Formulation_Manager::get_forcing_params can correctly find files that have the
 * token multiple times in the path or name
 */
TEST_F(FormulationManagerForcingTest, ReplacesEveryIdentifierToken) {
    this->create_file("cat-27_cat-27.csv");
    const forcing_params found_parameters = this->lookup(
        make_properties({.pattern = R"({{id}}_{{id}}\.csv)"})
    );
    EXPECT_EQ(
        found_parameters.path,
        (this->root_ / "cat-27_cat-27.csv").string()
    );
}

/**
 * Ensure that Formulation_Manager::get_forcing_params can find the input even if the identifier
 * has characters that contain regular expression symbols
 */
TEST_F(FormulationManagerForcingTest, TreatsRegexMetacharactersInIdentifiersLiterally) {
    const std::vector<std::string> identifier_variations{
        "cat.27",
        "cat+27",
        "cat[27]",
        "cat(27)",
        "cat$27",
        "cat^27",
        "cat|27",
        "cat*27",
        "cat?27",
        "cat{27}",
        R"(cat\27)",
        "{{id}}"
    };
    // Include every escaped metacharacter, including a literal backslash and token text.
    for (const std::string& identifier : identifier_variations) {
        const std::string new_filename{identifier + ".csv"};
        SCOPED_TRACE("Testing to see if the following identifier, '" + identifier + "', and properly finds: " + new_filename);
        this->create_file(new_filename);
        EXPECT_EQ(
            this->lookup(this->make_properties(), identifier).path,
            (this->root_ / (identifier + ".csv")).string()
        );
    }
}

/**
 * Ensure that Formulation_Manager::get_forcing_params doesn't match on incorrect files
 * just because their identifiers have regular expression characters that would naturally match
 */
TEST_F(FormulationManagerForcingTest, EscapedIdentifiersDoNotMatchRegexLookalikes) {
    this->create_file("catX27.csv");
    this->create_file("cattt27.csv");
    this->create_file("cat2.csv");

    const std::vector<string> identifiers_to_test = {
        "cat.27",
        "cat+27",
        "cat[27]"
    };

    for (const std::string& identifier : identifiers_to_test) {
        SCOPED_TRACE("Test for: " + identifier);
        EXPECT_THROW(
            this->lookup(this->make_properties(), identifier),
            std::runtime_error
        );
    }
}

/**
 * Ensure that Formulation_Manager::get_forcing_params can identify input even if they are symlinks
 */
TEST_F(FormulationManagerForcingTest, FollowsSymlinkToRegularFile) {
    this->create_file("actual-forcing.txt");
    std::filesystem::create_symlink(
        "actual-forcing.txt",
        this->root_ / "cat-27.csv"
    );
    const forcing_params created_params = this->lookup(this->make_properties());
    EXPECT_EQ(
        created_params.path,
        (this->root_ / "cat-27.csv").string()
    );
}

/**
 * Ensure that Formulation_Manager::get_forcing_params doesn't accidentally match on directories
 */
TEST_F(FormulationManagerForcingTest, SkipsMatchingDirectoriesAndDirectorySymlinks) {
    std::filesystem::create_directory(this->root_ / "cat-27-directory.csv");
    std::filesystem::create_directory_symlink(
        "cat-27-directory.csv",
        this->root_ / "cat-27-link.csv"
    );
    this->create_file("cat-27-data.csv");
    const forcing_params generated_params = this->lookup(
        this->make_properties({.pattern = R"({{id}}-.*\.csv)"})
    );
    EXPECT_EQ(
        generated_params.path,
        (this->root_ / "cat-27-data.csv").string()
    );
}

/**
 * Ensure that Formulation_Manager::get_forcing_params doesn't match on non-file objects
 * when the pattern is overly broad.
 */
TEST_F(FormulationManagerForcingTest, BroadPatternSkipsDotEntriesAndOtherNonFiles) {
    std::filesystem::create_directory(this->root_ / "directory");
    std::filesystem::create_directory_symlink("directory", this->root_ / "directory-link");
    ASSERT_EQ(
        ::mkfifo((this->root_ / "pipe").c_str(), 0600),
        0
    );

    const geojson::PropertyMap search_properties = this->make_properties({
        .pattern = ".*"
    });
    EXPECT_THROW(
        this->lookup(search_properties),
        std::runtime_error
    );
    this->create_file("forcing.csv");
    EXPECT_EQ(
        this->lookup(this->make_properties({.pattern = ".*"})).path,
        (this->root_ / "forcing.csv").string()
    );
}

/**
 * Ensure that Formulation_Manager::get_forcing_params correctly reports why it didn't
 * load objects that matched the pattern
 */
TEST_F(FormulationManagerForcingTest, ReportsMatchingNonFilesWhenNoRegularFileExists) {
    std::filesystem::create_directory(this->root_ / "cat-27-directory.csv");
    std::filesystem::create_directory_symlink(
        "cat-27-directory.csv",
        this->root_ / "cat-27-link.csv"
    );
    const auto config = this->make_properties({.pattern = R"({{id}}-.*\.csv)"});
    try {
        this->lookup(config);
        FAIL() << "Lookup should fail when every matching entry is a non-file";
    }
    catch (const std::runtime_error& error) {
        const std::string message = error.what();
        EXPECT_NE(
            message.find(this->root_.string()),
            std::string::npos
        );
        EXPECT_NE(
            message.find("cat-27"),
            std::string::npos
        );
        EXPECT_NE(
            message.find(R"(cat-27-.*\.csv)"),
            std::string::npos
        );
        EXPECT_NE(
            message.find("2 actual matches"),
            std::string::npos
        );
    }
}

/**
 * Ensure that Formulation_Manager::get_forcing_params fails when the path is empty and when
 * nothing matches
 */
TEST_F(FormulationManagerForcingTest, RejectsEmptyDirectoryAndNonmatchingFiles) {
    EXPECT_THROW(this->lookup(this->make_properties()), std::runtime_error);
    this->create_file("cat-270.csv");
    this->create_file("cat-27Xcsv");
    this->create_file("cat-27.csv.backup");
    EXPECT_THROW(
        this->lookup(this->make_properties()),
        std::runtime_error
    );
}

/**
 * Ensure that Formulation_Manager::get_forcing_params fails when the path is an empty value and
 * when the path property just isn't present.
 */
TEST_F(FormulationManagerForcingTest, PatternRequiresNonemptyPath) {
    auto config = this->make_properties({.path = ""});
    config.at("path") = geojson::JSONProperty("path", "");
    EXPECT_THROW(
        this->lookup(config),
        std::runtime_error
    );
    config.erase("path");
    EXPECT_THROW(
        this->lookup(config),
        std::runtime_error
    );
}

/**
 * Make sure that Formulation_Manager::get_forcing_params fails when looking in a nonexistent path
 * and when the path is a file, not a directory
 */
TEST_F(FormulationManagerForcingTest, RejectsMissingDirectoryAndRegularFileAsSearchPath) {
    auto config = this->make_properties({
        .path = (this->root_ / "missing").string()
    });

    EXPECT_THROW(
        this->lookup(config),
        std::runtime_error
    );

    this->create_file("not-a-directory");
    config.at("path") = {
        "path",
        (this->root_ / "not-a-directory").string()
    };
    EXPECT_THROW(
        this->lookup(config),
        std::runtime_error
    );
}

/**
 * Test to make sure Formulation_Manager::get_forcing_params fails when the given pattern
 * is invalid
 */
TEST_F(FormulationManagerForcingTest, RejectsMalformedRegex) {
    EXPECT_THROW(
        this->lookup(this->make_properties({.pattern = "["})),
        std::regex_error
    );
}

/**
 * Test to make sure that Formulation_Manager::get_forcing_params fails on a symlink that
 * matches but doesn't point at anything
 */
TEST_F(FormulationManagerForcingTest, ReportsStatFailureForDanglingSymlink) {
    std::filesystem::create_symlink("missing.csv", this->root_ / "cat-27.csv");
    EXPECT_THROW(
        this->lookup(this->make_properties()),
        std::runtime_error
    );
}

/**
 * Test to make sure that Formulation_Manager::get_forcing_params works as expected when
 * called repeatedly
 */
TEST_F(FormulationManagerForcingTest, RepeatedSuccessfulAndUnsuccessfulLookups) {
    this->create_file("cat-27.csv");
    const auto config = this->make_properties();
    for (int iteration = 0; iteration < 512; ++iteration) {
        SCOPED_TRACE("Searching for cat-27.csv, iteration: " + std::to_string(iteration));
        EXPECT_EQ(
            this->lookup(config).path,
            (this->root_ / "cat-27.csv").string()
        );
        EXPECT_THROW(
            this->lookup(config, "cat-missing"),
            std::runtime_error
        );
    }
}

const double EPSILON = 0.0000001;

const std::string EXAMPLE_1 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\":\"bmi_c++\","
          "\"params\": {"
            "\"model_type_name\": \"test_bmi_cpp\","
            "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
            "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
            "\"main_output_variable\": \"OUTPUT_VAR_2\","
            "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
              "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
              "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "},"
            "\"create_function\": \"bmi_model_create\","
            "\"destroy_function\": \"bmi_model_destroy\","
            "\"uses_forcing_file\": false"
          "} "
        "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{id}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"disable_catchment_output\": true,"
    "\"catchments\": { "
        "\"cat-52\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "}, "
        "\"cat-67\": { "
        "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "} "
    "} "
"}";
const std::string EXAMPLE_2 = "{ "
    "\"output_root\": \"./output_dir/\","
    "\"global\": { "
      "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{ID}}.*.csv\", " // DIFF from Ex.1
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"catchments\": { "
        "\"cat-52\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "}, "
        "\"cat-67\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "} "
    "} "
"}";

const std::string EXAMPLE_3 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{ID}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"catchments\": { "
        "\"cat-52\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "}, "
        "\"cat-67\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
        "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "} "
    "} "
"}";

const std::string EXAMPLE_4 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
    "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{ID}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"catchments\": { "
        "\"cat-27115\": { " // DIFF from Ex.1+2+3
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { " // BLOCK DIFF from Ex.1+2+3
              "\"path\": \"./data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv\" "
          "} "
        "}, "
        "\"cat-67\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
        "], "
          "\"forcing\": { " // BLOCK DIFF from Ex.1+2+3
              "\"path\": \"./data/forcing/cat-67_2015-12-01 00_00_00_2015-12-30 23_00_00.csv\" "
          "} "
        "} "
    "} "
"}";

/**
 * Configuration for model_params parsing at levels:
 * - global single-bmi
 * - catchment-specific single-bmi
 * - catchment-specific multi-bmi
 */
const std::string EXAMPLE_5_a =
"{"
"    \"global\": {"
"        \"formulations\": ["
"            {"
"                \"name\": \"bmi_c++\","
"                \"params\": {"
"                    \"model_type_name\": \"test_bmi_c++\","
"                    \"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
"                    \"init_config\": \"{{BMI_CPP_INIT_DIR_PATH}}/test_bmi_cpp_config_2.txt\","
"                    \"allow_exceed_end_time\": true,"
"                    \"main_output_variable\": \"OUTPUT_VAR_4\","
"                    \"uses_forcing_file\": false,"
"                    \"model_params\": {"
"                        \"MODEL_VAR_1\": {"
"                            \"source\": \"hydrofabric\","
"                            \"from\": \"pi\""
"                        },"
"                        \"MODEL_VAR_2\": {"
"                            \"source\": \"hydrofabric\""
"                        }"
"                    },"
"                    \"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": {"
"                        \"INPUT_VAR_1\": \"APCP_surface\","
"                        \"INPUT_VAR_2\": \"APCP_surface\""
"                    }"
"                }"
"            }"
"        ],"
"        \"forcing\": { "
"            \"file_pattern\": \".*{{id}}.*.csv\","
"            \"path\": \"./data/forcing/\","
"            \"provider\": \"CsvPerFeature\""
"        }"
"    },"
"    \"time\": {"
"        \"start_time\": \"2015-12-01 00:00:00\","
"        \"end_time\": \"2015-12-30 23:00:00\","
"        \"output_interval\": 3600"
"    },"
"    \"catchments\": {"
"        \"cat-67\": {"
"            \"formulations\": ["
"                {"
"                    \"name\": \"bmi_c++\","
"                    \"params\": {"
"                        \"model_type_name\": \"test_bmi_c++\","
"                        \"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
"                        \"init_config\": \"{{BMI_CPP_INIT_DIR_PATH}}/test_bmi_cpp_config_2.txt\","
"                        \"allow_exceed_end_time\": true,"
"                        \"main_output_variable\": \"OUTPUT_VAR_4\","
"                        \"uses_forcing_file\": false,"
"                        \"model_params\": {"
"                            \"MODEL_VAR_1\": {"
"                                \"source\": \"hydrofabric\","
"                                \"from\": \"n\""
"                            },"
"                            \"MODEL_VAR_2\": {"
"                                \"source\": \"hydrofabric\""
"                            }"
"                        },"
"                        \"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": {"
"                            \"INPUT_VAR_1\": \"APCP_surface\","
"                            \"INPUT_VAR_2\": \"APCP_surface\""
"                        }"
"                    }"
"                }"
"            ],"
"            \"forcing\": {"
"                \"path\": \"./data/forcing/cat-67_2015-12-01 00_00_00_2015-12-30 23_00_00.csv\""
"            }"
"        },"
"        \"cat-27115\": {"
"            \"formulations\": ["
"                {"
"                    \"name\": \"bmi_multi\","
"                    \"params\": {"
"                        \"model_type_name\": \"bmi_multi_c++\","
"                        \"forcing_file\": \"\","
"                        \"init_config\": \"\","
"                        \"allow_exceed_end_time\": true,"
"                        \"main_output_variable\": \"OUTPUT_VAR_4\","
"                        \"uses_forcing_file\": false,"
"                        \"modules\": ["
"                            {"
"                                \"name\": \"bmi_c++\","
"                                \"params\": {"
"                                    \"model_type_name\": \"test_bmi_c++\","
"                                    \"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
"                                    \"init_config\": \"{{BMI_CPP_INIT_DIR_PATH}}/test_bmi_cpp_config_2.txt\","
"                                    \"allow_exceed_end_time\": true,"
"                                    \"main_output_variable\": \"OUTPUT_VAR_4\","
"                                    \"uses_forcing_file\": false,"
"                                    \"model_params\": {"
"                                        \"MODEL_VAR_1\": {"
"                                            \"source\": \"hydrofabric\","
"                                            \"from\": \"e\""
"                                        },"
"                                        \"MODEL_VAR_2\": {"
"                                            \"source\": \"hydrofabric\""
"                                        }"
"                                    },"
"                                    \"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": {"
"                                        \"INPUT_VAR_1\": \"Q2D\","
"                                        \"INPUT_VAR_2\": \"Q2D\""
"                                    }"
"                                }"
"                            }"
"                        ]"
"                    }"
"                }"
"            ],"
"            \"forcing\": {"
"                \"path\": \"./data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv\""
"            }"
"        }"
"    }"
"}";

/**
 * Configuration for model_params parsing at level:
 * - global multi-bmi
 */
const std::string EXAMPLE_5_b =
"{"
"    \"global\": {"
"         \"formulations\": ["
"             {"
"                 \"name\": \"bmi_multi\","
"                 \"params\": {"
"                     \"model_type_name\": \"bmi_multi_c++\","
"                     \"forcing_file\": \"\","
"                     \"init_config\": \"\","
"                     \"allow_exceed_end_time\": true,"
"                     \"main_output_variable\": \"OUTPUT_VAR_4\","
"                     \"uses_forcing_file\": false,"
"                     \"modules\": ["
"                         {"
"                             \"name\": \"bmi_c++\","
"                             \"params\": {"
"                                 \"model_type_name\": \"test_bmi_c++\","
"                                  \"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
"                                  \"init_config\": \"{{BMI_CPP_INIT_DIR_PATH}}/test_bmi_cpp_config_2.txt\","
"                                 \"allow_exceed_end_time\": true,"
"                                 \"main_output_variable\": \"OUTPUT_VAR_4\","
"                                 \"uses_forcing_file\": false,"
"                                 \"model_params\": {"
"                                     \"MODEL_VAR_1\": {"
"                                         \"source\": \"hydrofabric\","
"                                         \"from\": \"val\""
"                                     },"
"                                     \"MODEL_VAR_2\": {"
"                                         \"source\": \"hydrofabric\""
"                                     }"
"                                 },"
"                                 \"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": {"
"                                     \"INPUT_VAR_1\": \"APCP_surface\","
"                                     \"INPUT_VAR_2\": \"APCP_surface\""
"                                 }"
"                             }"
"                         }"
"                     ]"
"                 }"
"             }"
"         ],"
"        \"forcing\": { "
"            \"file_pattern\": \".*{{id}}.*.csv\","
"            \"path\": \"./data/forcing/\","
"            \"provider\": \"CsvPerFeature\""
"        }"
"    },"
"    \"time\": {"
"        \"start_time\": \"2015-12-01 00:00:00\","
"        \"end_time\": \"2015-12-30 23:00:00\","
"        \"output_interval\": 3600"
"    }"
"}";

const std::string EXAMPLE_6 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\":\"bmi_c++\","
          "\"params\": {"
            "\"model_type_name\": \"test_bmi_cpp\","
            "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
            "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
            "\"main_output_variable\": \"OUTPUT_VAR_2\","
            "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
              "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
              "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "},"
            "\"create_function\": \"bmi_model_create\","
            "\"destroy_function\": \"bmi_model_destroy\","
            "\"uses_forcing_file\": false"
          "} "
        "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{id}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"disable_catchment_output\": false,"
    "\"catchments\": { "
        "\"cat-52\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "}, "
        "\"cat-67\": { "
        "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "} "
    "} "
"}";


const std::string EXAMPLE_7 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\":\"bmi_c++\","
          "\"params\": {"
            "\"model_type_name\": \"test_bmi_cpp\","
            "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
            "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_{{id}}.txt\","
            "\"main_output_variable\": \"OUTPUT_VAR_2\","
            "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
              "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
              "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "},"
            "\"create_function\": \"bmi_model_create\","
            "\"destroy_function\": \"bmi_model_destroy\","
            "\"uses_forcing_file\": false"
          "} "
        "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{id}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"disable_catchment_output\": true"
"}";

const std::string EXAMPLE_8 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\":\"bmi_c++\","
          "\"params\": {"
            "\"model_type_name\": \"test_bmi_cpp\","
            "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
            "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/GARBAGE.txt\","
            "\"main_output_variable\": \"OUTPUT_VAR_2\","
            "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
              "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
              "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "},"
            "\"create_function\": \"bmi_model_create\","
            "\"destroy_function\": \"bmi_model_destroy\","
            "\"uses_forcing_file\": false"
          "} "
        "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{id}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
  "\"disable_catchment_output\": true,"
    "\"catchments\": { "
        "\"cat-67\": { "
        "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_{{id}}.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "} "
    "} "
"}";


const std::string EXAMPLE_9 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\":\"bmi_c++\","
          "\"params\": {"
            "\"model_type_name\": \"test_bmi_cpp\","
            "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
            "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
            "\"main_output_variable\": \"OUTPUT_VAR_2\","
            "\"cache_input_variable_metadata\": true,"
            "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
              "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
              "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "},"
            "\"create_function\": \"bmi_model_create\","
            "\"destroy_function\": \"bmi_model_destroy\","
            "\"uses_forcing_file\": false"
          "} "
        "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{id}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"disable_catchment_output\": true,"
    "\"catchments\": { "
        "\"cat-52\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "}, "
        "\"cat-67\": { "
        "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "} "
    "} "
"}";

const std::string EXAMPLE_10 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"cache_input_variable_metadata\": true,"
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{ID}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"catchments\": { "
        "\"cat-52\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"cache_input_variable_metadata\": true,"
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
          "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "}, "
        "\"cat-67\": { "
          "\"formulations\": [ "
            "{"
              "\"name\":\"bmi_c++\","
              "\"params\": {"
                "\"model_type_name\": \"test_bmi_cpp\","
                "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
                "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
                "\"main_output_variable\": \"OUTPUT_VAR_2\","
                "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
                  "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
                  "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
                "},"
                "\"create_function\": \"bmi_model_create\","
                "\"destroy_function\": \"bmi_model_destroy\","
                "\"uses_forcing_file\": false"
              "} "
            "} "
        "], "
          "\"forcing\": { "
              "\"file_pattern\": \".*{{id}}.*.csv\", "
              "\"path\": \"./data/forcing/\", "
              "\"provider\": \"CsvPerFeature\" "
          "} "
        "} "
    "} "
"}";

// Like EXAMPLE_9, but with no catchment-specific formulations, so added features fall through to the global
// formulation and thus inherit its `cache_input_variable_metadata` value of `true`.
const std::string EXAMPLE_11 = "{ "
    "\"global\": { "
      "\"formulations\": [ "
        "{"
          "\"name\":\"bmi_c++\","
          "\"params\": {"
            "\"model_type_name\": \"test_bmi_cpp\","
            "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
            "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
            "\"main_output_variable\": \"OUTPUT_VAR_2\","
            "\"cache_input_variable_metadata\": true,"
            "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { "
              "\"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG  "\","
              "\"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\""
            "},"
            "\"create_function\": \"bmi_model_create\","
            "\"destroy_function\": \"bmi_model_destroy\","
            "\"uses_forcing_file\": false"
          "} "
        "} "
      "], "
      "\"forcing\": { "
          "\"file_pattern\": \".*{{id}}.*.csv\", "
          "\"path\": \"./data/forcing/\", "
          "\"provider\": \"CsvPerFeature\" "
      "} "
    "}, "
    "\"time\": { "
        "\"start_time\": \"2015-12-01 00:00:00\", "
        "\"end_time\": \"2015-12-30 23:00:00\", "
        "\"output_interval\": 3600 "
    "}, "
    "\"disable_catchment_output\": true "
"}";

TEST_F(Formulation_Manager_Test, basic_reading_1) {
    std::stringstream stream;

    stream << fix_paths(EXAMPLE_1);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 2);

    ASSERT_TRUE(manager.contains("cat-52"));
    ASSERT_TRUE(manager.contains("cat-67"));
    ASSERT_EQ(manager.get_output_config().root, "./");
}

TEST_F(Formulation_Manager_Test, basic_reading_2) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_2);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 2);

    ASSERT_TRUE(manager.contains("cat-52"));
    ASSERT_TRUE(manager.contains("cat-67"));
    ASSERT_EQ(manager.get_output_config().root, "./output_dir/");
}

namespace {
    realization::Formulation_Manager manager_from_json(const std::string& json) {
        boost::property_tree::ptree tree;
        std::stringstream ss(json);
        boost::property_tree::json_parser::read_json(ss, tree);
        return realization::Formulation_Manager(tree);
    }
}

// Output config parsing (grouping / enable / root / legacy keys) is covered by Output_Test, and
// the factory's config->mode selection by CatchmentOutput_Test. What remains genuinely FM's is that
// it parses the output config at construction and validates it against build support:

// netcdf nexus output maps to per-formulation files when supported, and is
// rejected at construction when NGEN was built without NetCDF.
TEST_F(Formulation_Manager_Test, nexus_netcdf_format_respects_build_support)
{
    const std::string json = R"({ "output": { "nexus": { "format": "netcdf" } } })";
#if NGEN_WITH_NETCDF
    auto manager = manager_from_json(json);
    EXPECT_EQ(manager.get_output_config().nexus.format, realization::config::OutputFormat::netcdf);
#else
    EXPECT_THROW(manager_from_json(json), std::runtime_error);
#endif
}

// ---------------------------------------------------------------------------
// Integration: a real (BMI C++) formulation built by Formulation_Manager feeds the catchment
// output manager the driver builds via the factory. This is the one place the full composition
// (Formulation_Manager -> formulation -> factory -> manager -> file) is exercised end to end; the
// factory's mode selection lives in CatchmentOutput_Test and the CSV layout in
// CatchmentCsvOutputMgr_Test. A mocked (programmatic) hydrofabric avoids on-disk geojson data; the
// manager is scoped so its streams flush/close before the file is read.
// ---------------------------------------------------------------------------

namespace {
    const std::string MOCK_LINK_KEY = "toid";

    // Catchment-only collection (each draining to nexus_id), as manager.read()
    // expects -- read() builds a formulation for every feature it is given.
    geojson::GeoJSON build_catchments(const std::vector<std::string>& catchment_ids,
                                      const std::string& nexus_id)
    {
        geojson::three_dimensional_coordinates coords{
            {{1.0,2.0},{3.0,4.0},{5.0,6.0}}, {{7.0,8.0},{9.0,10.0},{11.0,12.0}}
        };
        auto fabric = std::make_shared<geojson::FeatureCollection>();
        for (const auto& id : catchment_ids) {
            geojson::PropertyMap props{ {MOCK_LINK_KEY, geojson::JSONProperty(MOCK_LINK_KEY, nexus_id)} };
            fabric->add_feature(std::make_shared<geojson::PolygonFeature>(
                geojson::PolygonFeature(geojson::polygon(coords), id, props)));
        }
        return fabric;
    }

    std::vector<std::string> read_lines(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) lines.push_back(line);
        return lines;
    }

    // A global test-BMI-C++ realization config with an output block, driven
    // through fix_paths() by the caller.
    std::string bmi_cpp_output_config(const std::string& output_root, const std::string& catchment_grouping) {
        return std::string("{ ")
          + "\"global\": { \"formulations\": [ { \"name\":\"bmi_c++\", \"params\": {"
          +   "\"model_type_name\": \"test_bmi_cpp\","
          +   "\"library_file\": \"{{EXTERN_LIB_DIR_PATH}}" BMI_TEST_CPP_LIB_NAME "\","
          +   "\"init_config\": \"{{BMI_C_INIT_DIR_PATH}}/test_bmi_c_config_0.txt\","
          +   "\"main_output_variable\": \"OUTPUT_VAR_2\","
          +   "\"" BMI_REALIZATION_CFG_PARAM_OPT__VAR_STD_NAMES "\": { \"INPUT_VAR_2\": \"" AORC_FIELD_NAME_TEMP_2M_AG "\", \"INPUT_VAR_1\": \"" AORC_FIELD_NAME_PRECIP_RATE "\" },"
          +   "\"create_function\": \"bmi_model_create\", \"destroy_function\": \"bmi_model_destroy\", \"uses_forcing_file\": false"
          + "} } ], \"forcing\": { \"file_pattern\": \".*{{id}}.*.csv\", \"path\": \"./data/forcing/\", \"provider\": \"CsvPerFeature\" } }, "
          + "\"time\": { \"start_time\": \"2015-12-01 00:00:00\", \"end_time\": \"2015-12-30 23:00:00\", \"output_interval\": 3600 }, "
          + "\"output\": { \"root\": \"" + output_root + "\", \"catchment\": { \"grouping\": \"" + catchment_grouping + "\" } } "
          + "}";
    }
}

TEST_F(Formulation_Manager_Test, aggregated_catchment_output_writes_single_file)
{
    namespace fs = std::filesystem;
    const std::string out_dir = "./test_agg_output_agg";
    fs::remove_all(out_dir);

    std::stringstream stream;
    stream << fix_paths(bmi_cpp_output_config(out_dir, "per_formulation"));
    boost::property_tree::ptree cfg;
    simulation_time_params sim_time = get_time_from_load_realization_config(stream, cfg);

    {
        auto manager = std::make_shared<realization::Formulation_Manager>(cfg);
        manager->read(sim_time, build_catchments({"cat-52", "cat-67"}, "nex-1"));
        // Mirror the driver's per_formulation wiring: gather every catchment's columns, then
        // construct the manager with the full set (output is owned by the driver, not HY_Features);
        // the aggregated file name defaults.
        std::vector<utils::FeatureDescriptor> registrations;
        for (const std::string id : {"cat-52", "cat-67"}) {
            registrations.emplace_back(id, manager->get_formulation(id)->get_output_fields());
        }
        auto mgr = realization::config::make_catchment_output_mgr(
            manager->get_output_config(), std::move(registrations));
    } // streams flush/close on destruction

    const std::string agg_file = out_dir + "/cat_output.csv";
    ASSERT_TRUE(fs::exists(agg_file));
    auto lines = read_lines(agg_file);
    ASSERT_EQ(lines.size(), 1u);                              // header written exactly once
    EXPECT_EQ(lines[0].rfind("catchment_id,Time Step,Time,", 0), 0u);
    EXPECT_FALSE(fs::exists(out_dir + "/cat-52.csv"));        // not per-feature
    EXPECT_FALSE(fs::exists(out_dir + "/cat-67.csv"));

    fs::remove_all(out_dir);
}

TEST_F(Formulation_Manager_Test, basic_run_1) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_1);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 2);

    std::map<std::string, std::map<long, double>> calculated_results;

    double dt = 3600.0;

    for (std::pair<std::string, std::shared_ptr<realization::Catchment_Formulation>> formulation : manager) {
        if (calculated_results.count(formulation.first) == 0) {
            calculated_results.emplace(formulation.first, std::map<long, double>());
        }

        double calculation;

        for (long t = 0; t < 4; t++) {
            calculation = formulation.second->get_response(t, dt);

            calculated_results.at(formulation.first).emplace(t, calculation);
        }
    }
}

TEST_F(Formulation_Manager_Test, basic_run_3) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_3);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));

    std::vector<double> expected_results = {571.4, 570.6, 569.0};

    std::vector<double> actual_results(expected_results.size());

    for (int i = 0; i < expected_results.size(); i++) {
        actual_results[i] = manager.get_formulation("cat-67")->get_response(i, 3600);
    }

    for (int i = 0; i < actual_results.size(); i++) {
        double actual = actual_results[i];
        // This is an error margin of the largest of 0.1% of actual value, or 1 mm
        // TODO: this may not be precise enough long-term
        double error_margin = std::max(actual * 0.001, 0.001);
        double expected = expected_results[i];
        double diff = actual > expected ? actual - expected : expected - actual;
        ASSERT_LE(diff, error_margin);
    }
}

/**
 * Testing config the same as EX 1 (like in basic_run_1) but with `cache_input_variable_metadata` true for global
 * formulation config (which is not all formulations for that configuration, as there are two independently specified).
 */
TEST_F(Formulation_Manager_Test, basic_run_9) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_9);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 2);

    std::map<std::string, std::map<long, double>> calculated_results;

    double dt = 3600.0;

    for (std::pair<std::string, std::shared_ptr<realization::Catchment_Formulation>> formulation : manager) {
        if (calculated_results.count(formulation.first) == 0) {
            calculated_results.emplace(formulation.first, std::map<long, double>());
        }

        double calculation;

        for (long t = 0; t < 4; t++) {
            calculation = formulation.second->get_response(t, dt);

            calculated_results.at(formulation.first).emplace(t, calculation);
        }
    }
}

/**
 * Testing config the same as EX 3 (like in basic_run_3) but with `cache_input_variable_metadata` true for global and
 * one catchment formulation config (but not the other).
 */
TEST_F(Formulation_Manager_Test, basic_run_10) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_10);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));

    std::vector<double> expected_results = {571.4, 570.6, 569.0};

    std::vector<double> actual_results(expected_results.size());

    for (int i = 0; i < expected_results.size(); i++) {
        actual_results[i] = manager.get_formulation("cat-67")->get_response(i, 3600);
    }

    for (int i = 0; i < actual_results.size(); i++) {
        double actual = actual_results[i];
        // This is an error margin of the largest of 0.1% of actual value, or 1 mm
        // TODO: this may not be precise enough long-term
        double error_margin = std::max(actual * 0.001, 0.001);
        double expected = expected_results[i];
        double diff = actual > expected ? actual - expected : expected - actual;
        ASSERT_LE(diff, error_margin);
    }
}

/**
 * Verify BMI input variable metadata caching is disabled by default when nothing configures it (EX 1).
 *
 * EXAMPLE_1 sets `cache_input_variable_metadata` nowhere (neither globally nor for either catchment's own
 * formulation), so both catchments should resolve to the default of `false`.
 */
TEST_F(Formulation_Manager_Test, cache_bmi_var_metadata_1) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_1);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 2);

    for (const std::pair<std::string, bool>& expected : {std::make_pair(std::string("cat-52"), false),
                                                         std::make_pair(std::string("cat-67"), false)}) {
        std::shared_ptr<realization::Bmi_Module_Formulation> mod =
            std::dynamic_pointer_cast<realization::Bmi_Module_Formulation>(manager.get_formulation(expected.first));
        ASSERT_NE(mod, nullptr) << expected.first << " should be a BMI module formulation";
        ASSERT_EQ(mod->is_input_variable_metadata_cached(), expected.second) << expected.first;
    }
}

/**
 * Verify a global `cache_input_variable_metadata` of `true` is not inherited by catchments with their own
 * formulations (EX 9).
 *
 * EXAMPLE_1 config but with `cache_input_variable_metadata` `true` for the global formulation only.  Because both
 * catchments independently specify their own formulations (which omit the option), neither inherits the global
 * value, so both should resolve to the default of `false`.
 */
TEST_F(Formulation_Manager_Test, cache_bmi_var_metadata_9) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_9);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 2);

    for (const std::pair<std::string, bool>& expected : {std::make_pair(std::string("cat-52"), false),
                                                         std::make_pair(std::string("cat-67"), false)}) {
        std::shared_ptr<realization::Bmi_Module_Formulation> mod =
            std::dynamic_pointer_cast<realization::Bmi_Module_Formulation>(manager.get_formulation(expected.first));
        ASSERT_NE(mod, nullptr) << expected.first << " should be a BMI module formulation";
        ASSERT_EQ(mod->is_input_variable_metadata_cached(), expected.second) << expected.first;
    }
}

/**
 * Verify `cache_input_variable_metadata` resolves independently per catchment formulation (EX 10).
 *
 * EXAMPLE_10 enables the option globally and for cat-52's own formulation, but not for cat-67's own formulation.
 * So cat-52 should resolve to `true` and cat-67 to `false`, demonstrating the option is honored per catchment.
 */
TEST_F(Formulation_Manager_Test, cache_bmi_var_metadata_10) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_10);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 2);

    for (const std::pair<std::string, bool>& expected : {std::make_pair(std::string("cat-52"), true),
                                                         std::make_pair(std::string("cat-67"), false)}) {
        std::shared_ptr<realization::Bmi_Module_Formulation> mod =
            std::dynamic_pointer_cast<realization::Bmi_Module_Formulation>(manager.get_formulation(expected.first));
        ASSERT_NE(mod, nullptr) << expected.first << " should be a BMI module formulation";
        ASSERT_EQ(mod->is_input_variable_metadata_cached(), expected.second) << expected.first;
    }
}

/**
 * Verify a global `cache_input_variable_metadata` of `true` is inherited by catchments without their own
 * formulations (EX 11).
 *
 * EXAMPLE_11 enables the option only on the global formulation and specifies no catchment formulations.  Both
 * added features therefore fall through to the global formulation and should inherit its value of `true`.
 */
TEST_F(Formulation_Manager_Test, cache_bmi_var_metadata_11) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_11);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-52");
    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 2);

    for (const std::pair<std::string, bool>& expected : {std::make_pair(std::string("cat-52"), true),
                                                         std::make_pair(std::string("cat-67"), true)}) {
        std::shared_ptr<realization::Bmi_Module_Formulation> mod =
            std::dynamic_pointer_cast<realization::Bmi_Module_Formulation>(manager.get_formulation(expected.first));
        ASSERT_NE(mod, nullptr) << expected.first << " should be a BMI module formulation";
        ASSERT_EQ(mod->is_input_variable_metadata_cached(), expected.second) << expected.first;
    }
}

TEST_F(Formulation_Manager_Test, read_extra) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_3);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));
}

TEST_F(Formulation_Manager_Test, init_config_pattern_match_global) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_7);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));
}

TEST_F(Formulation_Manager_Test, init_config_pattern_match_specific) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_8);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    ASSERT_TRUE(manager.is_empty());

    this->add_feature("cat-67");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 1);
    ASSERT_TRUE(manager.contains("cat-67"));
}

TEST_F(Formulation_Manager_Test, forcing_provider_specification) {
    std::stringstream stream;
    stream << fix_paths(EXAMPLE_4);

    boost::property_tree::ptree realization_config;
    simulation_time_params simulation_time_config = get_time_from_load_realization_config(stream, realization_config);


    realization::Formulation_Manager manager = realization::Formulation_Manager(realization_config);

    this->add_feature("cat-67");
    this->add_feature("cat-27115");
    manager.read(simulation_time_config, this->fabric);

    ASSERT_EQ(manager.get_size(), 2);
    ASSERT_TRUE(manager.contains("cat-67"));
    ASSERT_TRUE(manager.contains("cat-27115"));

    //NOT
    std::vector<double> expected_results = {191.106140 / 1000.0};

    std::vector<double> actual_results(expected_results.size());

    for (std::pair<std::string, std::shared_ptr<realization::Catchment_Formulation>> formulation : manager) {
        formulation.second->get_response(0, 3600);
    }

    for (int i = 0; i < actual_results.size(); i++) {
        double actual = actual_results[i];
        // This is an error margin of the largest of 0.1% of actual value, or 1 mm
        // TODO: this may not be precise enough long-term
    }
}

TEST_F(Formulation_Manager_Test, read_external_attributes) {
    std::stringstream stream_a;
    stream_a << fix_paths(EXAMPLE_5_a);

    std::stringstream stream_b;
    stream_b << fix_paths(EXAMPLE_5_b);

    time_step_t ts = 2;

    /**
     * Lambda to add a feature to the fabric, and then assert that its properties exists.
     *
     * Assertions:
     * - Asserts that the feature with `id` correctly has all properties in `properties`.
     */
    auto add_and_check_feature = [&, this](const std::string& id, geojson::PropertyMap properties) {
      this->add_feature(id, properties);
      auto feature = this->fabric->get_feature(id);
      for (auto& pair : properties)
        ASSERT_TRUE(feature->has_property(pair.first));
    };

    /**
     * Lambda to check that formulation values are present in output.
     * 
     * Assertions:
     * - Asserts that the formulation manager contains the given catchment ID.
     * - Asserts that each expected value appears in the formulation's output
     *   values vector at timestep `ts`.
     *
     * @note The output values are retrieved as a vector of doubles
     *       (get_output_values_for_timestep) and checked directly with
     *       `std::find` -- no line formatting or string parsing.
     */
    auto check_formulation_values = [&](auto fm, const std::string& id, std::initializer_list<double> expected) {
        ASSERT_TRUE(fm.contains(id));
        auto formulation = fm.get_formulation(id);
        formulation->get_response(ts, 3600);
        std::vector<double> values = formulation->get_output_values_for_timestep(ts);

        for (auto& expect : expected) {
            ASSERT_NE(std::find(values.begin(), values.end(), expect), values.end());
        }
    };

    boost::property_tree::ptree realization_config_a;
    simulation_time_params simulation_time_config_a = get_time_from_load_realization_config(stream_a, realization_config_a);

    auto manager = realization::Formulation_Manager(realization_config_a);
  
    add_and_check_feature("cat-67", geojson::PropertyMap{
      { "MODEL_VAR_2", geojson::JSONProperty{"MODEL_VAR_2", 10 } },
      { "n",           geojson::JSONProperty{"n",           1.70352 } }
    });

    add_and_check_feature("cat-52", geojson::PropertyMap{
      { "MODEL_VAR_2", geojson::JSONProperty{"MODEL_VAR_2", 15 } },
      { "pi",           geojson::JSONProperty{"pi",         3.14159 } }
    });

    add_and_check_feature("cat-27115", geojson::PropertyMap{
      { "MODEL_VAR_2", geojson::JSONProperty{"MODEL_VAR_2", 20 } },
      { "e",           geojson::JSONProperty{"e",           2.71828 } }
    });

    manager.read(simulation_time_config_a, this->fabric);

    ASSERT_EQ(manager.get_size(), 3);
    check_formulation_values(manager, "cat-67",    { 1.70352, 10.0 });
    check_formulation_values(manager, "cat-52",    { 3.14159, 15.0 });
    check_formulation_values(manager, "cat-27115", { 2.71828, 20.0 });

    this->fabric->remove_feature_by_id("cat-67");
    this->fabric->remove_feature_by_id("cat-52");
    this->fabric->remove_feature_by_id("cat-27115");

    boost::property_tree::ptree realization_config_b;
    simulation_time_params simulation_time_config_b = get_time_from_load_realization_config(stream_b, realization_config_b);

    manager = realization::Formulation_Manager(realization_config_b);
   
    //Test that two hydrofabric features, using global formulation (EXAMPLE_5_b)
    //end up with unique hydrofabric parameters in the formulations after they
    //are created and linked to the attributes.  Uses the same parameter name with
    //different values.
 
    add_and_check_feature("cat-67", geojson::PropertyMap{
      { "MODEL_VAR_2", geojson::JSONProperty{"MODEL_VAR_2", 9231 } },
      { "val",           geojson::JSONProperty{"val",       7.41722 } }
    });
    
    add_and_check_feature("cat-27", geojson::PropertyMap{
      { "MODEL_VAR_2", geojson::JSONProperty{"MODEL_VAR_2", 18 } },
      { "val",          geojson::JSONProperty{"val", 3} }
    });
    
    manager.read(simulation_time_config_b, this->fabric);
    
    check_formulation_values(manager, "cat-27",    { 3.00000, 18.0 });
    check_formulation_values(manager, "cat-67", { 7.41722, 9231 });
}
