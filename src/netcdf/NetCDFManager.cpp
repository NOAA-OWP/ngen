#if NGEN_WITH_NETCDF
#include "NetCDFManager.hpp"
#include "Formulation_Manager.hpp"
#include "Catchment_Formulation.hpp"
#include "simulation_time/Simulation_Time.hpp"
#include "Logger.hpp"
#include <stdexcept>
#include <filesystem>
#include <cstdint>
#include <NGenConfig.h>

NetCDFManager::NetCDFManager(std::shared_ptr<realization::Formulation_Manager> manager, 
        const std::string& output_name, Simulation_Time const& sim_time, bool create_new_file, int mpi_rank, int mpi_num_procs)
    : manager_{manager}, open_mode_{create_new_file ? NetCDFOpenMode::CREATE : NetCDFOpenMode::OPEN_WRITE}
{
    std::filesystem::path check_path(output_name);
    if(check_path.is_absolute()){
        nc_filename_ = output_name;
    }
    else{
        nc_filename_ = manager_->get_output_root() + output_name + ".nc";
    }

    rank_ = mpi_rank;
    num_procs_ = mpi_num_procs;
#if NGEN_WITH_MPI
    is_mpi_ = (num_procs_ > 1);
    comm_ = (num_procs_ > 1) ? MPI_COMM_WORLD : MPI_COMM_NULL;
#else
    is_mpi_ = false;
#endif
    int num_catchments = manager_->get_size();
    std::vector<int64_t> catchments_in_proc; 
    catchments_in_proc.reserve(num_catchments);
    for (auto const& formulation_info : manager_->get_all_formulations())
    {
        int64_t catchment_id;
        std::string catchm = formulation_info.first;
        if (catchm.rfind("cat-", 0) == 0){
            catchment_id = std::stoll(catchm.substr(4));
        }
        else{
            catchment_id = std::stoll(formulation_info.first);
        }
        catchments_in_proc.push_back(catchment_id);
    }
    gather_all_catchments(catchments_in_proc);
    if (rank_ == 0){
        nc_file_ = std::make_unique<NetCDFFile>(nc_filename_, this->open_mode_, is_mpi_);
        if (create_new_file)
            define_catchment_netcdf_components(sim_time);
        else
            read_catchment_netcdf_components();
    }
#if NGEN_WITH_MPI
    if (comm_ != MPI_COMM_NULL) //This check is important if the user runs MPI with a single process.
        MPI_Barrier(comm_);
#endif
}

NetCDFManager::NetCDFManager(const std::string& filename, NetCDFOpenMode open_mode)
    : open_mode_{open_mode}
{
    if (open_mode_ == NetCDFOpenMode::OPEN_READ) {
        nc_file_ = std::make_unique<NetCDFFile>(filename, open_mode, false);
    }
    else{
        throw std::runtime_error("Write non-MPI function not implemented.");
    }
#if NGEN_WITH_MPI
    comm_ = MPI_COMM_NULL;
    rank_ = 0;
    num_procs_ = 1;
#endif
}

NetCDFManager::NetCDFManager()
{}

int NetCDFManager::create_file(const std::string& filename)
{
#if NGEN_WITH_MPI
    if (num_procs_ > 1) {
        // MPI-enabled NetCDF
        is_mpi_ = true;
        nc_file_ = std::make_unique<NetCDFFile>(filename, NetCDFOpenMode::CREATE, is_mpi_);
    }else{
        is_mpi_ = false;
    }
#endif
    nc_filename_ = filename;
    nc_file_ = std::make_unique<NetCDFFile>(nc_filename_, NetCDFOpenMode::CREATE, is_mpi_);
    if (!nc_file_) {
        throw std::runtime_error("Failed to create NetCDF file: " + filename);
    }
    return nc_file_ ->get_ncid();
}

