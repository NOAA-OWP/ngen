#include <network.hpp>
#include <FileChecker.h>
#include <boost/lexical_cast.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <vector>
#include <unordered_set>
#include <tuple>

#include "core/Partition_Parser.hpp"

using PartitionVSet = std::vector<std::unordered_set<std::string> >;
/**
 * @brief A tuple representing a remote connection
 * 
 * These tuples contain (partition-id, nexus-id, catchment-id, topology-string)
 * 
 * Where
 * partition-id = partition containing the `nexus-id` and its related `catchment-id`
 * nexus-id = which nexus is participating in the remote connection
 * catchment-id = which catchment is topologically connected to the `nexus-id` involved in the communication
 * topoloy-string = "orig_cat-to-nex" iff `catchment-id` is an origin feature of `nexus-id`
 *                  "nex-to-dest_cat" iff `catchment-id` is a destination feature of `nexus-id`
 */
using RemoteConnection = std::tuple<int, std::string, std::string, std::string>;
using RemoteConnectionVec = std::vector< RemoteConnection >;

/**
 * @brief Write the partition details to the @p outFile
 * 
 * @param catchment_part 
 * @param nexus_part
 * @param remote_connections_vec 
 * @param num_part 
 * @param outFile 
 */
void write_remote_connections(PartitionVSet catchment_part, PartitionVSet nexus_part,
                 std::vector<RemoteConnectionVec> remote_connections_vec,
                 int num_part, std::ofstream& outFile)
{
    outFile<<"{"<<std::endl;
    outFile<<"    \"partitions\":["<<std::endl;

    int id = 0;
    std::streamoff backspace(2);
    // loop over all partitions
    for (int i =0; i < catchment_part.size(); ++i)
    //for (int i =0; i < 2; ++i)  // for a quick test
    {
        // write catchments
        outFile<<"        {\"id\":" << id <<",\n        \"cat-ids\":[";
        std::unordered_set<std::string> cat_set = catchment_part[i];
        // iterate over elements in catchment set
        for (auto it = cat_set.begin(); it != cat_set.end(); it++) {
            std::string catchment_id = *it;
            outFile <<"\"" << catchment_id <<"\"" << ", ";
        }
        outFile.seekp( outFile.tellp() - backspace );
        outFile<<"],\n";

        // write nexuses
        outFile<<"        \"nex-ids\":[";
        std::unordered_set<std::string> nex_set = nexus_part[i];
        // loop over elements in nexus set
        for (auto it = nex_set.begin(); it != nex_set.end(); it++) {
            std::string nexus_id = *it;
            outFile <<"\"" << nexus_id <<"\"" << ", ";
        }
        outFile.seekp( outFile.tellp() - backspace );
        outFile<<"],\n";

        // wrtie remote_connections
        RemoteConnectionVec remote_conn_vec;
        remote_conn_vec = remote_connections_vec[i];

        outFile<<"        \"remote-connections\":[";
        int vec_size = remote_conn_vec.size();
        int set_counter = 0;
        // loop over elements in remote connection vector of tuples
        for(auto const &remote_conn : remote_conn_vec)
        {
            // remote_conn is a tuple of {mpi_rank, nex-id, cat-id}
            int part_id = std::get<0>(remote_conn);
            std::string nexus_id = std::get<1>(remote_conn);
            std::string catchment_id = std::get<2>(remote_conn);
            const std::string catchment_direction = std::get<3>(remote_conn);
            {
                outFile << "{" << "\"mpi-rank\":" << part_id << ", ";
                outFile << "\"nex-id\":" << "\""<< nexus_id <<"\"" << ", ";
                outFile << "\"cat-id\":" << "\""<< catchment_id << "\"" << ", ";
                outFile << "\"cat-direction\":" << "\""<< catchment_direction << "\"" << "}";
                if (set_counter == (vec_size-1))
                {
                    outFile << "";
                }
                else
                {
                    outFile << ", ";
                }
            }
            set_counter++;
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
 * @brief Generate a vector of PartitionVSets by iterating the network and assigning catchments to partitions.
 * 
 * @param network 
 * @param num_partitions 
 * @param num_catchments
 * @param catchment_part 
 */
void generate_partitions(network::Network& network, const int& num_partitions, const int& num_catchments, PartitionVSet& catchment_part,
     PartitionVSet& nexus_part)
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
    std::unordered_set<std::string> catchment_set, nexus_set;
    std::string part_id, partition_str;
    std::vector<std::string> part_ids;

    std::pair<std::string, std::string> remote_up_id, remote_down_id, remote_up_part, remote_down_part;
    std::vector<std::pair<std::string, std::string> > remote_up, remote_down;

    std::string up_nexus;
    std::string down_nexus;
    for(const auto& catchment : network.filter("cat", network::SortOrder::TransposedDepthFirstPreorder)){
            if (partition < remainder)
                partition_size = partition_size_plus1;
            else
                partition_size = partition_size_norm;

            //Find all associated nexuses and add to nexus list
            //Some of these will end up being "remote" but still must be present in the
            //list of all required nexus the partition needs to worry about
            for( auto downstream : network.get_destination_ids(catchment) ){
                nexus_set.emplace(downstream);
                //nexus_list.push_back(downstream);
            }
            if(nexus_set.size() == 0){
                std::cerr<<"Error: Catchment "<<catchment<<" has no destination nexus.\n";
                exit(1);
            }
            for( auto upstream : network.get_origination_ids(catchment) ){
                nexus_set.emplace(upstream);
                //nexus_list.push_back(upstream);
            }
            //std::cout<<catchment<<" -> "<<nexus<<std::endl;

            //keep track of all the features in this partition
            catchment_set.emplace(catchment);
            counter++;
            if(counter == partition_size)
            {
                //std::cout<<"nexus "<<nexus<<" is remote DOWN on partition "<<partition<<std::endl;
                //FIXME partitioning shouldn't have to assume dendridic network
                std::vector<std::string> destinations = network.get_destination_ids(catchment);
                if(destinations.size() == 0){
                    std::cerr<<"Error: Catchment "<<catchment<<" has no destinatin nexus.\n";
                    exit(1);
                }
                down_nexus = destinations[0];

                part_id = std::to_string(partition);  // Is id used?
                partition_str = std::to_string(partition);

                //push the catchment_set and nexus_set on to a vector (can be a 1-d vector)
                part_ids.push_back(part_id);
                catchment_part.push_back(catchment_set);
                nexus_part.push_back(nexus_set);
                catchment_set.clear();
                nexus_set.clear();

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

                // Clear remote_up and remote_down vectors before next round
                remote_up.clear();
                remote_down.clear();

                partition++;
                counter = 0;
                //std::cout<<"\nnexus "<<nexus<<" is remote UP on partition "<<partition<<std::endl;

                //this nexus overlaps partitions
                //Handeled above by ensure all up/down stream nexuses are recorded 
                //nexus_list.push_back(nexus);
                up_nexus = down_nexus;
                //std::cout<<"\nin partition "<<partition<<":"<<std::endl;
            }
    }

    // validating catchment partition
    std::cout << "Validating catchments..." << std::endl;
    std::vector<std::string> cat_id_vec;
    for (int i =0; i < catchment_part.size(); ++i) {
        std::unordered_set<std::string> cat_set = catchment_part[i];
        // convert unordered_set to vector
        for (const auto &it: cat_set) {
            cat_id_vec.push_back(it);
        }
    }

    int i, j;
    for (i = 0; i < cat_id_vec.size(); ++i) {
        if (i%1000 == 0)
            std::cout << "i = " << i << std::endl;
        for (j = i+1; j < cat_id_vec.size(); ++j) {
            if ( cat_id_vec[i] == cat_id_vec[j] )
            {
                std::cout << "catchment duplication" << std::endl;
                exit(-1);
            }
        }
    }
    std::cout << "\nNumber of catchments is: " << cat_id_vec.size();
    std::cout << "\nCatchment validation completed" << std::endl;
}

/**
 * @brief Find the remote rank of a given feature in the partitions
 * 
 * @param id feature id to find in the partitions
 * @param catchment_partitions The global set of partitions
 * @return int partition number containing the id
 * 
 * @throws runtime_error if no partition contains the requested id
 */
int find_remote_rank(std::string id, PartitionVSet catchment_partitions)
{
    int pos = -1;
    for ( int i = 0; i < catchment_partitions.size(); ++i )
    {
        // this iterate through the unordered_set
        auto iter = std::find(catchment_partitions[i].begin(), catchment_partitions[i].end(), id);
        
        // if we find a match then we have found the target partition containing this id
        if ( iter != catchment_partitions[i].end() )
        {
            pos = i;
            break;
        }
    }
    if(pos < 0){
        std::string msg = "find_remote_rank: Could not find feature id "+id+" in any partition";
        throw std::runtime_error(msg);
    }
    return pos;
}

/**
 * @brief Find the remote connections for a given @p nexus
 * 
 * This function searches the local catchments in @p catchments to determine if the given
 * @p nexus can communicate with it on the local partition.
 * 
 * If the connected feature is NOT found locally, it is located in the @p catchment_partitions
 * and marked as remote by adding it to the remote_connections set.
 * 
 * The determination of the need to communicate is influenced by the topology.
 * 
 * Any @p nexus which is connected to a @p destination_ids_to_find id is mapped
 * as a remote connection where the @p nexus is considered to be a `nex-to-dest_cat`
 * which indicates that the nexus must send to the partition containing the destination catchment.
 * 
 * Any @p nexus which is connected to a @p origin_ids_to_find id is mapped
 * as a remote connection if and only if the @p partition_number contains a destination feature in @p destination_ids_to_find
 * which indicates that the nexus must receive from the partition containing the origin catchment
 *
 * @param nexus The nexus to identify remote connections for
 * @param catchment_partitions The global set of partitions
 * @param partition_number The partition to consider local
 * @param origin_ids_to_find The origin (upstream) ids connected to @p nexus to search on
 * @param destination_ids_to_find The destination (downstream) ids connected to @p nexus to search on
 * @param remote_connections The output unordered_set containing tuples of remote (partition, nexus, id)
 * @return int Number of identified remote catchments
 * 
 * @throws invalid_argument if the partition_number is not in the range of valid partition numbers (size of catchment_partitions)
 */
int find_partition_connections(std::string nexus, PartitionVSet catchment_partitions, int partition_number,  std::vector<std::string>& origin_ids_to_find, std::vector<std::string>& destination_ids_to_find, RemoteConnectionVec& remote_connections )
{

    const std::string origination_cat_to_nex = "orig_cat-to-nex";
    const std::string nex_to_destination_cat = "nex-to-dest_cat";

    if( partition_number < 0 || partition_number >= catchment_partitions.size() ){
        throw std::invalid_argument("find_partition_connections: partition_number not valid for catchment_partitions of size "+
                                     std::to_string( catchment_partitions.size()) + ".");
    }
    std::unordered_set<std::string> catchments = catchment_partitions[partition_number];
    int remote_catchments = 0;

    //Find senders
    for( auto id : destination_ids_to_find )
    {
        auto iter = std::find(catchments.begin(), catchments.end(), id);
        if ( iter == catchments.end() )
        {
            //we do not operate the receiving end of this nexus, it must be remote
            //so we need to indicate the need to send
            int pos = find_remote_rank(id, catchment_partitions);
            remote_connections.push_back(std::make_tuple(pos, nexus, id, nex_to_destination_cat));
            ++remote_catchments;
        }
        else
        {
            //we operate the receiving end of this nexus
            //it can either be a local nexus
            //or a remote nexus where this partition operates the catchment downstream
            //in the latter case, the remote connection is handled by the origin_ids (find receivers) loop below
        }
    }

    //Find receivers
    for( auto id : origin_ids_to_find )
    {
        auto iter = std::find(catchments.begin(), catchments.end(), id);
        if ( iter == catchments.end() )
        {
            //These are remotes I need establish connection with only if I operate the receiving end of the remote pairs
            //I am considered the receiver iff I contain the destination feature
            //we do not operate the sending end of this nexus, it must be remote
            //the appropriate sending tag should have been set in the previous destination_ids loop on appropriate partition
            for(auto did : destination_ids_to_find )
            {
                auto dest_iter = std::find(catchments.begin(), catchments.end(), did);
                if( dest_iter != catchments.end() )
                {
                    //We operate the receving end of this remote nexus for the communiction pair
                    // (id -> N) (N -> did)
                    //map that relationship
                    int pos = find_remote_rank(id, catchment_partitions);
                    remote_connections.push_back(std::make_tuple(pos, nexus, id, origination_cat_to_nex));
                    ++remote_catchments;
                }
            }
        }
        else
        {
            //we operate the sending end of this nexus,
            //it can either be a local nexus
            //or a remote nexus where this partition operates the catchment upstream,
            //in the latter case, the remote connection is handled by the destination_ids (find senders) loop above
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
    PartitionVSet catchment_part, nexus_part;
    
    //catchment_network.print_network();

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

    //Generate the partitioning
    generate_partitions(global_network, num_partitions, num_catchments, catchment_part, nexus_part);

    //global_network.print_network();

    //The container holding all remote_connections
    std::vector<RemoteConnectionVec> remote_connections_vec;

    int total_remotes = 0;
    // loop over all partitions by partition id
    for (int ipart=0; ipart < catchment_part.size(); ++ipart)
    //for (int ipart=0; ipart < 2; ++ipart) // for a quick test
    {
        // declare and initialize remote_connections
        RemoteConnectionVec remote_connections;

        //std::vector<std::string> local_cat_ids = catchment_part[ipart]["cat-ids"];
        std::unordered_set<std::string> local_cat_set = catchment_part[ipart];
        std::unordered_set<std::string> local_nex_set = nexus_part[ipart];
        std::unordered_set<std::string> local_node_set;
        
        std::merge(local_cat_set.begin(), local_cat_set.end(),
                local_nex_set.begin(), local_nex_set.end(),
                std::inserter(local_node_set, local_node_set.begin()));

        geojson::GeoJSON local_catchment_collection = std::make_shared<geojson::FeatureCollection>(*global_nexus_collection, local_node_set);

        // make a local network
        Network local_network(local_catchment_collection, &link_key);
        //NOTE: This doesn't include nexi in the network... if you check, this results in a difference of 1
        // local nexus in roughly half the cases between nexus_part[i] and local_nexuses... this doesn't *seem*
        // to have any practical effect on remote nexus determination, but it's still a mismatch.
        
        // test each nexus in the local network to make sure its upstream and downstream exist in the local network
        auto local_cats = local_network.filter("cat");
        auto local_nexuses = local_network.filter("nex");


        int remote_catchments = 0;

        for ( const auto& n : local_nexuses )
        {
            //Find upstream connections
            auto orgin_ids = global_network.get_origination_ids(n);
            //Find downstream connections
            auto dest_ids = global_network.get_destination_ids(n);
            remote_catchments += find_partition_connections(n, catchment_part, ipart, orgin_ids, dest_ids, remote_connections );
        }

        remote_connections_vec.push_back(remote_connections);
        
        //std::cout << "local network size: " << local_network.size() << "\n";
        //std::cout << "global network size " << global_network.size() << "\n";
        std::cout << "Found " << remote_catchments << " remotes in partition "<<ipart<<"\n";
        total_remotes += remote_catchments;
    }
    std::cout << "Found " << total_remotes << " total remotes (average of approximately " << (total_remotes/num_partitions) << " remotes per partition)" << std::endl;

    write_remote_connections(catchment_part, nexus_part, remote_connections_vec, num_partitions, outFile);

    outFile.close();
        
    return 0;
}
