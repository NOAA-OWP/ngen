#include <NGenConfig.h>

#include <ostream>
#include <string>
#include <unordered_map>
#include <cstdlib>

#if NGEN_WITH_PYTHON
#include <pybind11/embed.h>
namespace py = pybind11;
#endif // NGEN_WITH_PYTHON

void ngen::exec_info::runtime_summary(std::ostream& stream) noexcept
{
    stream << "Runtime configuration summary:\n";

#if NGEN_WITH_PYTHON // -------------------------------------------------------
    { // START RAII
        py::scoped_interpreter guard{};

        auto sys       = py::module_::import("sys");
        auto sysconfig = py::module_::import("sysconfig");

        // try catch
        py::module_ numpy;
        bool imported_numpy = false;
        std::string err;
        try {
            numpy = py::module_::import("numpy");
            imported_numpy = true;
        } catch(py::error_already_set& e) {
            err = e.what();
        }

        // Lambda to convert py::dict -> std::unordered_map<std::string, std::string>
        const auto dict_to_map = [](const py::dict& dict) -> std::unordered_map<std::string, std::string> {
            std::unordered_map<std::string, std::string> map;
            for (const auto& kv : dict)
                map[kv.first.cast<std::string>()] = kv.second.cast<std::string>();

            return map;
        };

        const auto python_paths = dict_to_map(sysconfig.attr("get_paths")().cast<py::dict>());
        const auto python_venv = std::getenv("VIRTUAL_ENV") == nullptr ? "<none>" : std::getenv("VIRTUAL_ENV");

        stream << "  Python:\n"
               << "    Version: "         << sys.attr("version").cast<std::string>() << "\n"
               << "    Virtual Env: "     << python_venv                 << "\n"
               << "    Executable: "      << sys.attr("executable").cast<std::string>() << "\n"
               << "    Site Library: "    << python_paths.at("purelib")  << "\n"
               << "    Include: "         << python_paths.at("include")  << "\n"
               << "    Runtime Library: " << python_paths.at("stdlib")   << "\n";

        if (imported_numpy) {
            stream << "    NumPy Version: "   << numpy.attr("version").attr("version").cast<std::string>() << "\n"
                   << "    NumPy Include: "   << numpy.attr("get_include")().cast<std::string>() << "\n";
        } else {
            // Output NumPy import error
            stream << "    NumPy: " << err << "\n";
        }

#if NGEN_WITH_ROUTING

        // TODO: Maybe hash the package sources?
        //
        // In site-packages, the RECORD file for dist contains
        // hashes generated for all files -- maybe parse this and
        // pull a combined hash?

#endif // NGEN_WITH_ROUTING
    } // END RAII
#endif // NGEN_WITH_PYTHON // -------------------------------------------------
}

void ngen::exec_info::runtime_usage(const std::string& cmd, std::ostream& stream) noexcept
{
    stream << "Usage: " << std::endl;
    stream << cmd << " <catchment_data_path> <catchment subset ids> <nexus_data_path> <nexus subset ids>"
           << " <realization_config_path>" << std::endl
           << "Arguments for <catchment subset ids> and <nexus subset ids> must be given." << std::endl
           << "Use \"all\" as explicit argument when no subset is needed." << std::endl;
}
