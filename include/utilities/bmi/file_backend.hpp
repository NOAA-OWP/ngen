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
File-based `RecordBackend` implementation.

Persists records using the v0.2 wire format defined in
`src/utilities/bmi/wire_format.hpp`: each record is a fixed-size
prefix + id bytes + payload bytes, written sequentially to a
single append-mode file. A FileBackend opened for a given path
walks the file once at construction to build an in-memory index
keyed by `Record::id`, then services `find_*` queries via index
lookup + per-record seek.

Per-path sharing
----------------
`FileBackend` is **shared per path** via a process-static
registry. `FileBackend::create(path, scope)` is a `get_or_create`
factory: the first caller for a given path constructs the backend,
walks the file, and registers the resulting `shared_ptr` under a
`weak_ptr` slot. Subsequent callers for the same path receive the
same `shared_ptr`. The backend (and its index, and its write fd)
die when the last caller drops their handle; the `weak_ptr` slot
becomes recoverable and is opportunistically pruned on the next
lookup.

This pattern is what makes N independent clients on the same
checkpoint file viable at scale: N callers that each ask the
registry for a backend at the same path share **one** index walk,
**one** write fd, and **one** mutex-serialized write critical
section, instead of paying N times.

Two-layer scope filtering
-------------------------
`IdPredicate` filtering happens at two distinct layers:

  - **Construction scope** — passed to `create(path, scope)`.
    Records whose id does not satisfy the predicate are never
    added to the in-memory index. This bounds index memory and
    is the coarse-grained "what's worth indexing at all" filter
    — set once when the backend is first constructed.

    Because the registry is keyed by path only, the first caller's
    `scope` is what the index is built against. Subsequent callers
    requesting the same path get the cached backend regardless of
    the `scope` they pass. **Callers MUST agree on the construction
    scope for a given path** — this is a contract on the
    application that uses the library, not a runtime check the
    library performs.

  - **Read scope** — passed to `reader(scope)`. Per-Reader
    predicate applied at `find_*` time. A record is returned iff
    *both* the construction-time predicate (already enforced by
    not being in the index) AND the read-time predicate accept its
    id. The read scope is how a single Reader narrows further
    within a backend whose construction scope was deliberately
    broadened.

Durability semantics
--------------------
`RecordBackend`'s abstract contract distinguishes `Durability::strict`
(synchronous + durable) from `Durability::relaxed` (best-effort
durability, errors *may* be deferred to `finalize()`). FileBackend
fills in those two parameters as follows:

  - **`strict`**: `commit()` calls `::fsync()` on the owned fd
    before returning, ensuring kernel page-cache pages reach the
    storage device. All write- and commit-side I/O failures
    surface synchronously through the call's `expected<>` return
    arm. Matches the abstract contract exactly.

  - **`relaxed`**: `commit()` skips `::fsync()` — bytes are
    handed to the kernel and may sit in the page cache when
    `commit()` returns. The OS will eventually flush, but a
    crash between commit and OS flush can lose those bytes.
    This is the documented "best-effort durability" the abstract
    contract permits.

    However, FileBackend's relaxed mode **still surfaces write-
    and commit-side errors immediately** through the call's
    `expected<>` return arm. It does NOT use the abstract
    contract's optional deferred-error mechanism. The underlying
    I/O is synchronous (`::write()` returns before `commit()`
    runs), so there is no natural moment of deferred discovery
    for FileBackend — any error is known at the originating call.

    A consequence: callers of FileBackend do not need to call
    `finalize()` to learn about per-write or per-commit failures.
    `write().has_value()` and `commit().has_value()` are full
    immediate signals regardless of durability mode. `finalize()`
    returns success on a clean FileBackend by construction.

    The `deferred_errors_` member and `finalize()` aggregation
    path still exist because they're required by the abstract
    `RecordBackend` interface; for this backend the vector
    remains empty in practice at this time.

Sub-handle lifetime patterns
----------------------------
See `include/utilities/serialization/README.md` for the general
abstract-surface contract. FileBackend uses:

  - **Writer** (shared-state, thin handle): holds
    `shared_ptr<FileBackend>` so the parent backend's owned fd and
    write mutex remain reachable. Writers do not own the fd; they
    serialize their write+commit through the backend's
    `io_mutex_`. Multiple Writer handles may coexist on one
    backend — they take turns through the mutex. The single
    `::open()`/`::close()` on the fd happens at backend lazy-open
    / destruction, not per-Writer.

  - **Reader** (snapshot): owns its own `ifstream` (independent
    read cursor) and a `shared_ptr<const Index>` (the backend's
    immutable indexed snapshot). Carries its own
    `IdPredicate` for read-time filtering. Multiple Readers on one
    backend share the Index via `shared_ptr` aliasing.

FileBackend MUST be constructed via the static `create()` factory.
The private constructor + factory enforces both the registry
discipline (no rogue direct instantiation) and the
`shared_ptr` management the Writer pattern depends on.

Index freshness and the dirty flag
----------------------------------
The Index is a point-in-time snapshot. A Writer appending records
after the Index was built does NOT update the Index — the
snapshot stays frozen. A `dirty_` flag flipped under the
`io_mutex_` on every write marks the backend as having had
post-construction appends; the next `reader()` call observes the
flag and rebuilds the Index before returning the Reader. For the
canonical "restore at init, save during run" workflow the flag
never fires after restore-init completes (no one opens new
Readers after that point). For mid-run restore-after-save it does
the obvious right thing.

