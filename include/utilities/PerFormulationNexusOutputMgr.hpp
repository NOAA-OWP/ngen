//
// Created by Robert Bartel on 12/5/25.
//

#ifndef NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP
#define NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP

#include "NexusOutputsMgr.hpp"
#include "netcdf.h"
#if NGEN_WITH_MPI
#if NGEN_WITH_PARALLEL_NETCDF
#include "netcdf_par.h"
#endif
#include "mpi.h"
#endif
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>

// Forward declaration to provide access to protected items in testing
class PerFormulationNexusOutputMgr_Test;

namespace utils
{

    /**
     * Subtype that write/manages per-formulation nexus data files stored in NetCDF format.
     *
     * Because it works with NetCDF files, this file does not write received data entries until ``commit_writes`` is
     * called.
     *
     * While the constructor supports receiving a collection of formulation ids, the class itself can only deal with a
     * single formulation.  As such, the current implementation expects the collection of formulation ids passed to the
     * constructor to either be empty - in which case, a default formulation id is used - or of size 1.
     */
    class PerFormulationNexusOutputMgr : public NexusOutputsMgr
    {

    public:

        /**
         * Construct instance set for managing/writing nexus data files, creating the file(s) appropriately.
         *
         * Note that the class is designed to be aware of its rank in the context of multiple instances operating on the
         * same managed output file (e.g., when there are multiple MPI ranks) and supports multiple instances across one
         * or many processes writing to the same file.  However, only rank `0` will initially create the nexus output
         * file. Further, the instance relies on users/callers to maintain any synchronization (i.e., call MPI_Barrier
         * so other ranks don't get ahead of rank `0` while it creates the file).
         *
         * While the constructor supports receiving a collection of formulation ids, the class itself can only deal with
         * a single formulation.  As such, the current implementation expects the collection of formulation ids passed
         * to the constructor to either be empty - in which case, a default formulation id is used - or of size 1.
         *
         * @param nexus_ids Nexus ids for which this instance manages data (in particular, local nexuses when using MPI).
         * @param formulation_ids Either ``null`` or pointer to vector with at most one formulation id (see above).
         * @param output_root The output root for written files (as a string).
         * @param total_timesteps The total number of timesteps that will be written to the managed file.
         * @param rank The rank of this instance when multiple instances may operate on the same file; e.g., when
         *             using MPI (always `0` if only one instance may operate on the managed file).
         * @param nexuses_per_rank The total number nexuses for each running rank.
         */
        PerFormulationNexusOutputMgr(const std::vector<std::string>& nexus_ids,
                                     std::shared_ptr<std::vector<std::string>> formulation_ids,
                                     const std::string &output_root,
                                     const size_t total_timesteps,
                                     const int rank,
                                     const std::vector<int>& nexuses_per_rank)
            :   nexus_ids(nexus_ids),
                rank(rank),
                nexuses_per_rank(nexuses_per_rank),
                total_timesteps(total_timesteps)
        {
            // TODO: look at potentially making this configurable rather than static
            data_flush_interval = 100;

            // TODO: definitely come back in future and make this configurable (but only when executing with MPI).
            use_collective_nc_var_access = true;

            // TODO: look at making these chunking details more configurable
            //use_chunking = true;
            use_chunking = false;
            // In the nexus_id dimension, set chunk size to be the nexus count
            flow_var_chunk_size_per_dim[0] = 0;
            for (size_t r = 0; r < rank; ++r) {
                flow_var_chunk_size_per_dim[0] += nexuses_per_rank[r];
            }
            // Chunk size is just 1 in the time step dimensions
            flow_var_chunk_size_per_dim[1] = 1;


            if (this->nexuses_per_rank.empty()) {
                if (this->rank != 0)
                    throw std::runtime_error("Must supply nexuses_per_rank values when using multiple MPI processes.");
                this->nexuses_per_rank.push_back(this->nexus_ids.size());
            }

            if (this->nexuses_per_rank.size() <= this->rank) {
                throw std::runtime_error("To few values in nexuses_per_rank value ("
                    + std::to_string(this->nexuses_per_rank.size()) + ") for rank "
                    + std::to_string(rank) + ".");
            }

            if (this->nexuses_per_rank[this->rank] != this->nexus_ids.size()) {
                throw std::runtime_error("Invalid nexuses_per_rank value for rank " + std::to_string(this->rank)
                    + ": " + std::to_string(this->nexuses_per_rank[this->rank]) + " does not match number of "
                    + "supplied nexus ids (" + std::to_string(this->nexus_ids.size()) + ").");
            }

            if (this->rank == 0) {
                for (size_t r = 0; r < this->nexuses_per_rank.size(); r++) {
                    if (this->nexuses_per_rank[r] < 0) {
                        throw std::runtime_error("Invalid nexuses_per_rank value for rank " + std::to_string(r) + ".");
                    }
                }
            }

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

            this->local_offset = 0;

            for (size_t r = 0; r < rank; ++r) {
                this->local_offset += nexuses_per_rank[r];
            }

            nexus_outfile = output_root + "/formulation_" + this->formulation_id + "_nexuses.nc";

            // To support unit testing when MPI and parallel netcdf compiled in (which won't always be running through
            // MPI), we have to detect things.
            #if NGEN_WITH_MPI && NGEN_WITH_PARALLEL_NETCDF
            // If 2+ ranks yes MPI init, all ranks: para create + setup dims/vars
            if (nexuses_per_rank.size() > 1 && isMpiInitialized()) {
                create_netcdf_file_parallel();
                setup_netcdf_metadata();
                set_nc_var_parallel_collective(flow_nc_var_id);
            }
            // If 1 rank, any MPI, rank 0 (i.e., all ranks): reg create + setup dims/vars
            // If 2+ ranks, no MPI init, rank 0: reg create + setup dims/vars
            else if (nexuses_per_rank.size() == 1 || rank == 0) {
                create_netcdf_file();
                setup_netcdf_metadata();
            }
            // If 2+ ranks, no MPI init, not rank 0: just open
            else {
                open_netcdf_file();
                lookup_netcdf_metadata();
            }
            #else

            // If 1 rank or rank 0, reg create
            if (rank == 0) {
                create_netcdf_file();
                setup_netcdf_metadata();
            }
            // If rank > 0, reg open
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

        /**
         * Construct instance set for managing/writing nexus data files, supplying a default value of `0` for MPI rank.
         *
         * @param nexus_ids Nexus ids for which this instance manages data (in particular, local nexuses when using MPI).
         * @param formulation_ids
         * @param output_root The output root for written files (as a string).
         * @param total_timesteps The total number of timesteps that will be written to the managed file.
         */
        PerFormulationNexusOutputMgr(const std::vector<std::string> nexus_ids,
                                     std::shared_ptr<std::vector<std::string>> formulation_ids,
                                     const std::string &output_root,
                                     const size_t total_timesteps)
            : PerFormulationNexusOutputMgr(nexus_ids, formulation_ids, output_root, total_timesteps, 0, {}) {}

        /**
         * Write any received data entries that were not written immediately upon receipt to the managed data files.
         *
         * Function expects/requires data for all local nexus ids to have been received. Since it expects this, it also
         * increments the ::attribute:`current_time_index` value before exiting.
         *
         * Additionally, it clears the current ::attribute:`data_cache` and ::attribute:`current_formulation_id` values.
         */
        void commit_writes() override {
            // If no current formulation id set, that should mean there is nothing to write
            if (current_formulation_id.empty()) {
                return;
            }

            // Get properly ordered data vector
            std::vector<double> data(nexus_ids.size());
            for (size_t i = 0; i < nexus_ids.size(); ++i) {
                // Sanity check we have everything in our block of nexus ids
                if (data_cache.find(nexus_ids[i]) == data_cache.end()) {
                    throw std::runtime_error(
                        "PerFormulationNexusOutputMgr missing data attempting to commit_writes for nexus "
                        + nexus_ids[i] + " in formulation " + current_formulation_id + " at time index "
                        + std::to_string(current_time_index) + ".");
                }
                data[i] = data_cache[nexus_ids[i]];
            }

            int nc_status;

            // On the first write, also write the nexus id variable values
            write_nexus_ids_once();

            // For just rank 0, write the time value
            if (rank == 0) {
                long epoch_minutes = current_epoch_time / 60;
                const size_t start_t = static_cast<size_t>(current_time_index);
                const size_t count_t = 1;
                // TODO: (later) consider if we need to sanity check that times are consistent across ranks (we were
                // TODO:        effectively assuming this to be the case when not explicitly writing times).

                nc_status = nc_put_vara_long(netcdf_file_id, time_nc_var_id, &start_t, &count_t, &epoch_minutes);
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

            nc_status = nc_put_vara_double(netcdf_file_id, flow_nc_var_id, start_f, count_f, data.data());
            if (nc_status != NC_NOERR) {
                throw std::runtime_error("Error writing flow value to nexus file '" + nexus_outfile + "' ("
                    + parse_netcdf_return_code(nc_status) + ") at time index " + std::to_string(current_time_index) + ".");
            }

            current_time_index++;

            // Flush to disk every so often, or on the last timestep
            if (current_time_index % data_flush_interval == 0 || current_time_index == total_timesteps) {
                nc_status = nc_sync(netcdf_file_id);
                if (nc_status != NC_NOERR) {
                    throw std::runtime_error("Error syncing/flushing data to nexus file '" + nexus_outfile + "' after "
                        + "write for time step " + std::to_string(current_time_index) + ": "
                        + parse_netcdf_return_code(nc_status));
                }
            }

            current_epoch_time = 0;
            data_cache.clear();
            current_formulation_id.clear();

            if (current_time_index == total_timesteps) {
                nc_status = nc_close(netcdf_file_id);
                if (nc_status != NC_NOERR) {
                    throw std::runtime_error("Error closing nexus file '" + nexus_outfile + "' after write for "
                        + "FINAL time step " + std::to_string(current_time_index) + ": "
                        + parse_netcdf_return_code(nc_status));
                }
            }
        }

        /**
         * Get a new vector containing the filenames for managed files for this instance.
         *
         * @return Pointer to new vector with the managed files for this instance.
         */
        std::shared_ptr<std::vector<std::string>> get_filenames() {
            auto filenames = std::make_shared<std::vector<std::string>>();
            filenames->push_back(nexus_outfile);
            return filenames;
        }

        /**
         * Receive a data entry for this nexus, specifying details including the formulation id.
         *
         * @param formulation_id The id of the formulation involved in producing this data.
         * @param nexus_id The id for the nexus to which this data applies.
         * @param data_time_marker A marker for the current simulation time for the data.
         * @param flow_data_at_t The nexus flow contribution at this time index (the main data to write).
         */
        void receive_data_entry(const std::string &formulation_id, const std::string &nexus_id,
                                const time_marker &data_time_marker, const double flow_data_at_t) override
        {
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

            data_cache[nexus_id] = flow_data_at_t;
        }

    private:

        #if NGEN_WITH_MPI
        /**
         * Convenience function to determine whether MPI has been initialized.
         *
         * This is used to support cases when the class was compiled for MPI support but is not being executed via MPI.
         * The primary use case for this is testing.
         *
         * @return Whether MPI has been initialized.
         */
        static bool isMpiInitialized() {
            int mpi_init_flag;
            MPI_Initialized(&mpi_init_flag);
            return static_cast<bool>(mpi_init_flag);
        }
        #endif

        static std::string parse_netcdf_return_code(const int nc_status) {
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
            default:
                return "unrecognized error code '" + std::to_string(nc_status) + "'";
            }
        }

        const std::string nc_nex_id_dim_name = std::string("feature_id");
        const std::string nc_time_dim_name = "time";
        const std::string nc_flow_var_name = "runoff_rate";

        /** Array of chunk sizes for  dimensions - nexus_id and time - of flow NetCDF variable, to set chunking. */
        size_t flow_var_chunk_size_per_dim[2];

        /** Map of nexus ids to corresponding cached flow data from ``receive_data_entry``. */
        std::unordered_map<std::string, double> data_cache;

        /**
         * An offset for NetCDF data arrays that indicates the start of where this instance should begin writing data,
         * in particular when multiple instances are writing to the same file and need to not write in overlapping
         * regions.
         */
        std::vector<unsigned long>::size_type local_offset;

        /** Nexus ids for which this instance/process will write data to the file (i.e., local when using MPI, all otherwise). */
        const std::vector<std::string> nexus_ids;

        /** The number of nexuses assigned to each rank. */
        std::vector<int> nexuses_per_rank;

        /** The current/last formulation id value received by `receive_data_entry`. */
        std::string current_formulation_id;

        std::string formulation_id;

        /** Nexus data file path (as string) */
        std::string nexus_outfile;

        /** Current time index of latest ``receive_data_entry``. */
        long current_time_index = 0;

        /** Current epoch timestamp. */
        time_t current_epoch_time = 0;

        /** How many time steps should occur before flushing/syncing data to the NetCDF file. */
        size_t data_flush_interval;

        /** The total number of timesteps that will be written to the managed file. */
        size_t total_timesteps;

        /** File id member variable for opening and writing to NetCDF file via the C API. */
        int netcdf_file_id = -1;

        /** The rank (MPI rank if using MPI) for this instance, which will be 0 if parallel execution is not in use. */
        int rank;

        /** Variable id for the ``nexus_id`` NetCDF variable. */
        int nexus_nc_var_id;

        /** Variable id for the ``time`` NetCDF variable. */
        int time_nc_var_id;

        /** Variable id for the ``flow`` NetCDF variable. */
        int flow_nc_var_id;

        /**
         * Whether to use specified chunking for certain NetCDF variables when creating them in
         * @ref setup_netcdf_metadata.
         */
        bool use_chunking;

        /**
         * Whether instance should use ``NC_COLLECTIVE`` parallel access mode for NetCDF variables (except ``time``).
         *
         * Note that the ``time`` variable is excepted and will always only be written by rank 0, and thus remain
         * ``NC_INDEPENDENT``.
         *
         * See https://docs.unidata.ucar.edu/netcdf-c/4.9.3/parallel_io.html for details on collective/independent
         * access.
         */
        bool use_collective_nc_var_access;

        // For unit testing
        friend class ::PerFormulationNexusOutputMgr_Test;

        /**
         * Add a NetCDF dimension to the managed NetCDF file/dataset, via the C API.
         *
         * @param dim_name The dimension name
         * @param size The dimension size
         * @param dim_id_ptr A point to a location to hold the NetCDF dimension id for the added dimension
         */
        void add_dimension(const std::string& dim_name, const size_t size, int* dim_id_ptr) const {
            int nc_status = nc_def_dim(netcdf_file_id, dim_name.c_str(), size, dim_id_ptr);

            if (nc_status != NC_NOERR) {
                throw std::runtime_error("Cannot add dimension '" + dim_name + "': "
                                         + parse_netcdf_return_code(nc_status));
            }
        }

        /**
         * Add a NetCDF variable to the managed NetCDF file/dataset via the C API.
         *
         * @param var_name The variable name.
         * @param var_type The NetCDF C API variable type code.
         * @param dimension_nc_ids Ordered collection of dimensions nc_ids for the dimensions of the new variable.
         * @param var_id_ptr Pointer to the location in which to save the nc_id for the created variable.
         */
        void add_variable(const std::string& var_name, nc_type var_type, const std::vector<int>& dimension_nc_ids, int* var_id_ptr) const {
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

        /**
         * Add a variable to the managed NetCDF file/dataset, and also put string attributes in place for the variable.
         *
         * @param var_name The variable name.
         * @param var_type The NetCDF variable type code.
         * @param dim_nc_ids Ordered collection of dimensions nc_ids for the dimensions of the new variable.
         * @param attributes Map of name:value strings for attributes for the variable.
         * @param var_id_ptr Pointer to the location in which to save the nc_id for the created variable.
         */
        void add_variable(const std::string& var_name, nc_type var_type, const std::vector<int>& dim_nc_ids, const std::map<std::string, std::string>& attributes, int* var_id_ptr) const {
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

        /**
         * Add a variable, and also put string attributes and a fill value in place for the variable.
         *
         * @param var_name The variable name.
         * @param var_type The NetCDF variable type code.
         * @param dim_nc_ids Ordered collection of dimensions nc_ids for the dimensions of the new variable.
         * @param attributes Map of name:value strings for attributes for the variable.
         * @param fill_value Void pointer to a fill value, which must be of the right actual type for the variable.
         * @param var_id_ptr Pointer to the location in which to save the nc_id for the created variable.
         */
        void add_variable(const std::string& var_name, nc_type var_type, const std::vector<int>& dim_nc_ids, const std::map<std::string, std::string>& attributes, const void* fill_value, int* var_id_ptr) const {
            add_variable(var_name, var_type, dim_nc_ids, attributes, var_id_ptr);
            int nc_status = nc_def_var_fill(netcdf_file_id, *var_id_ptr, NC_FILL, fill_value);
            if (nc_status != NC_NOERR) {
                throw std::runtime_error("Cannot set fill value for variable '" + var_name + "': "
                                         + parse_netcdf_return_code(nc_status));
            }
        }

        /**
         * Create the managed NetCDF file/dataset for regular (i.e., non-parallel) access via the C API.
         *
         * In any successful execution, this function will have the side effect of setting the @ref netcdf_file_id
         * member variable.
         *
         * Note this function is expected to only be called at most once by an instance, during the constructor (though
         * after the @ref nexus_outfile and @ref rank member variables are properly set).  An exception will be thrown
         * if the @ref netcdf_file_id member variable is already set (to something other than ``-1``) when this function
         * is called.
         */
        void create_netcdf_file() {
            // Bail with error if this has already been run
            if (netcdf_file_id != -1) {
                throw std::runtime_error("Cannot create netCDF file after already created and have nc_id set.");
            }
            std::cout << "Creating nexus NetCDF file '" << nexus_outfile << "' for regular access." << std::endl;
            int nc_status = nc_create(nexus_outfile.c_str(), NC_NETCDF4 | NC_NOCLOBBER, &netcdf_file_id);
            if (nc_status != NC_NOERR) {
                throw std::runtime_error("PerFormulationNexusOutputMgr rank " + std::to_string(rank) + " could not "
                    + "create file '" + nexus_outfile + "' for regular access: " + parse_netcdf_return_code(nc_status));
            }
        }

        #if NGEN_WITH_MPI && NGEN_WITH_PARALLEL_NETCDF
        /**
         * Create the managed NetCDF file/dataset for parallel access via the C API.
         *
         * In any successful execution, this function will have the side effect of setting the @ref netcdf_file_id
         * member variable.
         *
         * Note this function is expected to only be called at most once by an instance, during the constructor (though
         * after the @ref nexus_outfile and @ref rank member variables are properly set).  An exception will be thrown
         * if the @ref netcdf_file_id member variable is already set (to something other than ``-1``) when this function
         * is called.
         */
        void create_netcdf_file_parallel() {
            // Bail with error if this has already been run
            if (netcdf_file_id != -1) {
                throw std::runtime_error("Cannot (parallel) create netCDF file after already created and have nc_id set.");
            }
            std::cout << "Creating nexus NetCDF file '" << nexus_outfile << "' for parallel access." << std::endl;
            int nc_status = nc_create_par(nexus_outfile.c_str(), NC_NETCDF4 | NC_NOCLOBBER, MPI_COMM_WORLD, MPI_INFO_NULL, &netcdf_file_id);
            if (nc_status != NC_NOERR) {
                throw std::runtime_error("PerFormulationNexusOutputMgr rank " + std::to_string(rank) + " could not "
                    + "create file '" + nexus_outfile + "' for parallel access: " + parse_netcdf_return_code(nc_status));
            }
        }
        #endif

        /**
         * Function to open an existing NetCDF file/dataset and set the @ref netcdf_file_id member variable.
         *
         * In any successful execution, this function will have the side effect of setting the @ref netcdf_file_id
         * member variable.
         *
         * Primarily intended to be run in constructor, in (somewhat less common) cases when there are multiple
         * instances all running in the same process (i.e., without MPI).  In such cases, parallel NetCDF capabilities
         * are not needed, so only one "create" function call is needed, but other instances still need a way to get
         * the NetCDF id.
         *
         */
        void open_netcdf_file() {
            // Bail with error if this has already been run
            if (netcdf_file_id != -1) {
                throw std::runtime_error("Cannot open netCDF file after already have nc_id set.");
            }
            // While I thought about retries, because things are serial in this scenario, if it doesn't work the
            // first time, it still won't for the second, etc.
            int nc_status = nc_open(nexus_outfile.c_str(), NC_NETCDF4 | NC_NOCLOBBER, &netcdf_file_id);
            if (nc_status != NC_NOERR) {
                throw std::runtime_error("PerFormulationNexusOutputMgr rank " + std::to_string(rank) + " could not "
                                         + " open file '" + nexus_outfile + "': "
                                         + parse_netcdf_return_code(nc_status));
            }
        }

        /**
         * For cases when only opening a NetCDF file/dataset, retrieve necessary variable metadata for execution.
         *
         * Function will lookup and store the variable ids for the nexus_id, time, and flow/runoff variables.
         */
        void lookup_netcdf_metadata() {
            int nc_status = nc_inq_varid(netcdf_file_id, nc_nex_id_dim_name.c_str(), &nexus_nc_var_id);
            if (nc_status != NC_NOERR) {
                throw std::runtime_error("Failed to lookup Nexus_id NetCDF variable: " + parse_netcdf_return_code(nc_status));
            }
            nc_status = nc_inq_varid(netcdf_file_id, nc_time_dim_name.c_str(), &time_nc_var_id);
            if (nc_status != NC_NOERR) {
                throw std::runtime_error("Failed to lookup Time NetCDF variable: " + parse_netcdf_return_code(nc_status));
            }
            nc_status = nc_inq_varid(netcdf_file_id, nc_flow_var_name.c_str(), &flow_nc_var_id);
            if (nc_status != NC_NOERR) {
                throw std::runtime_error("Failed to lookup Flow NetCDF variable: " + parse_netcdf_return_code(nc_status));
            }
        }

        /**
         * Setup the NetCDF metadata (dimensions and variables) for the managed NetCDF file/dataset.
         *
         * Function will create and store the variable ids for the nexus_id, time, and flow/runoff variables.  To
         * support those variables, it will also create the nexus_id and time dimensions, though not store those ids.
         */
        void setup_netcdf_metadata() {
            int nc_nex_id_dim_id, nc_time_dim_id;

            size_t total_nexuses = 0;
            for (const int n : nexuses_per_rank) {
                total_nexuses += n;
            }

            add_dimension(nc_nex_id_dim_name, total_nexuses, &nc_nex_id_dim_id);
            add_dimension(nc_time_dim_name, total_timesteps, &nc_time_dim_id);

            std::map<std::string, std::string> nex_id_var_attrs = {{"long_name", "Feature ID"}};
            add_variable(nc_nex_id_dim_name, NC_UINT, {nc_nex_id_dim_id}, nex_id_var_attrs, &nexus_nc_var_id);

            std::map<std::string, std::string> time_var_attrs = {
                {"units", "minutes since 1970-01-01 00:00:00"},
                {"calendar", "gregorian"},
                {"long_name", "Time"}
            };
            add_variable(nc_time_dim_name, NC_UINT, {nc_time_dim_id}, time_var_attrs, &time_nc_var_id);

            std::map<std::string, std::string> flow_var_attrs = {
                {"units", "m3 s-1"},
                {"long_name", "Simulated Surface Runoff"}
            };
            const double fill_value = -9999.0;
            add_variable(nc_flow_var_name, NC_DOUBLE, {nc_nex_id_dim_id, nc_time_dim_id}, flow_var_attrs,
                         &fill_value, &flow_nc_var_id);

            if (use_chunking) {
                std::cout << "Setting nexus NetCDF variable '" << nc_flow_var_name << "' up for chunking ("
                          << std::to_string(flow_var_chunk_size_per_dim[0]) + "," + std::to_string(flow_var_chunk_size_per_dim[1])
                          << std::endl;
                int nc_status = nc_def_var_chunking(netcdf_file_id, flow_nc_var_id, NC_CHUNKED, flow_var_chunk_size_per_dim);
                if (nc_status != NC_NOERR) {
                    throw std::runtime_error("Could not set up chunking for variable '" + nc_flow_var_name + "': "
                                             + parse_netcdf_return_code(nc_status));
                }
            }
        }

        /**
         * On the first time step, write this instance's managed nexus ids to the NetCDF data.
         */
        void write_nexus_ids_once() const {
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

            int nc_status = nc_put_vara_uint(netcdf_file_id, nexus_nc_var_id, start.data(), count.data(),
                                          numeric_nex_ids.data());
            if (nc_status != NC_NOERR) {
                throw std::runtime_error("Error writing nexus ids to netcdf for nexus output manager: "
                    + parse_netcdf_return_code(nc_status));
            }
        }

        #if NGEN_WITH_MPI && NGEN_WITH_PARALLEL_NETCDF
        /**
         * Set the NetCDF variable with the given name to have ``NC_COLLECTIVE`` parallel access.
         *
         * @param var_name The variable name
         */
        void set_nc_var_parallel_collective(const std::string& var_name) {
            int* nc_var_id;
            if (var_name == nc_nex_id_dim_name) {
                nc_var_id = &nexus_nc_var_id;
            }
            else if (var_name == nc_time_dim_name) {
                nc_var_id = &time_nc_var_id;
            }
            else if (var_name == nc_flow_var_name) {
                nc_var_id = &flow_nc_var_id;
            }
            else {
                throw std::runtime_error("Can't set NC_COLLECTIVE for invalid variable name '" + var_name + "'.");
            }
            set_nc_var_parallel_collective(*nc_var_id);
        }

        /**
         * Set the NetCDF variable with the given variable id to have ``NC_COLLECTIVE`` parallel access.
         *
         * @param nc_var_id The numeric NetCDF variable id
         */
        void set_nc_var_parallel_collective(const int nc_var_id) const {
            int nc_status = nc_var_par_access(netcdf_file_id, nc_var_id, NC_COLLECTIVE);

            if (nc_status == NC_NOERR) {
                std::cout << "Setting NetCDF var '" << std::to_string(nc_var_id) << "' to NC_COLLECTIVE." << std::endl;
                return;
            }
            throw std::runtime_error("Failed to set variable id '" + std::to_string(nc_var_id) + "' to NC_COLLECTIVE "
                                     + "access: " + parse_netcdf_return_code(nc_status));
        }
        #endif

    };
} // utils

#endif //NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP
