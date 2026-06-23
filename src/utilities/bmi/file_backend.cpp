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
*/

#include "file_backend.hpp"
#include "serialization_record.hpp" // BMI-side wire-format helpers
#include "wire_format.hpp" // encode_record_prefix primitive
#include <algorithm>
#include <boost/optional.hpp>
#include <tuple>

// POSIX-only fsync support: when <unistd.h> + <fcntl.h> are present
// we own the write fd directly via `::open()` and call `::fsync()`
// on it under `Durability::strict`. On non-POSIX builds we refuse
// `Durability::strict` at writer() time and fall back to an
// `std::ofstream` for the write path.
//
// The detection guard can be overridden from outside (build system
// or a test TU) by defining `NGEN_HAVE_POSIX_FSYNC` to 0 before
// compiling this file. The dedicated `test_file_backend_no_fsync`
// target uses that override to exercise the non-POSIX error path
// from a POSIX test runner.
#ifndef NGEN_HAVE_POSIX_FSYNC
#if __has_include(<unistd.h>) && __has_include(<fcntl.h>)
#define NGEN_HAVE_POSIX_FSYNC 1
#else
#define NGEN_HAVE_POSIX_FSYNC 0
#endif
#endif

#if NGEN_HAVE_POSIX_FSYNC
#include <fcntl.h> // ::open, O_WRONLY, O_APPEND, O_CREAT
#include <unistd.h> // ::fsync, ::close, ::write
#endif

#include <array>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <utility>

namespace ngen {
namespace serialization {

namespace {

namespace wf  = models::bmi::protocols::serialization::wire_format;
namespace bmi = models::bmi::protocols;

inline BackendError io_error(std::string msg) {
    return BackendError{BackendError::Kind::IOError, std::move(msg)};
}

inline BackendError corrupted_error(std::string msg) {
    return BackendError{BackendError::Kind::Corrupted, std::move(msg)};
}

inline BackendError not_found_error(std::string msg) {
    return BackendError{BackendError::Kind::NotFound, std::move(msg)};
}

#if NGEN_HAVE_POSIX_FSYNC
// Write exactly @p n bytes from @p data to @p fd, handling EINTR and
// short writes. Caller MUST hold the backend's io_mutex_ across the
// span of all writes for a single record so other writers can't
// interleave at sub-record granularity.
inline auto
write_all_to_fd(int fd, const void* data, std::size_t n, const char* what, const std::string& path)
    -> expected<void, BackendError> {
    const auto* p     = static_cast<const std::uint8_t*>(data);
    std::size_t total = 0;
    while (total < n) {
        const ssize_t r = ::write(fd, p + total, n - total);
        if (r < 0) {
            if (errno == EINTR) continue;
            const int saved_errno = errno;
            return make_unexpected(io_error(
                std::string("FileBackend::Writer: ::write of ") + what + " failed on '" + path
                + "': " + std::strerror(saved_errno)
            ));
        }
        if (r == 0) {
            return make_unexpected(io_error(
                std::string("FileBackend::Writer: ::write of ") + what + " returned 0 on '" + path
                + "' (no progress)"
            ));
        }
        total += static_cast<std::size_t>(r);
    }
    return {};
}
#endif // NGEN_HAVE_POSIX_FSYNC

} // anonymous namespace

// ---------------------------------------------------------------------
// FileBackend::Index — record-metadata index built once at backend
// construction (and optionally rebuilt on demand when `dirty_` is
// set). Shared with every Reader via shared_ptr<const Index>.
// ---------------------------------------------------------------------

class FileBackend::Index {
  public:
    struct Entry {
        int64_t time_step;
        int64_t simulation_timestamp;
        int64_t checkpoint_epoch;
        uint64_t file_offset; // start of the record's prefix
    };

    /** Walk @p path and build an index of records whose id satisfies
     *  @p construction_scope. An empty predicate means "index
     *  everything." Returns a non-null shared_ptr<const Index> on
     *  success; the IOError arm fires only on stream-open failure of
     *  a kind that isn't "file doesn't exist" (missing-file yields
     *  an empty index — a valid outcome that pairs with NotFound on
     *  every Reader lookup). */
    static auto build(const std::string& path, const IdPredicate& construction_scope)
        -> expected<std::shared_ptr<const Index>, BackendError> {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            return std::shared_ptr<const Index>(new Index{});
        }

        std::shared_ptr<Index> idx(new Index{});

