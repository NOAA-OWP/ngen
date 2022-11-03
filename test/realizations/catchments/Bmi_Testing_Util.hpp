#ifndef NGEN_BMI_TESTING_UTIL_CPP
#define NGEN_BMI_TESTING_UTIL_CPP

#ifndef SHARED_LIB_FILE_EXTENSION
#ifdef __APPLE__
#define SHARED_LIB_FILE_EXTENSION ".dylib"
#else
#ifdef __GNUC__
    #define SHARED_LIB_FILE_EXTENSION ".so"
    #endif // __GNUC__
#endif // __APPLE__
#endif // SHARED_LIB_FILE_EXTENSION

#ifndef BMI_TEST_C_LIB_NAME
#define BMI_TEST_C_LIB_NAME "libtestbmicmodel"
#endif

#ifndef BMI_TEST_CPP_LIB_NAME
#define BMI_TEST_CPP_LIB_NAME "libtestbmicppmodel"
#endif

#ifndef BMI_TEST_FORTRAN_LIB_NAME
#define BMI_TEST_FORTRAN_LIB_NAME "libtestbmifortranmodel"
#endif

#ifndef BMI_TEST_PYTHON_LIB_NAME
#define BMI_TEST_PYTHON_LIB_NAME "test_bmi_py.bmi_model"
#endif

#ifndef BMI_CPP_TYPE
#define BMI_CPP_TYPE "bmi_c++"
#endif

#ifndef BMI_C_TYPE
#define BMI_C_TYPE "bmi_c"
#endif

#ifndef BMI_FORTRAN_TYPE
#define BMI_FORTRAN_TYPE "bmi_fortran"
#endif

#ifndef BMI_PYTHON_TYPE
#define BMI_PYTHON_TYPE "bmi_python"
#endif

#ifndef BMI_MULTI_TYPE
#define BMI_MULTI_TYPE "bmi_multi"
#endif

#include <map>
#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "Bmi_Formulation.hpp"
#include "FileChecker.h"

using namespace realization;
using namespace std;

namespace ngen {
    namespace test {

        class Bmi_Testing_Util {
        public:

            Bmi_Testing_Util() {
                projectRoot = detectProjectRootPath();
            }

            /**
             * All available types for BMI formulations.
             */
            const vector<string> bmiAllFormulationTypes = {BMI_C_TYPE, BMI_FORTRAN_TYPE, BMI_PYTHON_TYPE, BMI_MULTI_TYPE};

            /**
             * Available types for BMI formulations that are backed by a single module.
             */
            const vector<string> bmiModuleFormulationTypes = {BMI_C_TYPE, BMI_FORTRAN_TYPE, BMI_PYTHON_TYPE};

            /**
             * Config names for BMI formulation types.
             */
            const map<string, string> bmiFormulationConfigNames = {
                    {BMI_CPP_TYPE, "test_bmi_cpp"},
                    {BMI_C_TYPE, "test_bmi_c"},
                    {BMI_FORTRAN_TYPE, "test_bmi_fortran"},
                    {BMI_PYTHON_TYPE, BMI_TEST_PYTHON_LIB_NAME}
            };

            /**
             * Files that can serve as a template for a realization config for the given types.
             *
             * Note that these example are not (necessarily) immediately suitable for use with these examples.  They
             * likely need to be read to property tree objects, and then have those config trees customized according to
             * the particular state of a given example.
             */
            const map<string, string> bmiRealizationConfigTemplates = {
                    {BMI_C_TYPE, "data/example_realization_config_w_bmi_c__lin_mac.json"},
                    {BMI_FORTRAN_TYPE, "data/example_realization_config_w_bmi_c__lin_mac.json"},
                    {BMI_PYTHON_TYPE, "data/example_realization_config_w_bmi_c__lin_mac.json"},
                    {BMI_MULTI_TYPE, "data/example_bmi_multi_realization_config.json"}
            };

            /**
             * Library/package names for BMI module formulation types.
             */
            const map<string, string> bmiFormulationLibNames = {
                    {BMI_CPP_TYPE, BMI_TEST_CPP_LIB_NAME},
                    {BMI_C_TYPE, BMI_TEST_C_LIB_NAME},
                    {BMI_FORTRAN_TYPE, BMI_TEST_FORTRAN_LIB_NAME},
                    {BMI_PYTHON_TYPE, BMI_TEST_PYTHON_LIB_NAME}
            };

