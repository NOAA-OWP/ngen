#include <gtest/gtest.h>

#include <algorithm>

#include "geopackage.hpp"
#include "FileChecker.h"

// Joins run against test/data/geopackage/example_aux.gpkg, whose four attribute tables and their
// deliberate gaps are described in test/data/geopackage/example_aux.sql.
class AttributeJoin_Test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        this->path = utils::FileChecker::find_first_readable({
            "test/data/geopackage/example_aux.gpkg",
            "../test/data/geopackage/example_aux.gpkg",
            "../../test/data/geopackage/example_aux.gpkg"
        });

        if (this->path.empty()) {
            FAIL() << "can't find test/data/geopackage/example_aux.gpkg";
        }
    }

    //! The fixture's feature layer, optionally subset to the given feature IDs.
    std::shared_ptr<geojson::FeatureCollection> features(const std::vector<std::string>& ids = {})
    {
        return ngen::geopackage::read(this->path, "test", ids);
    }

    //! Join without the warnings the fixture's coverage gap provokes, for the cases not about them.
    void join_quietly(geojson::FeatureCollection& collection, const std::string& table,
                      const std::string& key_column, const std::string& prefix)
    {
        testing::internal::CaptureStderr();
        ngen::geopackage::join_attributes(collection, this->path, table, key_column, prefix, false);
        testing::internal::GetCapturedStderr();
    }

    std::string path;
};

TEST_F(AttributeJoin_Test, join_prefixes_with_table_name)
{
    auto collection = this->features();
    join_quietly(*collection, "aux_params_one", "divide_id", "aux_params_one");

    const auto& first = collection->get_feature("First");
    ASSERT_TRUE(first != nullptr);
    EXPECT_TRUE(first->has_property("aux_params_one.int_value"));
    EXPECT_EQ(first->get_property("aux_params_one.int_value").as_natural_number(), 42);
}

TEST_F(AttributeJoin_Test, join_prefixes_with_alias)
{
    auto collection = this->features();
    join_quietly(*collection, "aux_params_one", "divide_id", "one");

    const auto& first = collection->get_feature("First");
    EXPECT_TRUE(first->has_property("one.int_value"));
    EXPECT_FALSE(first->has_property("aux_params_one.int_value"));
    EXPECT_FALSE(first->has_property("int_value"));
}

TEST_F(AttributeJoin_Test, join_maps_column_types_to_property_types)
{
    auto collection = this->features();
    join_quietly(*collection, "aux_params_one", "divide_id", "one");

    const auto& first = collection->get_feature("First");
    EXPECT_EQ(first->get_property("one.int_value").get_type(), geojson::PropertyType::Natural);
    EXPECT_EQ(first->get_property("one.int_value").as_natural_number(), 42);
    EXPECT_EQ(first->get_property("one.real_value").get_type(), geojson::PropertyType::Real);
    EXPECT_DOUBLE_EQ(first->get_property("one.real_value").as_real_number(), 3.5);
    EXPECT_EQ(first->get_property("one.text_value").get_type(), geojson::PropertyType::String);
    EXPECT_EQ(first->get_property("one.text_value").as_string(), "alpha");
}

// A NULL cell means the table carries no value for that divide, which is not the same as a value
// that happens to be null; it must not reach a model as a parameter at all.
TEST_F(AttributeJoin_Test, join_omits_null_cells)
{
    auto collection = this->features();
    join_quietly(*collection, "aux_params_one", "divide_id", "one");

    EXPECT_FALSE(collection->get_feature("First")->has_property("one.sparse_value"));
}

TEST_F(AttributeJoin_Test, join_warns_for_feature_without_row)
{
    auto collection = this->features();

    testing::internal::CaptureStderr();
    ngen::geopackage::join_attributes(*collection, this->path, "aux_params_one", "divide_id", "one", false);
    const std::string captured = testing::internal::GetCapturedStderr();

    EXPECT_NE(captured.find("Second"), std::string::npos);
    EXPECT_NE(captured.find("aux_params_one"), std::string::npos);
    EXPECT_EQ(captured.find("First"), std::string::npos);

    // the warned-about feature is left alone, not given empty properties
    const auto& second = collection->get_feature("Second");
    EXPECT_FALSE(second->has_property("one.int_value"));
    EXPECT_FALSE(second->has_property("one.text_value"));
}

TEST_F(AttributeJoin_Test, join_errors_for_feature_without_row_when_required)
{
    auto collection = this->features();
    EXPECT_THROW(
        ngen::geopackage::join_attributes(*collection, this->path, "aux_params_one", "divide_id", "one", true),
        std::runtime_error
    );
}

TEST_F(AttributeJoin_Test, join_succeeds_when_required_and_fully_covered)
{
    auto collection = this->features();
    ASSERT_NO_THROW(
        ngen::geopackage::join_attributes(*collection, this->path, "aux_params_two", "catchment_id", "two", true)
    );

    EXPECT_EQ(collection->get_feature("First")->get_property("two.donor_id").as_string(), "gauge-01");
    EXPECT_EQ(collection->get_feature("Second")->get_property("two.donor_id").as_string(), "gauge-02");
}

TEST_F(AttributeJoin_Test, join_errors_on_absent_table)
{
    auto collection = this->features();
    EXPECT_THROW(
        ngen::geopackage::join_attributes(*collection, this->path, "aux_params_three", "divide_id", "three", false),
        std::runtime_error
    );
}

TEST_F(AttributeJoin_Test, join_errors_on_absent_key_column)
{
    auto collection = this->features();
    // aux_params_two is keyed on catchment_id, so the default key column is not present
    EXPECT_THROW(
        ngen::geopackage::join_attributes(*collection, this->path, "aux_params_two", "divide_id", "two", false),
        std::runtime_error
    );
}

// Under partitioning a rank holds a subset of the divides, so most of a table's rows belong to
// other ranks and must pass without comment.
TEST_F(AttributeJoin_Test, join_ignores_rows_without_a_feature)
{
    auto collection = this->features({ "First" });
    ASSERT_EQ(collection->get_size(), 1);

    testing::internal::CaptureStderr();
    ngen::geopackage::join_attributes(*collection, this->path, "aux_params_one", "divide_id", "one", false);
    const std::string captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(collection->get_size(), 1);
    EXPECT_EQ(captured, "");
    EXPECT_EQ(collection->get_feature("First")->get_property("one.text_value").as_string(), "alpha");
}

TEST_F(AttributeJoin_Test, join_of_two_tables_keeps_shared_column_names_distinct)
{
    auto collection = this->features();

    join_quietly(*collection, "aux_params_one", "divide_id", "one");
    ngen::geopackage::join_attributes(*collection, this->path, "aux_params_two", "catchment_id", "two", false);

    const auto& first = collection->get_feature("First");
    EXPECT_DOUBLE_EQ(first->get_property("one.real_value").as_real_number(), 3.5);
    EXPECT_DOUBLE_EQ(first->get_property("two.real_value").as_real_number(), 1.5);
    EXPECT_EQ(first->get_property("one.text_value").as_string(), "alpha");
    EXPECT_EQ(first->get_property("two.text_value").as_string(), "beta");

    // the feature table one has no row for still gets everything table two offers
    const auto& second = collection->get_feature("Second");
    EXPECT_FALSE(second->has_property("one.text_value"));
    EXPECT_EQ(second->get_property("two.text_value").as_string(), "delta");
}

// A composed name already on the feature belongs to the fabric layer or to an earlier entry, and
// keeping it would hand the model that other value under this table's name.
TEST_F(AttributeJoin_Test, join_errors_on_property_name_already_present)
{
    auto collection = this->features({ "First" });
    join_quietly(*collection, "aux_params_one", "divide_id", "one");

    EXPECT_THROW(
        ngen::geopackage::join_attributes(*collection, this->path, "aux_params_one", "divide_id", "one", false),
        std::runtime_error
    );
    EXPECT_EQ(collection->get_feature("First")->get_property("one.text_value").as_string(), "alpha");
}