        while (true) {
            const auto pos_stream = in.tellg();
            if (pos_stream == std::streampos(-1)) break;
            const uint64_t pos = static_cast<uint64_t>(pos_stream);

            wf::RecordPrefix prefix;
            std::string id;
            auto status = bmi::read_record_metadata(in, prefix, id);
            if (!status) {
                std::cerr << "FileBackend: index walk stopped on "
                          << "malformed record in '" << path << "': " << status.error()
                          << std::endl;
                break;
            }
            if (status.value() == bmi::Status::Eof) break;

            // Construction-scope filter — out-of-scope ids never
            // enter the index.
            if (construction_scope && !construction_scope(id)) continue;

            Entry
                entry{prefix.time_step, prefix.simulation_timestamp, prefix.checkpoint_epoch, pos};
            idx->insert_or_replace(std::move(id), entry, path);
        }

        return std::static_pointer_cast<const Index>(idx);
    }

    boost::optional<const Entry&> find_latest(const std::string& id) const {
        auto it = by_id_.find(id);
        if (it == by_id_.end() || it->second.empty()) return boost::none;
        auto best_it = std::max_element(
            it->second.begin(),
            it->second.end(),
            [](const Entry& a, const Entry& b) {
                // First compare time steps, if they are equal compare checkpoint_epoch
                return std::tie(a.time_step, a.checkpoint_epoch)
                       < std::tie(b.time_step, b.checkpoint_epoch);
            }
        );
        return *best_it;
    }

    boost::optional<const Entry&> find_at_step(const std::string& id, int64_t step) const {
        auto it = by_id_.find(id);
        if (it == by_id_.end()) return boost::none;
        boost::optional<const Entry&> best;
        for (const auto& e : it->second) {
            if (e.time_step != step) continue;
            if (!best || e.checkpoint_epoch > best->checkpoint_epoch) best = e;
        }
        return best;
    }

    boost::optional<const Entry&>
    find_at_simulation_timestamp(const std::string& id, int64_t simulation_timestamp) const {
        auto it = by_id_.find(id);
        if (it == by_id_.end()) return boost::none;
        boost::optional<const Entry&> best;
        for (const auto& e : it->second) {
            if (e.simulation_timestamp != simulation_timestamp) continue;
            if (!best || e.checkpoint_epoch > best->checkpoint_epoch) best = e;
        }
        return best;
    }

    std::size_t size() const noexcept {
        std::size_t n = 0;
        for (const auto& kv : by_id_) n += kv.second.size();
        return n;
    }

    // Public default constructor: Index is a private inner class of
    // FileBackend so visibility is bounded; the public ctor lets the
    // FileBackend factory construct an empty Index.
    Index() = default;

  private:
    void insert_or_replace(std::string id, const Entry& entry, const std::string& path) {
        auto& bucket = by_id_[std::move(id)];
        for (auto& existing : bucket) {
            if (existing.time_step == entry.time_step
                && existing.checkpoint_epoch == entry.checkpoint_epoch) {
                std::cerr << "FileBackend: duplicate checkpoint record "
                          << "(time_step=" << entry.time_step
                          << ", checkpoint_epoch=" << entry.checkpoint_epoch << ") in '" << path
                          << "'; restoring from offset "
                          << std::max(existing.file_offset, entry.file_offset)
                          << ". This indicates a save-event collision in "
                          << "the producing run." << std::endl;
                if (entry.file_offset > existing.file_offset) {
                    existing = entry;
                }
                return;
            }
        }
        bucket.push_back(entry);
    }

    std::unordered_map<std::string, std::vector<Entry>> by_id_;
};

// ---------------------------------------------------------------------
// FileBackend::Writer — thin logical handle. Does not own a fd; the
// backend owns the fd. Each write() / commit() acquires the backend's
// io_mutex_ so concurrent Writers cannot interleave records at
// sub-record granularity. Multiple Writer handles may coexist on one
// backend; they take turns through the mutex.
// ---------------------------------------------------------------------

class FileBackend::Writer : public RecordBackend::Writer {
  public:
    Writer(std::shared_ptr<FileBackend> parent, Durability durability)
        : parent_(std::move(parent))
        , durability_(durability) {
    }

    ~Writer() override = default;

