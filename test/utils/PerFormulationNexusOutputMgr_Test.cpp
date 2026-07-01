//
// Created by Robert Bartel on 12/29/25.
//

#ifdef NGEN_NETCDF_TESTS_ACTIVE

#ifndef LARGE_NEXUS_EXAMPLE_SIZE
#define LARGE_NEXUS_EXAMPLE_SIZE 40396
#endif

#include "gtest/gtest.h"
#include "output/PerFormulationNexusOutputMgr.hpp"
#include "FileChecker.h"

// Again non-Windows specific here
#include <sys/stat.h>
#include <sys/types.h>

#include <cstring>
#include <algorithm>

#include "HY_HydroNexus.hpp"

#include <netcdf>

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
    static std::string friend_get_nexus_outfile(const utils::PerFormulationNexusOutputMgr* obj) { return obj->nexus_outfile; }
    static std::string friend_get_nc_flow_var_name(const utils::PerFormulationNexusOutputMgr* obj) { return obj->nc_var_name_flow; }
    static std::string friend_get_nc_nex_id_dim_name(const utils::PerFormulationNexusOutputMgr* obj) { return obj->nc_dim_name_nexus_id; }
    static std::string friend_get_nc_time_dim_name(const utils::PerFormulationNexusOutputMgr* obj) { return obj->nc_dim_name_time; }
    static std::string friend_get_parsed_nc_status(const utils::PerFormulationNexusOutputMgr* obj, int nc_status) {return obj->parse_netcdf_return_code(nc_status); }
    static bool friend_is_chunking(const utils::PerFormulationNexusOutputMgr* obj) { return obj->use_chunking_flow_var;}

    static void friend_write_nexus_ids_once(utils::PerFormulationNexusOutputMgr* obj) {
        return obj->write_nexus_ids_once();
    }

    static size_t friend_nexus_id_string_width(const utils::PerFormulationNexusOutputMgr* obj) {
        return obj->nexus_id_string_width;
    }

    /** Longest string length among the given IDs; the expected feature_id string-length dimension. */
    static size_t longest_id_length(const std::vector<std::string>& ids) {
        size_t max_len = 0;
        for (const std::string& id : ids) {
            max_len = std::max(max_len, id.size());
        }
        return max_len;
    }

    static void friend_pack_nexus_id(const std::string& nexus_id, char* buffer, size_t width) {
        utils::PerFormulationNexusOutputMgr::pack_nexus_id(nexus_id, buffer, width);
    }

    /**
     * Read the 2-D fixed-width char ``feature_id`` variable back and reconstruct the full string ID for each
     * nexus, taking the characters up to the first null (CF-style fixed-width char labels).
     *
     * @param var The ``feature_id`` NetCDF variable (dimensioned by nexus count then fixed string width).
     * @param count The number of nexus ID records to read.
     * @return Vector of reconstructed full string IDs, prefix included.
     */
    static std::vector<std::string> read_feature_id_strings(const netCDF::NcVar& var, size_t count) {
        const size_t width = var.getDim(1).getSize();
        std::vector<char> chars(count * width);
        var.getVar(chars.data());
        std::vector<std::string> ids(count);
        for (size_t i = 0; i < count; ++i) {
            const char* start = chars.data() + i * width;
            ids[i] = std::string(start, strnlen(start, width));
        }
        return ids;
    }

    /*
    static void friend_open_file_for_writing_once(utils::PerFormulationNexusOutputMgr* obj) {
        obj->open_unopened_netcdf_file_via_c_api();
    }
    */

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

    // Ex 3 will be small time steps but 40396 nexuses, to test scalability
    std::shared_ptr<std::vector<std::string>> ex_3_form_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>{"form-0"});
    size_t ex_3_nexus_count = LARGE_NEXUS_EXAMPLE_SIZE;
    std::vector<std::string> ex_3_timestamps = {"2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z"};
    std::vector<std::time_t> ex_3_timestamps_seconds = {1735707600, 1735711200};
    std::vector<std::string> ex_3_form_0_group_a_nexus_ids;
    std::vector<std::string> ex_3_form_0_group_b_nexus_ids;
    std::vector<std::string> ex_3_form_0_all_nexus_id;
    std::vector<std::vector<double>> ex_3_group_a_data;
    std::vector<std::vector<double>> ex_3_group_b_data;
    std::vector<std::vector<double>> ex_3_all_data;

    // TODO: Might also need EX 4 with 40396 nexuses but spread about real partitions (and tested exclusively via the MPI stuff)

    // Example 5: nexus feature ID collision scenario. It is possible for the same numeric suffix to appear
    // under more than one ID prefix (here the regular "nex-" prefix alongside a "tnx-" prefix, nex-1
    // alongside tnx-1); the collision is only of the numeric component, since the full IDs remain distinct.
    // The full string feature IDs must therefore be written out so the two physically distinct nexuses stay
    // distinct rather than collapsing to a single numeric ID. Distinct flow values per ID let the read-back
    // verify each row.
    std::shared_ptr<std::vector<std::string>> ex_5_form_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>{"form-0"});
    std::vector<std::string> ex_5_form_0_all_nexus_id = {"nex-1", "tnx-1", "nex-2", "tnx-2"};
    std::vector<std::vector<double>> ex_5_all_data = {{1.0, 101.0, 2.0, 102.0}, {11.0, 111.0, 12.0, 112.0}};
    std::vector<std::string> ex_5_timestamps = {"2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z"};
    std::vector<std::time_t> ex_5_timestamps_seconds = {1735707600, 1735711200};

    // Example 6: the nexus feature ID (numeric) collision exercised across MPI ranks. The colliding
    // pair (nex-1 / tnx-1) is split one ID per rank, so the single output file can only be correct if the
    // MPI gather-to-root path assembles both ranks' fixed-width string IDs (not just the in-process write).
    // Rank 0 owns the regular "nex-" IDs and rank 1 owns the terminal "tnx-" IDs; the global file order is
    // rank-contiguous (rank 0's block then rank 1's block). Distinct per-ID flow values let the read-back
    // verify each row and catch any cross-rank row mix-up.
    std::shared_ptr<std::vector<std::string>> ex_6_form_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>{"form-0"});
    std::vector<std::string> ex_6_form_0_group_a_nexus_ids = {"nex-1", "nex-2"};                      // rank 0
    std::vector<std::string> ex_6_form_0_group_b_nexus_ids = {"tnx-1", "tnx-2"};                      // rank 1
    std::vector<std::string> ex_6_form_0_all_nexus_id = {"nex-1", "nex-2", "tnx-1", "tnx-2"};         // global, rank-contiguous
    std::vector<std::vector<double>> ex_6_group_a_data = {{1.0, 2.0}, {11.0, 12.0}};
    std::vector<std::vector<double>> ex_6_group_b_data = {{101.0, 102.0}, {111.0, 112.0}};
    std::vector<std::vector<double>> ex_6_all_data = {{1.0, 2.0, 101.0, 102.0}, {11.0, 12.0, 111.0, 112.0}};
    std::vector<std::string> ex_6_timestamps = {"2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z"};
    std::vector<std::time_t> ex_6_timestamps_seconds = {1735707600, 1735711200};
    size_t ex_6_num_time_steps = 2;

    // Example 7: a single-instance nexus set whose IDs differ in length to exercise the dynamically-sized
    // feature_id string-length dimension. The stored width must equal the longest ID, and every shorter ID
    // must round-trip exactly (null-padded), not truncated.
    std::shared_ptr<std::vector<std::string>> ex_7_form_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>{"form-0"});
    std::vector<std::string> ex_7_form_0_all_nexus_id = {
        "nx-2", "nex-1", "cat-1000012", "a-very-long-nexus-identifier-000000042"
    };
    std::vector<std::vector<double>> ex_7_all_data = {{1.0, 2.0, 3.0, 4.0}, {11.0, 12.0, 13.0, 14.0}};
    std::vector<std::string> ex_7_timestamps = {"2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z"};
    std::vector<std::time_t> ex_7_timestamps_seconds = {1735707600, 1735711200};

    // Example 8: differing per-rank ID lengths across the MPI write. Rank 0 owns only short IDs while rank 1
    // owns a much longer ID, so the feature_id string-length dimension is correct only if the maximum ID
    // length is reduced across ranks rather than taken from the (rank 0) writing rank's local IDs.
    std::shared_ptr<std::vector<std::string>> ex_8_form_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>{"form-0"});
    std::vector<std::string> ex_8_form_0_group_a_nexus_ids = {"nex-1", "nex-2"};                                 // rank 0 (local max 5)
    std::vector<std::string> ex_8_form_0_group_b_nexus_ids = {"terminal-nexus-1234567890123", "tnx-2"};          // rank 1 (local max 28)
    std::vector<std::string> ex_8_form_0_all_nexus_id = {"nex-1", "nex-2", "terminal-nexus-1234567890123", "tnx-2"};
    std::vector<std::vector<double>> ex_8_group_a_data = {{1.0, 2.0}, {11.0, 12.0}};
    std::vector<std::vector<double>> ex_8_group_b_data = {{101.0, 102.0}, {111.0, 112.0}};
    std::vector<std::vector<double>> ex_8_all_data = {{1.0, 2.0, 101.0, 102.0}, {11.0, 12.0, 111.0, 112.0}};
    std::vector<std::string> ex_8_timestamps = {"2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z"};
    std::vector<std::time_t> ex_8_timestamps_seconds = {1735707600, 1735711200};
    size_t ex_8_num_time_steps = 2;

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

    // Generate the IDs and data used for example 3 (2 time steps)
    ex_3_form_0_all_nexus_id.resize(ex_3_nexus_count);
    ex_3_form_0_group_b_nexus_ids.resize(ex_3_nexus_count / 2);
    ex_3_form_0_group_a_nexus_ids.resize(ex_3_nexus_count - ex_3_form_0_group_b_nexus_ids.size());

    ex_3_group_a_data = {std::vector<double>(ex_3_form_0_group_a_nexus_ids.size()), std::vector<double>(ex_3_form_0_group_a_nexus_ids.size())};
    ex_3_group_b_data = {std::vector<double>(ex_3_form_0_group_b_nexus_ids.size()), std::vector<double>(ex_3_form_0_group_b_nexus_ids.size())};
    ex_3_all_data = {std::vector<double>(ex_3_form_0_all_nexus_id.size()), std::vector<double>(ex_3_form_0_all_nexus_id.size())};

    size_t a_idx = 0, b_idx = 0;
    for (size_t n = 0; n < ex_3_nexus_count; ++n) {
        bool is_group_a = n % 2 == 0;
        std:: string nid = "nex-" + std::to_string(n+1);
        double data_ts_0 = (n+1)/1000.0;
        double data_ts_1 = (data_ts_0) * 2.0;

        // Add ID and data to appropriate "all" data structures
        ex_3_form_0_all_nexus_id[n] = nid;
        ex_3_all_data[0][n] = data_ts_0;
        ex_3_all_data[1][n] = data_ts_1;

        // Add ID and data to appropriate group data structures
        if (is_group_a) {
            ex_3_form_0_group_a_nexus_ids[a_idx] = nid;
            ex_3_group_a_data[0][a_idx] = data_ts_0;
            ex_3_group_a_data[1][a_idx] = data_ts_1;
            a_idx++;
        }
        else {
            ex_3_form_0_group_b_nexus_ids[b_idx] = nid;
            ex_3_group_b_data[0][b_idx] = data_ts_0;
            ex_3_group_a_data[1][b_idx] = data_ts_1;
            b_idx++;
        }
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

    // Pick the right nexus ID and data group for this rank
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
    std::vector<int> local_offsets = {0, nexus_per_rank[0]};
    utils::PerFormulationNexusOutputMgr mgr(*nexus_ids, test_form_names, output_root, ex_2_num_time_steps, rank, local_offsets[rank], 2, ex_2_form_0_all_nexus_id.size());

    if (rank == 0) {
        // Make sure we know what files to clean up
        std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
        for (const std::string& f : *filenames) {
            files_to_cleanup.push_back(f);
        }
    }

    mgr.close();
    MPI_Barrier(MPI_COMM_WORLD);

    ASSERT_TRUE(utils::FileChecker::file_can_be_written(mgr.get_filenames()->at(0)));

    MPI_Barrier(MPI_COMM_WORLD);
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

    // Pick the right nexus ID and data group for this rank
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
    std::vector<int> local_offsets = {0, nexus_per_rank[0]};
    utils::PerFormulationNexusOutputMgr mgr(*nexus_ids, ex_2_form_names, output_root, ex_2_num_time_steps, rank, local_offsets[rank], 2, ex_2_form_0_all_nexus_id.size());

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

    mgr.close();

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
            ASSERT_EQ(values[n][t], ex_2_all_data[t][n]) << "On timestep " << t << " for nexus ID " << nexus_ids->at(n)
                << " value was " << values[n][t] << " but expected was " << ex_2_all_data[t][n] << "; \n"
                << "Full timestep data for this timestep is: \n" << values << "\n ***** \n";
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

/** Test example 2 writes feature IDs correctly using multiple simultaneous writes via MPI. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_2_b)
{
    MPI_Barrier(MPI_COMM_WORLD);

    // Expect only two ranks
    ASSERT_LE(rank, 1);
    ASSERT_EQ(size, 2);

    // Pick the right nexus ID and data group for this rank
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
    std::vector<int> local_offsets = {0, nexus_per_rank[0]};
    utils::PerFormulationNexusOutputMgr mgr(*nexus_ids, ex_2_form_names, output_root, ex_2_num_time_steps, rank, local_offsets[rank], 2, ex_2_form_0_all_nexus_id.size());

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
    mgr.close();

    // When done writing, another barrier before any checks/asserts
    MPI_Barrier(MPI_COMM_WORLD);

    // Now, compare feature ID variable data from both

    // Should only be one filename
    const netCDF::NcFile ncf(mgr.get_filenames()->at(0), netCDF::NcFile::read);
    const netCDF::NcVar nc_var_nex_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr));

    ASSERT_FALSE(nc_var_nex_ids.isNull());
    ASSERT_EQ(nc_var_nex_ids.getDim(0).getSize(), ex_2_form_0_all_nexus_id.size());
    // feature_id is now a 2-D fixed-width char variable: nexus dimension then string-length dimension.
    ASSERT_EQ(nc_var_nex_ids.getDim(1).getSize(), longest_id_length(ex_2_form_0_all_nexus_id));

    std::vector<std::string> nex_id_strs = read_feature_id_strings(nc_var_nex_ids, ex_2_form_0_all_nexus_id.size());
    ASSERT_EQ(nex_id_strs, ex_2_form_0_all_nexus_id);

    MPI_Barrier(MPI_COMM_WORLD);
}

/**
 * Test the MPI gather-to-root write path for a nexus feature ID collision.
 *
 * The colliding pair nex-1 / tnx-1 is split across ranks (rank 0 owns the regular "nex-" IDs, rank 1 owns
 * the terminal "tnx-" IDs), so a correct single output file requires the fixed-width MPI_CHAR ID gather to
 * assemble both ranks' IDs — the in-process single-instance write alone could never reproduce this. Asserts
 * every rank's ID is present as a distinct full string in the correct global (rank-contiguous) order, that
 * the colliding pair occupies distinct rows, and that each row's flow data matches what was sent for that
 * specific ID, so a cross-rank row mix-up would be caught.
 */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_6_a)
{
    MPI_Barrier(MPI_COMM_WORLD);

    // Expect exactly two ranks (the colliding pair is split one ID per rank)
    ASSERT_LE(rank, 1);
    ASSERT_EQ(size, 2);

    // Pick the right nexus ID and data group for this rank
    std::vector<std::string> *nexus_ids;
    std::vector<std::vector<double>> *group_data;
    if (rank == 0) {
        nexus_ids = &ex_6_form_0_group_a_nexus_ids;
        group_data = &ex_6_group_a_data;
    }
    else {
        nexus_ids = &ex_6_form_0_group_b_nexus_ids;
        group_data = &ex_6_group_b_data;
    }

    // Create manager instance for this rank
    const std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_6_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_6_form_0_group_b_nexus_ids.size())
    };
    std::vector<int> local_offsets = {0, nexus_per_rank[0]};
    utils::PerFormulationNexusOutputMgr mgr(*nexus_ids, ex_6_form_names, output_root, ex_6_num_time_steps, rank, local_offsets[rank], 2, ex_6_form_0_all_nexus_id.size());

    // Add to files_to_clean_up, but only for rank 0 to deal with (they should be the same sets of files)
    if (rank == 0) {
        std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
        for (const std::string& f : *filenames) {
            files_to_cleanup.push_back(f);
        }
    }

    // Write for this rank's nexuses over both time steps
    for (size_t t = 0; t < ex_6_timestamps.size(); ++t) {
        for (size_t n = 0; n < nexus_ids->size(); ++n) {
            mgr.receive_data_entry(ex_6_form_names->at(0),
                                   nexus_ids->at(n),
                                   utils::time_marker(t, ex_6_timestamps_seconds[t], ex_6_timestamps[t]),
                                   group_data->at(t)[n]);
        }
        mgr.commit_writes();
    }

    mgr.close();

    // When done writing everything, another barrier before any checks/asserts
    MPI_Barrier(MPI_COMM_WORLD);

    // Read back the single assembled file and verify IDs and flow together
    const netCDF::NcFile ncf(mgr.get_filenames()->at(0), netCDF::NcFile::read);

    // feature_id: every rank's ID present as a distinct, exact full string in the correct global order
    const netCDF::NcVar nc_var_nex_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr));
    ASSERT_FALSE(nc_var_nex_ids.isNull());
    ASSERT_EQ(nc_var_nex_ids.getDim(0).getSize(), ex_6_form_0_all_nexus_id.size());
    // feature_id is a 2-D fixed-width char variable: nexus dimension then string-length dimension.
    ASSERT_EQ(nc_var_nex_ids.getDim(1).getSize(), longest_id_length(ex_6_form_0_all_nexus_id));

    std::vector<std::string> nex_id_strs = read_feature_id_strings(nc_var_nex_ids, ex_6_form_0_all_nexus_id.size());
    ASSERT_EQ(nex_id_strs, ex_6_form_0_all_nexus_id);

    // The colliding pair, gathered from two different ranks, must occupy distinct rows (the core of the bug).
    auto nex1_it = std::find(nex_id_strs.begin(), nex_id_strs.end(), "nex-1");
    auto tnx1_it = std::find(nex_id_strs.begin(), nex_id_strs.end(), "tnx-1");
    ASSERT_NE(nex1_it, nex_id_strs.end());
    ASSERT_NE(tnx1_it, nex_id_strs.end());
    ASSERT_NE(nex1_it, tnx1_it) << "nex-1 and tnx-1 must occupy distinct rows, not a single collapsed ID";

    // Each row's flow data must match what was sent for that specific ID, so the distinct IDs are not merely
    // labels on data swapped/merged between ranks during the gather.
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr));
    ASSERT_FALSE(flow.isNull());
    ASSERT_EQ(flow.getDim(0).getSize(), ex_6_form_0_all_nexus_id.size());
    ASSERT_EQ(flow.getDim(1).getSize(), ex_6_num_time_steps);
    // Note that nexus feature_id dim comes before time dim, so have to order this way
    double values[4][2];
    flow.getVar(values);
    for (size_t t = 0; t < ex_6_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_6_form_0_all_nexus_id.size(); ++n) {
            ASSERT_EQ(values[n][t], ex_6_all_data[t][n])
                << "Flow mismatch for nexus ID " << nex_id_strs[n] << " at time step " << t;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

/**
 * Test cross-rank sizing of the feature_id string-length dimension when ranks own IDs of differing lengths.
 *
 * Rank 0 owns only short IDs and rank 1 owns a much longer ID, so the single assembled output file is correct
 * only if the maximum ID length is reduced across ranks (MPI_Allreduce) rather than taken from the writing
 * rank's local IDs. Asserts the stored width equals the global longest ID length, that both ranks' managers
 * report that same global width (so the reduction reached every rank, including the shorter-ID rank 0 that
 * sizes the dimension), that rank 0's short IDs round-trip exactly at the larger global width, and that all
 * IDs and their flow data land in the correct rows.
 */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_8_a)
{
    MPI_Barrier(MPI_COMM_WORLD);

    ASSERT_LE(rank, 1);
    ASSERT_EQ(size, 2);

    std::vector<std::string> *nexus_ids;
    std::vector<std::vector<double>> *group_data;
    if (rank == 0) {
        nexus_ids = &ex_8_form_0_group_a_nexus_ids;
        group_data = &ex_8_group_a_data;
    }
    else {
        nexus_ids = &ex_8_form_0_group_b_nexus_ids;
        group_data = &ex_8_group_b_data;
    }

    // The global longest ID (owned by rank 1) must exceed rank 0's local longest, so a rank-local width would
    // be wrong for the assembled file.
    const size_t global_width = longest_id_length(ex_8_form_0_all_nexus_id);
    ASSERT_GT(global_width, longest_id_length(ex_8_form_0_group_a_nexus_ids));

    const std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_8_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_8_form_0_group_b_nexus_ids.size())
    };
    std::vector<int> local_offsets = {0, nexus_per_rank[0]};
    utils::PerFormulationNexusOutputMgr mgr(*nexus_ids, ex_8_form_names, output_root, ex_8_num_time_steps, rank, local_offsets[rank], 2, ex_8_form_0_all_nexus_id.size());

    if (rank == 0) {
        std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
        for (const std::string& f : *filenames) {
            files_to_cleanup.push_back(f);
        }
    }

    // Every rank must have reduced to the same global width, not its own local maximum.
    ASSERT_EQ(friend_nexus_id_string_width(&mgr), global_width);

    for (size_t t = 0; t < ex_8_timestamps.size(); ++t) {
        for (size_t n = 0; n < nexus_ids->size(); ++n) {
            mgr.receive_data_entry(ex_8_form_names->at(0),
                                   nexus_ids->at(n),
                                   utils::time_marker(t, ex_8_timestamps_seconds[t], ex_8_timestamps[t]),
                                   group_data->at(t)[n]);
        }
        mgr.commit_writes();
    }

    mgr.close();

    MPI_Barrier(MPI_COMM_WORLD);

    const netCDF::NcFile ncf(mgr.get_filenames()->at(0), netCDF::NcFile::read);

    const netCDF::NcVar nc_var_nex_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr));
    ASSERT_FALSE(nc_var_nex_ids.isNull());
    ASSERT_EQ(nc_var_nex_ids.getDim(0).getSize(), ex_8_form_0_all_nexus_id.size());
    // String-length dimension sized to the global longest ID, verified independently of the manager's width.
    ASSERT_EQ(nc_var_nex_ids.getDim(1).getSize(), global_width);

    std::vector<std::string> nex_id_strs = read_feature_id_strings(nc_var_nex_ids, ex_8_form_0_all_nexus_id.size());
    ASSERT_EQ(nex_id_strs, ex_8_form_0_all_nexus_id);

    // Rank 0's short IDs must survive verbatim at the (larger) global width, i.e. correctly null-padded.
    ASSERT_NE(std::find(nex_id_strs.begin(), nex_id_strs.end(), "nex-1"), nex_id_strs.end());
    ASSERT_NE(std::find(nex_id_strs.begin(), nex_id_strs.end(), "terminal-nexus-1234567890123"), nex_id_strs.end());

    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr));
    ASSERT_FALSE(flow.isNull());
    ASSERT_EQ(flow.getDim(0).getSize(), ex_8_form_0_all_nexus_id.size());
    ASSERT_EQ(flow.getDim(1).getSize(), ex_8_num_time_steps);
    // Note that nexus feature_id dim comes before time dim, so have to order this way
    double values[4][2];
    flow.getVar(values);
    for (size_t t = 0; t < ex_8_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_8_form_0_all_nexus_id.size(); ++n) {
            ASSERT_EQ(values[n][t], ex_8_all_data[t][n])
                << "Flow mismatch for nexus ID " << nex_id_strs[n] << " at time step " << t;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
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

/** Test correct chunking setup for example 0 (single instance). */
TEST_F(PerFormulationNexusOutputMgr_Test, construct_0_c) {

    std::string form_name = ex_0_form_names->at(0);

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    // TODO: address this better one this is no longer a static, hard-coded setting
    ASSERT_TRUE(friend_is_chunking(&mgr));

    netCDF::NcVar::ChunkMode chunk_mode;
    std::vector<size_t> size_vector;

    // Should only be one filename
    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr));

    ASSERT_FALSE(flow.isNull());
    flow.getChunkingParameters(chunk_mode, size_vector);
    ASSERT_EQ(size_vector.size(), 2);
    ASSERT_EQ(size_vector[0], ex_0_form_0_nexus_ids.size());
    ASSERT_EQ(size_vector[1], 1);
}

