#include <gtest/gtest.h>

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "realizations/config/output.hpp"

using realization::config::Output;
using realization::config::OutputDomain;
using realization::config::OutputFormat;
using realization::config::OutputGrouping;
using realization::config::rank_output_root;

namespace {
    boost::property_tree::ptree parse(const std::string& json) {
        boost::property_tree::ptree tree;
        std::stringstream ss(json);
        boost::property_tree::json_parser::read_json(ss, tree);
        return tree;
    }
}

// Defaults when no output settings are present at all.
TEST(Output_Config_Test, DefaultsWhenAbsent)
{
    auto out = Output::from_realization(parse(R"({"time": {}})"));
    EXPECT_FALSE(out.from_legacy_keys);
    EXPECT_EQ(out.root, "./");   // an unset root resolves to "./"
    EXPECT_TRUE(out.catchment.enable);
    EXPECT_TRUE(out.nexus.enable);
    EXPECT_EQ(out.catchment.format, OutputFormat::csv);
    EXPECT_EQ(out.nexus.format, OutputFormat::csv);
    EXPECT_EQ(out.catchment.grouping, OutputGrouping::per_feature);
    EXPECT_EQ(out.nexus.grouping, OutputGrouping::per_feature);
    EXPECT_FALSE(out.catchment.rank_subdir);   // rank subdirectories are opt-in
    EXPECT_FALSE(out.nexus.rank_subdir);
}

// The modern "output" block is parsed across both domains, including grouping and rank_subdir.
TEST(Output_Config_Test, ParsesModernBlock)
{
    const std::string json = R"({
        "output": {
            "root": "/tmp/out/",
            "catchment": { "enable": false, "format": "csv", "grouping": "per_formulation", "rank_subdir": true },
            "nexus":     { "enable": true,  "format": "netcdf", "grouping": "per_feature" }
        }
    })";
    auto out = Output::from_realization(parse(json));
    EXPECT_FALSE(out.from_legacy_keys);
    EXPECT_EQ(out.root, "/tmp/out/");
    EXPECT_FALSE(out.catchment.enable);
    EXPECT_EQ(out.catchment.grouping, OutputGrouping::per_formulation);
    EXPECT_TRUE(out.catchment.rank_subdir);
    EXPECT_TRUE(out.nexus.enable);
    EXPECT_EQ(out.nexus.format, OutputFormat::netcdf);
    EXPECT_EQ(out.nexus.grouping, OutputGrouping::per_feature);
    EXPECT_FALSE(out.nexus.rank_subdir);
}

// Precision: DEFAULT_PRECISION when unset; an explicit value in the modern block is parsed.
TEST(Output_Config_Test, PrecisionDefaultsAndParses)
{
    Output defaulted = Output::from_realization(parse(R"({"time": {}})"));
    EXPECT_EQ(defaulted.precision, Output::DEFAULT_PRECISION);

    Output configured_a = Output::from_realization(parse(R"({"output": {"precision": 4}})"));
    EXPECT_EQ(configured_a.precision, 4);

    Output configured_b = Output::from_realization(parse(R"({"output": {"precision": 12}})"));
    EXPECT_EQ(configured_b.precision, 12);
}

// Legacy top-level keys map onto the new structure and flag deprecation.
TEST(Output_Config_Test, LegacyKeysMapped)
{
    const std::string json = R"({
        "output_root": "./legacy_dir",
        "disable_catchment_output": true,
        "per_formulation_nexus_files": true
    })";
    auto out = Output::from_realization(parse(json));
    EXPECT_TRUE(out.from_legacy_keys);
    EXPECT_EQ(out.root, "./legacy_dir/");                // resolved: normalized to a trailing slash
    EXPECT_FALSE(out.catchment.enable);                  // disable_catchment_output: true
    EXPECT_EQ(out.nexus.format, OutputFormat::netcdf);   // per_formulation_nexus_files: true
}

// Legacy disable_catchment_output=false explicitly keeps catchment output enabled.
TEST(Output_Config_Test, LegacyDisableCatchmentFalseKeepsEnabled)
{
    auto out = Output::from_realization(parse(R"({ "disable_catchment_output": false })"));
    EXPECT_TRUE(out.from_legacy_keys);
    EXPECT_TRUE(out.catchment.enable);
}

// The modern block takes precedence; legacy keys are ignored when it is present.
TEST(Output_Config_Test, ModernBlockWinsOverLegacy)
{
    const std::string json = R"({
        "output_root": "./legacy_dir",
        "disable_catchment_output": true,
        "output": { "root": "./modern_dir" }
    })";
    auto out = Output::from_realization(parse(json));
    EXPECT_FALSE(out.from_legacy_keys);
    EXPECT_EQ(out.root, "./modern_dir/");   // resolved: normalized to a trailing slash
    EXPECT_TRUE(out.catchment.enable);   // legacy disable ignored
}

// Invalid enum values are rejected with a clear error.
TEST(Output_Config_Test, InvalidValuesThrow)
{
    EXPECT_THROW(Output::from_realization(parse(R"({"output":{"nexus":{"format":"parquet"}}})")),
                 std::runtime_error);
    EXPECT_THROW(Output::from_realization(parse(R"({"output":{"catchment":{"grouping":"per_node"}}})")),
                 std::runtime_error);
}

// rank_output_root: serial runs are never rank-partitioned, regardless of settings.
TEST(Output_Config_Test, RankRootUnchangedInSerial)
{
    OutputDomain d;
    d.rank_subdir = true;
    d.grouping = OutputGrouping::per_formulation;
    EXPECT_EQ(rank_output_root("/out/", d, /*rank*/0, /*num_procs*/1), "/out/");
}

// rank_output_root: per_feature only nests under rank_<N>/ when rank_subdir is set (MPI).
TEST(Output_Config_Test, RankRootPerFeatureHonorsFlag)
{
    OutputDomain d;   // per_feature, rank_subdir defaults false
    EXPECT_EQ(rank_output_root("/out/", d, 3, 4), "/out/");
    d.rank_subdir = true;
    EXPECT_EQ(rank_output_root("/out/", d, 3, 4), "/out/rank_3/");
}

// rank_output_root: per_formulation forces rank_<N>/ under MPI even with rank_subdir off,
// so ranks never collide on a shared aggregated file.
TEST(Output_Config_Test, RankRootPerFormulationForcesSubdir)
{
    OutputDomain d;
    d.grouping = OutputGrouping::per_formulation;
    d.rank_subdir = false;
    EXPECT_EQ(rank_output_root("/out/", d, 2, 8), "/out/rank_2/");
}
