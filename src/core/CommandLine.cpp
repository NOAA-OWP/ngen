#include <CommandLine.hpp>

#include <boost/algorithm/string.hpp>

#include <cstring>
#include <string>

#if NGEN_WITH_MPI
#ifndef MPI_HF_SUB_CLI_FLAG
#define MPI_HF_SUB_CLI_FLAG "--subdivided-hydrofabric"
#endif
#endif // NGEN_WITH_MPI

namespace ngen {
namespace driver {

namespace {
//! Split a comma-separated id list. "all", or an empty argument, yields no
//! subset (an empty vector).
std::vector<std::string> split_subset_ids(const std::string& arg)
{
    std::vector<std::string> ids;
    boost::split(ids, arg, [](char c) { return c == ','; });
    // A single id or no id leaves the vector with one element: the id or "".
    if (ids.size() == 1 && (ids[0] == "all" || ids[0].empty())) {
        ids.pop_back();
    }
    return ids;
}
} // namespace

CommandLineParse parse(int argc, char* argv[], [[maybe_unused]] int process_count)
{
    CommandLineParse result;

    if (argc > 1 && std::string{argv[1]} == "--info") {
        result.action = CommandLineAction::show_info;
        return result;
    }

    if (argc < 2) {
        result.action = CommandLineAction::show_usage;
        return result;
    }

    if (argc < 6) {
        result.action = CommandLineAction::missing_arguments;
        return result;
    }

    CommandLine& cli = result.command_line;
    cli.catchment_data_path     = argv[1];
    cli.nexus_data_path         = argv[3];
    cli.realization_config_path = argv[5];
    cli.catchment_subset_ids    = split_subset_ids(argv[2]);
    cli.nexus_subset_ids        = split_subset_ids(argv[4]);

#if NGEN_WITH_MPI
    if (argc >= 7) {
        cli.partition_path = argv[6];
    }
    else if (process_count > 1) {
        result.action = CommandLineAction::invalid_arguments;
        result.message = "Missing required argument for partition file path.";
        return result;
    }

    if (argc >= 8) {
        if (std::strcmp(argv[7], MPI_HF_SUB_CLI_FLAG) == 0) {
            cli.subdivided_hydrofabric_requested = true;
        }
        else if (process_count > 1) {
            result.action = CommandLineAction::invalid_arguments;
            result.message =
                std::string("Unexpected arg '") + argv[7] + "'; try " + MPI_HF_SUB_CLI_FLAG;
            return result;
        }
    }
#endif // NGEN_WITH_MPI

    result.action = CommandLineAction::run;
    return result;
}

} // namespace driver
} // namespace ngen
