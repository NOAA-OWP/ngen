/*
Copyright (C) 2026 Lynker
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

#include "CatchmentCsvOutputMgr.hpp"

#include <cstddef>
#include <iomanip>
#include <stdexcept>
#include <string_view>
#include <utility>

using utils::CatchmentCsvOutputMgr;

namespace {
    static constexpr std::string_view DELIMITER = ",";

    //! Join the column output names with commas for a CSV header. CSV writes only each column's
    //! output_name; the source_name, units, and other OutputField metadata are not emitted here.
    std::string join_columns(const std::vector<utils::OutputField> &columns)
    {
        std::string out;
        for (const auto &c : columns) {
            out += (out.empty() ? "" : DELIMITER);
            out += c.output_name;
        }
        return out;
    }

    //! Build the CSV header line. When include_id is true a leading "catchment_id"
    //! column is prepended (aggregated output, where one file holds many catchments);
    //! otherwise the layout is the long-standing "Time Step,Time,<vars>". The column
    //! order is kept identical to catchment_output_data_row so header and data rows
    //! always agree.
    std::string catchment_output_header_line(bool include_id,
                                             const std::string_view variable_header,
                                             const std::string_view delimiter = DELIMITER)
    {
        std::string line;
        if (include_id) {
            line += "catchment_id";
            line += delimiter;
        }
        line += "Time Step";
        line += delimiter;
        line += "Time";
        line += delimiter;
        line += variable_header;
        return line;
    }

}

CatchmentCsvOutputMgr::CatchmentCsvOutputMgr(std::string output_root, std::optional<std::string> aggregated_filename,
                                             int precision, std::vector<FeatureDescriptor> features)
    : output_root_(std::move(output_root))
    , aggregated_filename_(std::move(aggregated_filename))
    , precision_(precision)
{
    // The full set of catchments is known up front, so open every file and write its header now;
    // the manager is ready to receive data immediately after construction.
    for (const auto &feature : features) {
        add_feature(feature);
    }
}

CatchmentCsvOutputMgr::~CatchmentCsvOutputMgr()
{
    // Best-effort backstop for callers who don't close() explicitly. close() is the path that
    // surfaces commit/finalize errors; a destructor must never let one escape (here it cannot, but
    // the guard sets the pattern for backends whose commit can fail).
    try {
        close();
    } catch (...) {
        // nothing actionable during destruction
    }
}

std::filesystem::path CatchmentCsvOutputMgr::output_path(const std::string &formulation_id, const std::string &catchment_id)
{
    // Aggregated output shares one file per formulation (the catchment id becomes a column);
    // per-feature output uses one file per catchment. Either way the default formulation keeps the
    // flat layout and any other formulation id gets its own subdirectory, so independent setups --
    // and, when aggregating, each formulation's uniform column set -- never collide.
    const std::string filename = aggregated_filename_ ? *aggregated_filename_ : catchment_id + ".csv";
    if (formulation_id == default_formulation_id()) {
        return output_root_ + filename;
    }
    return output_root_ + formulation_id + "/" + filename;
}

std::pair<std::ofstream*, std::mutex*> CatchmentCsvOutputMgr::stream_for(const std::string &formulation_id, const std::string &catchment_id)
{
    const auto it = streams_.find(output_path(formulation_id, catchment_id));
    return {it == streams_.end() ? nullptr : it->second.get(), &all_streams_mutex_};
}

void CatchmentCsvOutputMgr::add_feature(const FeatureDescriptor &feature)
{
    const auto path = output_path(feature.formulation_id, feature.catchment_id);
    // When aggregating, several catchments of one formulation resolve to the same file: the first
    // opens it and writes the header, the rest share the already-open stream. Per-feature paths are
    // unique. Opening a file and writing its header therefore happen together, exactly once per file.
    if (streams_.find(path) != streams_.end()) {
        return;
    }
    // output_path owns the layout; derive the directory from it so the two can't drift.
    if (const auto dir = path.parent_path(); !dir.empty()) {
        std::filesystem::create_directories(dir);
    }
    auto stream = std::make_shared<std::ofstream>();
    stream->open(path, std::ios::trunc);
    // Aggregated files carry a leading catchment_id column; per-feature files do not.
    (*stream) << catchment_output_header_line(aggregated_filename_.has_value(), join_columns(feature.fields)) << "\n";
    (*stream) << std::setprecision(precision_);
    streams_[path] = std::move(stream);
}

void CatchmentCsvOutputMgr::receive_data_entry(const std::string &formulation_id, const std::string &catchment_id,
                                               const time_marker &data_time_marker, const std::vector<double> &values)
{
    if (closed_) {
        throw std::runtime_error("Can't receive data on a closed CatchmentCsvOutputMgr");
    }

    auto [out, mutex] = stream_for(formulation_id, catchment_id);
    if (out == nullptr) {
        throw std::runtime_error("CatchmentCsvOutputMgr received data for an unknown catchment '" + catchment_id + "'");
    }
    std::lock_guard stream_lock(*mutex);
    // In aggregated output each row carries the (full) catchment id so rows stay attributable.
    if( aggregated_filename_.has_value() ) {
        (*out) << catchment_id << DELIMITER;
    }
    // Add time step/timestamp
    (*out) << data_time_marker.sim_time_index << DELIMITER
           << data_time_marker.time_stamp;
    // Add values, precision set on the stream at construction.
    for (std::size_t i = 0; i < values.size(); ++i) {
        (*out) << DELIMITER;
        (*out) << values[i];
    }
    // Use \n so we don't force a flush
    (*out) << "\n";
}

void CatchmentCsvOutputMgr::commit_writes()
{
    // Rows are written immediately on receipt; just flush what is buffered.
    if (closed_) {
        return;
    }
    for (auto &entry : streams_) {
        entry.second->flush();
    }
}

void CatchmentCsvOutputMgr::close()
{
    if (closed_) {
        return;
    }
    // close() == commit + finalize: flush everything received, then close the files. Committing via
    // commit_writes() (rather than relying on ofstream::close to flush) makes the "close commits"
    // contract explicit and sets the shape for backends whose finalize differs from their flush.
    commit_writes();
    for (auto &entry : streams_) {
        if (entry.second->is_open()) {
            entry.second->close();
        }
    }
    closed_ = true;
}

bool CatchmentCsvOutputMgr::is_closed()
{
    return closed_;
}
