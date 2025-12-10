//
// Created by Robert Bartel on 12/3/25.
//

#ifndef NGEN_NEXUSOUTPUTSMGR_HPP
#define NGEN_NEXUSOUTPUTSMGR_HPP

#include <string>
#include "boost/range/any_range.hpp"

namespace utils
{

    #if NGEN_WITH_MPI
    typedef hy_features::HY_Features_MPI HY_Features_Type;
    #else
    typedef hy_features::HY_Features HY_Features_Type;
    #endif

    /**
     * Abstract class for managing and writing to nexus data files.
     */
    class NexusOutputsMgr
    {

    public:

        /**
         * Construct instance set for managing/writing nexus data files.
         *
         * @param nexus_features Nexus features object, from which we can get nexus ids and remote status.
         */
        explicit NexusOutputsMgr(const std::shared_ptr<HY_Features_Type>& nexus_features) : nexus_features(nexus_features) { }

        /**
         * Write any received data entries that were not written immediately upon receipt to the managed data files.
         */
        virtual void commit_writes() = 0;

        /**
         * Get whether this instance manages writing data for the given nexus to a managed data file.
         *
         * @param nexus_id The id for the Nexus of interest.
         * @return Whether this instance manage a file storing data for the given nexus.
         */
        virtual bool is_nexus_managed(const std::string &nexus_id) = 0;

        /**
         * Receive a data entry for this nexus, specifying details including the formulation id.
         *
         * Implementations can write immediately or cache
         *
         * @param formulation_id The id of the formulation involved in producing this data.
         * @param nexus_id The id for the nexus to which this data applies.
         * @param current_time_index The simulation output time index for the data.
         * @param current_timestamp The timestamp for the data.
         * @param flow_data_at_t The nexus flow contribution at this time index (the main data to write).
         */
        virtual void receive_data_entry(const std::string &formulation_id, const std::string &nexus_id, long current_time_index, const std::string &current_timestamp, double flow_data_at_t) = 0;

        /**
         * Receive a data entry for this nexus, specifying details but using the default formulation id (however that is
         * determined).
         *
         * @param nexus_id The id for the nexus to which this data applies.
         * @param current_time_index The simulation output time index for the data.
         * @param current_timestamp The timestamp for the data.
         * @param flow_data_at_t The nexus flow contribution at this time index (the main data to write).
         *
         * @see get_default_formulation_id
         */
        virtual void receive_data_entry(const std::string &nexus_id, long current_time_index, const std::string &current_timestamp, double flow_data_at_t) {
            receive_data_entry(get_default_formulation_id(), nexus_id, current_time_index, current_timestamp, flow_data_at_t);
        }

    protected:
        std::shared_ptr<HY_Features_Type> nexus_features;

        ~NexusOutputsMgr() = default;

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