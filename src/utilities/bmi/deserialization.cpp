/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
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
Version 0.1 (see deserialization.hpp)
*/

#include "deserialization.hpp"
#include "serialization.hpp"         // reserved variable name constants + keys
#include "serialization_record.hpp"

#include <boost/optional.hpp>

#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace models{ namespace bmi{ namespace protocols{

namespace {

    // One record's metadata + where to find it on disk.
    // `file_offset` points at the start of the length prefix; the body
    // begins `RECORD_LENGTH_PREFIX_BYTES` later. The body's own length
    // is not cached here — `read_next_record` reads it back from disk
    // on demand, avoiding a duplicated source of truth and shrinking
    // this struct by 8 bytes per record.
    struct IndexEntry {
        int      time_step       = 0;
        int64_t  timestamp_epoch = 0;
        uint64_t file_offset     = 0;
    };

    // Process-global, per-path index of every record in a checkpoint file.
    // Built lazily on the first restore against a path; subsequent restores
    // against the same path reuse the index. Entries are grouped by id so
    // restoring one feature is O(entries-for-that-id), not O(file).
    //
    // Thread safety
    // -------------
    // The build is guarded by a per-instance mutex so concurrent "first
    // restore" calls against the same path serialize on construction.
    // After `built_` flips, all reads proceed lock-free against the
    // immutable map — the contract is that ids/entries never change
    // after the initial build completes. Callers that need to refresh
    // the index (e.g. because the file has grown) must construct a
    // fresh CheckpointIndex; we don't support in-place invalidation
    // because the target workload is one-restore-per-run.
    class CheckpointIndex {
      public:
        explicit CheckpointIndex(std::string path) : path_(std::move(path)) {}

        // Build the index if it hasn't been built yet. When @p id_subset
        // is non-empty the build caps memory by retaining only records
        // whose id is in that subset; an empty vector means "index
        // every record" — that's the sole signal the method takes, no
        // optional/pointer wrapper needed.
        //
        // Returns `void` by design: an unopenable file, a truncated
        // record, and a corrupt record all resolve to "stop the walk
        // cleanly and leave the index with whatever records were
        // successfully read." The caller surfaces any subsequent
        // "no matching record for id X" through the protocol's
        // standard warning/error channel — there is no additional
        // failure reason for `ensure_built` itself to report.
        void ensure_built(const std::vector<std::string>& id_subset)
        {
            std::lock_guard<std::mutex> lock(build_mutex_);
            if (built_) return;

            std::ifstream in(path_, std::ios::binary);
            if (!in) {
                // Missing or unreadable file: treat as "empty index". The
                // downstream `find()` returns no match and the protocol's
                // standard "no matching record" error is raised.
                built_ = true;
                return;
            }

            // Pre-hash the subset for O(1) membership tests during the walk.
            std::unordered_set<std::string> subset_hash;
            if (!id_subset.empty()) {
                subset_hash.reserve(id_subset.size());
                for (const auto& s : id_subset) subset_hash.insert(s);
            }
            const bool have_subset = !subset_hash.empty();

            while (true) {
                const auto pos_stream = in.tellg();
                if (pos_stream == std::streampos(-1)) break;
                const uint64_t pos = static_cast<uint64_t>(pos_stream);

                // Delegate the prefix + body + archive parse to the
                // shared record helper — single source of truth for
                // the on-disk framing. We still pay the payload's
                // deserialization cost (boost::serialization has no
                // "stop partway" primitive) but the payload is
                // immediately discarded, so the memory cost stays
                // flat in the number of indexed records rather than
                // total payload bytes. Malformed records inside an
                // otherwise intact file are treated the same as
                // truncation: stop the walk cleanly; any records
                // already indexed remain usable.
                SerializationRecord rec;
                try {
                    if (!read_next_record(in, rec)) break;  // EOF / torn
                } catch (const std::exception&) {
                    break;  // corrupt record — stop the walk here
                }

                if (have_subset && subset_hash.count(rec.id) == 0) {
                    // Skip: not in the caller's subset of interest.
                    continue;
                }

                IndexEntry entry;
                entry.time_step       = rec.time_step;
                entry.timestamp_epoch = rec.timestamp;
                entry.file_offset     = pos;
                by_id_[std::move(rec.id)].push_back(entry);
            }

            built_ = true;
        }
        // Look up the single record matching @p id and the caller's
        // match mode. Returns a copy of the matching `IndexEntry` (24
        // bytes — cheap) rather than a pointer into the internal map,
        // so callers don't have to reason about the entry's lifetime
        // relative to the index. `boost::optional` is the C++14 way
        // to express "zero or one" — an empty optional signals no
        // match.
        auto find(const std::string& id,
                  bool by_timestamp,
                  int64_t target_timestamp_epoch,
                  bool step_latest,
                  int target_step) const -> boost::optional<IndexEntry>
        {
            auto it = by_id_.find(id);
            if (it == by_id_.end()) return boost::none;
            const auto& entries = it->second;
            if (entries.empty()) return boost::none;

            if (by_timestamp) {
                for (const auto& e : entries) {
                    if (e.timestamp_epoch == target_timestamp_epoch) return e;
                }
                return boost::none;
            }
            if (step_latest) {
                boost::optional<IndexEntry> best;
                for (const auto& e : entries) {
                    if (!best || e.time_step > best->time_step) best = e;
                }
                return best;
            }
            for (const auto& e : entries) {
                if (e.time_step == target_step) return e;
            }
            return boost::none;
        }

