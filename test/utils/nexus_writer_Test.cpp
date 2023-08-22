#include <nexus_writer.hpp>

#include <gtest/gtest.h>

#ifdef NETCDF_ACTIVE
#include <netcdf>
#endif

class nexus_writer_Test : public ::testing::Test
{
  protected:
    ~nexus_writer_Test() override = default;

    void TearDown() override
    {
        unlink((tempfile + ".csv").c_str());
        unlink((tempfile + ".nc").c_str());
    }

    template<typename WriterType>
    std::string write_to_file()
    {
        std::unique_ptr<ngen::utils::nexus_writer> writer = std::make_unique<WriterType>();

        writer->init(1);
        writer->next(tempfile, 2);
        writer->write("wb-1", "nex-1", 5);
        writer->write("wb-2", "nex-2", 10);
        writer->flush();

        return tempfile + (
            std::is_same<WriterType, ngen::utils::nexus_netcdf_writer>::value
            ? ".nc"
            : ".csv"
        );
    }

  private:
    std::string tempfile = testing::TempDir() + "/ngen_nexus_writer__test";
};

TEST_F(nexus_writer_Test, csv_writer)
{
    std::string output = this->write_to_file<ngen::utils::nexus_csv_writer>();

    std::ifstream stream{ output, std::ios::in };
    if (!stream.good())
        FAIL() << "Failed to open stream";
    
    std::string line;
    stream >> line;
    ASSERT_EQ(line, "feature_id,segment_id,q_lateral");
    stream >> line;
    ASSERT_EQ(line, "nex-1,wb-1,5");
    stream >> line;
    ASSERT_EQ(line, "nex-2,wb-2,10");
    stream.close();
}

TEST_F(nexus_writer_Test, netcdf_writer)
{
#ifndef NETCDF_ACTIVE
    GTEST_SKIP() << "NetCDF is not available";
#else
    std::string output = this->write_to_file<ngen::utils::nexus_netcdf_writer>();

    netCDF::NcFile stream{ output, netCDF::NcFile::read };

    ASSERT_EQ(stream.getDimCount(), 1);
    ASSERT_EQ(stream.getVarCount(), 3);

    const auto& dim = stream.getDim("feature_id");
    ASSERT_FALSE(dim.isNull());
    ASSERT_EQ(dim.getSize(), 2);
    
    const auto& var_nexus_id = stream.getVar("feature_id");
    ASSERT_FALSE(var_nexus_id.isNull());
    char* nex_id_1 = nullptr;
    var_nexus_id.getVar({ 0 }, &nex_id_1);
    char* nex_id_2 = nullptr;
    var_nexus_id.getVar({ 1 }, &nex_id_2);
    ASSERT_STREQ(nex_id_1, "nex-1");
    ASSERT_STREQ(nex_id_2, "nex-2");
    delete nex_id_1;
    delete nex_id_2;

    const auto& var_segment_id = stream.getVar("segment_id");
    ASSERT_FALSE(var_segment_id.isNull());
    char* seg_id_1 = nullptr;
    var_segment_id.getVar({ 0 }, &seg_id_1);
    char* seg_id_2 = nullptr;
    var_segment_id.getVar({ 1 }, &seg_id_2);
    ASSERT_STREQ(seg_id_1, "wb-1");
    ASSERT_STREQ(seg_id_2, "wb-2");
    delete seg_id_1;
    delete seg_id_2;

    const auto& var_qlat = stream.getVar("q_lateral");
    ASSERT_FALSE(var_qlat.isNull());
    double qlat_1 = 0;
    var_qlat.getVar({ 0 }, &qlat_1);
    double qlat_2 = 0;
    var_qlat.getVar({ 1 }, &qlat_2);
    ASSERT_EQ(qlat_1, 5);
    ASSERT_EQ(qlat_2, 10);

    stream.close();
#endif
}
