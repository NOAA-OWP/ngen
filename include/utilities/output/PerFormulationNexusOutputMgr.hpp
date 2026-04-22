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

#ifndef NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP
#define NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP

#if NGEN_WITH_NETCDF

#include "NexusOutputsMgr.hpp"
#include "netcdf.h"
#include <iostream>
#if NGEN_WITH_MPI
#if NGEN_WITH_PARALLEL_NETCDF
#include "netcdf_par.h"
#endif
#include "mpi.h"
#endif
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <math.h>

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
         * Note that the class is designed to be aware of its object id in the context of multiple instances operating
         * on the same managed output file (e.g., when there are multiple MPI ranks, in which case @ref obj_id and MPI
         * rank are synonymous) and supports multiple instances across one or many processes writing to the same file.
         * However, only obj_id `0` will initially create the nexus output file. Further, the instance relies on
         * users/callers to maintain any synchronization (i.e., call MPI_Barrier so other obj_ids don't get ahead of
         * obj_id `0` while it creates the file).
         *
         * While the constructor supports receiving a collection of formulation ids, the class itself can only deal with
         * a single formulation.  As such, the current implementation expects the collection of formulation ids passed
         * to the constructor to either be empty - in which case, a default formulation id is used - or of size 1.
         *
         * @param nexus_ids Nexus ids for which this instance manages data (in particular, local nexuses when using MPI).
         * @param formulation_ids Either ``null`` or pointer to vector with at most one formulation id (see above).
         * @param output_root The output root for written files (as a string).
         * @param total_timesteps The total number of timesteps that will be written to the managed file.
         * @param obj_id The obj_id of this instance when multiple instances may operate on the same file; same as rank
         *               when using MPI (always `0` if only one instance may operate on the managed file).
         * @param local_offset An offset for NetCDF data arrays that indicates the start of where this instance should
         *                     begin writing data, in particular when multiple instances are writing to the same file
         *                     and need to not write in overlapping regions.
         * @param instance_count The total number of instances of this type in use across an entire simulation, which is
         *                       usually (though not always) the number of processes/ranks.
         * @param total_nexus_count The total number of nexuses involved throughout the entire current simulation.
         */
        PerFormulationNexusOutputMgr(const std::vector<std::string>& nexus_ids,
                                     std::shared_ptr<std::vector<std::string>> formulation_ids,
                                     const std::string &output_root,
                                     size_t total_timesteps,
                                     int obj_id,
                                     int local_offset,
                                     int instance_count,
                                     int total_nexus_count);

        /**
         * Construct instance set for managing/writing nexus data files when there is only one obj_id/instance.
         *
         * @param nexus_ids Nexus ids for which this instance manages data (in particular, local nexuses when using MPI).
         * @param formulation_ids
         * @param output_root The output root for written files (as a string).
         * @param total_timesteps The total number of timesteps that will be written to the managed file.
         */
        PerFormulationNexusOutputMgr(const std::vector<std::string> nexus_ids,
                                     std::shared_ptr<std::vector<std::string>> formulation_ids,
                                     const std::string &output_root,
                                     const size_t total_timesteps);

        ~PerFormulationNexusOutputMgr() override;

        /**
         * Close this manager instance and the managed NetCDF file.
         *
         * Close the managed NetCDF file and (if successful) clear the corresponding file id by resetting
         * @ref netcdf_file_id to `-1`.
         *
         * Once an instance is closed, it cannot receive new data. Subsequent calls to @ref receive_data_entry functions
         * will result in an exception.
         *
         * Any data received since the last call to @ref commit_writes is not written to the managed file.  Subsequent
         * calls to @ref commit_writes simply return immediately and perform no action.
         *
         * If this instance is already closed, the function will simply return.
         */
        void close() override;

        /**
         * Write any received data entries that were not written immediately upon receipt to the managed data files.
         *
         * Function expects/requires data for all local nexus ids to have been received. Since it expects this, it also
         * increments the ::attribute:`current_time_index` value before exiting.
         *
         * Additionally, it clears the current ::attribute:`data_cache` and ::attribute:`current_formulation_id` values.
         *
         * If called when @ref is_closed is ``true``, the function simply returns immediately without performing any
         * actions.
         */
        void commit_writes() override;

        /**
         * A test of whether this instance is closed.
         *
         * This type determines whether it is closed by whether it has a valid NetCDF file id.
         *
         * @return Whether this instance is closed.
         */
        bool is_closed() override;

        /**
         * Get a new vector containing the filenames for managed files for this instance.
         *
         * @return Pointer to new vector with the managed files for this instance.
         */
        std::shared_ptr<std::vector<std::string>> get_filenames();

        /**
         * Receive a data entry for this nexus, specifying details including the formulation id.
         *
         * If this instance is closed (@ref is_closed) an exception will be raised.
         *
         * @param formulation_id The id of the formulation involved in producing this data.
         * @param nexus_id The id for the nexus to which this data applies.
         * @param data_time_marker A marker for the current simulation time for the data.
         * @param flow_data_at_t The nexus flow contribution at this time index (the main data to write).
         * @see close
         * @see is_closed
         */
        void receive_data_entry(const std::string &formulation_id, const std::string &nexus_id,
                                const time_marker &data_time_marker, double flow_data_at_t) override;

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
        static bool isMpiInitialized();
        #endif

        static std::string parse_netcdf_return_code(const int nc_status);

        const std::string nc_dim_name_nexus_id = "feature_id";
        const std::string nc_dim_name_time = "time";
        const std::string nc_var_name_flow = "runoff_rate";

        const double flow_var_fill_value = nan("");

        /** Array of chunk sizes for  dimensions - nexus_id and time - of flow NetCDF variable, to set chunking. */
        size_t flow_var_chunk_size_per_dim[2];

        /** Map of nexus id to corresponding index for that nexus in several data structures for this instance. */
        std::unordered_map<std::string, size_t> nexus_data_indices;

        /**
         * Data values for each nexus in the current time step, with indices corresponding to @ref nexus_data_indices.
         *
         * Note that at constructions and at the end of every call to @ref commit_writes (i.e., after current data was
         * written), all values should be set to the default fill value
         */
        std::vector<double> current_nexus_data;

        /**
         * An offset for NetCDF data arrays that indicates the start of where this instance should begin writing data,
         * in particular when multiple instances are writing to the same file and need to not write in overlapping
         * regions.
         */
        std::vector<unsigned long>::size_type local_offset;

        /** Nexus ids for which this instance/process will write data to the file (i.e., local when using MPI, all otherwise). */
        const std::vector<std::string> nexus_ids;

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

        /**
         * The instance id for this instance, which should be the MPI rank if using MPI.
         *
         * For non-MPI use cases, there must always be an id 0, and it is assumed each new id is incremented by 1.
         */
        int obj_id;

        /** Variable id for the ``nexus_id`` NetCDF variable. */
        int nc_var_id_nexus_id;

        /** Variable id for the ``time`` NetCDF variable. */
        int nc_var_id_time;

        /** Variable id for the ``flow`` NetCDF variable. */
        int nc_var_id_flow;

        /** The total number of nexuses in this simulation, potentially across multiple instances and ranks. */
        int total_nexus_count;

        /**
         * Whether to use specified chunking for the NetCDF flow/runoff variable when creating it within
         * @ref setup_netcdf_metadata.
         */
        bool use_chunking_flow_var;

        /**
         * Whether instance should use ``NC_COLLECTIVE`` parallel access mode for NetCDF variables (except ``time``).
         *
         * Note that the ``time`` variable is excepted and will always only be written by obj_id 0, and thus remain
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
        void add_dimension(const std::string& dim_name, const size_t size, int* dim_id_ptr) const;

        /**
         * Add a NetCDF variable to the managed NetCDF file/dataset via the C API.
         *
         * @param var_name The variable name.
         * @param var_type The NetCDF C API variable type code.
         * @param dimension_nc_ids Ordered collection of dimensions nc_ids for the dimensions of the new variable.
         * @param var_id_ptr Pointer to the location in which to save the nc_id for the created variable.
         */
        void add_variable(const std::string& var_name, nc_type var_type, const std::vector<int>& dimension_nc_ids, int* var_id_ptr) const;

        /**
         * Add a variable to the managed NetCDF file/dataset, and also put string attributes in place for the variable.
         *
         * @param var_name The variable name.
         * @param var_type The NetCDF variable type code.
         * @param dim_nc_ids Ordered collection of dimensions nc_ids for the dimensions of the new variable.
         * @param attributes Map of name:value strings for attributes for the variable.
         * @param var_id_ptr Pointer to the location in which to save the nc_id for the created variable.
         */
        void add_variable(const std::string& var_name, nc_type var_type, const std::vector<int>& dim_nc_ids, const std::map<std::string, std::string>& attributes, int* var_id_ptr) const;

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
        void add_variable(const std::string& var_name, nc_type var_type, const std::vector<int>& dim_nc_ids, const std::map<std::string, std::string>& attributes, const void* fill_value, int* var_id_ptr) const;

        /**
         * Create the managed NetCDF file/dataset for regular (i.e., non-parallel) access via the C API.
         *
         * In any successful execution, this function will have the side effect of setting the @ref netcdf_file_id
         * member variable.
         *
         * If a file already exists at @ref nexus_outfile, it will be overwritten and a warning will be printed to
         * standard output.
         *
         * Note this function is expected to only be called at most once by an instance, during the constructor (though
         * after the @ref nexus_outfile and @ref obj_id member variables are properly set).  An exception will be thrown
         * if the @ref netcdf_file_id member variable is already set (to something other than ``-1``) when this function
         * is called.
         */
        void create_netcdf_file();

        #if NGEN_WITH_MPI && NGEN_WITH_PARALLEL_NETCDF
        /**
         * Create the managed NetCDF file/dataset for parallel access via the C API.
         *
         * In any successful execution, this function will have the side effect of setting the @ref netcdf_file_id
         * member variable.
         *
         * If a file already exists at @ref nexus_outfile, it will be overwritten and a warning will be printed to
         * standard output by obj_id ``0`` (so the message is not duplicated across ranks).
         *
         * Note this function is expected to only be called at most once by an instance, during the constructor (though
         * after the @ref nexus_outfile and @ref obj_id member variables are properly set).  An exception will be thrown
         * if the @ref netcdf_file_id member variable is already set (to something other than ``-1``) when this function
         * is called.
         *
         * @param mpi_comm The MPI communicator passed to the NetCDF API function calls
         */
        void create_netcdf_file_parallel(MPI_Comm mpi_comm);
        #endif

        /**
         * Close the (presumed open) NetCDF file and (if successful) clear the corresponding file id by resetting
         * @ref netcdf_file_id to `-1`.
         */
        void close_netcdf_file();

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
        void open_netcdf_file();

        /**
         * For cases when only opening a NetCDF file/dataset, retrieve necessary variable metadata for execution.
         *
         * Function will lookup and store the variable ids for the nexus_id, time, and flow/runoff variables.
         */
        void lookup_netcdf_metadata();

        /**
         * Setup the NetCDF metadata (dimensions and variables) for the managed NetCDF file/dataset.
         *
         * Function will create and store the variable ids for the nexus_id, time, and flow/runoff variables.  To
         * support those variables, it will also create the nexus_id and time dimensions, though not store those ids.
         */
        void setup_netcdf_metadata();

        /**
         * On the first time step, write this instance's managed nexus ids to the NetCDF data.
         */
        void write_nexus_ids_once() const;

        #if NGEN_WITH_MPI && NGEN_WITH_PARALLEL_NETCDF
        /**
         * Set the NetCDF variable with the given variable id to have ``NC_COLLECTIVE`` parallel access.
         *
         * @param nc_var_id The numeric NetCDF variable id
         */
        void set_nc_var_parallel_collective(const int nc_var_id) const;
        #endif

    };
} // utils

#endif // #if NGEN_WITH_NETCDF

#endif //NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP
