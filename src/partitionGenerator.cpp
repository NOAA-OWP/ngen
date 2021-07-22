#include <network.hpp>
#include <FileChecker.h>
#include <boost/lexical_cast.hpp>
#include <string>
#include <iostream>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <vector>

#include "core/Partition_Parser.hpp"


void write_remote_connections(std::vector<std::unordered_map<std::string, std::vector<std::string> > > catchment_part,
                 std::vector<std::unordered_map<std::string, std::vector<std::pair<std::string, int> > > > remote_connections_vec,
                 int num_part, std::ofstream& outFile)
{
    int id = 0;
    //for (std::vector<std::unordered_map<std::string, std::vector<std::string> > >::const_iterator i = catchment_part.begin();
    //     i != catchment_part.end(); ++i)
    for (int i =0; i < catchment_part.size(); ++i)
    //for (int i =0; i < 2; ++i)  // for a quick test
    {
        // write catchments
        std::unordered_map<std::string, std::vector<std::string> > catchment_map;
        catchment_map = catchment_part[i];
        
        outFile<<"        {\"id\":" << id <<", \"cat-ids\":[";
        for(auto const cat_id : catchment_map)
        {
            std::vector<std::string> list_item = cat_id.second;
            for (std::vector<std::string>::const_iterator j = list_item.begin(); j != list_item.end(); ++j)
                {
                    if (j != (list_item.end()-1))
                        outFile <<"\"" << *j <<"\"" << ", ";
                    else
                        outFile <<"\"" << *j <<"\"";
                }
        }
        outFile<<"], ";

        // wrtie remote_connections
        std::unordered_map<std::string, std::vector<std::pair<std::string, int> > > remote_conn_map;
        remote_conn_map = remote_connections_vec[i];

        outFile<<"\"remote-connections\":[";
        // loop through elements in map
        int map_size = remote_conn_map.size();
        int map_counter = 0;
        for(auto const remote_conn : remote_conn_map)
        {
            // map key is nexus_id, each nexus_id may have more than one catchment, both up stream and down stream
            std::string nexus_id = remote_conn.first;

            // map value is vector of pairs
            std::vector<std::pair<std::string, int> > list_pairs = remote_conn.second;

            // loop through list of pairs
            for (std::vector<std::pair<std::string, int> >::const_iterator j = list_pairs.begin(); j != list_pairs.end(); ++j)
            {
                outFile << "{" << "\"nex-id\":" << "\""<< nexus_id <<"\"" << ", ";
                std::string catchment_id = (*j).first;
                int part_id = (*j).second;
                outFile << "\"cat-id\":" << "\""<< catchment_id <<"\"" << ", ";
                outFile << "\"mpi-rank\":" << part_id  << "}";
                if (map_counter == (map_size-1))
                {
                    {
                    if (j != (list_pairs.end()-1))
                        outFile << ", ";
                    else
                        outFile << "";
                    }
                }
                else
                {
                    outFile << ", ";
                }
            }
            map_counter++;
        }
        outFile<<"]";

        if (id != (num_part-1))
        //if (id != 1)  // for a quick test
            outFile<<"}," << std::endl;
        else
            outFile<<"}" << std::endl;

        id++;
    }
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


int main(int argc, char* argv[])
{
    std::string catchmentDataFile;
    std::string partitionOutFile;
    int num_partitions = 0;
    int num_catchments = 0;

    if( argc < 5 ){
        std::cout << "Missing required args:" << std::endl;
        std::cout << argv[0] << " <catchment_data_path> <number of partitions>" << std::endl;
    }
    else {
        bool error = false;
        if( !utils::FileChecker::file_is_readable(argv[1]) ) {
            std::cout<<"catchment data path "<<argv[1]<<" not readable"<<std::endl;
            error = true;
        }
        else{ catchmentDataFile = argv[1]; }

        partitionOutFile = argv[2];
        if (partitionOutFile == "") {
            std::cout << "Missing output file name " << std::endl;
            error = true;
        }
    
        try {
            num_partitions = boost::lexical_cast<int>(argv[3]);
            if(num_partitions < 0) throw boost::bad_lexical_cast();
        }
        catch(boost::bad_lexical_cast &e) {
            std::cout<<"number of partitions must be a postive integer."<<std::endl;
            error = true;
        }

        try {
            num_catchments = boost::lexical_cast<int>(argv[4]);
            if(num_catchments < 0) throw boost::bad_lexical_cast();
        }
        catch(boost::bad_lexical_cast &e) {
            std::cout<<"number of catchments must be a postive integer."<<std::endl;
            error = true;
        }

        if(error) exit(-1);
    }

    std::ofstream outFile;
    outFile.open(partitionOutFile, std::ios::trunc);

    //Get the feature collecion for the given hydrofabric
    geojson::GeoJSON catchment_collection = geojson::read(catchmentDataFile);
    std::string link_key = "toid";
  
    network::Network network(catchment_collection, &link_key);
    //Assumes dendridic, can add check in network if needed.
    int partition = 0;
    int counter = 0;
    //int total = network.size()/2; //Note network.size is the number of catchments + nexuses.  This should be a rough count.
    int total = num_catchments;
    int partition_size = total/num_partitions;
    int partition_size_norm = partition_size;
    int remainder;
    remainder = total - partition_size*num_partitions;
    //int partition_size_plus1 = partition_size + 1;
    int partition_size_plus1 = ++partition_size;
    std::cout << "num_partition:" << num_partitions << std::endl;
    std::cout << "partition_size_norm:" << partition_size_norm << std::endl;
    std::cout << "partition_size_plus1:" << partition_size_plus1 << std::endl;
    std::cout << "remainder:" << remainder << std::endl;
    std::vector<std::string> catchment_list, nexus_list;
    std::vector<std::string> cat_vec_1d;
    std::vector<std::vector<std::string> > vec_cat_list;

    std::string id, partition_str, empty_up, empty_down;
    std::vector<std::string> empty_vec;
    std::unordered_map<std::string, std::string> this_part_id;
    std::unordered_map<std::string, std::vector<std::string> > this_catchment_part, this_nexus_part;
    std::vector<std::unordered_map<std::string, std::string> > part_ids;
    std::vector<std::unordered_map<std::string, std::vector<std::string> > > catchment_part, nexus_part;

    std::pair<std::string, std::string> remote_up_id, remote_down_id, remote_up_part, remote_down_part;
    std::vector<std::pair<std::string, std::string> > remote_up, remote_down;

    outFile<<"{"<<std::endl;
    outFile<<"    \"partitions\":["<<std::endl;
    std::cout<<"in partition 0:"<<std::endl;
    std::string up_nexus;
    std::string down_nexus;
    for(const auto& catchment : network.filter("cat")){
            if (partition < remainder)
                partition_size = partition_size_plus1;
            else
                partition_size = partition_size_norm;

            std::string nexus = network.get_destination_ids(catchment)[0];
            //std::cout<<catchment<<" -> "<<nexus<<std::endl;

            //keep track of all the features in this partition
            catchment_list.push_back(catchment);
            nexus_list.push_back(nexus);
            counter++;
            if(counter == partition_size)
            {
                //std::cout<<"nexus "<<nexus<<" is remote DOWN on partition "<<partition<<std::endl;
                down_nexus = nexus;

                id = std::to_string(partition);
                partition_str = std::to_string(partition);
                this_part_id.emplace("id", partition_str);
                this_catchment_part.emplace("cat-ids", catchment_list);
                this_nexus_part.emplace("nex-ids", nexus_list);
                part_ids.push_back(this_part_id);
                catchment_part.push_back(this_catchment_part);
                nexus_part.push_back(this_nexus_part);

                vec_cat_list.push_back(catchment_list);

                if (partition == 0)
                {
                    remote_up_id = std::make_pair ("id", "\0");
                    remote_up_part = std::make_pair ("partition", "\0");
                    remote_up.push_back(remote_up_id);
                    remote_up.push_back(remote_up_part);
                }
                else
                {
                    partition_str = std::to_string(partition-1);
                    remote_up_id = std::make_pair ("id", up_nexus);
                    remote_up_part = std::make_pair ("partition", partition_str);
                    remote_up.push_back(remote_up_id);
                    remote_up.push_back(remote_up_part);
                }

                partition_str = std::to_string(partition+1);
                remote_down_id = std::make_pair ("id", down_nexus);
                remote_down_part = std::make_pair ("partition", partition_str);
                remote_down.push_back(remote_down_id);
                remote_down.push_back(remote_down_part);

                partition_str = std::to_string(partition);
                //write_part(partition_str, this_catchment_part, this_nexus_part, remote_up, remote_down, num_partitions, outFile);
                //outFile << std::endl;

                // Clear unordered_map before next round of emplace
                this_part_id.clear();
                this_catchment_part.clear();
                this_nexus_part.clear();

                // Clear remote_up and remote_down vectors before next round
                remote_up.clear();
                remote_down.clear();

                partition++;
                counter = 0;
                //std::cout<<"\nnexus "<<nexus<<" is remote UP on partition "<<partition<<std::endl;

                catchment_list.clear();
                nexus_list.clear();

                //this nexus overlaps partitions
                nexus_list.push_back(nexus);
                up_nexus = nexus;
                //std::cout<<"\nin partition "<<partition<<":"<<std::endl;
            }
    }
        //write_remote_connections(catchment_part, num_partitions, outFile);

    std::cout << "Validating catchments:" << std::endl;
    //converting vector to 1-d
    for(int i = 0; i < vec_cat_list.size(); ++i)
    {
        for(int j = 0; j < vec_cat_list[i].size(); ++j)
        {
            cat_vec_1d.push_back(vec_cat_list[i][j]);
        }
    }

    int i, j;
    for(i = 0; i < cat_vec_1d.size(); ++i) {
        if (i%1000 == 0)
            std::cout << "i = " << i << std::endl;
        for (j = i+1; j < cat_vec_1d.size(); ++j)
            if ( cat_vec_1d[i] == cat_vec_1d[j] )
            {
                std::cout << "catchment duplication" << std::endl;
                exit(-1);
            }
    }
    std::cout << "Catchment validation completed" << std::endl;


// read in the partition file and build the remote connections from network
    using network::Network;

    std::vector<std::string> data_paths;

    data_paths = {
                "test/data/partitions/",
                "./test/data/partitions/",
                "../test/data/partitions/",
                "../../test/data/partitions/",
        };

    // partition_huc01.json file format need to be consistent with that used by partition class
    const std::string file_path = file_search(data_paths,"partition_huc01.json");
    //std::unordered_map<std::string, std::pair<std::string, int> > remote_connections;
    
    Partitions_Parser partition_parser = Partitions_Parser(file_path);

    // get the catchement lists from the partition files
    partition_parser.parse_partition_file();

    // read the global hydrofabric
    geojson::GeoJSON global_catchment_collection = geojson::read("/apd_common/test/hydrofabric/catchment_data.geojson");
    geojson::GeoJSON global_nexus_collection = geojson::read("/apd_common/test/hydrofabric/nexus_data.geojson");
    
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

    std::vector<std::unordered_map<std::string, std::vector<std::pair<std::string, int> > > > remote_connections_vec;

    // loop over all partitions by partition id
    for (int ipart; ipart < num_partitions; ++ipart)
    //for (int ipart; ipart < 2; ++ipart)  // for a quick test of the code
    {
    // loop over all partitions by the order in the unordered_map
    for (const auto& partition : partitions) {
        //auto& local_data = partitions["1"];        // for some reason rank 0 has the string "root tree: 1" instead of 1
        int part_idn;
        //std::cout << "partition.first = " << partition.first << std::endl;
        //std::cout << "type of partition.first is: " << typeid(partition.first).name() << std::endl;
        part_idn = std::stoi(partition.first);
        //std::cout << "new type of part_idn is: " << typeid(part_idn).name() << " value = " << part_idn << std::endl;

      // choose the one that match the "ipart", this should reorder the remote_connections by ipart
      if (part_idn == ipart)
      {
        // declare and initialize remote_connections
        std::unordered_map<std::string, std::vector<std::pair<std::string, int> > > remote_connections;
        auto& local_data = partition.second;
        
        //TODO need more efficient method for doing this
        // read the local catchment collection (if possible change this to not re read the json file)
        geojson::GeoJSON local_catchment_collection = geojson::read("/apd_common/test/hydrofabric/catchment_data.geojson", local_data.cat_ids);
        
        // test each nexus in the local network to make sure its upstream and downstream exist in the local network
        /*
        std::cout << "Printing local catchment ids.\n";
        for ( const auto& n : local_data.cat_ids )
        {
            std::cout << n << "\n";
        }
        */
        
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
                        //remote_connections[n] = std::make_pair(id,pos);
                        remote_connections[n].push_back(std::make_pair(id,pos));
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
                        //remote_connections[n] = std::make_pair(id,pos);
                        remote_connections[n].push_back(std::make_pair(id,pos));
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

        remote_connections_vec.push_back(remote_connections);
        
        std::cout << "local network size: " << local_network.size() << "\n";
        std::cout << "global network size " << global_network.size() << "\n";
        std::cout << "remote catchments found " << remote_catchments << "\n";

      }
    }
    }
        write_remote_connections(catchment_part, remote_connections_vec, num_partitions, outFile);

    outFile<<"    ]"<<std::endl;
    outFile<<"}"<<std::endl;

    outFile.close();
        
    return 0;
}

