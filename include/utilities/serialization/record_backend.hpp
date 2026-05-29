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
Abstract record-storage interface.

`RecordBackend` is the engine-agnostic abstraction over "how
does this `Record` end up persisted and retrievable later."
Concrete backends implement the interface and choose their own
storage details; the abstract surface is silent on byte layout.

This file declares the abstract surface and per-method API
contracts. Cross-cutting design — sub-handle lifetime patterns,
error categorization, single-in-flight rule, durability,
identity, recommended call patterns — lives in this directory's
README.md; that's the discoverable reference for backend
implementers.
*/
#pragma once

#include "id_predicates.hpp"
#include "record.hpp"

#include <nonstd/expected.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ngen{ namespace serialization{

// Pull `expected` and `make_unexpected` into the library namespace
// so signatures below stay unqualified. Switching the underlying
// implementation (e.g. to std::expected when the codebase moves to
// C++23) is a one-line change here.
using nonstd::expected;
using nonstd::make_unexpected;

/** @brief Save-time durability requirement.
 *
 *  Backend-specific in meaning; the contract is that `strict`
 *  surfaces errors synchronously and blocks until durably stored,
 *  while `relaxed` is best-effort with deferred error surfacing.
 */
enum class Durability { relaxed, strict };

/** @brief Categorized error carried on every abstract-backend
 *  method's `expected<>` error arm.
 *
 *  Callers branch on `kind` (NotFound / Corrupted / IOError)
 *  without parsing `message`; concrete backends translate their
 *  native error sources into this small vocabulary. See the
 *  README ("Error categorization" + "Writing a new
 *  `RecordBackend`") for the extension rule.
 */
struct BackendError {
    enum class Kind { NotFound, Corrupted, IOError };
    Kind        kind;
    std::string message;

    BackendError() = default;
    BackendError(Kind k, std::string m)
        : kind(k), message(std::move(m)) {}
};

/** @brief Abstract storage interface for `Record`s. See the
 *  README for the cross-cutting rules every backend must
 *  satisfy; this header documents the per-method API contract. */
class RecordBackend {
public:
    /** A writer for one checkpoint event. Obtained only via
     *  `RecordBackend::writer()`; cannot be constructed
     *  standalone or mixed across backends. */
    class Writer {
    public:
        virtual ~Writer() = default;

        /** Append a record to this checkpoint event. Error arm
         *  carries a backend-specific diagnostic if the append
         *  fails (I/O error, oversized payload, etc.). */
        [[nodiscard]] virtual auto write(const Record& record)
            -> expected<void, BackendError> = 0;

        /** Commit the event: flush pending writes, make them
         *  durable (under `Durability::strict`), surface deferred
         *  errors, run any collective barriers the backend
         *  requires. With `Durability::relaxed`, durability is
         *  best-effort and errors may not surface until the
         *  parent backend's `finalize()` call. */
        [[nodiscard]] virtual auto commit()
            -> expected<void, BackendError> = 0;
    };

    /** A read handle for one restore session. Scoped at
     *  construction by an `IdPredicate`. Obtained only via
     *  `RecordBackend::reader()`; cannot be constructed
     *  standalone or mixed across backends. "Not found" is
     *  returned on the `expected<>` error arm as
     *  `BackendError::Kind::NotFound`. */
    class Reader {
    public:
        virtual ~Reader() = default;

        /** Return the latest record for @p id within this Reader's
         *  scope. Ordering: highest `time_step` first, ties broken
         *  by highest `checkpoint_epoch` (latest write wins). */
        [[nodiscard]] virtual auto find_latest(const std::string& id)
            -> expected<Record, BackendError> = 0;

        /** Return the record for @p id at the given simulation
         *  step. Multiple records with the same (id, time_step)
         *  resolve to the latest `checkpoint_epoch`. */
        [[nodiscard]] virtual auto find_at_step(const std::string& id,
                                                int64_t time_step)
            -> expected<Record, BackendError> = 0;