    auto write(const RecordView& rec) -> expected<void, BackendError> override {
        // Validate body-size caps against the wire format before
        // touching the fd. Doing this outside the lock is fine —
        // the values don't depend on backend state.
        if (rec.id.size() > 0xFFFFu) {
            return make_unexpected(io_error(
                "FileBackend::Writer: id length " + std::to_string(rec.id.size())
                + " exceeds the wire format's uint16 id_length field on '" + parent_->path_ + "'"
            ));
        }
        if (rec.payload.size() > bmi::MAX_RECORD_PAYLOAD_BYTES) {
            return make_unexpected(io_error(
                "FileBackend::Writer: payload length " + std::to_string(rec.payload.size())
                + " exceeds MAX_RECORD_PAYLOAD_BYTES on '" + parent_->path_ + "'"
            ));
        }

#if NGEN_HAVE_POSIX_FSYNC
        // To enable fsync in commit(), we need write a file descriptor
        // directly.  So if posix/fsync are available, we use this mechanism
        // for hitting the disk so we can use fsync for strict durability when requested.
        std::lock_guard<std::mutex> lk(parent_->io_mutex_);

        // lazy open the descriptor, if something goes wrong then this will fail
        // and we just forward the error.
        if (auto r = parent_->ensure_write_fd_locked_(); !r) {
            return r;
        }

        wf::RecordPrefix prefix;
        prefix.time_step            = rec.time_step;
        prefix.simulation_timestamp = rec.simulation_timestamp;
        prefix.checkpoint_epoch     = rec.checkpoint_epoch;
        prefix.id_length            = static_cast<std::uint16_t>(rec.id.size());
        prefix.payload_length       = static_cast<std::uint64_t>(rec.payload.size());

        std::array<std::uint8_t, wf::RecordPrefix::PREFIX_BYTES> prefix_buf;
        wf::encode_record_prefix(prefix_buf.data(), prefix);

        auto r = write_all_to_fd(
            parent_->fd_,
            prefix_buf.data(),
            prefix_buf.size(),
            "prefix",
            parent_->path_
        );
        // propagate error if write failed
        if (!r) return r;
        if (!rec.id.empty()) {
            r = write_all_to_fd(
                parent_->fd_,
                rec.id.data(),
                rec.id.size(),
                "id" /**/,
                parent_->path_
            );
            if (!r) return r;
        }
        if (!rec.payload.empty()) {
            r = write_all_to_fd(
                parent_->fd_,
                rec.payload.data(),
                rec.payload.size(),
                "payload",
                parent_->path_
            );
            if (!r) return r;
        }

        parent_->dirty_ = true;
        return {};
#else
        // Non-POSIX build: the backend has no owned fd; fall back to
        // a transient ofstream per write under the same mutex so
        // concurrent Writers still serialize.
        std::lock_guard<std::mutex> lk(parent_->io_mutex_);

        std::ofstream out(parent_->path_, std::ios::binary | std::ios::app);
        if (!out.is_open()) {
            const int saved_errno = errno;
            return make_unexpected(io_error(
                "FileBackend::Writer: failed to open '" + parent_->path_
                + "' for write: " + std::strerror(saved_errno)
            ));
        }
        auto r = bmi::write_record(out, rec);
        if (!r) {
            return make_unexpected(io_error("FileBackend::Writer: " + r.error()));
        }
        out.flush();
        if (!out) {
            return make_unexpected(
                io_error("FileBackend::Writer: stream error on write to '" + parent_->path_ + "'")
            );
        }
        parent_->dirty_ = true;
        return {};
#endif
    }

    auto commit() -> expected<void, BackendError> override {
#if NGEN_HAVE_POSIX_FSYNC
        std::lock_guard<std::mutex> lk(parent_->io_mutex_);

        // Nothing to fsync if no writes have happened — fd is still -1.
        if (parent_->fd_ < 0) return {};

        if (durability_ == Durability::strict) {
            if (::fsync(parent_->fd_) != 0) {
                const int saved_errno = errno;
                return make_unexpected(io_error(
                    "FileBackend::Writer: fsync failed on '" + parent_->path_
                    + "': " + std::strerror(saved_errno)
                ));
            }
        }
        return {};
#else
        if (durability_ == Durability::strict) {
            return make_unexpected(
                io_error("FileBackend::Writer: Durability::strict is not supported "
                         "on this build (no POSIX fsync available); use "
                         "Durability::relaxed for best-effort flush semantics on "
                         "this platform.")
            );
        }
        // Relaxed: the per-write ofstream already flushed under the
        // mutex; nothing to do here.
        return {};
#endif
    }

