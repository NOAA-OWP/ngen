/*
Created by Robert Bartel on 12/5/25.
Copyright (C) 2025-2026 Lynker
------------------------------------------------------------------------
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
------------------------------------------------------------------------
*/

#include "PerFormulationNexusOutputMgr.hpp"

utils::PerFormulationNexusOutputMgr::PerFormulationNexusOutputMgr(
    const std::vector<std::string>& nexus_ids,
    std::shared_ptr<std::vector<std::string>> formulation_ids,
    const std::string& output_root,
    const size_t total_timesteps,
    const int obj_id,
    const int local_offset,
    const int instance_count,
    const int total_nexus_count) : local_offset(local_offset),
                                   nexus_ids(nexus_ids),
                                   total_timesteps(total_timesteps),
                                   obj_id(obj_id),
                                   total_nexus_count(total_nexus_count)
{
    // This instance will later create a NetCDF variable for Time that is of type NC_INT64 and then subsequently
    // put values into it using the nc_put_vara_longlong function.  Because NC_INT64 values appear to always be
    // 8 bytes (https://docs.unidata.ucar.edu/nug/current/md_types.html#data_type), we are sanity checking up
    // front:
    if (sizeof(long long) != 8) {
        throw std::runtime_error("Unsupported platform for PerFormulationNexusOutputMgr: `long long` values "
            "are not the same size as an NC_INT64 var");
    }

    // TODO: look at potentially making this configurable rather than static
    data_flush_interval = 100;

    // TODO: definitely come back in future and make this configurable (but only when executing with MPI).
    use_collective_nc_var_access = true;

    // TODO: look at making these chunking details more configurable
    use_chunking_flow_var = true;
    // In the nexus_id dimension, set chunk size to be the nexus count
    flow_var_chunk_size_per_dim[0] = total_nexus_count;
    // Chunk size is just 1 in the time step dimensions
    flow_var_chunk_size_per_dim[1] = 1;

    // TODO: (later) in future modify if we decide this class needs to support multiple formulations
    // Should always have one formulation at least, so
    if (formulation_ids == nullptr || formulation_ids->empty()) {
        this->formulation_id = get_default_formulation_id();
    }
    else if (formulation_ids->size() > 1) {
        throw std::runtime_error("PerFormulationNexusOutputMgr currently cannot have more than one formulation id.");
    }
    else {
        this->formulation_id = (*formulation_ids)[0];
    }

    // Initialize data structure for holding nexus data and set with default of fill value
    current_nexus_data = std::vector<double>(nexus_ids.size(), flow_var_fill_value);

    // Initialize data structure for holding the index of a given nexus in some other instance data structures
    for (size_t n = 0; n < nexus_ids.size(); ++n) {
        nexus_data_indices[this->nexus_ids[n]] = n;
    }

    nexus_outfile = output_root + "/formulation_" + this->formulation_id + "_nexuses.nc";

    // To support unit testing when not running via MPI, but when MPI and parallel netcdf are compiled in, we
    // have to detect things.
#if NGEN_WITH_MPI && NGEN_WITH_PARALLEL_NETCDF
    // If:      >=2 mgr instances in simulation **AND** MPI is initialized
    // Then:    true parallel execution use case, where instances in all ranks need to run same commands
    // ->       for all obj_ids/ranks, run parallel create + setup dims/vars
    if (instance_count > 1 && isMpiInitialized()) {
        create_netcdf_file_parallel(MPI_COMM_WORLD);
        setup_netcdf_metadata();
        if (use_collective_nc_var_access) {
            set_nc_var_parallel_collective(nc_var_id_nexus_id);
            set_nc_var_parallel_collective(nc_var_id_flow);
        }
    }
    // If:      1 mgr instance in simulation
    //          **OR**
    //          >=2 mgr instances in simulation **AND** MPI not initialized **AND** "this" is obj_id 0
    // Then:    this is obj_id 0 in a non-parallel use case (either nothing to parallelize or not initialized)
    // ->       call non-parallel create + setup dims/vars
    else if (instance_count == 1 || obj_id == 0) {
        create_netcdf_file();
        setup_netcdf_metadata();
    }
    // If:      >=2 mgr instances in simulation **AND** MPI not initialized **AND** "this" is **not** obj_id 0
    // Then:    this is some non-primary obj_id in a non-parallel use case (obj_id 0 creates, so just open/lookup)
    // ->       call open + lookup dims/vars
    else {
        open_netcdf_file();
        lookup_netcdf_metadata();
    }
#else

    // As with MPI-case code above, to support testing, support possibility of multiple instances opened,
    // with obj_id 0 being the "primary".
    // As such, only let obj_id 0 create and setup the file; have any others just open and lookup metadata
    if (obj_id == 0) {
        create_netcdf_file();
        setup_netcdf_metadata();
    }
    else {
        open_netcdf_file();
        lookup_netcdf_metadata();
    }
#endif

    int nc_status = nc_sync(netcdf_file_id);
    if (nc_status != NC_NOERR) {
        throw std::runtime_error("Could not sync metadata during setup of '" + nexus_outfile + "': "
            + parse_netcdf_return_code(nc_status));
    }
}

