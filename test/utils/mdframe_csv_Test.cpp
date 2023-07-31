#include "gtest/gtest.h"

#include <fstream>

#include "mdframe.hpp"


namespace ngen {

// Defined in handler_csv.cpp
extern void cartesian_indices(
    boost::span<const std::size_t>         shape,
    std::vector<std::vector<std::size_t>>& output
);

}

class mdframe_csv_Test : public ::testing::Test
{
  protected:
    mdframe_csv_Test()
        : path(testing::TempDir())
    {
        char last_char = *(path.end() - 1);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
        if (last_char == '\\')
            path.append("\\");
#else
        if (last_char != '/')
            path.append("/");
#endif

        path.append("ngen__mdframe_Test_CSV.csv");
    }

    ~mdframe_csv_Test() override
    {
        unlink(this->path.c_str());
    }

    std::string path;
};

/**
 * This unit test checks that `cartesian_indices`
 * correctly generates lists of indices based on the
 * cartesian product of the dimensions given (i.e. the `shape`).
 */
TEST_F(mdframe_csv_Test, io_csv_cartesian)
{
    std::vector<size_t> shape { 1, 2, 3 };
    std::vector<std::vector<size_t>> output;
    output.reserve(6);
    ngen::cartesian_indices(shape, output);

    EXPECT_EQ(output[0][0], 0);
    EXPECT_EQ(output[0][1], 0);
    EXPECT_EQ(output[0][2], 0);

    EXPECT_EQ(output[1][0], 0);
    EXPECT_EQ(output[1][1], 0);
    EXPECT_EQ(output[1][2], 1);

    EXPECT_EQ(output[2][0], 0);
    EXPECT_EQ(output[2][1], 0);
    EXPECT_EQ(output[2][2], 2);

    EXPECT_EQ(output[3][0], 0);
    EXPECT_EQ(output[3][1], 1);
    EXPECT_EQ(output[3][2], 0);

    EXPECT_EQ(output[4][0], 0);
    EXPECT_EQ(output[4][1], 1);
    EXPECT_EQ(output[4][2], 1);

    EXPECT_EQ(output[5][0], 0);
    EXPECT_EQ(output[5][1], 1);
    EXPECT_EQ(output[5][2], 2);
}

TEST_F(mdframe_csv_Test, io_csv)
{
    ngen::mdframe df;

    df.add_dimension("x", 2)
      .add_dimension("y", 2);

    df.add_variable<int>("x", { "x" })
      .add_variable<int>("y", { "y" })
      .add_variable<double>("v", {"x", "y"});

    for (size_t i = 0; i < 2; ++i) {
        df["x"].insert({{ i }}, i + 1);
        df["y"].insert({{ i }}, i + 1);
    }

    for (size_t x = 0; x < 2; x++) {
        for (size_t y = 0; y < 2; y++) {
            df["v"].insert({{ x, y }}, (x + 1) * (y + 1));
        }
    }

    df.to_csv(this->path, "x");

    std::ifstream csv{this->path};
    if (!csv.is_open())
        FAIL() << "failed to open " << this->path;

    std::stringstream buffer;
    buffer << csv.rdbuf();
    csv.close();
    ASSERT_EQ(buffer.str(), "v,y,x\n1.000000,1,1\n2.000000,1,2\n2.000000,2,1\n4.000000,2,2\n");
}
