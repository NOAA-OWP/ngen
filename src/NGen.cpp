#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

#include "realizations/catchment/Formulation_Manager.hpp"
#include <Catchment_Formulation.hpp>
#include <HY_Features.hpp>

#include "NGenConfig.h"
#include "tshirt_params.h"

#include <FileChecker.h>
#include <boost/algorithm/string.hpp>

#ifdef ACTIVATE_PYTHON
#include <pybind11/embed.h>
namespace py = pybind11;
#endif // ACTIVATE_PYTHON
    
#ifdef NGEN_ROUTING_ACTIVE
#include "routing/Routing_Py_Adapter.hpp"
#endif // NGEN_ROUTING_ACTIVE

std::string catchmentDataFile = "";
std::string nexusDataFile = "";
std::string REALIZATION_CONFIG_PATH = "";
bool is_subdivided_hydrofabric_wanted = false;

#ifdef NGEN_MPI_ACTIVE

#ifndef MPI_HF_SUB_CLI_FLAG
#define MPI_HF_SUB_CLI_FLAG "--subdivided-hydrofabric"
#endif

#ifndef MPI_HF_SUB_CODE_GOOD
#define MPI_HF_SUB_CODE_GOOD 0
#endif

#ifndef MPI_HF_SUB_CODE_BAD
#define MPI_HF_SUB_CODE_BAD 1
#endif

#include <mpi.h>
#include "core/Partition_Parser.hpp"
#include <HY_Features_MPI.hpp>
#include "PyHydrofabricSubsetter.hpp"

std::string PARTITION_PATH = "";
int mpi_rank;
int mpi_num_procs;
#endif

std::unordered_map<std::string, std::ofstream> nexus_outfiles;

//Note: Use below if developing in-memory transfer of nexus flows to routing
//std::unordered_map<std::string, std::vector<double>> nexus_flows;

pdm03_struct get_et_params() {
  // create the struct used for ET
    pdm03_struct pdm_et_data;
    pdm_et_data.scaled_distribution_fn_shape_parameter = 1.3;
    pdm_et_data.vegetation_adjustment = 0.99;
    pdm_et_data.model_time_step = 0.0;
    pdm_et_data.max_height_soil_moisture_storerage_tank = 400.0;
    pdm_et_data.maximum_combined_contents = pdm_et_data.max_height_soil_moisture_storerage_tank /
                                            (1.0 + pdm_et_data.scaled_distribution_fn_shape_parameter);
    return pdm_et_data;
}

#ifdef NGEN_MPI_ACTIVE

/**
 * Perform an "AND" type sync across all ranks to synchronize whether all are in a 'success' or 'ready' state.
 *
 * Function should be called at the same time by all ranks to sync on some 'success' or 'ready' state for all the ranks.
 * It handles the required MPI communication and the processing of received message content.
 *
 * Function accepts a value for some boolean status property.  This initial value is applicable only to the local MPI
 * rank, with all MPI ranks having the status property and each having its own independent value.  The function has all
 * ranks (except ``0``) send their local status to rank ``0``.  Rank ``0`` then applies a Boolean AND to the statuses
 * (including its own) to produce a global status value.  The global status is then broadcast back to the other ranks.
 * Finally, the value indicated by this global status is returned.
 *
 * @param status The initial individual state for the current MPI rank.
 * @param taskDesc A description of the related task, used by rank 0 to print a message when included for any rank that
 *                 is not in the success/ready state.
 * @return Whether all ranks coordinating status had a success/ready status value.
 */
