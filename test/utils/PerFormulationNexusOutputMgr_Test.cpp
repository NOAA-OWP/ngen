//
// Created by Robert Bartel on 12/29/25.
//

#ifdef NGEN_NETCDF_TESTS_ACTIVE

#include "gtest/gtest.h"
#include "PerFormulationNexusOutputMgr.hpp"
#include "FileChecker.h"

// Again non-Windows specific here
#include <sys/stat.h>
#include <sys/types.h>

#include "HY_HydroNexus.hpp"

#if NGEN_WITH_MPI
#include <mpi.h>
#include <algorithm>
#endif

#if NGEN_MPI_UNIT_TESTS && !NGEN_WITH_MPI
static_assert(false, "Can't activate MPI tests for PerFormulationNexusOutputMgr_Test without MPI active!");
#endif

class PerFormulationNexusOutputMgr_Test: public ::testing::Test {

protected:

    void SetUp() override;
    void TearDown() override;

    static std::string friend_get_current_formulation_id(const utils::PerFormulationNexusOutputMgr* obj) { return obj->current_formulation_id; }
    static std::unordered_map<std::string, std::string> friend_get_nexus_outfiles(const utils::PerFormulationNexusOutputMgr* obj) { return obj->nexus_outfiles; }
    static std::string friend_get_nc_flow_var_name(const utils::PerFormulationNexusOutputMgr* obj) { return obj->nc_flow_var_name; }
    static std::string friend_get_nc_nex_id_dim_name(const utils::PerFormulationNexusOutputMgr* obj) { return obj->nc_nex_id_dim_name; }
    static std::string friend_get_nc_time_dim_name(const utils::PerFormulationNexusOutputMgr* obj) { return obj->nc_time_dim_name; }


    static void friend_write_nexus_ids_once(utils::PerFormulationNexusOutputMgr* obj, int nc_id) {
        return obj->write_nexus_ids_once(nc_id);
    }

    int rank, size;

    std::string output_root = "/tmp/PerFormulationNexusOutputMgrTest";

    std::vector<std::string> ex_0_form_0_nexus_ids = {"nex-1", "nex-2", "nex-3", "nex-4"};
    std::shared_ptr<std::vector<std::string>> ex_0_form_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>{"form-0"});
    std::vector<std::string> ex_0_timestamps = {"2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z"};
    std::vector<std::time_t> ex_0_timestamps_seconds = {1735707600, 1735711200};
    // Per timestamp, per-nexus data
    std::vector<std::vector<double>> ex_0_data = {{1.0, 2.0, 3.0, 4.0}, {11.0, 12.0, 13.0, 14.0}};

    std::shared_ptr<std::vector<std::string>> ex_1_form_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>{"form-0"});
    std::vector<std::string> ex_1_form_0_group_a_nexus_ids = ex_0_form_0_nexus_ids;
    std::vector<std::string> ex_1_form_0_group_b_nexus_ids = {"nex-5", "nex-6", "nex-7", "nex-8"};
    std::vector<std::string> ex_1_form_0_all_nexus_id = {"nex-1", "nex-2", "nex-3", "nex-4", "nex-5", "nex-6", "nex-7", "nex-8"};
    std::vector<std::vector<double>> ex_1_group_a_data = ex_0_data;
    std::vector<std::vector<double>> ex_1_group_b_data = {{51.0, 52.0, 53.0, 54.0}, {61.0, 62.0, 63.0, 64.0}};
    std::vector<std::vector<double>> ex_1_all_data = {{1.0, 2.0, 3.0, 4.0, 51.0, 52.0, 53.0, 54.0}, {11.0, 12.0, 13.0, 14.0, 61.0, 62.0, 63.0, 64.0}};
    std::vector<std::string> ex_1_timestamps = {"2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z"};
    std::vector<std::time_t> ex_1_timestamps_seconds = {1735707600, 1735711200};

    std::shared_ptr<std::vector<std::string>> ex_2_form_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>{"form-0"});
    std::vector<std::string> ex_2_form_0_group_a_nexus_ids = ex_1_form_0_group_a_nexus_ids;
    std::vector<std::string> ex_2_form_0_group_b_nexus_ids = ex_1_form_0_group_b_nexus_ids;
    std::vector<std::string> ex_2_form_0_all_nexus_id = ex_1_form_0_all_nexus_id;

    size_t ex_2_num_time_steps = 720;
    long ex_2_initial_time_seconds = 1735707600;
    std::vector<std::vector<double>> ex_2_group_a_data;
    std::vector<std::vector<double>> ex_2_group_b_data;
    std::vector<std::vector<double>> ex_2_all_data;
    std::vector<std::string> ex_2_timestamps; // Will use dummy data for text time stamps ... should be fine for this
    std::vector<std::time_t> ex_2_timestamps_seconds;

    std::vector<std::string> files_to_cleanup;

};

