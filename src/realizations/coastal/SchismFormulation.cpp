#include <NGenConfig.h>

#if NGEN_WITH_BMI_FORTRAN

#include <realizations/coastal/SchismFormulation.hpp>

const static auto s_schism_registration_function = "schism_registration_function";

std::set<std::string> SchismFormulation::expected_input_variable_names_ =
    {
        "RAINRATE",
        "SFCPRS",
        "SPHF2m",
        "TMP2m",
        "UU10m",
        "VV10m",
        "ETA2_bnd",
        "Q_bnd"
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
         );
}

void SchismFormulation::initialize()
{
    bmi_->Initialize();

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

    // ETA2_bnd - water surface elevation at the boundaries
    // Q_bnd - flows at boundaries


    bmi_->Update();
}

#endif // NGEN_WITH_BMI_FORTRAN