bool mpiSyncStatusAnd(bool status, const std::string &taskDesc) {
    // Expect 0 is good and 1 is no good for goodCode
    // TODO: assert this in constructor or somewhere, or maybe just in a unit test
    unsigned short codeBuffer;
    bool printMessage = !taskDesc.empty();
    // For the other ranks, start by properly setting the status code value in the buffer and send to rank 0
    if (mpi_rank != 0) {
        codeBuffer = status ? MPI_HF_SUB_CODE_GOOD : MPI_HF_SUB_CODE_BAD;
        MPI_Send(&codeBuffer, 1, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD);
    }
    // In rank 0, the first step is to receive and process codes from the other ranks into unified global status
    else {
        for (int i = 1; i < mpi_num_procs; ++i) {
            MPI_Recv(&codeBuffer, 1, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // If any is ever "not good", overwrite status to be "false"
            if (codeBuffer != MPI_HF_SUB_CODE_GOOD) {
                if (printMessage) {
                    std::cout << "Rank " << i << " not successful/ready after " << taskDesc << std::endl;
                }
                status = false;
            }
        }
        // Rank 0 must also now prepare the codeBuffer value for broadcasting the global status
        codeBuffer = status ? MPI_HF_SUB_CODE_GOOD : MPI_HF_SUB_CODE_BAD;
    }
    // Execute broadcast of global status rooted at rank 0
    MPI_Bcast(&codeBuffer, mpi_num_procs - 1, MPI_UNSIGNED_SHORT, 0, MPI_COMM_WORLD);
    return codeBuffer == MPI_HF_SUB_CODE_GOOD;
}

/**
 * Convenience method for overloaded function when no message is needed, and thus no description param provided.
 *
 * @param status The initial individual state for the current MPI rank.
 * @return Whether all ranks coordinating status had a success/ready status value.
 * @see mpiSyncStatusAnd(bool, const std::string&)
 */
bool mpiSyncStatusAnd(bool status) {
    return mpiSyncStatusAnd(status, "");
}

/**
 * Check whether the parameter hydrofabric files have been subdivided into appropriate per partition files.
 *
 * Checks to see if partition specific subfiles corresponding to each partition/process already exist.  E.g., for a file
 * at ``/dirname/catchment_data.geojson`` and two MPI processes, checks if both ``/dirname/catchment_data.geojson.0``
 * and ``/dirname/catchment_data.geojson.1 `` exist.
 *
 * This check is performed for both the catchment and nexus hydrofabric base file names, as stored in the global
 * ``catchmentDataFile`` and ``nexusDataFile`` variables respectively.  The number of MPI processes is obtained from the
 * global ``mpi_rank`` variable.
 *
 * @param printMessage Whether a supplemental message should be printed to standard out indicating status.
 *
 * @return Whether proprocessing has already been performed to divide the main hydrofabric into existing, individual
 *         sub-hydrofabric files for each partition/process.
 */
bool is_hydrofabric_subdivided(bool printMsg) {
    std::string name = catchmentDataFile + "." + std::to_string(mpi_rank);
    // Initialize isGood based on local state.  Here, local file is "good" when it already exists.
    // TODO: this isn't actually checking whether the files are right (just that they are present) so do we need to?
    bool isGood = utils::FileChecker::file_is_readable(name);
    if (mpiSyncStatusAnd(isGood)) {
        if (printMsg) { std::cout << "Hydrofabric already subdivided in " << mpi_num_procs << " files." << std::endl; }
        return true;
    }
    else {
        if (printMsg) { std::cout << "Hydrofabric has not yet been subdivided." << std::endl; }
        return false;
    }
}

/**
 * Convenience overloaded method for when no supplemental output message is required.
 *
 * @return Whether proprocessing has already been performed to divide the main hydrofabric into existing, individual
 *         sub-hydrofabric files for each partition/process.
 * @see is_hydrofabric_subdivided(bool)
 */
bool is_hydrofabric_subdivided() {
    return is_hydrofabric_subdivided(false);
}

/**
 * Attempt to subdivide the passed hydrofabric files into a series of per-partition files.
 *
 * This function assumes that, when it is called, the intent is for it to produce a freshly subdivided hydrofabric and
 * associated files.  As a result, if there are any other subdivided hydrofabric files present having the same names
 * as the files the function will write, then those preexisting files are considered stale and overwritten.
 *
 * @return Whether subdividing was successful.
 */
bool subdivide_hydrofabric() {
    unsigned short codeBuffer;
    // Track whether things are good, meaning ok to continue and, at the end, whether successful
    // Start with a value of true
    bool isGood = true;

    // For now just have this be responsible for its own rank file
    // Later consider whether it makes more sense for one rank (per host) to write all files
    std::vector<int> indices = {mpi_rank};
    std::unique_ptr<utils::PyHydrofabricSubsetter> subdivider;
    try {
        subdivider = std::make_unique<utils::PyHydrofabricSubsetter>(catchmentDataFile, nexusDataFile, PARTITION_PATH);
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        // Set not good if the subdivider object couldn't be instantiated
        isGood = false;
    }
    // Sync with the rest of the ranks and bail if any aren't ready to proceed for any reason
    if (!mpiSyncStatusAnd(isGood, "initializing hydrofabric subdivider")) {
        return false;
    }

    // Try to perform the subdividing (for now, have each rank handle its own file)
    try {
        isGood = subdivider->execSubdivision(mpi_rank);
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        // Set not good if the subdivider object couldn't be instantiated
        isGood = false;
    }

    // Now sync ranks on whether the subdividing function was executed successfully, and return
    return mpiSyncStatusAnd(isGood, "executing hydrofabric subdivision");
}

#endif // NGEN_MPI_ACTIVE

int main(int argc, char *argv[]) {
    std::cout << "Hello there " << ngen_VERSION_MAJOR << "."
              << ngen_VERSION_MINOR << "."
              << ngen_VERSION_PATCH << std::endl;
    std::ios::sync_with_stdio(false);

    #ifdef ACTIVATE_PYTHON
    // Start Python interpreter and keep it alive
    py::scoped_interpreter guard{};
    #endif // ACTIVATE_PYTHON

    //Pull a few "options" form the cli input, this is a temporary solution to CLI parsing!
    //Use "positional args"
    //arg 0 is program name
    //arg 1 is catchment_data file path
    //arg 2 is catchment subset ids, comma seperated string of ids (no spaces!), "" for all
    //arg 3 is nexus_data file path
    //arg 4 is nexus subset ids, comma seperated string of ids (no spaces!), "" for all
    //arg 5 is realization config path
    //arg 7 is the partition file path
    //arg 8 is an optional flag that driver should, if not already preprocessed this way, subdivided the hydrofabric

    std::vector<string> catchment_subset_ids;
    std::vector<string> nexus_subset_ids;

    if( argc < 6) {
        std::cout << "Missing required args:" << std::endl;
        std::cout << argv[0] << " <catchment_data_path> <catchment subset ids> <nexus_data_path> <nexus subset ids>"
                  << " <realization_config_path>" << std::endl;
        if(argc > 3) {
            std::cout << std::endl << "Note:" << std::endl
                      << "Arguments for <catchment subset ids> and <nexus subset ids> must be given." << std::endl
                      << "Use empty string (\"\") as explicit argument when no subset is needed." << std::endl;
        }
        exit(-1);
    }
    else {
        catchmentDataFile = argv[1];
        nexusDataFile = argv[3];
        REALIZATION_CONFIG_PATH = argv[5];

        #ifdef NGEN_MPI_ACTIVE
        if (argc >= 7) {
            PARTITION_PATH = argv[6];
        }
        else {
            std::cout << "Missing required argument for partition file path." << std::endl;
            exit(-1);
        }

        if (argc >= 8) {
            if (strcmp(argv[7], MPI_HF_SUB_CLI_FLAG) == 0) {
                is_subdivided_hydrofabric_wanted = true;
            }
            else {
                std::cout << "Unexpected arg '" << argv[7] << "'; try " << MPI_HF_SUB_CLI_FLAG << std::endl;
                exit(-1);
            }
        }

        // Initalize MPI
        MPI_Init(NULL, NULL);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_num_procs);
        #endif // NGEN_MPI_ACTIVE

        bool error = !utils::FileChecker::file_is_readable(catchmentDataFile, "Catchment data") ||
                !utils::FileChecker::file_is_readable(nexusDataFile, "Nexus data") ||
                !utils::FileChecker::file_is_readable(REALIZATION_CONFIG_PATH, "Realization config");

        #ifdef NGEN_MPI_ACTIVE
        if (!PARTITION_PATH.empty()) {
            error = error || !utils::FileChecker::file_is_readable(PARTITION_PATH, "Partition config");
        }

        // Do some extra steps if we expect to load a subdivided hydrofabric
        if (is_subdivided_hydrofabric_wanted) {
            // Ensure the hydrofabric is subdivided (either already or by doing it now), and then adjust these paths
            if (is_hydrofabric_subdivided(true) || subdivide_hydrofabric()) {
                catchmentDataFile += "." + std::to_string(mpi_rank);
                nexusDataFile += "." + std::to_string(mpi_rank);
            }
            // If subdivided was needed, subdividing was not already done, and we could not subdivide just now ...
            else {
                std::cout << "Unable to successfully preprocess hydrofabric files into subdivided files per partition.";
                error = true;
            }
        }
        #endif // NGEN_MPI_ACTIVE

        if(error) exit(-1);

        //split the subset strings into vectors
        boost::split(catchment_subset_ids, argv[2], [](char c){return c == ','; } );
        boost::split(nexus_subset_ids, argv[4], [](char c){return c == ','; } );
        //If a single id or no id is passed, the subset vector will have size 1 and be the id or the ""
        //if we get an empy string, pop it from the subset list.
        if(nexus_subset_ids.size() == 1 && nexus_subset_ids[0] == "") nexus_subset_ids.pop_back();
        if(catchment_subset_ids.size() == 1 && catchment_subset_ids[0] == "") catchment_subset_ids.pop_back();
    } // end else if (argc < 6)

    //Read the collection of nexus
    std::cout << "Building Nexus collection" << std::endl;
    
    #ifdef NGEN_MPI_ACTIVE
    Partitions_Parser partition_parser(PARTITION_PATH);
    // TODO: add something here to make sure this step worked for every rank, and maybe to checksum the file
    partition_parser.parse_partition_file();
    
    auto& partitions = partition_parser.partition_ranks;  
    auto& local_data = partitions[std::to_string(mpi_rank)];   
    nexus_subset_ids = local_data.nex_ids;
    catchment_subset_ids = local_data.cat_ids; 
    #endif // NGEN_MPI_ACTIVE

    // TODO: Instead of iterating through a collection of FeatureBase objects mapping to nexi, we instead want to iterate through HY_HydroLocation objects
    geojson::GeoJSON nexus_collection = geojson::read(nexusDataFile, nexus_subset_ids);
    std::cout << "Building Catchment collection" << std::endl;

    // TODO: Instead of iterating through a collection of FeatureBase objects mapping to catchments, we instead want to iterate through HY_Catchment objects
    geojson::GeoJSON catchment_collection = geojson::read(catchmentDataFile, catchment_subset_ids);
    
    for(auto& feature: *catchment_collection)
    {
        //feature->set_id(feature->get_property("ID").as_string());
        nexus_collection->add_feature(feature);
        //std::cout<<"Catchment "<<feature->get_id()<<" -> Nexus "<<feature->get_property("toID").as_string()<<std::endl;
    }

    std::shared_ptr<realization::Formulation_Manager> manager = std::make_shared<realization::Formulation_Manager>(REALIZATION_CONFIG_PATH);
    manager->read(catchment_collection, utils::getStdOut());
    std::string link_key = "toid";
    #ifdef NGEN_MPI_ACTIVE
    nexus_collection->link_features_from_property(nullptr, &link_key);
    hy_features::HY_Features_MPI features = hy_features::HY_Features_MPI(local_data, nexus_collection, manager, mpi_rank, mpi_num_procs);
    #else
    hy_features::HY_Features features = hy_features::HY_Features(catchment_collection, &link_key, manager);
    #endif

    //validate dendridic connections
    features.validate_dendridic();
    //TODO don't really need catchment_collection once catchments are added to nexus collection
    //Still using  catchments for geometry at the moment, fix this later
    //catchment_collection.reset();
    nexus_collection.reset();

    //Still hacking nexus output for the moment
    for(const auto& id : features.nexuses()) {
        #ifdef NGEN_MPI_ACTIVE
        if (!features.is_remote_sender_nexus(id)) {
          nexus_outfiles[id].open("./"+id+"_output.csv", std::ios::trunc);
        }
        #else
        nexus_outfiles[id].open("./"+id+"_output.csv", std::ios::trunc);
        #endif
    }

    std::cout<<"Running Models"<<std::endl;

    std::shared_ptr<pdm03_struct> pdm_et_data = std::make_shared<pdm03_struct>(get_et_params());

    //Now loop some time, iterate catchments, do stuff for total number of output times
    for(int output_time_index = 0; output_time_index < manager->Simulation_Time_Object->get_total_output_times(); output_time_index++) {
      //std::cout<<"Output Time Index: "<<output_time_index<<std::endl;
      if(output_time_index%100 == 0) std::cout<<"Running timestep "<<output_time_index<<std::endl;
      std::string current_timestamp = manager->Simulation_Time_Object->get_timestamp(output_time_index);
      for(const auto& id : features.catchments()) {
        //std::cout<<"Running cat "<<id<<std::endl;
        auto r = features.catchment_at(id);
        //TODO redesign to avoid this cast
        auto r_c = dynamic_pointer_cast<realization::Catchment_Formulation>(r);
        r_c->set_et_params(pdm_et_data);
        double response = r_c->get_response(output_time_index, 3600.0);
        std::string output = std::to_string(output_time_index)+","+current_timestamp+","+
                             r_c->get_output_line_for_timestep(output_time_index)+"\n";
        r_c->write_output(output);
        //TODO put this somewhere else.  For now, just trying to ensure we get m^3/s into nexus output
        response *= (catchment_collection->get_feature(id)->get_property("areasqkm").as_real_number() * 1000000);
        //update the nexus with this flow
        for(auto& nexus : features.destination_nexuses(id)) {
          //TODO in a DENDRIDIC network, only one destination nexus per catchment
          //If there is more than one, some form of catchment partitioning will be required.
          //for now, only contribute to the first one in the list
          nexus->add_upstream_flow(response, id, output_time_index);
	        break;
        }
      } //done catchments
      //At this point, could make an internal routing pass, extracting flows from nexuses and routing
      //across the flowpath to the next nexus.
      //Once everything is updated for this timestep, dump the nexus output
      for(const auto& id : features.nexuses()) {
  #ifdef NGEN_MPI_ACTIVE
        if (!features.is_remote_sender_nexus(id)) { //Ensures only one side of the dual sided remote nexus actually doing this...
  #endif
          //Get the correct "requesting" id for downstream_flow
	        const auto& nexus = features.nexus_at(id);
          const auto& cat_ids = nexus->get_receiving_catchments();
          std::string cat_id;
          if( cat_ids.size() > 0 ) {
            //Assumes dendridic, e.g. only a single downstream...it will consume 100%  of the available flow
            cat_id = cat_ids[0];
          }
          else {
            //This is a terminal node, SHOULDN'T be remote, so ID shouldn't matter too much
            cat_id = "terminal";
          }
          double contribution_at_t = features.nexus_at(id)->get_downstream_flow(cat_id, output_time_index, 100.0);
          if(nexus_outfiles[id].is_open()) {
            nexus_outfiles[id] << output_time_index << ", " << current_timestamp << ", " << contribution_at_t << std::endl;
          }
  #ifdef NGEN_MPI_ACTIVE
        }
  #endif
        //std::cout<<"\tNexus "<<id<<" has "<<contribution_at_t<<" m^3/s"<<std::endl;

        //Note: Use below if developing in-memory transfer of nexus flows to routing
        //If using below, then another single time vector would be needed to hold the timestamp
        //nexus_flows[id].push_back(contribution_at_t); 
      } //done nexuses
    } //done time
    std::cout<<"Finished "<<manager->Simulation_Time_Object->get_total_output_times()<<" timesteps."<<std::endl;


  #ifdef NGEN_ROUTING_ACTIVE
    
    //Syncronization here. MPI barrier. If rank == 0, do routing

    if(manager->get_using_routing()) {
      std::cout<<"Using Routing"<<std::endl;

      std::string t_route_connection_path = manager->get_t_route_connection_path();
      
      std::string input_path = manager->get_input_path();
   
      int number_of_timesteps = manager->Simulation_Time_Object->get_total_output_times();

      int delta_time = manager->Simulation_Time_Object->get_output_interval_seconds();
 
      routing_py_adapter::Routing_Py_Adapter routing_py_adapter1(t_route_connection_path, input_path, catchment_subset_ids, number_of_timesteps, delta_time);
    }
    else {
      std::cout<<"Not Using Routing"<<std::endl;
    }

  #endif // NGEN_ROUTING_ACTIVE

  #ifdef NGEN_MPI_ACTIVE
    MPI_Finalize();
  #endif
}