void NetCDFManager::gather_all_catchments(const std::vector<int64_t>& catchments_in_proc)
{
    if (!is_mpi_)
    {
        catchments_ = catchments_in_proc;
        return;
    }
#if NGEN_WITH_MPI
    if (rank_ == 0)
    {
        catchments_ = catchments_in_proc;
        for (int proc = 1;proc < num_procs_; ++proc)
        {
            int count;
            MPI_Recv(&count, 1, MPI_INT, proc, 100, comm_, MPI_STATUS_IGNORE);
            for (int i = 0;i < count; ++i)
            {
                int64_t catchment_id;
                MPI_Recv(&catchment_id, 1, MPI_LONG_LONG, proc, 101, comm_, MPI_STATUS_IGNORE);
                catchments_.push_back(catchment_id);
            }
        }
    }
    else
    {
        int count = catchments_in_proc.size();
        MPI_Send( &count, 1, MPI_INT, 0, 100, comm_);
        for (const auto& catchment : catchments_in_proc)
        {
            int64_t catchment_id = catchment;
            MPI_Send( &catchment_id, 1, MPI_LONG_LONG, 0, 101, comm_);
        }
    }
#endif
}

void NetCDFManager::define_catchment_netcdf_components(Simulation_Time sim_time)
{
    std::string name;
    int dim_id;
    std::vector<int> dim_ids;
    std::vector<std::string> names;
    
    //add time dimension and variable
    try
    {
        name = "time";
        int num_timesteps = sim_time.get_total_output_times();
        dim_id = add_dimension(name, num_timesteps);
        dim_ids = {dim_id};
        names = {name};
        add_variable(name, NC_INT, dim_ids, names);
        
        //add timestep values and attributes for time
        std::vector<int> time_epoch_seconds(num_timesteps);
        time_epoch_seconds[0] = sim_time.get_current_epoch_time();
        for(int time_index = 1; time_index < num_timesteps; time_index++)
        {
            sim_time.advance_timestep();
            time_epoch_seconds[time_index] = sim_time.get_current_epoch_time();
        }
        nc_file_->write_variable_data(name, time_epoch_seconds);
        nc_file_->write_attribute_to_ncvar(name, "units", "Seconds since 1970-01-01 00:00:00");
        nc_file_->write_attribute_to_ncvar(name, "calendar", "gregorian");
    }
    catch(const std::runtime_error& e)
    {
        LOG(std::string("Error in adding time information to catchment NetCDF: ") + e.what(), LogLevel::FATAL);
        throw std::runtime_error(std::string("Error in adding time information to catchment NetCDF: ") + e.what());
    }

    //add catchments dimension and variable
    try
    {
        name = "catchments";
        int num_catchments = catchments_.size();
        dim_id = add_dimension(name, num_catchments);
        dim_ids = {dim_id};
        names = {name};
        add_variable(name, NC_INT64, dim_ids, names);
        nc_file_->write_variable_data(name, catchments_);
        nc_file_->write_attribute_to_ncvar(name, "Catchment ID", "Catchment identifier in input");
    }
    catch(const std::runtime_error& e)
    {
        LOG(std::string("Error in adding catchment IDs to catchment NetCDF: ") + e.what(), LogLevel::FATAL);
        throw std::runtime_error(std::string("Error in adding catchment IDs to catchment NetCDF: ") + e.what());
    }

    // Add realization config output variables
    try
    {
        add_output_variable_data_from_formulation();
    }
    catch(const std::runtime_error& e)
    {
        LOG(std::string("Error in adding output variables information to catchment NetCDF: ") + e.what(), LogLevel::FATAL);
        throw std::runtime_error(std::string("Error in adding output variables information to catchment NetCDF: ") + e.what());
    }
    // Switch to data mode for writing
    nc_file_->end_def_mode();
}

