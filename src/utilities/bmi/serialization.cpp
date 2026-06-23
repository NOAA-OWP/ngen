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
Version 0.2 (see serialization.hpp)
*/

#include "serialization.hpp"
#include "file_backend.hpp"
#include "serialization_record.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace models {
namespace bmi {
namespace protocols {

namespace {
// The reserved-variable table (`RESERVED_VARS`) and the
// `parse_timestamp` helper are shared with the restore protocol
// and live in serialization.hpp / serialization_record.hpp
// respectively. This .cpp consumes them in place.

// RAII guard: `SetValue(create)` on construction, `SetValue(free)` on
// destruction. Region (2) in the thread-safety map — the guard is
// per-call so (a) a throw from the middle of run() still releases
// model-side resources, and (b) the (create, free) scope is a single
// syntactic region that the reader can't accidentally split.
//
// Two paths to release:
//   * `release()` — the happy path calls this once the capture has
//     been consumed. It issues the `SetValue(FREE, ...)` itself and
//     propagates any exception so the caller can surface a real BMI
//     error. Calling `release()` more than once is a no-op.
//   * Destructor — best-effort fallback for exception paths. Swallows
//     any thrown error from the model because destructors must not
//     throw and a model that already errored earlier in the scope
//     is in no state to surface a meaningful second failure.
//
// Each (CREATE, FREE) pair uses its own local trigger value: BMI
// `SetValue` reads the pointed-to byte(s) and does not retain the
// pointer, so the guard doesn't need to remember an address.
class ScopedCapture {
  public:
    explicit ScopedCapture(const ModelPtr& model)
        : model_(model) {
        int trigger = 1;
        model_->SetValue(SERIALIZATION_CREATE_NAME, &trigger);
    }

    ~ScopedCapture() {
        if (!released_) {
            try {
                int trigger = 1;
                model_->SetValue(SERIALIZATION_FREE_NAME, &trigger);
            } catch (...) {
                // Suppress — see class-level doc.
            }
        }
    }

    ScopedCapture(const ScopedCapture&)            = delete;
    ScopedCapture& operator=(const ScopedCapture&) = delete;

    // Issue the FREE now, on the happy path. Propagates model
    // exceptions. Idempotent: callers may invoke this exactly once,
    // or not at all if they want the destructor's fallback.
    void release() {
        if (released_) return;
        int trigger = 1;
        model_->SetValue(SERIALIZATION_FREE_NAME, &trigger);
        released_ = true;
    }

