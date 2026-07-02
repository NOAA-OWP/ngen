#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>   // getpid

#include <CatchmentCsvOutputMgr.hpp>

namespace {
    std::vector<std::string> read_lines(const std::filesystem::path& p) {
        std::vector<std::string> lines;
        std::ifstream in(p);
        std::string line;
        while (std::getline(in, line)) lines.push_back(line);
        return lines;
    }

    utils::time_marker marker(long index, const std::string& stamp) {
        return utils::time_marker(index, 0, stamp);
    }

    // A column whose output name is its source and has no units -- terse shorthand for building test
    // column lists (production OutputField construction states source, output, and units explicitly).
    utils::OutputField col(const std::string& name) { return {name, name, std::nullopt}; }

    // A fresh, empty temp directory unique to this process (and each call), so tests never collide
    // with a concurrently running test binary or with each other over fixed file names.
    std::filesystem::path fresh_dir() {
        static int counter = 0;
        std::filesystem::path d = std::filesystem::temp_directory_path()
            / ("ngen_cat_csv_test_" + std::to_string(getpid())) / std::to_string(counter++);
        std::filesystem::remove_all(d);
        std::filesystem::create_directories(d);
        return d;
    }
}

// Aggregated: all catchments share one file, the header (with the id column) is
// written once, and every row carries the full catchment id. Values are joined
// from the typed vector at the manager's configured precision.
TEST(CatchmentCsvOutputMgr_Test, AggregatedSharesFileHeaderOnce)
{
    const auto dir  = fresh_dir();
    const std::string root  = (dir / "").string();
    const std::string fname = "ngen_cat_agg_mgr_test.csv";

    {
        utils::CatchmentCsvOutputMgr mgr(root, fname, /*precision=*/6,
                                         { {"cat-1", {col("Q_OUT")}},
                                           {"cat-2", {col("Q_OUT")}} });   // header written once
        mgr.receive_data_entry("cat-1", marker(0, "t0"), {1.5});
        mgr.receive_data_entry("cat-2", marker(0, "t0"), {2.25});
        mgr.receive_data_entry("cat-1", marker(1, "t1"), {3.5});
        mgr.commit_writes();
        mgr.close();
        EXPECT_TRUE(mgr.is_closed());
    }

    const auto lines = read_lines(dir / fname);
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(lines[0], "catchment_id,Time Step,Time,Q_OUT");
    EXPECT_EQ(lines[1], "cat-1,0,t0,1.5");
    EXPECT_EQ(lines[2], "cat-2,0,t0,2.25");
    EXPECT_EQ(lines[3], "cat-1,1,t1,3.5");

    std::filesystem::remove_all(dir);
}

// Aggregated with more than one formulation: each formulation's catchments aggregate into their
// own file (per-formulation subdirectory), so every file's header is that formulation's own column
// set -- distinct formulations with different columns never share a file or a header. Rows route to
// the file for the (formulation, catchment) they were received under.
TEST(CatchmentCsvOutputMgr_Test, AggregatedSplitsByFormulation)
{
    const auto dir = fresh_dir();
    const std::string root = (dir / "").string();
    const std::string fname = "agg.csv";

    {
        utils::CatchmentCsvOutputMgr mgr(root, fname, /*precision=*/6,
                                         { {"formA", "cat-1", {col("Q_OUT")}},
                                           {"formA", "cat-2", {col("Q_OUT")}},
                                           {"formB", "cat-3", {col("Q_OUT"), col("ET")}} });   // different columns
        mgr.receive_data_entry("formA", "cat-1", marker(0, "t0"), {1.0});
        mgr.receive_data_entry("formB", "cat-3", marker(0, "t0"), {2.0, 0.5});
        mgr.receive_data_entry("formA", "cat-2", marker(0, "t0"), {3.0});
        mgr.close();
    }

    const auto a = read_lines(dir / "formA" / fname);
    const auto b = read_lines(dir / "formB" / fname);
    ASSERT_EQ(a.size(), 3u);                                     // header + two catchments
    EXPECT_EQ(a[0], "catchment_id,Time Step,Time,Q_OUT");       // formA's single-column schema
    EXPECT_EQ(a[1], "cat-1,0,t0,1");
    EXPECT_EQ(a[2], "cat-2,0,t0,3");
    ASSERT_EQ(b.size(), 2u);                                     // header + one catchment
    EXPECT_EQ(b[0], "catchment_id,Time Step,Time,Q_OUT,ET");    // formB's own two-column schema
    EXPECT_EQ(b[1], "cat-3,0,t0,2,0.5");

    std::filesystem::remove_all(dir);
}