        // Read the archive bytes for an entry and deserialize back to a
        // SerializationRecord. Uses a fresh ifstream per call so concurrent
        // restores of distinct features don't lock each other out on a
        // shared file handle.
        //
        // The record-format layer owns the seek-past-prefix + read-body
        // + parse-archive sequence via `read_next_record`; we seek to
        // the *start* of the record (its length prefix) and let the
        // helper do the rest. `read_next_record` itself enforces
        // `MAX_RECORD_ARCHIVE_BYTES` (throws on an absurd on-disk
        // length) and treats `length == 0` as a malformed record
        // because a valid archive always contains at least a
        // boost-library header — the empty-archive case surfaces here
        // as a "failed to parse" error.
        auto load(const IndexEntry& entry) const
            -> expected<SerializationRecord, std::string>
        {
            std::ifstream in(path_, std::ios::binary);
            if (!in) {
                return make_unexpected(
                    "checkpoint index: unable to reopen '" + path_ + "' for payload read");
            }
            in.seekg(static_cast<std::streamoff>(entry.file_offset), std::ios::beg);
            if (!in) {
                return make_unexpected(
                    "checkpoint index: seek to record start failed in '" + path_ + "'");
            }

            SerializationRecord rec;
            try {
                if (!read_next_record(in, rec)) {
                    return make_unexpected(
                        "checkpoint index: record missing or truncated at offset "
                        + std::to_string(entry.file_offset) + " in '" + path_ + "'");
                }
            } catch (const std::exception& e) {
                return make_unexpected(
                    std::string("checkpoint index: failed to parse record body: ") + e.what());
            }
            return rec;
        }

      private:
        std::string                                                       path_;
        std::mutex                                                        build_mutex_;
        bool                                                              built_ = false;
        std::unordered_map<std::string, std::vector<IndexEntry>>          by_id_;
    };

    // One CheckpointIndex per distinct file path, kept in a file-scope
    // cache so N protocol instances pointing at the same `path` share
    // the single index. If each instance owned its own, the restore-
    // time O(N) build would be paid N times, reintroducing the O(N²)
    // cost the index exists to avoid.
    //
    // The cache holds its entries until program exit by default. For
    // memory-sensitive runs where restores happen only during model
    // initialization — the common case — call
    // `clear_checkpoint_indexes()` (see deserialization.hpp) once the
    // last restore completes; that releases every cached index
    // immediately. Any subsequent restore against a freed path
    // rebuilds lazily on next access.
    //
    // The mutex + map live at namespace scope (not function-local) so
    // the eviction helper below can reach the same storage the lookup
    // uses. Both have internal linkage via the anonymous namespace.
    std::mutex                                                        g_index_cache_mu;
    std::unordered_map<std::string, std::unique_ptr<CheckpointIndex>> g_index_cache;

    CheckpointIndex& checkpoint_index_for(const std::string& path) {
        std::lock_guard<std::mutex> lock(g_index_cache_mu);
        // `operator[]` inserts a default (null) `unique_ptr` on first
        // call for this `path` and returns a reference to the slot.
        // On subsequent calls for the same `path` the slot is already
        // populated, so the `if (!slot)` branch is skipped and we
        // reuse the existing index.
        auto& slot = g_index_cache[path];
        if (!slot) {
            slot = std::unique_ptr<CheckpointIndex>(new CheckpointIndex(path));
        }
        return *slot;
    }

} // namespace

