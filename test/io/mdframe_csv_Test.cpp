#include "gtest/gtest.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

#include "mdframe.hpp"

class mdframe_csv_Test : public ::testing::Test
{
  protected:
    mdframe_csv_Test()
        : tempfile(testing::TempDir())
    {}

    ~mdframe_csv_Test() override
    {}

    void SetUp() override
    {
        this->tempfile.append("mdframeTest_ioCSV.csv");
    }

    void TearDown() override
    {
        // fs::remove(this->tempfile);
    }

    fs::path tempfile;
};

TEST_F(mdframe_csv_Test, io_csv)
{
    io::mdframe df;

    df.add_dimension("x", 2)
      .add_dimension("y", 2);

    df.add_variable<int>("x", { "x" })
      .add_variable<int>("y", { "y" })
      .add_variable<double>("v", {"x", "y"});

    for (size_t x = 0; x < 2; x++) {
        df["x"].insert({ x }, x);
        df["y"].insert({ x }, x);
        for (size_t y = 0; y < 2; y++) {
            df["v"].insert({ x, y }, x * y);
        }
    }

    df.to_csv(this->tempfile.string(), "x");
}