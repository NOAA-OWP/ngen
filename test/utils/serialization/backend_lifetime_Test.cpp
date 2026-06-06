/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
------------------------------------------------------------------------
Tests for the sub-handle lifetime contract documented in
`record_backend.hpp`. Exercises both satisfying patterns:

  - Snapshot — sub-handle is self-contained; backend may be
    destroyed independently while the sub-handle remains valid.

  - Shared-state — sub-handle holds `shared_ptr<Backend>`; backend
    stays alive while any sub-handle does, even after the caller
    has dropped its own backend reference.

Both patterns satisfy the same abstract guarantee: a sub-handle
remains valid for its entire lifetime regardless of whether the
original holder of the backend has released its reference.

Test structure
--------------
The pure-contract tests are typed-parameterized over a list of
backend traits, so each contract is exercised against every
backend that claims to satisfy that pattern. The trait pattern
gives the typed fixture a uniform `create()` factory regardless
of the concrete backend's construction surface:

  SnapshotReaderBackends — backends whose Reader is snapshot-
    pattern (Reader outlives Backend without keeping it alive).
    Currently: SnapshotBackend, FileBackend.

  SharedStateWriterBackends — backends whose Writer holds
    `shared_ptr<Backend>` and keeps the parent alive while in
    flight. Currently: SharedStateBackend, FileBackend.

Tests that depend on a specific mock's machinery (`SnapshotBackend`'s
tagged Reader, `SharedStateBackend`'s `live_subhandles` counter,
etc.) stay as plain `TEST()` with that mock — they exercise the
mock, not the abstract contract.

Adding a new backend to the parameterized suites
------------------------------------------------
When a new concrete `RecordBackend` implementation lands, drop it
into the existing type lists with three steps:

  1. **Write a Traits struct** in the anonymous namespace below.
     It needs:
       - `using Backend = YourBackend;`  (the concrete type, not the
         abstract `RecordBackend` — the typed tests need
         `weak_ptr<Backend>` for lifetime assertions, which requires
         a concrete pointee).
       - `static std::shared_ptr<Backend> create();` returning a
         ready-to-use instance. Construction details (paths,
         resources, registry side-effects) are the traits' problem;
         the test fixture just calls `create()`.

     See `FileBackendTraits` for a non-trivial example (path
     allocation + sanitization for Google Test's typed-suite name
     format).

  2. **Identify which contracts the backend satisfies.** Match each
     of its sub-handles against the pattern definitions above:
       - Reader is snapshot-pattern (Reader outlives Backend
         without keeping it alive) → add to
         `SnapshotReaderBackends`.
       - Writer holds `shared_ptr<Backend>` and keeps the parent
         alive → add to `SharedStateWriterBackends`.

     A backend may satisfy multiple patterns (`FileBackend` does:
     its Reader is snapshot, its Writer is shared-state) and will
     appear in multiple type lists.

  3. **Add the traits to the relevant `::testing::Types<...>` type
     lists.** Each insertion automatically replays every typed
     test in that suite against the new backend. No new test
     bodies needed.

That's it for the lifetime contracts. If the new backend has
behavioral quirks that aren't covered by the abstract patterns
(specific failure modes, performance characteristics, particular
internal state), put a separate `TEST(YourBackend, ...)` for that
in the backend's own test file — don't pollute the parameterized
suites with mock- or backend-specific assertions.

ROI for parameterizing other test files
---------------------------------------
This file is the highest-ROI candidate for parameterization: the
lifetime contracts are nearly pattern-agnostic, so a single
fixture body covers every backend uniformly. The other abstract
test files (`backend_writer_Test.cpp`, `backend_reader_Test.cpp`)
are heavier on mock-specific assertions (call-count counters,
per-method failure switches, etc.) — extracting their
pure-contract subset would require more refactoring per test and
yields less coverage win per backend. Revisit when a second
non-mock backend (e.g. an HDF5 backend) arrives and there's a
clearer cost/benefit picture.
*/
#include "gtest/gtest.h"

#include "mocks.hpp"
#include "utilities/bmi/file_backend.hpp"
#include "utilities/serialization/id_predicates.hpp"
#include "utilities/serialization/record_backend.hpp"

#include <chrono>
#include <cstdio>
#include <memory>
#include <string>

