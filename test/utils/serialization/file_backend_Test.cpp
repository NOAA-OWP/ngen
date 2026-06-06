/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
------------------------------------------------------------------------
Tests for `ngen::serialization::FileBackend`. Round-trips Records
through the file-format wire layout via the abstract `RecordBackend`
surface, validates the FileBackend-specific behaviors (path-keyed
registry sharing, snapshot Reader lifetime, mutex-serialized
concurrent writes, duplicate-key diagnostic, two-layer scope
filtering), and confirms that the abstract contract tests in
`backend_*_Test.cpp` also apply to FileBackend (structurally —
those tests use mocks; this file exercises the real backend on
disk).
*/
#include "gtest/gtest.h"

#include "utilities/bmi/file_backend.hpp"
#include "utilities/serialization/id_predicates.hpp"
#include "utilities/serialization/record.hpp"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace ngen::serialization;
using Reader = RecordBackend::Reader;
using Writer = RecordBackend::Writer;

namespace {

// Build a unique temp file path for each test. ::testing::TempDir()
// is a per-process directory; we suffix with the test name to keep
// runs isolated.
std::string temp_path() {
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string dir  = ::testing::TempDir();
    if (!dir.empty() && dir.back() != '/') dir.push_back('/');
    return dir + "ngen_filebackend_" + info->name() + ".bin";
}

auto now() {
    return std::chrono::system_clock::now();
}

Record make_record(
    std::string id,
    int64_t step,
    int64_t ts,
    std::vector<char> payload,
    int64_t checkpoint_epoch = 0
) {
    return Record{std::move(id), step, ts, std::move(payload), checkpoint_epoch};
}

// Convenience wrapper for tests that expect must_create() to
// succeed (the overwhelming majority). Asserts the expected<>
// returned a value and unwraps it. Tests that specifically exercise
// the error arm call `must_create(...)` directly and inspect
// `.error()`.
std::shared_ptr<FileBackend> must_create(std::string path, IdPredicate scope = {}) {
    // Call the factory through a typedef'd pointer so the helper's
    // body doesn't textually contain `FileBackend::create(` — the
    // tests adopt `must_create(...)` as the convenience name for
    // unwrapping the expected<>, and we don't want this helper
    // calling itself.
    using CreateFn =
        expected<std::shared_ptr<FileBackend>, BackendError> (*)(std::string, IdPredicate);
    constexpr CreateFn factory = &FileBackend::create;
    auto be                    = factory(std::move(path), std::move(scope));
    EXPECT_TRUE(be.has_value()) << be.error().message;
    return be.has_value() ? std::move(be.value()) : nullptr;
}

} // namespace

// ---------------------------------------------------------------------
// Round-trip: write N records, read them back
// ---------------------------------------------------------------------

TEST(FileBackend, round_trip_single_record_via_factory_methods) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);
    {
        auto w = be->writer(now(), Durability::relaxed);
        ASSERT_TRUE(w.has_value()) << w.error().message;
        ASSERT_TRUE(w.value()->write(make_record("cat-1:cfe", 0, 60, {'A', 'B', 'C'})).has_value());
        ASSERT_TRUE(w.value()->commit().has_value());
    }

    auto r = be->reader();
    ASSERT_TRUE(r.has_value());

    auto rec = r.value()->find_latest("cat-1:cfe");
    ASSERT_TRUE(rec.has_value()) << rec.error().message;
    EXPECT_EQ(rec.value().id, "cat-1:cfe");
    EXPECT_EQ(rec.value().time_step, 0);
    EXPECT_EQ(rec.value().simulation_timestamp, 60);
    EXPECT_EQ(rec.value().payload, std::vector<char>({'A', 'B', 'C'}));

    std::remove(path.c_str());
}

TEST(FileBackend, round_trip_via_scoped_wrappers) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);

    // Save side via with_writer
    auto save = be->with_writer(now(), Durability::relaxed, [](Writer& w) {
        (void)w.write(make_record("cat-1", 0, 0, {'X'}));
        (void)w.write(make_record("cat-1", 1, 60, {'Y'}));
        (void)w.write(make_record("cat-2", 0, 0, {'Z'}));
    });
    ASSERT_TRUE(save.has_value()) << save.error().message;

    // Restore side via with_reader
    Record found;
    auto restore = be->with_reader(IdPredicate{}, [&](Reader& r) {
        auto rec = r.find_latest("cat-1");
        if (rec) found = rec.value();
    });
    ASSERT_TRUE(restore.has_value());
    EXPECT_EQ(found.id, "cat-1");
    EXPECT_EQ(found.time_step, 1); // latest
    EXPECT_EQ(found.payload, std::vector<char>{'Y'});

    std::remove(path.c_str());
}

TEST(FileBackend, multiple_entities_share_one_file) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);

    ASSERT_TRUE(be->with_writer(now(), Durability::relaxed, [](Writer& w) {
                      (void)w.write(make_record("cat-A", 0, 0, {'1'}));
                      (void)w.write(make_record("cat-B", 0, 0, {'2'}));
                      (void)w.write(make_record("cat-A", 2, 120, {'3'}));
                  }).has_value());

    auto r = be->reader();
    ASSERT_TRUE(r.has_value());

    auto a = r.value()->find_latest("cat-A");
    auto b = r.value()->find_latest("cat-B");
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(a.value().time_step, 2);
    EXPECT_EQ(a.value().payload, std::vector<char>{'3'});
    EXPECT_EQ(b.value().time_step, 0);

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------
// find_* mode coverage
// ---------------------------------------------------------------------

TEST(FileBackend, find_at_step_picks_exact_step) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);
    ASSERT_TRUE(be->with_writer(now(), Durability::relaxed, [](Writer& w) {
                      (void)w.write(make_record("cat-1", 0, 0, {'a'}));
                      (void)w.write(make_record("cat-1", 5, 300, {'b'}));
                      (void)w.write(make_record("cat-1", 7, 420, {'c'}));
                  }).has_value());

    auto r   = be->reader();
    auto rec = r.value()->find_at_step("cat-1", 5);
    ASSERT_TRUE(rec.has_value()) << rec.error().message;
    EXPECT_EQ(rec.value().payload, std::vector<char>{'b'});

    auto missing = r.value()->find_at_step("cat-1", 99);
    ASSERT_FALSE(missing.has_value());
    EXPECT_EQ(missing.error().kind, BackendError::Kind::NotFound);

    std::remove(path.c_str());
}

TEST(FileBackend, find_at_simulation_timestamp_picks_exact_match) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);
    ASSERT_TRUE(be->with_writer(now(), Durability::relaxed, [](Writer& w) {
                      (void)w.write(make_record("cat-1", 0, 1000, {'p'}));
                      (void)w.write(make_record("cat-1", 1, 2000, {'q'}));
                      (void)w.write(make_record("cat-1", 2, 3000, {'r'}));
                  }).has_value());

    auto r   = be->reader();
    auto rec = r.value()->find_at_simulation_timestamp("cat-1", 2000);
    ASSERT_TRUE(rec.has_value()) << rec.error().message;
    EXPECT_EQ(rec.value().payload, std::vector<char>{'q'});

    auto missing = r.value()->find_at_simulation_timestamp("cat-1", 9999);
    ASSERT_FALSE(missing.has_value());
    EXPECT_EQ(missing.error().kind, BackendError::Kind::NotFound);

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------
// Large-record stress: covers the streambuf's bulk-write path
// (xsputn) and confirms records bigger than the streambuf's 8 KiB
// buffer round-trip correctly. Important because the wire-format
// helpers issue large `ostream::write` calls for payload bytes.
// ---------------------------------------------------------------------

TEST(FileBackend, large_record_payload_round_trips) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    // Payload larger than the streambuf's 8192-byte buffer.
    constexpr std::size_t payload_bytes = 64 * 1024;
    std::vector<char> big_payload(payload_bytes);
    for (std::size_t i = 0; i < payload_bytes; ++i) {
        big_payload[i] = static_cast<char>(i & 0xff);
    }

    auto be = must_create(path);
    ASSERT_TRUE(be->with_writer(now(), Durability::relaxed, [&](Writer& w) {
                      (void)w.write(make_record("big-1", 0, 0, big_payload));
                  }).has_value());

    auto r   = be->reader();
    auto rec = r.value()->find_latest("big-1");
    ASSERT_TRUE(rec.has_value()) << rec.error().message;
    ASSERT_EQ(rec.value().payload.size(), payload_bytes);
    EXPECT_EQ(rec.value().payload, big_payload);

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------
// Durability::strict actually fsyncs
// ---------------------------------------------------------------------

