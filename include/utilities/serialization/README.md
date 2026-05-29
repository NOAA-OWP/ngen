# `ngen_serialization` library

Engine-agnostic abstractions for record-based state checkpointing.
This document captures the design rationale, the rules every
concrete backend must satisfy, and the recommended consumer call
patterns.

The audience for this document is **developers writing or reviewing
a `RecordBackend` implementation** or library-internal code. Model
authors and end-user operators have separate docs (see below).

## Library version

**v0.1.0** — initial release. The library API version is exposed at
compile time via `LIBRARY_VERSION_{MAJOR,MINOR,PATCH}` constants in
[`version.hpp`](version.hpp). Consumers that depend on a feature
added in a particular release can `static_assert` against the
constants; the library itself does not read or enforce the version.

### Pre-1.0: no API compatibility guarantee

Every release in the **0.X** series MAY change the public abstract
surface without a MAJOR bump. The library is in active design; the
abstract `RecordBackend` interface, the `BackendError::Kind` enum,
the `IdPredicate` kit, and the scoped wrapper signatures are all
subject to revision based on what we learn from the first concrete
backend implementations. Consumers should pin to an exact 0.X.Y
release and expect to update on each new release. A 1.0.0 release
will lock in whatever the abstract surface has settled into by
that point.

The 0.X minor/patch numbers are still useful as release markers —
0.2.0 ships something materially different from 0.1.0 — but the
MAJOR / MINOR / PATCH distinction below only becomes a compatibility
guarantee at 1.0 and beyond.

### Post-1.0 policy (what the numbers will mean once we get there)

| Bump | Means |
|---|---|
| **MAJOR** | Incompatible change to the public abstract surface — renames, removed methods, signature changes on virtual methods, behavioral changes existing backends would react to. |
| **MINOR** | Additive — new methods with default implementations, new predicate-kit helpers, new `BackendError::Kind` values, new optional sub-handle methods. Existing backend implementations continue to compile and run. |
| **PATCH** | Non-API — docs, internal refactor, tests, build tweaks. |

Note that the *library API* version is independent of any concrete
backend's *wire-format* version (e.g. the BMI library's file-format
prefix carries its own `wire_version` byte).

## Where the headers live

The public API is the four headers in this directory:

  - [`record.hpp`](record.hpp) — the `Record` value type +
    `parse_timestamp` helper.
  - [`record_backend.hpp`](record_backend.hpp) — `RecordBackend`,
    nested `Writer` / `Reader`, `BackendError`, `Durability`, plus
    the scoped `with_writer` / `with_reader` wrappers. Method-level
    docstrings live in the header; this README covers the rationale
    and the cross-cutting design.
  - [`id_predicates.hpp`](id_predicates.hpp) — `IdPredicate` type
    + stock helper kit + combinators.
  - [`version.hpp`](version.hpp) — library version constants and
    versioning-policy doc.

## Companion documents

  - [`../../../doc/NGEN_SERIALIZATION.md`](../../../doc/NGEN_SERIALIZATION.md)
    — how ngen drives this library through the BMI serialization
    protocol (configuration, engine integration points, scaling
    considerations). Audience: ngen operators and contributors.
  - [`../../../doc/BMI_SERIALIZATION_PROTOCOL.md`](../../../doc/BMI_SERIALIZATION_PROTOCOL.md)
    — the BMI model-author contract that produces the records this
    library stores. Audience: model authors writing BMI extensions
    for any engine.

---

## 1. What this library does and does not do

**Does**: define the engine-agnostic abstractions through which
checkpoint records are produced, persisted, and retrieved. The
library provides:

  - The `Record` value type — the unit of state interchange.
  - The `RecordBackend` interface — the storage abstraction.
  - `BackendError` — categorized error type for polymorphic
    error handling across backends.
  - `IdPredicate` and a kit of helpers — the application's seam
    for id-string interpretation.
  - `Durability` enum — the toggle that distinguishes
    fsync/commit-blocking from best-effort flush.
  - Scoped convenience wrappers `with_writer` / `with_reader`.

