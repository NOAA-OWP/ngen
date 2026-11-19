#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "realizations/config/output.hpp"
#include "realizations/config/catchment_output.hpp"
#include <CatchmentOutputsMgr.hpp>
#include "test_temp_paths.hpp"

// Unit tests for the catchment output factory (realization::config::make_catchment_output_mgr):
// that it selects the manager's mode from the configuration. The CSV layout details themselves
// (header-once, precision, id column, per-formulation subdirs) are covered by
// CatchmentCsvOutputMgr_Test; config parsing by Output_Test. Here we assert the selection and
// that configured settings (e.g. precision) reach the writer, so no Formulation_Manager or BMI
// model is involved -- catchments are registered with explicit fields.

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
}

// per_formulation grouping -> aggregated manager: one shared file with a leading catchment_id
// column, not one file per catchment. The aggregated file name defaults (cat_output.csv) since the
// caller supplies none.
TEST(CatchmentOutput_Factory_Test, PerFormulationConfigSelectsAggregatedFile)
{
    namespace fs = std::filesystem;
    const std::string root = (ngen::test::test_temp_path("output") / "").string(); // created by the manager

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
    const std::string root = (ngen::test::test_temp_path("output") / "").string(); // created by the manager

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
    const std::string root = (ngen::test::test_temp_path("output") / "").string(); // created by the manager

    {
        auto mgr = make_catchment_output_mgr(config_with(root, "per_feature"),
                                             { {"cat-1", {col("Q_OUT")}} }, "cat_output.csv");
        mgr->commit_writes();
        mgr->close();
    }

    EXPECT_TRUE(fs::exists(root + "cat-1.csv"));
    EXPECT_FALSE(fs::exists(root + "cat_output.csv"));        // name ignored; grouping wins
}

// A non-default "precision" in the output block reaches the writer: values render at the
// configured significant digits (config JSON -> factory -> CSV backend).
TEST(CatchmentOutput_Factory_Test, PrecisionFromConfigReachesWriter)
{
    const std::string root = (ngen::test::test_temp_path("output") / "").string(); // created by the manager

    std::stringstream ss(std::string("{\"output\":{\"root\":\"") + root
        + "\",\"precision\":4,\"catchment\":{\"grouping\":\"per_feature\"}}}");
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(ss, tree);
    Output cfg = Output::from_realization(tree);

    {
        std::shared_ptr<utils::CatchmentOutputsMgr> mgr =
            make_catchment_output_mgr(cfg, { {"cat-1", {col("Q_OUT")}} });
        mgr->receive_data_entry("cat-1", utils::time_marker(0, 0, "t0"), {1.0 / 3.0});
        mgr->commit_writes();
        mgr->close();
    }

    const std::vector<std::string> lines = read_lines(root + "cat-1.csv");
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "0,t0,0.3333");   // 4 significant digits, not the default 9
}

// A non-CSV catchment output format is not implemented and is rejected.
TEST(CatchmentOutput_Factory_Test, NonCsvFormatThrows)
{
    Output cfg;
    cfg.catchment.format = OutputFormat::netcdf;
    EXPECT_THROW(make_catchment_output_mgr(cfg, {}), std::runtime_error);   // format rejected before use
}