utils::PerFormulationNexusOutputMgr::PerFormulationNexusOutputMgr(
    const std::vector<std::string> nexus_ids,
    std::shared_ptr<std::vector<std::string>> formulation_ids,
    const std::string& output_root,
    const size_t total_timesteps)
        : PerFormulationNexusOutputMgr(nexus_ids, formulation_ids, output_root, total_timesteps, 0, 0, 1,
                         nexus_ids.size()
                                      ) {}

utils::PerFormulationNexusOutputMgr::~PerFormulationNexusOutputMgr() {
    if (netcdf_file_id != -1) {
        std::cout << "WARN: Nexus NetCDF file manager being destroyed before file was otherwise closed.\n";
        try {
            close_netcdf_file();
        }
        catch (const std::exception& e) {
            std::cout << "ERROR: Could not close nexus Netcdf file during manager destruction: " << e.what()
                << std::endl;
        }
    }
}

void utils::PerFormulationNexusOutputMgr::commit_writes() {
    // If no current formulation id set, that should mean there is nothing to write
    if (current_formulation_id.empty()) {
        return;
    }

    int nc_status;

    // On the first write, also write the nexus id variable values
    write_nexus_ids_once();

    // For just obj_id 0, write the time value
    if (obj_id == 0) {
        long long epoch_minutes = current_epoch_time / 60;
        const size_t start_t = static_cast<size_t>(current_time_index);
        const size_t count_t = 1;
        // TODO: (later) consider if we need to sanity check that times are consistent across obj_ids (we were
        // TODO:        effectively assuming this to be the case when not explicitly writing times).

        nc_status = nc_put_vara_longlong(netcdf_file_id, nc_var_id_time, &start_t, &count_t, &epoch_minutes);
        if (nc_status != NC_NOERR) {
            throw std::runtime_error("Error writing time value to nexus file '" + nexus_outfile + "' ("
                + parse_netcdf_return_code(nc_status) + ") at time index " + std::to_string(current_time_index) + ".");
        }
    }

    // Assume base on how constructor was set up (imply for conciseness)
    //size_t nexus_dim_index = 0;
    //size_t time_dim_index = 1;
    const size_t start_f[2] = {local_offset, static_cast<size_t>(current_time_index)};
    const size_t count_f[2] = {nexus_ids.size(), 1};

    // TODO: perhaps later a configurable option about whether we should thrown an exception if any nexuses
    //  didn't have a data value set (i.e., are still set to fill value)
    nc_status = nc_put_vara_double(netcdf_file_id, nc_var_id_flow, start_f, count_f, current_nexus_data.data());
    if (nc_status != NC_NOERR) {
        throw std::runtime_error("Error writing flow value to nexus file '" + nexus_outfile + "' ("
            + parse_netcdf_return_code(nc_status) + ") at time index " + std::to_string(current_time_index) + ".");
    }

    current_time_index++;

    // Trigger a flush to disk every so often
    // It might be nice to utilize NetCDF's built-in chunk caching, but with MPI it looks like we can't:
    //  https://support.hdfgroup.org/documentation/hdf5/latest/group___f_a_p_l.html#ga034a5fc54d9b05296555544d8dd9fe89
    if (current_time_index % data_flush_interval == 0) {
        nc_status = nc_sync(netcdf_file_id);
        if (nc_status != NC_NOERR) {
            throw std::runtime_error("Error syncing/flushing data to nexus file '" + nexus_outfile + "' after "
                + "write for time step " + std::to_string(current_time_index) + ": "
                + parse_netcdf_return_code(nc_status));
        }
    }

    current_epoch_time = 0;
    // Reset all current values to default of fill value now that they are no longer current
    for (size_t i = 0; i < current_nexus_data.size(); ++i) {
        current_nexus_data[i] = flow_var_fill_value;
    }
    current_formulation_id.clear();

    if (current_time_index == total_timesteps) {
        close_netcdf_file();
    }
}

