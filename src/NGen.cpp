#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

#include <boost/core/span.hpp>

#include "realizations/catchment/Formulation_Manager.hpp"
#include <Catchment_Formulation.hpp>
#include <HY_Features.hpp>

#if NGEN_WITH_SQLITE3
#include <geopackage.hpp>
#endif

#include "NGenConfig.h"

#include <Logger.hpp>

#include <FileChecker.h>
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <NgenSimulation.hpp>

#ifdef WRITE_PID_FILE_FOR_GDB_SERVER
#include <unistd.h>
#endif // WRITE_PID_FILE_FOR_GDB_SERVER

#if NGEN_WITH_PYTHON
#include "python/InterpreterUtil.hpp"
#include <pybind11/embed.h>
#endif // NGEN_WITH_PYTHON

#if NGEN_WITH_MPI

#ifndef MPI_HF_SUB_CLI_FLAG
#define MPI_HF_SUB_CLI_FLAG "--subdivided-hydrofabric"
#endif

#include "core/Partition_Parser.hpp"
#include "parallel_utils.h"
#include <HY_Features_MPI.hpp>
#include <mpi.h>
#include <algorithm>

#include "core/Partition_One.hpp"

#endif // NGEN_WITH_MPI

#include <DomainLayer.hpp>
#include <Layer.hpp>
#include <SurfaceLayer.hpp>

void ngen::exec_info::runtime_summary(std::ostream& stream) noexcept {
    stream << "Runtime configuration summary:\n";

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
            numpy          = py::module_::import("numpy");
            imported_numpy = true;
        } catch (py::error_already_set& e) {
            err = e.what();
        }

        // Lambda to convert py::dict -> std::unordered_map<std::string, std::string>
        const auto dict_to_map = [](const py::dict& dict
                                 ) -> std::unordered_map<std::string, std::string> {
            std::unordered_map<std::string, std::string> map;
            for (const auto& kv : dict)
                map[kv.first.cast<std::string>()] = kv.second.cast<std::string>();

            return map;
        };

        const auto python_paths = dict_to_map(sysconfig.attr("get_paths")().cast<py::dict>());
        const auto python_venv =
            std::getenv("VIRTUAL_ENV") == nullptr ? "<none>" : std::getenv("VIRTUAL_ENV");

        stream << "  Python:\n"
               << "    Version: " << sys.attr("version").cast<std::string>() << "\n"
               << "    Virtual Env: " << python_venv << "\n"
               << "    Executable: " << sys.attr("executable").cast<std::string>() << "\n"
               << "    Site Library: " << python_paths.at("purelib") << "\n"
               << "    Include: " << python_paths.at("include") << "\n"
               << "    Runtime Library: " << python_paths.at("stdlib") << "\n";

        if (imported_numpy) {
            stream << "    NumPy Version: "
                   << numpy.attr("version").attr("version").cast<std::string>() << "\n"
                   << "    NumPy Include: " << numpy.attr("get_include")().cast<std::string>()
                   << "\n";
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

void write_nexus_outflow_csv_files(std::string const& output_root,
                                   std::unique_ptr<NgenSimulation> const& simulation,
                                   NgenSimulation::hy_features_t const& features)
{
    std::stringstream ss;

    auto num_times = simulation->get_num_output_times();

    for (const auto& id : features.nexuses()) {
        ss << "Preparing to write nexus outflow file for nexus '" << id << "'";
        LOG(ss.str(), LogLevel::DEBUG);
        ss.str("");

        Simulation_Time time = *simulation->sim_time_;

        std::string filename = output_root + id + "_output.csv";
        std::ofstream nexus_outfile(filename, std::ios::trunc);
        if (nexus_outfile.fail()) {
            // Log error, and try the next one
            ss << "Failed to open nexus outflow file for nexus '" << id << "', filename '" << filename << "'\n";
            LOG(ss.str(), LogLevel::SEVERE);
            ss.str("");
            continue;
        }

        auto nexus_index = simulation->get_nexus_index(id);
        for (int i = 0; i < num_times; ++i) {
            nexus_outfile << i << ", " << time.get_timestamp(i) << ", " << simulation->get_nexus_outflow(nexus_index, i) << "\n";

            if (nexus_outfile.fail()) {
                // Log error and move on
                ss << "Failed to write nexus outflow file for nexus '" << id << "', filename '" << filename << "'\n";
                LOG(ss.str(), LogLevel::SEVERE);
                ss.str("");
                break;
            }
        }

        nexus_outfile.close();
        if (nexus_outfile.fail()) {
            ss << "Failure reported while closing nexus outflow file for nexus '" << id << "', filename '" << filename << "'\n";
            LOG(ss.str(), LogLevel::SEVERE);
            ss.str("");
        }
    }
}

int main(int argc, char* argv[]) {
    std::string catchmentDataFile         = "";
    std::string nexusDataFile             = "";
    std::string REALIZATION_CONFIG_PATH   = "";
    bool is_subdivided_hydrofabric_wanted = false;
    std::string PARTITION_PATH = "";
    std::stringstream ss("");

     // This default value should lead to behavior matching the single-process case in the standalone or non-MPI case
    int mpi_num_procs = 1;
    // Define in the non-MPI case so that we don't need to conditionally compile `if (mpi_rank == 0)`
    int mpi_rank = 0;

    if (argc > 1 && std::string{argv[1]} == "--info") {
#if NGEN_WITH_MPI
        MPI_Init(nullptr, nullptr);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_num_procs);
#endif

        if (mpi_rank == 0) {
            std::ostringstream output;
            output << ngen::exec_info::build_summary;
            ngen::exec_info::runtime_summary(output);
#if NGEN_WITH_MPI
            output << "  MPI:\n"
                   << "    Rank: " << mpi_rank << "\n"
                   << "    Processors: " << mpi_num_procs << "\n";
#endif // NGEN_WITH_MPI

            ss << output.str() << std::endl;
            LOG(ss.str(), LogLevel::INFO);
            ss.str("");
        } // if (mpi_rank == 0)

#if NGEN_WITH_MPI
        MPI_Finalize();
#endif

        exit(1);
    }

    auto time_start = std::chrono::steady_clock::now();

    ss << "NGen Framework " << ngen_VERSION_MAJOR << "." << ngen_VERSION_MINOR << "."
       << ngen_VERSION_PATCH << std::endl;
    LOG(ss.str(), LogLevel::INFO);
    ss.str("");
    std::ios::sync_with_stdio(false);

#if NGEN_WITH_PYTHON
    // Start Python interpreter via the manager singleton
    // Need to bind to a variable so that the underlying reference count
    // is incremented, this essentially becomes the global reference to keep
    // the interpreter alive till the end of `main`
    auto _interp = utils::ngenPy::InterpreterUtil::getInstance();
// utils::ngenPy::InterpreterUtil::getInstance();
#endif // NGEN_WITH_PYTHON

    // Pull a few "options" form the cli input, this is a temporary solution to CLI parsing!
    // Use "positional args"
    // arg 0 is program name
    // arg 1 is catchment_data file path
    // arg 2 is catchment subset ids, comma seperated string of ids (no spaces!), "all" for all
    // arg 3 is nexus_data file path
    // arg 4 is nexus subset ids, comma seperated string of ids (no spaces!), "all" for all
    // arg 5 is realization config path
    // arg 7 is the partition file path
    // arg 8 is an optional flag that driver should, if not already preprocessed this way,
    // subdivided the hydrofabric

    std::vector<std::string> catchment_subset_ids;
    std::vector<std::string> nexus_subset_ids;

    if (argc < 2) {
        // Usage
        ss << "Usage: " << std::endl;
        ss << "  " << argv[0]
           << " <catchment_data_path> <catchment subset ids> <nexus_data_path> <nexus subset ids> "
              "<realization_config_path>"
           << std::endl;
        ss << "  " << "Arguments for <catchment subset ids> and <nexus subset ids> must be given."
           << std::endl;
        ss << "  " << "Use \"all\" as explicit argument when no subset is needed." << std::endl;

        // Build and environment information
        ss << "Build Info:" << std::endl;
        ss << "  NGen version: " // This is here mainly so that there will be *some* output if
                                 // somehow no other options are enabled.
           << ngen_VERSION_MAJOR << "." << ngen_VERSION_MINOR << "." << ngen_VERSION_PATCH
           << std::endl;
#if NGEN_WITH_MPI
        ss << "  Parallel build" << std::endl;
#endif
#if NGEN_WITH_NETCDF
        ss << "  NetCDF lumped forcing enabled" << std::endl;
#endif
#if NGEN_WITH_BMI_FORTRAN
        ss << "  Fortran BMI enabled" << std::endl;
#endif
#if NGEN_WITH_BMI_C
        ss << "  C BMI enabled" << std::endl;
#endif
#if NGEN_WITH_PYTHON
        ss << "  Python active" << std::endl;
        ss << "    Embedded interpreter version: " << PY_MAJOR_VERSION << "." << PY_MINOR_VERSION
           << "." << PY_MICRO_VERSION << std::endl;
#endif
#if NGEN_WITH_ROUTING
        ss << "  Routing active" << std::endl;
#endif
#if NGEN_WITH_PYTHON
        ss << "Python Environment Info:" << std::endl;
        ss << "  VIRTUAL_ENV environment variable: "
           << (std::getenv("VIRTUAL_ENV") == nullptr ? "(not set)" : std::getenv("VIRTUAL_ENV"))
           << std::endl;
        ss << "  Discovered venv: " << _interp->getDiscoveredVenvPath() << std::endl;
        auto paths = _interp->getSystemPath();
        ss << "  System paths:" << std::endl;
        for (std::string& path : std::get<1>(paths)) {
            if (!path.empty()) {
                ss << "    " << path << std::endl;
            }
        }
#endif
        // Put usage in log and send to stdout
        std::cout << ss.str() << std::endl;
        LOG(ss.str(), LogLevel::INFO);
        ss.str("");
        exit(0); // Unsure if this path should have a non-zero exit code?

    } else if (argc < 6) {
        ss << "Missing required args:" << std::endl;
        ss << argv[0]
           << " <catchment_data_path> <catchment subset ids> <nexus_data_path> <nexus subset ids>"
           << " <realization_config_path>" << std::endl;
        LOG(ss.str(), LogLevel::WARNING);
        ss.str("");
        if (argc > 3) {
            ss << std::endl
               << "Note:" << std::endl
               << "Arguments for <catchment subset ids> and <nexus subset ids> must be given."
               << std::endl
               << "Use \"all\" as explicit argument when no subset is needed." << std::endl;
            LOG(ss.str(), LogLevel::SEVERE);
            ss.str("");
        }

        exit(-1);
    } else {
        catchmentDataFile       = argv[1];
        nexusDataFile           = argv[3];
        REALIZATION_CONFIG_PATH = argv[5];

#if NGEN_WITH_MPI

        // Initalize MPI
        MPI_Init(NULL, NULL);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_num_procs);

        if (argc >= 7) {
            LOG("argc >= 7", LogLevel::INFO);
            ss.str("");
            PARTITION_PATH = argv[6];
        } else if (mpi_num_procs > 1) {
            ss << "Missing required argument for partition file path." << std::endl;
            LOG(ss.str(), LogLevel::WARNING);
            ss.str("");
            exit(-1);
        }

        if (argc >= 8) {
            LOG("argc >= 8", LogLevel::INFO);
            ss.str("");
            if (strcmp(argv[7], MPI_HF_SUB_CLI_FLAG) == 0) {
                is_subdivided_hydrofabric_wanted = true;
            } else if (mpi_num_procs > 1) {
                ss << "Unexpected arg '" << argv[7] << "'; try " << MPI_HF_SUB_CLI_FLAG
                   << std::endl;
                LOG(ss.str(), LogLevel::WARNING);
                ss.str("");
                exit(-1);
            }
        }
#endif // NGEN_WITH_MPI

#ifdef WRITE_PID_FILE_FOR_GDB_SERVER
        std::string pid_file_name = "./.ngen_pid";
#if NGEN_WITH_MPI
        pid_file_name += "." + std::to_string(mpi_rank);
#endif // NGEN_WITH_MPI
        ofstream outfile;
        outfile.open(pid_file_name, ios::out | ios::trunc);
        outfile << getpid();
        outfile.close();
        int total_time = 0;
        while (utils::FileChecker::file_is_readable(pid_file_name) && total_time < 180) {
            total_time += 30;
            sleep(30);
        }
#endif // WRITE_PID_FILE_FOR_GDB_SERVER

        bool error =
            !utils::FileChecker::file_is_readable(catchmentDataFile, "Catchment data") ||
            !utils::FileChecker::file_is_readable(nexusDataFile, "Nexus data") ||
            !utils::FileChecker::file_is_readable(REALIZATION_CONFIG_PATH, "Realization config");

#if NGEN_WITH_MPI
        if (!PARTITION_PATH.empty()) {
            error =
                error || !utils::FileChecker::file_is_readable(PARTITION_PATH, "Partition config");
        }

        // Do some extra steps if we expect to load a subdivided hydrofabric
        if (is_subdivided_hydrofabric_wanted) {
            // Ensure the hydrofabric is subdivided (either already or by doing it now), and then
            // adjust these paths
            if (parallel::is_hydrofabric_subdivided(catchmentDataFile, mpi_rank, mpi_num_procs, true) ||
                parallel::subdivide_hydrofabric(
                    mpi_rank,
                    mpi_num_procs,
                    catchmentDataFile,
                    nexusDataFile,
                    PARTITION_PATH
                )) {
                catchmentDataFile += "." + std::to_string(mpi_rank);
                nexusDataFile += "." + std::to_string(mpi_rank);
            }
            // If subdivided was needed, subdividing was not already done, and we could not
            // subdivide just now ...
            else {
                ss << "Unable to successfully preprocess hydrofabric files into subdivided files "
                      "per partition.";
                LOG(ss.str(), LogLevel::WARNING);
                ss.str("");
                error = true;
            }
        }
#endif // NGEN_WITH_MPI

        if (error)
            exit(-1);

        // split the subset strings into vectors
        boost::split(catchment_subset_ids, argv[2], [](char c) { return c == ','; });
        if (catchment_subset_ids.size() == 1 && catchment_subset_ids[0] == "all")
            catchment_subset_ids.pop_back();
        boost::split(nexus_subset_ids, argv[4], [](char c) { return c == ','; });
        if (nexus_subset_ids.size() == 1 && nexus_subset_ids[0] == "all")
            nexus_subset_ids.pop_back();
        // If a single id or no id is passed, the subset vector will have size 1 and be the id or
        // the "" if we get an empy string, pop it from the subset list.
        if (nexus_subset_ids.size() == 1 && nexus_subset_ids[0] == "")
            nexus_subset_ids.pop_back();
        if (catchment_subset_ids.size() == 1 && catchment_subset_ids[0] == "")
            catchment_subset_ids.pop_back();
    } // end else if (argc < 6)

    // Read the collection of nexus
    ss << "Building Nexus collection" << std::endl;
    LOG(ss.str(), LogLevel::INFO);
    ss.str("");