        /** Return the record for @p id at the given simulation
         *  timestamp (seconds since Unix epoch). Multiple records
         *  with the same (id, simulation_timestamp) resolve to
         *  the latest `checkpoint_epoch`. */
        [[nodiscard]] virtual auto
        find_at_simulation_timestamp(const std::string& id,
                                     int64_t simulation_timestamp)
            -> expected<Record, BackendError> = 0;

        /** Optional explicit close. Default is a no-op success;
         *  backends with fallible read-side teardown override to
         *  surface those errors. After a successful `close()`,
         *  subsequent `find_*` calls have undefined behavior.
         *  `with_reader` calls this automatically on success. */
        [[nodiscard]] virtual auto close() -> expected<void, BackendError> {
            return {};
        }
    };

    virtual ~RecordBackend() = default;

    /** Construct a `Writer` for one checkpoint event. Backends
     *  MUST return the error arm when a previous Writer is still
     *  alive on the same instance (single-in-flight rule).
     *  Callers should prefer `with_writer` below, which makes
     *  the rule a syntactic property of the call site. */
    [[nodiscard]] virtual auto
    writer(std::chrono::system_clock::time_point epoch,
           Durability durability)
        -> expected<std::unique_ptr<Writer>, BackendError> = 0;

    /** Construct a `Reader` scoped to records whose id satisfies
     *  @p in_scope. The predicate is both a hint and an enforced
     *  scope — out-of-scope ids return `Kind::NotFound`. An empty
     *  predicate means "no scope" — the Reader will find any
     *  record. */
    [[nodiscard]] virtual auto reader(IdPredicate in_scope = {})
        -> expected<std::unique_ptr<Reader>, BackendError> = 0;

    /** Lifecycle hook for cross-event cleanup: close process-level
     *  open handles, drain async queues, surface deferred errors.
     *  Engine calls this at MPI synchronization points or before
     *  shutdown.
     *
     *  Error arm carries the vector of every `BackendError`
     *  accumulated since the last `finalize()` call — typically
     *  those deferred under `Durability::relaxed` from earlier
     *  `write()`/`commit()` calls, or any errors that surfaced
     *  during finalize itself. Each entry keeps its own structured
     *  `Kind` and `message`, so callers can branch per-error:
     *
     *      if (auto r = backend->finalize(); !r) {
     *          for (const auto& e : r.error()) {
     *              switch (e.kind) {
     *                  case BackendError::Kind::IOError:   ...
     *                  case BackendError::Kind::Corrupted: ...
     *                  case BackendError::Kind::NotFound:  ...
     *              }
     *          }
     *      }
     *
     *  An empty deferred-error set is success — the success arm
     *  is returned, not an error arm with an empty vector. */
    [[nodiscard]] virtual auto finalize()
        -> expected<void, std::vector<BackendError>> = 0;

    // Scoped convenience wrappers — recommended call pattern.
    // Non-virtual; concrete backends do not override them. See
    // the README for why they exist and what they enforce.

    /** Run @p fn inside a scoped Writer; calls `commit()`
     *  automatically on success. Exceptions from @p fn unwind
     *  through the Writer destructor and re-throw to the caller.
     *  @tparam F  invocable as `void(Writer&)`. */
    template <typename F>
    [[nodiscard]] auto with_writer(std::chrono::system_clock::time_point epoch,
                                   Durability durability, F&& fn)
        -> expected<void, BackendError>
    {
        auto wo = writer(epoch, durability);
        if (!wo) return make_unexpected(std::move(wo.error()));
        std::forward<F>(fn)(*wo.value());
        return wo.value()->commit();
    }

    /** Run @p fn inside a scoped Reader; calls `close()`
     *  automatically on success. Exceptions from @p fn unwind
     *  through the Reader destructor and re-throw to the caller.
     *  @tparam F  invocable as `void(Reader&)`. */
    template <typename F>
    [[nodiscard]] auto with_reader(IdPredicate in_scope, F&& fn)
        -> expected<void, BackendError>
    {
        auto ro = reader(std::move(in_scope));
        if (!ro) return make_unexpected(std::move(ro.error()));
        std::forward<F>(fn)(*ro.value());
        return ro.value()->close();
    }
};

}}  // namespace ngen::serialization