void PerFormulationNexusOutputMgr_Test::SetUp()
{
    Test::SetUp();

    #if NGEN_MPI_UNIT_TESTS
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    #else
    // Make sure to set here
    rank = 0;
    size = 1;
    #endif

    // Create test output root dir, if it does not exist
    // Recall that if not using MPI, rank will always be 0, limiting to rank 0 correct in either serial or parallel
    if (rank == 0 && !utils::FileChecker::directory_exists(output_root) && mkdir(output_root.c_str(), 0777) != 0) {
        throw std::runtime_error("Failed to create output root directory '" + output_root + "' (rank " + std::to_string(rank) + ")");
    }

    // Generate the data used for example 2
    std::string dummy_time_stamp = "0000-00-00T00:00:00Z";
    ex_2_group_a_data.resize(720);
    ex_2_group_b_data.resize(720);
    ex_2_all_data.resize(720);
    ex_2_timestamps.resize(720);
    ex_2_timestamps_seconds.resize(720);

    for (size_t t = 0; t < ex_2_num_time_steps; ++t) {
        ex_2_all_data[t].resize(ex_2_form_0_all_nexus_id.size());
        ex_2_group_a_data[t].resize(ex_2_form_0_group_a_nexus_ids.size());
        ex_2_group_b_data[t].resize(ex_2_form_0_group_b_nexus_ids.size());

        for (size_t i = 0; i < 8; ++i) {
            double val = static_cast<double>(i) + static_cast<double>(t + 1) / 1000.0;
            ASSERT_GT(val, 0.0);
            ex_2_all_data[t][i] = val;
            if (i < ex_2_group_a_data[t].size()) {
                ex_2_group_a_data[t][i] = val;
            }
            else {
                ex_2_group_b_data[t][i - ex_2_group_a_data[t].size()] = val;
            }
        }

        ex_2_timestamps[t] = dummy_time_stamp;
        ex_2_timestamps_seconds[t] = ex_2_initial_time_seconds + t * 3600;
    }

    #if NGEN_MPI_UNIT_TESTS
    MPI_Barrier(MPI_COMM_WORLD);
    #endif
}

void PerFormulationNexusOutputMgr_Test::TearDown()
{
    Test::TearDown();

    // Have tearDown clean up produced file(s) and directory
    for (const auto& f : files_to_cleanup) {
        // Recall that if not using MPI, rank will always be 0, so works in either serial or parallel
        if (rank == 0 && std::remove(f.c_str()) != 0) {
            throw std::runtime_error("Failed to cleanup file '" + f + "'");
        }
    }
    #if NGEN_MPI_UNIT_TESTS
    MPI_Barrier(MPI_COMM_WORLD);
    #endif
}

#if NGEN_MPI_UNIT_TESTS

/** Test that example 2 gets constructed and one instance creates the NetCDF file. */
TEST_F(PerFormulationNexusOutputMgr_Test, construct_2_a)
{
    auto test_form_names = std::make_shared<std::vector<std::string>>(ex_2_form_names->size());
    for (size_t i = 0; i < ex_2_form_names->size(); ++i) {
        (*test_form_names)[i] = ex_2_form_names->at(i) + "_construct_2_a";
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Pick the right nexus id and data group for this rank
    std::vector<std::string> *nexus_ids;
    if (rank == 0) {
        nexus_ids = &ex_2_form_0_group_a_nexus_ids;
    }
    else {
        nexus_ids = &ex_2_form_0_group_b_nexus_ids;
    }

    // Create manager instance for this rank
    const std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_2_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_2_form_0_group_b_nexus_ids.size())
    };
    utils::PerFormulationNexusOutputMgr mgr(*nexus_ids, test_form_names, output_root, ex_2_num_time_steps, rank, nexus_per_rank);

    if (rank == 0) {
        // Make sure we know what files to clean up
        std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
        for (const std::string& f : *filenames) {
            files_to_cleanup.push_back(f);
        }
    }

    ASSERT_TRUE(utils::FileChecker::file_can_be_written(mgr.get_filenames()->at(0)));
}


