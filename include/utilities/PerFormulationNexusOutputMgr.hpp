//
// Created by Robert Bartel on 12/5/25.
//

#ifndef NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP
#define NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP

#include "NexusOutputsMgr.hpp"
#include <netcdf>
#include <vector>
#include <algorithm>

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

    public:
        virtual ~PerFormulationNexusOutputMgr() = default;

        /**
         * Construct instance set for managing/writing nexus data files.
         *
         * @param nexus_features Nexus features object, from which we can get nexus ids and remote status.
         * @param formulation_ids
         * @param output_root The output root for written files (as a string).
         * @param mpi_rank The MPI rank of this process, when using MPI.
         */
        PerFormulationNexusOutputMgr(const std::shared_ptr<HY_Features_Type>& nexus_features,
                                     std::shared_ptr<std::vector<const std::string>> formulation_ids,
                                     const std::string &output_root,
                                     const int mpi_rank)
            :   NexusOutputsMgr(nexus_features),
                all_nexus_ids(nexus_features->nexuses().begin(), nexus_features->nexuses().end())
        {
            // Should always have one formulation at least, so
            if (formulation_ids == nullptr) {
                formulation_ids = std::make_shared<std::vector<const std::string>>();
            }
            if (formulation_ids->empty()) {
                formulation_ids->push_back(get_default_formulation_id());
            }

            // Ensure order (especially if MPI in play)
            std::sort(all_nexus_ids.begin(), all_nexus_ids.end());

            #if NGEN_WITH_MPI
            // Figure out what's local (also sorted)
            for (const auto& nex_id : all_nexus_ids) {
                if (!nexus_features->is_remote_sender_nexus(nex_id)) {
                    local_nexus_ids.push_back(nex_id);
                }
            }
            std::sort(local_nexus_ids.begin(), local_nexus_ids.end());
            // And make sure we know where to start from
            for (size_t i = 0; i < all_nexus_ids.size(); ++i) {
                if (all_nexus_ids[i] == local_nexus_ids[0]) {
                    local_offset = i;
                    break;
                }
            }
            #else
            local_nexus_ids = all_nexus_ids;
            local_offset = 0;
            #endif

            // Have rank 0 set up the files
            if (mpi_rank == 0) {
                for (const std::string& fid : *formulation_ids) {
                    std::string filename = output_root + "/formulation_" + fid + "_nexuses.nc";
                    nexus_outfiles[fid] = filename;

                    netCDF::NcFile ncf(filename, netCDF::NcFile::replace);
                    /* ************************************************************************************************
                     * Important:  do not change order or add more dims w/out also updating commit_writes appropriately.
                     * ********************************************************************************************** */
                    netCDF::NcDim dim_nexus = ncf.addDim("nexus", all_nexus_ids.size());
                    netCDF::NcDim dim_time = ncf.addDim("time");

                    netCDF::NcVar flow = ncf.addVar("flow", netCDF::ncDouble, {dim_nexus, dim_time});
                }
            }

            #if NGEN_WITH_MPI
            MPI_Barrier(MPI_COMM_WORLD);
            #endif
        }

        /**
         * Construct instance set for managing/writing nexus data files, supplying a default value of `0` for MPI rank.
         *
         * @param nexus_features Nexus features object, from which we can get nexus ids and remote status.
         * @param formulation_ids
         * @param output_root The output root for written files (as a string).
         */
        PerFormulationNexusOutputMgr(const std::shared_ptr<HY_Features_Type>& nexus_features,
                                     std::shared_ptr<std::vector<const std::string>> formulation_ids,
                                     const std::string &output_root) : PerFormulationNexusOutputMgr(nexus_features, formulation_ids, output_root, 0) {}

        /**
         * Write any received data entries that were not written immediately upon receipt to the managed data files.
         *
         * Function expects/requires data for all local nexus ids to have been received.
         */
        void commit_writes() override {
            // If no current formulation id set, that should mean there is nothing to write
            if (current_formulation_id.empty()) {
                return;
            }

            // Get properly ordered data vector
            std::vector<double> data(local_nexus_ids.size());
            for (size_t i = 0; i < local_nexus_ids.size(); ++i) {
                // Sanity check we have everything in our block of nexus ids
                if (data_cache.find(local_nexus_ids[i]) == data_cache.end()) {
                    throw std::runtime_error(
                        "Missing data for nexus " + local_nexus_ids[i] + " in formulation " + current_formulation_id +
                        " at time index " + std::to_string(current_time_index) + ".");
                }
                data[i] = data_cache[local_nexus_ids[i]];
            }

            std::string filename = nexus_outfiles[current_formulation_id];

            netCDF::NcFile ncf(filename, netCDF::NcFile::write);
            netCDF::NcVar flow = ncf.getVar("flow");

            // Assume base on how constructor was set up (imply for conciseness)
            //size_t nexus_dim_index = 0;
            //size_t time_dim_index = 1;
            std::vector<size_t> start = {local_offset, static_cast<size_t>(current_time_index)};
            std::vector<size_t> count = {local_nexus_ids.size(), 1};

            flow.putVar(start, count, data.data());
            data_cache.clear();
            current_formulation_id.clear();
        }

        /**
         * Get whether this instance manages writing data for the given nexus to a managed data file.
         *
         * @param nexus_id The id for the Nexus of interest.
         * @return Whether this instance manages writing data for the given nexus to a managed data file.
         */
        bool is_nexus_managed(const std::string& nexus_id) override {
            return std::find(local_nexus_ids.begin(), local_nexus_ids.end(), nexus_id) != local_nexus_ids.end();
        }

        /**
         * Receive a data entry for this nexus, specifying details including the formulation id.
         *
         * Note that formulation id must be the default value when this instance is managing CSV output files.
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
        std::vector<std::string> all_nexus_ids;
        /** Map of nexus ids to corresponding cached flow data from ``receive_data_entry``. */
        std::unordered_map<std::string, double> data_cache;
        /** The current/last formulation id value received by `receive_data_entry`. */
        std::string current_formulation_id;
        /** Current time index of latest ``receive_data_entry``. */
        long current_time_index = 0;
        /** Non-remote nexus ids, which are the ones for which this instance/process will write data to the file. */
        std::vector<std::string> local_nexus_ids;
        /** The index offset for "all" nexus ids; e.g., the index in ``all_nexus_ids`` in which you will find
         * ``local_nexus_ids[0]``, etc.
         */
        size_t local_offset;
        /** Map of formulation ids to nexus data file paths (as string) */
        std::unordered_map<std::string, std::string> nexus_outfiles;

    };
} // utils

#endif //NGEN_PERFORMULATIONNEXUSOUTPUTMGR_HPP