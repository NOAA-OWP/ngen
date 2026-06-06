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
*/
#include "gtest/gtest.h"

#include "mocks.hpp"
#include "utilities/serialization/id_predicates.hpp"
#include "utilities/serialization/record_backend.hpp"

#include <memory>

using namespace ngen::serialization;
using ngen::serialization::test::SharedStateBackend;
using ngen::serialization::test::SnapshotBackend;
using Reader = RecordBackend::Reader;
using Writer = RecordBackend::Writer;

// ---------------------------------------------------------------------
// Snapshot pattern: sub-handle outlives the backend
// ---------------------------------------------------------------------

TEST(SubHandleLifetime, snapshot_reader_remains_valid_after_backend_destruction) {
    std::unique_ptr<Reader> reader;
    {
        SnapshotBackend be;
        be.tag = 42;
        auto r = be.reader(IdPredicate{});
        ASSERT_TRUE(r.has_value());
        reader = std::move(r.value());
        // backend goes out of scope here — destroyed
    }

    // Reader still works; the tag was captured by value at construction.
    auto rec = reader->find_latest("x");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec.value().id, "snapshot-tag-42");
}

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

TEST(SubHandleLifetime, shared_state_reader_keeps_backend_alive_after_local_drops_it) {
    std::weak_ptr<SharedStateBackend> weak;
    std::unique_ptr<Reader> reader;
    {
        auto be             = std::make_shared<SharedStateBackend>();
        be->shared_resource = "alive-after-local-drop";
        weak                = be;
        reader              = std::move(be->reader(IdPredicate{}).value());
        // `be` goes out of scope here; the Reader's shared_ptr is the
        // only ref left.
    }
    // Backend must still be alive (reader holds the only ref):
    EXPECT_FALSE(weak.expired()) << "backend died despite a live Reader holding shared_ptr";

    // Reader can still read through the shared backend:
    auto rec = reader->find_latest("x");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec.value().id, "alive-after-local-drop");

    // Drop the reader; backend must finally die.
    reader.reset();
    EXPECT_TRUE(weak.expired()) << "backend leaked — no live sub-handles, no local ref";
}

TEST(SubHandleLifetime, shared_state_writer_also_keeps_backend_alive) {
    // Same as the reader test, but with a Writer — Writers MUST always
    // hold a shared_ptr (single-in-flight + deferred-error coordination
    // require backend access through the Writer's lifetime).
    std::weak_ptr<SharedStateBackend> weak;
    std::unique_ptr<Writer> writer;
    {
        auto be = std::make_shared<SharedStateBackend>();
        weak    = be;
        writer =
            std::move(be->writer(std::chrono::system_clock::now(), Durability::relaxed).value());
    }
    EXPECT_FALSE(weak.expired());
    EXPECT_TRUE(writer->write(Record{}).has_value());
    EXPECT_TRUE(writer->commit().has_value());
    writer.reset();
    EXPECT_TRUE(weak.expired());
}

TEST(SubHandleLifetime, shared_state_multiple_subhandles_share_backend_lifetime) {
    std::weak_ptr<SharedStateBackend> weak;
    std::unique_ptr<Reader> r1, r2;
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

    // Drop one — backend still alive via the other.
    r1.reset();
    EXPECT_FALSE(weak.expired());

    // Drop the last one — backend dies.
    r2.reset();
    EXPECT_TRUE(weak.expired());
}
