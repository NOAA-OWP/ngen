//#include "gtest/gtest.h"
#include <utilities/parallel_utils.h>

#include "realizations/coastal/SchismFormulation.hpp"

const static std::string library_path = "/Users/phil/Code/noaa/BUILD/bmischism_2024-08-19-ngen/libtestbmifortranmodel.dylib";
const static std::string init_config_path = "/Users/phil/Code/noaa/SCHISM_Lake_Champlain_BMI_test/namelist.input";

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
        {"SFCPRS", 101325.0},
        // SPFH2m - specific humidity at 2m
        {"SPFH2m", 0.01},
        // TMP2m - temperature at 2m
        {"TMP2m", 293},
        // UU10m, VV10m - wind velocity components at 10m
        {"UU10m", 1.0},
        {"VV10m", 1.0},

        /* Input Boundary Conditions */
        // ETA2_bnd - water surface elevation at the boundaries
        {"ETA2_bnd", 30},
        // Q_bnd - flows at boundaries
        {"Q_bnd", 0.1},
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
    std::vector<double> get_values(const selection_type& selector, data_access::ReSampleMethod) override
    {
        auto default_value = input_variables_defaults[selector.variable_name];
        for (auto& val : data) {
            val = default_value;
        }
        return data;
    }
};

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    auto provider = std::make_shared<MockProvider>();
    auto schism = std::make_unique<SchismFormulation>(/*id=*/ "test_schism_formulation",
                                                      library_path,
                                                      init_config_path,
                                                      provider,
                                                      provider,
                                                      provider
                                                      );

    schism->initialize();

    schism->update();

    schism->finalize();
    MPI_Finalize();
    return 0;
}
