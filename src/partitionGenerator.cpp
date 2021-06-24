#include <network.hpp>
#include <FileChecker.h>
#include <boost/lexical_cast.hpp>
#include <string>
#include <iostream>
#include <fstream>

std::vector<std::string> list_item;
//void write_part( int id,
void write_part( std::string id,
                 std::unordered_map<std::string, std::vector<std::string>>& catchment_parts,
                 std::unordered_map<std::string, std::vector<std::string>>& nexus_parts,
                 std::vector<std::pair<std::string, std::string>>& remote_up,
                 std::vector<std::pair<std::string, std::string>>& remote_down, int num_part, std::ofstream& outFile)
{
    // Write out catchment list
    outFile<<"        {\"id\":" << "\"" << id << "\"" <<", \"cat-ids\":[";
    for(auto const cat_id : catchment_parts)
        list_item = cat_id.second;
        for (std::vector<std::string>::const_iterator i = list_item.begin(); i != list_item.end(); ++i)   
            {
                if (i != (list_item.end()-1))
                    outFile <<"\"" << *i <<"\"" << ", ";
                else
                    outFile <<"\"" << *i <<"\"";
            }
    outFile<<"], ";

    // Write out nexus list
    outFile<<"\"nex-ids\":[], ";

    // Write out remote up 
    outFile<<"\"remote-up\":[], ";

    // Write out remote down
    std::pair<std::string, std::string> remote_down_list;
    outFile<<"\"remote-down\":[]";

    if (id != std::to_string(num_part-1))
       outFile<<"},";
    else
        outFile<<"}";
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
                write_part(partition_str, this_catchment_part, this_nexus_part, remote_up, remote_down, num_partitions, outFile);
                outFile << std::endl;

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
    outFile<<"    ]"<<std::endl;
    outFile<<"}"<<std::endl;

    outFile.close();

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

    return 0;
}

