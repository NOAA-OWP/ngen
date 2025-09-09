#ifndef NGEN_INTERPRETERUTIL_HPP
#define NGEN_INTERPRETERUTIL_HPP

#include <NGenConfig.h>

#if NGEN_WITH_PYTHON

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
            static std::shared_ptr<InterpreterUtil> getInstance();

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
            InterpreterUtil();

            ~InterpreterUtil() = default;

        public:

            InterpreterUtil(InterpreterUtil const &instance) = delete;

            void operator=(InterpreterUtil const &instance) = delete;

            /**
             * Add to the Python system path of the singleton instance.
             *
             * @param directoryPath The desired directory to add to the Python system path.
             */
            static void addToPyPath(const std::string &directoryPath);

            /**
             * Add to the Python system path of this instance.
             *
             * @param directoryPath The desired directory to add to the Python system path.
             */
            void addToPath(const std::string &directoryPath);

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
            static py::object getPyModule(const std::string &name);

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
            static py::object getPyModule(const std::vector<std::string> &moduleLevelNames);

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
            py::object getModule(const std::string &name);

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
            py::object getModule(const std::vector<std::string> &moduleLevelNames);

        protected:

            /**
             * Find any virtual environment site packages directory, starting from options under the current directory.
             *
             * @return The absolute path of the site packages directory, as a string.
             */
            py::list getVenvPackagesDirOptions();

        public:
            /**
             * Get the current Python interpreter system path, returning in both a Python and C++ format.
             *
             * @return A tuple containing the Python list object and a C++ vector of strings, each representing the
             *         current Python system path.
             */
            std::tuple<py::list, std::vector<std::string>> getSystemPath();

            /**
             * Find any virtual environment site packages directory, starting from options under the current directory.
             *
             * @return The absolute path of the site packages directory, as a string.
             */
            std::string getDiscoveredVenvPath();

        protected:
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
            bool isImported(const std::vector<std::string> &moduleLevelNames);

            /**
             * Get whether a top level Python module is imported.
             *
             * @param name The name of the module/package.
             * @return Whether an Python module is imported.
             */
            bool isImported(const std::string &name);

        private:
            //List this FIRST, to ensure it isn't destroyed before anything else (e.g. Path) that might
            //need a valid interpreter in its destructor calls.
            // This is required for the Python interpreter and must be kept alive
            std::shared_ptr<py::scoped_interpreter> guardPtr;

            std::map<std::string, py::object> importedTopLevelModules;

            py::object Path;

            static const int python_major;
            static const int python_minor;
            static const int python_patch;
            static const char* python_version;

            // NumPy version is not broken down by CMake
            static const char* numpy_version;

            /**
             * Import a specified top level module.
             *
             * This import is done without any checking to see if the module has already been imported.
             *
             * @param topLevelName The name of the desired top level Python module to import.
             */
            void importTopLevelModule(const std::string &topLevelName);
        }; // class InterpreterUtil
    }
}

#endif // NGEN_WITH_PYTHON

#endif // NGEN_INTERPRETERUTIL_HPP
