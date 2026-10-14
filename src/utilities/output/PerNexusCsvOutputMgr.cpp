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

#include <filesystem>
#include <stdexcept>

struct PerNexusStepOutput : utils::FormulationStepOutput
{
    PerNexusStepOutput(std::string formulation_id, utils::time_marker time, utils::PerNexusCsvOutputMgr &output_mgr)
        : formulation_id_(std::move(formulation_id))
        , time_(std::move(time))
        , output_mgr_(output_mgr)
    { }
    const std::string formulation_id_;
    const utils::time_marker time_;
    utils::PerNexusCsvOutputMgr &output_mgr_;

    void receive_data_entry(const std::string &nexus_id, const double flow_data_at_t) override {
        output_mgr_.receive_data_entry(formulation_id_, nexus_id, time_, flow_data_at_t);
    }
};

utils::PerNexusCsvOutputMgr::PerNexusCsvOutputMgr(const std::vector<std::string>& nexus_ids,
                                                  const std::string& output_root) {
    // output_root is the effective directory this manager writes to;
    // ensure it exists, then open one file per nexus directly in it.
    if (!output_root.empty()) {
        std::filesystem::create_directories(output_root);
    }
    for(const auto& id : nexus_ids) {
        nexus_outfiles[id].open(output_root + id + "_output.csv", std::ios::trunc);
    }
}

void utils::PerNexusCsvOutputMgr::close() {
    if (is_closed()) return;

    for (auto& p: nexus_outfiles) {
        p.second.close();
    }
    closed = true;
}

void utils::PerNexusCsvOutputMgr::commit_writes() {
    if (is_closed()) return;
    for (auto &f : nexus_outfiles) {
        f.second.flush();
    }
}

bool utils::PerNexusCsvOutputMgr::is_closed() {
    return closed;
}

std::unique_ptr<utils::FormulationStepOutput> utils::PerNexusCsvOutputMgr::prepare_for_step(const std::string &formulation_id,
                                                                                     const time_marker &data_time_marker)
{
    if (is_closed()) {
        throw std::runtime_error("Can't run PerNexusCsvOutputMgr::receive_data_entry() if instance is closed");
    }
    if (formulation_id != get_default_formulation_id()) {
        throw std::runtime_error("Cannot prepare data entry for non-default formulation " + formulation_id + " when per-nexus CSV output is enabled.");
    }

    return make_unique<PerNexusStepOutput>(formulation_id, data_time_marker, *this);
}

void utils::PerNexusCsvOutputMgr::receive_data_entry(const std::string& formulation_id, const std::string& nexus_id,
                                                     const time_marker& data_time_marker, const double flow_data_at_t) {
    nexus_outfiles.at(nexus_id) << data_time_marker.sim_time_index << ", " << data_time_marker.time_stamp << ", " << flow_data_at_t << '\n';
}
