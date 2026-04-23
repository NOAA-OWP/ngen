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
Version 0.1
Interface of the BMI state restore protocol. Pairs with
NgenSerializationProtocol (save); shares the on-disk SerializationRecord
format defined in serialization_record.hpp.

Restore at scale: the protocol delegates the file walk to a process-
global `CheckpointIndex` keyed by file path. The first restore against a
given file builds an in-memory `id -> [(step, timestamp, file_offset)]`
index in a single O(N) pass; subsequent restores are O(log k) per feature
where k is the number of records for that feature. This turns a
naive-O(features × records) scan into O(records + features) for the
whole run. Optional `id_subset` further caps memory by keeping index
entries only for ids the caller intends to restore.

The cache survives beyond the lifetime of individual protocol
instances. Callers running in memory-constrained contexts where
restores happen only at initialization should invoke
`clear_checkpoint_indexes()` (below) once the initialization-phase
restore sequence completes, releasing the entire cache immediately.
*/
#pragma once

#include <string>
#include <vector>
#include <protocol.hpp>
#include <nonstd/expected.hpp>

namespace models{ namespace bmi{ namespace protocols{
    using nonstd::expected;

    /** Restore-side sub-block key under the top-level `serialization` block. */
    constexpr const char* const DESERIALIZATION_RESTORE_KEY    = "restore";
    /** Restore-side "which step to load" key. Accepts an integer or "latest". */
    constexpr const char* const DESERIALIZATION_STEP_KEY       = "step";
    /** Sentinel value for `step` meaning "latest step present for this id". */
    constexpr const char* const DESERIALIZATION_STEP_LATEST    = "latest";
    /** Restore-side "match by simulation timestamp" key. Parsed as an
     *  integer (seconds since epoch, or any monotone encoding the caller
     *  chose when writing) and compared against the record's `timestamp`
     *  field. If both `timestamp` and `step` are configured, `timestamp`
     *  wins. */
    constexpr const char* const DESERIALIZATION_TIMESTAMP_KEY  = "timestamp";
    /** Optional restore-side list of ids to index. When present, only
     *  records whose id is in this subset are retained in the checkpoint
     *  index — useful at scale when a run only needs to restore a known
     *  subset of features out of a much larger checkpoint file. */
    constexpr const char* const DESERIALIZATION_ID_SUBSET_KEY  = "id_subset";

    class NgenDeserializationProtocol : public NgenBmiProtocol {
        /** @brief Restore-side state deserialization protocol.
         *
         * Sibling of NgenSerializationProtocol. At `run()`, looks up a
         * SerializationRecord matching the Context's `id` in the shared
         * `CheckpointIndex` for the configured path and writes the payload
         * bytes to the model via `SetValue(ngen::serialization_state, ...)`.
         *
         * The protocol does not care about the simulation's current time
         * step — restore is a one-shot operation and the engine is expected
         * to invoke it at whatever lifecycle point makes sense. Subsequent
         * `run()` calls are idempotent but will re-read and re-apply the
         * same record; callers that want "restore once" should run the
         * protocol a single time.
         *
         * Thread safety
         * -------------
         * The `CheckpointIndex` guards its build with a per-path mutex, so
         * concurrent first-restores of different features sharing a path
         * serialize on build but run lookups in parallel afterwards. The
         * restore protocol itself holds no mutable per-instance I/O state;
         * the per-call payload read goes through a local ifstream so two
         * threads restoring distinct features can proceed without locking
         * out each other's reads.
         */
      public:
        NgenDeserializationProtocol(const ModelPtr& model, const Properties& properties);
        NgenDeserializationProtocol();
        virtual ~NgenDeserializationProtocol() override;

      private:
        auto run(const ModelPtr& model, const Context& ctx) const -> expected<void, ProtocolError> override;
        nsel_NODISCARD auto check_support(const ModelPtr& model) -> expected<void, ProtocolError> override;

        /** @brief Parse top-level `path` and the `restore` sub-block.
         *
         * Recognized keys:
         *   serialization.path                (string, required when restore.check is true)
         *   serialization.restore.check       (bool, default true when `restore` block present)
         *   serialization.restore.fatal       (bool, default true)
         *   serialization.restore.step        ("latest" or int, default "latest")
         *   serialization.restore.timestamp   (string, optional; wins over `step` if set)
         *   serialization.restore.id_subset   (array of strings, optional; caps index memory)
         *
         * Absent top-level block or absent `restore` sub-block disables the
         * protocol silently.
         */
        auto initialize(const ModelPtr& model, const Properties& properties) -> expected<void, ProtocolError> override;

        bool is_supported() const override final;

      private:
        std::string path;
        // Match mode. When `by_timestamp` is true, `target_timestamp_epoch`
        // is the sole matching criterion. Otherwise `step_latest` (if true)
        // finds the highest step for the id, else `target_step` must match
        // exactly.
        bool        by_timestamp = false;
        int64_t     target_timestamp_epoch = 0;
        bool        step_latest = true;
        int         target_step = 0;
        // Optional: ids the caller intends to restore. Passed to the shared
        // CheckpointIndex at build time so unrelated records are skipped.
        // Empty = "index everything"; see CheckpointIndex::ensure_built.
        std::vector<std::string> id_subset;
        bool        supported = false;
        bool        check;
        bool        is_fatal;
    };

    /** @brief Release every cached `CheckpointIndex` immediately.
     *
     * At scale each per-path index pins memory proportional to the
     * number of records it holds. In the common workflow where
     * restores happen only during model initialization, the indexes
     * are dead weight once every model has been restored — this
     * function frees them on demand rather than waiting for process
     * exit.
     *
     * Thread-safe. Safe to call at any time: any subsequent restore
     * against a freed path will lazily rebuild that path's index from
     * disk on next access. Expected to be called at most once per
     * run by the engine, after the initialization-phase restore
     * sequence completes.
     */
    void clear_checkpoint_indexes();

}}} // end namespace models::bmi::protocols
