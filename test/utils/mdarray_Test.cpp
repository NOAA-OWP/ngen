#include "gtest/gtest.h"
#include <initializer_list>
#include "mdarray.hpp"

TEST(mdarray_Test, construction)
{
    ngen::mdarray<double> s{{2, 2}};

    EXPECT_EQ(s.rank(), 2);

    ASSERT_NO_THROW(s.insert({{0, 0}}, 1));
    EXPECT_EQ(s.at({{0, 0}}), 1);

    ASSERT_NO_THROW(s.insert({{0, 1}}, 2));
    EXPECT_EQ(s.at({{0, 1}}), 2);

    EXPECT_THROW(s.at({{2, 2}}), std::out_of_range);

    ASSERT_NO_THROW(s.at({{0, 0}}) = 3);
    EXPECT_EQ(s.at({{0, 0}}), 3);
}

TEST(mdarray_Test, indexing)
{
    ngen::mdarray<double> s{
        {10, 10}
    };

    const auto expect = [s](
        std::vector<std::size_t> idx,
        std::size_t addr
    ) {
        std::size_t x = s.index(idx);
        ASSERT_EQ(x, addr);

        std::vector<std::size_t> y = s.deindex(x);
        ASSERT_EQ(y.size(), idx.size());
        for (std::size_t i = 0; i < y.size(); i++) {
            EXPECT_EQ(y[i], idx[i]);
        }
    };

    expect({0, 0}, 0);
    expect({0, 1}, 10);
    expect({0, 2}, 20);
    expect({1, 1}, 11);
    expect({3, 5}, 53);
}

TEST(mdarray_Test, layout)
{
    ngen::mdarray<int> a{{2, 2, 2, 2, 2}};

    ASSERT_EQ(
        a.index({{0, 0, 0, 0, 0}}) + 1,
        a.index({{1, 0, 0, 0, 0}})
    );

    ASSERT_EQ(
        a.index({{0, 1, 1, 0, 1}}) + 1,
        a.index({{1, 1, 1, 0, 1}})
    );

    ngen::mdarray<int> b{{2, 2, 2}};

    ASSERT_EQ(
        b.index({{0, 0, 0}}) + 1,
        b.index({{1, 0, 0}})
    );

    ASSERT_EQ(
        b.index({{0, 1, 1}}) + 1,
        b.index({{1, 1, 1}})
    );

    // add `i` to prevent braces around scalar warnings
    std::initializer_list<std::size_t> i = { 2 };
    ngen::mdarray<int> c{i};

    ASSERT_EQ(
        c.index({{0}}) + 1,
        c.index({{1}})
    );

    ASSERT_EQ(
        c.index({{0}}) + 1,
        c.index({{1}})
    );
}
