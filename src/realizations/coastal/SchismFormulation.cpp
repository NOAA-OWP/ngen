#include <realizations/coastal/SchismFormulation.hpp>

const static auto library_path = "/path/to/built/libpschism.so";
const static auto schism_registration_function = "schism_registration_function";

SchismFormulation::SchismFormulation(
                                     std::string const& id
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
         , true // allow_model_exceed_end_time
         , true // model_time_step_fixed
         , schism_registration_function
         );
}
