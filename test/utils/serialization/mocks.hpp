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
Reusable mock backends for testing the `ngen::serialization` abstract
surface. Each mock exercises a different slice of the contract:

  - `NoopBackend`     — minimal happy-path; every operation succeeds.
                        Use as a baseline that other tests can rely on.

  - `TrackingBackend` — same as Noop, but records call counts so tests
                        can assert "write was called N times,"
                        "commit fired once," etc.

  - `FailingBackend`  — every fallible method has a per-instance switch
                        to force an error-arm return. Used to test
                        callers' error-handling branches.

  - `SnapshotBackend` — illustrates the "snapshot" sub-handle lifetime
                        pattern: Readers own everything at construction
                        and remain valid after the backend is destroyed.

  - `SharedStateBackend` — illustrates the "shared-state" pattern via
                           `std::enable_shared_from_this`. Sub-handles
                           keep the backend alive while they're in
                           flight.

These mocks are header-only — the test files include this header
directly. They are NOT part of any production library target.
*/
#pragma once

#include "utilities/serialization/id_predicates.hpp"
#include "utilities/serialization/record.hpp"
#include "utilities/serialization/record_backend.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

namespace ngen {
namespace serialization {
namespace test {

using Reader = RecordBackend::Reader;
using Writer = RecordBackend::Writer;

// ---------------------------------------------------------------------
// NoopBackend — every operation returns success with a trivial Record.
// ---------------------------------------------------------------------
struct NoopBackend : RecordBackend {
    struct W : Writer {
        expected<void, BackendError> write(const Record&) override {
            return {};
        }

        expected<void, BackendError> commit() override {
            return {};
        }
    };

    struct R : Reader {
        expected<Record, BackendError> find_latest(const std::string&) override {
            return Record{};
        }

        expected<Record, BackendError> find_at_step(const std::string&, int64_t) override {
            return Record{};
        }

        expected<Record, BackendError>
        find_at_simulation_timestamp(const std::string&, int64_t) override {
            return Record{};
        }
    };

    expected<std::unique_ptr<Writer>, BackendError>
    writer(std::chrono::system_clock::time_point, Durability) override {
        return std::unique_ptr<Writer>(new W);
    }

    expected<std::unique_ptr<Reader>, BackendError> reader(IdPredicate) override {
        return std::unique_ptr<Reader>(new R);
    }

    expected<void, std::vector<BackendError>> finalize() override {
        return {};
    }
};

// ---------------------------------------------------------------------
// TrackingBackend — records call counts + tracks the single-in-flight
// writer flag so tests can assert lifecycle behavior.
// ---------------------------------------------------------------------
struct TrackingBackend : RecordBackend {
    // Counters incremented by the corresponding sub-handle method.
    std::atomic<int> writes_called{0};
    std::atomic<int> writer_commits_called{0};
    std::atomic<int> writers_constructed{0};
    std::atomic<int> writers_destroyed{0};
    std::atomic<int> readers_constructed{0};
    std::atomic<int> readers_destroyed{0};
    std::atomic<int> reader_closes_called{0};
    std::atomic<int> finalize_called{0};

    // Single-in-flight bookkeeping.
    bool writer_in_flight = false;

    struct W : Writer {
        TrackingBackend* parent;

        explicit W(TrackingBackend* p)
            : parent(p) {
            parent->writer_in_flight = true;
            ++parent->writers_constructed;
        }

        ~W() override {
            parent->writer_in_flight = false;
            ++parent->writers_destroyed;
        }

        expected<void, BackendError> write(const Record&) override {
            ++parent->writes_called;
            return {};
        }

        expected<void, BackendError> commit() override {
            ++parent->writer_commits_called;
            return {};
        }
    };

    struct R : Reader {
        TrackingBackend* parent;

        explicit R(TrackingBackend* p)
            : parent(p) {
            ++parent->readers_constructed;
        }

        ~R() override {
            ++parent->readers_destroyed;
        }

        expected<Record, BackendError> find_latest(const std::string&) override {
            return Record{};
        }

