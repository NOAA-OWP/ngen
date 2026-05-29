#include <InputPreparation.hpp>
#include <ParallelEnvironment.hpp>

#include <FileChecker.h>

#include <iostream>
#include <string>

#if NGEN_WITH_MPI
#include "parallel_utils.h"
#endif

namespace ngen {
namespace driver {

bool inputs_are_readable(const CommandLine& command_line)
{
    bool readable =
        utils::FileChecker::file_is_readable(command_line.catchment_data_path, "Catchment data") &&
        utils::FileChecker::file_is_readable(command_line.nexus_data_path, "Nexus data") &&
        utils::FileChecker::file_is_readable(command_line.realization_config_path, "Realization config");

    // A partition config is only ever set for parallel runs; check it when present.
    if (!command_line.partition_path.empty()) {
        readable = readable &&
            utils::FileChecker::file_is_readable(command_line.partition_path, "Partition config");
    }

    return readable;
}

PreparedInputs prepare_inputs(const CommandLine& command_line,
                              [[maybe_unused]] const ParallelEnvironment& parallel_env)
{
    PreparedInputs prepared;
    prepared.catchment_data_path = command_line.catchment_data_path;
    prepared.nexus_data_path = command_line.nexus_data_path;

    bool error = !inputs_are_readable(command_line);

#if NGEN_WITH_MPI
    // When requested, ensure the hydrofabric is subdivided per partition (either
    // already, or by subdividing now), then use the rank-specific files.
    if (command_line.subdivided_hydrofabric_requested) {
        if (parallel::is_hydrofabric_subdivided(prepared.catchment_data_path, parallel_env.comm(), true) ||
            parallel::subdivide_hydrofabric(
                parallel_env.comm(),
                prepared.catchment_data_path,
                prepared.nexus_data_path,
                command_line.partition_path
            )) {
            prepared.catchment_data_path += "." + std::to_string(parallel_env.rank());
            prepared.nexus_data_path += "." + std::to_string(parallel_env.rank());
        }
        // If subdivision was needed, was not already done, and could not be done now ...
        else {
            std::cout << "Unable to successfully preprocess hydrofabric files into subdivided files per partition.";
            error = true;
        }
    }
#endif // NGEN_WITH_MPI

    prepared.ok = !error;
    return prepared;
}

} // namespace driver
} // namespace ngen