std::shared_ptr<std::vector<std::string>> utils::PerFormulationNexusOutputMgr::get_filenames() {
    auto filenames = std::make_shared<std::vector<std::string>>();
    filenames->push_back(nexus_outfile);
    return filenames;
}

void utils::PerFormulationNexusOutputMgr::receive_data_entry(const std::string& formulation_id,
                                                             const std::string& nexus_id,
                                                             const time_marker& data_time_marker,
                                                             const double flow_data_at_t) {
    if (current_formulation_id.empty()) {
        current_formulation_id = formulation_id;
    }
    else if (current_formulation_id != formulation_id) {
        throw std::runtime_error(
            "Cannot receive data for formulation " + formulation_id + " for nexus " + nexus_id +
            " when expecting data for " + current_formulation_id + ".");
    }

    if (data_time_marker.sim_time_index != current_time_index) {
        throw std::runtime_error(
            "Cannot receive data for formulation " + formulation_id + " for nexus " + nexus_id +
            " at time index " + std::to_string(data_time_marker.sim_time_index) + " when expecting data for "
            + "time index " + std::to_string(current_time_index) + ".");
    }

    if (current_epoch_time == 0) {
        current_epoch_time = data_time_marker.epoch_time;
    }
    else if (current_epoch_time != data_time_marker.epoch_time) {
        throw std::runtime_error(
            "Cannot receive data for formulation " + formulation_id + " for nexus " + nexus_id +
            ": expected '" + std::to_string(current_epoch_time) + "' for epoch time at current time " +
            "index " + std::to_string(current_time_index) + " but got '"
            + std::to_string(data_time_marker.epoch_time) + "'.");
    }

    current_nexus_data[nexus_data_indices.at(nexus_id)] = flow_data_at_t;
}

#if NGEN_WITH_MPI
bool utils::PerFormulationNexusOutputMgr::isMpiInitialized() {
    int mpi_init_flag;
    MPI_Initialized(&mpi_init_flag);
    return static_cast<bool>(mpi_init_flag);
}
#endif

