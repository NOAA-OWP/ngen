#ifndef NGEN_INTERPRETERUTIL_HPP
#define NGEN_INTERPRETERUTIL_HPP

#ifdef ACTIVATE_PYTHON

#include <map>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <vector>
#include <tuple>
namespace py = pybind11;

namespace utils {
    namespace ngenPy {

        using namespace pybind11::literals; // to bring in the `_a` literal

        /**
         * A central utility class owning and managing the embedded Python interpreter and its imported modules.
         *
         * This type manages imported modules for the interpreter and provides an interface for accessing handles to
         * desired modules and classes.  It does this by maintaining a map of already-imported top level Python modules,
         * keyed by name.  A request for something that is a submodule is fulfilled by accessing appropriate attributes
         * of a top level module and descendents at one or more levels, until a handle to the desired module is reached.
         *
         * Because of the need to maintain the single interpreter, this type is implemented as a singleton.
         */
        class InterpreterUtil {

        public:
            static InterpreterUtil &getInstance() {
                static InterpreterUtil instance;
                return instance;
            }

        private:
            InterpreterUtil() {
                guardPtr = std::make_shared<py::scoped_interpreter>();
                // Go ahead and do these
                importTopLevelModule("pathlib");
                importTopLevelModule("sys");
                Path = importedTopLevelModules["pathlib"].attr("Path");

                py::list venv_package_dirs = getVenvPackagesDirOptions();
                // Add any found options
                for (auto &opt : venv_package_dirs) {
                    addToPath(std::string(py::str(opt.attr("parent"))));
                    addToPath(std::string(py::str(opt)));
                }
            }

            ~InterpreterUtil() = default;

        public:

            /**
             * Add to the Python system path of the singleton instance.
             *
             * @param directoryPath The desired directory to add to the Python system path.
             */
            static void addToPyPath(const std::string &directoryPath) {
                getInstance().addToPath(directoryPath);
            }

            /**
             * Add to the Python system path of this instance.
             *
             * @param directoryPath The desired directory to add to the Python system path.
             */
            void addToPath(const std::string &directoryPath) {
                std::tuple<py::list, std::vector<std::string>> sysPathTuple = getSystemPath();
                py::list sys_path = std::get<0>(sysPathTuple);
                std::vector<std::string> sys_path_vector = std::get<1>(sysPathTuple);

                py::object requestedDirPath = Path(directoryPath);
                if (py::bool_(requestedDirPath.attr("is_dir")())) {
                    if (std::find(sys_path_vector.begin(), sys_path_vector.end(), directoryPath) == sys_path_vector.end()) {
                        sys_path.attr("insert")(sys_path_vector.size(), py::str(directoryPath));
                    }
                }
                else {
                    std::string dirPath = py::str(requestedDirPath);
                    throw std::runtime_error("Cannot add non-existing directory '" + dirPath + "' to Python PATH");
                }
            }

            /**
             * Return bound Python module handle from the singleton instance, importing a top-level module if necessary.
             *
             * E.g., ``Path`` from the ``pathlib`` module should be in a vector ``pathlib.Path``.
             *
             * @param name Full name of the desired module, as a ``.`` delimited string.
             * @return Handle to the desired Python module or type.
             */
            static py::module_ getPyModule(const std::string &name) {
                return getInstance().getModule(splitFullModuleName(name));
            }

            /**
             * Return bound Python module handle from the singleton instance, importing a top-level module if necessary.
             *
             * E.g., a request for a handle to the ``Path`` class from the ``pathlib`` package module should be made
             * using a vector ``{"pathlib", "Path"}``.
             *
             * Note that anything that has had its top level module already imported is itself considered to be imported
             * already by this class.  This is because of how the type has been designed to work with the pybind11
             * embedded interpreter.
             *
             * @param moduleLevelNames Full name of the desired module, as a vector of the names of the top-level module
             *                         and (when applicable) submodules.
             * @return Handle to the desired Python module or type.
             */
            static py::module_ getPyModule(const std::vector<std::string> &moduleLevelNames) {
                return getInstance().getModule(moduleLevelNames);
            }

            /**
             * Return a bound handle to a Python module, importing the top-level module if necessary.
             *
             * E.g., ``Path`` from the ``pathlib`` module should be in a vector ``pathlib.Path``.
             *
             * @param name Full name of the desired module, as a ``.`` delimited string.
             * @return Handle to the desired Python module or type.
             */
            py::module_ getModule(const std::string &name) {
                return getModule(splitFullModuleName(name));
            }