// Prefixes are unique across config entries, but two entries can still compose the same property
// name, e.g. alias "one" over a column "shared.x" and alias "one.shared" over a column "x".
TEST_F(AttributeJoin_Test, join_errors_on_property_name_from_another_table)
{
    auto collection = this->features({ "First" });
    join_quietly(*collection, "aux_params_one", "divide_id", "shared");

    EXPECT_THROW(
        ngen::geopackage::join_attributes(*collection, this->path, "aux_params_two", "catchment_id", "shared", false),
        std::runtime_error
    );
}

// Which of two rows keyed the same a scan reaches first can change when a GeoPackage is rebuilt,
// so taking one silently would make model output depend on the file's layout.
TEST_F(AttributeJoin_Test, join_errors_on_duplicate_keyed_rows)
{
    auto collection = this->features();
    EXPECT_THROW(
        ngen::geopackage::join_attributes(*collection, this->path, "aux_params_dupe", "divide_id", "dupe", false),
        std::runtime_error
    );
}

// Only rows for divides this run holds are joined, so a duplicate elsewhere in a whole-hydrofabric
// table is another rank's problem, not this one's.
TEST_F(AttributeJoin_Test, join_ignores_duplicate_rows_for_absent_features)
{
    auto collection = this->features({ "Second" });
    ASSERT_NO_THROW(
        ngen::geopackage::join_attributes(*collection, this->path, "aux_params_dupe", "divide_id", "dupe", true)
    );

    EXPECT_DOUBLE_EQ(collection->get_feature("Second")->get_property("dupe.dupe_value").as_real_number(), 3.5);
}

// A cell of a type no property can hold is skipped like a NULL, rather than becoming a property
// holding some stand-in value.
TEST_F(AttributeJoin_Test, join_omits_cells_of_unconvertible_type)
{
    auto collection = this->features();
    ngen::geopackage::join_attributes(*collection, this->path, "aux_params_blob", "divide_id", "blob", true);

    const auto& first = collection->get_feature("First");
    EXPECT_FALSE(first->has_property("blob.blob_value"));
    EXPECT_EQ(first->get_property("blob.int_value").as_natural_number(), 11);
}

// Nothing keeps a collection to one feature per id, and a feature sharing an id is as much the
// subject of that id's row as the one indexed under it.
TEST_F(AttributeJoin_Test, join_reaches_every_feature_sharing_an_id)
{
    auto collection = this->features({ "First" });
    collection->add_feature(this->features({ "First" })->get_feature("First"));
    ASSERT_EQ(collection->get_size(), 2);

    ngen::geopackage::join_attributes(*collection, this->path, "aux_params_one", "divide_id", "one", true);

    EXPECT_EQ(collection->get_feature(0)->get_property("one.text_value").as_string(), "alpha");
    EXPECT_EQ(collection->get_feature(1)->get_property("one.text_value").as_string(), "alpha");
}

// The gap is one id with no row, so it is reported once however many features carry that id, and
// is still a gap when the entry is required.
TEST_F(AttributeJoin_Test, join_reports_a_missing_row_once_for_a_shared_id)
{
    auto collection = this->features({ "Second" });
    collection->add_feature(this->features({ "Second" })->get_feature("Second"));
    ASSERT_EQ(collection->get_size(), 2);

    testing::internal::CaptureStderr();
    ngen::geopackage::join_attributes(*collection, this->path, "aux_params_one", "divide_id", "one", false);
    const std::string captured = testing::internal::GetCapturedStderr();

    EXPECT_NE(captured.find("Second"), std::string::npos);
    EXPECT_EQ(std::count(captured.begin(), captured.end(), '\n'), 1);

    EXPECT_THROW(
        ngen::geopackage::join_attributes(*collection, this->path, "aux_params_one", "divide_id", "one", true),
        std::runtime_error
    );
}
