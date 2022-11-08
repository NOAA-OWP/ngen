#ifndef NGEN_INTERPRETERUTIL_HPP
#define NGEN_INTERPRETERUTIL_HPP

#ifdef ACTIVATE_PYTHON

#include <cstdlib>
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
            static std::shared_ptr<InterpreterUtil> getInstance() {
                /**
                 * @brief Singleton instance of embdedded python interpreter.
                 * 
                 * Note that if no client holds a currernt reference to this singleton, it will get destroyed
                 * and the next call to getInstance will create and initialize a new scoped interpreter
                 * 
                 * This is required for the simple reason that a traditional static singleton instance has no guarantee
                 * about the destrutction order of resources across multiple compliation units,
                 * and this will cause seg faults if the python interpreter is torn down before the destruction of bound modules
                 * (such as this class' Path module).  If the interpreter is not available when those destructors are called,
                 * a seg fault will occur.  With this implementaiton, the interpreter is guarnateed to exist as long as anything 
                 * referencing it needs it, and then can cleanly clean up internal references before the `py::scoped_interpreter`
                 * guard is destroyed, removing the python interpreter.
                 * 
                 * See the following issues and links for reference:
                 * 
                 * https://cplusplus.com/forum/general/37113/
                 * https://stackoverflow.com/questions/60100922/keeping-python-interpreter-alive-only-during-the-life-of-an-object-instance
                 * https://github.com/pybind/pybind11/issues/1598
                 * 
                 */
                //Functionally, a global instance variable
                //This can be made a class attribute, but would need to find a place to put a single definition
                //e.g. make a .cpp file
                static std::weak_ptr<InterpreterUtil> _instance;
                std::shared_ptr<InterpreterUtil> instance = _instance.lock();
                if(!instance){
                    //instance is null
                    InterpreterUtil* pt = new InterpreterUtil();
                    instance = std::shared_ptr<InterpreterUtil>(pt, Deleter{});
                    //update the weak ref
                    _instance = instance;
                }
                return instance;
            }

            struct Deleter {
                /**
                 * @brief Custom deleter functor to provide access to private destructor of singleton
                 * 
                 * In theory, could probably safely move the destructor to public scope as well...
                 * 
                 * @param ptr 
                 */
                void operator()(InterpreterUtil* ptr){delete ptr;}
            };
            friend Deleter;

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

            InterpreterUtil(InterpreterUtil const &instance) = delete;

            void operator=(InterpreterUtil const &instance) = delete;

            /**
             * Add to the Python system path of the singleton instance.
             *
             * @param directoryPath The desired directory to add to the Python system path.
             */
            static void addToPyPath(const std::string &directoryPath) {
                getInstance()->addToPath(directoryPath);
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
             * Return bound Python top level module handle from the singleton instance, importing the module if needed.
             *
             * Return a handle to the given top level Python module or package, importing if necessary and adding the
             * handle to the singleton's maintained collection of import handles for its embedded interpreter.  Standard
             * modules/packages and namespace package names are supported.
             *
             * @param name Name of the desired top level module or package.
             * @return Handle to the desired top level Python module.
             */
            static py::object getPyModule(const std::string &name) {
                return getInstance()->getModule(name);
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
            static py::object getPyModule(const std::vector<std::string> &moduleLevelNames) {
                return getInstance()->getModule(moduleLevelNames);
            }

            /**
             * Return a bound handle to a top-level Python module, importing the module if necessary.
             *
             * Return a handle to the given top level Python module or package, importing if necessary and adding the
             * handle to the internally maintained collection of import handles for the embedded interpreter.  Standard
             * modules/packages and namespace package names are supported.
             *
             * @param name Name of the desired top level module or namespace package.
             * @return Handle to the desired top level Python module.
             */
            py::object getModule(const std::string &name) {
                if (!isImported(name)) {
                    importTopLevelModule(name);
                }
                return importedTopLevelModules.find(name)->second;
            }

            /**
             * Return a bound handle to a Python module or type, importing the top-level module if necessary.
             *
             * E.g., a request for a handle to the ``Path`` class from the ``pathlib`` package module should be made
             * using a vector ``{"pathlib", "Path"}``.
             *
             * @param moduleLevelNames Full name of the desired module, as a vector of the names of the top-level module
             *                         and (when applicable) submodules.
             * @return Handle to the desired Python module or type.
             */
            py::object getModule(const std::vector<std::string> &moduleLevelNames) {
                // Start with top-level module name, then descend through attributes as needed
                py::object module = getModule(moduleLevelNames[0]);
                for (size_t i = 1; i < moduleLevelNames.size(); ++i) {
                    module = module.attr(moduleLevelNames[i].c_str());
                }
                return module;
            }

        protected:

            /**
             * Search for and return a recognized virtual env directory.
             *
             * Both ``.venv`` and ``venv`` will be recognized.  Search locations are the current working directory, one
             * level up (parent directory), and two levels up, with the first find being return.
             *
             * A Python ``None`` object is returned if no existing directory is found in the search locations.
             *
             * @return A Python Path object for a found venv dir, or a Python ``None`` object.
             */
            py::object searchForVenvDir() {
                py::object current_dir = Path.attr("cwd")();
                std::vector<py::object> parent_options = {
                        current_dir,
                        current_dir.attr("parent"),
                        current_dir.attr("parent").attr("parent")
                };
                std::vector<std::string> dir_name_options = {".venv", "venv"};
                for (py::object &parent_option : parent_options) {
                    for (const std::string &dir_name_option : dir_name_options) {
                        py::object venv_dir_candidate = parent_option.attr("joinpath")(dir_name_option);
                        if (py::bool_(venv_dir_candidate.attr("is_dir")())) {
                            return venv_dir_candidate;
                        }
                    }
                }
                return py::none();
            }

            /**
             * Find any virtual environment site packages directory, starting from options under the current directory.
             *
             * @return The absolute path of the site packages directory, as a string.
             */
            py::list getVenvPackagesDirOptions() {
                // Look for a local virtual environment directory also, if there is one
                const char* env_var_venv = std::getenv("VIRTUAL_ENV");
                py::object venv_dir = env_var_venv != nullptr ? Path(env_var_venv): searchForVenvDir();

                if (!venv_dir.is_none() && py::bool_(venv_dir.attr("is_dir")())) {
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
             * Get whether a Python module is imported.
             *
             * Note that anything that has had its top level module imported already will itself be considered imported,
             * because of the way workings of importing modules in this type using pybind11.  E.g., if ``Path`` from
             * ``pathlib`` was imported, ``PurePath`` would return as imported as well.
             *
             * @param moduleLevelNames Full name of the desired module, as a vector of the names of the top-level module
             *                         and (when applicable) submodules.
             * @return Whether an Python module is imported.
             */
            inline bool isImported(const std::vector<std::string> &moduleLevelNames) {
                return isImported(moduleLevelNames[0]);
            }

            /**
             * Get whether a top level Python module is imported.
             *
             * @param name The name of the module/package.
             * @return Whether an Python module is imported.
             */
            inline bool isImported(const std::string &name) {
                return importedTopLevelModules.find(name) != importedTopLevelModules.end();
            }

        private:
            //List this FIRST, to ensure it isn't destroyed before anything else (e.g. Path) that might
            //need a valid interpreter in its destructor calls.
            // This is required for the Python interpreter and must be kept alive
            std::shared_ptr<py::scoped_interpreter> guardPtr;

            std::map<std::string, py::object> importedTopLevelModules;

            py::object Path;

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

        };
    }
}

#endif // ACTIVATE_PYTHON

#endif // NGEN_INTERPRETERUTIL_HPP
