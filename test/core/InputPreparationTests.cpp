#include <gtest/gtest.h>

#include <CommandLine.hpp>
#include <InputPreparation.hpp>

#include <filesystem>
#include <fstream>
#include <string>

//! Fixture that lays down a temporary directory of readable input files; tests
//! remove or add files to exercise inputs_are_readable().
class InputPreparation_Test : public ::testing::Test {
protected:
    std::filesystem::path dir;
    std::string catchment;
    std::string nexus;
    std::string realization;

    void SetUp() override {
        dir = std::filesystem::temp_directory_path() / "ngen_input_preparation_test";
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
        catchment   = (dir / "catchment.geojson").string();
        nexus       = (dir / "nexus.geojson").string();
        realization = (dir / "realization.json").string();
        write_file(catchment);
        write_file(nexus);
        write_file(realization);
    }

    void TearDown() override {
        std::filesystem::remove_all(dir);
    }

    static void write_file(const std::string& path) {
        std::ofstream out(path);
        out << "{}";
    }

    ngen::driver::CommandLine command_line() const {
        ngen::driver::CommandLine cli;
        cli.catchment_data_path = catchment;
        cli.nexus_data_path = nexus;
        cli.realization_config_path = realization;
        return cli;
    }
};

TEST_F(InputPreparation_Test, AllPresentInputsAreReadable)
{
    EXPECT_TRUE(ngen::driver::inputs_are_readable(command_line()));
}

TEST_F(InputPreparation_Test, MissingCatchmentIsNotReadable)
{
    std::filesystem::remove(catchment);
    EXPECT_FALSE(ngen::driver::inputs_are_readable(command_line()));
}

TEST_F(InputPreparation_Test, MissingNexusIsNotReadable)
{
    std::filesystem::remove(nexus);
    EXPECT_FALSE(ngen::driver::inputs_are_readable(command_line()));
}

TEST_F(InputPreparation_Test, MissingRealizationIsNotReadable)
{
    std::filesystem::remove(realization);
    EXPECT_FALSE(ngen::driver::inputs_are_readable(command_line()));
}

TEST_F(InputPreparation_Test, MissingPartitionConfigIsNotReadable)
{
    ngen::driver::CommandLine cli = command_line();
    cli.partition_path = (dir / "no_such_partition.json").string();
    EXPECT_FALSE(ngen::driver::inputs_are_readable(cli));
}

TEST_F(InputPreparation_Test, PresentPartitionConfigIsReadable)
{
    ngen::driver::CommandLine cli = command_line();
    const std::string partition = (dir / "partition.json").string();
    write_file(partition);
    cli.partition_path = partition;
    EXPECT_TRUE(ngen::driver::inputs_are_readable(cli));
}
