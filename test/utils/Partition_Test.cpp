#include "gtest/gtest.h"
#include <stdio.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>

#include "core/Partition_Parser.hpp"
#include "FileChecker.h"


class PartitionsParserTest: public ::testing::Test {

    protected:

    std::vector<std::string> data_paths;
    std::vector<std::string> hydro_fabric_paths;

    PartitionsParserTest() {

    }

    ~PartitionsParserTest() override {

    }

    std::string file_search(const std::vector<std::string> &parent_dir_options, const std::string& file_basename)
    {
        // Build vector of names by building combinations of the path and basename options
        std::vector<std::string> name_combinations;

        // Build so that all path names are tried for given basename before trying a different basename option
        for (auto & path_option : parent_dir_options)
            name_combinations.push_back(path_option + file_basename);

        return utils::FileChecker::find_first_readable(name_combinations);
    }

    void SetUp() override;

    void TearDown() override;

    void setupArbitraryExampleCase();

};

void PartitionsParserTest::SetUp() {
    setupArbitraryExampleCase();
}

void PartitionsParserTest::TearDown() {

}

void PartitionsParserTest::setupArbitraryExampleCase() {
    data_paths = {
                "test/data/partitions/",
                "./test/data/partitions/",
                "../test/data/partitions/",
                "../../test/data/partitions/",
        };

    hydro_fabric_paths = {
        "data/",
        "./data/",
        "../data/",
        "../../data/",

    };
}

TEST_F(PartitionsParserTest, TestFileReader)
{
  const std::string file_path = file_search(data_paths,"partition_huc01.json");
  Partitions_Parser partitions_parser = Partitions_Parser(file_path);

  partitions_parser.parse_partition_file();

  ASSERT_TRUE(true);

}

TEST_F(PartitionsParserTest, DisplayPartitionData)
{
  const std::string file_path = file_search(data_paths,"partition_huc01.json");
  Partitions_Parser partitions_parser = Partitions_Parser(file_path);

  partitions_parser.parse_partition_file();

  //In real application, num_partitions may be an input parameter
  //For unit test, num_partitions value should be consistent with that of partition_huc01.json
  int num_partitions = 100;

  int i;
  int part_id;
  for (i = 0; i < num_partitions; ++i)
  //for (i = 0; i < 2; i++)
  {
    partitions_parser.get_partition_struct(i);
  }

  for (i = 0; i < num_partitions; ++i)
  //for (i = 0; i < 2; i++)
  {
    partitions_parser.get_mpi_rank(i);
  }

  
  ASSERT_TRUE(true);
  
}
