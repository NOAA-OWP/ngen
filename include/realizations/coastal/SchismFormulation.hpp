#pragma once

#include <NGenConfig.h>

#include <realizations/coastal/CoastalFormulation.hpp>
#include <bmi/Bmi_Fortran_Adapter.hpp>
#include <memory>

class SchismFormulation : public CoastalFormulation
{
public:
    using MeshPointsDataProvider = data_access::DataProvider<double, MeshPointsSelector>;
    SchismFormulation(
                      std::string const& id
                      , std::string const& init_config_path
                      , std::shared_ptr<MeshPointsDataProvider> met_forcings
                      , std::shared_ptr<MeshPointsDataProvider> offshore_boundary
                      , std::shared_ptr<MeshPointsDataProvider> inflow_boundary
                      );

    ~SchismFormulation();

    // Implementation of DataProvider
    boost::span<const std::string> get_available_variable_names() override;

    long get_data_start_time() override;
    long get_data_stop_time() override;
    long record_duration() override;
    size_t get_ts_index_for_time(const time_t &epoch_time) override;

    data_type get_value(const selection_type& selector, data_access::ReSampleMethod m) override;
    std::vector<data_type> get_values(const selection_type& selector, data_access::ReSampleMethod m) override;

    // Implementation of CoastalFormulation
    void initialize() override;
    void finalize() override;
    void update() override;

    void get_values(const selection_type& selector, boost::span<double> data) override;

private:
    std::unique_ptr<models::bmi::Bmi_Fortran_Adapter> bmi_;

    // TODO: Some of these maybe should be members of
    // CoastalFormulation

    // TODO: We really want something that we can ask for
    // area-averaged RAINRATE over elements, but we're going to makde
    // do with point values at the element centroids
    std::shared_ptr<data_access::DataProvider<double, MeshPointsSelector>> meteorological_forcings_provider_;
    std::shared_ptr<data_access::DataProvider<double, MeshPointsSelector>> offshore_boundary_provider_;
    std::shared_ptr<data_access::DataProvider<double, MeshPointsSelector>> inflows_boundary_provider_;
};