/** Test example 2 writes flow data correctly (same as would be over serial) with multiple simultaneous writes via MPI. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_2_a)
{
    MPI_Barrier(MPI_COMM_WORLD);

    // Expect only two ranks
    ASSERT_LE(rank, 1);
    ASSERT_EQ(size, 2);

    for (size_t t = 0; t < ex_2_all_data.size(); ++t) {
        ASSERT_TRUE(std::all_of(ex_2_all_data[t].cbegin(), ex_2_all_data[t].cend(), [](double d) { return d > 0.0;}));
    }

    // Pick the right nexus id and data group for this rank
    std::vector<std::string> *nexus_ids;
    std::vector<std::vector<double>> *group_data;
    if (rank == 0) {
        nexus_ids = &ex_2_form_0_group_a_nexus_ids;
        group_data = &ex_2_group_a_data;
    }
    else {
        nexus_ids = &ex_2_form_0_group_b_nexus_ids;
        group_data = &ex_2_group_b_data;
    }

    // Create manager instance for this rank
    const std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_2_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_2_form_0_group_b_nexus_ids.size())
    };
    utils::PerFormulationNexusOutputMgr mgr(*nexus_ids, ex_2_form_names, output_root, ex_2_num_time_steps, rank, nexus_per_rank);

    // Add to files_to_clean_up, but only for rank 0 to deal with (they should be the same sets of files)
    if (rank == 0) {
        // Make sure we know what files to clean up
        std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
        for (const std::string& f : *filenames) {
            files_to_cleanup.push_back(f);
        }
    }

    // Write for this rank's nexuses
    for (size_t t = 0; t < ex_2_timestamps.size(); ++t) {
        for (size_t n = 0; n < nexus_ids->size(); ++n) {
            mgr.receive_data_entry(ex_2_form_names->at(0),
                                   nexus_ids->at(n),
                                   utils::time_marker(t, ex_2_timestamps_seconds[t], ex_2_timestamps[t]),
                                   group_data->at(t)[n]);
        }
        mgr.commit_writes();
    }

    // When done writing everything, another barrier before any checks/asserts
    MPI_Barrier(MPI_COMM_WORLD);

    // Finally, compare data read from both

    // Should only be one filename
    const netCDF::NcFile ncf(mgr.get_filenames()->at(0), netCDF::NcFile::read);
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr));

    ASSERT_FALSE(flow.isNull());
    ASSERT_EQ(flow.getDim(0).getSize(), 8);
    ASSERT_EQ(flow.getDim(1).getSize(), 720);
    // Note that nexus feature_id dim comes before time dim, so have to order this way
    double values[8][720];
    flow.getVar(values);
    for (size_t t = 0; t < ex_2_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_2_form_0_all_nexus_id.size(); ++n) {
            ASSERT_EQ(values[n][t], ex_2_all_data[t][n]) << "On timestep " << t << " for nexus id " << nexus_ids->at(n)
                << " value was " << values[n][t] << " but expected was " << ex_2_all_data[t][n] << "; \n"
                << "Full timestep data for this timestep is: \n" << values << "\n ***** \n";
        }
    }
}

/** Test example 2 writes feature ids correctly using multiple simultaneous writes via MPI. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_2_b)
{
    MPI_Barrier(MPI_COMM_WORLD);

    // Expect only two ranks
    ASSERT_LE(rank, 1);
    ASSERT_EQ(size, 2);

    // Pick the right nexus id and data group for this rank
    std::vector<std::string> *nexus_ids;
    std::vector<std::vector<double>> *group_data;
    if (rank == 0) {
        nexus_ids = &ex_2_form_0_group_a_nexus_ids;
        group_data = &ex_2_group_a_data;
    }
    else {
        nexus_ids = &ex_2_form_0_group_b_nexus_ids;
        group_data = &ex_2_group_b_data;
    }

    // Create manager instance for this rank
    const std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_2_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_2_form_0_group_b_nexus_ids.size())
    };
    utils::PerFormulationNexusOutputMgr mgr(*nexus_ids, ex_2_form_names, output_root, ex_2_num_time_steps, rank, nexus_per_rank);

    // Add to files_to_clean_up, but only for rank 0 to deal with (they should be the same sets of files)
    if (rank == 0) {
        // Make sure we know what files to clean up
        std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
        for (const std::string& f : *filenames) {
            files_to_cleanup.push_back(f);
        }
    }

    // Write for this rank's nexuses, but only for the first timestep (i.e., 0) to test the feature ID writes
    size_t t = 0;
    for (size_t n = 0; n < nexus_ids->size(); ++n) {
        mgr.receive_data_entry(ex_2_form_names->at(0),
                               nexus_ids->at(n),
                               utils::time_marker(t, ex_2_timestamps_seconds[t], ex_2_timestamps[t]),
                               group_data->at(t)[n]);
    }
    mgr.commit_writes();

    // When done writing, another barrier before any checks/asserts
    MPI_Barrier(MPI_COMM_WORLD);

    // Now, compare feature id variable data from both

    // Should only be one filename
    const netCDF::NcFile ncf(mgr.get_filenames()->at(0), netCDF::NcFile::read);
    const netCDF::NcVar nc_var_nex_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr));

    ASSERT_FALSE(nc_var_nex_ids.isNull());
    ASSERT_EQ(nc_var_nex_ids.getDim(0).getSize(), ex_2_form_0_all_nexus_id.size());

    // Note that nexus feature_id dim comes before time dim, so have to order this way
    std::vector<unsigned int> nex_id_numeric(ex_2_form_0_all_nexus_id.size());
    nc_var_nex_ids.getVar(nex_id_numeric.data());

    std::vector<std::string> nex_id_strs(nex_id_numeric.size());
    for (size_t i = 0; i < nex_id_strs.size(); ++i) {
        nex_id_strs[i] = "nex-" + std::to_string(nex_id_numeric[i]);
    }
    ASSERT_EQ(nex_id_strs, ex_2_form_0_all_nexus_id);
}

#else


/** Test that example 0 gets constructed and expects a single managed file. */
TEST_F(PerFormulationNexusOutputMgr_Test, construct_0_a)
{
    std::string form_name = ex_0_form_0_nexus_ids[0];

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    ASSERT_EQ(filenames->size(), 1);
}

