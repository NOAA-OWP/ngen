//
// Created by Robert Bartel on 12/3/25.
//

#ifndef NGEN_NEXUSOUTPUTSMGR_HPP
#define NGEN_NEXUSOUTPUTSMGR_HPP

#include <string>

namespace utils
{
    /** Simple helper struct for tracking meaningful points in time for output writing. */
    struct time_marker
    {
        const long sim_time_index;
        const time_t epoch_time;
        const std::string time_stamp;

        time_marker(const long sim_time_index, const time_t epoch_time, const std::string& time_stamp)
            : sim_time_index(sim_time_index), epoch_time(epoch_time), time_stamp(time_stamp) {}
    };

    /**
     * Abstract class for managing and writing to nexus data files.
     */
    class NexusOutputsMgr
    {

    public:

        /**
         * Write any received data entries that were not written immediately upon receipt to the managed data files.
         */
        virtual void commit_writes() = 0;

        /**
         * Receive a data entry for this nexus, specifying details including the formulation id.
         *
         * @param formulation_id The id of the formulation involved in producing this data.
         * @param nexus_id The id for the nexus to which this data applies.
         * @param data_time_marker A marker for the current simulation time for the data.
         * @param flow_data_at_t The nexus flow contribution at this time index (the main data to write).
         */
        virtual void receive_data_entry(const std::string &formulation_id, const std::string &nexus_id,
                                        const time_marker &data_time_marker, const double flow_data_at_t) = 0;

        /**
         * Receive a data entry for this nexus, specifying details but using the default formulation id (however that is
         * determined).
         *
         * @param nexus_id The id for the nexus to which this data applies.
         * @param data_time_marker A marker for the current simulation time for the data.
         * @param flow_data_at_t The nexus flow contribution at this time index (the main data to write).
         *
         * @see get_default_formulation_id
         */
        virtual void receive_data_entry(const std::string &nexus_id, const time_marker &data_time_marker, const double flow_data_at_t) {
            receive_data_entry(get_default_formulation_id(), nexus_id, data_time_marker, flow_data_at_t);
        }

    protected:
        virtual ~NexusOutputsMgr() = default;

        /**
         * Get some a formulation id value.
         *
         * @return The value for the default formulation id.
         */
        const std::string get_default_formulation_id() {
            return "default";
        }

    };
} // utils

#endif //NGEN_NEXUSOUTPUTSMGR_HPP
