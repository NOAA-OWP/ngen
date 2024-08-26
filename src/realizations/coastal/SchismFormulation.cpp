#include <NGenConfig.h>

#if NGEN_WITH_BMI_FORTRAN && NGEN_WITH_MPI

#include <realizations/coastal/SchismFormulation.hpp>
#include <mpi.h>

const static auto s_schism_registration_function = "register_bmi";

std::set<std::string> SchismFormulation::expected_input_variable_names_ =
    {
        "RAINRATE",
        "SFCPRS",
        "SPFH2m",
        "TMP2m",
        "UU10m",
        "VV10m",
        "ETA2_bnd",
        "Q_bnd"
    };

std::vector<std::string> SchismFormulation::exported_output_variable_names_ =
    {
        "ETA2",
        "VX",
        "VY",
        "BEDLEVEL"
    };

SchismFormulation::SchismFormulation(
                                     std::string const& id
                                     , std::string const& library_path
                                     , std::string const& init_config_path
                                     , std::shared_ptr<MeshPointsDataProvider> met_forcings
                                     , std::shared_ptr<MeshPointsDataProvider> offshore_boundary
                                     , std::shared_ptr<MeshPointsDataProvider> inflow_boundary
                                     )
    : CoastalFormulation(id)
    , meteorological_forcings_provider_(met_forcings)
    , offshore_boundary_provider_(offshore_boundary)
    , inflows_boundary_provider_(inflow_boundary)
{
    bmi_ = std::make_unique<models::bmi::Bmi_Fortran_Adapter>
        (
         "schism_coastal_formulation"
         , library_path
         , init_config_path
         , /* model_time_step_fixed = */ true
         , s_schism_registration_function
         , MPI_COMM_WORLD
         );
}

SchismFormulation::~SchismFormulation() = default;

void SchismFormulation::initialize()
{
    auto const& input_vars = bmi_->GetInputVarNames();

    for (auto const& name : input_vars) {
        if (expected_input_variable_names_.find(name) == expected_input_variable_names_.end()) {
            throw std::runtime_error("SCHISM instance requests unexpected input variable '" + name + "'");
        }

        input_variable_units_[name] = bmi_->GetVarUnits(name);
        input_variable_type_[name] = bmi_->GetVarType(name);

        auto nbytes = bmi_->GetVarNbytes(name);
        auto itemsize = bmi_->GetVarItemsize(name);
        if (nbytes % itemsize != 0) {
            throw std::runtime_error("For SCHISM input variable '" + name + "', itemsize " + std::to_string(itemsize) +
                                     " does not evenly divide nbytes " + std::to_string(nbytes));
        }
        input_variable_count_[name] = nbytes / itemsize;
    }
}

void SchismFormulation::finalize()
{
    meteorological_forcings_provider_->finalize();
    offshore_boundary_provider_->finalize();
    inflows_boundary_provider_->finalize();

    bmi_->Finalize();
}

void SchismFormulation::update()
{
    // Meteorological forcings
    // RAINRATE - precipitation
    // SFCPRS - surface atmospheric pressure
    // SPFH2m - specific humidity at 2m
    // TMP2m - temperature at 2m
    // UU10m, VV10m - wind velocity components at 10m

    //auto rain_points = MeshPointsSelector{"RAINRATE", current_time_, time_step_length_, input_variable_units_["RAINRATE"], all_points};

    // ETA2_bnd - water surface elevation at the boundaries
    // Q_bnd - flows at boundaries


    bmi_->Update();
}

boost::span<const std::string> SchismFormulation::get_available_variable_names() const
{
    return exported_output_variable_names_;
}

long SchismFormulation::get_data_start_time() const
{
    throw std::runtime_error(__func__);
    return 0;
}

long SchismFormulation::get_data_stop_time() const
{
    throw std::runtime_error(__func__);
    return 0;
}

long SchismFormulation::record_duration() const
{
    throw std::runtime_error(__func__);
    return 0;
}

size_t SchismFormulation::get_ts_index_for_time(const time_t &epoch_time) const
{
    throw std::runtime_error(__func__);
    return 0;
}

SchismFormulation::data_type SchismFormulation::get_value(const selection_type& selector, data_access::ReSampleMethod m)
{
    throw std::runtime_error(__func__);
    return 0.0;
}

void SchismFormulation::get_values(const selection_type& selector, boost::span<double> data)
{
    throw std::runtime_error(__func__);
}

size_t SchismFormulation::mesh_size(std::string const& variable_name)
{
    throw std::runtime_error(__func__);
    return 0;
}


#endif // NGEN_WITH_BMI_FORTRAN && NGEN_WITH_MPI