/** Test that example 0 gets constructed and single instance creates the NetCDF file. */
TEST_F(PerFormulationNexusOutputMgr_Test, construct_0_b)
{
    std::string form_name = ex_0_form_0_nexus_ids[0];

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    ASSERT_TRUE(utils::FileChecker::file_can_be_written(filenames->at(0)));
}

/** Test that example 0 gets constructed and has expected nexus output files. */
TEST_F(PerFormulationNexusOutputMgr_Test, nexus_out_files_0_a)
{
    std::string form_name = ex_0_form_0_nexus_ids[0];

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    std::unordered_map<std::string, std::string> nexus_outfiles = friend_get_nexus_outfiles(&mgr);
    ASSERT_EQ(nexus_outfiles.size(), ex_0_form_names->size());
    for (size_t i = 0; i < ex_0_form_names->size(); ++i) {
        ASSERT_TRUE(nexus_outfiles.find(ex_0_form_names->at(i)) != nexus_outfiles.end());
    }
}

/** Test that current formulation id is as expected after receiving data entry. */
TEST_F(PerFormulationNexusOutputMgr_Test, receive_data_entry_0_a) {

    int nex_id_index = 0;
    std::string form_name = ex_0_form_0_nexus_ids[0];
    int time_index = 0;

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    utils::time_marker current_time = utils::time_marker(time_index, ex_0_timestamps_seconds[time_index], ex_0_timestamps[time_index]);

    mgr.receive_data_entry(form_name, ex_0_form_0_nexus_ids[nex_id_index], current_time,
        ex_0_data[time_index][nex_id_index]);

    std::string current_form_id = friend_get_current_formulation_id(&mgr);
    ASSERT_EQ(current_form_id, form_name);
}

/** Make sure can't receive additional data entries for more than one time steps without writing first. */
TEST_F(PerFormulationNexusOutputMgr_Test, receive_data_entry_0_b)
{
    std::string form_name = ex_0_form_names->at(0);
    int time_index = 0;

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    utils::time_marker current_time = utils::time_marker(time_index, ex_0_timestamps_seconds[time_index], ex_0_timestamps[time_index]);

    for (size_t n = 0; n < ex_0_form_0_nexus_ids.size(); ++n) {
        mgr.receive_data_entry(form_name,
            ex_0_form_0_nexus_ids[n],
            utils::time_marker(0, ex_0_timestamps_seconds[0], ex_0_timestamps[0]),
            ex_0_data[0][n]);
    }

    ASSERT_THROW(mgr.receive_data_entry(form_name,
                                ex_0_form_0_nexus_ids[0],
                                        utils::time_marker(1, ex_0_timestamps_seconds[1], ex_0_timestamps[1]),
                                        ex_0_data[1][0]),
                 std::runtime_error);
}

/** Test that receive_data_entry doesn't actually write any data to the file. */
TEST_F(PerFormulationNexusOutputMgr_Test, receive_data_entry_0_c) {

    std::string form_name = ex_0_form_names->at(0);

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    for (size_t n = 0; n < ex_0_form_0_nexus_ids.size(); ++n) {
        mgr.receive_data_entry(form_name,
                               ex_0_form_0_nexus_ids[n],
                               utils::time_marker(0, ex_0_timestamps_seconds[0], ex_0_timestamps[0]),
                               ex_0_data[0][n]);
    }

    // Should only be one filename
    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr));

    ASSERT_FALSE(flow.isNull());
    double values[4];
    flow.getVar(values);
    for (size_t i = 0; i < ex_0_form_0_nexus_ids.size(); ++i) {
        ASSERT_NE(values[i], ex_0_data[0][i]);
    }
}

/** Make sure writes work for example 0 (single instance) for a single time step. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_0_a) {

    std::string form_name = ex_0_form_names->at(0);

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    for (size_t n = 0; n < ex_0_form_0_nexus_ids.size(); ++n) {
        mgr.receive_data_entry(form_name,
                               ex_0_form_0_nexus_ids[n],
                               utils::time_marker(0, ex_0_timestamps_seconds[0], ex_0_timestamps[0]),
                               ex_0_data[0][n]);
    }
    mgr.commit_writes();

    // Should only be one filename
    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr));

    ASSERT_FALSE(flow.isNull());
    double values[4][2];
    flow.getVar(values);
    for (size_t i = 0; i < ex_0_form_0_nexus_ids.size(); ++i) {
        ASSERT_EQ(values[i][0], ex_0_data[0][i]);
    }
}

/** Make sure writes work example 0 (single instance) for multiple time steps. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_0_b) {

    std::string form_name = ex_0_form_names->at(0);

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    for (size_t t = 0; t < ex_0_timestamps.size(); ++t) {
        for (int n = 0; n < ex_0_form_0_nexus_ids.size(); ++n) {
            mgr.receive_data_entry(form_name,
                                   ex_0_form_0_nexus_ids[n],
                                   utils::time_marker(t, ex_0_timestamps_seconds[t], ex_0_timestamps[t]),
                                   ex_0_data[t][n]);
        }
        mgr.commit_writes();
    }

    // Should only be one filename
    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr));

    ASSERT_FALSE(flow.isNull());
    // Note that nexus feature_id dim comes before time dim, so have to order this way
    double values[4][2];
    flow.getVar(values);
    for (size_t t = 0; t < ex_0_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_0_form_0_nexus_ids.size(); ++n) {
            ASSERT_EQ(values[n][t], ex_0_data[t][n]);
        }
    }
}

/** Make sure writes don't work for example 0 if it has not received data for all managed nexuses. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_0_c) {

    std::string form_name = ex_0_form_names->at(0);

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    // Importantly, using " - 1" below to not do things for all the nexuses
    for (size_t n = 0; n < ex_0_form_0_nexus_ids.size() - 1; ++n) {
        mgr.receive_data_entry(form_name,
                               ex_0_form_0_nexus_ids[n],
                               utils::time_marker(0, ex_0_timestamps_seconds[0], ex_0_timestamps[0]),
                               ex_0_data[0][n]);
    }
    ASSERT_THROW(mgr.commit_writes(), std::runtime_error);
}

/** Test that example 0 has the nexus id var values written to the NetCDF file. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_0_d)
{
    std::string form_name = ex_0_form_names->at(0);

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    for (size_t n = 0; n < ex_0_form_0_nexus_ids.size(); ++n) {
        mgr.receive_data_entry(form_name,
                               ex_0_form_0_nexus_ids[n],
                               utils::time_marker(0, ex_0_timestamps_seconds[0], ex_0_timestamps[0]),
                               ex_0_data[0][n]);
    }
    mgr.commit_writes();

    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar nexus_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr));

    // These should all have size 4 for the current example, equal to the size of ex_0_form_0_nexus_ids
    ASSERT_EQ(nexus_ids.getDim(0).getSize(), 4);

    std::vector<unsigned int> nex_id_numeric(4);
    nexus_ids.getVar(nex_id_numeric.data());

    std::vector<std::string> nex_id_strs(nex_id_numeric.size());
    for (size_t i = 0; i < nex_id_strs.size(); ++i) {
        nex_id_strs[i] = "nex-" + std::to_string(nex_id_numeric[i]);
    }

    ASSERT_EQ(nex_id_strs, ex_0_form_0_nexus_ids);
}

/** Make sure writes work example 1 (multiple instances) for multiple time steps, alternating writes. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_1_a) {

    std::string form_name = ex_1_form_names->at(0);


    std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_1_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_1_form_0_group_b_nexus_ids.size())
    };

    utils::PerFormulationNexusOutputMgr mgr_a(ex_1_form_0_group_a_nexus_ids, ex_1_form_names, output_root, 2, 0, nexus_per_rank);
    utils::PerFormulationNexusOutputMgr mgr_b(ex_1_form_0_group_b_nexus_ids, ex_1_form_names, output_root, 2, 1, nexus_per_rank);

    // Make sure we know what files to clean up (and these should be the same for each)
    std::shared_ptr<std::vector<std::string>> filenames = mgr_a.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    // Alternate writing, first all for group_a in a time step, then all for group b in a time step, then the next time step
    for (size_t t = 0; t < ex_1_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_1_form_0_group_a_nexus_ids.size(); ++n) {
            mgr_a.receive_data_entry(form_name,
                                     ex_1_form_0_group_a_nexus_ids[n],
                                     utils::time_marker(t, ex_1_timestamps_seconds[t], ex_1_timestamps[t]),
                                     ex_1_group_a_data[t][n]);
        }
        mgr_a.commit_writes();
        for (size_t n = 0; n < ex_1_form_0_group_b_nexus_ids.size(); ++n) {
            mgr_b.receive_data_entry(form_name,
                                     ex_1_form_0_group_b_nexus_ids[n],
                                     utils::time_marker(t, ex_1_timestamps_seconds[t], ex_1_timestamps[t]),
                                     ex_1_group_b_data[t][n]);
        }
        mgr_b.commit_writes();
    }

    // Should still only be one filename
    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr_a));

    ASSERT_FALSE(flow.isNull());
    // Note that nexus feature_id dim comes before time dim, so have to order this way
    double values[8][2];
    flow.getVar(values);
    for (size_t t = 0; t < ex_1_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_1_form_0_all_nexus_id.size(); ++n) {
            ASSERT_EQ(values[n][t], ex_1_all_data[t][n]);
        }
    }
}

/**
 * Make sure writes work example 1 (multiple instances) for multiple time steps, where all the second instance stuff is
 * written and then all the first instance stuff is written.
 */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_1_b) {

    std::string form_name = ex_1_form_names->at(0);

    std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_1_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_1_form_0_group_b_nexus_ids.size())
    };

    utils::PerFormulationNexusOutputMgr mgr_a(ex_1_form_0_group_a_nexus_ids, ex_1_form_names, output_root, 2, 0, nexus_per_rank);
    utils::PerFormulationNexusOutputMgr mgr_b(ex_1_form_0_group_b_nexus_ids, ex_1_form_names, output_root, 2, 1, nexus_per_rank);

    // Make sure we know what files to clean up (and these should be the same for each)
    std::shared_ptr<std::vector<std::string>> filenames = mgr_a.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    // Write all the b group stuff first
    for (size_t t = 0; t < ex_1_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_1_form_0_group_b_nexus_ids.size(); ++n) {
            mgr_b.receive_data_entry(form_name,
                                     ex_1_form_0_group_b_nexus_ids[n],
                                     utils::time_marker(t, ex_1_timestamps_seconds[t], ex_1_timestamps[t]),
                                     ex_1_group_b_data[t][n]);
        }
        mgr_b.commit_writes();
    }
    // Then come back and write all the a group stuff
    for (size_t t = 0; t < ex_1_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_1_form_0_group_a_nexus_ids.size(); ++n) {
            mgr_a.receive_data_entry(form_name,
                                     ex_1_form_0_group_a_nexus_ids[n],
                                     utils::time_marker(t, ex_1_timestamps_seconds[t], ex_1_timestamps[t]),
                                     ex_1_group_a_data[t][n]);
        }
        mgr_a.commit_writes();
    }

    // Should still only be one filename
    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr_a));

    ASSERT_FALSE(flow.isNull());
    // Note that nexus feature_id dim comes before time dim, so have to order this way
    double values[8][2];
    flow.getVar(values);
    for (size_t t = 0; t < ex_1_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_1_form_0_all_nexus_id.size(); ++n) {
            ASSERT_EQ(values[n][t], ex_1_all_data[t][n]);
        }
    }
}