/** Test that example 0 gets constructed and is immediately not marked as closed. */
TEST_F(PerFormulationNexusOutputMgr_Test, construct_0_d)
{
    std::string form_name = ex_0_form_0_nexus_ids[0];

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    ASSERT_FALSE(mgr.is_closed());
}

/** Test correct chunking setup for example 3 (single instance). */
TEST_F(PerFormulationNexusOutputMgr_Test, construct_3_c) {

    std::string form_name = ex_3_form_names->at(0);

    utils::PerFormulationNexusOutputMgr mgr(ex_3_form_0_all_nexus_id, ex_3_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    // TODO: address this better one this is no longer a static, hard-coded setting
    ASSERT_TRUE(friend_is_chunking(&mgr));

    netCDF::NcVar::ChunkMode chunk_mode;
    std::vector<size_t> size_vector;

    // Should only be one filename
    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr));

    ASSERT_FALSE(flow.isNull());
    flow.getChunkingParameters(chunk_mode, size_vector);
    ASSERT_EQ(size_vector.size(), 2);
    ASSERT_EQ(size_vector[0], ex_3_nexus_count);
    ASSERT_EQ(size_vector[1], 1);
}

/** Test correct chunking setup for example 3 (multiple instance). */
TEST_F(PerFormulationNexusOutputMgr_Test, construct_3_d) {

    std::string form_name = ex_3_form_names->at(0);

    std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_3_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_3_form_0_group_b_nexus_ids.size())
    };

    //utils::PerFormulationNexusOutputMgr mgr(ex_3_form_0_all_nexus_id, ex_3_form_names, output_root, 2);

    utils::PerFormulationNexusOutputMgr mgr_a(ex_3_form_0_group_a_nexus_ids, ex_3_form_names, output_root, 2, 0, 0, 2, ex_3_form_0_all_nexus_id.size());
    utils::PerFormulationNexusOutputMgr mgr_b(ex_3_form_0_group_b_nexus_ids, ex_3_form_names, output_root, 2, 1, ex_3_form_0_all_nexus_id.size(), 2, ex_3_form_0_all_nexus_id.size());

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr_a.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    // TODO: address this better one this is no longer a static, hard-coded setting
    //if (friend_is_chunking(&mgr)) {
    if (true) {
        netCDF::NcVar::ChunkMode chunk_mode;
        std::vector<size_t> size_vector;

        // Should only be one filename
        const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
        const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr_a));

        ASSERT_FALSE(flow.isNull());
        flow.getChunkingParameters(chunk_mode, size_vector);
        ASSERT_EQ(size_vector.size(), 2);
        ASSERT_EQ(size_vector[0], ex_3_nexus_count);
        ASSERT_EQ(size_vector[1], 1);
    }
}

