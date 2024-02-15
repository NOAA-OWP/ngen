#include <gtest/gtest.h>

#include <string>

#include "ngen_sqlite.hpp"
#include "FileChecker.h"

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
    ngen::sqlite::database db {this->path};
    // user wants metadata
    EXPECT_TRUE(db.contains("gpkg_contents"));
    EXPECT_FALSE(db.contains("some_fake_table"));
}

TEST_F(SQLite_Test, sqlite_query_test)
{
    ngen::sqlite::database db {this->path};

    if (db.connection() == nullptr) {
        FAIL() << "database is not loaded";
    }

    // user provides a query
    const std::string query = "SELECT * FROM gpkg_contents WHERE table_name = 'flowpaths' LIMIT 1";
    ngen::sqlite::database::iterator iter = db.query(query);

    EXPECT_EQ(iter.num_columns(), 10);

    std::vector<std::string> expected_columns = {
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
    };

    for (size_t i = 0; i < expected_columns.size(); i++) {
        EXPECT_EQ(iter.columns()[i], expected_columns[i]);
    }

    // user iterates over row
    ASSERT_NO_THROW(iter.next());

    // using column indices
    EXPECT_EQ(iter.get<std::string>(0), "flowpaths");
    EXPECT_EQ(iter.get<std::string>(1), "features");
    EXPECT_EQ(iter.get<std::string>(2), "flowpaths");
    EXPECT_EQ(iter.get<std::string>(3), "");
    EXPECT_NEAR(iter.get<double>(5), 1995219.0, 1e2);
    EXPECT_NEAR(iter.get<double>(6), 2502240.0, 1e2);
    EXPECT_NEAR(iter.get<double>(7), 2002526.0, 1e2);
    EXPECT_NEAR(iter.get<double>(8), 2508383.0, 1e2);
    EXPECT_EQ(iter.get<int>(9), 5070);

    // using column_names
    EXPECT_EQ(iter.get<std::string>("table_name"), "flowpaths");
    EXPECT_EQ(iter.get<std::string>("data_type"), "features");
    EXPECT_EQ(iter.get<std::string>("identifier"), "flowpaths");
    EXPECT_EQ(iter.get<std::string>("description"), "");
    EXPECT_NEAR(iter.get<double>(5), 1995219.0, 1e2);
    EXPECT_NEAR(iter.get<double>(6), 2502240.0, 1e2);
    EXPECT_NEAR(iter.get<double>(7), 2002526.0, 1e2);
    EXPECT_NEAR(iter.get<double>(8), 2508383.0, 1e2);
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