/** Test that example 1 has the nexus id var values written to the NetCDF file with multiple instances. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_1_c)
{
    std::string form_name = ex_1_form_names->at(0);

    std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_1_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_1_form_0_group_b_nexus_ids.size())
    };

    utils::PerFormulationNexusOutputMgr mgr_a(ex_1_form_0_group_a_nexus_ids, ex_1_form_names, output_root, 2, 0, nexus_per_rank);
    utils::PerFormulationNexusOutputMgr mgr_b(ex_1_form_0_group_b_nexus_ids, ex_1_form_names, output_root, 2, 1, nexus_per_rank);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr_a.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    // Alternate writing, first all for group_a in a time step, then all for group b in a time step, then the next time step
    for (size_t t = 0; t < ex_1_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_1_form_0_group_a_nexus_ids.size(); ++n) {
            mgr_a.receive_data_entry(form_name,
                                     ex_1_form_0_group_a_nexus_ids[n],
                                     utils::time_marker(t, ex_1_timestamps_seconds[t], ex_1_timestamps[t]),
                                     ex_1_group_a_data[t][n]);
        }
        mgr_a.commit_writes();
        for (size_t n = 0; n < ex_1_form_0_group_b_nexus_ids.size(); ++n) {
            mgr_b.receive_data_entry(form_name,
                                     ex_1_form_0_group_b_nexus_ids[n],
                                     utils::time_marker(t, ex_1_timestamps_seconds[t], ex_1_timestamps[t]),
                                     ex_1_group_b_data[t][n]);
        }
        mgr_b.commit_writes();
    }


    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar nexus_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr_a));

    // These should all have size 8 for the current example, equal to the size of ex_1_form_0_all_nexus_id
    ASSERT_EQ(nexus_ids.getDim(0).getSize(), 8);
    std::vector<unsigned int> nex_id_numeric(8);
    nexus_ids.getVar(nex_id_numeric.data());

    std::vector<std::string> nex_id_strs(nex_id_numeric.size());
    for (size_t i = 0; i < nex_id_strs.size(); ++i) {
        nex_id_strs[i] = "nex-" + std::to_string(nex_id_numeric[i]);
    }

    ASSERT_EQ(nex_id_strs, ex_1_form_0_all_nexus_id);
}

/** Make sure writes work for example 2 for multiple time steps, but using just a single instance. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_2_c) {

    std::string form_name = ex_2_form_names->at(0);
    std::vector<std::string> *nexus_ids = &(ex_2_form_0_all_nexus_id);
    std::vector<std::vector<double>> *group_data = &(ex_2_all_data);

    for (size_t t = 0; t < ex_2_all_data.size(); ++t) {
        ASSERT_TRUE(std::all_of(ex_2_all_data[t].cbegin(), ex_2_all_data[t].cend(), [](double d) { return d > 0.0;}));
    }

    utils::PerFormulationNexusOutputMgr mgr(*nexus_ids, ex_2_form_names, output_root, 720);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    for (size_t t = 0; t < ex_2_timestamps.size(); ++t) {
        for (int n = 0; n < nexus_ids->size(); ++n) {
            mgr.receive_data_entry(form_name,
                                   nexus_ids->at(n),
                                   utils::time_marker(t, ex_2_timestamps_seconds[t], ex_2_timestamps[t]),
                                   group_data->at(t)[n]);
        }
        mgr.commit_writes();
    }

    // Should only be one filename
    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr));

    ASSERT_FALSE(flow.isNull());
    // Note that nexus feature_id dim comes before time dim, so have to order this way
    double values[8][720];
    flow.getVar(values);
    for (size_t t = 0; t < ex_2_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_2_form_0_all_nexus_id.size(); ++n) {
            ASSERT_EQ(values[n][t], ex_2_all_data[t][n]) << "On timestep " << t << " for nexus id " << nexus_ids->at(n)
                << " value was " << values[n][t] << " but expected was " << ex_2_all_data[t][n] << "; \n"
                << "Full timestep data for this timestep is: \n" << values << "\n ***** \n";
        }
    }
}

/** Test that example 0 works with write_nexus_ids_once and has nexus id var values written to NetCDF file. */
TEST_F(PerFormulationNexusOutputMgr_Test, write_nexus_ids_once_0_a)
{
    std::string form_name = ex_0_form_names->at(0);

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    {
        // Just call write_nexus_ids_once_c
        int write_nc_id;
        nc_open(filenames->at(0).c_str(), NC_WRITE, &write_nc_id);
        friend_write_nexus_ids_once(&mgr, write_nc_id);
    }

    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar nexus_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr));

    // These should all have size 4 for the current example, equal to the size of ex_0_form_0_nexus_ids
    ASSERT_EQ(nexus_ids.getDim(0).getSize(), 4);

    std::vector<unsigned int> nex_id_numeric(4);
    nexus_ids.getVar(nex_id_numeric.data());

    std::vector<std::string> nex_id_strs(nex_id_numeric.size());
    for (size_t i = 0; i < nex_id_strs.size(); ++i) {
        nex_id_strs[i] = "nex-" + std::to_string(nex_id_numeric[i]);
    }

    ASSERT_EQ(nex_id_strs, ex_0_form_0_nexus_ids);
}