/** Test that example 0 gets constructed and has expected nexus output file. */
TEST_F(PerFormulationNexusOutputMgr_Test, nexus_out_file_0_a)
{
    std::string form_name = ex_0_form_0_nexus_ids[0];

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    std::string nexus_outfile = friend_get_nexus_outfile(&mgr);
    std::string expected = output_root + "/formulation_" + ex_0_form_names->at(0) + "_nexuses.nc";
    ASSERT_EQ(nexus_outfile, expected);
}

/** Test that current formulation ID is as expected after receiving data entry. */
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
    double values[4][2];
    flow.getVar(values);
    for (size_t i = 0; i < ex_0_form_0_nexus_ids.size(); ++i) {
        ASSERT_NE(values[i][0], ex_0_data[0][i]);
    }

    mgr.commit_writes();

    flow.getVar(values);
    for (size_t i = 0; i < ex_0_form_0_nexus_ids.size(); ++i) {
        ASSERT_EQ(values[i][0], ex_0_data[0][i]);
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
// TODO: (later) for now, commit_writes has been updated to always just write fill values for anything missing
// TODO: (later) if that's changed later, perhaps to be configurable, modify and re-add this test (and others)
TEST_F(PerFormulationNexusOutputMgr_Test, DISABLED_commit_writes_0_c) {

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

/** Test that example 0 has the nexus ID var values written to the NetCDF file. */
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
    // feature_id is now a 2-D fixed-width char variable: nexus dimension then string-length dimension.
    ASSERT_EQ(nexus_ids.getDim(1).getSize(), longest_id_length(ex_0_form_0_nexus_ids));

    std::vector<std::string> nex_id_strs = read_feature_id_strings(nexus_ids, ex_0_form_0_nexus_ids.size());

    ASSERT_EQ(nex_id_strs, ex_0_form_0_nexus_ids);
}

/** Make sure writes work example 1 (multiple instances) for multiple time steps, alternating writes. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_1_a) {

    std::string form_name = ex_1_form_names->at(0);


    std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_1_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_1_form_0_group_b_nexus_ids.size())
    };

    utils::PerFormulationNexusOutputMgr mgr_a(ex_1_form_0_group_a_nexus_ids, ex_1_form_names, output_root, 2, 0, 0, 2, ex_1_form_0_all_nexus_id.size());
    utils::PerFormulationNexusOutputMgr mgr_b(ex_1_form_0_group_b_nexus_ids, ex_1_form_names, output_root, 2, 1, ex_1_form_0_group_a_nexus_ids.size(), 2, ex_1_form_0_all_nexus_id.size());

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

    utils::PerFormulationNexusOutputMgr mgr_a(ex_1_form_0_group_a_nexus_ids, ex_1_form_names, output_root, 2, 0, 0, 2, ex_1_form_0_all_nexus_id.size());
    utils::PerFormulationNexusOutputMgr mgr_b(ex_1_form_0_group_b_nexus_ids, ex_1_form_names, output_root, 2, 1, ex_1_form_0_group_a_nexus_ids.size(), 2, ex_1_form_0_all_nexus_id.size());

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

/** Test that example 1 has the nexus ID var values written to the NetCDF file with multiple instances. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_1_c)
{
    std::string form_name = ex_1_form_names->at(0);

    std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_1_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_1_form_0_group_b_nexus_ids.size())
    };

    utils::PerFormulationNexusOutputMgr mgr_a(ex_1_form_0_group_a_nexus_ids, ex_1_form_names, output_root, 2, 0, 0, 2, ex_1_form_0_all_nexus_id.size());
    utils::PerFormulationNexusOutputMgr mgr_b(ex_1_form_0_group_b_nexus_ids, ex_1_form_names, output_root, 2, 1, ex_1_form_0_group_a_nexus_ids.size(), 2, ex_1_form_0_all_nexus_id.size());

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
    // feature_id is now a 2-D fixed-width char variable: nexus dimension then string-length dimension.
    ASSERT_EQ(nexus_ids.getDim(1).getSize(), longest_id_length(ex_1_form_0_all_nexus_id));

    std::vector<std::string> nex_id_strs = read_feature_id_strings(nexus_ids, ex_1_form_0_all_nexus_id.size());

    ASSERT_EQ(nex_id_strs, ex_1_form_0_all_nexus_id);
}

/**
 * Regression test for a nexus feature ID (numeric) collision (single instance, end-to-end).
 *
 * The nexus ID set contains the colliding pair nex-1 and tnx-1, which share the numeric suffix 1. Under the
 * old integer feature_id schema both collapsed to feature_id == 1, conflating two physically distinct
 * nexuses; with the full-string ID schema they must read back as two distinct rows ("nex-1" and "tnx-1"),
 * each carrying its own flow data. tnx-2/nex-2 are included so the collision is not the only ID present.
 */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_5_a)
{
    std::string form_name = ex_5_form_names->at(0);

    utils::PerFormulationNexusOutputMgr mgr(ex_5_form_0_all_nexus_id, ex_5_form_names, output_root, ex_5_timestamps.size());

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    for (size_t t = 0; t < ex_5_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_5_form_0_all_nexus_id.size(); ++n) {
            mgr.receive_data_entry(form_name,
                                   ex_5_form_0_all_nexus_id[n],
                                   utils::time_marker(t, ex_5_timestamps_seconds[t], ex_5_timestamps[t]),
                                   ex_5_all_data[t][n]);
        }
        mgr.commit_writes();
    }

    // Should only be one filename
    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);

    // The feature_id char variable must contain every ID as a distinct, exact full string (prefix included).
    const netCDF::NcVar nexus_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr));
    ASSERT_FALSE(nexus_ids.isNull());
    ASSERT_EQ(nexus_ids.getDim(0).getSize(), ex_5_form_0_all_nexus_id.size());
    // feature_id is a 2-D fixed-width char variable: nexus dimension then string-length dimension.
    ASSERT_EQ(nexus_ids.getDim(1).getSize(), longest_id_length(ex_5_form_0_all_nexus_id));

    std::vector<std::string> nex_id_strs = read_feature_id_strings(nexus_ids, ex_5_form_0_all_nexus_id.size());
    ASSERT_EQ(nex_id_strs, ex_5_form_0_all_nexus_id);

    // Explicitly confirm the colliding pair is present as two distinct rows (the core of the bug).
    auto nex1_it = std::find(nex_id_strs.begin(), nex_id_strs.end(), "nex-1");
    auto tnx1_it = std::find(nex_id_strs.begin(), nex_id_strs.end(), "tnx-1");
    ASSERT_NE(nex1_it, nex_id_strs.end());
    ASSERT_NE(tnx1_it, nex_id_strs.end());
    ASSERT_NE(nex1_it, tnx1_it) << "nex-1 and tnx-1 must occupy distinct rows, not a single collapsed ID";

    // Each row's flow data must match what was sent for that specific ID across both time steps, so the
    // distinct IDs are not merely labels on swapped/merged data.
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr));
    ASSERT_FALSE(flow.isNull());
    // Note that nexus feature_id dim comes before time dim, so have to order this way
    double values[4][2];
    flow.getVar(values);
    for (size_t t = 0; t < ex_5_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_5_form_0_all_nexus_id.size(); ++n) {
            ASSERT_EQ(values[n][t], ex_5_all_data[t][n])
                << "Flow mismatch for nexus ID " << nex_id_strs[n] << " at time step " << t;
        }
    }
}

