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

#include "PerNexusCsvOutputMgr.hpp"

utils::PerNexusCsvOutputMgr::PerNexusCsvOutputMgr(const std::vector<std::string>& nexus_ids,
                                                  const std::string& output_root) {
    for(const auto& id : nexus_ids) {
        nexus_outfiles[id].open(output_root + id + "_output.csv", std::ios::trunc);
    }
}

void utils::PerNexusCsvOutputMgr::commit_writes() {
    for (auto &f : nexus_outfiles) {
        f.second.flush();
    }
}

void utils::PerNexusCsvOutputMgr::receive_data_entry(const std::string& formulation_id, const std::string& nexus_id,
                                                     const time_marker& data_time_marker, const double flow_data_at_t) {
    if (formulation_id != get_default_formulation_id()) {
        throw std::runtime_error("Cannot write data entry for non-default formulation " + formulation_id + " for nexus " + nexus_id + " when per-nexus CSV output is enabled.");
    }
    nexus_outfiles.at(nexus_id) << data_time_marker.sim_time_index << ", " << data_time_marker.time_stamp << ", " << flow_data_at_t << '\n';
}
