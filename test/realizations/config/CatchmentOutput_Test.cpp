#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>   // getpid

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "realizations/config/output.hpp"
#include "realizations/config/catchment_output.hpp"
#include <CatchmentOutputsMgr.hpp>

// Unit tests for the catchment output factory (realization::config::make_catchment_output_mgr):
// that it selects the manager's mode from the configuration. The CSV layout details themselves
// (header-once, precision, id column, per-formulation subdirs) are covered by
// CatchmentCsvOutputMgr_Test; config parsing by Output_Test. Here we assert only the selection,
// so no Formulation_Manager or BMI model is involved -- catchments are registered with explicit
// fields.

using realization::config::Output;
using realization::config::OutputFormat;
using realization::config::make_catchment_output_mgr;

namespace {
    // A column whose output name is its source and has no units -- terse shorthand for building test
    // column lists (production OutputField construction states source, output, and units explicitly).
    utils::OutputField col(const std::string& name) { return {name, name, std::nullopt}; }

    Output config_with(const std::string& root, const std::string& catchment_grouping) {
        std::stringstream ss(std::string("{\"output\":{\"root\":\"") + root
            + "\",\"catchment\":{\"grouping\":\"" + catchment_grouping + "\"}}}");
        boost::property_tree::ptree tree;
        boost::property_tree::json_parser::read_json(ss, tree);
        return Output::from_realization(tree);   // normalizes the root (the manager creates it on use)
    }

    std::vector<std::string> read_lines(const std::filesystem::path& p) {
        std::vector<std::string> lines;
        std::ifstream in(p);
        std::string line;
        while (std::getline(in, line)) lines.push_back(line);
        return lines;
    }

    // The parent for this binary's temp output roots. PID-scoped so concurrent test binaries don't
    // collide over fixed names.
    std::filesystem::path pid_test_root() {
        return std::filesystem::temp_directory_path() / ("ngen_cat_factory_test_" + std::to_string(getpid()));
    }

    // A unique output root (trailing slash) under pid_test_root(). Not created here -- the manager
    // creates it, which is part of what we exercise; TempDirCleanup owns wiping the tree.
    std::string fresh_root() {
        static int counter = 0;
        return (pid_test_root() / std::to_string(counter++) / "").string();
    }

    // Own the PID-scoped temp tree's lifecycle: clear any residue from a prior same-PID run up front,
    // and remove this run's tree at exit (which also covers a root left behind by a test that failed
    // an assertion before its own cleanup). This is the only thing that touches the tree wholesale.
    struct TempDirCleanup : ::testing::Environment {
        void SetUp() override    { std::filesystem::remove_all(pid_test_root()); }
        void TearDown() override { std::filesystem::remove_all(pid_test_root()); }
    };
    const auto* temp_dir_cleanup = ::testing::AddGlobalTestEnvironment(new TempDirCleanup);
}

// per_formulation grouping -> aggregated manager: one shared file with a leading catchment_id
// column, not one file per catchment. The aggregated file name defaults (cat_output.csv) since the
// caller supplies none.
TEST(CatchmentOutput_Factory_Test, PerFormulationConfigSelectsAggregatedFile)
{
    namespace fs = std::filesystem;
    const std::string root = fresh_root();

    {
        auto mgr = make_catchment_output_mgr(config_with(root, "per_formulation"),
                                             { {"cat-1", {col("Q_OUT")}},
                                               {"cat-2", {col("Q_OUT")}} });
        mgr->commit_writes();
        mgr->close();
    }

    ASSERT_TRUE(fs::exists(root + "cat_output.csv"));         // single aggregated file
    const auto lines = read_lines(root + "cat_output.csv");
    ASSERT_EQ(lines.size(), 1u);                             // header written once, no data pushed
    EXPECT_EQ(lines[0].rfind("catchment_id,", 0), 0u);      // leading catchment_id column
    EXPECT_FALSE(fs::exists(root + "cat-1.csv"));            // not per-feature
    EXPECT_FALSE(fs::exists(root + "cat-2.csv"));
}

// per_feature (no file name) -> per-feature manager: one file per catchment, no aggregated file
// and no catchment_id column.
TEST(CatchmentOutput_Factory_Test, PerFeatureConfigSelectsFilePerCatchment)
{
    namespace fs = std::filesystem;
    const std::string root = fresh_root();

    {
        auto mgr = make_catchment_output_mgr(config_with(root, "per_feature"),
                                             { {"cat-1", {col("Q_OUT")}},
                                               {"cat-2", {col("Q_OUT")}} });
        mgr->commit_writes();
        mgr->close();
    }

    ASSERT_TRUE(fs::exists(root + "cat-1.csv"));              // one file per catchment
    ASSERT_TRUE(fs::exists(root + "cat-2.csv"));
    const auto lines = read_lines(root + "cat-1.csv");
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0].rfind("Time Step,", 0), 0u);         // no catchment_id column
    EXPECT_FALSE(fs::exists(root + "cat_output.csv"));
}

// A supplied file name does not force aggregation: with per_feature grouping the factory stays
// consistent with the config and writes per-feature files, ignoring the name.
TEST(CatchmentOutput_Factory_Test, FileNameDoesNotOverridePerFeatureGrouping)
{
    namespace fs = std::filesystem;
    const std::string root = fresh_root();

    {
        auto mgr = make_catchment_output_mgr(config_with(root, "per_feature"),
                                             { {"cat-1", {col("Q_OUT")}} }, "cat_output.csv");
        mgr->commit_writes();
        mgr->close();
    }

    EXPECT_TRUE(fs::exists(root + "cat-1.csv"));
    EXPECT_FALSE(fs::exists(root + "cat_output.csv"));        // name ignored; grouping wins
}

// A non-CSV catchment output format is not implemented and is rejected.
TEST(CatchmentOutput_Factory_Test, NonCsvFormatThrows)
{
    Output cfg;
    cfg.catchment.format = OutputFormat::netcdf;
    EXPECT_THROW(make_catchment_output_mgr(cfg, {}), std::runtime_error);   // format rejected before use
}