std::string utils::PerFormulationNexusOutputMgr::parse_netcdf_return_code(const int nc_status) {
    switch (nc_status) {
    case NC_NOERR:
        return "";
    case NC_EBADID:
        return "not a valid NetCDF ID";
    case NC_EBADNAME:
        return "not a valid NetCDF name";
    case NC_EBADGRPID:
        return "bad group id or nc_id that does not contain group id";
    case NC_EDIMSIZE:
        return "invalid dimension size";
    case NC_EEXIST:
        return "already exists";
    case NC_EFILEMETA:
        return "error with NetCDF-4 file-level metadata in HDF5 file (NetCDF-4 files only)";
    case NC_EGLOBAL:
        return "attempting to set on NC_GLOBAL";
    case NC_EHDFERR:
        return "HDF5 error (NetCDF-4 files only)";
    case NC_EINVAL:
        return "invalid input params to backing netcdf function";
    case NC_ELATEDEF:
        return "attempting to set after data is written";
    case NC_EMAXDIMS:
        return "maximum number of dimensions exceeded";
    case NC_EMAXNAME:
        return "name too long";
    case NC_ENAMEINUSE:
        return "name already in use";
    case NC_ENFILE:
        return "too many files open";
    case NC_ENOMEM:
        return "system out of or otherwise unable to allocate memory";
    case NC_ENOPAR:
        return "library was not built with parallel I/O features";
    case NC_ENOTBUILT:
        return "library was not built with NetCDF-4 or PnetCDF";
    case NC_ENOTINDEFINE:
        return "not in define mode";
    case NC_ENOTVAR:
        return "not a variable";
    case NC_EPERM:
        return "insufficient permissions or writing to read-only item";
    case NC_EUNLIMIT:
        return "unlimited dimension size already in use";
    case NC_EINVALCOORDS:
        return "index exceeds dimension bounds";
    case NC_EBADCHUNK:
        return "bad chunk size";
    default:
        return "unrecognized error code '" + std::to_string(nc_status) + "'";
    }
}

void utils::PerFormulationNexusOutputMgr::add_dimension(const std::string& dim_name, const size_t size,
                                                        int* dim_id_ptr) const {
    int nc_status = nc_def_dim(netcdf_file_id, dim_name.c_str(), size, dim_id_ptr);

    if (nc_status != NC_NOERR) {
        throw std::runtime_error("Cannot add dimension '" + dim_name + "': "
            + parse_netcdf_return_code(nc_status));
    }
}

void utils::PerFormulationNexusOutputMgr::add_variable(const std::string& var_name, nc_type var_type,
                                                       const std::vector<int>& dimension_nc_ids,
                                                       int* var_id_ptr) const {
    int nc_status = nc_def_var(netcdf_file_id, var_name.c_str(), var_type, dimension_nc_ids.size(), dimension_nc_ids.data(), var_id_ptr);
    if (nc_status == NC_NOERR) {
        return;
    }
    std::string dim_str = std::to_string(dimension_nc_ids[0]);
    for (size_t i = 1; i < dimension_nc_ids.size(); ++i) {
        dim_str += "," + std::to_string(dimension_nc_ids[i]);
    }
    throw std::runtime_error("Cannot add variable '" + var_name + "' with nc_type " + std::to_string(var_type)
        + " and dims [" + dim_str + "]: " + parse_netcdf_return_code(nc_status));
}

void utils::PerFormulationNexusOutputMgr::add_variable(const std::string& var_name, nc_type var_type,
                                                       const std::vector<int>& dim_nc_ids,
                                                       const std::map<std::string, std::string>& attributes,
                                                       int* var_id_ptr) const {
    add_variable(var_name, var_type, dim_nc_ids, var_id_ptr);
    int nc_status;
    for (std::pair<const std::string, const std::string> attr_pair : attributes) {
        const char* val_cstr[] = {const_cast<char*>(attr_pair.second.c_str())};
        nc_status = nc_put_att_string(netcdf_file_id, *var_id_ptr, attr_pair.first.c_str(), 1, val_cstr);
        if (nc_status != NC_NOERR) {
            throw std::runtime_error("Cannot add attribute '" + attr_pair.first + "' with nc_type "
                + std::to_string(var_type) + " for variable '" + var_name + "': "
                + parse_netcdf_return_code(nc_status));
        }
    }
}

void utils::PerFormulationNexusOutputMgr::add_variable(const std::string& var_name, nc_type var_type,
                                                       const std::vector<int>& dim_nc_ids,
                                                       const std::map<std::string, std::string>& attributes,
                                                       const void* fill_value,
                                                       int* var_id_ptr) const {
    add_variable(var_name, var_type, dim_nc_ids, attributes, var_id_ptr);
    int nc_status = nc_def_var_fill(netcdf_file_id, *var_id_ptr, NC_FILL, fill_value);
    if (nc_status != NC_NOERR) {
        throw std::runtime_error("Cannot set fill value for variable '" + var_name + "': "
            + parse_netcdf_return_code(nc_status));
    }
}

void utils::PerFormulationNexusOutputMgr::create_netcdf_file() {
    // Bail with error if this has already been run
    if (netcdf_file_id != -1) {
        throw std::runtime_error("Cannot create netCDF file after already created and have nc_id set.");
    }
    std::cout << "Creating nexus NetCDF file '" << nexus_outfile << "' for regular access." << std::endl;
    int nc_status = nc_create(nexus_outfile.c_str(), NC_NETCDF4 | NC_NOCLOBBER, &netcdf_file_id);
    if (nc_status != NC_NOERR) {
        throw std::runtime_error("PerFormulationNexusOutputMgr id " + std::to_string(obj_id) + " could not "
            + "create file '" + nexus_outfile + "' for regular access: " + parse_netcdf_return_code(nc_status));
    }
}

#if NGEN_WITH_MPI && NGEN_WITH_PARALLEL_NETCDF
void utils::PerFormulationNexusOutputMgr::create_netcdf_file_parallel(MPI_Comm mpi_comm) {
    // Bail with error if this has already been run
    if (netcdf_file_id != -1) {
        throw std::runtime_error("Cannot (parallel) create netCDF file after already created and have nc_id set.");
    }
    std::cout << "Creating nexus NetCDF file '" << nexus_outfile << "' for parallel access." << std::endl;
    int nc_status = nc_create_par(nexus_outfile.c_str(), NC_NETCDF4 | NC_NOCLOBBER, mpi_comm, MPI_INFO_NULL, &netcdf_file_id);
    if (nc_status != NC_NOERR) {
        throw std::runtime_error("PerFormulationNexusOutputMgr id " + std::to_string(obj_id) + " could not "
            + "create file '" + nexus_outfile + "' for parallel access: " + parse_netcdf_return_code(nc_status));
    }
}
#endif

void utils::PerFormulationNexusOutputMgr::close_netcdf_file() {
    int nc_status = nc_close(netcdf_file_id);
    if (nc_status != NC_NOERR) {
        throw std::runtime_error("Error closing nexus file '" + nexus_outfile + "' after write for "
            + "FINAL time step " + std::to_string(current_time_index) + ": "
            + parse_netcdf_return_code(nc_status));
    }
    netcdf_file_id = -1;
}

void utils::PerFormulationNexusOutputMgr::open_netcdf_file() {
    // Bail with error if this has already been run
    if (netcdf_file_id != -1) {
        throw std::runtime_error("Cannot open netCDF file after already have nc_id set.");
    }
    // While I thought about retries, because things are serial in this scenario, if it doesn't work the
    // first time, it still won't for the second, etc.
    int nc_status = nc_open(nexus_outfile.c_str(), NC_WRITE, &netcdf_file_id);
    if (nc_status != NC_NOERR) {
        throw std::runtime_error("PerFormulationNexusOutputMgr obj_id " + std::to_string(obj_id) + " could not "
            + " open file '" + nexus_outfile + "': "
            + parse_netcdf_return_code(nc_status));
    }
}

void utils::PerFormulationNexusOutputMgr::lookup_netcdf_metadata() {
    int nc_status = nc_inq_varid(netcdf_file_id, nc_dim_name_nexus_id.c_str(), &nc_var_id_nexus_id);
    if (nc_status != NC_NOERR) {
        throw std::runtime_error("Failed to lookup Nexus_id NetCDF variable: " + parse_netcdf_return_code(nc_status));
    }
    nc_status = nc_inq_varid(netcdf_file_id, nc_dim_name_time.c_str(), &nc_var_id_time);
    if (nc_status != NC_NOERR) {
        throw std::runtime_error("Failed to lookup Time NetCDF variable: " + parse_netcdf_return_code(nc_status));
    }
    nc_status = nc_inq_varid(netcdf_file_id, nc_var_name_flow.c_str(), &nc_var_id_flow);
    if (nc_status != NC_NOERR) {
        throw std::runtime_error("Failed to lookup Flow NetCDF variable: " + parse_netcdf_return_code(nc_status));
    }
}