/**
 * Test that a single-instance nexus set with differing ID lengths (including one longer than the former
 * fixed width of 32) is written with a feature_id string-length dimension sized to the longest ID, and that
 * every shorter ID round-trips exactly with correct null padding rather than being truncated or over-padded.
 */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_7_a)
{
    std::string form_name = ex_7_form_names->at(0);

    // Guard the premise: the set really mixes lengths and its longest ID exceeds the former fixed width.
    const size_t expected_width = longest_id_length(ex_7_form_0_all_nexus_id);
    ASSERT_GT(expected_width, 32u);
    ASSERT_GT(expected_width, ex_7_form_0_all_nexus_id.front().size());

    utils::PerFormulationNexusOutputMgr mgr(ex_7_form_0_all_nexus_id, ex_7_form_names, output_root, ex_7_timestamps.size());

    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    for (size_t t = 0; t < ex_7_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_7_form_0_all_nexus_id.size(); ++n) {
            mgr.receive_data_entry(form_name,
                                   ex_7_form_0_all_nexus_id[n],
                                   utils::time_marker(t, ex_7_timestamps_seconds[t], ex_7_timestamps[t]),
                                   ex_7_all_data[t][n]);
        }
        mgr.commit_writes();
    }

    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar nexus_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr));
    ASSERT_FALSE(nexus_ids.isNull());
    ASSERT_EQ(nexus_ids.getDim(0).getSize(), ex_7_form_0_all_nexus_id.size());
    // String-length dimension sized to the longest ID, verified independently of the manager's own width.
    ASSERT_EQ(nexus_ids.getDim(1).getSize(), expected_width);
    ASSERT_EQ(friend_nexus_id_string_width(&mgr), expected_width);

    std::vector<std::string> nex_id_strs = read_feature_id_strings(nexus_ids, ex_7_form_0_all_nexus_id.size());
    ASSERT_EQ(nex_id_strs, ex_7_form_0_all_nexus_id);
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
            ASSERT_EQ(values[n][t], ex_2_all_data[t][n]) << "On timestep " << t << " for nexus ID " << nexus_ids->at(n)
                << " value was " << values[n][t] << " but expected was " << ex_2_all_data[t][n] << "; \n"
                << "Full timestep data for this timestep is: \n" << values << "\n ***** \n";
        }
    }
}

