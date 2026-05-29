#ifndef NGEN_DRIVER_COMMAND_LINE_HPP
#define NGEN_DRIVER_COMMAND_LINE_HPP

#include <NGenConfig.h>

#include <string>
#include <vector>

namespace ngen {
namespace driver {

//! What the driver should do after inspecting the command line.
enum class CommandLineAction {
    run,                //!< Inputs parsed; proceed with a simulation run.
    show_info,          //!< "--info" given: report configuration, then stop.
    show_usage,         //!< No arguments given: print usage/help, then stop.
    missing_arguments,  //!< Too few arguments given: report, then stop.
    invalid_arguments   //!< An argument is malformed/inconsistent (see message).
};

/**
 * Inputs parsed from the command line for a NextGen run.
 *
 * Positional arguments of the driver invocation:
 *   1: catchment data file path
 *   2: catchment subset ids (comma-separated, no spaces; "all" for no subset)
 *   3: nexus data file path
 *   4: nexus subset ids (comma-separated, no spaces; "all" for no subset)
 *   5: realization config file path
 *   6: partition config file path (required for multi-process parallel runs)
 *   7: optional flag requesting on-the-fly hydrofabric subdivision
 */
struct CommandLine {
    std::string catchment_data_path;
    std::string nexus_data_path;
    std::string realization_config_path;
    std::string partition_path;                    //!< Empty when not given.
    std::vector<std::string> catchment_subset_ids; //!< Empty means no subset (all).
    std::vector<std::string> nexus_subset_ids;     //!< Empty means no subset (all).
    bool subdivided_hydrofabric_requested = false;
};

/**
 * The outcome of parse(): the action the driver should take, the parsed inputs
 * (meaningful when action == run), and a human-readable message (populated for
 * the invalid_arguments action).
 */
struct CommandLineParse {
    CommandLineAction action = CommandLineAction::run;
    CommandLine command_line;
    std::string message;
};

/**
 * Classify and parse the process command line.
 *
 * Performs no I/O and never terminates the process: it inspects argv, decides
 * what the driver should do, and -- for a run -- fills in the parsed inputs.
 *
 * @param process_count The number of parallel processes. When greater than one,
 *        a partition config argument is required; ignored in non-MPI builds.
 */
CommandLineParse parse(int argc, char* argv[], int process_count);

} // namespace driver
} // namespace ngen

#endif // NGEN_DRIVER_COMMAND_LINE_HPP
