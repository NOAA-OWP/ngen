#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "PerNexusCsvOutputMgr.hpp"

namespace fs = std::filesystem;
using utils::PerNexusCsvOutputMgr;
using utils::time_marker;

namespace {
    fs::path fresh_temp_dir() {
        fs::path d = fs::temp_directory_path() / "ngen_per_nexus_csv_test";
        fs::remove_all(d);
        fs::create_directories(d);
        return d;
    }
}

// Default behavior: one CSV per nexus, written directly under output_root.
TEST(PerNexusCsvOutputMgr_Test, WritesPerNexusFilesInOutputRoot)
{
    const fs::path root = fresh_temp_dir();
    const std::vector<std::string> ids = {"nex-1", "nex-2"};
    {
        PerNexusCsvOutputMgr mgr(ids, root.string() + "/");
        // Use the base-class convenience overload (as callers do via the
        // NexusOutputsMgr interface); the derived 4-arg override otherwise hides it.
        utils::NexusOutputsMgr& base = mgr;
        base.receive_data_entry("nex-1", time_marker(0, 0, "2020-01-01 00:00:00"), 1.5);
        mgr.commit_writes();

        EXPECT_TRUE(fs::exists(root / "nex-1_output.csv"));
        EXPECT_TRUE(fs::exists(root / "nex-2_output.csv"));

        std::ifstream f((root / "nex-1_output.csv").string());
        std::stringstream ss; ss << f.rdbuf();
        EXPECT_NE(ss.str().find("1.5"), std::string::npos);
    }
    fs::remove_all(root);
}

// rank_subdir layout: the caller folds the rank subdirectory into the effective root (as the driver
// does via rank_output_root); the manager creates that directory and places the per-nexus files
// there.
TEST(PerNexusCsvOutputMgr_Test, CreatesAndUsesPerRankSubdirectory)
{
    const fs::path root = fresh_temp_dir();
    const std::vector<std::string> ids = {"nex-1"};
    {
        PerNexusCsvOutputMgr mgr(ids, root.string() + "/rank_7/");
        mgr.commit_writes();

        EXPECT_TRUE(fs::is_directory(root / "rank_7"));
        EXPECT_TRUE(fs::exists(root / "rank_7" / "nex-1_output.csv"));
        EXPECT_FALSE(fs::exists(root / "nex-1_output.csv"));
    }
    fs::remove_all(root);
}
