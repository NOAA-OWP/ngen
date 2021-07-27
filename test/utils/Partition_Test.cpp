#include "gtest/gtest.h"
#include <stdio.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>

#include "core/Partition_Parser.hpp"
#include "FileChecker.h"
#include "network.hpp"


// This class provides tests of the partition data type used to represent a segment of the complete hydrofabric that is to be run on one MPI rank

class PartitionsParserTest: public ::testing::Test {

    protected:

    std::vector<std::string> data_paths;
    std::vector<std::string> hydro_fabric_paths;
    std::vector<std::string> ref_hydro_fabric_paths;

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
    
    ref_hydro_fabric_paths = {
        "/apd_common/test/hydrofabric/",   
    };  
}

TEST_F(PartitionsParserTest, TestFileReader)
{

  const std::string file_path = file_search(data_paths,"partition_huc01_100.json");
  Partitions_Parser partitions_parser = Partitions_Parser(file_path);

  partitions_parser.parse_partition_file();


  ASSERT_TRUE(true);
}

// This test verifys that the data in a partition file can be read

TEST_F(PartitionsParserTest, DisplayPartitionData)
{
    const std::string file_path = file_search(data_paths,"partition_huc01_100.json");
    Partitions_Parser partitions_parser = Partitions_Parser(file_path);

    partitions_parser.parse_partition_file();

    for( const auto& n : partitions_parser.partition_ranks )
    {
        std::cout << n.first << ": (";  
        
        const PartitionData& part_data = n.second;
        
        for ( const auto& s : part_data.cat_ids )
        {
            std::cout << s << " ";
        }
        
        std::cout << ") \n";
    }


    ASSERT_TRUE(true);
      
}

// This test shows how the global catchments and nexus hydrofabric files can be used to determine which nexus members of a 
// catchment have remote communications pairs, and to determine what MPI id those paris should reference.
// Currently disabled because the test requires data file that will not be added to the repository and is very slow

