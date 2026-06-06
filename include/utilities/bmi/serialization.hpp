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
Interface of the save-side ngen BMI state serialization protocol. Pairs
with NgenDeserializationProtocol (restore, see deserialization.hpp) and
shares the on-disk SerializationRecord format (see serialization_record.hpp).
Record identity per run() call is taken from Context::id.
*/
#pragma once

#include "utilities/serialization/record_backend.hpp"
#include <memory>
#include <nonstd/expected.hpp>
#include <protocol.hpp>
#include <string>
#include <vector>

namespace models {
namespace bmi {
namespace protocols {
using nonstd::expected;

/** Reserved BMI variable names driving the protocol's capture sequence. */
constexpr const char* const SERIALIZATION_CREATE_NAME = "ngen::serialization_create";
constexpr const char* const SERIALIZATION_FREE_NAME   = "ngen::serialization_free";
constexpr const char* const SERIALIZATION_SIZE_NAME   = "ngen::serialization_size";
constexpr const char* const SERIALIZATION_STATE_NAME  = "ngen::serialization_state";

/** Top-level configuration key. Shared by save and restore protocols. */
constexpr const char* const SERIALIZATION_CONFIGURATION_KEY = "serialization";
/** Shared path key at the top-level block. */
constexpr const char* const SERIALIZATION_PATH_KEY = "path";
/** Save-side sub-block key. */
constexpr const char* const SERIALIZATION_SAVE_KEY = "save";
/** Save-side / restore-side shared sub-keys. */
constexpr const char* const SERIALIZATION_CHECK_KEY     = "check";
constexpr const char* const SERIALIZATION_FATAL_KEY     = "fatal";
constexpr const char* const SERIALIZATION_FREQUENCY_KEY = "frequency";

/** (Reserved-name, expected-unit-string) pair used by both the save
 *  and restore protocols' `check_support` probes. Hoisted to the
 *  shared header so the two protocols agree on the exact set of
 *  variables they require a model to expose, and so neither .cpp
 *  rebuilds the table on every probe call. */
struct ReservedVar {
    const char* name;
    const char* unit;
};

constexpr ReservedVar RESERVED_VARS[] = {
    {SERIALIZATION_CREATE_NAME, "ngen::trigger"},
    {SERIALIZATION_FREE_NAME,   "ngen::trigger"},
    {SERIALIZATION_SIZE_NAME,   "bytes"        },
    {SERIALIZATION_STATE_NAME,  "ngen::opaque" },
};

class NgenSerializationProtocol : public NgenBmiProtocol {
    /** @brief Save-side state serialization protocol.
     *
     * Drives the four reserved BMI variables to capture a model's state and
     * appends the bytes to a shared file as a SerializationRecord tagged
     * with the feature id (from Context) and current time step.
     *
     *   SetValue(create) -> GetValue(size) -> GetValue(state) -> append record -> SetValue(free)
     *
     * All protocol instances pointed at the same `path` write into the same
     * file; one entity (id) may have at most one record per time step but
     * may have records at multiple steps.
     *
     * Thread safety
     * -------------
     * Storage I/O is delegated to a `shared_ptr<RecordBackend>` that
     * owns the file handle and the write-side mutex. The default
     * `FileBackend` is **shared per path** (path-keyed registry),
     * so N protocol instances configured against the same
     * `serialization.path` (the realization-level inheritance
     * pattern) cooperate through one backend: one fd, one
     * `io_mutex_`, no record interleaving across features. The
     * protocol itself holds no mutable I/O state — each `run()`
     * call acquires a thin Writer handle, writes, commits, and
     * drops the handle. Concurrent `run()` calls on the same or
     * different protocol instances sharing a backend are
     * serialized by the backend's mutex at write-granularity.
     *
     * The BMI capture scope (`SetValue(create) ... SetValue(free)`)
     * is per-call and managed by an RAII `ScopedCapture` guard so
     * an exception partway through still releases the model-side
     * resources.
     *
     * Cross-process coordination (separate ngen processes writing
     * to the same file) is NOT this protocol's concern — engines
     * that fan writes out across processes own that coordination,
     * typically via per-rank file naming (see the `{{rank}}`
     * templating in the docs).
     */
  public:
    NgenSerializationProtocol(const ModelPtr& model, const Properties& properties);
    NgenSerializationProtocol();
    virtual ~NgenSerializationProtocol() override;

  private:
    /** @brief Append one state record to the shared file when the frequency fires.
     *
     * Frequency semantics match NgenMassBalance::run():
     *   frequency > 0 : fire every `frequency` time steps
     *   frequency < 0 : fire only at the final time step
     *   frequency == 0 : disabled at initialize() time
     */
    auto run(const ModelPtr& model, const Context& ctx) const
        -> expected<void, ProtocolError> override;

    /** @brief Probe the model for all four reserved serialization variable names. */
    nsel_NODISCARD auto check_support(const ModelPtr& model)
        -> expected<void, ProtocolError> override;

    /** @brief Parse top-level `path` and the `save` sub-block from the
     * `serialization` configuration.
     *
     * Recognized keys:
     *   serialization.path            (string, required when save.check is true)
     *   serialization.save.check      (bool,   default true when `save` block present)
     *   serialization.save.frequency  (int,    default 1; see run())
     *   serialization.save.fatal      (bool,   default true)
     *
     * Absent top-level or save sub-block disables the protocol silently.
     */
    auto initialize(const ModelPtr& model, const Properties& properties)
        -> expected<void, ProtocolError> override;

    bool is_supported() const override final;

  private:
    int frequency;
    bool supported = false;
    bool check;
    bool is_fatal;

    // Storage backend. Acquired in initialize() via
    // `FileBackend::create(path, ...)`, which is a path-keyed
    // get_or_create — all protocol instances on the same
    // realization that share `serialization.path` end up with the
    // same `shared_ptr<FileBackend>`. The backend owns the write
    // fd (opened lazily, lived for the backend's lifetime) and
    // the mutex that serializes record writes across all Writer
    // handles. `nullptr` means save is disabled / unconfigured.
    std::shared_ptr<::ngen::serialization::RecordBackend> backend_;

    // Reusable per-instance scratch for the model's GetValue
    // payload. Lives across `run()` calls — once the model's
    // state size has stabilized (which happens after the first
    // call), subsequent resize() calls are no-ops and avoid
    // re-allocation. Mutated from the const override of run().
    mutable std::vector<char> payload_buffer_;
};

} // namespace protocols
} // namespace bmi
} // namespace models