void utils::PerFormulationNexusOutputMgr::setup_netcdf_metadata() {
    int nc_nex_id_dim_id, nc_time_dim_id;

    add_dimension(nc_dim_name_nexus_id, total_nexus_count, &nc_nex_id_dim_id);
    add_dimension(nc_dim_name_time, total_timesteps, &nc_time_dim_id);

    std::map<std::string, std::string> nex_id_var_attrs = {{"long_name", "Feature ID"}};
    add_variable(nc_dim_name_nexus_id, NC_UINT, {nc_nex_id_dim_id}, nex_id_var_attrs, &nc_var_id_nexus_id);

    std::map<std::string, std::string> time_var_attrs = {
        {"units", "minutes since 1970-01-01 00:00:00"},
        {"calendar", "gregorian"},
        {"long_name", "Time"}
    };
    add_variable(nc_dim_name_time, NC_INT64, {nc_time_dim_id}, time_var_attrs, &nc_var_id_time);

    std::map<std::string, std::string> flow_var_attrs = {
        {"units", "m3 s-1"},
        {"long_name", "Simulated Surface Runoff"}
    };
    add_variable(nc_var_name_flow, NC_DOUBLE, {nc_nex_id_dim_id, nc_time_dim_id}, flow_var_attrs,
                 &flow_var_fill_value, &nc_var_id_flow);

    if (use_chunking_flow_var) {
        std::cout << "Setting nexus NetCDF variable '" << nc_var_name_flow << "' up for chunking ("
            << std::to_string(flow_var_chunk_size_per_dim[0]) + "," + std::to_string(flow_var_chunk_size_per_dim[1])
            << ")" << std::endl;
        int nc_status = nc_def_var_chunking(netcdf_file_id, nc_var_id_flow, NC_CHUNKED, flow_var_chunk_size_per_dim);
        if (nc_status != NC_NOERR) {
            throw std::runtime_error("Could not set up chunking for variable '" + nc_var_name_flow + "': "
                + parse_netcdf_return_code(nc_status));
        }
    }
}

void utils::PerFormulationNexusOutputMgr::write_nexus_ids_once() const {
    // Only do anything on the first time step
    if (current_time_index != 0)
        return;

    std::vector<unsigned int> numeric_nex_ids(nexus_ids.size());
    const char delimiter = '-';

    for (size_t i = 0; i < nexus_ids.size(); ++i) {
        const std::string::size_type pos = nexus_ids[i].find(delimiter);
        if (pos == std::string::npos) {
            throw std::runtime_error("Invalid nexus id '" + nexus_ids[i] + "' (no delimiter '"
                + std::string(1, delimiter) + "').");
        }
        numeric_nex_ids[i] = std::stoi(nexus_ids[i].substr(pos + 1));
    }

    std::vector<size_t> start{this->local_offset};
    std::vector<size_t> count{numeric_nex_ids.size()};

    int nc_status = nc_put_vara_uint(netcdf_file_id, nc_var_id_nexus_id, start.data(), count.data(),
                                     numeric_nex_ids.data());
    if (nc_status != NC_NOERR) {
        throw std::runtime_error("Error writing nexus ids to netcdf for nexus output manager: "
            + parse_netcdf_return_code(nc_status));
    }
}

#if NGEN_WITH_MPI && NGEN_WITH_PARALLEL_NETCDF
void utils::PerFormulationNexusOutputMgr::set_nc_var_parallel_collective(const int nc_var_id) const {
    int nc_status = nc_var_par_access(netcdf_file_id, nc_var_id, NC_COLLECTIVE);

    if (nc_status == NC_NOERR) {
        std::cout << "Setting NetCDF var '" << std::to_string(nc_var_id) << "' to NC_COLLECTIVE." << std::endl;
        return;
    }
    throw std::runtime_error("Failed to set variable id '" + std::to_string(nc_var_id) + "' to NC_COLLECTIVE "
        + "access: " + parse_netcdf_return_code(nc_status));
}
#endif