void NetCDFManager::read_catchment_netcdf_components() {
    int cat_id;
    std::shared_ptr<NetCDFVar> cat_var;
    try {
        this->nc_file_->read_dimension_definition("time");
        this->nc_file_->read_variable_definition("time");
        cat_id = this->nc_file_->read_dimension_definition("catchments");
        cat_var = this->nc_file_->read_variable_definition("catchments");
    } catch (const std::exception& e) {
        LOG(LogLevel::FATAL, "The existing netCDF file must have existing dimensions and variables for 'time' and 'catchments'.");
        LOG(LogLevel::FATAL, e.what());
        throw;
    }
    try {
        size_t cat_size = cat_var->get_dim_size(cat_id);
        if (cat_size != this->catchments_.size())
            throw std::runtime_error("The existing netCDF file has a different catchment dimension size than the run formulation.");
        cat_var->build_catchments_index(cat_size);
    } catch (const std::exception& e) {
        LOG(LogLevel::FATAL, "NetCDF failed to build catchment output index.");
        LOG(LogLevel::FATAL, e.what());
        throw;
    }
    try {
        this->read_output_variable_data_from_formulation();
    } catch (const std::exception& e) {
        LOG(LogLevel::FATAL, "Failed to match netCDF output variables to forumation output variables.");
        LOG(LogLevel::FATAL, e.what());
        throw;
    }
}

int NetCDFManager::add_dimension(const std::string& name, size_t len){
    return nc_file_->add_dimension(name, len);
}

void NetCDFManager::add_variable(const std::string& var_name, nc_type type, const std::vector<int>& dim_ids, const std::vector<std::string>& dim_names){
    nc_file_->add_variable(var_name, type, dim_ids, dim_names);
}

void NetCDFManager::add_output_variable_data_from_formulation() 
{
    typename std::map<std::string, std::shared_ptr<realization::Catchment_Formulation>>::const_iterator it = manager_->begin();
    const auto& catchment_info = *it;
    auto r_c = std::dynamic_pointer_cast<realization::Bmi_Formulation>(catchment_info.second);
    if(r_c->get_output_header_count() > 0){
        std::vector<std::string>output_variables = r_c->get_output_variable_names();
        std::vector<std::string>output_headers = r_c->get_output_header_fields();
        std::vector<std::string>output_units = r_c->get_output_variable_units();
        nc_output_variables_.reserve(output_variables.size());
        std::vector<int> dims_2D = {nc_file_->get_dim_id("time"), nc_file_->get_dim_id("catchments")};
        std::vector<std::string> dim_names{"time", "catchments"};

        for(int index = 0; index < output_variables.size(); index ++){
            nc_file_->add_variable(output_headers[index], NC_DOUBLE, dims_2D, dim_names);
            nc_file_->write_attribute_to_ncvar(output_headers[index], "variable name", output_variables[index]);
            nc_file_->write_attribute_to_ncvar(output_headers[index], "variable units", output_units[index]);
            nc_file_->write_attribute_to_ncvar(output_headers[index], "_FillValue", "-1.0");
            nc_file_->write_attribute_to_ncvar(output_headers[index], "missing_value", "-1.0");
            nc_output_variables_.push_back(output_headers[index]);
        }
    }
}

void NetCDFManager::read_output_variable_data_from_formulation() {
    typename std::map<std::string, std::shared_ptr<realization::Catchment_Formulation>>::const_iterator it = manager_->begin();
    const auto& catchment_info = *it;
    auto r_c = std::dynamic_pointer_cast<realization::Bmi_Formulation>(catchment_info.second);
    if(r_c->get_output_header_count() > 0){
        std::vector<std::string>output_variables = r_c->get_output_variable_names();
        std::vector<std::string>output_headers = r_c->get_output_header_fields();
        nc_output_variables_.reserve(output_variables.size());
        std::vector<std::string> dim_names{"time", "catchments"};

        for (int index = 0; index < output_variables.size(); index ++){
            auto var = nc_file_->read_variable_definition(output_headers[index]);
            const auto& var_dim_names = var->get_dim_names();
            for (const std::string& var_dim : dim_names)
                if (std::find(var_dim_names.begin(), var_dim_names.end(), var_dim) == var_dim_names.end())
                    throw std::runtime_error("NetCDF output variable " + var->get_name() + " does not use dimension " + var_dim);
            if (var->get_type() != NC_DOUBLE)
                throw std::runtime_error("NetCDF output variable " + var->get_name() + " is not of type double.");
            nc_output_variables_.push_back(output_headers[index]);
        }
    }
}