  private:
    const ModelPtr& model_;
    bool released_ = false;
};
} // namespace

NgenSerializationProtocol::NgenSerializationProtocol(
    const ModelPtr& model,
    const Properties& properties
)
    : frequency(1)
    , check(false)
    , is_fatal(true) {
    (void)initialize(model, properties);
}

NgenSerializationProtocol::NgenSerializationProtocol()
    : frequency(1)
    , check(false)
    , is_fatal(true) {
}

NgenSerializationProtocol::~NgenSerializationProtocol() = default;

auto NgenSerializationProtocol::run(const ModelPtr& model, const Context& ctx) const
    -> expected<void, ProtocolError> {
    if (model == nullptr) {
        return make_unexpected<ProtocolError>(ProtocolError(
            Error::UNITIALIZED_MODEL,
            "Cannot run serialization protocol with null model."
        ));
    }
    if (!check || !supported) {
        return {};
    }

    // Frequency gate — matches NgenMassBalance.
    bool checkpoint_step = false;
    if (frequency > 0) {
        checkpoint_step = (ctx.current_time_step % frequency) == 0;
    }
    // Context contract: current_time_step is the index of the step just
    // computed, in [0, total_steps - 1]. The end-of-run sentinel at this
    // position is therefore total_steps - 1.
    else if (ctx.current_time_step == ctx.total_steps - 1) {
        checkpoint_step = true;
    }
    if (!checkpoint_step) {
        return {};
    }

    int size = 0;
    try {
        // (create, free) scope is RAII-managed so any throw from
        // GetValue, the record build, or the backend write still
        // releases the model-side capture. See ScopedCapture docstring.
        ScopedCapture capture(model);

        model->GetValue(SERIALIZATION_SIZE_NAME, &size);
        if (size <= 0) {
            // ScopedCapture's destructor will still call FREE on return.
            std::stringstream ss;
            ss << "serialization: non-positive state size (" << size << ") reported by '"
               << model->GetComponentName() << "' at timestep " << ctx.current_time_step << " ("
               << ctx.timestamp << ") for feature id " << ctx.id;
            return make_unexpected<ProtocolError>(
                ProtocolError(is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING, ss.str())
            );
        }
        // Acquire a thin Writer handle, hand it a view of the
        // payload bytes, write, commit, drop. The backend owns the
        // file descriptor and the write critical section's mutex;
        // the Writer handle is just an intent carrier for
        // durability + lifetime. Acquiring is cheap (no syscalls
        // in the steady state — the fd was opened on first write
        // and stays open for the backend's lifetime). The mutex
        // inside the backend serializes records across all
        // protocol instances sharing the same FileBackend.
        //
        // the writer takes a non-owning
        // `RecordView` over the fucntion-scoped payload buffer
        // and writes it before the scope ends. 
        // A future zero-copy path can drop `buf` entirely and hand 
        // the writer a view over model-owned storage instead via
        // GetValuePtr
        auto wo = backend_->writer(
            std::chrono::system_clock::now(),
            ::ngen::serialization::Durability::relaxed
        );
        if (!wo) {
            std::stringstream ss;
            ss << "serialization: " << wo.error().message << " for '" << model->GetComponentName()
               << "' at timestep " << ctx.current_time_step << " (" << ctx.timestamp << ")";
            return make_unexpected<ProtocolError>(
                ProtocolError(is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING, ss.str())
            );
        }
        auto& writer = *wo.value();

        // Payload buffer to serialize.  This is a defensive double buffer
        // at the moment, ensuring a "snapshot" of model state without 
        // concerns for pointer lifetime/validity.
        // a pure zero-copy could try GetValuePtr and get a span over
        // model-owned storage instead. At the moment, cost here is really
        // just allocation per feature per checkpoint.
        std::vector<char> buf(static_cast<size_t>(size));
        model->GetValue(SERIALIZATION_STATE_NAME, buf.data());

        // `checkpoint_epoch` is stamped per record with the writing
        // host's wall-clock time.
        auto write_outcome = writer.write(::ngen::serialization::RecordView{
            ctx.id,
            static_cast<int64_t>(ctx.current_time_step),
            parse_timestamp(ctx.timestamp),
            boost::span<const char>{buf.data(), buf.size()},
            static_cast<int64_t>(std::time(nullptr))
        });

        if (!write_outcome) {
            std::stringstream ss;
            ss << "serialization: " << write_outcome.error().message << " for '"
               << model->GetComponentName() << "' at timestep " << ctx.current_time_step << " ("
               << ctx.timestamp << ")";
            return make_unexpected<ProtocolError>(
                ProtocolError(is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING, ss.str())
            );
        }

        auto commit_outcome = writer.commit();
        if (!commit_outcome) {
            std::stringstream ss;
            ss << "serialization: " << commit_outcome.error().message << " for '"
               << model->GetComponentName() << "' at timestep " << ctx.current_time_step << " ("
               << ctx.timestamp << ")";
            return make_unexpected<ProtocolError>(
                ProtocolError(is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING, ss.str())
            );
        }

        // Happy-path FREE: `release()` issues the BMI SetValue itself
        // and propagates any model-side exception. Exception paths fall
        // through to the destructor's best-effort fallback FREE.
        capture.release();
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "serialization: BMI or archive exchange failed for '" << model->GetComponentName()
           << "' at timestep " << ctx.current_time_step << " (" << ctx.timestamp
           << ") for feature id " << ctx.id << ": " << e.what();
        return make_unexpected<ProtocolError>(
            ProtocolError(is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING, ss.str())
        );
    }

    return {};
}

auto NgenSerializationProtocol::check_support(const ModelPtr& model)
    -> expected<void, ProtocolError> {
    if (model == nullptr || !model->is_model_initialized()) {
        return make_unexpected<ProtocolError>(ProtocolError(
            Error::UNITIALIZED_MODEL,
            "Cannot check serialization support for uninitialized model. Disabling serialization "
            "protocol."
        ));
    }

    // Metadata-only probe. For each reserved variable we require the model to
    //   (a) recognize the name via GetVarUnits, and
    //   (b) report the exact expected unit string from the protocol convention
    //       (see doc/BMI_SERIALIZATION_PROTOCOL.md).
    // No SetValue / GetValue(state) is performed — BMI state mutation is the
    // engine's responsibility, not the support probe's. Failures from either
    // clause yield an INTEGRATION_ERROR naming the offending variable.
    // The reserved-variable table is defined in serialization.hpp and
    // shared with the restore protocol so the two agree on the exact
    // set of variables a conforming model must expose.
    for (const auto& ev : RESERVED_VARS) {
        std::string reported;
        try {
            reported = model->GetVarUnits(ev.name);
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << "serialization: model '" << model->GetComponentName()
               << "' does not expose reserved variable '" << ev.name
               << "' (GetVarUnits failed: " << e.what() << ").";
            return make_unexpected<ProtocolError>(
                ProtocolError(Error::INTEGRATION_ERROR, ss.str())
            );
        }
        if (reported != ev.unit) {
            std::stringstream ss;
            ss << "serialization: model '" << model->GetComponentName()
               << "' exposes reserved variable '" << ev.name << "' but with unit '" << reported
               << "'; expected '" << ev.unit << "'.";
            return make_unexpected<ProtocolError>(
                ProtocolError(Error::INTEGRATION_ERROR, ss.str())
            );
        }
    }
    this->supported = true;
    return {};
}

auto NgenSerializationProtocol::initialize(const ModelPtr& model, const Properties& properties)
    -> expected<void, ProtocolError> {
    auto top_it = properties.find(SERIALIZATION_CONFIGURATION_KEY);
    if (top_it == properties.end()) {
        // No serialization block at all — stay silent, stay disabled.
        check = false;
        return {};
    }
    geojson::PropertyMap top = top_it->second.get_values();

    // Shared path lives at the top level. Local-only: the string is
    // handed to the backend factory below and the backend owns it
    // thereafter — the protocol does not need its own copy.
    std::string path;
    auto path_it = top.find(SERIALIZATION_PATH_KEY);
    if (path_it != top.end()) {
        path = path_it->second.as_string();
    }

    // Save-specific options live under "save".
    auto save_it = top.find(SERIALIZATION_SAVE_KEY);
    if (save_it == top.end()) {
        // Restore may still be configured; save is not.
        check = false;
        return {};
    }
    geojson::PropertyMap save = save_it->second.get_values();

    auto _it = save.find(SERIALIZATION_CHECK_KEY);
    if (_it != save.end()) {
        check = _it->second.as_boolean();
    } else {
        check = true; // save block present without check -> enable
    }

    _it = save.find(SERIALIZATION_FATAL_KEY);
    if (_it != save.end()) {
        is_fatal = _it->second.as_boolean();
    }

    _it = save.find(SERIALIZATION_FREQUENCY_KEY);
    if (_it != save.end()) {
        frequency = static_cast<int>(_it->second.as_natural_number());
    } else {
        frequency = 1;
    }
    if (frequency == 0) {
        check = false;
    }

    if (check && path.empty()) {
        check = false;
        return error_or_warning(ProtocolError(
            Error::PROTOCOL_WARNING,
            "serialization: 'path' not specified at the top-level serialization block; disabling "
            "save protocol."
        ));
    }

    if (check) {
        auto probe = check_support(model);
        if (!probe.has_value()) {
            // Escalate only on true *non-conformance* (INTEGRATION_ERROR:
            // the model is loaded and initialized but its declared units
            // don't match the protocol). UNITIALIZED_MODEL and other
            // probe failures stay on the warn-and-disable path so that
            // a null model reported at init fails at run() time with the
            // usual UNITIALIZED_MODEL signal instead of throwing from
            // the constructor.
            if (is_fatal && probe.error().error_code() == Error::INTEGRATION_ERROR) {
                throw ProtocolError(Error::PROTOCOL_ERROR, probe.error().get_message());
            }
            // Non-fatal (or non-conformance error of another kind):
            // warn, disable, make run() a no-op. Propagate the warning
            // result so the caller can observe it; constructor discards
            // explicitly.
            check = false;
            return error_or_warning(probe.error());
        }
    }
    // All other initialization is good, build the file backend.
    // Construction can fail (e.g. corrupted index walk on an
    // existing file); we surface that as a protocol-level error
    // and disable rather than register a half-broken backend.
    // Future work may swap this via a set_backend() setter for
    // testing or non-file storage.
    auto be = ::ngen::serialization::FileBackend::create(path);
    if (!be) {
        check = false;
        std::stringstream ss;
        ss << "serialization: failed to construct backend at '" << path
           << "': " << be.error().message;
        return error_or_warning(
            ProtocolError(is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING, ss.str())
        );
    }
    backend_ = std::move(be.value());
    return {};
}

bool NgenSerializationProtocol::is_supported() const {
    return this->supported;
}

} // namespace protocols
} // namespace bmi
} // namespace models
