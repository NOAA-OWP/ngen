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
#ifndef NGEN_PERNEXUSCSVOUTPUTMGR_HPP
#define NGEN_PERNEXUSCSVOUTPUTMGR_HPP

#include <unordered_map>
#include <vector>
#include <fstream>
#include "NexusOutputsMgr.hpp"

namespace utils
{
    /**
     * Subtype that manages/writes per-nexus data files in CSV format.
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
        PerNexusCsvOutputMgr(const std::vector<std::string>& nexus_ids, const std::string &output_root);

        /**
         * Close this instance, closing all @ref nexus_outfiles streams.
         *
         * Once a manager is closed, it will not be able to receive new data. Subsequent calls to
         * @ref receive_data_entry functions will result in an exception.
         *
         * However, because of the way this type is implemented, all data received before the call to @ref close should
         * get flushed to disk as part of closing the files.
         *
         * If this instance is already closed, the function will simply return.
         */
        void close() override;

        /**
         * Commit writes by flushing output file `ofstream`s to filesystem.
         *
         * Note that this function will do nothing and immediately return if the instance has already been closed with
         * @ref close.
         */
        void commit_writes() override;

        /**
         * A test of whether this instance is closed.
         *
         * @return Whether this instance is closed.
         * @see close
         */
        bool is_closed() override;

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
        std::unordered_map<std::string, std::ofstream> nexus_outfiles;
        bool closed = false;

    };
} // utils

#endif //NGEN_PERNEXUSCSVOUTPUTMGR_HPP
