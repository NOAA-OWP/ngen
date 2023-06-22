#include "gtest/gtest.h"
#include <netcdf>

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

#include "mdframe.hpp"

TEST(mdframe_Test, io_netcdf)
{
    io::mdframe df;

    df.add_dimension("x", 2)
      .add_dimension("y", 2);

    df.add_variable<int>("x", { "x" })
      .add_variable<int>("y", { "y" })
      .add_variable<double>("v", {"x", "y"});

    for (size_t x = 0; x < 2; x++) {
        df.insert("x", { x }, x + 1);
        for (size_t y = 0; y < 2; y++) {
            df.insert("y", { y }, y + 1);
            df.insert("v", { x, y }, (x + 1) * (y + 1));
        }
    }
    
    fs::path tempfile(testing::TempDir());
    tempfile.append("mdframeTest_ioNetcdf.netcdf");
    df.to_netcdf(tempfile.string());

    netCDF::NcFile ex;
    ex.open(tempfile.string(), netCDF::NcFile::read);

    const auto xdim = ex.getDim("x");
    ASSERT_FALSE(xdim.isNull());
    ASSERT_EQ(xdim.getSize(), 2);

    const auto ydim = ex.getDim("y");
    ASSERT_FALSE(ydim.isNull());
    ASSERT_EQ(ydim.getSize(), 2);

    const auto xvar = ex.getVar("x");
    const auto yvar = ex.getVar("y");
    const auto vvar = ex.getVar("v");

    EXPECT_EQ(xvar.getDimCount(), 1);
    EXPECT_EQ(yvar.getDimCount(), 1);
    EXPECT_EQ(vvar.getDimCount(), 2);

    int    xval = 0;
    int    yval = 0;
    double vval = 0;
    for (size_t x = 0; x < 2; x++) {
        xvar.getVar({ x }, &xval);
        EXPECT_EQ(xval, x + 1);
        for (size_t y = 0; y < 2; y++) {
            yvar.getVar({ y }, &yval);
            EXPECT_EQ(yval, y + 1);
            
            vvar.getVar({ x, y}, &vval);
            EXPECT_EQ(vval, (x + 1) * (y + 1));
        }
    }

  ex.close();
  
  fs::remove(tempfile);
}