// Per-feature: one file per catchment, each with its own header; multiple
// columns join in order.
TEST(CatchmentCsvOutputMgr_Test, PerFeatureOneFilePerCatchment)
{
    const auto dir = fresh_dir();
    const std::string root = (dir / "").string();

    {
        utils::CatchmentCsvOutputMgr mgr(root, std::nullopt, /*precision=*/6,
                                         { {"cat-1", {col("Q_OUT"), col("ET")}},
                                           {"cat-2", {col("Q_OUT"), col("ET")}} });
        mgr.receive_data_entry("cat-1", marker(0, "t0"), {1.5, 0.25});
        mgr.receive_data_entry("cat-2", marker(0, "t0"), {2.25, 0.5});
        mgr.close();
    }

    const auto l1 = read_lines(dir / "cat-1.csv");
    const auto l2 = read_lines(dir / "cat-2.csv");
    ASSERT_EQ(l1.size(), 2u);
    EXPECT_EQ(l1[0], "Time Step,Time,Q_OUT,ET");
    EXPECT_EQ(l1[1], "0,t0,1.5,0.25");
    ASSERT_EQ(l2.size(), 2u);
    EXPECT_EQ(l2[1], "0,t0,2.25,0.5");

    std::filesystem::remove_all(dir);
}

// A non-default formulation id routes output into a per-formulation subdirectory, so independent
// formulation setups over the same catchment id do not collide. The default id stays flat.
TEST(CatchmentCsvOutputMgr_Test, PerFormulationSubdirectoryAvoidsCollisions)
{
    const auto dir = fresh_dir();
    const std::string root = (dir / "").string();

    {
        utils::CatchmentCsvOutputMgr mgr(root, std::nullopt, /*precision=*/6,
                                         { {"formA", "cat-1", {col("Q_OUT")}},
                                           {"formB", "cat-1", {col("Q_OUT")}},
                                           {"cat-1", {col("Q_OUT")}} });   // default id -> flat layout
        mgr.receive_data_entry("formA", "cat-1", marker(0, "t0"), {1.0});
        mgr.receive_data_entry("formB", "cat-1", marker(0, "t0"), {2.0});
        mgr.receive_data_entry("cat-1", marker(0, "t0"), {3.0});    // default id
        mgr.close();
    }

    const auto a = read_lines(dir / "formA" / "cat-1.csv");
    const auto b = read_lines(dir / "formB" / "cat-1.csv");
    const auto d = read_lines(dir / "cat-1.csv");
    ASSERT_EQ(a.size(), 2u);
    EXPECT_EQ(a[1], "0,t0,1");
    ASSERT_EQ(b.size(), 2u);
    EXPECT_EQ(b[1], "0,t0,2");
    ASSERT_EQ(d.size(), 2u);
    EXPECT_EQ(d[1], "0,t0,3");

    std::filesystem::remove_all(dir);
}

// Once closed, receiving more data is an error (mirrors NexusOutputsMgr semantics).
TEST(CatchmentCsvOutputMgr_Test, ThrowsAfterClose)
{
    const auto dir = fresh_dir();
    const std::string root = (dir / "").string();

    utils::CatchmentCsvOutputMgr mgr(root, std::nullopt, /*precision=*/6, { {"cat-1", {col("Q_OUT")}} });
    mgr.close();
    EXPECT_TRUE(mgr.is_closed());
    EXPECT_THROW(mgr.receive_data_entry("cat-1", marker(0, "t0"), {1.0}), std::runtime_error);

    std::filesystem::remove_all(dir);
}

// Receiving data for a (formulation, catchment) that was not in the set the manager was constructed
// with is an error: the manager only writes the features it was told about up front.
TEST(CatchmentCsvOutputMgr_Test, ThrowsForUnregisteredCatchment)
{
    const auto dir = fresh_dir();
    const std::string root = (dir / "").string();

    utils::CatchmentCsvOutputMgr mgr(root, std::nullopt, /*precision=*/6, { {"cat-1", {col("Q_OUT")}} });
    EXPECT_THROW(mgr.receive_data_entry("cat-unknown", marker(0, "t0"), {1.0}), std::runtime_error);

    std::filesystem::remove_all(dir);
}

// Values are rendered at the manager's configured significant-digit precision (set once on each
// stream at open). A value with more significant digits than the precision is rounded to it.
TEST(CatchmentCsvOutputMgr_Test, ValuesRenderAtConfiguredPrecision)
{
    const auto dir = fresh_dir();
    const std::string root = (dir / "").string();

    {
        utils::CatchmentCsvOutputMgr mgr(root, std::nullopt, /*precision=*/6, { {"cat-1", {col("A"), col("B")}} });
        mgr.receive_data_entry("cat-1", marker(0, "t0"), {1.0 / 3.0, 2.0 / 3.0});
        mgr.close();
    }

    const auto l = read_lines(dir / "cat-1.csv");
    ASSERT_EQ(l.size(), 2u);
    EXPECT_EQ(l[1], "0,t0,0.333333,0.666667");   // 6 significant digits; the second value rounds up

    std::filesystem::remove_all(dir);
}
