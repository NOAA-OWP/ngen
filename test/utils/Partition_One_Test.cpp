#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <stdio.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>

#include "core/Partition_One.hpp"
#include "FileChecker.h"


class PartitionOneTest: public ::testing::Test {

    protected:

    std::vector<std::string> hydro_fabric_paths;

    std::string catchmentDataFile;
    geojson::GeoJSON catchment_collection, nexus_collection;

    std::vector<std::string> catchment_subset_ids;
    std::vector<std::string> nexus_subset_ids;

    std::unordered_set<std::string> catchment_ids;
    std::unordered_set<std::string> nexus_ids;

    Partition_One partition_one;
    PartitionData partition_data;

    PartitionOneTest() {} 

    ~PartitionOneTest() override {}

    std::string file_search(const std::vector<std::string> &parent_dir_options, const std::string& file_basename)
    {
        // Build vector of names by building combinations of the path and basename options
        std::vector<std::string> name_combinations;

        // Build so that all path names are tried for given basename before trying a different basename option
        for (auto & path_option : parent_dir_options)
            name_combinations.push_back(path_option + file_basename);

        return utils::FileChecker::find_first_readable(name_combinations);
    }

    void read_file_generate_partition_data()
    {
        const std::string file_path = file_search(hydro_fabric_paths, "catchment_data.geojson");
        catchment_collection = geojson::read(file_path, catchment_subset_ids);

        for(auto& feature: *catchment_collection)
        {
            std::string cat_id = feature->get_id();
            partition_data.catchment_ids.emplace(cat_id);
            std::string nex_id = feature->get_property("toid").as_string();
            partition_data.nexus_ids.emplace(nex_id);
        }
    }

    void read_file_nexus_data()
    {
        const std::string file_path = file_search(hydro_fabric_paths, "nexus_data.geojson");
        nexus_collection = geojson::read(file_path, nexus_subset_ids);
    }

    void SetUp() override;

    void TearDown() override;

    void setupArbitraryExampleCase();

};

void PartitionOneTest::SetUp() {
    setupArbitraryExampleCase();
}

void PartitionOneTest::TearDown() {

}

void PartitionOneTest::setupArbitraryExampleCase() {
    hydro_fabric_paths = {
        "data/",
        "./data/",
        "../data/",
        "../../data/",

    };
}

TEST_F(PartitionOneTest, TestPartitionData_1a)
{
    read_file_generate_partition_data();

    partition_one.generate_partition(catchment_collection);
    PartitionData data_struct = partition_one.partition_data;
    catchment_ids = data_struct.catchment_ids;

    //check catchment partition
    std::vector<std::string> cat_id_vec;
    // convert unordered_set to vector
    for (const auto& id: catchment_ids) {
        cat_id_vec.push_back(id);
    }

    //sort ids
    std::sort(cat_id_vec.begin(), cat_id_vec.end());
    //create set of unique ids
    std::set<std::string> unique(cat_id_vec.begin(), cat_id_vec.end());
    std::set<std::string> duplicates;
    //use set difference to identify all duplicates
    std::set_difference(cat_id_vec.begin(), cat_id_vec.end(), unique.begin(), unique.end(), std::inserter(duplicates, duplicates.end()));

    for( auto& id: duplicates){
        std::cout << "duplicates string set contains " << id << std::endl;
    }

    //process the original read in data
    std::vector<std::string> input_cat_ids;
    for(auto& feature: *catchment_collection)
    {
        std::string cat_id = feature->get_id();
        input_cat_ids.push_back(cat_id);
    }
    std::sort(input_cat_ids.begin(), input_cat_ids.end());
    
    for (int i = 0; i < input_cat_ids.size(); ++i) {
        if (input_cat_ids[i] != cat_id_vec[i]) {
            std::cout << "Input cat_id: " << input_cat_ids[i] << " differs from patition cat_id: " << cat_id_vec[i] << std::endl;
        }
    }

    //get input number of catchments
    int num_catchments = catchment_collection->get_size();

    ASSERT_EQ(catchment_ids.size(), num_catchments);
    ASSERT_EQ(duplicates.size(), 0);
}

TEST_F(PartitionOneTest, TestPartitionData_1b)
{
    read_file_generate_partition_data();
    read_file_nexus_data();

    partition_one.generate_partition(catchment_collection);
    PartitionData data_struct = partition_one.partition_data;
    nexus_ids = data_struct.nexus_ids;

    //check nexus partition
    std::vector<std::string> nex_id_vec;
    //convert unordered_set to vector
    for (const auto& id: nexus_ids) {
        nex_id_vec.push_back(id);
    }

    //sort ids
    std::sort(nex_id_vec.begin(), nex_id_vec.end());
    //create set of unique ids
    std::set<std::string> unique(nex_id_vec.begin(), nex_id_vec.end());
    std::set<std::string> duplicates;
    //use set difference to identify all duplicates
    std::set_difference(nex_id_vec.begin(), nex_id_vec.end(), unique.begin(), unique.end(), std::inserter(duplicates, duplicates.end()));

    for( auto& id: duplicates){
        std::cout << "duplicates string set contains " << id << std::endl;
    }

    //process the original read in data
    std::vector<std::string> input_nex_ids;
    for(auto& feature: *nexus_collection)
    {
        std::string nex_id = feature->get_id();
        input_nex_ids.push_back(nex_id);
    }
    std::sort(input_nex_ids.begin(), input_nex_ids.end());

    for (int i = 0; i < input_nex_ids.size(); ++i) {
        if (input_nex_ids[i] != nex_id_vec[i]) {
            std::cout << "Input nex_id: " << input_nex_ids[i] << " differs from patition nex_id: " << nex_id_vec[i] << std::endl;
        }
    }

    //get input number of nexus
    int num_nexus = nexus_collection->get_size();

    ASSERT_EQ(nexus_ids.size(), num_nexus);
    ASSERT_EQ(duplicates.size(), 0);
}