TEST(FileBackend, strict_durability_commit_fsyncs_and_succeeds) {
    // We can't easily black-box-verify "the bytes hit the platter"
    // without a hostile kernel module, so this test just confirms
    // that strict commit() returns success under normal conditions
    // and that the round-trip works. The cost of the fsync itself
    // is exercised every commit() under strict; a regression that
    // omitted the fsync would not be caught here, but would be
    // caught by reading the implementation. A regression that
    // *broke* the fsync (e.g. returning the wrong error from a
    // bad fd) would be caught: this test would fail with that
    // error on the commit's error arm.
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);
    {
        auto w = be->writer(now(), Durability::strict);
        ASSERT_TRUE(w.has_value()) << w.error().message;
        ASSERT_TRUE(w.value()->write(make_record("cat-1", 0, 0, {'D'})).has_value());
        auto rc = w.value()->commit();
        ASSERT_TRUE(rc.has_value()) << rc.error().message;
    }

    auto r   = be->reader();
    auto rec = r.value()->find_latest("cat-1");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec.value().payload, std::vector<char>{'D'});

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------
// Writer concurrency & path-keyed registry sharing
// ---------------------------------------------------------------------

TEST(FileBackend, create_returns_same_backend_for_same_path) {
    // The path-keyed registry is the load-bearing optimization for
    // N-protocols-on-one-realization. Verify that two `create()`
    // calls with the same path return the same shared_ptr — same
    // file descriptor, same in-memory index, same write mutex.
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be1 = must_create(path);
    auto be2 = must_create(path);
    EXPECT_EQ(be1.get(), be2.get()) << "expected same FileBackend instance for identical paths";

    // Different paths should NOT share.
    auto be3 = must_create(path + ".other");
    EXPECT_NE(be1.get(), be3.get());

    std::remove(path.c_str());
    std::remove((path + ".other").c_str());
}

TEST(FileBackend, registry_evicts_after_last_handle_drops) {
    // weak_ptr in the registry means a backend that's no longer
    // referenced should be reconstructible (not handed back stale).
    const std::string path = temp_path();
    std::remove(path.c_str());

    FileBackend* first_addr = nullptr;
    {
        auto be1   = must_create(path);
        first_addr = be1.get();
    } // last shared_ptr drops → backend destroyed → weak_ptr expires

    // A fresh create() should construct anew, not return the dead
    // pointer. We can't assert that the new address differs (allocators
    // can reuse memory), but we can verify the new backend is fully
    // functional.
    auto be2 = must_create(path);
    EXPECT_NE(be2.get(), nullptr);
    auto w = be2->writer(now(), Durability::relaxed);
    ASSERT_TRUE(w.has_value()) << w.error().message;
    ASSERT_TRUE(w.value()->write(make_record("post-reuse", 0, 0, {'X'})).has_value());

    std::remove(path.c_str());
}