  private:
    std::shared_ptr<FileBackend> parent_;
    Durability durability_;
};

// ---------------------------------------------------------------------
// FileBackend::Reader — snapshot pattern. Owns its own ifstream and
// shares a shared_ptr<const Index> with the parent backend. Applies
// the per-Reader read_scope_ at find_* time.
// ---------------------------------------------------------------------

class FileBackend::Reader : public RecordBackend::Reader {
  public:
    Reader(
        std::string path,
        std::ifstream stream,
        std::shared_ptr<const Index> index,
        IdPredicate read_scope
    )
        : path_(std::move(path))
        , in_(std::move(stream))
        , index_(std::move(index))
        , read_scope_(std::move(read_scope)) {
    }

    auto find_latest(const std::string& id) -> expected<Record, BackendError> override {
        if (read_scope_ && !read_scope_(id)) {
            return make_unexpected(not_found_error(
                "FileBackend::Reader: id '" + id + "' is out of this Reader's read scope on '"
                + path_ + "'"
            ));
        }
        if (auto entry = index_->find_latest(id)) {
            return load_at(*entry);
        }
        return make_unexpected(not_found_error(
            "FileBackend::Reader: no matching record for id '" + id + "' in '" + path_ + "'"
        ));
    }

    auto find_at_step(const std::string& id, int64_t time_step)
        -> expected<Record, BackendError> override {
        if (read_scope_ && !read_scope_(id)) {
            return make_unexpected(not_found_error(
                "FileBackend::Reader: id '" + id + "' is out of this Reader's read scope on '"
                + path_ + "'"
            ));
        }
        if (auto entry = index_->find_at_step(id, time_step)) {
            return load_at(*entry);
        }
        return make_unexpected(not_found_error(
            "FileBackend::Reader: no matching record for id '" + id + "' at step "
            + std::to_string(time_step) + " in '" + path_ + "'"
        ));
    }

    auto find_at_simulation_timestamp(const std::string& id, int64_t simulation_timestamp)
        -> expected<Record, BackendError> override {
        if (read_scope_ && !read_scope_(id)) {
            return make_unexpected(not_found_error(
                "FileBackend::Reader: id '" + id + "' is out of this Reader's read scope on '"
                + path_ + "'"
            ));
        }
        if (auto entry = index_->find_at_simulation_timestamp(id, simulation_timestamp)) {
            return load_at(*entry);
        }
        return make_unexpected(not_found_error(
            "FileBackend::Reader: no matching record for id '" + id + "' at timestamp '"
            + std::to_string(simulation_timestamp) + "' in '" + path_ + "'"
        ));
    }

  private:
    expected<Record, BackendError> load_at(const Index::Entry& entry) {
        in_.clear();
        in_.seekg(static_cast<std::streamoff>(entry.file_offset), std::ios::beg);
        if (!in_) {
            return make_unexpected(io_error(
                "FileBackend::Reader: seek to offset " + std::to_string(entry.file_offset)
                + " failed in '" + path_ + "'"
            ));
        }

        Record rec;
        auto r = bmi::read_next_record(in_, rec);
        if (!r) {
            return make_unexpected(corrupted_error("FileBackend::Reader: " + r.error()));
        }
        if (r.value() == bmi::Status::Eof) {
            return make_unexpected(corrupted_error(
                "FileBackend::Reader: record missing at indexed offset "
                + std::to_string(entry.file_offset) + " in '" + path_ + "'"
            ));
        }
        return rec;
    }

