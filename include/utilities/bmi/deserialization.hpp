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
Version 0.2
Interface of the BMI state restore protocol. Pairs with
`NgenSerializationProtocol` (save); shares the `Record` value type
defined in `include/utilities/serialization/record.hpp`.

Storage is delegated to a `RecordBackend` acquired at `initialize()`
time, optionally scoped by a caller-provided `id_subset` so that a
run restoring only a known subset of features can give the backend
a chance to build a smaller index.

Each `run()` opens a short-lived `Reader` for the current
`ctx.id` and asks the configured match mode (`latest`, by step, or
by timestamp) for the corresponding record.
*/
#pragma once

#include "utilities/serialization/record_backend.hpp"
#include <memory>
#include <nonstd/expected.hpp>
#include <protocol.hpp>

namespace models {
namespace bmi {
namespace protocols {
using nonstd::expected;

/** Restore-side sub-block key under the top-level `serialization` block. */
constexpr const char* const DESERIALIZATION_RESTORE_KEY = "restore";
/** Restore-side "which step to load" key. Accepts an integer or "latest". */
constexpr const char* const DESERIALIZATION_STEP_KEY = "step";
/** Sentinel value for `step` meaning "latest step present for this id". */
constexpr const char* const DESERIALIZATION_STEP_LATEST = "latest";
/** Restore-side "match by simulation timestamp" key. Parsed as an
 *  integer (seconds since epoch, or any monotone encoding the caller
 *  chose when writing) and compared against the record's `timestamp`
 *  field. If both `timestamp` and `step` are configured, `timestamp`
 *  wins. */
constexpr const char* const DESERIALIZATION_TIMESTAMP_KEY = "timestamp";
/** Optional restore-side list of ids to index. When present, only
 *  records whose id is in this subset are retained in the checkpoint
 *  index — useful at scale when a run only needs to restore a known
 *  subset of features out of a much larger checkpoint file. */
constexpr const char* const DESERIALIZATION_ID_SUBSET_KEY = "id_subset";

class NgenDeserializationProtocol : public NgenBmiProtocol {
    /** @brief Restore-side state deserialization protocol.
     *
     * Sibling of NgenSerializationProtocol. At `run()`, opens a
     * Reader on the protocol's `RecordBackend`, looks up a Record
     * matching the Context's `id` and the configured step/timestamp
     * selector, and writes the payload bytes to the model via
     * `SetValue(ngen::serialization_state, ...)`.
     *
     * The protocol does not care about the simulation's current
     * time step — restore is a one-shot operation and the caller
     * decides at which lifecycle point to invoke it. The protocol
     * does not track whether it has already fired: a second
     * `run()` call re-opens a Reader, looks up the same record
     * (the match-mode config is set once in `initialize()` and
     * never changes), and re-applies the same payload bytes to
     * the model — effectively rewinding it to the restore point.
     * Callers that want "restore once" should invoke the protocol
     * a single time.
     *
     * To restore from a *different* record later in the run (e.g.
     * a multi-phase driver advancing to a later checkpoint without
     * rewinding), construct a separate
     * `NgenDeserializationProtocol` instance configured for the
     * new target step/timestamp. Each instance carries its own
     * match-mode state and its own backend handle; they are
     * independent. Reusing one instance with a different config in
     * mind is not supported — `initialize()` is the only place the
     * match mode is set.
     *
     * Scope filtering applies at two layers:
     *   - Construction scope: an optional `id_subset` passed at
     *     `initialize()` time bounds the backend's index to the
     *     caller's intended subset. Records outside that subset
     *     are never indexed.
     *   - Read scope: each `run()` opens a Reader for the current
     *     `ctx.id` — the one feature the caller is asking about.
     *     The Reader cannot return any other id's record.
     */
  public:
    NgenDeserializationProtocol(const ModelPtr& model, const Properties& properties);
    NgenDeserializationProtocol();
    virtual ~NgenDeserializationProtocol() override;

  private:
    auto run(const ModelPtr& model, const Context& ctx) const
        -> expected<void, ProtocolError> override;
    nsel_NODISCARD auto check_support(const ModelPtr& model)
        -> expected<void, ProtocolError> override;

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
    auto initialize(const ModelPtr& model, const Properties& properties)
        -> expected<void, ProtocolError> override;

    bool is_supported() const override final;

  private:
    // Match mode. When `by_timestamp` is true,
    // `target_simulation_timestamp` is the sole matching criterion.
    // Otherwise `step_latest` (if true) finds the highest step for
    // the id, else `target_step` must match exactly.
    bool by_timestamp                   = false;
    int64_t target_simulation_timestamp = 0;
    bool step_latest                    = true;
    int target_step                     = 0;
    // Optional set of ids the caller wants indexed, parsed from
    // config in initialize() into an
    // `any_of(primary_prefix(...))` predicate. Passed to the
    // backend at construction as its **construction-time**
    // filter — records whose id fails this predicate are not
    // indexed.
    //
    // This is the *backend-level* scope. The *per-Reader* scope
    // used inside `run()` is a tighter `exact_id(ctx.id)`
    // predicate constructed per-call so each Reader returns
    // exactly the one feature's record.
    //
    // Empty / default-constructed means "no filter".
    ::ngen::serialization::IdPredicate id_scope;
    bool supported = false;
    bool check;
    bool is_fatal;

    // Storage backend, acquired in initialize() via a
    // `RecordBackend`-typed factory.
    std::shared_ptr<::ngen::serialization::RecordBackend> backend_;
};

} // namespace protocols
} // namespace bmi
} // namespace models