            /**
             * Relative paths to the directories containing BMI init config examples, from the project root.
             */
            const map<string, string> bmiInitConfigDirRelativePaths = {
                    {BMI_CPP_TYPE, "test/data/bmi/test_bmi_cpp/"},
                    {BMI_C_TYPE, "test/data/bmi/test_bmi_c/"},
                    {BMI_FORTRAN_TYPE, "test/data/bmi/test_bmi_fortran/"},
                    {BMI_PYTHON_TYPE, "test/data/bmi/test_bmi_python/"}
            };

            /**
             * Init config example file basename pattern.
             */
            const map<string, string> bmiInitConfigBasenamePattern = {
                    {BMI_CPP_TYPE, "test_bmi_cpp_config_"},
                    {BMI_C_TYPE, "test_bmi_c_config_"},
                    {BMI_FORTRAN_TYPE, "test_bmi_fortran_config_"},
                    {BMI_PYTHON_TYPE, "test_bmi_python_config_"}
            };

            /**
             * Init config example file extensions.
             */
            const map<string, string> bmiInitConfigBasenameExtensions = {
                    {BMI_CPP_TYPE, ".txt"},
                    {BMI_C_TYPE, ".txt"},
                    {BMI_FORTRAN_TYPE, ".txt"},
                    {BMI_PYTHON_TYPE, ".yml"}
            };

            /**
             * Relative paths to the directories containing each testing BMI modules, from the project root.
             */
            const map<string, string> bmiModuleRelativePaths = {
                    {BMI_CPP_TYPE, "extern/test_bmi_cpp/cmake_build/"},
                    {BMI_C_TYPE, "extern/test_bmi_c/cmake_build/"},
                    {BMI_FORTRAN_TYPE, "extern/test_bmi_fortran/cmake_build/"},
                    // TODO, this may need to just be extern/
                    {BMI_PYTHON_TYPE, "extern/test_bmi_py"}
            };

            /**
             * Relative path to forcing data directory, from project root.
             */
            const string forcingDirRelative = "data/forcing/";

            /**
             * Paths to forcing data files by catchment id, relative with respect to the project root.
             */
            const map<string, string> forcingFileRelativePaths = {
                    {"cat-27", forcingDirRelative + "cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv"}
            };

            /**
             * Detect the relative path to the project root from the current working directory.
             *
             * Detect the relative path to the project root, returning a prefix that can be add to other directories that are
             * relative with respect to it, thus making it possible to obtain
             *
             * Implemented to search for the forcing data directory, and assume root can be inferred from that when
             * found.
             *
             * Prefix options are hard coded and are ``./``, ``../``, and ``../../``.
             */
            string detectProjectRootPath() {
                std::vector<std::string> dir_prefix_opts = {"./", "../", "../../"};
                const string &filePathFromProjRoot = forcingFileRelativePaths.begin()->second;
                string forcingFilePath;
                // For each prefix, see if the combination of that and an example forcing file relative path
                for (auto &proj_root_option : dir_prefix_opts) {
                    forcingFilePath = proj_root_option + filePathFromProjRoot;
                    if (utils::FileChecker::file_is_readable(forcingFilePath)) {
                        return proj_root_option;
                    }
                }
                return "";
            }

            string getForcingDirectory() {
                return projectRoot + forcingDirRelative;
            }

            /**
             * Get the path of an init config for a particular type and example number.
             *
             * Function combines the detected project root with the particular characteristics for the given BMI type -
             * init config directory, basename pattern, and filename extension - as well as the example number (assumed
             * to be a part of the file basename pattern) to produce a standard expected BMI init config file path.
             *
             * @param bmiType The BMI type
             * @param example_num The index of the test scenario for which the standardized BMI init config is needed.
             * @return The path of the BMI init config for this particular type and example.
             */
            string getBmiInitConfigFilePath(const string &bmiType, const int example_num) {
                return projectRoot +
                       bmiInitConfigDirRelativePaths.at(bmiType) +
                       bmiInitConfigBasenamePattern.at(bmiType) +
                       to_string(example_num) +
                       bmiInitConfigBasenameExtensions.at(bmiType);
            }

            string getForcingFilePath(const string &catchmentId) {
                return projectRoot + forcingFileRelativePaths.at(catchmentId);
            }

            string getModuleFilePath(const string &bmiType) {
                return projectRoot + bmiModuleRelativePaths.at(bmiType) + bmiFormulationLibNames.at(bmiType) +
                       string(SHARED_LIB_FILE_EXTENSION);
            }

        private:

            string projectRoot;

        };
    }
}

#endif //NGEN_BMI_TESTING_UTIL_CPP
