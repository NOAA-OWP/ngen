#pragma once

#include <NGenConfig.h>

#if NGEN_WITH_BMI_FORTRAN && NGEN_WITH_MPI

#include <realizations/coastal/CoastalFormulation.hpp>
#include <bmi/Bmi_Fortran_Adapter.hpp>
#include <memory>
#include <map>
#include <vector>

class SchismFormulation final : public CoastalFormulation
{
public:
    using MeshPointsDataProvider = data_access::DataProvider<double, MeshPointsSelector>;
    SchismFormulation(
                      std::string const& id
                      , std::string const& library_path
                      , std::string const& init_config_path
                      , std::shared_ptr<MeshPointsDataProvider> met_forcings
                      , std::shared_ptr<MeshPointsDataProvider> offshore_boundary
                      , std::shared_ptr<MeshPointsDataProvider> inflow_boundary
                      );

    ~SchismFormulation();

    // Implementation of DataProvider
    boost::span<const std::string> get_available_variable_names() const override;

    long get_data_start_time() const override;
    long get_data_stop_time() const override;
    long record_duration() const override;
    size_t get_ts_index_for_time(const time_t &epoch_time) const override;

    data_type get_value(const selection_type& selector, data_access::ReSampleMethod m) override;

    // Implementation of CoastalFormulation
    void initialize() override;
    void finalize() override;
    void update() override;

    void get_values(const selection_type& selector, boost::span<double> data) override;

protected:
    size_t mesh_size(std::string const& variable_name) override;

private:
    std::unique_ptr<models::bmi::Bmi_Fortran_Adapter> bmi_;

    enum ForcingSelector { METEO, OFFSHORE, INFLOW };
    static std::map<std::string, ForcingSelector> expected_input_variables_;
    std::map<std::string, std::string> input_variable_units_;
    std::map<std::string, std::string> input_variable_type_;
    std::map<std::string, size_t> input_variable_count_;

    static std::vector<std::string> exported_output_variable_names_;

    std::chrono::time_point<std::chrono::steady_clock> current_time_;
    std::chrono::seconds time_step_length_;

    // TODO: Some of these maybe should be members of
    // CoastalFormulation

    // TODO: We really want something that we can ask for
    // area-averaged RAINRATE over elements, but we're going to make
    // do with point values at the element centroids

    using ProviderType = data_access::DataProvider<double, MeshPointsSelector>;
    std::shared_ptr<ProviderType> meteorological_forcings_provider_;
    std::shared_ptr<ProviderType> offshore_boundary_provider_;
    std::shared_ptr<ProviderType> inflows_boundary_provider_;

    void set_inputs();
};

#endif // NGEN_WITH_BMI_FORTRAN && NGEN_WITH_MPI
