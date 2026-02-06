#include "gtest/gtest.h"
#include <utilities/parallel_utils.h>

#include "realizations/coastal/SchismFormulation.hpp"
#include <cmath>
#include <limits>
#include <iostream>
#include <NetCDFMeshPointsDataProvider.hpp>

const static std::string library_path = "/mnt/BUILD/schism_2025-07-02-linux/lib/libschism_bmi.so";
const static std::string init_config_path = "/mnt/SCHISM_Lake_Champlain_BMI_Driver_Test/namelist.input";
const static std::string met_forcing_netcdf_path = "/mnt/SCHISM_Lake_Champlain_BMI_test/NextGen_Forcings_Engine_MESH_output_201104302300.nc";

#if 0
struct Schism_Formulation_IT : public ::testing::Test
{

};
#endif

std::map<std::string, double> input_variables_defaults =
    {
        /* Meteorological Forcings */
        // RAINRATE - precipitation
        {"RAINRATE", 0.01},
        // SFCPRS - surface atmospheric pressure
        {"PSFC", 101325.0},
        // SPFH2m - specific humidity at 2m
        {"Q2D", 0.01},
        // TMP2m - temperature at 2m
        {"T2D", 293},
        // UU10m, VV10m - wind velocity components at 10m
        {"U2D", 1.0},
        {"V2D", 1.0},

        /* Input Boundary Conditions */
        // ETA2_bnd - water surface elevation at the boundaries
        {"ETA2_bnd", 30},
        // Q_bnd - flows at boundaries
        {"Q_bnd_source", 0.1},
        // Q_bnd - flows at boundaries
        {"Q_bnd_sink", 0.1},
    };

struct MockProvider : data_access::DataProvider<double, MeshPointsSelector>
{
    std::vector<double> data;

    MockProvider()
        : data(552697, 0.0)
    {}
    ~MockProvider() = default;

    // Implementation of DataProvider
    std::vector<std::string> variables;
    boost::span<const std::string> get_available_variable_names() const override { return variables; }

    long get_data_start_time() const override { return 0; }
    long get_data_stop_time() const override { return 0; }
    long record_duration() const override { return 3600; }
    size_t get_ts_index_for_time(const time_t &epoch_time) const override { return 1; }

    data_type get_value(const selection_type& selector, data_access::ReSampleMethod m) override { return data[0]; }
    std::vector<data_type> get_values(const selection_type& selector, data_access::ReSampleMethod m) override { throw ""; return data; }
    void get_values(const selection_type& selector, boost::span<double> out_data) override
    {
        auto default_value = input_variables_defaults[selector.variable_name];
        for (auto& val : out_data) {
            val = default_value;
        }
    }
};

void test_netcdf_met_provider(std::shared_ptr<data_access::NetCDFMeshPointsDataProvider> provider)
{

    return;
    auto available_variables = provider->get_available_variable_names();
    for (auto const& expected : SchismFormulation::expected_input_variables_) {
        SchismFormulation::InputMapping const& mapping = expected.second;
        if (mapping.selector == SchismFormulation::METEO) {
            auto pos = std::find(available_variables.begin(), available_variables.end(), mapping.name);
            EXPECT_NE(pos, available_variables.end());
        }
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    auto provider = std::make_shared<MockProvider>();

    std::tm start_time_tm{};
    start_time_tm.tm_year = 2011 - 1900;
    start_time_tm.tm_mon = 5 - 1;
    start_time_tm.tm_mday = 2;
    auto start_time_t = std::mktime(&start_time_tm);

    std::tm stop_time_tm{};
    stop_time_tm.tm_year = 2011 - 1900;
    stop_time_tm.tm_mon = 5 - 1;
    stop_time_tm.tm_mday = 3;
    auto stop_time_t = std::mktime(&stop_time_tm);

    auto netcdf_met_provider = std::make_shared<data_access::NetCDFMeshPointsDataProvider>(met_forcing_netcdf_path,
                                                                                           std::chrono::system_clock::from_time_t(start_time_t),
                                                                                           std::chrono::system_clock::from_time_t(stop_time_t));

    test_netcdf_met_provider(netcdf_met_provider);

    std::unique_ptr<CoastalFormulation> schism =
        std::make_unique<SchismFormulation>(/*id=*/ "test_schism_formulation",
                                            library_path,
                                            init_config_path,
                                            MPI_COMM_SELF,
                                            netcdf_met_provider,
                                            provider,
                                            provider
                                            );

    schism->initialize();

    for (int i = 0; i < 3; ++i) {
        std::cout << "Step " << i << std::endl;
        schism->update();
    }

    using namespace std::chrono_literals;

    auto report = [](std::vector<double> const& data, std::string name) {
        double min = 10e6, max = -10e6;
        for (int i = 0; i < data.size(); ++i) {
            double val = data[i];
            if (std::isnan(val)) {
                std::cout << "Nan found at " << i << std::endl;
                break;
            }

            min = std::min(val, min);
            max = std::max(val, max);
        }
        std::cout << name << " with " << data.size() << " entries ranges from " << min << " to " << max << std::endl;
    };

    MeshPointsSelector bedlevel_selector{"BEDLEVEL", std::chrono::system_clock::now(), 3600s, "m", all_points};
    auto bedlevel = schism->get_values(bedlevel_selector, data_access::ReSampleMethod::FRONT_FILL);
    report(bedlevel, "BEDLEVEL");

    for (int i = 0; i < bedlevel.size(); ++i) {
        if (bedlevel[i] == -9999) {
            std::cout << "Bed level is sentinel at index " << i << std::endl;
        }
    }

    MeshPointsSelector eta2_selector{"ETA2", std::chrono::system_clock::now(), 3600s, "m", all_points};
    auto eta2 = schism->get_values(eta2_selector, data_access::ReSampleMethod::FRONT_FILL);
    report(eta2, "ETA2");

    MeshPointsSelector tr_eta2_selector{"TROUTE_ETA2", std::chrono::system_clock::now(), 3600s, "m", all_points};
    auto tr_eta2 = schism->get_values(tr_eta2_selector, data_access::ReSampleMethod::FRONT_FILL);
    report(tr_eta2, "TROUTE_ETA2");

    MeshPointsSelector vx_selector{"VX", std::chrono::system_clock::now(), 3600s, "m s-1", all_points};
    auto vx = schism->get_values(vx_selector, data_access::ReSampleMethod::FRONT_FILL);
    report(vx, "VX");

    MeshPointsSelector vy_selector{"VY", std::chrono::system_clock::now(), 3600s, "m s-1", all_points};
    auto vy = schism->get_values(vy_selector, data_access::ReSampleMethod::FRONT_FILL);
    report(vy, "VY");

    schism->update();
    schism->get_values(vx_selector, data_access::ReSampleMethod::FRONT_FILL);
    schism->get_values(vy_selector, data_access::ReSampleMethod::FRONT_FILL);
    report(vx, "VX");
    report(vy, "VY");

    schism->finalize();
    MPI_Finalize();
    return 0;
}
