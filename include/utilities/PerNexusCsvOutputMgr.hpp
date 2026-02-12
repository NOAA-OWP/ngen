//
// Created by Robert Bartel on 12/5/25.
//

#ifndef NGEN_PERNEXUSCSVOUTPUTMGR_HPP
#define NGEN_PERNEXUSCSVOUTPUTMGR_HPP

#include "NexusOutputsMgr.hpp"

namespace utils
{
    /**
     * Subtype that manages/writes per-nexus data files in CSV format.
     *
     * Because it works with CSVs, this type writes data entries to the managed files immediately upon receipt.
     */
    class PerNexusCsvOutputMgr : public NexusOutputsMgr
    {
    public:

        /**
         * Construct instance set for managing/writing nexus data files.
         *
         * @param nexus_ids Nexus ids for which this instance manages data (in particular, local nexuses when using MPI).
         * @param output_root The output root for written files (as a string).
         */
        PerNexusCsvOutputMgr(const std::vector<std::string>& nexus_ids, const std::string &output_root) {
            for(const auto& id : nexus_ids) {
                nexus_outfiles[id].open(output_root + id + "_output.csv", std::ios::trunc);
            }
        }

        /**
         * No-op, since this type writes entries as they are received.
         */
        void commit_writes() override { }

        /**
         * Receive a data entry for this nexus, specifying details including the formulation id.
         *
         * @param formulation_id The id of the formulation involved in producing this data.
         * @param nexus_id The id for the nexus to which this data applies.
         * @param data_time_marker A marker for the current simulation time for the data.
         * @param flow_data_at_t The nexus flow contribution at this time index (the main data to write).
         */
        void receive_data_entry(const std::string &formulation_id, const std::string &nexus_id,
                                const time_marker &data_time_marker, const double flow_data_at_t) override {
            if (formulation_id != get_default_formulation_id()) {
                throw std::runtime_error("Cannot write data entry for non-default formulation " + formulation_id + " for nexus " + nexus_id + " when per-nexus CSV output is enabled.");
            }
            nexus_outfiles[nexus_id] << data_time_marker.sim_time_index << ", " << data_time_marker.time_stamp << ", " << flow_data_at_t << std::endl;
        }

    private:
        std::unordered_map<std::string, std::ofstream> nexus_outfiles;

    };
} // utils

#endif //NGEN_PERNEXUSCSVOUTPUTMGR_HPP
