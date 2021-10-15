#ifndef NGEN_HYDROFABRICSUBSETTER_HPP
#define NGEN_HYDROFABRICSUBSETTER_HPP

#ifdef ACTIVATE_PYTHON

#include <pybind11/embed.h>
#include "InterpreterUtil.hpp"

namespace py = pybind11;

#include "FileChecker.h"

namespace utils {
    namespace ngenPy {
        using namespace pybind11::literals; // to bring in the `_a` literal

        /**
         * Class for performing splitting hydrofabric files using external Python tools.
         */
        class HydrofabricSubsetter {

        public:

            HydrofabricSubsetter(const std::string &catchmentDataFile, const std::string &nexusDataFile,
                                 const std::string &crosswalkDataFile, const std::string &partitionsConfigFile)
                    : catchmentDataFile(catchmentDataFile), nexusDataFile(nexusDataFile),
                      crosswalkDataFile(crosswalkDataFile), partitionsConfigFile(partitionsConfigFile)
            {

                if (!FileChecker::file_is_readable(catchmentDataFile)) {
                    throw std::runtime_error(
                            "Cannot subdivided hydrofabric files: base catchment file " + catchmentDataFile +
                            " does not exist");
                }
                if (!FileChecker::file_is_readable(nexusDataFile)) {
                    throw std::runtime_error(
                            "Cannot subdivided hydrofabric files: base nexus file " + nexusDataFile + " does not exist");
                }
                #ifdef ACTIVATE_PYTHON
                py::object Cli_Class = InterpreterUtil::getPyModule("dmod.subsetservice").attr("Cli");
                py::object crosswalk_arg;
                if (crosswalkDataFile.empty()) {
                    crosswalk_arg = py::none();
                }
                else {
                    crosswalk_arg = py::str(crosswalkDataFile);
                }

                py_cli = Cli_Class("catchment_geojson"_a=catchmentDataFile,
                                   "nexus_geojson"_a=nexusDataFile,
                                   "crosswalk_json"_a=crosswalk_arg,
                                   "partition_file_str"_a=partitionsConfigFile);

                if (crosswalkDataFile.empty()) {
                    this->crosswalkDataFile = py::str(py_cli.attr("crosswalk"));
                }
                #else
                throw std::runtime_error("Cannot use Python hydrofabric subsetter tool unless Python support is active");
                #endif

            }

            HydrofabricSubsetter(const std::string &catchmentDataFile, const std::string &nexusDataFile,
                                 const std::string &partitionsConfigFile)
                    : HydrofabricSubsetter(catchmentDataFile, nexusDataFile, "", partitionsConfigFile) { }

            #ifdef ACTIVATE_PYTHON
            /**
             * Copy constructor.
             *
             * @param pyHySub
             */
            HydrofabricSubsetter(HydrofabricSubsetter &p) : catchmentDataFile(p.catchmentDataFile),
                                                            nexusDataFile(p.nexusDataFile),
                                                            crosswalkDataFile(p.crosswalkDataFile),
                                                            py_cli(p.py_cli),
                                                            partitionsConfigFile(p.partitionsConfigFile) { }

            /**
             * Move constructor.
             *
             * @param p
             */
            HydrofabricSubsetter(HydrofabricSubsetter &&p) : catchmentDataFile(std::move(p.catchmentDataFile)),
                                                             nexusDataFile(std::move(p.nexusDataFile)),
                                                             crosswalkDataFile(std::move(p.crosswalkDataFile)),
                                                             py_cli(std::move(p.py_cli)),
                                                             partitionsConfigFile(std::move(p.partitionsConfigFile)) { }

            virtual bool execSubdivision() {
                bool result;
                try {
                    py::bool_ bool_result = py_cli.attr("divide_hydrofabric")();
                    result = bool_result;
                }
                catch (const std::exception &e) {
                    std::cerr << "Failed to subdivide hydrofabric: " << e.what() << std::endl;
                    result = false;
                }
                return result;
            }

            virtual bool execSubdivision(int index) {
                bool result;
                try {
                    py::bool_ bool_result = py_cli.attr("divide_hydrofabric")(index);
                    result = bool_result;
                }
                catch (const std::exception &e) {
                    std::cerr << "Failed to subdivide hydrofabric for index " << index << ": " << e.what() << std::endl;
                    result = false;
                }
                return result;
            }

            #endif // ACTIVATE_PYTHON

        private:
            std::string catchmentDataFile;
            std::string crosswalkDataFile;
            std::string nexusDataFile;
            std::string partitionsConfigFile;
            #ifdef ACTIVATE_PYTHON
            py::object py_cli;
            #endif

        };
    }
}

#endif // ACTIVATE_PYTHON

#endif //NGEN_HYDROFABRICSUBSETTER_HPP
