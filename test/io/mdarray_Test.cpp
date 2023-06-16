#include "gtest/gtest.h"
#include "mdarray.hpp"

TEST(mdarray_Test, construction)
{
    io::mdarray<double> s{};

    EXPECT_EQ(s.rank(), 0);

    s.rank(2);
    EXPECT_EQ(s.rank(), 2);

    s.emplace({0, 0}, 1);
    EXPECT_EQ(s.size(), 1);
    EXPECT_EQ(s.at({0, 0}), 1);

    s.emplace({0, 1}, 2);
    EXPECT_EQ(s.size(), 2);
    EXPECT_EQ(s.at({0, 1}), 2);

    EXPECT_THROW(s.at({1, 1}), std::out_of_range);

    EXPECT_NO_THROW(s.at({0, 0}) = 3);
    EXPECT_EQ(s.at({0, 0}), 3);

    EXPECT_NO_THROW(s.clear());
    EXPECT_EQ(s.size(), 0);
}

TEST(mdarray_Test, iteration)
{
    io::mdarray<double> s{2};

    // This is in Z-order form
    s.emplace({
        { {0, 0}, 1 },
        { {1, 0}, 2 },
        { {0, 1}, 3 },
        { {1, 1}, 4 }
    });

    EXPECT_EQ(s.size(), 4);
    
    size_t i = 1;
    for (auto ss : s) {
        EXPECT_EQ(ss, i++);
    }
}