Duplicate-key resolution
------------------------
When the index walk encounters two records with the same
`(id, time_step, checkpoint_epoch)` tuple, the latest-offset
record wins for lookup and a WARNING is emitted on stderr. This
makes accidental cross-process collisions visible without
forcing a policy on the protocol layer.
*/
#pragma once

#include "utilities/serialization/record_backend.hpp"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace ngen {
namespace serialization {

class FileBackend
    : public RecordBackend
    , public std::enable_shared_from_this<FileBackend> {
  public:
    /** Construct (or retrieve from registry) a `shared_ptr`-managed
     *  FileBackend at @p path with the given index-construction
     *  scope.
     *
     *  Behavior:
     *    - First call for @p path constructs the backend, walks the
     *      file once applying @p construction_scope to filter the
     *      index, opens nothing for write (write fd is lazy), and
     *      registers the resulting shared_ptr.
     *    - Subsequent calls for the same @p path return the cached
     *      backend. The `construction_scope` argument is ignored on
     *      cached hits — callers must agree on scope for a given
     *      path. The application is responsible for ensuring
     *      agreement; the library does not detect mismatches at
     *      runtime.
     *
     *  An empty (default-constructed) `construction_scope` means
     *  "index all records."
     *
     *  Returns the error arm if the initial index walk surfaces a
     *  non-recoverable problem (e.g. a partially constructed
     *  backend would silently misbehave). A missing file at @p path
     *  is NOT an error — the backend is constructed with an empty
     *  index, which the Reader honors as "every find returns
     *  NotFound." */
    [[nodiscard]]
    static auto create(std::string path, IdPredicate construction_scope = {})
        -> expected<std::shared_ptr<FileBackend>, BackendError>;

    ~FileBackend() override;

    // -- RecordBackend abstract surface --------------------------------

    [[nodiscard]]
    auto writer(std::chrono::system_clock::time_point epoch, Durability durability)
        -> expected<std::unique_ptr<RecordBackend::Writer>, BackendError> override;

    [[nodiscard]]
    auto reader(IdPredicate read_scope = {})
        -> expected<std::unique_ptr<RecordBackend::Reader>, BackendError> override;

    [[nodiscard]]
    auto finalize() -> expected<void, std::vector<BackendError>> override;

    // -- Accessors (diagnostics / tests) -------------------------------

    /** Path the backend was constructed against. */
    const std::string& path() const noexcept {
        return path_;
    }

  private:
    FileBackend(std::string path, IdPredicate construction_scope);

    // Sub-handle and index types; full definitions live in
    // file_backend.cpp. As nested classes, they have member-level
    // access to FileBackend's private state (the C++ rule under
    // [class.access.nest]) — no `friend` declaration needed for
    // Writer to reach `io_mutex_`, `fd_`, `dirty_`, etc.
    class Writer;
    class Reader;
    class Index;

    // -- Internal helpers ---------------------------------------------

    // Build (or rebuild) the index from disk applying
    // `construction_scope_`. Caller must hold `io_mutex_` (or be
    // inside the construction path where no other reference to
    // `this` exists yet). Sets `dirty_` to false on success.
    auto build_index_locked_() -> expected<void, BackendError>;

    // Open the write fd if not already open. Must be called with
    // `io_mutex_` held. POSIX-only path uses `::open`; non-POSIX
    // refuses with an IOError directing callers to use relaxed
    // durability via stream fallback (which is handled inside the
    // Writer's commit/write code under #ifndef NGEN_HAVE_POSIX_FSYNC).
    auto ensure_write_fd_locked_() -> expected<void, BackendError>;

    // -- State --------------------------------------------------------

    const std::string path_;
    const IdPredicate construction_scope_;

    // Index built once at construction (under construction_scope_)
    // and rebuilt on demand if `dirty_` is set when a Reader is
    // requested. Shared with every Reader via shared_ptr<const>.
    std::shared_ptr<const Index> index_;

    // Single fd backing all writes; opened lazily on first write
    // request, closed in the destructor. -1 when not yet opened or
    // not supported on this build.
    int fd_ = -1;

    // Serializes the write critical section (prefix + id + payload
    // + optional fsync) so concurrent Writers cannot interleave a
    // record's bytes. Also guards `fd_`, `dirty_`, and the
    // deferred-error vector.
    mutable std::mutex io_mutex_;

    // Set true under `io_mutex_` after any successful write that
    // mutates on-disk state. Cleared by `build_index_locked_()`. A
    // `reader()` call that observes `dirty_ == true` rebuilds the
    // index before returning.
    bool dirty_ = false;

    // Deferred errors stashed by Writers under
    // `Durability::relaxed`; drained and returned by `finalize()`.
    std::vector<BackendError> deferred_errors_;

    // -- Registry -----------------------------------------------------

    // Process-static path-keyed registry. weak_ptr<> entries allow
    // automatic eviction: when the last shared_ptr<FileBackend> for
    // a path drops, the backend is destroyed and the slot becomes
    // recoverable. The registry is pruned opportunistically inside
    // the critical section on every `create()` call.
    static std::mutex registry_mutex_;
    static std::unordered_map<std::string, std::weak_ptr<FileBackend>> registry_;
};

} // namespace serialization
} // namespace ngen