/** Make sure writes work and scale for example 3 (single instance) for multiple time steps. */
TEST_F(PerFormulationNexusOutputMgr_Test, commit_writes_3_b) {

    std::string form_name = ex_3_form_names->at(0);

    utils::PerFormulationNexusOutputMgr mgr(ex_3_form_0_all_nexus_id, ex_3_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }



    for (size_t t = 0; t < ex_3_timestamps.size(); ++t) {
        for (int n = 0; n < ex_3_form_0_all_nexus_id.size(); ++n) {
            mgr.receive_data_entry(form_name,
                                   ex_3_form_0_all_nexus_id[n],
                                   utils::time_marker(t, ex_3_timestamps_seconds[t], ex_3_timestamps[t]),
                                   ex_3_all_data[t][n]);
        }
        mgr.commit_writes();
    }

    // Should only be one filename
    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar flow = ncf.getVar(friend_get_nc_flow_var_name(&mgr));

    ASSERT_FALSE(flow.isNull());
    // Note that nexus feature_id dim comes before time dim, so have to order this way
    double values[LARGE_NEXUS_EXAMPLE_SIZE][2];
    flow.getVar(values);
    for (size_t t = 0; t < ex_3_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_3_form_0_all_nexus_id.size(); ++n) {
            ASSERT_EQ(values[n][t], ex_3_all_data[t][n]);
        }
    }

}

