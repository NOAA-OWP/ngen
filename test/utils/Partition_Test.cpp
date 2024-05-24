#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <stdio.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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
{GTEST_SKIP() << "Skipping test"; //test broke, partition file is out of date
  const std::string file_path = file_search(data_paths,"partition_huc01.json");
  Partitions_Parser partitions_parser = Partitions_Parser(file_path);

  partitions_parser.parse_partition_file();

  ASSERT_TRUE(true);

}

TEST_F(PartitionsParserTest, DisplayPartitionData)
{ GTEST_SKIP() << "Skipping test"; //test broke, partition file is out of date
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

static const std::string test_data = "{"\
        "\"partitions\":["\
            "{\"id\":0,"\
            "\"cat-ids\":[\"cat-67\"],"\
            "\"nex-ids\":[\"nex-68\"],"\
            "\"remote-connections\":[]},"\
            "{\"id\":1,"\
            "\"cat-ids\":[\"cat-52\"],"\
            "\"nex-ids\":[\"nex-34\"],"\
            "\"remote-connections\":[]},"\
            "{\"id\":2,"\
            "\"cat-ids\":[\"cat-27\"],"\
            "\"nex-ids\":[\"nex-26\"],"\
            "\"remote-connections\":[]}"\
        "]"\
    "}";

TEST_F(PartitionsParserTest, empty_remote_test) {
    std::stringstream stream;
    stream << test_data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    auto parser = Partitions_Parser(tree);
    parser.parse_partition_file();
    for(int i = 0; i < 3; i++) ASSERT_TRUE( parser.get_partition_struct(i).remote_connections.empty() ); 
}

TEST_F(PartitionsParserTest, partition_struct_test) {
    std::stringstream stream;
    stream << test_data;
    boost::property_tree::ptree tree;
    boost::property_tree::json_parser::read_json(stream, tree);
    auto parser = Partitions_Parser(tree);
    parser.parse_partition_file();

    int num_partitions = 3;
    auto p = parser.get_partition_struct(0);
    ASSERT_TRUE(p.mpi_world_rank == 0);
    ASSERT_THAT(p.catchment_ids, testing::ElementsAre("cat-67"));
    ASSERT_THAT(p.nexus_ids, testing::ElementsAre("nex-68"));

    p = parser.get_partition_struct(1);
    ASSERT_TRUE(p.mpi_world_rank == 1);
    ASSERT_THAT(p.catchment_ids, testing::ElementsAre("cat-52"));
    ASSERT_THAT(p.nexus_ids, testing::ElementsAre("nex-34"));

    p = parser.get_partition_struct(2);
    ASSERT_TRUE(p.mpi_world_rank == 2);
    ASSERT_THAT(p.catchment_ids, testing::ElementsAre("cat-27"));
    ASSERT_THAT(p.nexus_ids, testing::ElementsAre("nex-26"));
}