#if NGEN_WITH_MPI
    PartitionData local_data;
    if (mpi_num_procs > 1) {
        Partitions_Parser partition_parser(PARTITION_PATH);
        // TODO: add something here to make sure this step worked for every rank, and maybe to
        // checksum the file
        partition_parser.parse_partition_file();

        std::vector<PartitionData>& partitions = partition_parser.partition_ranks;
        local_data                             = std::move(partitions[mpi_rank]);
        if (!nexus_subset_ids.empty()) {
            ss << "Warning: CLI provided nexus subset will be ignored when using partition config";
            LOG(ss.str(), LogLevel::WARNING);
            ss.str("");
        }
        if (!catchment_subset_ids.empty()) {
            ss << "Warning: CLI provided catchment subset will be ignored when using partition "
                  "config";
            LOG(ss.str(), LogLevel::WARNING);
            ss.str("");
        }
        nexus_subset_ids =
            std::vector<std::string>(local_data.nexus_ids.begin(), local_data.nexus_ids.end());
        catchment_subset_ids = std::vector<std::string>(
            local_data.catchment_ids.begin(),
            local_data.catchment_ids.end()
        );
    }
#endif // NGEN_WITH_MPI

    // TODO: Instead of iterating through a collection of FeatureBase objects mapping to nexi, we
    // instead want to iterate through HY_HydroLocation objects
    geojson::GeoJSON nexus_collection;
    if (boost::algorithm::ends_with(nexusDataFile, "gpkg")) {
#if NGEN_WITH_SQLITE3
        try {
            nexus_collection = ngen::geopackage::read(nexusDataFile, "nexus", nexus_subset_ids);
        } catch (...) {
            // Handle all exceptions
            std::string msg = "Geopackage error occurred reading nexuses: " + nexusDataFile;
            LOG(msg,LogLevel::FATAL);
            throw std::runtime_error(msg);
        }
#else
        Logger::logMsgAndThrowError("SQLite3 support required to read GeoPackage files.");
#endif
    } else {
        nexus_collection = geojson::read(nexusDataFile, nexus_subset_ids);
    }
    ss << "Building Catchment collection" << std::endl;
    LOG(ss.str(), LogLevel::INFO);
    ss.str("");

    // TODO: Instead of iterating through a collection of FeatureBase objects mapping to catchments,
    // we instead want to iterate through HY_Catchment objects
    geojson::GeoJSON catchment_collection;
    // As part of the fix for NOAA-OWP/ngen#284 / NGWPC-6553,
    // partitioning may insert sentinel flowpaths downstream of
    // terminal nexuses. Those sentinels will not exist in the
    // catchmentDataFile. Their listing in catchment_subset_ids works
    // because the respective geoFOO::read() functions return the
    // intersection of features in the file and the specified subset,
    // rather than erroring on missing features.
    if (boost::algorithm::ends_with(catchmentDataFile, "gpkg")) {
#if NGEN_WITH_SQLITE3
        try {
        catchment_collection =
            ngen::geopackage::read(catchmentDataFile, "divides", catchment_subset_ids);
        } catch (...) {
            // Handle all exceptions
            std::string msg = "Geopackage error occurred reading divides: " + catchmentDataFile;
            LOG(msg,LogLevel::FATAL);
            throw std::runtime_error(msg);
        }

#else
        Logger::logMsgAndThrowError("SQLite3 support required to read GeoPackage files.");
#endif
    } else {
        catchment_collection = geojson::read(catchmentDataFile, catchment_subset_ids);
    }

    for (auto& feature : *catchment_collection) {
        // feature->set_id(feature->get_property("id").as_string());
        nexus_collection->add_feature(feature);
        // ss<<"Catchment "<<feature->get_id()<<" -> Nexus
        // "<<feature->get_property("toid").as_string()<<std::endl;
    }
    // Update the feature ids for the combined collection, using the alternative property 'id'
    // to map features to their primary id as well as the alternative property
    nexus_collection->update_ids("id");
    ss << "Initializing formulations" << std::endl;
    LOG(ss.str(), LogLevel::INFO);
    ss.str("");
    std::shared_ptr<realization::Formulation_Manager> manager =
        std::make_shared<realization::Formulation_Manager>(REALIZATION_CONFIG_PATH);
    manager->read(catchment_collection, utils::getStdOut());

