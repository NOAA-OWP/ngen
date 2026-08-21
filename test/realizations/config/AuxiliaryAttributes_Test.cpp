#include <gtest/gtest.h>

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "realizations/config/auxiliary_attributes.hpp"

using realization::config::parse_auxiliary_attributes;

namespace {
    boost::property_tree::ptree parse(const std::string& json) {
        boost::property_tree::ptree tree;
        std::stringstream ss(json);
        boost::property_tree::json_parser::read_json(ss, tree);
        return tree;
    }
}

// A config predating the feature parses to no entries at all.
TEST(AuxiliaryAttributes_Config_Test, AbsentKeyYieldsNoEntries)
{
    EXPECT_TRUE(parse_auxiliary_attributes(parse(R"({"time": {}})")).empty());
}

// An explicitly empty list is equally benign.
TEST(AuxiliaryAttributes_Config_Test, EmptyListYieldsNoEntries)
{
    EXPECT_TRUE(parse_auxiliary_attributes(parse(R"({"auxiliary_hydrofabric_attributes": []})")).empty());
}

// Every field spelled out is carried through, and the alias wins as the prefix.
TEST(AuxiliaryAttributes_Config_Test, AllFieldsParsed)
{
    const std::string json = R"({
        "auxiliary_hydrofabric_attributes": [
            {
                "table": "cfe_x_nom_regionalization",
                "alias": "cfexnom",
                "file": "/data/regionalization.gpkg",
                "key_column": "catchment_id",
                "required": true
            }
        ]
    })";
    const auto entries = parse_auxiliary_attributes(parse(json));
    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].table, "cfe_x_nom_regionalization");
    EXPECT_EQ(entries[0].alias, "cfexnom");
    EXPECT_EQ(entries[0].file, "/data/regionalization.gpkg");
    EXPECT_EQ(entries[0].key_column, "catchment_id");
    EXPECT_TRUE(entries[0].required);
    EXPECT_EQ(entries[0].prefix(), "cfexnom");
}

// Only 'table' is required; everything else defaults, and the table name becomes the prefix.
TEST(AuxiliaryAttributes_Config_Test, DefaultsWhenOnlyTableDeclared)
{
    const auto entries = parse_auxiliary_attributes(
        parse(R"({"auxiliary_hydrofabric_attributes": [{"table": "aux_params_one"}]})"));
    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].table, "aux_params_one");
    EXPECT_TRUE(entries[0].alias.empty());
    EXPECT_TRUE(entries[0].file.empty());   // empty means "the catchment data file from the CLI"
    EXPECT_EQ(entries[0].key_column, "divide_id");
    EXPECT_FALSE(entries[0].required);
    EXPECT_EQ(entries[0].prefix(), "aux_params_one");
}

// Several tables may be declared at once, and they keep their configured order.
TEST(AuxiliaryAttributes_Config_Test, MultipleEntriesKeepOrder)
{
    const std::string json = R"({
        "auxiliary_hydrofabric_attributes": [
            {"table": "aux_params_one"},
            {"table": "aux_params_two", "alias": "two", "key_column": "catchment_id"}
        ]
    })";
    const auto entries = parse_auxiliary_attributes(parse(json));
    ASSERT_EQ(entries.size(), 2);
    EXPECT_EQ(entries[0].prefix(), "aux_params_one");
    EXPECT_EQ(entries[0].key_column, "divide_id");
    EXPECT_EQ(entries[1].prefix(), "two");
    EXPECT_EQ(entries[1].key_column, "catchment_id");
}

// Two entries sharing an alias would publish colliding property names.
TEST(AuxiliaryAttributes_Config_Test, DuplicateAliasThrows)
{
    const std::string json = R"({
        "auxiliary_hydrofabric_attributes": [
            {"table": "aux_params_one", "alias": "aux"},
            {"table": "aux_params_two", "alias": "aux"}
        ]
    })";
    EXPECT_THROW(parse_auxiliary_attributes(parse(json)), std::runtime_error);
}