static std::vector<double> string_split(std::string str, char delimiter)
{
    std::stringstream ss(str);
    std::vector<double> res;
    std::string token;
    while (getline(ss, token, delimiter)) { //will return full string if no comma (or single output variable).
        res.push_back(std::stod(token)); 
    }
    return res;
}

void NetCDFManager::write_simulations_response_from_formulation(size_t time_index, const std::map<std::string, std::string>& catchment_output_values)
{
    std::map<int64_t, std::string> output_values;
    try{
        for (auto const& catchment_val : catchment_output_values)
        {
            int64_t catchment_id;
            std::string catchm = catchment_val.first;
            if (catchm.rfind("cat-", 0) == 0){
                catchment_id = std::stoll(catchm.substr(4));
            }
            else{
                catchment_id = std::stoll(catchment_val.first);
            }
            output_values[catchment_id] = catchment_val.second;
        }
    }
    catch(const std::runtime_error& e)
    {
        LOG(std::string("Error in reading simulation response line string: ") + e.what(), LogLevel::FATAL);
        throw std::runtime_error(std::string("Error in reading simulation response line string: ") + e.what());
    }
    try{
        if(!is_mpi_){
            auto var = nc_file_->get_ncvar("catchments");
            if (!var){
                LOG("Catchments variable/dimension not found in NetCDF", LogLevel::FATAL);
                throw std::runtime_error("Catchments variable/dimension not found");
            }
            for (auto const& catchment_val : output_values)
            {
                int64_t catchment_id;
                catchment_id = catchment_val.first;
                size_t catchment_index = var->get_catchment_index(catchment_id);
                if (catchment_index < 0) {
                    LOG("Catchments not found in NetCDF", LogLevel::FATAL);
                    throw std::runtime_error("Catchment not found in NetCDF: " + std::to_string(catchment_id));
                }
                std::vector<double> catchment_output = string_split(catchment_val.second, ',');
                std::vector<size_t> start = {time_index, catchment_index};
                std::vector<size_t> count = {1, 1};
                
                for(int var_index = 0; var_index < nc_output_variables_.size(); ++var_index)
                {
                    nc_file_ ->write_catchment_output_data(nc_output_variables_[var_index], start, count, catchment_output[var_index]);
                }
            }
            return;
        }
    }
    catch(const std::runtime_error& e)
    {
        LOG(std::string("Error in writing simulation response to catchment NetCDF: ") + e.what(), LogLevel::FATAL);
        throw std::runtime_error(std::string("Error in writing simulation response to catchment NetCDF: ") + e.what());
    }
#if NGEN_WITH_MPI
    try{
        if (is_mpi_){
            if (rank_ == 0){
                primary_netcdf_writer(time_index, output_values);
            }
            else{
                secondary_netcdf_worker(output_values);
            }
        }
    }
    catch(const std::runtime_error& e)
    {
        LOG(std::string("Error in writing simulation response to catchment NetCDF: ") + e.what(), LogLevel::FATAL);
        throw std::runtime_error(std::string("Error in writing simulation response to catchment NetCDF: ") + e.what());
    }
#endif
}