// TODO refactor manager->read so certain configs can be queried before the entire
// realization collection is created
#if NGEN_WITH_ROUTING
    if (mpi_rank == 0) { // Run t-route from single process
        if (manager->get_using_routing()) {
            ss << "Using Routing" << std::endl;
            LOG(ss.str(), LogLevel::INFO);
            ss.str("");
        } else {
            ss << "Not Using Routing" << std::endl;
            LOG(ss.str(), LogLevel::INFO);
            ss.str("");
        }
    }
#endif // NGEN_WITH_ROUTING
    ss << "Building Feature Index" << std::endl;
    LOG(ss.str(), LogLevel::INFO);
    ss.str("");
    std::string link_key = "toid";
    nexus_collection->link_features_from_property(nullptr, &link_key);

#if NGEN_WITH_MPI
    // mpirun with one processor without partition file
    if (mpi_num_procs == 1) {
        Partition_One partition_one;
        partition_one.generate_partition(catchment_collection);
        local_data = std::move(partition_one.partition_data);
    }
    hy_features::HY_Features_MPI features = hy_features::HY_Features_MPI(
        local_data,
        nexus_collection,
        manager,
        mpi_rank,
        mpi_num_procs
    );
#else
    hy_features::HY_Features features = hy_features::HY_Features(nexus_collection, manager);
#endif

    // validate dendritic connections
    features.validate_dendritic();
    // TODO don't really need catchment_collection once catchments are added to nexus collection
    // Still using  catchments for geometry at the moment, fix this later
    // catchment_collection.reset();

    // T-ROUTE data storage
    std::unordered_map<std::string, int> nexus_indexes;
#if NGEN_WITH_ROUTING
    {
        int nexus_index = 0;
        for (int i = 0; i < nexus_collection->get_size(); ++i) {
            auto const& feature = nexus_collection->get_feature(i);
            std::string feature_id = feature->get_id();
            if (hy_features::identifiers::isNexus(feature_id.substr(0, 3))) {
                nexus_indexes[feature_id] = nexus_index;
                nexus_index += 1;
            }
        }
    }
#endif // NGEN_WITH_ROUTING

    nexus_collection.reset();

    ss << "Running Models" << std::endl;
    LOG(ss.str(), LogLevel::INFO);
    ss.str("");

    // check the time loops for the existing layers
    ngen::LayerDataStorage& layer_meta_data = manager->get_layer_metadata();

    // get the keys for the existing layers
    std::vector<int>& keys = layer_meta_data.get_keys();

    // FIXME refactor the layer building to avoid this mess
    std::vector<double> time_steps;
    unsigned int errCount = 0;
    for (int i = 0; i < keys.size(); ++i) {
        auto& m_data = layer_meta_data.get_layer(keys[i]);
        try {
            double c_value =
                UnitsHelper::get_converted_value(m_data.time_step_units, m_data.time_step, "s");
            time_steps.push_back(c_value);
        } catch (const std::runtime_error& e) {
            time_steps.push_back(m_data.time_step);
            errCount++;
        }
    }
    if (errCount) {
        ss << "ngen main: layer_meta_data timesteps Used " << errCount << " unconverted value(s)."
           << std::endl;
        LOG(ss.str(), LogLevel::SEVERE);
        ss.str("");
    }

    // now create the layer objects

    // first make sure that the layer are listed in decreasing order
    boost::range::sort(keys, std::greater<int>());

    std::vector<std::shared_ptr<ngen::Layer>> layers;
    layers.resize(keys.size());

    for (long i = 0; i < keys.size(); ++i) {
        auto& desc = layer_meta_data.get_layer(keys[i]);
        std::vector<std::string> cat_ids;

        // make a new simulation time object with a different output interval
        Simulation_Time sim_time(*manager->Simulation_Time_Object, time_steps[i]);
        if (manager->has_domain_formulation(keys[i])) {
            // create a domain wide layer
            auto formulation = manager->get_domain_formulation(keys[i]);
            layers[i] =
                std::make_shared<ngen::DomainLayer>(desc, sim_time, features, 0, formulation);
        } else {
            for (std::string id : features.catchments(keys[i])) {
                cat_ids.push_back(id);
            }
            if (keys[i] != 0) {
                layers[i] = std::make_shared<ngen::Layer>(
                    desc,
                    cat_ids,
                    sim_time,
                    features,
                    catchment_collection,
                    0
                );
            } else {
                layers[i] = std::make_shared<ngen::SurfaceLayer>(
                    desc,
                    cat_ids,
                    sim_time,
                    features,
                    catchment_collection,
                    0
                );
            }
        }
    }

    // T-ROUTE data storage
    std::unordered_map<std::string, int> catchment_indexes;
#if NGEN_WITH_ROUTING
    size_t catchment_collection_size = catchment_collection->get_size();
    for (int i = 0; i < catchment_collection_size; ++i) {
        auto feature = catchment_collection->get_feature(i);
        std::string feature_id = feature->get_id();
        catchment_indexes[feature_id] = i;
    }
#endif // NGEN_WITH_ROUTING

    auto simulation = std::make_unique<NgenSimulation>(manager,
                                                       layers,
                                                       std::move(catchment_indexes),
                                                       std::move(nexus_indexes),
                                                       mpi_rank,
                                                       mpi_num_procs);

    auto time_done_init                             = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_elapsed_init = time_done_init - time_start;

    simulation->run_catchments();

#if NGEN_WITH_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    if (mpi_rank == 0) {
        ss << "Finished " << manager->Simulation_Time_Object->get_total_output_times()
           << " timesteps." << std::endl;
        LOG(ss.str(), LogLevel::INFO);
        ss.str("");
    }

    auto time_done_simulation                             = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_elapsed_simulation = time_done_simulation - time_done_init;

    // Write nexus outflow CSV files in bulk at the end of the run,
    // rather than as the simulation runs, to avoid issues when
    // restarting an interrupted run. This would be less of an issue
    // if the data were written to a more resilient structured format.
    write_nexus_outflow_csv_files(manager->get_output_root(), simulation, features);

#if NGEN_WITH_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    if (mpi_rank == 0) {
        ss << "Finished writing nexus outflow files" << std::endl;
        LOG(ss.str(), LogLevel::INFO);
        ss.str("");
    }

    auto time_done_nexus_output                             = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_elapsed_nexus_output = time_done_nexus_output - time_done_simulation;

#if NGEN_WITH_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    if (manager->get_using_routing()) {
        simulation->run_routing(features);
    }

    auto time_done_routing                             = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_elapsed_routing = time_done_routing - time_done_nexus_output;

#if NGEN_WITH_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

#if NGEN_WITH_COASTAL
    if (manager->get_using_coastal()) {
        simulation->run_coastal();
    }

    auto time_done_coastal                             = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_elapsed_coastal = time_done_coastal - time_done_routing;
#endif

    if (mpi_rank == 0) {
        ss << "NGen top-level timings:"
           << "\n\tNGen::init: " << time_elapsed_init.count()
           << "\n\tNGen::simulation: " << time_elapsed_simulation.count()
#if NGEN_WITH_ROUTING
           << "\n\tNGen::routing: " << time_elapsed_routing.count()
#endif
#if NGEN_WITH_COASTAL
           << "\n\tNGen::coastal: " << time_elapsed_coastal.count()
#endif
           << std::endl;
        LOG(ss.str(), LogLevel::INFO);
        ss.str("");
    }

    manager->finalize();

#if NGEN_WITH_MPI
    MPI_Finalize();
#endif

    return 0;
}