using namespace ngen::serialization;
using ngen::serialization::test::SharedStateBackend;
using ngen::serialization::test::SnapshotBackend;
using Reader = RecordBackend::Reader;
using Writer = RecordBackend::Writer;

// ---------------------------------------------------------------------
// Backend traits — each one provides a uniform `create()` factory so
// the typed-test fixtures don't have to know how individual backends
// are constructed.
// ---------------------------------------------------------------------
namespace {

// Construct a unique-per-test temp file path, namespaced by @p
// prefix and the current test's identity. Disk-backed backend
// traits (FileBackend today, future HDF5 / SQLite / ... backends)
// can use this to avoid colliding on shared paths between tests
// and to play nicely with the path-keyed registries some backends
// keep.
//
// Implementation notes:
//   - Sanitizes the suite/test names because Google Test embeds
//     '/' in typed-test suite names (e.g. "Pattern/1" for the
//     second type-list entry), which would create a missing
//     subdirectory inside the temp dir.
//   - Removes any pre-existing file at the resolved path so the
//     caller gets a clean slate without having to remember to.
inline std::string unique_temp_path(const std::string& prefix) {
    auto sanitize = [](const std::string& s) {
        std::string out = s;
        for (auto& c : out)
            if (c == '/') c = '_';
        return out;
    };
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string path = ::testing::TempDir();
    if (!path.empty() && path.back() != '/') path.push_back('/');
    path += prefix;
    path += "_";
    path += sanitize(info->test_suite_name());
    path += "_";
    path += sanitize(info->name());
    path += ".bin";
    std::remove(path.c_str());
    return path;
}

struct SnapshotBackendTraits {
    using Backend = SnapshotBackend;
    static std::shared_ptr<Backend> create() {
        return std::make_shared<SnapshotBackend>();
    }
};

struct SharedStateBackendTraits {
    using Backend = SharedStateBackend;
    static std::shared_ptr<Backend> create() {
        return std::make_shared<SharedStateBackend>();
    }
};

struct FileBackendTraits {
    using Backend = FileBackend;
    static std::shared_ptr<Backend> create() {
        auto be = FileBackend::create(unique_temp_path("ngen_filebackend_lifetime"));
        EXPECT_TRUE(be.has_value()) << be.error().message;
        return be.value();
    }
};

} // namespace

// ---------------------------------------------------------------------
// Snapshot pattern: sub-handle outlives the backend
// ---------------------------------------------------------------------

template <typename TraitsT>
class SnapshotReaderPattern : public ::testing::Test {};

using SnapshotReaderBackends = ::testing::Types<SnapshotBackendTraits, FileBackendTraits>;
TYPED_TEST_SUITE(SnapshotReaderPattern, SnapshotReaderBackends);

TYPED_TEST(SnapshotReaderPattern, reader_remains_callable_after_backend_destruction) {
    // Pure contract: a Reader handed out by the backend keeps working
    // after every shared_ptr<Backend> reference (including the local
    // one in this scope) has been dropped. We don't assert on the
    // record content — that's per-backend semantics — only on the
    // structural property that `find_latest` returns an `expected<>`
    // and does not crash.
    std::unique_ptr<Reader> reader;
    {
        auto be = TypeParam::create();
        auto r  = be->reader(IdPredicate{});
        ASSERT_TRUE(r.has_value()) << r.error().message;
        reader = std::move(r.value());
        // `be` goes out of scope. For backends in this type list,
        // the Reader continues to be valid.
    }

    // Call must not crash; the value/error outcome is per-backend.
    auto rec = reader->find_latest("any-id");
    (void)rec;
}

// Mock-specific: SnapshotBackend captures its tag by value at
// Reader construction, so two Readers from differently-tagged
// backends don't alias. This is a property of the mock's
// implementation, not the abstract contract, so it stays as a
// plain TEST() instead of joining the parameterized suite above.
TEST(SubHandleLifetime, snapshot_readers_with_different_tags_dont_alias) {
    std::unique_ptr<Reader> reader_a;
    std::unique_ptr<Reader> reader_b;
    {
        SnapshotBackend be_a;
        be_a.tag = 1;
        SnapshotBackend be_b;
        be_b.tag = 2;
        reader_a = std::move(be_a.reader(IdPredicate{}).value());
        reader_b = std::move(be_b.reader(IdPredicate{}).value());
    } // both backends destroyed

    EXPECT_EQ(reader_a->find_latest("x").value().id, "snapshot-tag-1");
    EXPECT_EQ(reader_b->find_latest("x").value().id, "snapshot-tag-2");
}

