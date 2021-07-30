#include <network.hpp>
#include <FileChecker.h>
#include <boost/lexical_cast.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <vector>

#include "core/Partition_Parser.hpp"

using PartitionMap = std::unordered_map<std::string, std::vector<std::string> >;
using RemoteConnectionMap = std::unordered_map<std::string, std::vector<std::pair<std::string, int> > >;

/**
 * @brief Write the partition details to the @p outFile
 * 
 * @param catchment_part 
 * @param remote_connections_vec 
 * @param num_part 
 * @param outFile 
 */
void write_remote_connections(std::vector<PartitionMap > catchment_part,
                 std::vector<RemoteConnectionMap > remote_connections_vec,
                 int num_part, std::ofstream& outFile)
{
    outFile<<"{"<<std::endl;
    outFile<<"    \"partitions\":["<<std::endl;

    int id = 0;
    //for (std::vector<std::unordered_map<std::string, std::vector<std::string> > >::const_iterator i = catchment_part.begin();
    //     i != catchment_part.end(); ++i)
    std::streamoff backspace(2);
    for (int i =0; i < catchment_part.size(); ++i)
    //for (int i =0; i < 2; ++i)  // for a quick test
    {
        // write catchments
        outFile<<"        {\"id\":" << id <<",\n        \"cat-ids\":[";
        for(auto const cat_id : catchment_part[i]["cat-ids"])
        {
            outFile <<"\"" << cat_id <<"\"" << ", ";
        }
        outFile.seekp( outFile.tellp() - backspace );
        outFile<<"],\n";

        // write nexuses
        outFile<<"        \"nex-ids\":[";
        for(auto const nex_id : catchment_part[i]["nex-ids"])
        {
            outFile <<"\"" << nex_id <<"\"" << ", ";
        }
        outFile.seekp( outFile.tellp() - backspace );
        outFile<<"],\n";

        // wrtie remote_connections
        std::unordered_map<std::string, std::vector<std::pair<std::string, int> > > remote_conn_map;
        remote_conn_map = remote_connections_vec[i];

        outFile<<"        \"remote-connections\":[";
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
    outFile<<"    ]"<<std::endl;
    outFile<<"}"<<std::endl;
}

/**
 * @brief Generate a vector of PartitionMaps by iterating the network and assigning catchments to partitions.
 * 
 * @param network 
 * @param num_partitions 
 * @param num_catchments
 * @param catchment_part 
 */
void generate_partitions(network::Network& network, const int& num_partitions, const int& num_catchments, std::vector<PartitionMap>& catchment_part)
{
    int partition = 0;
    int counter = 0;
    int total = num_catchments;
    int partition_size = total/num_partitions;
    int partition_size_norm = partition_size;
    int remainder;
    remainder = total - partition_size*num_partitions;
    //int partition_size_plus1 = partition_size + 1;
    int partition_size_plus1 = ++partition_size;
    /**
    std::cout << "num_partition:" << num_partitions << std::endl;
    std::cout << "partition_size_norm:" << partition_size_norm << std::endl;
    std::cout << "partition_size_plus1:" << partition_size_plus1 << std::endl;
    std::cout << "remainder:" << remainder << std::endl;
    **/
    std::vector<std::string> catchment_list, nexus_list;
    std::vector<std::string> cat_vec_1d;
    std::vector<std::vector<std::string> > vec_cat_list;

    std::string id, partition_str, empty_up, empty_down;
    std::vector<std::string> empty_vec;
    std::unordered_map<std::string, std::string> this_part_id;
    std::unordered_map<std::string, std::vector<std::string> > this_catchment_part, this_nexus_part;
    std::vector<std::unordered_map<std::string, std::string> > part_ids;
    std::vector<PartitionMap> nexus_part;

    std::pair<std::string, std::string> remote_up_id, remote_down_id, remote_up_part, remote_down_part;
    std::vector<std::pair<std::string, std::string> > remote_up, remote_down;

    std::string up_nexus;
    std::string down_nexus;
    for(const auto& catchment : network.filter("cat")){
            if (partition < remainder)
                partition_size = partition_size_plus1;
            else
                partition_size = partition_size_norm;

            //Find all associated nexuses and add to nexus list
            //Some of these will end up being "remote" but still must be present in the
            //list of all required nexus the partition needs to worry about
            for( auto downstream : network.get_destination_ids(catchment) ){
                nexus_list.push_back(downstream);
            }
            for( auto upstream : network.get_origination_ids(catchment) ){
                nexus_list.push_back(upstream);
            }
            //std::cout<<catchment<<" -> "<<nexus<<std::endl;

            //keep track of all the features in this partition
            catchment_list.push_back(catchment);
            counter++;
            if(counter == partition_size)
            {
                //std::cout<<"nexus "<<nexus<<" is remote DOWN on partition "<<partition<<std::endl;
                //FIXME partitioning shouldn't have to assume dendridic network
                std::string nexus = network.get_destination_ids(catchment)[0];
                down_nexus = nexus;

                id = std::to_string(partition);
                partition_str = std::to_string(partition);
                this_part_id.emplace("id", partition_str);
                this_catchment_part.emplace("cat-ids", catchment_list);
                this_catchment_part.emplace("nex-ids", nexus_list);
                //this_nexus_part.emplace("nex-ids", nexus_list);
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
                //Handeled above by ensure all up/down stream nexuses are recorded 
                //nexus_list.push_back(nexus);
                up_nexus = nexus;
                //std::cout<<"\nin partition "<<partition<<":"<<std::endl;
            }
    }

    std::cout << "Validating catchments..." << std::endl;
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
}

/**
 * @brief Find the remote connections for a given @p nexus
 * 
 * This function searches the local catchments in @p catchments to determine if the given @p nexus can communicate with it on the local partition.
 * If the connected feature is NOT found locally, it is located in the @p catchment_partitions and marked as remote by adding it to the remote_connections map.
 * 
 * @param nexus The nexus to identify remote connections for
 * @param catchment_partitions The global set of partitions
 * @param partition_number The partition to consider local
 * @param ids_to_find The ids connected to @p nexus to search  on
 * @param remote_connections The output map containing pairs of remote (id, partition) keyed by the @p nexus
 * @return int Number of identified remote catchments
 * 
 * @throws invalid_argument if the partition_number is not in the range of valid partition numbers (size of catchment_partitions)
 */
int find_partition_connections(std::string nexus, std::vector<PartitionMap> catchment_partitions, int partition_number,  std::vector<std::string>& ids_to_find, RemoteConnectionMap& remote_connections )
{
    if( partition_number < 0 || partition_number >= catchment_partitions.size() ){
        throw std::invalid_argument("find_partition_connections: partition_number not valid for catchment_partitions of size "+
                                     std::to_string( catchment_partitions.size()) + ".");
    }
    std::vector<std::string> catchments = catchment_partitions[partition_number]["cat-ids"];
    int remote_catchments = 0;
    for( auto id : ids_to_find )
            {
                // try to get each origin id
                auto iter = std::find(catchments.begin(), catchments.end(), id);
                
                if ( iter == catchments.end() )
                {
                    // catchemnt is remote find the partition that contains it
                    //std::cout << id << ": is not in local catchment set searching remote partitions.\n";
                    
                    int pos = -1;
                    for ( int i = 0; i < catchment_partitions.size(); ++i )
                    {
                        auto iter2 = std::find(catchment_partitions[i]["cat-ids"].begin(), catchment_partitions[i]["cat-ids"].end(), id);
                        
                        // if we find a match then we have found the target partition containing this id
                        if ( iter2 != catchment_partitions[i]["cat-ids"].end() )
                        {
                            pos = i;
                            break;
                        }
                    }
                    
                    if ( pos >= 0 )
                    {
                        //std::cout << "Found id: " << id << " in partition: " << pos << "\n";
                        //remote_connections[n] = std::make_pair(id,pos);
                        remote_connections[nexus].push_back(std::make_pair(id,pos));
                        ++remote_catchments;
                    }
                    else
                    {
                        std::cout << "Could not find id: " << id << " in any partition\n";
                        ++remote_catchments;
                    }   
                }
                else
                {
                    //std::cout << "Catchment with id: " << id << " is local\n";
                }
            }
    return remote_catchments;

}

int main(int argc, char* argv[])
{
    using network::Network;
    std::string catchmentDataFile, nexusDataFile;
    std::string partitionOutFile;
    int num_partitions = 0;
    bool  error;
    if( argc < 7 ){
        std::cout << "Missing required args:" << std::endl;
        std::cout << argv[0] << " <catchment_data_path> <nexus_data_path> <partition_output_name> <number of partitions> <catchment_subset_ids> <nexus_subset_ids> " << std::endl;
        std::cout << "Use empty strings for subset_ids for no subsetting, e.g ''\nUse \'cat-X,cat-Y\', \'nex-X,nex-Y\' to partition only the defined catchment and nexus"<<std::endl;
        std::cout << "Note the use of single quotes, and no spaces between the ids.  (no quotes will also work, but  \"\" will not."<<std::endl;
        error = true;
    }
    else {
        error = false;
        if( !utils::FileChecker::file_is_readable(argv[1]) ) {
            std::cout<<"catchment data path "<<argv[1]<<" not readable"<<std::endl;
            error = true;
        }
        else{ catchmentDataFile = argv[1]; }

        if( !utils::FileChecker::file_is_readable(argv[2]) ) {
            std::cout<<"nexus data path "<<argv[2]<<" not readable"<<std::endl;
            error = true;
        }
        else{ nexusDataFile = argv[2]; }

        partitionOutFile = argv[3];
        if (partitionOutFile == "") {
            std::cout << "Missing output file name " << std::endl;
            error = true;
        }
    
        try {
            num_partitions = boost::lexical_cast<int>(argv[4]);
            if(num_partitions < 0) throw boost::bad_lexical_cast();
        }
        catch(boost::bad_lexical_cast &e) {
            std::cout<<"number of partitions must be a postive integer."<<std::endl;
            error = true;
        }
        
    }
    if(error) exit(-1);

    std::vector<std::string> catchment_subset_ids;
    std::vector<std::string> nexus_subset_ids;
    //split the subset strings into vectors
    boost::split(catchment_subset_ids, argv[5], [](char c){return c == ','; } );
    boost::split(nexus_subset_ids, argv[6], [](char c){return c == ','; } );

    //If a single id or no id is passed, the subset vector will have size 1 and be the id or the ""
    //if we get an empy string, pop it from the subset list.
    if(nexus_subset_ids.size() == 1 && nexus_subset_ids[0] == "") nexus_subset_ids.pop_back();
    if(catchment_subset_ids.size() == 1 && catchment_subset_ids[0] == "") catchment_subset_ids.pop_back();

    std::ofstream outFile;
    outFile.open(partitionOutFile, std::ios::trunc);

    //Get the feature collecion for the given hydrofabric
    geojson::GeoJSON catchment_collection = geojson::read(catchmentDataFile, catchment_subset_ids);
    int num_catchments = catchment_collection->get_size();
    std::cout<<"Partitioning "<<num_catchments<<" catchments into "<<num_partitions<<" partitions."<<std::endl;
    std::string link_key = "toid";
  
    Network catchment_network(catchment_collection, &link_key);
    //Assumes dendridic, can add check in network if needed.
    std::vector<PartitionMap>  catchment_part;
    
    //Generate the partitioning
    generate_partitions(catchment_network, num_partitions, num_catchments, catchment_part);

    //build the remote connections from network
    // read the nexus hydrofabric, reuse the catchments
    geojson::GeoJSON global_nexus_collection = geojson::read(nexusDataFile, nexus_subset_ids);

    //Now read the collection of catchments, iterate it and add them to the nexus collection
    //also link them by to->id
    //std::cout << "Iterating Catchment Features" << std::endl;
    for(auto& feature: *catchment_collection)
    {
        //feature->set_id(feature->get_property("ID").as_string());
        global_nexus_collection->add_feature(feature);
        //std::cout<<"Catchment "<<feature->get_id()<<" -> Nexus "<<feature->get_property("toID").as_string()<<std::endl;
    }

    global_nexus_collection->link_features_from_property(nullptr, &link_key);
    // make a global network
    Network global_network(global_nexus_collection);

    //The container holding all remote_connections
    std::vector<std::unordered_map<std::string, std::vector<std::pair<std::string, int> > > > remote_connections_vec;
    // loop over all partitions by partition id
    for (int ipart=0; ipart < catchment_part.size(); ++ipart)
    {
        // declare and initialize remote_connections
        RemoteConnectionMap remote_connections;

        std::vector<std::string> local_cat_ids = catchment_part[ipart]["cat-ids"];
        //TODO need more efficient method for doing this
        // read the local catchment collection (if possible change this to not re read the json file)
        geojson::GeoJSON local_catchment_collection = geojson::read(catchmentDataFile, local_cat_ids);
        
        // make a local network
        Network local_network(local_catchment_collection, &link_key);
        
        // test each nexus in the local network to make sure its upstream and downstream exist in the local network
        auto local_cats = local_network.filter("cat");
        auto local_nexuses = local_network.filter("nex");

        int remote_catchments = 0;
        
        for ( const auto& n : local_nexuses )
        {
            //std::cout << "Searching for catchements connected to " << n << "\n"; 
            //Find upstream connections
            auto orgin_ids = global_network.get_origination_ids(n);
            //std::cout << "Found " << orgin_ids.size() << " upstream catchments for nexus with id: " << n << "\n";
            remote_catchments += find_partition_connections(n, catchment_part, ipart, orgin_ids, remote_connections );
            //Find downstream connections
            auto dest_ids = global_network.get_destination_ids(n);
            //std::cout << "Found " << dest_ids.size() << " downstream catchments for nexus with id: " << n << "\n";
            remote_catchments += find_partition_connections(n, catchment_part, ipart, dest_ids, remote_connections );
        }

        remote_connections_vec.push_back(remote_connections);
        
        //std::cout << "local network size: " << local_network.size() << "\n";
        //std::cout << "global network size " << global_network.size() << "\n";
        std::cout << "Found " << remote_catchments << " remotes in partition "<<ipart<<"\n";

    }
    write_remote_connections(catchment_part, remote_connections_vec, num_partitions, outFile);

    outFile.close();
        
    return 0;
}
