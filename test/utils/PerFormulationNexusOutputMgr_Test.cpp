//
// Created by Robert Bartel on 12/29/25.
//

#include "gtest/gtest.h"
#include "PerFormulationNexusOutputMgr.hpp"
#include "FileChecker.h"

// Again non-Windows specific here
#include <sys/stat.h>
#include <sys/types.h>

#include "HY_HydroNexus.hpp"

class PerFormulationNexusOutputMgrTest: public ::testing::Test {

protected:

    void SetUp() override;
    void TearDown() override;

    std::string output_root = "/tmp/PerFormulationNexusOutputMgrTest";

    std::vector<std::string> ex_0_form_0_nexus_ids = {"nex-1", "nex-2", "nex-3", "nex-4"};
    std::shared_ptr<std::vector<std::string>> ex_0_form_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>{"form-0"});
    std::vector<std::string> ex_0_timestamps = {"2025-01-01T00:00:00Z"};
    // Per timestamp, per-nexus data
    std::vector<std::vector<double>> ex_0_data = {{1.0, 2.0, 3.0, 4.0}, {11.0, 12.0, 13.0, 14.0}};

};

void PerFormulationNexusOutputMgrTest::SetUp()
{
    Test::SetUp();

    if (utils::FileChecker::directory_exists(output_root)) {
        throw std::runtime_error("Output root directory '" + output_root + "' already exists");
    }
    if (mkdir(output_root.c_str(), 0777) != 0) {
        throw std::runtime_error("Failed to create output root directory '" + output_root + "'");
    }

    // TODO: have setup make sure no file(s) present where tests will produce them
}

void PerFormulationNexusOutputMgrTest::TearDown()
{
    Test::TearDown();
    // TODO: have tearDown clean up produced file(s)
    if (utils::FileChecker::directory_exists(output_root)) {
        rmdir(output_root.c_str());
    }
}

// TODO: (though elsewhere) test that the right type of mgr class is created per config settings
// TODO: (though elsewhere) test that things work in the CSV version also
// TODO: (though elsewhere) test that things only work if global formulation is exclusively used

// TODO: test that receive_data_entry works but does not write anything


TEST_F(PerFormulationNexusOutputMgrTest, receive_data_entry_0_a) {

    int nex_id_index = 0;
    std::string form_name = ex_0_form_0_nexus_ids[0];
    int time_index = 0;

    utils::PerFormulationNexusOutputMgr mgr(ex_0_form_0_nexus_ids, ex_0_form_names, output_root);
    mgr.receive_data_entry(form_name, ex_0_form_0_nexus_ids[nex_id_index], time_index, ex_0_timestamps[time_index], ex_0_data[time_index][nex_id_index]);

}


// TODO: test that commit_writes actually writes things

// TODO: test that commit_writes writes everything for multiple time steps

// TODO: test that commit_writes writes everything for multiple nexuses

// TODO: test that commit_writes doesn't work unless we've received data for all nexuses for a time step

// TODO: test that receive_data_entry doesn't work if we get data for different time steps

// TODO: (maybe) test that receive_data_entry and commit_writes keep things separate and write separate files for different formulations

// TODO: test that multiple instances can write to the same file successfully

// TODO: test that output format is as expected



