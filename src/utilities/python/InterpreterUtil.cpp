#include <NGenConfig.h>


#if NGEN_WITH_PYTHON

#include <utilities/python/InterpreterUtil.hpp>
#include <utilities/Logger.hpp>

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <cstdlib>
#include <map>
#include <vector>
#include <tuple>

namespace py = pybind11;

static inline const char* safe(const char* s) {
    return s ? s : "<null>";
}

namespace utils {
    namespace ngenPy {

        using namespace pybind11::literals; // to bring in the `_a` literal

        /* static */ std::shared_ptr<InterpreterUtil> InterpreterUtil::getInstance() {
                /**
                 * @brief Singleton instance of embedded python interpreter.
                 * 
                 * Note that if no client holds a current reference to this singleton, it will get destroyed
                 * and the next call to getInstance will create and initialize a new scoped interpreter
                 * 
                 * This is required for the simple reason that a traditional static singleton instance has no guarantee
                 * about the destruction order of resources across multiple compilation units,
                 * and this will cause seg faults if the python interpreter is torn down before the destruction of bound modules
                 * (such as this class' Path module).  If the interpreter is not available when those destructors are called,
                 * a seg fault will occur.  With this implementation, the interpreter is guaranteed to exist as long as anything
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

        InterpreterUtil::InterpreterUtil() {
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

                py::object python_version_info = importedTopLevelModules["sys"].attr("version_info");
                py::str runtime_python_version = importedTopLevelModules["sys"].attr("version");
                int major = py::int_(python_version_info.attr("major"));
                int minor = py::int_(python_version_info.attr("minor"));
                int patch = py::int_(python_version_info.attr("micro"));
                if (major != python_major
                    || minor != python_minor
                    || patch != python_patch) {
                    std::string throw_msg; throw_msg.assign("Python version mismatch between configure/build ("
                                             + std::string(python_version)
                                             + ") and runtime (" + std::string(runtime_python_version) + ")");
                    LOG(throw_msg, LogLevel::WARNING);
                    throw std::runtime_error(throw_msg);
                }

                importTopLevelModule("numpy");
                py::str runtime_numpy_version = importedTopLevelModules["numpy"].attr("version").attr("version");
                if(std::string(runtime_numpy_version) != numpy_version) {
                    std::string version_str = runtime_numpy_version.cast<std::string>();
                    const char* version_cstr = version_str.c_str();
                    std::string throw_msg; throw_msg.assign("NumPy version mismatch between configure/build ("
                                             + std::string(safe(numpy_version))
                                             + ") and runtime (" + std::string(safe(version_cstr)) + ")");
                    LOG(throw_msg, LogLevel::WARNING);
                    throw std::runtime_error(throw_msg);
                }
            }


            /* static */ void InterpreterUtil::addToPyPath(const std::string &directoryPath) {
                getInstance()->addToPath(directoryPath);
            }

            void InterpreterUtil::addToPath(const std::string &directoryPath) {
                std::tuple<py::list, std::vector<std::string>> sysPathTuple = getSystemPath();
                py::list sys_path = std::get<0>(sysPathTuple);
                std::vector<std::string> sys_path_vector = std::get<1>(sysPathTuple);

                py::object requestedDirPath = Path(directoryPath);
                if (py::bool_(requestedDirPath.attr("is_dir")())) {
                    if (std::find(sys_path_vector.begin(), sys_path_vector.end(), directoryPath) == sys_path_vector.end()) {
#ifdef __APPLE__
                        sys_path.attr("insert")(1, py::str(directoryPath));
#else
                        sys_path.attr("insert")(sys_path_vector.size(), py::str(directoryPath));
#endif
                    }
                }
                else {
                    std::string dirPath = py::str(requestedDirPath);
                    std::string throw_msg; throw_msg.assign("Cannot add non-existing directory '" + dirPath + "' to Python PATH");
                    LOG(throw_msg, LogLevel::WARNING);
                    throw std::runtime_error(throw_msg);
                }
            }

            /* static */ py::object InterpreterUtil::getPyModule(const std::string &name) {
                return getInstance()->getModule(name);
            }

            /* static */ py::object InterpreterUtil::getPyModule(const std::vector<std::string> &moduleLevelNames) {
                return getInstance()->getModule(moduleLevelNames);
            }

            py::object InterpreterUtil::getModule(const std::string &name) {
                if (!isImported(name)) {
                    importTopLevelModule(name);
                }
                return importedTopLevelModules.find(name)->second;
            }

            py::object InterpreterUtil::getModule(const std::vector<std::string> &moduleLevelNames) {
                // Start with top-level module name, then descend through attributes as needed
                py::object module = getModule(moduleLevelNames[0]);
                for (size_t i = 1; i < moduleLevelNames.size(); ++i) {
                    module = module.attr(moduleLevelNames[i].c_str());
                }
                return module;
            }

            py::list InterpreterUtil::getVenvPackagesDirOptions() {
                // Look for a local virtual environment directory also, if there is one
                const char* env_var_venv = std::getenv("VIRTUAL_ENV");
                py::object venv_dir = env_var_venv != nullptr ? Path(env_var_venv): py::none();

                if (!venv_dir.is_none() && py::bool_(venv_dir.attr("is_dir")())) {
                    // Resolve the full path
                    venv_dir = venv_dir.attr("resolve")();
                    // Get options for the packages dir
                    py::list site_packages_options = py::list(venv_dir.attr("glob")("**/site-packages/"));
                    return site_packages_options;
                }

                return py::list();
            }

            std::tuple<py::list, std::vector<std::string>> InterpreterUtil::getSystemPath() {
                py::list sys_path = importedTopLevelModules.find("sys")->second.attr("path");
                std::vector<std::string> sys_path_vector(sys_path.size());
                size_t i = 0;
                for (auto item : sys_path) {
                    sys_path_vector[i++] = py::str(item);
                }
                return std::make_tuple(sys_path, sys_path_vector);
            }

            std::string InterpreterUtil::getDiscoveredVenvPath() {
                // Look for a local virtual environment directory also, if there is one
                const char* env_var_venv = std::getenv("VIRTUAL_ENV");
                if(env_var_venv != nullptr){
                    //std::string r(env_var_venv);
                    return std::string(env_var_venv);
                }
                return std::string("None");
            }

            bool InterpreterUtil::isImported(const std::vector<std::string> &moduleLevelNames) {
                return isImported(moduleLevelNames[0]);
            }

            bool InterpreterUtil::isImported(const std::string &name) {
                return importedTopLevelModules.find(name) != importedTopLevelModules.end();
            }

            void InterpreterUtil::importTopLevelModule(const std::string &topLevelName) {
                try {
                    auto module = py::module_::import(topLevelName.c_str());
                    importedTopLevelModules[topLevelName] = std::move(module);
                }
                catch (std::exception &e) {
                    std::stringstream ss;
                    ss << "importTopLevelModule: " << topLevelName << ": " << e.what() << std::endl;
                    ss << "Already imported modules: ";
                    for (const auto& module : importedTopLevelModules) {
                        ss << module.first << ", ";
                    }
                    LOG(ss.str(), LogLevel::WARNING);
                    throw;
                }
            }
    }
}

#endif // NGEN_WITH_PYTHON