**Out of scope, always**:

  - **Wire formats.** Each concrete backend chooses how its bytes
    are laid out (file-format prefix, HDF5 dataset layout, S3
    object schema, etc.). The abstract surface is silent on the
    representation.
  - **Cross-process coordination.** The library cannot hide MPI
    from its callers; when multiple processes write to a shared
    target, the engine owns the synchronization. The library
    documents this boundary but does not try to abstract over it.

**Not in v0.1, planned for later releases**:

  - **Scheduling** — *when* checkpoints fire. A
    `CheckpointSchedule` SDK type (with `EveryN`, `Calendar`,
    `EndOfRun`, `Manual`, and composite implementations) is
    planned. Today, scheduling is the caller's responsibility.
  - **Snapshot composition** — how multiple subsystems' state
    (BMI modules, engine-level state like routing topology, etc.)
    aggregates into a coherent simulation-wide save. A
    `Snapshot` / `SnapshotCoordinator` layer is planned. Today,
    each producer writes its own records and the engine is on the
    hook to drive them.

---

## 2. The core types at a glance

```
                ┌─────────────────────┐
                │       Record        │   ← value type
                │  id, time_step,     │     (in record.hpp)
                │  simulation_ts,     │
                │  checkpoint_epoch,  │
                │  payload            │
                └─────────────────────┘
                          ▲
                          │ persists / retrieves
                          │
┌─────────────────────────┴─────────────────────────┐
│                  RecordBackend                    │
│                                                   │
│  writer(epoch, durability)  → unique_ptr<Writer>  │
│  reader(scope)              → unique_ptr<Reader>  │
│  finalize()                                       │
│  with_writer(epoch, dur, fn) — scoped convenience │
│  with_reader(scope, fn)      — scoped convenience │
│                                                   │
│  ┌───────────────┐   ┌────────────────────────┐   │
│  │  Writer       │   │  Reader                │   │
│  │  write(rec)   │   │  find_latest(id)       │   │
│  │  commit()     │   │  find_at_step(id, n)   │   │
│  │               │   │  find_at_sim_ts(id, t) │   │
│  │               │   │  close() [optional]    │   │
│  └───────────────┘   └────────────────────────┘   │
└───────────────────────────────────────────────────┘
```

All fallible operations return
`expected<T, BackendError>`. The library never throws on its own;
underlying stream / OS / model code may throw, and concrete
backends catch at the seam and convert to the error arm.

---

## 3. Rules every backend must satisfy

### 3.1 Identity contract

A record's identity for restore lookup is its `Record::id` string,
**exactly as the producer composed it**. Backends address records
by that string *verbatim* — they do not strip, normalize, or
augment it from out-of-band sources (rank, hostname, process id).

This transparency is what enables N-to-M rank restart: a producer
that wrote `"cat-1:cfe"` restores it at any rank count without the
backend silently entangling it with the writing rank's identity.

**Implication for backends**: never derive the lookup key from
anything other than the bytes the producer placed in `Record::id`.
Backends MAY consult rank/host for *file-level* aggregation
(per-rank file naming via path templates) but that information
never enters the record's storage key.

### 3.2 Error categorization

The `expected<>` error arm carries a `BackendError`:

```cpp
struct BackendError {
    enum class Kind { NotFound, Corrupted, IOError };
    Kind        kind;
    std::string message;   // free-form diagnostic
};
```

Each concrete backend translates its native error sources into this
small vocabulary. A polymorphic caller branches on `kind` without
parsing the message:

| `Kind` | When to use it | Typical caller response |
|---|---|---|
| `NotFound` | The query criteria don't match any record (no entry for id, or no entry for the given step/timestamp, or id is outside the open-time scope predicate). | Warn-and-continue; treat restore-for-this-id as a fresh start. |
| `Corrupted` | A record exists but its bytes don't parse: bad magic, unsupported wire_version, oversized payload, file truncated mid-record. | Abort. Operator action required. |
| `IOError` | Everything else: file open failed, permission denied, network blip, stream went bad mid-write, single-in-flight rule violation, backend not yet open. | Backend-specific — retry, surface to operator, fall back to a different backend. The `message` field carries the detail. |

