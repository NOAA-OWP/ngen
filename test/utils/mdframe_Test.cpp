#include "gtest/gtest.h"

#include "mdframe.hpp"

TEST(mdframe_Test, construction)
{
    ngen::mdframe df;

    // Create constrained dimensions for time/x/y
    // (using method chaining)
    df.add_dimension("time", 100)
      .add_dimension("x", 10)
      .add_dimension("y", 10);

    EXPECT_TRUE(df.has_dimension("time"));
    EXPECT_TRUE(df.has_dimension("x"));
    EXPECT_TRUE(df.has_dimension("y"));

    // Can't use EXPECT_EQ or EXPECT_NE here due to
    // boost::optional IO requirements. It's not worth it
    // to implement those overloads either.

    EXPECT_TRUE(df.get_dimension("time") != boost::none);
    EXPECT_TRUE(df.get_dimension("fake") == boost::none);

    df.add_variable<int>("1D", { "time" });
    df.add_variable<double>("2D", { "x", "y" });
    df.add_variable<int>("3D", { "time", "x", "y" });

    EXPECT_EQ(df["1D"].shape()[0], 100);
    
    for (std::size_t x : df["2D"].shape()) {
      EXPECT_EQ(x, 10);
    }

    EXPECT_EQ(df["2D"].size(), 100);
    EXPECT_EQ(df["2D"].rank(), 2);


    size_t index[] = {0, 0};
    EXPECT_NO_THROW(df["2D"].insert(index, 1.0));
    index[1] = 1;
    EXPECT_NO_THROW(df["2D"].insert(index, 2));
    index[0] = 10;
    index[1] = 0;
    EXPECT_THROW(df["2D"].insert(index, 3), std::out_of_range);
    index[0] = 0;
    index[1] = 0;
    EXPECT_EQ(df["2D"].at<double>(index), 1);
    index[1] = 1;
    EXPECT_EQ(df["2D"].at<double>(index), 2);
}
