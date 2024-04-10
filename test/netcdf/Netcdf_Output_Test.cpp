#include "gtest/gtest.h"
#include "Simulation_Time.hpp"
#include <memory>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <ctime>
#include <time.h>

#include "output/NetcdfOutputWriter.hpp"

using namespace data_output;

class NetcdfOuputTest : public ::testing::Test {

    protected:

    void SetUp() override;

    void TearDown() override;
};


void NetcdfOuputTest::SetUp() {

}

void NetcdfOuputTest::TearDown() {
}

///Test simulation time Object
TEST_F(NetcdfOuputTest, TestNetcdfCreation) {

    // Setup the Variable and Dimension descriptions
    std::vector<data_output::NetcdfDimensionDiscription> dimension_discription;
    std::vector<data_output::NetcdfVariableDiscription> variable_discription;

    dimension_discription.push_back(data_output::NetcdfDimensionDiscription("time"));
    dimension_discription.push_back(data_output::NetcdfDimensionDiscription("lat",100));
    dimension_discription.push_back(data_output::NetcdfDimensionDiscription("lon",100));
    dimension_discription.push_back(data_output::NetcdfDimensionDiscription("cat-id",1000));

    variable_discription.push_back(data_output::NetcdfVariableDiscription("Ngen-Framework-Version", "string"));
    variable_discription.push_back(data_output::NetcdfVariableDiscription("lat", "float","lat"));
    variable_discription.push_back(data_output::NetcdfVariableDiscription("lon", "float", "lon"));
    variable_discription.push_back(data_output::NetcdfVariableDiscription("RAINRATE", "float", std::vector<std::string>{"time","lat","lon"}));
    variable_discription.push_back(data_output::NetcdfVariableDiscription("discharge", "float", std::vector<std::string>{"time","cat-id"}));

    // make the output file
    NetcdfOutputWriter output_file("netcdf-output-test.nc",dimension_discription,variable_discription);

    // check that the file matches expectation
    std::shared_ptr<NcFile>& ncfile = output_file.ncfile();

    // testing the dimensions
    auto dimension_map = ncfile->getDims();
    EXPECT_EQ(dimension_map.find("time")->second.getName(), std::string("time"));
    EXPECT_EQ(dimension_map.find("time")->second.getSize(), 0); 

    EXPECT_EQ(dimension_map.find("lat")->second.getName(), std::string("lat"));
    EXPECT_EQ(dimension_map.find("lat")->second.getSize(), 100); 

    EXPECT_EQ(dimension_map.find("lon")->second.getName(), std::string("lon"));
    EXPECT_EQ(dimension_map.find("lon")->second.getSize(), 100); 

    EXPECT_EQ(dimension_map.find("cat-id")->second.getName(), std::string("cat-id"));
    EXPECT_EQ(dimension_map.find("cat-id")->second.getSize(), 1000); 

    // testing the variables
    auto variable_map = ncfile->getVars();
    EXPECT_EQ(variable_map.find("Ngen-Framework-Version")->second.getName(), std::string("Ngen-Framework-Version"));
    EXPECT_EQ(variable_map.find("Ngen-Framework-Version")->second.getDimCount(), 0);
    EXPECT_EQ(variable_map.find("Ngen-Framework-Version")->second.getType(), NC_STRING);

    EXPECT_EQ(variable_map.find("lat")->second.getName(), std::string("lat"));
    EXPECT_EQ(variable_map.find("lat")->second.getDimCount(), 1);
    EXPECT_EQ(variable_map.find("lat")->second.getType(), NC_FLOAT);

    EXPECT_EQ(variable_map.find("lon")->second.getName(), std::string("lon"));
    EXPECT_EQ(variable_map.find("lon")->second.getDimCount(), 1);
    EXPECT_EQ(variable_map.find("lon")->second.getType(), NC_FLOAT);

    EXPECT_EQ(variable_map.find("RAINRATE")->second.getName(), std::string("RAINRATE"));
    EXPECT_EQ(variable_map.find("RAINRATE")->second.getDimCount(), 3);
    EXPECT_EQ(variable_map.find("RAINRATE")->second.getType(), NC_FLOAT);

    EXPECT_EQ(variable_map.find("discharge")->second.getName(), std::string("discharge"));
    EXPECT_EQ(variable_map.find("discharge")->second.getDimCount(), 2);
    EXPECT_EQ(variable_map.find("discharge")->second.getType(), NC_FLOAT);
    
    SUCCEED();
}


TEST_F(NetcdfOuputTest, TestNetcdfWrite) {
        // Setup the Variable and Dimension descriptions
    std::vector<data_output::NetcdfDimensionDiscription> dimension_discription;
    std::vector<data_output::NetcdfVariableDiscription> variable_discription;

    dimension_discription.push_back(data_output::NetcdfDimensionDiscription("time"));
    dimension_discription.push_back(data_output::NetcdfDimensionDiscription("x",100));
    dimension_discription.push_back(data_output::NetcdfDimensionDiscription("y",100));
    dimension_discription.push_back(data_output::NetcdfDimensionDiscription("id",1000));

    variable_discription.push_back(data_output::NetcdfVariableDiscription("version", "string"));
    variable_discription.push_back(data_output::NetcdfVariableDiscription("x", "float","x"));
    variable_discription.push_back(data_output::NetcdfVariableDiscription("y", "float", "y"));
    variable_discription.push_back(data_output::NetcdfVariableDiscription("output1", "float", std::vector<std::string>{"time","x","y"}));
    variable_discription.push_back(data_output::NetcdfVariableDiscription("output2", "float", std::vector<std::string>{"time","id"}));
    
    NetcdfOutputWriter output_file("netcdf-write-test-1.nc",dimension_discription, variable_discription);

    std::vector<float> data_vec(1000);

    for( std::size_t i = 0; i < data_vec.size(); ++i)
    {
        data_vec[i] = i * 0.1f;
    }

    output_file["output2"] << nc_offset(0,0) << nc_stride(1,1000) << data_vec;

    SUCCEED();
}