**The extension rule**: add a new `Kind` value only when a real
caller wants to branch on it differently. Speculative additions are
costly because every concrete backend must update its translation.
Today's intentionally minimal set sweeps several distinguishable
situations into `IOError` (permission denied, transient network,
backend-not-open, etc.); split them only when a real consumer
appears that would act differently on each.

The full extension-rule docstring lives at `BackendError`'s
declaration in `record_backend.hpp`.

### 3.3 Sub-handle lifetime

A *sub-handle* is a `Reader` or `Writer` obtained from
`backend->reader(...)` / `backend->writer(...)`. Two guarantees
apply to **every** sub-handle, regardless of which concrete
backend produced it:

  1. **Move-only ownership.** The factory returns
     `std::unique_ptr<...>` inside the value arm. Lifetime is
     bound to that unique_ptr; destruction tears the sub-handle
     down.

  2. **Lifetime independence from the original backend handle.**
     A sub-handle remains valid for its entire lifetime even if
     the caller drops its backend reference between when the
     sub-handle was created and when it is destroyed. **Callers
     do not have to keep an explicit reference to the backend
     alive alongside their sub-handles.**

Concrete backends satisfy guarantee (2) one of two ways:

#### Pattern A: Snapshot

The sub-handle owns everything it needs at construction. The
backend object itself is not referenced after construction and may
be destroyed independently.

```cpp
// Inside the backend's factory:
auto reader(IdPredicate scope) override {
    return std::unique_ptr<Reader>(new MyReader{
        /* own ifstream */,
        /* shared_ptr<const Index> */,
        std::move(scope)
    });
}
```

The Reader holds no `*this` or `shared_ptr<MyBackend>` — its
captured resources are all it needs. The backend's `~MyBackend()`
may run while Readers are still alive; the Readers keep working
because they captured their state by value or via independent
shared_ptrs.

Fits backends where per-handle resources are cheap to acquire and
don't require cross-handle coordination. Typical for Reader-side
in a file-based backend (each Reader opens its own ifstream).

#### Pattern B: Shared-state

The sub-handle holds a `shared_ptr` to its parent backend,
typically obtained via `std::enable_shared_from_this`. The backend
stays alive while any sub-handle does.

```cpp
class MyBackend
  : public RecordBackend
  , public std::enable_shared_from_this<MyBackend>
{
    H5::H5File file_;          // opened ONCE
    std::shared_ptr<Index> index_;
    std::mutex op_mutex_;
    // ...
public:
    auto reader(IdPredicate scope) override {
        return std::unique_ptr<Reader>(
            new MyReader(shared_from_this(), std::move(scope)));
    }
};

class MyBackend::MyReader : public Reader {
    std::shared_ptr<MyBackend> backend_;  // pins backend alive
    IdPredicate scope_;
    // accesses backend_->file_, backend_->index_ under op_mutex_
};
```

**Required** for backends with expensive global state (open HDF5
files, S3 connection pools, TileDB contexts, remote sessions) and
**required for all Writers** regardless of backend, because the
single-in-flight rule and deferred-error accounting both need
backend coordination.

#### Why both patterns coexist

The choice is per-backend, per-sub-handle. A file-based backend's
Reader is naturally snapshot (cheap ifstream); its Writer is
naturally shared-state (single-in-flight tracking). An HDF5 backend
would use shared-state for both Reader and Writer (the open file
handle is shared).

The abstract guarantee is: *the sub-handle stays valid for its
lifetime*. How the backend ensures that is an implementation
detail.

### 3.4 Single-in-flight Writer rule

A backend MUST refuse to issue a second Writer while the previous
one is still alive on the same backend instance. The intent: prevent
intra-process same-epoch collisions by construction. The mechanism:
a flag (typically `bool writer_in_flight_`), set on `writer()`,
cleared in the Writer's destructor or on `commit()`.