void NetCDFManager::primary_netcdf_writer(size_t time_index, const std::map<int64_t, std::string>& catchment_output_values)
{
    // Lambda helper function for writing the data to netcdf file
    auto write_data_to_netcdf = [&](const int64_t& catchment_id, const std::string& csv_output_line) {
        std::vector<double> catchment_output = string_split(csv_output_line, ',');
        auto var = nc_file_->get_ncvar("catchments");
        if (!var){
            LOG("Catchments variable/dimension not found in NetCDF", LogLevel::FATAL);
            throw std::runtime_error("Catchments variable/dimension not found");
        }
        size_t catchment_index = var->get_catchment_index(catchment_id);
        if (catchment_index < 0) {
            LOG("Catchments not found in NetCDF", LogLevel::FATAL);
            throw std::runtime_error("Catchment not found in NetCDF: " + std::to_string(catchment_id));
        }
        std::vector<size_t> start = {time_index, catchment_index};
        std::vector<size_t> count = {1, 1};
        for(int var_index = 0; var_index < nc_output_variables_.size(); ++var_index)
        {
            nc_file_->write_catchment_output_data(nc_output_variables_[var_index], start, count, catchment_output[var_index]);
        }
    };
    //Rank 0 writer
    for (auto const& catchment_val : catchment_output_values)
    {
        int64_t catchment_id = catchment_val.first;
        write_data_to_netcdf(catchment_id, catchment_val.second);
    }
#if NGEN_WITH_MPI
    // Other ranks - data sender to rank 0
    for (int proc = 1; proc < num_procs_; ++proc) {
        int catchment_count = 0;
        MPI_Recv(&catchment_count, 1, MPI_INT, proc, 200, comm_, MPI_STATUS_IGNORE);
        for (int i = 0; i < catchment_count; ++i) {
            int64_t catchment_id = 0;
            MPI_Recv(&catchment_id, 1, MPI_LONG_LONG, proc, 201, comm_, MPI_STATUS_IGNORE);
            int data_len = 0;
            MPI_Recv(&data_len, 1, MPI_INT, proc, 202, comm_, MPI_STATUS_IGNORE);
            std::string csv_data(data_len, ' ');
            MPI_Recv(&csv_data[0], data_len, MPI_CHAR, proc, 203, comm_, MPI_STATUS_IGNORE);
            write_data_to_netcdf(catchment_id, csv_data);
        }
    }
#endif
    nc_sync(nc_file_->get_ncid()); 
}

#if NGEN_WITH_MPI
void NetCDFManager::secondary_netcdf_worker(const std::map<int64_t, std::string>& catchment_output_values) {
    int catchment_count = catchment_output_values.size();
    MPI_Send(&catchment_count, 1, MPI_INT, 0, 200, comm_);
    for (const auto& [id, csv_output] : catchment_output_values) {
        int64_t catchment_id = id;
        std::string csv_data = csv_output;
        MPI_Send(&catchment_id, 1, MPI_LONG_LONG, 0, 201, comm_);
        int data_len = csv_data.size();
        MPI_Send(&data_len, 1, MPI_INT, 0, 202, comm_);
        MPI_Send(const_cast<char*>(csv_data.data()), data_len, MPI_CHAR, 0, 203, comm_);
    }
}
#endif

std::vector<std::string> NetCDFManager::list_variables() const
{
    if(!nc_file_) return {};
    return nc_file_->list_variables();
}

std::shared_ptr<NetCDFVar> NetCDFManager::get_ncvar_by_name(const std::string& name) const
{
    if(!nc_file_) return nullptr;
    return nc_file_->get_ncvar(name);
}

std::string NetCDFManager::get_string_attribute(const std::string& var_name, const std::string& att_name) const
{
    auto var = nc_file_->get_ncvar(var_name);
    return var->get_string_attribute(att_name);
}

int NetCDFManager::get_int_attribute(const std::string& var_name, const std::string& att_name) const
{
    auto var = nc_file_->get_ncvar(var_name);
    return var->get_int_attribute(att_name);
}

double NetCDFManager::get_double_attribute(const std::string& var_name, const std::string& att_name) const
{
    auto var = nc_file_->get_ncvar(var_name);
    return var->get_double_attribute(att_name);
}

void NetCDFManager::open_file()
{
    if(nc_file_) {
        nc_file_->load_variables();
    } else {
        throw std::runtime_error("NetCDF file not initialized!");
    }
}

void NetCDFManager::close_file()
{
    if (rank_ == 0 && nc_file_->get_ncid() != -1)
        nc_file_->close_file();
}

NetCDFManager::~NetCDFManager() {}
#endif // NGEN_WITH_NETCDF