/** Test that close works automatically after all time steps for example 0. */
TEST_F(PerFormulationNexusOutputMgr_Test, is_closed_0_a) {

    std::string form_name = ex_0_form_names->at(0);

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root, 2);

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    for (size_t t = 0; t < ex_0_timestamps.size(); ++t) {
        for (size_t n = 0; n < ex_0_form_0_nexus_ids.size(); ++n) {
            mgr.receive_data_entry(form_name,
                                   ex_0_form_0_nexus_ids[n],
                                   utils::time_marker(t, ex_0_timestamps_seconds[t], ex_0_timestamps[t]),
                                   ex_0_data[t][n]);
        }
        ASSERT_FALSE(mgr.is_closed());
        mgr.commit_writes();
    }
    ASSERT_TRUE(mgr.is_closed());
}

/** Test that close does not work automatically if all time steps not gone through for example 0. */
TEST_F(PerFormulationNexusOutputMgr_Test, is_closed_0_b) {

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
    ASSERT_FALSE(mgr.is_closed());
    mgr.commit_writes();
    ASSERT_FALSE(mgr.is_closed());
}

/** Test that close works if called explicitly for example 0. */
TEST_F(PerFormulationNexusOutputMgr_Test, is_closed_0_c) {

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
    ASSERT_FALSE(mgr.is_closed());
    mgr.close();
    ASSERT_TRUE(mgr.is_closed());
}

