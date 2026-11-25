#include <NGenConfig.h>

#include <FileChecker.h>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/join.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <iterator>
#include <set>
#include <vector>
#include <unordered_set>

#include <HydrofabricReaderFactory.hpp>

#include "partitioning/Partition_By_Nexus.hpp"

using PartitionVSet = std::vector<std::unordered_set<std::string> >;

/**
 * @brief Write the partition details to the @p outFile
 *
 * Every partition holds each nexus with all of the catchments that drain to it, so its
 * "remote-connections" are always empty. They are written anyway, for the sake of readers that
 * require them.
 *
 * @param catchment_part
 * @param nexus_part
 * @param outFile
 */
void write_partitions(const PartitionVSet& catchment_part, const PartitionVSet& nexus_part, std::ofstream& outFile)
{
    outFile<<"{"<<std::endl;
    outFile<<"    \"partitions\":["<<std::endl;

    auto quote = [] (const std::string& s) { return '"' + s + '"'; };

    using boost::algorithm::join;
    using boost::adaptors::transformed;

    for (int i =0; i < catchment_part.size(); ++i)
    {
        if (i != 0)
            outFile << ", " << std::endl;

        outFile << "        {\"id\":" << i << ",\n";

        // write catchments
        outFile << "        \"cat-ids\":[";
        outFile << join(catchment_part[i] | transformed(quote), ", ");
        outFile << "],\n";

        // write nexuses
        outFile << "        \"nex-ids\":[";
        outFile << join(nexus_part[i] | transformed(quote), ", ");
        outFile << "],\n";

        // write remote_connections, of which there are none
        outFile << "        \"remote-connections\":[]";

        outFile << "}";
    }
    outFile<<"    ]"<<std::endl;
    outFile<<"}"<<std::endl;
}

/**
 * @brief Generate a vector of PartitionVSets by assigning each nexus, with all of the catchments
 * that drain to it, to a partition.
 *
 * This is the partitioning ngen does when it partitions automatically: nexuses are cut into
 * contiguous blocks, weighted by the number of catchments that drain to each, so the partitions
 * need no remote connections.
 *
 * @param catchment_collection - the catchments to partition, each naming its nexus as "toid"
 * @param num_partitions
 * @param catchment_part  - which catchments will be simulation on each partition
 * @param nexus_part - which nexuses have contributing catchments on each partition
 */
void generate_partitions(const geojson::GeoJSON& catchment_collection, const int& num_partitions, PartitionVSet& catchment_part, PartitionVSet& nexus_part)
{
    using namespace ngen::partitioning;

    for (auto& partition : partition_by_nexus(catchment_collection, num_partitions)) {
        if (partition.catchment_ids.empty()) {
            throw std::runtime_error("Input error: the catchments drain to fewer nexuses than the number of partitions: "
                                     + std::to_string(num_partitions));
        }
        catchment_part.push_back(std::move(partition.catchment_ids));
        nexus_part.push_back(std::move(partition.nexus_ids));
    }

    // validating catchment partition
    std::cout << "Validating catchments..." << std::endl;
    std::vector<std::string> cat_id_vec;
    for (int i =0; i < catchment_part.size(); ++i) {
        std::unordered_set<std::string>& cat_set = catchment_part[i];
        // convert unordered_set to vector
        for (const auto &it: cat_set) {
            cat_id_vec.push_back(it);
        }
    }
    //sort ids
    std::sort(cat_id_vec.begin(), cat_id_vec.end());
    //create set of unique ids
    std::set<std::string> unique(cat_id_vec.begin(), cat_id_vec.end());
    std::set<std::string> duplicates;
    //use set difference to identify all duplicates
    std::set_difference(cat_id_vec.begin(), cat_id_vec.end(), unique.begin(), unique.end(), std::inserter(duplicates, duplicates.end()));
    if( duplicates.size() > 0 ){
        for( auto& id: duplicates){
            std::cout << "catchment "<<id<<" is duplicated!"<<std::endl;
        }
    }
    std::cout << "\nNumber of catchments is: " << cat_id_vec.size();
    std::cout << "\nCatchment validation completed" << std::endl;
}

