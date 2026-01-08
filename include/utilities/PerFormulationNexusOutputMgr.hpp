//
// Created by Robert Bartel on 12/5/25.
//

#ifndef NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP
#define NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP

#include "NexusOutputsMgr.hpp"
#include <netcdf>
#include <vector>
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
     */
    class PerFormulationNexusOutputMgr : public NexusOutputsMgr
    {
    private:
        // For unit testing
        friend class ::PerFormulationNexusOutputMgr_Test;

    public:
        virtual ~PerFormulationNexusOutputMgr() = default;

        /**
         * Construct instance set for managing/writing nexus data files, creating the file(s) appropriately.
         *
         * Note that the class is designed to be aware of its MPI rank and supports multiple instances across different
         * ranks writing to the same file.  However, only rank `0` will initially create the nexus output file. Further,
         * the instance relies on users/callers to maintain MPI synchronization (i.e., call MPI_Barrier so other ranks
         * don't get ahead of rank `0` while it creates the file).
         *
         * @param nexus_ids Nexus ids for which this instance manages data (in particular, local nexuses when using MPI).
         * @param formulation_ids
         * @param output_root The output root for written files (as a string).
         * @param mpi_rank The MPI rank of this process, when using MPI (always `0` if no MPI).
         * @param nexuses_per_rank The total number nexuses for each running rank.
         */
        PerFormulationNexusOutputMgr(const std::vector<std::string> nexus_ids,
                                     std::shared_ptr<std::vector<std::string>> formulation_ids,
                                     const std::string &output_root,
                                     const int mpi_rank,
                                     const std::vector<int>& nexuses_per_rank)
            :   nexus_ids(nexus_ids),
                mpi_rank(mpi_rank),
                nexuses_per_rank(nexuses_per_rank)
        {
            if (this->nexuses_per_rank.empty()) {
                if (this->mpi_rank > 0)
                    throw std::runtime_error("Must supply nexuses_per_rank values when using multiple MPI processes.");
                this->nexuses_per_rank.push_back(this->nexus_ids.size());
            }

            if (this->nexuses_per_rank.size() <= this->mpi_rank) {
                throw std::runtime_error("To few values in nexuses_per_rank value ("
                    + std::to_string(this->nexuses_per_rank.size()) + ") for rank "
                    + std::to_string(mpi_rank) + ".");
            }

            if (this->nexuses_per_rank[this->mpi_rank] != this->nexus_ids.size()) {
                throw std::runtime_error("Invalid nexuses_per_rank value for rank " + std::to_string(this->mpi_rank)
                    + ": " + std::to_string(this->nexuses_per_rank[this->mpi_rank]) + " does not match number of "
                    + "supplied nexus ids (" + std::to_string(this->nexus_ids.size()) + ").");
            }

            if (this->mpi_rank == 0) {
                for (size_t r = 0; r < this->nexuses_per_rank.size(); r++) {
                    if (this->nexuses_per_rank[r] < 0) {
                        throw std::runtime_error("Invalid nexuses_per_rank value for rank " + std::to_string(r) + ".");
                    }
                }
            }

            // Should always have one formulation at least, so
            if (formulation_ids == nullptr) {
                formulation_ids = std::make_shared<std::vector<std::string>>();
            }
            if (formulation_ids->empty()) {
                formulation_ids->push_back(get_default_formulation_id());
            }

            this->local_offset = 0;

            for (size_t r = 0; r < mpi_rank; ++r) {
                this->local_offset += nexuses_per_rank[r];
            }

            int total_nexuses = 0;
            for (const int n : nexuses_per_rank) {
                total_nexuses += n;
            }

            for (const std::string& fid : *formulation_ids) {
                std::string filename = output_root + "/formulation_" + fid + "_nexuses.nc";
                this->nexus_outfiles[fid] = filename;

                // Have rank 0 set up the files
                if (this->mpi_rank == 0) {
                    netCDF::NcFile ncf(filename, netCDF::NcFile::replace);
                    /* ************************************************************************************************
                     * Important:  do not change order or add more dims w/out also updating commit_writes appropriately.
                     * ********************************************************************************************** */
                    netCDF::NcDim dim_nexus = ncf.addDim(this->nc_nex_id_dim_name, total_nexuses);
                    netCDF::NcDim dim_time = ncf.addDim(this->nc_time_dim_name);

                    netCDF::NcVar flow = ncf.addVar(this->nc_flow_var_name, netCDF::ncDouble, {dim_nexus, dim_time});
                }
            }
        }

        /**
         * Construct instance set for managing/writing nexus data files, supplying a default value of `0` for MPI rank.
         *
         * @param nexus_ids Nexus ids for which this instance manages data (in particular, local nexuses when using MPI).
         * @param formulation_ids
         * @param output_root The output root for written files (as a string).
         */
        PerFormulationNexusOutputMgr(const std::vector<std::string> nexus_ids,
                                     std::shared_ptr<std::vector<std::string>> formulation_ids,
                                     const std::string &output_root) : PerFormulationNexusOutputMgr(nexus_ids, formulation_ids, output_root, 0, {}) {}

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
                        "Missing data for nexus " + nexus_ids[i] + " in formulation " + current_formulation_id +
                        " at time index " + std::to_string(current_time_index) + ".");
                }
                data[i] = data_cache[nexus_ids[i]];
            }

            const std::string filename = nexus_outfiles[current_formulation_id];

            const netCDF::NcFile ncf(filename, netCDF::NcFile::write);
            const netCDF::NcVar flow = ncf.getVar(nc_flow_var_name);

            // Assume base on how constructor was set up (imply for conciseness)
            //size_t nexus_dim_index = 0;
            //size_t time_dim_index = 1;
            const std::vector<size_t> start = {local_offset, static_cast<size_t>(current_time_index)};
            const std::vector<size_t> count = {nexus_ids.size(), 1};

            flow.putVar(start, count, data.data());

            current_time_index++;
            data_cache.clear();
            current_formulation_id.clear();
        }

        /**
         * Get a new vector containing the filenames for managed files for this instance.
         *
         * @return Pointer to new vector with the managed files for this instance.
         */
        std::shared_ptr<std::vector<std::string>> get_filenames() {
            std::shared_ptr<std::vector<std::string>> filenames(new std::vector<std::string>());
            for (const auto& pair : nexus_outfiles) {
                filenames->push_back(pair.second);
            }
            return filenames;
        }

        /**
         * Get whether this instance manages writing data for the given nexus to a managed data file.
         *
         * @param nexus_id The id for the Nexus of interest.
         * @return Whether this instance manages writing data for the given nexus to a managed data file.
         */
        bool is_nexus_managed(const std::string& nexus_id) override {
            return std::find(nexus_ids.begin(), nexus_ids.end(), nexus_id) != nexus_ids.end();
        }

        /**
         * Receive a data entry for this nexus, specifying details including the formulation id.
         *
         * @param formulation_id The id of the formulation involved in producing this data.
         * @param nexus_id The id for the nexus to which this data applies.
         * @param current_time_index The simulation output time index for the data.
         * @param current_timestamp The timestamp for the data.
         * @param flow_data_at_t The nexus flow contribution at this time index (the main data to write).
         */
        void receive_data_entry(const std::string& formulation_id, const std::string& nexus_id, long current_time_index,
                                const std::string& current_timestamp, double flow_data_at_t) override
        {
            if (current_formulation_id.empty()) {
                current_formulation_id = formulation_id;
            }
            else if (current_formulation_id != formulation_id) {
                throw std::runtime_error(
                    "Cannot receive data for formulation " + formulation_id + " for nexus " + nexus_id +
                    " when expecting data for " + current_formulation_id + ".");
            }

            if (current_time_index != this->current_time_index) {
                throw std::runtime_error(
                    "Cannot receive data for formulation " + formulation_id + " for nexus " + nexus_id +
                    " at time index " + std::to_string(current_time_index) + " when expecting data for time index " +
                    std::to_string(this->current_time_index) + ".");
            }

            data_cache[nexus_id] = flow_data_at_t;
        }

    private:

        const std::string nc_nex_id_dim_name = std::string("feature_id");
        const std::string nc_time_dim_name = "time";
        const std::string nc_flow_var_name = "runoff_rate";

        /** Map of nexus ids to corresponding cached flow data from ``receive_data_entry``. */
        std::unordered_map<std::string, double> data_cache;
        /** The current/last formulation id value received by `receive_data_entry`. */
        std::string current_formulation_id;
        /** Current time index of latest ``receive_data_entry``. */
        long current_time_index = 0;
        /** Nexus ids for which this instance/process will write data to the file (i.e., local when using MPI, all otherwise). */
        const std::vector<std::string> nexus_ids;
        int mpi_rank;
        std::vector<unsigned long>::size_type local_offset;
        /** Map of formulation ids to nexus data file paths (as string) */
        std::unordered_map<std::string, std::string> nexus_outfiles;
        /** The number of nexuses assigned to each rank. */
        std::vector<int> nexuses_per_rank;

    };
} // utils

#endif //NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP
