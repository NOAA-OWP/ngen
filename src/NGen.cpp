#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <chrono>

#include <boost/core/span.hpp>

#include "realizations/catchment/Formulation_Manager.hpp"
#include <Catchment_Formulation.hpp>
#include <HY_Features.hpp>

#ifdef NGEN_WITH_SQLITE3
#include <geopackage.hpp>
#endif

#include "NGenConfig.h"

#include <FileChecker.h>
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm/sort.hpp>

#ifdef WRITE_PID_FILE_FOR_GDB_SERVER
#include <unistd.h>
#endif // WRITE_PID_FILE_FOR_GDB_SERVER

#ifdef ACTIVATE_PYTHON
#include <pybind11/embed.h>
#include "python/InterpreterUtil.hpp"
#endif // ACTIVATE_PYTHON
    
#ifdef NGEN_ROUTING_ACTIVE
#include "routing/Routing_Py_Adapter.hpp"
#endif // NGEN_ROUTING_ACTIVE

std::string catchmentDataFile = "";
std::string nexusDataFile = "";
std::string REALIZATION_CONFIG_PATH = "";
bool is_subdivided_hydrofabric_wanted = false;

// Define in the non-MPI case so that we don't need to conditionally compile `if (mpi_rank == 0)`
int mpi_rank = 0;

#ifdef NGEN_MPI_ACTIVE

#ifndef MPI_HF_SUB_CLI_FLAG
#define MPI_HF_SUB_CLI_FLAG "--subdivided-hydrofabric"
#endif

#include <mpi.h>
#include "parallel_utils.h"
#include "core/Partition_Parser.hpp"
#include <HY_Features_MPI.hpp>

#include "core/Partition_One.hpp"

std::string PARTITION_PATH = "";
int mpi_num_procs;
#endif // NGEN_MPI_ACTIVE

#include <Layer.hpp>
#include <SurfaceLayer.hpp>

std::unordered_map<std::string, std::ofstream> nexus_outfiles;

void ngen::exec_info::runtime_summary(std::ostream& stream) noexcept
{
    stream << "Runtime configuration summary:\n";

#if NGEN_WITH_MPI
    stream << "  MPI:\n"
           << "    Rank: " << mpi_rank << "\n"
           << "    Processors: " << mpi_num_procs << "\n";
#endif // NGEN_WITH_MPI
  
#if NGEN_WITH_PYTHON // -------------------------------------------------------
    { // START RAII
        py::scoped_interpreter guard{};

        auto sys       = py::module_::import("sys");
        auto sysconfig = py::module_::import("sysconfig");

        // try catch
        py::module_ numpy;
        bool imported_numpy = false;
        std::string err;
        try {
            numpy = py::module_::import("numpy");
            imported_numpy = true;
        } catch(py::error_already_set& e) {
            err = e.what();
        }

        // Lambda to convert py::dict -> std::unordered_map<std::string, std::string>
        const auto dict_to_map = [](const py::dict& dict) -> std::unordered_map<std::string, std::string> {
            std::unordered_map<std::string, std::string> map;
            for (const auto& kv : dict)
                map[kv.first.cast<std::string>()] = kv.second.cast<std::string>();

            return map;
        };

        const auto python_paths = dict_to_map(sysconfig.attr("get_paths")().cast<py::dict>());
        const auto python_venv = std::getenv("VIRTUAL_ENV") == nullptr ? "<none>" : std::getenv("VIRTUAL_ENV");
      
        stream << "  Python:\n"
               << "    Version: "         << sys.attr("version").cast<std::string>() << "\n"
               << "    Virtual Env: "     << python_venv                 << "\n"
               << "    Executable: "      << sys.attr("executable").cast<std::string>() << "\n"
               << "    Site Library: "    << python_paths.at("purelib")  << "\n"
               << "    Include: "         << python_paths.at("include")  << "\n"
               << "    Runtime Library: " << python_paths.at("stdlib")   << "\n";

        if (imported_numpy) {
            stream << "    NumPy Version: "   << numpy.attr("version").attr("version").cast<std::string>() << "\n"
                   << "    NumPy Include: "   << numpy.attr("get_include")().cast<std::string>() << "\n";
        } else {
            // Output NumPy import error
            stream << "    NumPy: " << err << "\n";
        }
               
#if NGEN_WITH_ROUTING

        // TODO: Maybe hash the package sources?
        //
        // In site-packages, the RECORD file for dist contains
        // hashes generated for all files -- maybe parse this and
        // pull a combined hash?

#endif // NGEN_WITH_ROUTING
    } // END RAII
#endif // NGEN_WITH_PYTHON // -------------------------------------------------


} // ngen::exec_info::runtime_summary

int main(int argc, char *argv[]) {

    if (argc > 1 && std::string{argv[1]} == "--info") {
        #if NGEN_WITH_MPI
        MPI_Init(nullptr, nullptr);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_num_procs);
        #endif

        if (mpi_rank == 0)
        {
            std::ostringstream output;
            output << ngen::exec_info::build_summary;
            ngen::exec_info::runtime_summary(output);
            std::cout << output.str() << std::endl;
        } // if (mpi_rank == 0)

        #if NGEN_WITH_MPI
        MPI_Finalize();
        #endif

        exit(1);
    } 

    auto time_start = std::chrono::steady_clock::now();

    std::cout << "NGen Framework " << ngen_VERSION_MAJOR << "."
              << ngen_VERSION_MINOR << "."
              << ngen_VERSION_PATCH << std::endl;
    std::ios::sync_with_stdio(false);

    #ifdef ACTIVATE_PYTHON
    // Start Python interpreter via the manager singleton
    // Need to bind to a variable so that the underlying reference count
    // is incremented, this essentially becomes the global reference to keep
    // the interpreter alive till the end of `main`
    auto _interp = utils::ngenPy::InterpreterUtil::getInstance();
    //utils::ngenPy::InterpreterUtil::getInstance();
    #endif // ACTIVATE_PYTHON

    //Pull a few "options" form the cli input, this is a temporary solution to CLI parsing!
    //Use "positional args"
    //arg 0 is program name
    //arg 1 is catchment_data file path
    //arg 2 is catchment subset ids, comma seperated string of ids (no spaces!), "all" for all
    //arg 3 is nexus_data file path
    //arg 4 is nexus subset ids, comma seperated string of ids (no spaces!), "all" for all
    //arg 5 is realization config path
    //arg 7 is the partition file path
    //arg 8 is an optional flag that driver should, if not already preprocessed this way, subdivided the hydrofabric

    std::vector<std::string> catchment_subset_ids;
    std::vector<std::string> nexus_subset_ids;

    if( argc < 2) {
        // Usage
        std::cout << "Usage: " << std::endl;
        std::cout << argv[0] << " <catchment_data_path> <catchment subset ids> <nexus_data_path> <nexus subset ids>"
                  << " <realization_config_path>" << std::endl
                  << "Arguments for <catchment subset ids> and <nexus subset ids> must be given." << std::endl
                  << "Use \"all\" as explicit argument when no subset is needed." << std::endl;

        // Build and environment information
        std::cout<<std::endl<<"Build Info:"<<std::endl;
        std::cout<<"  NGen version: " // This is here mainly so that there will be *some* output if somehow no other options are enabled.
          << ngen_VERSION_MAJOR << "."
          << ngen_VERSION_MINOR << "."
          << ngen_VERSION_PATCH << std::endl;
        #ifdef NGEN_MPI_ACTIVE
        std::cout<<"  Parallel build"<<std::endl;
        #endif
        #ifdef NETCDF_ACTIVE
        std::cout<<"  NetCDF lumped forcing enabled"<<std::endl;
        #endif
        #ifdef NGEN_BMI_FORTRAN_ACTIVE
        std::cout<<"  Fortran BMI enabled"<<std::endl;
        #endif
        #ifdef NGEN_C_LIB_ACTIVE
        std::cout<<"  C BMI enabled"<<std::endl;
        #endif
        #ifdef ACTIVATE_PYTHON
        std::cout<<"  Python active"<<std::endl;
        std::cout<<"    Embedded interpreter version: "<<PY_MAJOR_VERSION<<"."<<PY_MINOR_VERSION<<"."<<PY_MICRO_VERSION<<std::endl;
        #endif
        #ifdef NGEN_ROUTING_ACTIVE
        std::cout<<"  Routing active"<<std::endl;
        #endif
        #ifdef ACTIVATE_PYTHON
        std::cout<<std::endl<<"Python Environment Info:"<<std::endl;
        std::cout<<"  VIRTUAL_ENV environment variable: "<<(std::getenv("VIRTUAL_ENV") == nullptr ? "(not set)" : std::getenv("VIRTUAL_ENV"))<<std::endl;
        std::cout<<"  Discovered venv: "<<_interp->getDiscoveredVenvPath()<<std::endl;
        auto paths = _interp->getSystemPath();
        std::cout<<"  System paths:"<<std::endl;
        for(std::string& path: std::get<1>(paths)){
          std::cout<<"    "<<path<<std::endl;
        }
        #endif
        std::cout<<std::endl;
        exit(0); // Unsure if this path should have a non-zero exit code?
    } else if( argc < 6) {
        std::cout << "Missing required args:" << std::endl;
        std::cout << argv[0] << " <catchment_data_path> <catchment subset ids> <nexus_data_path> <nexus subset ids>"
                  << " <realization_config_path>" << std::endl;
        if(argc > 3) {
            std::cout << std::endl << "Note:" << std::endl
                      << "Arguments for <catchment subset ids> and <nexus subset ids> must be given." << std::endl
                      << "Use \"all\" as explicit argument when no subset is needed." << std::endl;
        }

        exit(-1);
    }
    else {
        catchmentDataFile = argv[1];
        nexusDataFile = argv[3];
        REALIZATION_CONFIG_PATH = argv[5];

        #ifdef NGEN_MPI_ACTIVE

        // Initalize MPI
        MPI_Init(NULL, NULL);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_num_procs);

        if (argc >= 7) {
            PARTITION_PATH = argv[6];
        }
        else if (mpi_num_procs > 1) {
            std::cout << "Missing required argument for partition file path." << std::endl;
            exit(-1);
        }

        if (argc >= 8) {
            if (strcmp(argv[7], MPI_HF_SUB_CLI_FLAG) == 0) {
                is_subdivided_hydrofabric_wanted = true;
            }
            else if (mpi_num_procs > 1) {
                std::cout << "Unexpected arg '" << argv[7] << "'; try " << MPI_HF_SUB_CLI_FLAG << std::endl;
                exit(-1);
            }
        }
        #endif // NGEN_MPI_ACTIVE

        #ifdef WRITE_PID_FILE_FOR_GDB_SERVER
        std::string pid_file_name = "./.ngen_pid";
        #ifdef NGEN_MPI_ACTIVE
        pid_file_name += "." + std::to_string(mpi_rank);
        #endif // NGEN_MPI_ACTIVE
        ofstream outfile;
        outfile.open(pid_file_name, ios::out | ios::trunc );
        outfile << getpid();
        outfile.close();
        int total_time = 0;
        while (utils::FileChecker::file_is_readable(pid_file_name) && total_time < 180) {
            total_time += 30;
            sleep(30);
        }
        #endif // WRITE_PID_FILE_FOR_GDB_SERVER

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
            if (parallel::is_hydrofabric_subdivided(mpi_rank, mpi_num_procs, true) ||
                parallel::subdivide_hydrofabric(mpi_rank, mpi_num_procs, catchmentDataFile, nexusDataFile,
                                                PARTITION_PATH))
            {
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
        if( catchment_subset_ids.size() == 1 && catchment_subset_ids[0] == "all")
          catchment_subset_ids.pop_back();
        boost::split(nexus_subset_ids, argv[4], [](char c){return c == ','; } );
        if( nexus_subset_ids.size() == 1 && nexus_subset_ids[0] == "all")
          nexus_subset_ids.pop_back();
        //If a single id or no id is passed, the subset vector will have size 1 and be the id or the ""
        //if we get an empy string, pop it from the subset list.
        if(nexus_subset_ids.size() == 1 && nexus_subset_ids[0] == "") nexus_subset_ids.pop_back();
        if(catchment_subset_ids.size() == 1 && catchment_subset_ids[0] == "") catchment_subset_ids.pop_back();
    } // end else if (argc < 6)

    //Read the collection of nexus
    std::cout << "Building Nexus collection" << std::endl;
    
    #ifdef NGEN_MPI_ACTIVE
    PartitionData local_data;
    if (mpi_num_procs > 1) {
        Partitions_Parser partition_parser(PARTITION_PATH);
        // TODO: add something here to make sure this step worked for every rank, and maybe to checksum the file
        partition_parser.parse_partition_file();

        std::vector<PartitionData> &partitions = partition_parser.partition_ranks;
        local_data = std::move(partitions[mpi_rank]);
        if (!nexus_subset_ids.empty()) {
            std::cerr << "Warning: CLI provided nexus subset will be ignored when using partition config";
        }
        if (!catchment_subset_ids.empty()) {
            std::cerr << "Warning: CLI provided catchment subset will be ignored when using partition config";
        }
        nexus_subset_ids = std::vector<std::string>(local_data.nexus_ids.begin(), local_data.nexus_ids.end());
        catchment_subset_ids = std::vector<std::string>(local_data.catchment_ids.begin(), local_data.catchment_ids.end());
    }
    #endif // NGEN_MPI_ACTIVE

    // TODO: Instead of iterating through a collection of FeatureBase objects mapping to nexi, we instead want to iterate through HY_HydroLocation objects
    geojson::GeoJSON nexus_collection;
    if (boost::algorithm::ends_with(nexusDataFile, "gpkg")) {
      #ifdef NGEN_WITH_SQLITE3
      nexus_collection = ngen::geopackage::read(nexusDataFile, "nexus", nexus_subset_ids);
      #else
      throw std::runtime_error("SQLite3 support required to read GeoPackage files.");
      #endif
    } else {
      nexus_collection = geojson::read(nexusDataFile, nexus_subset_ids);
    }
    std::cout << "Building Catchment collection" << std::endl;

    // TODO: Instead of iterating through a collection of FeatureBase objects mapping to catchments, we instead want to iterate through HY_Catchment objects
    geojson::GeoJSON catchment_collection;
    if (boost::algorithm::ends_with(catchmentDataFile, "gpkg")) {
      #ifdef NGEN_WITH_SQLITE3
      catchment_collection = ngen::geopackage::read(catchmentDataFile, "divides", catchment_subset_ids);
      #else
      throw std::runtime_error("SQLite3 support required to read GeoPackage files.");
      #endif
    } else {
      catchment_collection = geojson::read(catchmentDataFile, catchment_subset_ids);
    }
    
    for(auto& feature: *catchment_collection)
    {
        //feature->set_id(feature->get_property("id").as_string());
        nexus_collection->add_feature(feature);
        //std::cout<<"Catchment "<<feature->get_id()<<" -> Nexus "<<feature->get_property("toid").as_string()<<std::endl;
    }
    //Update the feature ids for the combined collection, using the alternative property 'id'
    //to map features to their primary id as well as the alternative property
    nexus_collection->update_ids("id");
    std::cout<<"Initializing formulations" << std::endl;
    std::shared_ptr<realization::Formulation_Manager> manager = std::make_shared<realization::Formulation_Manager>(REALIZATION_CONFIG_PATH);
    manager->read(catchment_collection, utils::getStdOut());

    //TODO refactor manager->read so certain configs can be queried before the entire
    //realization collection is created
    #ifdef NGEN_ROUTING_ACTIVE
    std::unique_ptr<routing_py_adapter::Routing_Py_Adapter> router;
    if( mpi_rank == 0 )
    { // Run t-route from single process
    if(manager->get_using_routing()) {
      std::cout<<"Using Routing"<<std::endl;
      std::string t_route_config_file_with_path = manager->get_t_route_config_file_with_path();
      router = std::make_unique<routing_py_adapter::Routing_Py_Adapter>(t_route_config_file_with_path);
    }
    else {
      std::cout<<"Not Using Routing"<<std::endl;
    }
    }
    #endif //NGEN_ROUTING_ACTIVE
    std::cout<<"Building Feature Index" <<std::endl;;
    std::string link_key = "toid";
    nexus_collection->link_features_from_property(nullptr, &link_key);

    #ifdef NGEN_MPI_ACTIVE
    //mpirun with one processor without partition file
    if (mpi_num_procs == 1) {
        Partition_One partition_one;
        partition_one.generate_partition(catchment_collection);
        local_data = std::move(partition_one.partition_data);
    }
    hy_features::HY_Features_MPI features = hy_features::HY_Features_MPI(local_data, nexus_collection, manager, mpi_rank, mpi_num_procs);
    #else
    hy_features::HY_Features features = hy_features::HY_Features(nexus_collection, manager);
    #endif

    //validate dendritic connections
    features.validate_dendritic();
    //TODO don't really need catchment_collection once catchments are added to nexus collection
    //Still using  catchments for geometry at the moment, fix this later
    //catchment_collection.reset();
    nexus_collection.reset();

    //Still hacking nexus output for the moment
    for(const auto& id : features.nexuses()) {
        #ifdef NGEN_MPI_ACTIVE
        if (mpi_num_procs > 1) {
            if (!features.is_remote_sender_nexus(id)) {
                nexus_outfiles[id].open(manager->get_output_root() + id + "_output.csv", std::ios::trunc);
            }
        } else {
          nexus_outfiles[id].open(manager->get_output_root() + id + "_output.csv", std::ios::trunc);
        }
        #else
        nexus_outfiles[id].open(manager->get_output_root() + id + "_output.csv", std::ios::trunc);
        #endif
    }

    std::cout<<"Running Models"<<std::endl;

    // check the time loops for the existing layers
    ngen::LayerDataStorage& layer_meta_data = manager->get_layer_metadata();

    // get the keys for the existing layers
    std::vector<int>& keys = layer_meta_data.get_keys();

    // get the converted time steps for layers
    double min_time_step;
    double max_time_step;
    std::vector<double> time_steps;
    for(int i = 0; i < keys.size(); ++i)
    {
        auto& m_data = layer_meta_data.get_layer(keys[i]);
        double c_value = UnitsHelper::get_converted_value(m_data.time_step_units,m_data.time_step,"s");
        time_steps.push_back(c_value);
    }

    std::vector<int> output_time_index;
    output_time_index.resize(keys.size());
    std::fill(output_time_index.begin(), output_time_index.end(),0);

    // now create the layer objects

    // first make sure that the layer are listed in decreasing order
    boost::range::sort(keys, std::greater<int>());

    std::vector<std::shared_ptr<ngen::Layer> > layers;
    layers.resize(keys.size());

    for(long i = 0; i < keys.size(); ++i)
    {
      auto& desc = layer_meta_data.get_layer(keys[i]);
      std::vector<std::string> cat_ids;

      // make a new simulation time object with a different output interval
      Simulation_Time sim_time(*manager->Simulation_Time_Object, time_steps[i]);
  
      for ( std::string id : features.catchments(keys[i]) ) { cat_ids.push_back(id); }
      if (keys[i] != 0 )
      {
        layers[i] = std::make_shared<ngen::Layer>(desc, cat_ids, sim_time, features, catchment_collection, 0);
      }
      else
      {
        layers[i] = std::make_shared<ngen::SurfaceLayer>(desc, cat_ids, sim_time, features, catchment_collection, 0, nexus_subset_ids, nexus_outfiles);
      }

    }

    auto time_done_init = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_elapsed_init = time_done_init - time_start;

    //Now loop some time, iterate catchments, do stuff for total number of output times
    auto num_times = manager->Simulation_Time_Object->get_total_output_times();
    for( int count = 0; count < num_times; count++) 
    {
      // The Inner loop will advance all layers unless doing so will break one of two constraints
      // 1) A layer may not proceed ahead of the master simulation object's current time
      // 2) A layer may not proceed ahead of any layer that is computed before it
      // The do while loop ensures that all layers are tested at least once while allowing 
      // layers with small time steps to be updated more than once
      // If a layer with a large time step is after a layer with a small time step the
      // layer with the large time step will wait for multiple timesteps from the preceeding
      // layer.
      
      // this is the time that layers are trying to reach (or get as close as possible)
      auto next_time = manager->Simulation_Time_Object->next_timestep_epoch_time();

      // this is the time that the layer above the current layer is at
      auto prev_layer_time = next_time;

      // this is the time that the least advanced layer is at
      auto layer_min_next_time = next_time;
      do
      {
        for ( auto& layer : layers ) 
        {
          auto layer_next_time = layer->next_timestep_epoch_time();

          // only advance if you would not pass the master next time and the previous layer next time
          if ( layer_next_time <= next_time && layer_next_time <=  prev_layer_time)
          {
            layer->update_models();
            prev_layer_time = layer_next_time;
          }
          else
          {
            layer_min_next_time = prev_layer_time = layer->current_timestep_epoch_time(); 
          }

          if ( layer_min_next_time > layer_next_time)
          {
            layer_min_next_time = layer_next_time;
          }
        } //done layers
      } while( layer_min_next_time < next_time );  // rerun the loop until the last layer would pass the master next time

      if (count + 1 < num_times)
      {
        manager->Simulation_Time_Object->advance_timestep();
      }

    } //done time