            /**
             * Return a bound handle to a Python module, importing the top-level module if necessary.
             *
             * E.g., a request for a handle to the ``Path`` class from the ``pathlib`` package module should be made
             * using a vector ``{"pathlib", "Path"}``.
             *
             * Note that anything that has had its top level module already imported is itself considered to be imported
             * already by this class.  This is because of how the type has been designed to work with the pybind11
             * embedded interpreter.
             *
             * @param moduleLevelNames Full name of the desired module, as a vector of the names of the top-level module
             *                         and (when applicable) submodules.
             * @return Handle to the desired Python module or type.
             */
            py::module_ getModule(const std::vector<std::string> &moduleLevelNames) {
                if (!isImported(moduleLevelNames)) {
                    importTopLevelModule(moduleLevelNames[0]);
                }
                // Start with top-level component name
                py::module_ module = importedTopLevelModules.find(moduleLevelNames[0])->second;
                // Recurse as needed through sub-components via calls to "attr"
                // (make sure to start at the second level; i.e., index 1)
                for (size_t i = 1; i < moduleLevelNames.size(); ++i) {
                    module = module.attr(moduleLevelNames[i].c_str());
                }
                return module;
            }

        protected:

            /**
             * Find any virtual environment site packages directory, starting from options under the current directory.
             *
             * @return The absolute path of the site packages directory, as a string.
             */
            py::list getVenvPackagesDirOptions() {
                // Add the package dir from a local virtual environment directory also, if there is one
                py::object venv_dir = Path("./.venv");
                // Try switching to a secondary if primary isn't a directory
                if (!(py::bool_(venv_dir.attr("is_dir")()))) {
                    venv_dir = Path("./venv");
                }
                // At this point, proceed if we have a good venv dir found
                if (py::bool_(venv_dir.attr("is_dir")())) {
                    // Resolve the full path
                    venv_dir = venv_dir.attr("resolve")();
                    // Get options for the packages dir
                    py::list site_packages_options = py::list(venv_dir.attr("glob")("**/site-packages/"));
                    return site_packages_options;
                }
                return py::list();
            }

            /**
             * Get the current Python interpreter system path, returning in both a Python and C++ format.
             *
             * @return A tuple containing the Python list object and a C++ vector of strings, each representing the
             *         current Python system path.
             */
            inline std::tuple<py::list, std::vector<std::string>> getSystemPath() {
                py::list sys_path = importedTopLevelModules.find("sys")->second.attr("path");
                std::vector<std::string> sys_path_vector(sys_path.size());
                size_t i = 0;
                for (auto item : sys_path) {
                    sys_path_vector[i++] = py::str(item);
                }
                return std::make_tuple(sys_path, sys_path_vector);
            }

            /**
             * Get whether an Python module is imported.
             *
             * Note that anything that has had its root module already imported will be considered imported already, because
             * of the way pybind11 handles importing modules.  E.g., if ``Path`` from ``pathlib`` was imported, ``PurePath``
             * will be treated as imported as well, in the context of this function.
             *
             * @param moduleLevelNames Full name of the desired module, as a vector of the names of the top-level module
             *                         and (when applicable) submodules.
             * @return Whether an Python module is imported.
             */
            inline bool isImported(const std::vector<std::string> &moduleLevelNames) {
                return importedTopLevelModules.find(moduleLevelNames[0]) != importedTopLevelModules.end();
            }

            /**
             * Get whether an Python module is imported.
             *
             * Note that anything that has had its top-level module already imported will be considered imported already, because
             * of the way pybind11 handles importing modules.  E.g., if ``Path`` from ``pathlib`` was imported, ``PurePath``
             * will be treated as imported as well, in the context of this function.
             *
             * @param name The (possibly ``.`` delimited) name of the module.
             * @return Whether an Python module is imported.
             */
            inline bool isImported(const std::string &name) {
                return importedTopLevelModules.find(splitFullModuleName(name)[0]) != importedTopLevelModules.end();
            }

        private:
            // This is required for the Python interpreter and must be kept alive
            std::shared_ptr<py::scoped_interpreter> guardPtr;

            std::map<std::string, py::module_> importedTopLevelModules;

            py::module_ Path;

            /**
             * Import a specified top level module.
             *
             * This import is done without any checking to see if the module has already been imported.
             *
             * @param topLevelName The name of the desired top level Python module to import.
             */
            inline void importTopLevelModule(const std::string &topLevelName) {
                importedTopLevelModules[topLevelName] = py::module_::import(topLevelName.c_str());
            }

            /**
             * Split a ``.`` delimited fully qualified module name into a vector of the individual components.
             *
             * @param fullName Fully qualified module name.
             * @return A vector of the individual components.
             */
            static inline std::vector<std::string> splitFullModuleName(std::string fullName) {
                size_t pos = 0;
                std::string delimiter = ".";
                std::vector<std::string> splitName;
                while ((pos = fullName.find(delimiter)) != std::string::npos) {
                    splitName.push_back(fullName.substr(0, pos));
                    fullName.erase(0, pos + delimiter.length());
                }
                splitName.push_back(fullName);
                return splitName;
            }

        };
    }
}

#endif // ACTIVATE_PYTHON

#endif // NGEN_INTERPRETERUTIL_HPP