TEST_F(PartitionsParserTest, DISABLED_ReferenceHydrofabric)
{
    using network::Network;
    
    const std::string file_path = file_search(data_paths,"partition_huc01_100.json");
    const std::string global_catchment_data_path = file_search(hydro_fabric_paths,"catchment_data.geojson");
    
    std::string link_key = "toid";
    
    Partitions_Parser partition_parser = Partitions_Parser(file_path);

    // get the catchement lists from the partition files
    partition_parser.parse_partition_file();

    // read the global hydrofabric
    geojson::GeoJSON global_catchment_collection = geojson::read(file_search(ref_hydro_fabric_paths,"catchment_data.geojson"));
    geojson::GeoJSON global_nexus_collection = geojson::read(file_search(ref_hydro_fabric_paths,"nexus_data.geojson"));
    
    //Now read the collection of catchments, iterate it and add them to the nexus collection
    //also link them by to->id
    //std::cout << "Iterating Catchment Features" << std::endl;
    for(auto& feature: *global_catchment_collection)
    {
        //feature->set_id(feature->get_property("ID").as_string());
        global_nexus_collection->add_feature(feature);
        //std::cout<<"Catchment "<<feature->get_id()<<" -> Nexus "<<feature->get_property("toID").as_string()<<std::endl;
    }
    
    std::string linkage = "toid";
    global_nexus_collection->link_features_from_property(nullptr, &linkage);


    // make a global network
    Network global_network(global_nexus_collection);
  
    // get he local hydro fabric
    
    auto& partitions = partition_parser.partition_ranks;  // get the map of all partitions
    auto& local_data = partitions["1"];        // for some reason rank 0 has the string "root tree: 1" instead of 1
    
    // for( partition : partitions ) { ....
    std::unordered_map<std::string, std::pair<std::string, int> > remote_connections;
    
    // read the local catchment collection (if possible change this to not re read the json file)
    geojson::GeoJSON local_catchment_collection = geojson::read("/apd_common/test/hydrofabric/catchment_data.geojson", local_data.cat_ids);
    
    // test each nexus in the local network to make sure its upstream and downstream exist in the local network
    std::cout << "Printing local catchment ids.\n";
    for ( const auto& n : local_data.cat_ids )
    {
        std::cout << n << "\n";
    }
    
    // make a local network
    Network local_network(local_catchment_collection, &link_key);
    
    // test each nexus in the local network to make sure its upstream and downstream exist in the local network
    std::cout << "Printing local nexus ids.\n";
    auto local_cats = local_network.filter("cat");
    auto local_nexuses = local_network.filter("nex");
    
    bool test_value = true;
    int remote_catchments = 0;
    
    for ( const auto& n : local_nexuses )
    {
        std::cout << "Searching for catchements connected to " << n << "\n";
        
        auto orgin_ids = global_network.get_origination_ids(n);
        
        std::cout << "Found " << orgin_ids.size() << " upstream catchments for nexus with id: " << n << "\n";
        
        for( auto id : orgin_ids )
        {
            // try to get each origin id
            auto iter = std::find(local_cats.begin(), local_cats.end(), id);
            
            if ( iter == local_cats.end() )
            {
                // catchemnt is remote find the partition that contains it
                std::cout << id << ": is not in local catchment set searching remote partitions.\n";
                
                int pos = -1;
                for ( int i = 0; i < partitions.size(); ++i )
                {
                    std::string k = std::to_string(i);
                    auto iter2 = std::find(partitions[k].cat_ids.begin(), partitions[k].cat_ids.end(), id);
                    
                    // if we find a match then we have found the target partition containing this id
                    if ( iter2 != partitions[k].cat_ids.end() )
                    {
                        pos = i;
                        break;
                    }
                }
                
                if ( pos >= 0 )
                {
                    std::cout << "Found id: " << id << " in partition: " << pos << "\n";
                    remote_connections[n] = std::make_pair(id,pos);
                    ++remote_catchments;
                }
                else
                {
                    std::cout << "Could not find id: " << id << " in any partition\n";
                    test_value = false;
                    ++remote_catchments;
                }
                
                
            }
            else
            {
                std::cout << "Catchment with id: " << id << " is local\n";
            }
        }
        
        auto dest_ids = global_network.get_destination_ids(n);
        
        std::cout << "Found " << dest_ids.size() << " downstream catchments for nexus with id: " << n << "\n";
        
        for( auto id : dest_ids )
        {
            // try to get each origin id
            auto iter = std::find(local_cats.begin(), local_cats.end(), id);
            
            if ( iter == local_cats.end() )
            {
                // catchemnt is remote find the partition that contains it
                std::cout << id << ": is not in local catchment set searching remote partitions.\n";
                
                int pos = -1;
                for ( int i = 0; i < partitions.size(); ++i )
                {
                    std::string k = std::to_string(i);
                    auto iter2 = std::find(partitions[k].cat_ids.begin(), partitions[k].cat_ids.end(), id);
                    
                    // if we find a match then we have found the target partition containing this id
                    if ( iter2 != partitions[k].cat_ids.end() )
                    {
                        pos = i;
                        break;
                    }
                }
                
                if ( pos >= 0 )
                {
                    std::cout << "Found id: " << id << " in partition: " << pos << "\n";
                    remote_connections[n] = std::make_pair(id,pos);
                    ++remote_catchments;
                }
                else
                {
                    std::cout << "Could not find id: " << id << " in any partition\n";
                    test_value = false;
                    ++remote_catchments;
                }
                
                
            }
            else
            {
                std::cout << "Catchment with id: " << id << " is local\n";
            }
        }
        
    }
    
    std::cout << "local network size: " << local_network.size() << "\n";
    std::cout << "global network size " << global_network.size() << "\n";
    std::cout << "remote catchments found " << remote_catchments << "\n";
    


    ASSERT_TRUE(test_value);
}
