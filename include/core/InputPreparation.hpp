#ifndef NGEN_DRIVER_INPUT_PREPARATION_HPP
#define NGEN_DRIVER_INPUT_PREPARATION_HPP

#include <NGenConfig.h>
#include <CommandLine.hpp>

#include <string>

namespace ngen {
namespace driver {

class ParallelEnvironment;

/**
 * Check that the input files named in `command_line` exist and are readable:
 * the catchment, nexus, and realization config files, plus the partition config
 * file when one was given. A diagnostic naming the first unreadable file is
 * written to standard output.
 *
 * @return true if every required input file is readable.
 */
bool inputs_are_readable(const CommandLine& command_line);

//! A run's input data file paths, after preparation.
struct PreparedInputs {
    bool ok = false; //!< Whether the inputs are present and usable.
    std::string catchment_data_path;
    std::string nexus_data_path;
};

/**
 * Prepare a run's hydrofabric inputs: confirm the input files are readable
 * (@see inputs_are_readable) and, in MPI builds when subdivision was requested,
 * ensure per-partition subdivided hydrofabric files exist (subdividing now if
 * necessary), yielding the rank-specific catchment and nexus data paths.
 *
 * @return The (possibly rank-suffixed) catchment and nexus data file paths and
 *         whether preparation succeeded.
 */
PreparedInputs prepare_inputs(const CommandLine& command_line,
                              const ParallelEnvironment& parallel_env);

} // namespace driver
} // namespace ngen

#endif // NGEN_DRIVER_INPUT_PREPARATION_HPP
