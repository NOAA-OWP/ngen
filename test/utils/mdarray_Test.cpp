#include "gtest/gtest.h"
#include <initializer_list>
#include "mdarray.hpp"

TEST(mdarray_Test, construction)
{
    // Horrible workaround for GCC8 not quite getting initializer list stuff right
    size_t indices[] = {2, 2};

    ngen::mdarray<double> s{indices};

    EXPECT_EQ(s.rank(), 2);

    indices[0] = 0;
    indices[1] = 0;
    ASSERT_NO_THROW(s.insert(indices, 1));
    EXPECT_EQ(s.at(indices), 1);

    indices[1] = 1;
    ASSERT_NO_THROW(s.insert(indices, 2));
    EXPECT_EQ(s.at(indices), 2);

    indices[0] = 2;
    indices[1] = 2;
    EXPECT_THROW(s.at(indices), std::out_of_range);

    indices[0] = 0;
    indices[1] = 0;
    ASSERT_NO_THROW(s.at(indices) = 3);
    EXPECT_EQ(s.at(indices), 3);
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

        std::vector<std::size_t> y (s.rank());
        s.deindex(x, y);
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

    size_t indices0[] = {0, 0, 0, 0, 0};
    size_t indices1[] = {1, 0, 0, 0, 0};
    ASSERT_EQ(
        a.index(indices0) + 1,
        a.index(indices1)
    );

    size_t indices0a[] = {0, 1, 1, 0, 1};
    size_t indices1a[] = {1, 1, 1, 0, 1};
    ASSERT_EQ(
        a.index(indices0a) + 1,
        a.index(indices1a)
    );

    ngen::mdarray<int> b{{2, 2, 2}};

    size_t indices0b[] = {0, 0, 0};
    size_t indices1b[] = {1, 0, 0};
    ASSERT_EQ(
        b.index(indices0b) + 1,
        b.index(indices1b)
    );

    size_t indices0c[] = {0, 1, 1};
    size_t indices1c[] = {1, 1, 1};
    ASSERT_EQ(
        b.index(indices0c) + 1,
        b.index(indices1c)
    );

    // add `i` to prevent braces around scalar warnings
    std::initializer_list<std::size_t> i = { 2 };
    ngen::mdarray<int> c{i};

    size_t index0[] = {0};
    size_t index1[] = {1};
    ASSERT_EQ(
        c.index(index0) + 1,
        c.index(index1)
    );
}