/** Test that example 0 works with write_nexus_ids_once and has nexus ID var values written to NetCDF file. */
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
        friend_write_nexus_ids_once(&mgr);
    }

    const netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar nexus_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr));

    // These should all have size 4 for the current example, equal to the size of ex_0_form_0_nexus_ids
    ASSERT_EQ(nexus_ids.getDim(0).getSize(), 4);
    // feature_id is now a 2-D fixed-width char variable: nexus dimension then string-length dimension.
    ASSERT_EQ(nexus_ids.getDim(1).getSize(), longest_id_length(ex_0_form_0_nexus_ids));

    std::vector<std::string> nex_id_strs = read_feature_id_strings(nexus_ids, ex_0_form_0_nexus_ids.size());

    ASSERT_EQ(nex_id_strs, ex_0_form_0_nexus_ids);
}

/**
 * Test that example 1 works with write_nexus_ids_once and has nexus ID var values written to NetCDF file with multiple
 * instances.
 */
TEST_F(PerFormulationNexusOutputMgr_Test, write_nexus_ids_once_1_a)
{
    std::string form_name = ex_1_form_names->at(0);

    std::vector<int> nexus_per_rank = {
        static_cast<int>(ex_1_form_0_group_a_nexus_ids.size()), static_cast<int>(ex_1_form_0_group_b_nexus_ids.size())
    };

    utils::PerFormulationNexusOutputMgr mgr_a(ex_1_form_0_group_a_nexus_ids, ex_1_form_names, output_root, 2, 0, 0, 2, ex_1_form_0_all_nexus_id.size());
    utils::PerFormulationNexusOutputMgr mgr_b(ex_1_form_0_group_b_nexus_ids, ex_1_form_names, output_root, 2, 1, ex_1_form_0_group_a_nexus_ids.size(), 2, ex_1_form_0_all_nexus_id.size());

    // Make sure we know what files to clean up
    std::shared_ptr<std::vector<std::string>> filenames = mgr_a.get_filenames();
    for (const std::string& f : *filenames) {
        files_to_cleanup.push_back(f);
    }

    {
        friend_write_nexus_ids_once(&mgr_a);
        friend_write_nexus_ids_once(&mgr_b);
    }

    netCDF::NcFile ncf(filenames->at(0), netCDF::NcFile::read);
    const netCDF::NcVar nexus_ids = ncf.getVar(friend_get_nc_nex_id_dim_name(&mgr_a));

    // These should all have size 8 for the current example, equal to the size of ex_1_form_0_all_nexus_id
    ASSERT_EQ(nexus_ids.getDim(0).getSize(), 8);
    // feature_id is now a 2-D fixed-width char variable: nexus dimension then string-length dimension.
    ASSERT_EQ(nexus_ids.getDim(1).getSize(), longest_id_length(ex_1_form_0_all_nexus_id));

    std::vector<std::string> nex_id_strs = read_feature_id_strings(nexus_ids, ex_1_form_0_all_nexus_id.size());

    ASSERT_EQ(nex_id_strs, ex_1_form_0_all_nexus_id);
}