void clear_checkpoint_indexes() {
    // Reach into the anonymous-namespace cache; fine from a linkage
    // standpoint since both this function and the cache live in the
    // same translation unit.
    std::lock_guard<std::mutex> lock(g_index_cache_mu);
    g_index_cache.clear();
}

NgenDeserializationProtocol::NgenDeserializationProtocol(const ModelPtr& model, const Properties& properties)
    : check(false), is_fatal(true) {
    initialize(model, properties);
}

NgenDeserializationProtocol::NgenDeserializationProtocol()
    : check(false), is_fatal(true) {}

NgenDeserializationProtocol::~NgenDeserializationProtocol() = default;

auto NgenDeserializationProtocol::run(const ModelPtr& model, const Context& ctx) const -> expected<void, ProtocolError> {
    if (model == nullptr) {
        return make_unexpected<ProtocolError>( ProtocolError(
            Error::UNITIALIZED_MODEL,
            "Cannot run deserialization protocol with null model."
        ));
    }
    if (!check || !supported) {
        return {};
    }

    try {
        CheckpointIndex& index = checkpoint_index_for(path);
        // `ensure_built` is a no-op after the first call per path, and
        // never fails — an unopenable file leaves an empty index and
        // the `find` below surfaces a standard "no matching record"
        // warning. See `CheckpointIndex::ensure_built` docstring.
        index.ensure_built(id_subset);

        auto entry = index.find(
            ctx.id, by_timestamp, target_timestamp_epoch,
            step_latest, target_step);
        if (!entry) {
            std::stringstream ss;
            ss << "deserialization: no matching record for id '" << ctx.id << "'";
            if (by_timestamp) {
                ss << " at timestamp '" << target_timestamp_epoch << "'";
            } else if (!step_latest) {
                ss << " at step " << target_step;
            }
            ss << " in '" << path << "'";
            return make_unexpected<ProtocolError>( ProtocolError(
                is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING,
                ss.str()
            ));
        }

        auto loaded = index.load(*entry);
        if (!loaded.has_value()) {
            std::stringstream ss;
            ss << "deserialization: " << loaded.error()
               << " for feature id " << ctx.id;
            return make_unexpected<ProtocolError>( ProtocolError(
                is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING,
                ss.str()
            ));
        }

        // Feed the payload back through the reserved SetValue name; the model
        // owns the exact inverse of its save() machinery.
        model->SetValue(SERIALIZATION_STATE_NAME, loaded.value().payload.data());
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "deserialization: BMI or archive exchange failed for '"
           << model->GetComponentName()
           << "' at timestep " << ctx.current_time_step
           << " (" << ctx.timestamp << ") for feature id " << ctx.id
           << ": " << e.what();
        return make_unexpected<ProtocolError>( ProtocolError(
            is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING,
            ss.str()
        ));
    }

    return {};
}

auto NgenDeserializationProtocol::check_support(const ModelPtr& model) -> expected<void, ProtocolError> {
    if (model == nullptr || !model->is_model_initialized()) {
        return make_unexpected<ProtocolError>( ProtocolError(
            Error::UNITIALIZED_MODEL,
            "Cannot check deserialization support for uninitialized model. Disabling deserialization protocol."
        ));
    }

    // Metadata-only probe, same policy as NgenSerializationProtocol: verify
    // each reserved variable's name is recognized AND that the reported unit
    // string matches the protocol convention exactly. No SetValue or
    // GetValue(state) is performed during support detection — the engine
    // owns model state mutation. The restore-specific SetValue(state, ...)
    // cannot be safely probed, and doing so at init time would effectively
    // pre-execute run() on whatever record check_support's companion save
    // left behind; the trade-off is that a model which registers the names
    // with the right units but has a broken SetValue(state) implementation
    // will only fail at the first real run() call.
    // The reserved-variable table is defined in serialization.hpp and
    // shared with the save protocol so the two agree on the exact set
    // of variables a conforming model must expose.
    for (const auto& ev : RESERVED_VARS) {
        std::string reported;
        try {
            reported = model->GetVarUnits(ev.name);
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << "deserialization: model '" << model->GetComponentName()
               << "' does not expose reserved variable '" << ev.name
               << "' (GetVarUnits failed: " << e.what() << ").";
            return make_unexpected<ProtocolError>( ProtocolError(
                Error::INTEGRATION_ERROR, ss.str()
            ));
        }
        if (reported != ev.unit) {
            std::stringstream ss;
            ss << "deserialization: model '" << model->GetComponentName()
               << "' exposes reserved variable '" << ev.name
               << "' but with unit '" << reported << "'; expected '" << ev.unit << "'.";
            return make_unexpected<ProtocolError>( ProtocolError(
                Error::INTEGRATION_ERROR, ss.str()
            ));
        }
    }
    this->supported = true;
    return {};
}

