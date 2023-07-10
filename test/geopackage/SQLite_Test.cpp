#include <gtest/gtest.h>

#include "SQLite.hpp"
#include "FileChecker.h"

using namespace geopackage;

class SQLite_Test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        this->path = utils::FileChecker::find_first_readable({
            "test/data/routing/gauge_01073000.gpkg",
            "../test/data/routing/gauge_01073000.gpkg",
            "../../test/data/routing/gauge_01073000.gpkg"
        });

        if (this->path.empty()) {
            FAIL() << "can't find gauge_01073000.gpkg";
        }
    }

    void TearDown() override {};
    
    std::string path;
    
};

TEST_F(SQLite_Test, sqlite_access_test)
{
    sqlite db {this->path};
    // user wants metadata
    EXPECT_TRUE(db.has_table("gpkg_contents"));
    EXPECT_FALSE(db.has_table("some_fake_table"));
}

TEST_F(SQLite_Test, sqlite_query_test)
{
    sqlite db {this->path};

    if (db.connection() == nullptr) {
        FAIL() << "database is not loaded";
    }

    // user provides a query
    const std::string query = "SELECT * FROM gpkg_contents LIMIT 1";
    sqlite_iter iter = db.query(query);

    EXPECT_EQ(iter.num_columns(), 10);
    EXPECT_EQ(iter.columns(), std::vector<std::string>({
        "table_name",
        "data_type",
        "identifier",
        "description",
        "last_change",
        "min_x",
        "min_y",
        "max_x",
        "max_y",
        "srs_id"
    }));

    // user iterates over row
    ASSERT_NO_THROW(iter.next());

    // using column indices
    EXPECT_EQ(iter.get<std::string>(0), "flowpaths");
    EXPECT_EQ(iter.get<std::string>(1), "features");
    EXPECT_EQ(iter.get<std::string>(2), "flowpaths");
    EXPECT_EQ(iter.get<std::string>(3), "");
    EXPECT_EQ(iter.get<std::string>(4), "2022-10-25T14:33:51.668Z");
    EXPECT_EQ(iter.get<double>(5), 1995218.564876059);
    EXPECT_EQ(iter.get<double>(6), 2502240.321178956);
    EXPECT_EQ(iter.get<double>(7), 2002525.992495368);
    EXPECT_EQ(iter.get<double>(8), 2508383.058762011);
    EXPECT_EQ(iter.get<int>(9), 5070);

    // using column_names
    EXPECT_EQ(iter.get<std::string>("table_name"), "flowpaths");
    EXPECT_EQ(iter.get<std::string>("data_type"), "features");
    EXPECT_EQ(iter.get<std::string>("identifier"), "flowpaths");
    EXPECT_EQ(iter.get<std::string>("description"), "");
    EXPECT_EQ(iter.get<std::string>("last_change"), "2022-10-25T14:33:51.668Z");
    EXPECT_EQ(iter.get<double>("min_x"), 1995218.564876059);
    EXPECT_EQ(iter.get<double>("min_y"), 2502240.321178956);
    EXPECT_EQ(iter.get<double>("max_x"), 2002525.992495368);
    EXPECT_EQ(iter.get<double>("max_y"), 2508383.058762011);
    EXPECT_EQ(iter.get<int>("srs_id"), 5070);

    // reiteration
    EXPECT_EQ(iter.current_row(), 0);
    iter.restart();
    EXPECT_FALSE(iter.done());
    EXPECT_EQ(iter.current_row(), -1);
    ASSERT_ANY_THROW(iter.get<int>(0));
    iter.next();
    EXPECT_EQ(iter.current_row(), 0);
    ASSERT_EQ(iter.get<std::string>(0), "flowpaths");

    // finishing
    iter.next();
    EXPECT_TRUE(iter.done());
    EXPECT_EQ(iter.current_row(), 1);
    ASSERT_ANY_THROW(iter.get<int>(0));

    // next should be idempotent when iteration is done
    ASSERT_NO_THROW(iter.next());
    EXPECT_TRUE(iter.done());
    EXPECT_EQ(iter.current_row(), 1);
}