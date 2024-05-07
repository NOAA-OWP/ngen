#include <NGenConfig.h>

#include "gtest/gtest.h"

#if NGEN_WITH_NETCDF
#include <netcdf>
#endif

#include "mdframe.hpp"

class mdframe_netcdf_Test : public ::testing::Test
{
  protected:
    mdframe_netcdf_Test()
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

        path.append("ngen__mdframe_Test_netCDF.nc");
    }

    ~mdframe_netcdf_Test() override
    {
        unlink(this->path.c_str());
    }

    std::string path;
};

// TODO: Convert to test fixture for setup/teardown members.
TEST_F(mdframe_netcdf_Test, io_netcdf)
{
#if !NGEN_WITH_NETCDF
    GTEST_SKIP() << "NetCDF is not available";
#else

    ngen::mdframe df;

    df.add_dimension("x", 2)
      .add_dimension("y", 2);

    df.add_variable<int>("x", { "x" })
      .add_variable<int>("y", { "y" })
      .add_variable<double>("v", {"x", "y"});

    for (size_t x = 0; x < 2; x++) {
        df["x"].insert({{ x }}, x + 1);
        for (size_t y = 0; y < 2; y++) {
            df["y"].insert({{ y }}, y + 1);
            df["v"].insert({{ x, y }}, (x + 1) * (y + 1));
        }
    }

    df.to_netcdf(this->path);

    netCDF::NcFile ex;
    ex.open(this->path, netCDF::NcFile::read);

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
#endif
}
