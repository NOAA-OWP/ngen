//#include "gtest/gtest.h"
#include <utilities/parallel_utils.h>

#include "realizations/coastal/SchismFormulation.hpp"
#include <cmath>
#include <limits>
#include <iostream>
#include <NetCDFMeshPointsDataProvider.hpp>

const static std::string library_path = "/Users/phil/Code/noaa/BUILD/bmischism_2024-09-27-ngen/libtestbmifortranmodel.dylib";
const static std::string init_config_path = "/Users/phil/Code/noaa/SCHISM_Lake_Champlain_driver_test/namelist.input";
const static std::string met_forcing_netcdf_path = "/Users/phil/Code/noaa/SCHISM_Lake_Champlain_BMI_test/NextGen_Forcings_Engine_MESH_output_201104302300.nc";

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
    auto schism = std::make_unique<SchismFormulation>(/*id=*/ "test_schism_formulation",
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

    std::vector<double> bedlevel(278784, std::numeric_limits<double>::quiet_NaN());
    MeshPointsSelector bedlevel_selector{"BEDLEVEL", std::chrono::system_clock::now(), 3600s, "m", all_points};
    schism->get_values(bedlevel_selector, bedlevel);
    report(bedlevel, "BEDLEVEL");

    std::vector<double> eta2(278784, std::numeric_limits<double>::quiet_NaN());
    MeshPointsSelector eta2_selector{"ETA2", std::chrono::system_clock::now(), 3600s, "m", all_points};
    schism->get_values(eta2_selector, eta2);
    report(eta2, "ETA2");

    std::vector<double> vx(278784, std::numeric_limits<double>::quiet_NaN());
    MeshPointsSelector vx_selector{"VX", std::chrono::system_clock::now(), 3600s, "m s-1", all_points};
    schism->get_values(vx_selector, vx);
    report(vx, "VX");

    std::vector<double> vy(278784, std::numeric_limits<double>::quiet_NaN());
    MeshPointsSelector vy_selector{"VY", std::chrono::system_clock::now(), 3600s, "m s-1", all_points};
    schism->get_values(vy_selector, vy);
    report(vy, "VY");

    schism->update();
    schism->get_values(vx_selector, vx);
    schism->get_values(vy_selector, vy);
    report(vx, "VX");
    report(vy, "VY");

    schism->finalize();
    MPI_Finalize();
    return 0;
}