#if NGEN_MPI_ACTIVE
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    if (mpi_rank == 0)
    {
        std::cout << "Finished " << manager->Simulation_Time_Object->get_total_output_times() << " timesteps." << std::endl;
    }

    auto time_done_simulation = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_elapsed_simulation = time_done_simulation - time_done_init;

#ifdef NGEN_MPI_ACTIVE
    MPI_Barrier(MPI_COMM_WORLD);
#endif

#ifdef NGEN_ROUTING_ACTIVE
    if (mpi_rank == 0)
    { // Run t-route from single process
        if(manager->get_using_routing()) {
          //Note: Currently, delta_time is set in the t-route yaml configuration file, and the
          //number_of_timesteps is determined from the total number of nexus outputs in t-route.
          //It is recommended to still pass these values to the routing_py_adapter object in
          //case a future implmentation needs these two values from the ngen framework.
          int number_of_timesteps = manager->Simulation_Time_Object->get_total_output_times();

          int delta_time = manager->Simulation_Time_Object->get_output_interval_seconds();
          
          router->route(number_of_timesteps, delta_time); 
        }
    }
#endif

    auto time_done_routing = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_elapsed_routing = time_done_routing - time_done_simulation;

    if (mpi_rank == 0)
    {
        std::cout << "NGen top-level timings:"
                  << "\n\tNGen::init: " << time_elapsed_init.count()
                  << "\n\tNGen::simulation: " << time_elapsed_simulation.count()
#ifdef NGEN_ROUTING_ACTIVE
                  << "\n\tNGen::routing: " << time_elapsed_routing.count()
#endif
                  << std::endl;
    }

#ifdef NGEN_MPI_ACTIVE
    MPI_Finalize();
#endif

    return 0;
}