auto NgenDeserializationProtocol::initialize(const ModelPtr& model, const Properties& properties) -> expected<void, ProtocolError> {
    auto top_it = properties.find(SERIALIZATION_CONFIGURATION_KEY);
    if (top_it == properties.end()) {
        check = false;
        return {};
    }
    geojson::PropertyMap top = top_it->second.get_values();

    auto path_it = top.find(SERIALIZATION_PATH_KEY);
    if (path_it != top.end()) {
        path = path_it->second.as_string();
    }

    auto restore_it = top.find(DESERIALIZATION_RESTORE_KEY);
    if (restore_it == top.end()) {
        check = false;
        return {};
    }
    geojson::PropertyMap restore = restore_it->second.get_values();

    auto _it = restore.find(SERIALIZATION_CHECK_KEY);
    if (_it != restore.end()) {
        check = _it->second.as_boolean();
    } else {
        check = true;
    }

    _it = restore.find(SERIALIZATION_FATAL_KEY);
    if (_it != restore.end()) {
        is_fatal = _it->second.as_boolean();
    }

    // Optional id_subset: scope the checkpoint index to a known set of ids.
    // Callers that omit this key pay the full-file index cost on first
    // restore but don't have to enumerate their features up front; callers
    // that supply it get a smaller in-memory index proportional to the
    // subset.
    auto subset_it = restore.find(DESERIALIZATION_ID_SUBSET_KEY);
    if (subset_it != restore.end()) {
        try {
            auto entries = subset_it->second.as_list();
            id_subset.clear();
            id_subset.reserve(entries.size());
            for (const auto& e : entries) {
                id_subset.push_back(e.as_string());
            }
        } catch (const std::exception&) {
            // A malformed id_subset is non-fatal — fall through to
            // "index everything" semantics. The property tree layer
            // throws on type mismatch; we'd rather surface that as a
            // warning than abort restore entirely.
            id_subset.clear();
        }
    }

    // Match-mode precedence: `timestamp` beats `step` if both are specified.
    // If only `step` is set, use step-based matching. If neither, fall back to
    // "latest" step for this id.
    auto ts_it = restore.find(DESERIALIZATION_TIMESTAMP_KEY);
    if (ts_it != restore.end()) {
        by_timestamp = true;
        target_timestamp_epoch = parse_timestamp(ts_it->second.as_string());
    } else {
        by_timestamp = false;
        _it = restore.find(DESERIALIZATION_STEP_KEY);
        if (_it != restore.end()) {
            // The property layer reports the underlying JSON type; accept either
            // a string (expected value: "latest") or an integer.
            try {
                std::string s = _it->second.as_string();
                if (s == DESERIALIZATION_STEP_LATEST) {
                    step_latest = true;
                } else {
                    // Numeric string fallback (e.g. "3").
                    step_latest = false;
                    target_step = std::stoi(s);
                }
            } catch (const std::exception&) {
                step_latest = false;
                target_step = static_cast<int>(_it->second.as_natural_number());
            }
        } else {
            step_latest = true;
        }
    }

    if (check && path.empty()) {
        check = false;
        return error_or_warning( ProtocolError(
            Error::PROTOCOL_WARNING,
            "deserialization: 'path' not specified at the top-level serialization block; disabling restore protocol."
        ));
    }

    if (check) {
        auto probe = check_support(model);
        if (!probe.has_value()) {
            // Escalate only on true *non-conformance* (INTEGRATION_ERROR:
            // the model is loaded and initialized but its declared units
            // don't match the protocol). UNITIALIZED_MODEL and other
            // probe failures stay on the warn-and-disable path so that
            // a null model reported at init fails at run() time with the
            // usual UNITIALIZED_MODEL signal instead of throwing from
            // the constructor.
            if (is_fatal && probe.error().error_code() == Error::INTEGRATION_ERROR) {
                throw ProtocolError(Error::PROTOCOL_ERROR,
                                    probe.error().get_message());
            }
            // Non-fatal (or non-conformance error of another kind):
            // warn, disable, make run() a no-op.
            error_or_warning(probe.error());
            check = false;
        }
    }
    return {};
}

bool NgenDeserializationProtocol::is_supported() const {
    return this->supported;
}

}}} // end namespace models::bmi::protocols
