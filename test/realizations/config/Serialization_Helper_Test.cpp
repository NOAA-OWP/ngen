#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "realizations/config/serialization_helper.hpp"

using realization::config::apply_serialization_path_token_resolution;

namespace {
    boost::property_tree::ptree parse(const std::string& json) {
        boost::property_tree::ptree tree;
        std::stringstream ss(json);
        boost::property_tree::json_parser::read_json(ss, tree);
        return tree;
    }
}

// The top-level `path` field is resolved. Serves as the pre-split
// baseline: `{{rank}}` must produce a numeric substitution (0 in the
// non-MPI test binary) and the literal wrapper text must be preserved.
TEST(Serialization_Helper_Test, ResolvesTopLevelPath)
{
    auto block = parse(R"({ "path": "ckpt.rank_{{rank}}.bin" })");
    apply_serialization_path_token_resolution(block);
    const auto out = block.get<std::string>("path");
    EXPECT_EQ(out, "ckpt.rank_0.bin");
}

// A `save.path` override under the `save` sub-block resolves too — the
// case the split-paths feature broke and this helper repairs.
TEST(Serialization_Helper_Test, ResolvesSaveSubBlockPath)
{
    auto block = parse(R"({
        "save": { "check": true, "path": "out/save.rank_{{rank}}.bin" }
    })");
    apply_serialization_path_token_resolution(block);
    EXPECT_EQ(block.get<std::string>("save.path"), "out/save.rank_0.bin");
    // The neighbor `check` key survives untouched.
    EXPECT_TRUE(block.get<bool>("save.check"));
}

// A `restore.path` override under the `restore` sub-block resolves too.
TEST(Serialization_Helper_Test, ResolvesRestoreSubBlockPath)
{
    auto block = parse(R"({
        "restore": { "step": "latest", "path": "in/restore.rank_{{rank}}.bin" }
    })");
    apply_serialization_path_token_resolution(block);
    EXPECT_EQ(block.get<std::string>("restore.path"), "in/restore.rank_0.bin");
    EXPECT_EQ(block.get<std::string>("restore.step"), "latest");
}

// All three path fields resolve independently in one pass, and each
// keeps its own template. This is the write-forward integration case.
TEST(Serialization_Helper_Test, ResolvesAllThreePathsInOneCall)
{
    auto block = parse(R"({
        "path": "shared/rank_{{rank}}.bin",
        "save":    { "path": "save/rank_{{rank}}.bin" },
        "restore": { "path": "restore/rank_{{rank}}.bin" }
    })");
    apply_serialization_path_token_resolution(block);
    EXPECT_EQ(block.get<std::string>("path"),         "shared/rank_0.bin");
    EXPECT_EQ(block.get<std::string>("save.path"),    "save/rank_0.bin");
    EXPECT_EQ(block.get<std::string>("restore.path"), "restore/rank_0.bin");
}

// Absent sub-blocks stay absent — ptree's dotted-path `put()` would
// otherwise fabricate a `save` object with just a `path` child, which
// the downstream protocol code would misread as an enabled direction.
TEST(Serialization_Helper_Test, DoesNotCreateAbsentSubBlocks)
{
    auto block = parse(R"({ "path": "shared/rank_{{rank}}.bin" })");
    apply_serialization_path_token_resolution(block);
    EXPECT_EQ(block.get<std::string>("path"), "shared/rank_0.bin");
    // Neither `save` nor `restore` may have been synthesized.
    EXPECT_FALSE(block.get_child_optional("save"));
    EXPECT_FALSE(block.get_child_optional("restore"));
}

// Unknown tokens are passed through literally — same policy the
// underlying `utilities::resolve_path_tokens` enforces.
TEST(Serialization_Helper_Test, UnknownTokensLeftLiteral)
{
    auto block = parse(R"({
        "save":    { "path": "out/{{bogus}}/save.bin" },
        "restore": { "path": "in/{{alsobogus}}/restore.bin" }
    })");
    apply_serialization_path_token_resolution(block);
    EXPECT_EQ(block.get<std::string>("save.path"),    "out/{{bogus}}/save.bin");
    EXPECT_EQ(block.get<std::string>("restore.path"), "in/{{alsobogus}}/restore.bin");
}

// A block with no path fields at all is a no-op — no exceptions, no
// spurious keys added.
TEST(Serialization_Helper_Test, NoPathsIsNoOp)
{
    auto block = parse(R"({
        "save":    { "check": true, "frequency": 1 },
        "restore": { "check": true, "step": "latest" }
    })");
    apply_serialization_path_token_resolution(block);
    EXPECT_FALSE(block.get_optional<std::string>("path"));
    EXPECT_FALSE(block.get_optional<std::string>("save.path"));
    EXPECT_FALSE(block.get_optional<std::string>("restore.path"));
    EXPECT_TRUE(block.get<bool>("save.check"));
    EXPECT_EQ(block.get<std::string>("restore.step"), "latest");
}