    std::string path_;
    std::ifstream in_;
    std::shared_ptr<const Index> index_;
    IdPredicate read_scope_;
};

// ---------------------------------------------------------------------
// FileBackend itself
// ---------------------------------------------------------------------

// Static storage for the registry.
std::mutex FileBackend::registry_mutex_;
std::unordered_map<std::string, std::weak_ptr<FileBackend>> FileBackend::registry_;

auto FileBackend::create(std::string path, IdPredicate construction_scope)
    -> expected<std::shared_ptr<FileBackend>, BackendError> {
    std::lock_guard<std::mutex> lk(registry_mutex_);

    // Opportunistic prune: cheap walk since the table is small in
    // practice (typically one entry per distinct path, and
    // applications rarely use many paths concurrently).
    for (auto it = registry_.begin(); it != registry_.end();) {
        if (it->second.expired()) {
            it = registry_.erase(it);
        } else {
            ++it;
        }
    }

    auto it = registry_.find(path);
    if (it != registry_.end()) {
        if (auto sp = it->second.lock()) {
            // Cache hit. construction_scope arg is ignored; callers
            // for the same path must agree (documented contract).
            return sp;
        }
    }

    // Cache miss. Construct, build the index under the requested
    // scope, register. shared_ptr<new> instead of make_shared because
    // the constructor is private.
    auto sp =
        std::shared_ptr<FileBackend>(new FileBackend(std::move(path), std::move(construction_scope))
        );

    // Build the initial index. A failure here is a hard create()
    // error — we do NOT register a backend whose index didn't
    // build, because subsequent callers asking for the same path
    // would receive a permanently broken backend with no signal of
    // the underlying problem.
    {
        std::lock_guard<std::mutex> backend_lk(sp->io_mutex_);
        if (auto built = sp->build_index_locked_(); !built) {
            return make_unexpected(std::move(built.error()));
        }
    }

    registry_[sp->path_] = sp;
    return sp;
}

FileBackend::FileBackend(std::string path, IdPredicate construction_scope)
    : path_(std::move(path))
    , construction_scope_(std::move(construction_scope)) {
}

FileBackend::~FileBackend() {
    // Close the owned write fd if it's open. No fsync here — that
    // would silently change durability semantics. Callers requiring
    // strict durability must commit() explicitly before dropping.
#if NGEN_HAVE_POSIX_FSYNC
    if (fd_ >= 0) ::close(fd_);
#endif
}

auto FileBackend::build_index_locked_() -> expected<void, BackendError> {
    auto built = Index::build(path_, construction_scope_);
    if (!built) {
        return make_unexpected(std::move(built.error()));
    }
    index_ = std::move(built.value());
    dirty_ = false;
    return {};
}

auto FileBackend::ensure_write_fd_locked_() -> expected<void, BackendError> {
#if NGEN_HAVE_POSIX_FSYNC
    if (fd_ >= 0) return {};

    // 0644 — owner read/write, others read. Matches the typical
    // ofstream mode mask.
    const int new_fd = ::open(path_.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (new_fd < 0) {
        const int saved_errno = errno;
        return make_unexpected(io_error(
            "FileBackend: failed to open '" + path_ + "' for write: " + std::strerror(saved_errno)
        ));
    }
    fd_ = new_fd;
    return {};
#else
    // Non-POSIX: no owned fd. The Writer uses a transient ofstream
    // per write under the mutex. Nothing to do here.
    return {};
#endif
}

auto FileBackend::writer(std::chrono::system_clock::time_point /*epoch*/, Durability durability)
    -> expected<std::unique_ptr<RecordBackend::Writer>, BackendError> {
#if !NGEN_HAVE_POSIX_FSYNC
    if (durability == Durability::strict) {
        return make_unexpected(io_error("FileBackend::writer: Durability::strict is not supported "
                                        "on this build (no POSIX fsync available); use "
                                        "Durability::relaxed for best-effort flush semantics on "
                                        "this platform."));
    }
#endif

    // Writer is a thin handle; constructing it is cheap. The fd is
    // opened lazily on the first write under the io_mutex_.
    return std::unique_ptr<RecordBackend::Writer>(new Writer(shared_from_this(), durability));
}

auto FileBackend::reader(IdPredicate read_scope)
    -> expected<std::unique_ptr<RecordBackend::Reader>, BackendError> {
    // Rebuild the Index if any writer mutated the file since the
    // last build (or initial construction). Cheap on the no-writes
    // path; the typical restore-at-init flow never triggers a
    // rebuild because no writes have happened yet.
    std::shared_ptr<const Index> idx_snapshot;
    {
        std::lock_guard<std::mutex> lk(io_mutex_);
        if (dirty_) {
            auto built = build_index_locked_();
            if (!built) return make_unexpected(std::move(built.error()));
        }
        idx_snapshot = index_;
    }

    // Open a read stream. Missing-file is permitted — the index is
    // empty in that case, every find_* will return NotFound, which
    // is the right answer when there's nothing to look up.
    std::ifstream in(path_, std::ios::binary);

    return std::unique_ptr<RecordBackend::Reader>(
        new Reader(path_, std::move(in), std::move(idx_snapshot), std::move(read_scope))
    );
}

auto FileBackend::finalize() -> expected<void, std::vector<BackendError>> {
    std::vector<BackendError> drained;
    {
        std::lock_guard<std::mutex> lk(io_mutex_);
        drained.swap(deferred_errors_);
    }
    if (drained.empty()) return {};
    return make_unexpected(std::move(drained));
}

} // namespace serialization
} // namespace ngen
