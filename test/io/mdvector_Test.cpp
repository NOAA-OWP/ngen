#include "gtest/gtest.h"
#include "mdvector.hpp"

TEST(mdvector_Test, construction)
{
    // Construct an empty mdvector.
    // Should have rank of 0.
    io::mdvector<double> s{};
    EXPECT_EQ(s.rank(), 0);

    // Setting rank to 2
    s.rank(2);
    EXPECT_EQ(s.rank(), 2);

    // Pushing data
    s.push_back(3, 2);
    EXPECT_EQ(s.size(), 1);
    EXPECT_EQ(s.true_size(), 2);
    EXPECT_EQ(s.at(0, 0), 3);
    EXPECT_EQ(s.at(0, 1), 2);

    // Setting rank to 3
    s.rank(3);
    EXPECT_EQ(s.rank(), 3);
    EXPECT_EQ(s.size(), 1);

    // rank should append to make series divisible
    // by new rank. This prevents out of range exceptions.
    EXPECT_EQ(s.true_size(), 3);
    EXPECT_EQ(s.at(0, 0), 3);
    EXPECT_EQ(s.at(0, 1), 2);
    EXPECT_EQ(s.at(0, 2), 0);

    // Check setting a value
    s.at(0, 2) = 1;
    EXPECT_EQ(s.at(0, 2), 1);
}

TEST(mdvector_Test, vertical_iteration)
{
    // Construct a basic 4D mdvector
    io::mdvector<int> s{};
    s.rank(4);

    s.push_back(1, 2, 3, 4);
    s.push_back(5, 6, 7, 8);
    s.push_back(9, 10, 11, 12);

    // Vertical Axis Iteration
    EXPECT_EQ(s.vaxis(0).at(0), 1);
    EXPECT_EQ(s.vaxis(0).at(1), 5);
    EXPECT_EQ(s.vaxis(0).at(2), 9);

    EXPECT_EQ(s.vaxis(1).at(0), 2);
    EXPECT_EQ(s.vaxis(1).at(1), 6);
    EXPECT_EQ(s.vaxis(1).at(2), 10);

    EXPECT_EQ(s.vaxis(2).at(0), 3);
    EXPECT_EQ(s.vaxis(2).at(1), 7);
    EXPECT_EQ(s.vaxis(2).at(2), 11);

    EXPECT_EQ(s.vaxis(3).at(0), 4);
    EXPECT_EQ(s.vaxis(3).at(1), 8);
    EXPECT_EQ(s.vaxis(3).at(2), 12);

    EXPECT_THROW(s.vaxis(4), std::out_of_range);
    EXPECT_THROW(s.vaxis(3).at(3), std::out_of_range);

    // Vertical Axis range-based for loop iteration
    std::vector<std::vector<int>> expected = {
        { 12, 8, 4 },
        { 11, 7, 3 },
        { 10, 6, 2 },
        { 9,  5, 1 }
    };

    for (std::size_t i = 0; i < s.rank(); i++) {
        auto& e = expected.back();
        for (const int& value : s.vaxis(i))
        {
            if (e.empty()) {
                FAIL() << "vertical axis iterator overflowed (index: " << i << ")";
            }

            EXPECT_EQ(value, e.back());

            e.pop_back();
        }
        expected.pop_back();
    }
}

TEST(mdvector_Test, horizontal_iteration)
{
    // Construct a basic 4D mdvector
    io::mdvector<int> s{};
    s.rank(4);

    s.push_back(1, 2, 3, 4);
    s.push_back(5, 6, 7, 8);
    s.push_back(9, 10, 11, 12);

    // Horizontal Axis Iteration
    EXPECT_EQ(s.haxis(0).at(0), 1);
    EXPECT_EQ(s.haxis(0).at(1), 2);
    EXPECT_EQ(s.haxis(0).at(2), 3);
    EXPECT_EQ(s.haxis(0).at(3), 4);

    EXPECT_EQ(s.haxis(1).at(0), 5);
    EXPECT_EQ(s.haxis(1).at(1), 6);
    EXPECT_EQ(s.haxis(1).at(2), 7);
    EXPECT_EQ(s.haxis(1).at(3), 8);

    EXPECT_EQ(s.haxis(2).at(0), 9);
    EXPECT_EQ(s.haxis(2).at(1), 10);
    EXPECT_EQ(s.haxis(2).at(2), 11);
    EXPECT_EQ(s.haxis(2).at(3), 12);

    EXPECT_THROW(s.haxis(3), std::out_of_range);
    EXPECT_THROW(s.haxis(2).at(4), std::out_of_range);

    // Horizontal Axis range-based for loop iteration
    std::vector<std::vector<int>> expected = {
        { 12, 11, 10,  9 },
        {  8,  7,  6,  5 },
        {  4,  3,  2,  1 }
    };

    for (std::size_t i = 0; i < s.size(); i++) {
        auto& e = expected.back();
        for (const int& value : s.haxis(i))
        {
            if (e.empty()) {
                FAIL() << "horizontal axis iterator overflowed (index: " << i << ")";
            }

            EXPECT_EQ(value, e.back());

            e.pop_back();
        }
        expected.pop_back();
    }
}