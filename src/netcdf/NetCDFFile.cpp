#if NGEN_WITH_MPI
    #define _PARALLEL4
#endif

#if NGEN_WITH_NETCDF
#include "NetCDFFile.hpp"
#include "Logger.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <NGenConfig.h>

#define NC_CHECK(err, err_context) \
    do { \
        int retval = (err); \
        if (retval != NC_NOERR) { \
            std::string msg = std::string(err_context) \
                            + ". Internal NetCDF Error: " + std::string(nc_strerror(retval)) \
                            + ". Error origin: " + std::string(__func__); \
            LOG(msg, LogLevel::FATAL); \
            throw(msg); \
        } \
    } while (0)


NetCDFFile::NetCDFFile(const std::string& filename, NetCDFOpenMode open_mode, bool is_mpi)
    : nc_file_name_(filename), is_mpi_(is_mpi)
{
    switch (open_mode) {
        case NetCDFOpenMode::OPEN_READ:
            read_only_ = true;
            NC_CHECK(nc_open(nc_file_name_.c_str(), NC_NOWRITE, &ncid_), "Opening NetCDF file in read-only mode failed.");
            break;
        case NetCDFOpenMode::OPEN_WRITE:
            read_only_ = false;
            NC_CHECK(nc_open(nc_file_name_.c_str(), NC_WRITE, &ncid_), "Opening NetCDF file in write mode failed.");
            break;
        case NetCDFOpenMode::CREATE:
            read_only_ = false;
            NC_CHECK(nc_create(nc_file_name_.c_str(), NC_NETCDF4 | NC_CLOBBER, &ncid_), "Creating NetCDF file failed");
            break;
        default:
            std::string err_msg = "Invalid open mode for NetCDFFile";
            LOG(LogLevel::FATAL, err_msg);
            throw std::runtime_error(err_msg);
    }
    if(read_only_){
        load_variables(); //load all netcdf data to objects.
    } 
}

// Add a new dimension
int NetCDFFile::add_dimension(const std::string& name, size_t len) {
    int dimid;
    NC_CHECK(nc_def_dim(ncid_, name.c_str(), len, &dimid), "Defining dimension failed");
    dims_id_map_[name] = dimid;
    return dimid;
}

int NetCDFFile::read_dimension_definition(const std::string& name) {
    int dimid;
    NC_CHECK(nc_inq_dimid(ncid_, name.c_str(), &dimid), "NetCDF dimension " + name + " could not be found.");
    dims_id_map_[name] = dimid;
    return dimid;
}

void NetCDFFile::add_variable(const std::string& name, nc_type type, const std::vector<int>& dim_ids, const std::vector<std::string>& dim_names) {
    int varid;
    NC_CHECK(nc_def_var(ncid_, name.c_str(), type, dim_ids.size(), dim_ids.data(), &varid), "NetCDF variable definition failed");
    // set fill value and missing value for NC_DOUBLE
    if (type == NC_DOUBLE) {
        double fill_missing_val = -1.0;
        NC_CHECK(nc_def_var_fill(ncid_, varid, 0, &fill_missing_val), "NetCDF assigning fill value for variable failed"); 
        NC_CHECK(nc_put_att_double(ncid_, varid, "missing_value", NC_DOUBLE, 1, &fill_missing_val), "NetCDF assigning missing value for variable failed");
    }
    auto var = std::make_shared<NetCDFVar>(name, type, dim_ids, dim_names, varid, ncid_);
    add_ncvar(var);
}

std::shared_ptr<NetCDFVar> NetCDFFile::read_variable_definition(const std::string& name) {
    int varid, n_dim_ids;
    nc_type type;
    NC_CHECK(nc_inq_varid(ncid_, name.c_str(), &varid), "NetCDF variable " + name + " could not be found.");
    NC_CHECK(nc_inq_vartype(ncid_, varid, &type), "NetCDF variable " + name + " type read failed.");
    // get the variable's dimensions
    std::vector<std::string> dim_names;
    std::vector<int> dim_ids;
    NC_CHECK(nc_inq_varndims(ncid_, varid, &n_dim_ids), "NetCDF variable " + name + " associated dimensions count read failed.");
    dim_ids.resize(n_dim_ids);
    dim_names.reserve(n_dim_ids);
    NC_CHECK(nc_inq_vardimid(ncid_, varid, dim_ids.data()), "NetCDF variable " + name + " associated dimensions IDs read failed.");
    char dim_name[NC_MAX_NAME + 1];
    for (const int dim_id : dim_ids) {
        NC_CHECK(nc_inq_dimname(ncid_, dim_id, dim_name), "NetCDF dimension name query failed.");
        std::string dim_str(dim_name);
        dim_names.push_back(dim_str);
    }
    auto var = std::make_shared<NetCDFVar>(name, type, dim_ids, dim_names, varid, ncid_);
    add_ncvar(var);
    return var;
}

// Get dimension length by name
size_t NetCDFFile::get_dim_size(const std::string& name) const {
    auto it = dims_id_map_.find(name);
    if(it == dims_id_map_.end()) return -1;

    size_t len;
    int retval = nc_inq_dimlen(ncid_, it->second, &len);
    if(retval != NC_NOERR) return -1;
    return len;
}

int NetCDFFile::get_dim_id(const std::string& name) const {
    auto it = dims_id_map_.find(name);
    if(it == dims_id_map_.end()) return -1;
    return it->second;
}

// Add a variable
void NetCDFFile::add_ncvar(std::shared_ptr<NetCDFVar> var) {
    variables_map_[var->get_name()] = var;
}

void NetCDFFile::write_catchment_output_data(const std::string& name, std::vector<size_t> start,
                        std::vector<size_t> count, const double data)
{
    auto var = get_ncvar_by_name(name);
    if (!var) throw std::runtime_error("Variable not found: " + name);
    NC_CHECK(nc_put_vara_double(ncid_, var->get_varid(), start.data(), count.data(), &data), "Writing double value failed");
}

void NetCDFFile::write_data_to_ncvar(int ncid_, int varid, const std::vector<size_t>& start, 
                 const std::vector<size_t>& count, const std::vector<double>& data) {
    NC_CHECK(nc_put_vara_double(ncid_, varid, start.data(), count.data(), data.data()), "Writing double value failed");
}

void NetCDFFile::write_data_to_ncvar(int ncid_, int varid, const std::vector<size_t>& start, 
                 const std::vector<size_t>& count, const std::vector<int>& data) {
    NC_CHECK(nc_put_vara_int(ncid_, varid, start.data(), count.data(), data.data()), "Writing integer value failed");
}

void NetCDFFile::write_data_to_ncvar(int ncid_, int varid, const std::vector<size_t>& start, 
                 const std::vector<size_t>& count, const std::vector<int64_t>& data) {
    NC_CHECK(nc_put_vara_longlong(ncid_, varid, start.data(), count.data(), (const long long*)data.data()), "Writing big integer value failed");
}

void NetCDFFile::write_data_to_ncvar(int ncid_, int varid, const std::vector<size_t>& start, 
                 const std::vector<size_t>& count, const std::vector<std::string>& data) {
    std::vector<const char*> cstrs(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        cstrs[i] = data[i].c_str();
    }
    NC_CHECK(nc_put_vara_string(ncid_, varid, start.data(), count.data(), cstrs.data()), "Writing string value failed");
}

template<typename T>
void NetCDFFile::write_variable_data(const std::string& name, const std::vector<T>& data, size_t start_index) {
    auto var = get_ncvar_by_name(name);
    if (!var) throw std::runtime_error("Variable not found: " + name);
    std::vector<size_t> start = {start_index};
    std::vector<size_t> count = {data.size()};

    write_data_to_ncvar(ncid_, var->get_varid(), start, count, data);

    // We need to construct a map of variable value and the index in the nc file.
    // this will be used while writing the output values.
    // At this moment, we need this only for catchments

    if (name == "catchments") {
        var->build_catchments_index(data.size());
    }
}

void NetCDFFile::write_attribute_to_ncvar(const std::string& name, const std::string& att_name, const std::string& att_value) {
    auto var = get_ncvar_by_name(name);
    if (!var) throw std::runtime_error("Variable not found: " + name);
    double value;
    size_t pos;
    bool is_numeric = true;
    try {
        value = std::stod(att_value, &pos);
    }
    catch (...) {
        is_numeric = false;
    }
    if (pos == att_value.size()){ //needed to ensure that the entire string is a numeric value.
        var->add_attribute(att_name, value);
    }
    else{
        var->add_attribute(att_name, att_value);
    }
}

std::shared_ptr<NetCDFVar> NetCDFFile::get_ncvar(const std::string& name) const
{
    return get_ncvar_by_name(name);
}

void NetCDFFile::end_def_mode()
{
    NC_CHECK(nc_enddef(ncid_), "Switching to writing mode failed");
}

std::shared_ptr<NetCDFVar> NetCDFFile::get_ncvar_by_name(const std::string& name) const {
    auto it = variables_map_.find(name);
    if (it != variables_map_.end()) return it->second;
    return nullptr;
}

void NetCDFFile::load_variables()
{
    int mode = read_only_ ? NC_NOWRITE : NC_WRITE;

    // Load dimensions
    int num_dims;
    NC_CHECK(nc_inq_ndims(ncid_, &num_dims), "Retrieving number of dimensions failed");

    for (int i = 0; i < num_dims; ++i) {
        char dim_name[NC_MAX_NAME + 1];
        size_t dim_len;
        NC_CHECK(nc_inq_dim(ncid_, i, dim_name, &dim_len), "Retrieving dimensions length failed");;
        dims_id_map_[dim_name] = i;
    }

    // Load variables
    int num_variables;
    NC_CHECK(nc_inq_nvars(ncid_, &num_variables), "Retrieving number of variables failed");
    for (int i = 0; i < num_variables; ++i) {
        char var_name[NC_MAX_NAME + 1];
        NC_CHECK(nc_inq_varname(ncid_, i, var_name), "Retrieving variable name failed");

        int num_dims = 0;
        NC_CHECK(nc_inq_varndims(ncid_, i, &num_dims), "Retrieving number of dimensions for variable failed");
        
        nc_type type;
        int num_dims_var;
        int dim_ids[NC_MAX_DIMS];

        NC_CHECK(nc_inq_var(ncid_, i, nullptr, &type, &num_dims_var, dim_ids, nullptr), "Retrieving variable type and dimensions failed");
        
        // Collect dim names
        std::vector<std::string> dim_names;
        std::vector<int> dim_ids_var;
        for (int d = 0; d < num_dims_var; ++d) {
            dim_ids_var.push_back(dim_ids[d]);
            char dim_name[NC_MAX_NAME + 1];
            NC_CHECK(nc_inq_dim(ncid_, dim_ids[d], dim_name, nullptr), "Retrieving domensions name failed");
            dim_names.push_back(dim_name);
        }

        // Create shared_ptr NetCDFVar
        auto nc_var = std::make_shared<NetCDFVar>(var_name, type, dim_ids_var, dim_names, i, ncid_);
        load_attributes(nc_var);
        variables_.push_back(nc_var);
        variables_map_[var_name] = nc_var;
    }
}

void NetCDFFile::load_attributes(std::shared_ptr<NetCDFVar> nc_var) 
{
    int var_id = nc_var->get_varid();
    int retval;
    int num_atts;
    NC_CHECK(nc_inq_varnatts(ncid_, var_id, &num_atts), "Retrieving number of attributes failed");

    for (int att_id = 0; att_id < num_atts; ++att_id) {
        char att_name[NC_MAX_NAME + 1];
        NC_CHECK(nc_inq_attname(ncid_, var_id, att_id, att_name), "Retrieving attribute name failed");

        // Get type and length
        nc_type att_type;
        size_t att_len;
        NC_CHECK(nc_inq_att(ncid_, var_id, att_name, &att_type, &att_len), "Retrieving type and length of attributes failed");

        std::string value;

        switch (att_type) {
            case NC_CHAR: {
                char *data = new char[att_len + 1];
                NC_CHECK(nc_get_att_text(ncid_, var_id, att_name, data), "Retrieving attribute text failed");
                data[att_len] = '\0'; // ensure null termination
                value = std::string(data);
                delete[] data;
                break;
            }
            case NC_INT: {
                std::vector<int> data(att_len);
                NC_CHECK(nc_get_att_int(ncid_, var_id, att_name, data.data()), "Retrieving integer attribute failed");
                if (att_len == 1) value = std::to_string(data[0]);
                else {
                    for (size_t i = 0; i < data.size(); ++i) {
                        value += std::to_string(data[i]);
                        if (i != data.size() - 1) value += ",";
                    }
                }
                break;
            }
            case NC_FLOAT: {
                std::vector<float> data(att_len);
                NC_CHECK(nc_get_att_float(ncid_, var_id, att_name, data.data()), "Retrieving float attribute failed");
                if (att_len == 1) value = std::to_string(data[0]);
                else {
                    for (size_t i = 0; i < data.size(); ++i) {
                        value += std::to_string(data[i]);
                        if (i != data.size() - 1) value += ",";
                    }
                }
                break;
            }
            case NC_DOUBLE: {
                std::vector<double> data(att_len);
                NC_CHECK(nc_get_att_double(ncid_, var_id, att_name, data.data()), "Retrieving double attribute failed");
                if (att_len == 1) value = std::to_string(data[0]);
                else {
                    for (size_t i = 0; i < data.size(); ++i) {
                        value += std::to_string(data[i]);
                        if (i != data.size() - 1) value += ",";
                    }
                }
                break;
            }
            case NC_UINT: {
                std::vector<unsigned int> data(att_len);
                NC_CHECK(nc_inq_att(ncid_, var_id, att_name, &att_type, &att_len), "Retrieving type and length of attributes failed");
                if (att_len == 1) value = std::to_string(data[0]);
                else {
                    for (size_t i = 0; i < data.size(); ++i) {
                        value += std::to_string(data[i]);
                        if (i != data.size()-1) value += ",";
                    }
                }
                break;
            }
            case NC_STRING: {
                char **data = new char*[att_len];
                NC_CHECK(nc_get_att_string(ncid_, var_id, att_name, data), "Retrieving attribute value as string failed");
                if (att_len == 1) value = std::string(data[0]);
                else {
                    for (size_t i = 0; i < att_len; ++i) {
                        value += std::string(data[i]);
                        if (i != att_len - 1) value += ",";
                    }
                }
                NC_CHECK(nc_free_string(att_len, data), "Memory free up failed");
                delete[] data;
                break;
            }
            default:
                value = "<unsupported type>";
        }

        nc_var->add_attribute(std::string(att_name), value, false);
    }
}

std::vector<std::string> NetCDFFile::list_variables() const
{
    std::vector<std::string> names;
    for(const auto& var : variables_) {
        names.push_back(var->get_name());
    }
    return names;
}

void NetCDFFile::close_file() {
    if (ncid_ >= 0) nc_close(ncid_);
}

NetCDFFile::~NetCDFFile() {
    close_file();
}

template void NetCDFFile::write_variable_data(const std::string& name, const std::vector<double>& data, size_t start_index);
template void NetCDFFile::write_variable_data(const std::string& name, const std::vector<int>& data, size_t start_index);
template void NetCDFFile::write_variable_data(const std::string& name, const std::vector<int64_t>& data, size_t start_index);
template void NetCDFFile::write_variable_data(const std::string& name, const std::vector<std::string>& data, size_t start_index);
#endif // NGEN_WITH_NETCDF