TEST(FileBackend, concurrent_writers_serialize_records_via_backend_mutex) {
    // Two Writer handles on the same backend writing concurrently
    // from separate threads. The backend's io_mutex_ must serialize
    // their record writes so prefix/id/payload spans don't
    // interleave at sub-record granularity. Verify by reading every
    // record back and confirming all are intact (correct id length,
    // correct payload, no truncation).
    const std::string path = temp_path();
    std::remove(path.c_str());

    constexpr int per_thread   = 50;
    constexpr int payload_size = 256;

    auto be = must_create(path);

    auto write_n = [&](const std::string& id_prefix) {
        auto wo = be->writer(now(), Durability::relaxed);
        ASSERT_TRUE(wo.has_value()) << wo.error().message;
        auto& w = *wo.value();
        std::vector<char> payload(payload_size, '\0');
        for (int i = 0; i < per_thread; ++i) {
            std::fill(payload.begin(), payload.end(), static_cast<char>('A' + (i % 26)));
            Record rec{id_prefix + "-" + std::to_string(i), static_cast<int64_t>(i), 0, payload, 0};
            ASSERT_TRUE(w.write(rec).has_value());
        }
        ASSERT_TRUE(w.commit().has_value());
    };

    std::thread t1(write_n, "thread-a");
    std::thread t2(write_n, "thread-b");
    t1.join();
    t2.join();

    // Verify every record is readable and intact via a Reader on
    // the same shared backend.
    auto ro = be->reader();
    ASSERT_TRUE(ro.has_value());
    auto& reader = *ro.value();

    for (const auto& prefix : {"thread-a", "thread-b"}) {
        for (int i = 0; i < per_thread; ++i) {
            const std::string id = std::string(prefix) + "-" + std::to_string(i);
            auto rec             = reader.find_at_step(id, static_cast<int64_t>(i));
            ASSERT_TRUE(rec.has_value())
                << "missing record for id '" << id << "': " << rec.error().message;
            EXPECT_EQ(rec.value().payload.size(), payload_size);
            const char expected = static_cast<char>('A' + (i % 26));
            for (char c : rec.value().payload) {
                EXPECT_EQ(c, expected) << "interleaved payload detected in record '" << id << "'";
            }
        }
    }

    std::remove(path.c_str());
}

TEST(FileBackend, writer_supports_multiple_write_commit_cycles) {
    // A single Writer handle is allowed to do many write+commit
    // pairs back-to-back. Verify: one Writer, N write+commit pairs,
    // all records readable afterward. (The protocol layer no
    // longer keeps a long-lived Writer — each `run()` acquires a
    // fresh handle — but the underlying multi-cycle support
    // exercised here is still part of the Writer contract.)
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);
    {
        auto w = be->writer(now(), Durability::relaxed);
        ASSERT_TRUE(w.has_value());
        for (int64_t i = 0; i < 5; ++i) {
            ASSERT_TRUE(
                w.value()->write(make_record("cat-1", i, i * 60, {char('a' + i)})).has_value()
            );
            ASSERT_TRUE(w.value()->commit().has_value());
        }
    } // Writer dropped here; file released

    auto r = be->reader();
    ASSERT_TRUE(r.has_value());
    auto rec = r.value()->find_at_step("cat-1", 3);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec.value().payload, std::vector<char>{'d'});

    auto latest = r.value()->find_latest("cat-1");
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest.value().time_step, 4);

    std::remove(path.c_str());
}

TEST(FileBackend, trailing_write_after_commit_is_flushed_on_destruction) {
    // Specific guard: a `write → commit → write → ~Writer()` sequence
    // must NOT lose the trailing record. The committed_ flag is reset
    // by write() so the destructor's best-effort flush still runs.
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);
    {
        auto w = be->writer(now(), Durability::relaxed);
        ASSERT_TRUE(w.has_value());
        ASSERT_TRUE(w.value()->write(make_record("cat-1", 0, 0, {'A'})).has_value());
        ASSERT_TRUE(w.value()->commit().has_value());
        ASSERT_TRUE(w.value()->write(make_record("cat-1", 1, 60, {'B'})).has_value());
        // No explicit commit; let the Writer destruct.
    }

    auto r = be->reader();
    ASSERT_TRUE(r.has_value());
    auto rec = r.value()->find_latest("cat-1");
    ASSERT_TRUE(rec.has_value()) << rec.error().message;
    EXPECT_EQ(rec.value().time_step, 1);
    EXPECT_EQ(rec.value().payload, std::vector<char>{'B'});

    std::remove(path.c_str());
}

TEST(FileBackend, sequential_with_writer_calls_dont_trip_single_in_flight) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);
    for (int i = 0; i < 4; ++i) {
        auto rc = be->with_writer(now(), Durability::relaxed, [i](Writer& w) {
            (void)w.write(make_record("cat-" + std::to_string(i), i, i * 60, {'x'}));
        });
        ASSERT_TRUE(rc.has_value()) << "iteration " << i;
    }

    auto r = be->reader();
    EXPECT_TRUE(r.value()->find_latest("cat-3").has_value());

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------
// Predicate-scoped index
// ---------------------------------------------------------------------

