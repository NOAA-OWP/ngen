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
Interface of the save-side ngen BMI state serialization protocol. Pairs
with NgenDeserializationProtocol (restore, see deserialization.hpp) and
shares the on-disk SerializationRecord format (see serialization_record.hpp).
Record identity per run() call is taken from Context::id.
*/
#pragma once

#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <protocol.hpp>
#include <nonstd/expected.hpp>

namespace models{ namespace bmi{ namespace protocols{
    using nonstd::expected;

    /** Reserved BMI variable names driving the protocol's capture sequence. */
    constexpr const char* const SERIALIZATION_CREATE_NAME = "ngen::serialization_create";
    constexpr const char* const SERIALIZATION_FREE_NAME   = "ngen::serialization_free";
    constexpr const char* const SERIALIZATION_SIZE_NAME   = "ngen::serialization_size";
    constexpr const char* const SERIALIZATION_STATE_NAME  = "ngen::serialization_state";

    /** Top-level configuration key. Shared by save and restore protocols. */
    constexpr const char* const SERIALIZATION_CONFIGURATION_KEY = "serialization";
    /** Shared path key at the top-level block. */
    constexpr const char* const SERIALIZATION_PATH_KEY          = "path";
    /** Save-side sub-block key. */
    constexpr const char* const SERIALIZATION_SAVE_KEY          = "save";
    /** Save-side / restore-side shared sub-keys. */
    constexpr const char* const SERIALIZATION_CHECK_KEY         = "check";
    constexpr const char* const SERIALIZATION_FATAL_KEY         = "fatal";
    constexpr const char* const SERIALIZATION_FREQUENCY_KEY     = "frequency";

    /** (Reserved-name, expected-unit-string) pair used by both the save
     *  and restore protocols' `check_support` probes. Hoisted to the
     *  shared header so the two protocols agree on the exact set of
     *  variables they require a model to expose, and so neither .cpp
     *  rebuilds the table on every probe call. */
    struct ReservedVar { const char* name; const char* unit; };
    constexpr ReservedVar RESERVED_VARS[] = {
        { SERIALIZATION_CREATE_NAME, "ngen::trigger" },
        { SERIALIZATION_FREE_NAME,   "ngen::trigger" },
        { SERIALIZATION_SIZE_NAME,   "bytes"         },
        { SERIALIZATION_STATE_NAME,  "ngen::opaque"  },
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
         * Thread safety — three regions to reason about
         * ---------------------------------------------
         * Each region is documented at its point of use inside the .cpp; this
         * header lists them as a map so readers can find the relevant comment.
         *
         *   (1) Per-instance I/O state (the persistent ofstream and the
         *       reusable payload buffer): guarded by `io_mutex_` because a
         *       future threaded driver may invoke `run()` for different ids
         *       concurrently against the same protocol instance. Without the
         *       mutex, two threads could interleave archive bytes on the
         *       stream and corrupt the file.
         *
         *   (2) Per-instance BMI capture scope (`SetValue(create) ... SetValue(free)`):
         *       the engine currently owns the single serial driver, so the
         *       BMI model is not concurrently accessed across threads. Even
         *       so, the RAII `ScopedCapture` guard makes the create/free
         *       scope exception-safe and self-documenting — one (create,
         *       free) pair per call, period.
         *
         *   (3) Cross-instance coordination: multiple NgenSerializationProtocol
         *       instances pointed at the same `path` are NOT coordinated by
         *       this class. Concurrent writers across instances would race
         *       on the underlying file. If a driver ever fans out writes to
         *       the same path across threads or processes, that driver must
         *       introduce the coordination layer (e.g. a shared mutex for
         *       threads, or per-rank file naming for MPI — see the
         *       `{{rank}}` templating in the docs). The protocol stays out
         *       of that business on purpose.
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
        auto run(const ModelPtr& model, const Context& ctx) const -> expected<void, ProtocolError> override;

        /** @brief Probe the model for all four reserved serialization variable names. */
        nsel_NODISCARD auto check_support(const ModelPtr& model) -> expected<void, ProtocolError> override;

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
        auto initialize(const ModelPtr& model, const Properties& properties) -> expected<void, ProtocolError> override;

        bool is_supported() const override final;

      private:
        std::string path;
        int         frequency;
        bool        supported = false;
        bool        check;
        bool        is_fatal;

        // See region (1) in the class-level thread-safety map. All four
        // members below are mutated from the `const` override of `run()`
        // via the `mutable` specifier; `io_mutex_` is the one that makes
        // that mutation safe to do across threads.
        mutable std::mutex                   io_mutex_;
        mutable std::unique_ptr<std::ofstream> out_;
        mutable std::vector<char>            payload_buffer_;
        // Explicit stream buffer sized for a "typical" record prefix —
        // keeps ofstream from reallocating its internal buffer under load.
        mutable std::vector<char>            stream_buffer_;
    };

}}} // end namespace models::bmi::protocols