// ---------------------------------------------------------------------
// Shared-state pattern: sub-handle keeps the backend alive
// ---------------------------------------------------------------------

template <typename TraitsT>
class SharedStateWriterPattern : public ::testing::Test {};

using SharedStateWriterBackends = ::testing::Types<SharedStateBackendTraits, FileBackendTraits>;
TYPED_TEST_SUITE(SharedStateWriterPattern, SharedStateWriterBackends);

TYPED_TEST(SharedStateWriterPattern, writer_keeps_backend_alive_until_dropped) {
    // Pure contract: a Writer holds `shared_ptr<Backend>` so the
    // parent backend stays alive while the Writer does, even after
    // every other shared_ptr<Backend> reference has dropped.
    std::weak_ptr<typename TypeParam::Backend> weak;
    std::unique_ptr<Writer>                    writer;
    {
        auto be = TypeParam::create();
        weak    = be;
        auto w  = be->writer(std::chrono::system_clock::now(), Durability::relaxed);
        ASSERT_TRUE(w.has_value()) << w.error().message;
        writer = std::move(w.value());
        // `be` and `w` drop here; only `writer`'s internal
        // shared_ptr<Backend> keeps the backend alive.
    }
    EXPECT_FALSE(weak.expired()) << "backend died despite a live Writer holding shared_ptr";

    // Writer still functions through its retained shared_ptr.
    EXPECT_TRUE(writer->write(Record{}).has_value());
    EXPECT_TRUE(writer->commit().has_value());

    // Drop the writer; backend must finally die.
    writer.reset();
    EXPECT_TRUE(weak.expired()) << "backend leaked — no live sub-handles, no local ref";
}

// SharedStateBackend's Reader is also shared-state-pattern (holds
// `shared_ptr<Backend>`); FileBackend's Reader is NOT (it holds
// `shared_ptr<const Index>` instead, which is the snapshot pattern
// for the read side). So the "Reader keeps Backend alive" contract
// only applies to backends that explicitly opt their Reader into
// the shared-state pattern. That's currently just SharedStateBackend
// — not enough variety yet to justify a typed-fixture; if a future
// backend joins, promote this to a parameterized suite.
TEST(SubHandleLifetime, shared_state_reader_keeps_backend_alive_after_local_drops_it) {
    std::weak_ptr<SharedStateBackend> weak;
    std::unique_ptr<Reader>           reader;
    {
        auto be             = std::make_shared<SharedStateBackend>();
        be->shared_resource = "alive-after-local-drop";
        weak                = be;
        reader              = std::move(be->reader(IdPredicate{}).value());
        // `be` goes out of scope; the Reader's shared_ptr is the
        // only ref left.
    }
    EXPECT_FALSE(weak.expired()) << "backend died despite a live Reader holding shared_ptr";

    auto rec = reader->find_latest("x");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec.value().id, "alive-after-local-drop");

    reader.reset();
    EXPECT_TRUE(weak.expired()) << "backend leaked — no live sub-handles, no local ref";
}

// Mock-specific: SharedStateBackend exposes a `live_subhandles`
// counter so the test can assert two Readers share lifetime
// management. The counter itself is mock machinery, not part of
// the abstract contract.
TEST(SubHandleLifetime, shared_state_multiple_subhandles_share_backend_lifetime) {
    std::weak_ptr<SharedStateBackend> weak;
    std::unique_ptr<Reader>           r1, r2;
    {
        auto be = std::make_shared<SharedStateBackend>();
        weak    = be;
        r1      = std::move(be->reader(primary_prefix("cat-1")).value());
        r2      = std::move(be->reader(primary_prefix("cat-2")).value());
        EXPECT_EQ(be->live_subhandles, 2);
    }
    EXPECT_FALSE(weak.expired());

    // Both Readers see the same shared backend state.
    EXPECT_EQ(r1->find_latest("x").value().id, "shared");
    EXPECT_EQ(r2->find_latest("x").value().id, "shared");

    r1.reset();
    EXPECT_FALSE(weak.expired());

    r2.reset();
    EXPECT_TRUE(weak.expired());
}
