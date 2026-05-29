#include <gtest/gtest.h>

#include <CommandLine.hpp>

#include <string>
#include <vector>

using ngen::driver::CommandLineAction;

namespace {
//! Build an argv from string arguments and run parse(). The `args` storage must
//! outlive the call, since argv points into it.
ngen::driver::CommandLineParse run_parse(std::vector<std::string>& args, int process_count = 1)
{
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (std::string& arg : args) {
        argv.push_back(arg.data());
    }
    return ngen::driver::parse(static_cast<int>(argv.size()), argv.data(), process_count);
}
} // namespace

TEST(CommandLine_Test, InfoFlagRequestsInfo)
{
    std::vector<std::string> args{"ngen", "--info"};
    EXPECT_EQ(run_parse(args).action, CommandLineAction::show_info);
}

TEST(CommandLine_Test, NoArgumentsShowsUsage)
{
    std::vector<std::string> args{"ngen"};
    EXPECT_EQ(run_parse(args).action, CommandLineAction::show_usage);
}

TEST(CommandLine_Test, TooFewArgumentsReported)
{
    std::vector<std::string> args{"ngen", "cat.gpkg", "all", "nex.gpkg"};
    EXPECT_EQ(run_parse(args).action, CommandLineAction::missing_arguments);
}

TEST(CommandLine_Test, PositionalInputsParsed)
{
    std::vector<std::string> args{
        "ngen", "cat.gpkg", "cat-1,cat-2", "nex.gpkg", "nex-1", "realization.json"};
    const auto result = run_parse(args);
    ASSERT_EQ(result.action, CommandLineAction::run);
    EXPECT_EQ(result.command_line.catchment_data_path, "cat.gpkg");
    EXPECT_EQ(result.command_line.nexus_data_path, "nex.gpkg");
    EXPECT_EQ(result.command_line.realization_config_path, "realization.json");
    EXPECT_EQ(result.command_line.catchment_subset_ids,
              (std::vector<std::string>{"cat-1", "cat-2"}));
    EXPECT_EQ(result.command_line.nexus_subset_ids, (std::vector<std::string>{"nex-1"}));
}

TEST(CommandLine_Test, AllOrEmptyMeansNoSubset)
{
    std::vector<std::string> args{
        "ngen", "cat.gpkg", "all", "nex.gpkg", "", "realization.json"};
    const auto result = run_parse(args);
    ASSERT_EQ(result.action, CommandLineAction::run);
    EXPECT_TRUE(result.command_line.catchment_subset_ids.empty());
    EXPECT_TRUE(result.command_line.nexus_subset_ids.empty());
}

#if NGEN_WITH_MPI
TEST(CommandLine_Test, MultiProcessRequiresPartitionPath)
{
    std::vector<std::string> args{
        "ngen", "cat.gpkg", "all", "nex.gpkg", "all", "realization.json"};
    const auto result = run_parse(args, /*process_count=*/2);
    EXPECT_EQ(result.action, CommandLineAction::invalid_arguments);
    EXPECT_FALSE(result.message.empty());
}

TEST(CommandLine_Test, PartitionPathParsed)
{
    std::vector<std::string> args{
        "ngen", "cat.gpkg", "all", "nex.gpkg", "all", "realization.json", "part.json"};
    const auto result = run_parse(args, /*process_count=*/2);
    ASSERT_EQ(result.action, CommandLineAction::run);
    EXPECT_EQ(result.command_line.partition_path, "part.json");
}

TEST(CommandLine_Test, SubdividedHydrofabricFlagParsed)
{
    std::vector<std::string> args{
        "ngen", "cat.gpkg", "all", "nex.gpkg", "all", "realization.json",
        "part.json", "--subdivided-hydrofabric"};
    const auto result = run_parse(args, /*process_count=*/2);
    ASSERT_EQ(result.action, CommandLineAction::run);
    EXPECT_TRUE(result.command_line.subdivided_hydrofabric_requested);
}

TEST(CommandLine_Test, UnexpectedTrailingArgumentRejected)
{
    std::vector<std::string> args{
        "ngen", "cat.gpkg", "all", "nex.gpkg", "all", "realization.json",
        "part.json", "bogus"};
    const auto result = run_parse(args, /*process_count=*/2);
    EXPECT_EQ(result.action, CommandLineAction::invalid_arguments);
    EXPECT_FALSE(result.message.empty());
}
#endif // NGEN_WITH_MPI