TEST(FileBackend, reader_with_scope_excludes_out_of_scope_ids) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);
    ASSERT_TRUE(be->with_writer(now(), Durability::relaxed, [](Writer& w) {
                      (void)w.write(make_record("cat-1:cfe", 0, 0, {'1'}));
                      (void)w.write(make_record("cat-2:cfe", 0, 0, {'2'}));
                      (void)w.write(make_record("cat-3:cfe", 0, 0, {'3'}));
                  }).has_value());

    auto r = be->reader(primary_prefix("cat-1"));
    ASSERT_TRUE(r.has_value());

    EXPECT_TRUE(r.value()->find_latest("cat-1:cfe").has_value());

    // Out-of-scope ids report NotFound even though the underlying
    // file contains them — the scope is enforced at index build.
    auto outside = r.value()->find_latest("cat-2:cfe");
    ASSERT_FALSE(outside.has_value());
    EXPECT_EQ(outside.error().kind, BackendError::Kind::NotFound);

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------
// Snapshot Reader lifetime: Reader outlives Backend
// ---------------------------------------------------------------------

TEST(FileBackend, reader_remains_valid_after_backend_destruction) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    {
        auto be = must_create(path);
        ASSERT_TRUE(be->with_writer(now(), Durability::relaxed, [](Writer& w) {
                          (void)w.write(make_record("cat-1", 0, 0, {'A'}));
                      }).has_value());
    } // FileBackend destroyed

    // Re-open and grab a Reader; let the Backend handle die.
    std::unique_ptr<Reader> reader;
    {
        auto be = must_create(path);
        reader  = std::move(be->reader().value());
    } // FileBackend destroyed; Reader still alive (snapshot pattern)

    auto rec = reader->find_latest("cat-1");
    ASSERT_TRUE(rec.has_value()) << rec.error().message;
    EXPECT_EQ(rec.value().payload, std::vector<char>{'A'});

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------
// Missing-file behavior: empty Reader produces NotFound, not IOError
// ---------------------------------------------------------------------

TEST(FileBackend, reader_on_missing_file_returns_NotFound_on_lookups) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);
    auto r  = be->reader();
    ASSERT_TRUE(r.has_value());

    auto rec = r.value()->find_latest("cat-1");
    ASSERT_FALSE(rec.has_value());
    EXPECT_EQ(rec.error().kind, BackendError::Kind::NotFound);
}

// ---------------------------------------------------------------------
// Duplicate-key diagnostic
// ---------------------------------------------------------------------

TEST(FileBackend, duplicate_key_warning_fires_at_index_build) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);
    // Two records with the same (id, time_step, checkpoint_epoch).
    // This bypasses the single-in-flight rule by being in one
    // with_writer call, but normally they'd come from cross-process
    // collision.
    ASSERT_TRUE(be->with_writer(now(), Durability::relaxed, [](Writer& w) {
                      (void)w.write(make_record("cat-1", 0, 0, {'A'}, /*epoch=*/100));
                      (void)w.write(make_record("cat-1", 0, 0, {'B'}, /*epoch=*/100));
                  }).has_value());

    testing::internal::CaptureStderr();
    auto r = be->reader();
    ASSERT_TRUE(r.has_value());
    const std::string captured = testing::internal::GetCapturedStderr();

    EXPECT_NE(captured.find("duplicate checkpoint record"), std::string::npos)
        << "stderr was: " << captured;

    // Restore should resolve to the higher-offset (later) record.
    auto rec = r.value()->find_latest("cat-1");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec.value().payload, std::vector<char>{'B'})
        << "expected latest-offset to win (B); got: "
        << std::string(rec.value().payload.begin(), rec.value().payload.end());

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------
// finalize() drains deferred errors
// ---------------------------------------------------------------------

TEST(FileBackend, finalize_returns_success_with_no_deferred_errors) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be = must_create(path);
    EXPECT_TRUE(be->finalize().has_value());

    std::remove(path.c_str());
}
