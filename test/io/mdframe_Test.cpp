#include "gtest/gtest.h"
#include "mdframe.hpp"

TEST(mdframe_Test, construction)
{
    io::mdframe df;

    // Create an unlimited dimension for time
    df.add_dimension("time");

    // Create constrained dimensions for x/y
    // (using method chaining)
    df.add_dimension("x", 10)
      .add_dimension("y", 10);

    EXPECT_TRUE(df.has_dimension("time"));
    EXPECT_TRUE(df.has_dimension("x"));
    EXPECT_TRUE(df.has_dimension("y"));

    // Can't use EXPECT_EQ or EXPECT_NE here due to
    // boost::optional IO requirements. It's not worth it
    // to implement those overloads either.

    EXPECT_TRUE(df.get_dimension("time") != boost::none);
    EXPECT_TRUE(df.get_dimension("fake") == boost::none);

    df.add_variable<int>("0D");
    df.add_variable<std::string>("1D", { "time" });
    df.add_variable<double>("2D", { "x", "y" });
    df.add_variable<int>("3D", { "time", "x", "y" });
    
    EXPECT_TRUE(df.get_variable("0D") != boost::none);
    EXPECT_TRUE(df.get_variable("1D") != boost::none);
    EXPECT_TRUE(df.get_variable("2D") != boost::none);
    EXPECT_TRUE(df.get_variable("3D") != boost::none);
    EXPECT_TRUE(df.get_variable("4D") == boost::none);

    // EXPECT_NO_THROW(df.push_back("1D", { "t1", "t2", "t3", "t4", "t5" }));
    // EXPECT_THROW(df.push_back("1D", 1), std::logic_error);

    EXPECT_EQ(df["2D"]->size(), 0);
    EXPECT_EQ(df["2D"]->rank(), 2);
    EXPECT_EQ(df["2D"]->capacity(), 100);
}