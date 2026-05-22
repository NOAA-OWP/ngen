/*
Created by Robert Bartel on 12/3/25.
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
         * Close down this manager, closing any and all open files.
         *
         * Once a manager is closed, it should not be able to receive new data. Subsequent calls to
         * @ref receive_data_entry functions should result in an exception.
         *
         * Subtypes may choose how to handle any data received since the last call to @ref commit_writes.
         *
         * If an instance is already closed, implementations should simply return.
         */
        virtual void close() = 0;

        /**
         * Write any received data entries that were not written immediately upon receipt to the managed data files.
         */
        virtual void commit_writes() = 0;

        /**
         * A test of whether this instance is closed.
         *
         * @return Whether this instance is closed.
         * @see close
         */
        virtual bool is_closed() = 0;

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