/**
 * Test that example 1 works with write_nexus_ids_once and has nexus id var values written to NetCDF file with multiple
 * instances.
 */
TEST_F(PerFormulationNexusOutputMgr_Test, write_nexus_ids_once_1_a)
{
    std::string form_name = ex_1_form_names->at(0);

    std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_1_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_1_form_0_group_b_nexus_ids.size())
    };

    utils::PerFormulationNexusOutputMgr mgr_a(ex_1_form_0_group_a_nexus_ids, ex_1_form_names, output_root, 2, 0, nexus_per_rank);
    utils::PerFormulationNexusOutputMgr mgr_b(ex_1_form_0_group_b_nexus_ids, ex_1_form_names, output_root, 2, 1, nexus_per_rank);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr_a.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    {
        // Just call write_nexus_ids_once_c
        int write_nc_id;
        nc_open(filenames->at(0).c_str(), NC_WRITE, &write_nc_id);
        friend_write_nexus_ids_once(&mgr_a, write_nc_id);
        friend_write_nexus_ids_once(&mgr_b, write_nc_id);
    }

    netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar nexus_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr_a));

    // These should all have size 8 for the current example, equal to the size of ex_1_form_0_all_nexus_id
    ASSERT_EQ(nexus_ids.getDim(0).getSize(), 8);
    std::vector<unsigned int> nex_id_numeric(8);
    nexus_ids.getVar(nex_id_numeric.data());

    std::vector<std::string> nex_id_strs(nex_id_numeric.size());
    for (size_t i = 0; i < nex_id_strs.size(); ++i) {
        nex_id_strs[i] = "nex-" + std::to_string(nex_id_numeric[i]);
    }

    ASSERT_EQ(nex_id_strs, ex_1_form_0_all_nexus_id);
}

#endif // #if NGEN_WITH_MPI

#endif // #ifdef NGEN_NETCDF_TESTS_ACTIVE