        expected<Record, BackendError> find_at_step(const std::string&, int64_t) override {
            return Record{};
        }

        expected<Record, BackendError>
        find_at_simulation_timestamp(const std::string&, int64_t) override {
            return Record{};
        }

        expected<void, BackendError> close() override {
            ++parent->reader_closes_called;
            return {};
        }
    };

    expected<std::unique_ptr<Writer>, BackendError>
    writer(std::chrono::system_clock::time_point, Durability) override {
        if (writer_in_flight) {
            return make_unexpected(
                BackendError{
                    BackendError::Kind::IOError,
                    "TrackingBackend: single-in-flight rule violated"
                }
            );
        }
        return std::unique_ptr<Writer>(new W(this));
    }

    expected<std::unique_ptr<Reader>, BackendError> reader(IdPredicate) override {
        return std::unique_ptr<Reader>(new R(this));
    }

    expected<void, std::vector<BackendError>> finalize() override {
        ++finalize_called;
        return {};
    }
};

// ---------------------------------------------------------------------
// FailingBackend — each fallible method has a switch that forces it
// onto the error arm. Tests flip the switch they care about and
// inspect the caller's error-handling behavior.
// ---------------------------------------------------------------------
struct FailingBackend : RecordBackend {
    // Per-method failure switches. Default OFF; tests set ON to test
    // a specific failure path.
    bool writer_factory_fails   = false;
    bool reader_factory_fails   = false;
    bool write_fails            = false;
    bool writer_commit_fails    = false;
    bool reader_close_fails     = false;
    bool find_returns_not_found = false;
    bool find_returns_corrupted = false;
    bool finalize_fails         = false;

    struct W : Writer {
        FailingBackend* parent;

        explicit W(FailingBackend* p)
            : parent(p) {
        }

        expected<void, BackendError> write(const Record&) override {
            if (parent->write_fails)
                return make_unexpected(
                    BackendError{BackendError::Kind::IOError, "forced write failure"}
                );
            return {};
        }

        expected<void, BackendError> commit() override {
            if (parent->writer_commit_fails)
                return make_unexpected(
                    BackendError{BackendError::Kind::IOError, "forced commit failure"}
                );
            return {};
        }
    };

    struct R : Reader {
        FailingBackend* parent;

        explicit R(FailingBackend* p)
            : parent(p) {
        }

        expected<Record, BackendError> find_latest(const std::string&) override {
            return find_impl();
        }

        expected<Record, BackendError> find_at_step(const std::string&, int64_t) override {
            return find_impl();
        }

        expected<Record, BackendError>
        find_at_simulation_timestamp(const std::string&, int64_t) override {
            return find_impl();
        }

        expected<void, BackendError> close() override {
            if (parent->reader_close_fails)
                return make_unexpected(
                    BackendError{BackendError::Kind::IOError, "forced close failure"}
                );
            return {};
        }

      private:
        expected<Record, BackendError> find_impl() {
            if (parent->find_returns_not_found)
                return make_unexpected(
                    BackendError{BackendError::Kind::NotFound, "forced NotFound"}
                );
            if (parent->find_returns_corrupted)
                return make_unexpected(
                    BackendError{BackendError::Kind::Corrupted, "forced Corrupted"}
                );
            return Record{};
        }
    };

    expected<std::unique_ptr<Writer>, BackendError>
    writer(std::chrono::system_clock::time_point, Durability) override {
        if (writer_factory_fails)
            return make_unexpected(
                BackendError{BackendError::Kind::IOError, "forced writer factory failure"}
            );
        return std::unique_ptr<Writer>(new W(this));
    }

    expected<std::unique_ptr<Reader>, BackendError> reader(IdPredicate) override {
        if (reader_factory_fails)
            return make_unexpected(
                BackendError{BackendError::Kind::IOError, "forced reader factory failure"}
            );
        return std::unique_ptr<Reader>(new R(this));
    }