// The collision is on the effective prefix, so an alias may not shadow another entry's table name.
TEST(AuxiliaryAttributes_Config_Test, AliasCollidingWithTableNameThrows)
{
    const std::string json = R"({
        "auxiliary_hydrofabric_attributes": [
            {"table": "aux_params_one"},
            {"table": "aux_params_two", "alias": "aux_params_one"}
        ]
    })";
    EXPECT_THROW(parse_auxiliary_attributes(parse(json)), std::runtime_error);
}

// Two entries for the same table are only usable when their prefixes differ.
TEST(AuxiliaryAttributes_Config_Test, SameTableFromTwoFilesNeedsDistinctPrefixes)
{
    const std::string shared_prefix = R"({
        "auxiliary_hydrofabric_attributes": [
            {"table": "aux_params_one", "file": "a.gpkg"},
            {"table": "aux_params_one", "file": "b.gpkg"}
        ]
    })";
    EXPECT_THROW(parse_auxiliary_attributes(parse(shared_prefix)), std::runtime_error);

    const std::string aliased = R"({
        "auxiliary_hydrofabric_attributes": [
            {"table": "aux_params_one", "file": "a.gpkg", "alias": "from_a"},
            {"table": "aux_params_one", "file": "b.gpkg", "alias": "from_b"}
        ]
    })";
    EXPECT_EQ(parse_auxiliary_attributes(parse(aliased)).size(), 2);
}

// 'table' is the one field with no sensible default.
TEST(AuxiliaryAttributes_Config_Test, MissingTableThrows)
{
    EXPECT_THROW(parse_auxiliary_attributes(
        parse(R"({"auxiliary_hydrofabric_attributes": [{"alias": "aux"}]})")), std::runtime_error);
}

// A bare string where an entry object belongs is rejected rather than read as a table name.
TEST(AuxiliaryAttributes_Config_Test, ScalarEntryThrows)
{
    EXPECT_THROW(parse_auxiliary_attributes(
        parse(R"({"auxiliary_hydrofabric_attributes": ["aux_params_one"]})")), std::runtime_error);
}

// The key names a list; an object or scalar in its place is a config error.
TEST(AuxiliaryAttributes_Config_Test, NonListValueThrows)
{
    EXPECT_THROW(parse_auxiliary_attributes(
        parse(R"({"auxiliary_hydrofabric_attributes": "aux_params_one"})")), std::runtime_error);
    EXPECT_THROW(parse_auxiliary_attributes(
        parse(R"({"auxiliary_hydrofabric_attributes": {"table": "aux_params_one"}})")), std::runtime_error);
}

// String fields must be non-empty scalars.
TEST(AuxiliaryAttributes_Config_Test, MalformedStringFieldsThrow)
{
    EXPECT_THROW(parse_auxiliary_attributes(
        parse(R"({"auxiliary_hydrofabric_attributes": [{"table": ""}]})")), std::runtime_error);
    EXPECT_THROW(parse_auxiliary_attributes(
        parse(R"({"auxiliary_hydrofabric_attributes": [{"table": {"name": "aux"}}]})")), std::runtime_error);
    EXPECT_THROW(parse_auxiliary_attributes(
        parse(R"({"auxiliary_hydrofabric_attributes": [{"table": "aux", "alias": ""}]})")), std::runtime_error);
    EXPECT_THROW(parse_auxiliary_attributes(
        parse(R"({"auxiliary_hydrofabric_attributes": [{"table": "aux", "key_column": ""}]})")),
        std::runtime_error);
    EXPECT_THROW(parse_auxiliary_attributes(
        parse(R"({"auxiliary_hydrofabric_attributes": [{"table": "aux", "file": ["a.gpkg"]}]})")),
        std::runtime_error);
}

// 'required' is a boolean, not free text.
TEST(AuxiliaryAttributes_Config_Test, MalformedRequiredFlagThrows)
{
    EXPECT_THROW(parse_auxiliary_attributes(
        parse(R"({"auxiliary_hydrofabric_attributes": [{"table": "aux", "required": "yes"}]})")),
        std::runtime_error);
}