The error returned when the rule is violated is a `BackendError`
with `Kind::IOError` and a message identifying the rule. (The rule
violation is genuinely an I/O-shaped failure from the caller's
perspective — the storage is unavailable for a new event.)

The single-in-flight rule is **the call site's responsibility**
unless the call site uses `with_writer`, which makes the rule a
syntactic property of the closure scope.

### 3.5 Reader scope is fixed at construction

`reader(IdPredicate scope)` constructs a Reader that will only
return records whose id satisfies `scope`. The predicate is **both
a hint and a contract**:

  - As a *hint*, backends may use the scope to pre-build a smaller
    index (memory bound), short-circuit network calls, etc.
  - As a *contract*, out-of-scope ids return `NotFound` from every
    `find_*` call against this Reader, even if the underlying
    storage contains them. Callers that want a different scope
    construct a fresh Reader.

The empty predicate (default-constructed `std::function`) means "no
scope" — the Reader is willing to find any record. This is the
common case for tooling and exploratory restores; production
restore typically supplies a hydrofabric-derived scope to bound
index memory.

### 3.6 Writer commit semantics

`Writer::commit()` makes accumulated writes durable, with the
durability level the caller supplied to `writer(epoch, durability)`:

  - `Durability::strict` — `commit()` blocks until bytes are
    durably stored (fsync, collective barrier, ack from a remote
    object store, etc.) and errors surface synchronously on the
    `expected<>` arm.
  - `Durability::relaxed` — best-effort flush. Errors may be
    deferred to the parent backend's `finalize()` call.

If a Writer is destroyed without `commit()` having been called, the
destructor performs best-effort cleanup. Errors during destruction
cannot be surfaced (destructors must not throw); they are logged
and propagated to the next `RecordBackend::finalize()` call.
**Callers who care about commit errors call `commit()` explicitly
and inspect its return** — or use `with_writer`, which calls
`commit()` automatically on the success path.

### 3.7 Optional `Reader::close()`

`Reader` exposes a `close()` method analogous to `Writer::commit()`
for backends with fallible read-side teardown (release of leased
remote resources, graceful disconnect from a streaming source,
final telemetry flush). The default implementation is a **no-op
success**; backends that don't need fallible teardown inherit it.
The destructor is the best-effort fallback if `close()` isn't
called explicitly.

`with_reader` calls `close()` automatically on the success path —
so callers using the recommended scoped pattern get teardown-error
surfacing for free, regardless of whether the backend overrides
`close()`.

---

## 4. Recommended call patterns

### 4.1 Scoped wrappers (the default)

Use `with_writer` and `with_reader`. They bind the sub-handle to a
closure scope, guarantee `commit()` / `close()` runs on success,
and handle destructor cleanup on exception:

```cpp
// Save side
backend->with_writer(epoch, durability, [&](Writer& w) {
    for (auto& snap : snapshots) {
        w.write(snap.to_record(ctx)).value();
    }
});  // commit() called automatically; error returned via the wrapper

// Restore side
backend->with_reader(primary_prefix("cat-1"), [&](Reader& r) {
    auto rec = r.find_latest("cat-1:bmi_c_cfe").value();
    apply(formulation, rec);
});  // close() called automatically; error returned via the wrapper
```

Both wrappers return `expected<void, BackendError>` so the caller
has a single error-handling branch covering factory failure,
in-closure errors, and commit/close failures.

### 4.2 Low-level factories (the escape hatch)

The lower-level `writer(...)` / `reader(...)` factories are still
public for cases where the sub-handle genuinely needs to outlive a
single function scope: async writes during a long event, multi-phase
restore drivers, gradual snapshotting across iterations.

```cpp
auto w = backend->writer(epoch, durability).value();
// ... use w across many call sites ...
w->commit().value();   // explicit commit; check the return
```

When using the low-level form, **the call site owns the
single-in-flight rule**. A second `writer(...)` call on the same
backend while `w` is still alive returns an error. Calling
`commit()` (or letting `w` go out of scope) clears the flag.

### 4.3 Error category branching

The whole point of `BackendError::Kind` is that callers branch
without parsing the `message`:

```cpp
auto rec = reader->find_latest("cat-1:cfe");
if (!rec) {
    switch (rec.error().kind) {
        case BackendError::Kind::NotFound:
            // Common — no checkpoint for this feature.
            log_warn("no checkpoint for cat-1:cfe; starting fresh");
            return;
        case BackendError::Kind::Corrupted:
            // Operator action required.
            log_error("checkpoint corrupted: ", rec.error().message);
            throw RestoreFatal{rec.error().message};
        case BackendError::Kind::IOError:
            // Retry with backoff, fall back, or fail loudly.
            log_error("checkpoint I/O failure: ", rec.error().message);
            return;
    }
}
apply(formulation, rec.value());
```

### 4.4 Id predicates

The `IdPredicate` type is `std::function<bool(const std::string&)>`.
The library invokes it during the index walk and treats the
returned `bool` as authoritative. The library **never parses,
splits, or interprets the id itself** — id-string semantics are
exclusively the application's business.

Stock helpers cover the common composition schemes:

| Helper | Matches |
|---|---|
| `exact_id(s)` | `id == s` |
| `primary_prefix(s)` | First `:`/`.`-delimited segment equals `s` |
| `contains_segment(s)` | `s` appears as any `:`/`.`-delimited segment |
| `at_position(n, s)` | nth segment equals `s` |
| `starts_with(s)` | Substring prefix |
| `all_of({...})` | All sub-predicates true (short-circuiting) |
| `any_of({...})` | Any sub-predicate true (short-circuiting) |
| `not_fn(p)` | `!p(id)` |

Custom application closures are first-class. Anything that meets
the closure requirements — pure-ish, cheap-ish, stable for the
Reader's lifetime — is fair game:

```cpp
// Regex against composed ids:
std::regex pat(R"(^rank-\d+:cat-\d+:)");
auto reader = backend->reader(
    [&](const std::string& id) { return std::regex_search(id, pat); });

// Closure over engine state:
auto reader = backend->reader(
    [&new_partition](const std::string& id) {
        auto feat = id.substr(0, id.find(':'));
        return new_partition.owns(feat);
    });
```

---

## 5. Writing a new `RecordBackend`

Checklist for adding a concrete backend (e.g., `HDF5Backend`,
`S3Backend`):

1. **Pick a sub-handle lifetime pattern.** Snapshot if per-handle
   resources are cheap and the backend has no global state worth
   sharing; shared-state otherwise. Writers always use
   shared-state.

2. **Decide which `BackendError::Kind` your native errors map to.**
   Write a small translation function inside the backend's
   translation unit. Don't ship a backend that uses `IOError` for
   everything if you can meaningfully distinguish `NotFound` /
   `Corrupted` — categorization is what backends owe their callers.

3. **Implement the abstract methods.** Both `writer()` and
   `reader()` factories return `expected<unique_ptr<...>,
   BackendError>`; both can fail (file open / network setup /
   single-in-flight violation / etc.).

4. **Implement the Writer's `commit()` and the Reader's
   `close()`.** `commit()` is fallible and MUST surface errors on
   the `expected<>` arm. `close()` may inherit the default no-op
   success if the Reader has no fallible teardown.

5. **Enforce single-in-flight inside `writer()`.** Use a `bool`
   member protected by whatever concurrency mechanism your backend
   uses (mutex for thread-safe backends; nothing for
   single-threaded). Return `Kind::IOError` with a clear message
   when the rule is violated.

6. **Document your concurrency contract.** The abstract interface
   is concurrency-agnostic. Tell consumers whether your backend is
   single-threaded, thread-safe-within-a-single-instance,
   process-safe across multiple instances, etc.

7. **Drop your backend into the test harness in
   `test/utils/serialization/`.** The existing mock-based tests
   exercise the abstract surface. Adding your backend to that
   suite either passes (you satisfy the rules) or fails (you
   don't). New backend-specific tests cover the backend's own
   surface area (e.g. HDF5 file layout assertions) on top of the
   shared interface tests.