    expected<void, std::vector<BackendError>> finalize() override {
        if (finalize_fails)
            return make_unexpected(
                std::vector<BackendError>{
                    BackendError{BackendError::Kind::IOError, "forced finalize failure"}
            }
            );
        return {};
    }
};

// ---------------------------------------------------------------------
// SnapshotBackend — Reader is fully self-contained at construction.
// Backend can be destroyed independently of any live Readers.
// ---------------------------------------------------------------------
struct SnapshotBackend : RecordBackend {
    // Per-instance tag the Reader copies at construction; used by
    // tests to assert the Reader still works after the backend dies.
    int tag = 0;

    struct R : Reader {
        int captured_tag;

        explicit R(int t)
            : captured_tag(t) {
        }

        expected<Record, BackendError> find_latest(const std::string&) override {
            Record rec;
            rec.id = "snapshot-tag-" + std::to_string(captured_tag);
            return rec;
        }

        expected<Record, BackendError> find_at_step(const std::string&, int64_t) override {
            return find_latest("");
        }

        expected<Record, BackendError>
        find_at_simulation_timestamp(const std::string&, int64_t) override {
            return find_latest("");
        }
    };

    struct W : Writer {
        expected<void, BackendError> write(const Record&) override {
            return {};
        }

        expected<void, BackendError> commit() override {
            return {};
        }
    };

    expected<std::unique_ptr<Writer>, BackendError>
    writer(std::chrono::system_clock::time_point, Durability) override {
        return std::unique_ptr<Writer>(new W);
    }

    expected<std::unique_ptr<Reader>, BackendError> reader(IdPredicate) override {
        // Snapshot: Reader captures the tag *by value* at construction.
        // No pointer to `this` is retained.
        return std::unique_ptr<Reader>(new R(tag));
    }

    expected<void, std::vector<BackendError>> finalize() override {
        return {};
    }
};

// ---------------------------------------------------------------------
// SharedStateBackend — Reader/Writer hold shared_ptr<Backend>.
// Backend stays alive while any sub-handle does.
// ---------------------------------------------------------------------
struct SharedStateBackend
    : RecordBackend
    , std::enable_shared_from_this<SharedStateBackend> {
    // Shared resource the sub-handles access through the parent.
    std::string shared_resource = "shared";

    // Counters so tests can observe construction/destruction ordering.
    std::atomic<int> live_subhandles{0};

    struct R : Reader {
        std::shared_ptr<SharedStateBackend> parent;

        explicit R(std::shared_ptr<SharedStateBackend> p)
            : parent(std::move(p)) {
            ++parent->live_subhandles;
        }

        ~R() override {
            --parent->live_subhandles;
        }

        expected<Record, BackendError> find_latest(const std::string&) override {
            Record rec;
            rec.id = parent->shared_resource; // reads through the live backend
            return rec;
        }

        expected<Record, BackendError> find_at_step(const std::string&, int64_t) override {
            return find_latest("");
        }

        expected<Record, BackendError>
        find_at_simulation_timestamp(const std::string&, int64_t) override {
            return find_latest("");
        }
    };

    struct W : Writer {
        std::shared_ptr<SharedStateBackend> parent;

        explicit W(std::shared_ptr<SharedStateBackend> p)
            : parent(std::move(p)) {
            ++parent->live_subhandles;
        }

        ~W() override {
            --parent->live_subhandles;
        }

        expected<void, BackendError> write(const Record&) override {
            return {};
        }

        expected<void, BackendError> commit() override {
            return {};
        }
    };

    expected<std::unique_ptr<Writer>, BackendError>
    writer(std::chrono::system_clock::time_point, Durability) override {
        return std::unique_ptr<Writer>(new W(shared_from_this()));
    }

    expected<std::unique_ptr<Reader>, BackendError> reader(IdPredicate) override {
        return std::unique_ptr<Reader>(new R(shared_from_this()));
    }

    expected<void, std::vector<BackendError>> finalize() override {
        return {};
    }
};

} // namespace test
} // namespace serialization
} // namespace ngen