/** Test that pack_nexus_id packs a normal ID verbatim and null-pads the rest of the fixed-width buffer. */
TEST_F(PerFormulationNexusOutputMgr_Test, pack_nexus_id_a)
{
    const size_t width = 32; // pack helper is width-parameterized; exercise it at a representative width
    // Pre-fill with a non-null sentinel so the null padding is actually verified (not just left-over zeros).
    std::vector<char> buffer(width, 'X');
    const std::string id = "tnx-1";

    friend_pack_nexus_id(id, buffer.data(), width);

    // The ID bytes are copied verbatim, prefix included.
    for (size_t i = 0; i < id.size(); ++i) {
        ASSERT_EQ(buffer[i], id[i]) << "Mismatch at byte " << i;
    }
    // Everything after the ID is null padding.
    for (size_t i = id.size(); i < width; ++i) {
        ASSERT_EQ(buffer[i], '\0') << "Expected null padding at byte " << i;
    }
}

/** Test that pack_nexus_id handles an ID exactly the fixed width, filling every byte with no null terminator. */
TEST_F(PerFormulationNexusOutputMgr_Test, pack_nexus_id_b)
{
    const size_t width = 32; // pack helper is width-parameterized; exercise it at a representative width
    const std::string id(width, 'a'); // exactly the fixed width
    std::vector<char> buffer(width, '\0');

    friend_pack_nexus_id(id, buffer.data(), width);

    for (size_t i = 0; i < width; ++i) {
        ASSERT_EQ(buffer[i], 'a') << "Expected full-width fill at byte " << i;
    }
}

/** Test that pack_nexus_id throws when the ID is longer than the fixed width (no silent truncation). */
TEST_F(PerFormulationNexusOutputMgr_Test, pack_nexus_id_c)
{
    const size_t width = 32; // pack helper is width-parameterized; exercise it at a representative width
    const std::string id(width + 1, 'a'); // one char too long to fit
    std::vector<char> buffer(width, '\0');

    ASSERT_THROW(friend_pack_nexus_id(id, buffer.data(), width), std::runtime_error);
}

#endif // #if NGEN_MPI_UNIT_TESTS ... #else

#endif // #ifdef NGEN_NETCDF_TESTS_ACTIVE
