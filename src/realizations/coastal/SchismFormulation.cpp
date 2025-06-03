#include <NGenConfig.h>

#if NGEN_WITH_BMI_FORTRAN && NGEN_WITH_MPI

#include <realizations/coastal/SchismFormulation.hpp>
#include <utilities/parallel_utils.h>

const static auto s_schism_registration_function = "register_bmi";

std::map<std::string, SchismFormulation::InputMapping> SchismFormulation::expected_input_variables_ =
    {
        /* Meteorological Forcings */
        // RAINRATE - precipitation
        {"RAINRATE", { SchismFormulation::METEO, "RAINRATE"}},
        // SFCPRS - surface atmospheric pressure
        {"SFCPRS", { SchismFormulation::METEO, "PSFC"}},
        // SPFH2m - specific humidity at 2m
        {"SPFH2m", { SchismFormulation::METEO, "Q2D"}},
        // TMP2m - temperature at 2m
        {"TMP2m", { SchismFormulation::METEO, "T2D"}},
        // UU10m, VV10m - wind velocity components at 10m
        {"UU10m", { SchismFormulation::METEO, "U2D"}},
        {"VV10m", { SchismFormulation::METEO, "V2D"}},

        /* Input Boundary Conditions */
        // ETA2_bnd - water surface elevation at the boundaries
        {"ETA2_bnd", { SchismFormulation::OFFSHORE, "ETA2_bnd"}},
        // Q_bnd - flows at boundaries
        {"Q_bnd_source", { SchismFormulation::CHANNEL_FLOW, "Q_bnd_source"}},
        // Q_bnd - flows at boundaries
        {"Q_bnd_sink", { SchismFormulation::CHANNEL_FLOW, "Q_bnd_sink"}},
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
                                     , MPI_Comm mpi_comm
                                     , std::shared_ptr<ProviderType> met_forcings
                                     , std::shared_ptr<ProviderType> offshore_boundary
                                     , std::shared_ptr<ProviderType> channel_flow_boundary
                                     )
    : CoastalFormulation(id)
    , meteorological_forcings_provider_(met_forcings)
    , offshore_boundary_provider_(offshore_boundary)
    , channel_flow_boundary_provider_(channel_flow_boundary)
{
    bmi_ = std::make_unique<models::bmi::Bmi_Fortran_Adapter>
        (
         "schism_coastal_formulation"
         , library_path
         , init_config_path
         , /* model_time_step_fixed = */ true
         , s_schism_registration_function
         , mpi_comm
         );
}

SchismFormulation::~SchismFormulation() = default;

void SchismFormulation::initialize()
{
    auto const& input_vars = bmi_->GetInputVarNames();

    for (auto const& full_name : input_vars) {
        auto name = full_name.substr(0, full_name.size()-3); // Chop off _t0/_t1 suffixes
        if (expected_input_variables_.find(name) == expected_input_variables_.end()) {
            throw std::runtime_error("SCHISM instance requests unexpected input variable '" + name + "'");
        }

        input_variable_units_[name] = bmi_->GetVarUnits(name + "_t0");
        input_variable_type_[name] = bmi_->GetVarType(name + "_t0");
        input_variable_count_[name] = mesh_size(name + "_t0");
    }

    auto const& output_vars = bmi_->GetOutputVarNames();

    for (auto const& name : output_vars) {
        //if (expected_output_variables_.find(name) == expected_output_variables_.end()) {
        //    throw std::runtime_error("SCHISM instance requests unexpected output variable '" + name + "'");
        //}

        output_variable_units_[name] = bmi_->GetVarUnits(name);
        output_variable_type_[name] = bmi_->GetVarType(name);
        output_variable_count_[name] = mesh_size(name);
    }

    time_step_length_ = std::chrono::seconds((long long)bmi_->GetTimeStep());

    set_inputs(0);
}

void SchismFormulation::finalize()
{
    meteorological_forcings_provider_->finalize();
    offshore_boundary_provider_->finalize();
    channel_flow_boundary_provider_->finalize();

    bmi_->Finalize();
}

void SchismFormulation::set_inputs(int timestep_offset)
{
    for (auto var : expected_input_variables_) {
        auto const& name = var.first;
        auto const& mapping = var.second;
        auto selector = mapping.selector;
        auto const& source_name = mapping.name;
        auto points = MeshPointsSelector{source_name, current_time_, time_step_length_, input_variable_units_[name], all_points};

        ProviderType* provider = [this, selector](){
            switch(selector) {
            case METEO: return meteorological_forcings_provider_.get();
            case OFFSHORE: return offshore_boundary_provider_.get();
            case CHANNEL_FLOW: return channel_flow_boundary_provider_.get();
            default: throw std::runtime_error("Unknown SCHISM provider selector type");
            }
        }();
        auto offset_name = name + "_t" + std::to_string(timestep_offset);
        std::vector<double> buffer(mesh_size(offset_name));
        provider->get_values(points, buffer);
        bmi_->SetValue(offset_name, buffer.data());
    }
}

void SchismFormulation::update()
{
    current_time_ += time_step_length_;
    set_inputs(1);
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
    bmi_->GetValue(selector.variable_name, data.data());
}

size_t SchismFormulation::mesh_size(std::string const& variable_name)
{
    auto nbytes = bmi_->GetVarNbytes(variable_name);
    auto itemsize = bmi_->GetVarItemsize(variable_name);
    if (nbytes % itemsize != 0) {
        throw std::runtime_error("For SCHISM variable '" + variable_name + "', itemsize " + std::to_string(itemsize) +
                                 " does not evenly divide nbytes " + std::to_string(nbytes));
    }
    return nbytes / itemsize;
}

#endif // NGEN_WITH_BMI_FORTRAN && NGEN_WITH_MPI