void read_arguments(int argc, char* argv[],
                    std::string& catchmentDataFile,
                    std::string& nexusDataFile,
                    std::string& partitionOutFile,
                    int& numPartitions,
                    std::vector<std::string>& catchment_subset_ids,
                    std::vector<std::string>& nexus_subset_ids)
{
    if( argc < 7 ){
        std::cout << "Missing required args:" << std::endl;
        std::cout << argv[0] << " <catchment_data_path> <nexus_data_path> <partition_output_name> <number of partitions> <catchment_subset_ids> <nexus_subset_ids> " << std::endl;
        std::cout << "Use empty strings for subset_ids for no subsetting, e.g ''\nUse \'cat-X,cat-Y\' to partition only the defined catchments; nexus subset ids are ignored"<<std::endl;
        std::cout << "Note the use of single quotes, and no spaces between the ids.  (no quotes will also work, but  \"\" will not."<<std::endl;
        exit(-1);
    }

    bool error = false;
    if( !utils::FileChecker::file_is_readable(argv[1]) ) {
        std::cout << "catchment data path " << argv[1] << " not readable" << std::endl;
        error = true;
    } else {
        catchmentDataFile = argv[1];
    }

    if( !utils::FileChecker::file_is_readable(argv[2]) ) {
        std::cout << "nexus data path " << argv[2] << " not readable" << std::endl;
        error = true;
    } else {
        nexusDataFile = argv[2];
    }

    partitionOutFile = argv[3];
    if (partitionOutFile.empty()) {
        std::cout << "Missing output file name " << std::endl;
        error = true;
    }

    try {
        numPartitions = boost::lexical_cast<int>(argv[4]);
        if (numPartitions < 1) throw boost::bad_lexical_cast();
    }
    catch(boost::bad_lexical_cast &e) {
        std::cout << "number of partitions must be a positive integer." << std::endl;
        error = true;
    }

    //split the subset strings into vectors
    boost::split(catchment_subset_ids, argv[5], [](char c){return c == ','; } );
    boost::split(nexus_subset_ids, argv[6], [](char c){return c == ','; } );

    //If a single id or no id is passed, the subset vector will have size 1 and be the id or the ""
    //if we get an empty string, pop it from the subset list.
    if(catchment_subset_ids.size() == 1 && catchment_subset_ids[0] == "") {
        catchment_subset_ids.pop_back();
    }
    if(nexus_subset_ids.size() == 1 && nexus_subset_ids[0] == "") {
        nexus_subset_ids.pop_back();
    }

    if (error) exit(-1);
}

int main(int argc, char* argv[])
{
    std::string catchmentDataFile, nexusDataFile;
    std::string partitionOutFile;
    std::vector<std::string> catchment_subset_ids;
    std::vector<std::string> nexus_subset_ids;
    int num_partitions = 0;

    read_arguments(argc, argv,
                   catchmentDataFile, nexusDataFile, partitionOutFile,
                   num_partitions,
                   catchment_subset_ids, nexus_subset_ids);

    if (!nexus_subset_ids.empty()) {
        std::cerr << "Warning: nexus subset ids are ignored; each catchment's nexus is partitioned with it" << std::endl;
    }

    std::ofstream outFile;
    outFile.open(partitionOutFile, std::ios::trunc);

    //Get the connectivity of the catchments in the given hydrofabric
    std::unique_ptr<ngen::hydrofabric::HydrofabricReader> hydrofabric =
        ngen::hydrofabric::make_hydrofabric_reader(catchmentDataFile, nexusDataFile,
                                                   ngen::hydrofabric::Content::Connectivity);
    geojson::GeoJSON catchment_collection = hydrofabric->read_divides(catchment_subset_ids);
    int num_catchments = catchment_collection->get_size();
    std::cout<<"Partitioning "<<num_catchments<<" catchments into "<<num_partitions<<" partitions."<<std::endl;

    //Check that the number of partitions is less or equal to the number of catchment
    if (num_catchments < num_partitions) {
        throw std::runtime_error("Input error: total number of catchments: " + std::to_string(num_catchments) + \
                                 ", cannot be less than the number of partitions: " + std::to_string(num_partitions));
    }

    PartitionVSet catchment_part, nexus_part;

    //Generate the partitioning
    generate_partitions(catchment_collection, num_partitions, catchment_part, nexus_part);

    write_partitions(catchment_part, nexus_part, outFile);

    outFile.close();

    return 0;
}
