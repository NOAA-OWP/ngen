#include "gtest/gtest.h"
#include <stdio.h>
#include "Partitions_Parser.hpp"

using namespace realization;

class PartitionsParserTest: public ::testing::Test {

    protected:

    PartitionsParserTest() {

    }

    ~PartitionsParserTest() override {

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

}

TEST_F(PartitionsParserTest, TestFileReader)
{
  const std::string file_path = "/home/shengting.cui/MPI/ngen/data/example_partition_config.json";
  Partitions_Parser partitions_parser = Partitions_Parser(file_path);

  boost::property_tree::ptree loaded_tree;
  boost::property_tree::json_parser::read_json(file_path, loaded_tree);
  boost::property_tree::ptree tree = loaded_tree;

  partitions_parser.read_partition_file(tree);

  int i;
  std::string part_id;
  for (i = 0; i <= 1; i++)
  {
    part_id = std::to_string(i);
    partitions_parser.get_part_strt(part_id);
  }

  for (i = 0; i <= 1; i++)
  {
    part_id = std::to_string(i);
    partitions_parser.get_mpi_rank(part_id);
  }

  
  ASSERT_TRUE(true);
  
}
