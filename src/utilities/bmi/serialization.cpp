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
Version 0.1 (see serialization.hpp)
*/

#include "serialization.hpp"
#include "serialization_record.hpp"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace models{ namespace bmi{ namespace protocols{

namespace {
    // Starting capacity for the per-instance reusable payload buffer.
    // Chosen so the first `resize()` inside `run()` is almost never the
    // allocation-triggering one for realistic BMI state sizes. The
    // buffer only grows; it is never shrunk, which is fine because each
    // protocol instance is bound to one model whose state size is
    // stationary across timesteps.
    constexpr size_t INITIAL_PAYLOAD_BUFFER_BYTES = 4096;
    // Explicit streambuf size. std::ofstream defaults to an implementation-
    // defined (typically small) buffer. At 1M features writing tens of
    // bytes each, an explicit larger buffer amortizes the syscall rate
    // without pinning excessive memory per protocol instance.
    constexpr size_t STREAM_BUFFER_BYTES = 64 * 1024;

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
        explicit ScopedCapture(const ModelPtr& model) : model_(model) {
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
        ScopedCapture(const ScopedCapture&) = delete;
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
        bool            released_ = false;
    };
} // namespace

NgenSerializationProtocol::NgenSerializationProtocol(const ModelPtr& model, const Properties& properties)
    : frequency(1), check(false), is_fatal(true) {
    payload_buffer_.reserve(INITIAL_PAYLOAD_BUFFER_BYTES);
    initialize(model, properties);
}

NgenSerializationProtocol::NgenSerializationProtocol()
    : frequency(1), check(false), is_fatal(true) {
    payload_buffer_.reserve(INITIAL_PAYLOAD_BUFFER_BYTES);
}

NgenSerializationProtocol::~NgenSerializationProtocol() = default;

auto NgenSerializationProtocol::run(const ModelPtr& model, const Context& ctx) const -> expected<void, ProtocolError> {
    if (model == nullptr) {
        return make_unexpected<ProtocolError>( ProtocolError(
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
    else if (ctx.current_time_step == ctx.total_steps) {
        checkpoint_step = true;
    }
    if (!checkpoint_step) {
        return {};
    }

    // Region (1) in the thread-safety map. Everything below this line
    // touches `out_`, `payload_buffer_`, or the underlying file; a
    // future threaded driver running `run()` concurrently for different
    // feature ids on the same protocol instance would race on all
    // three without this guard.
    std::lock_guard<std::mutex> lock(io_mutex_);

    // Lazy-open the persistent ofstream on first fire.
    // Append mode is retained so restarts that reuse
    // the same path don't clobber earlier records.
    if (!out_) {
        stream_buffer_.resize(STREAM_BUFFER_BYTES);
        auto ofs = std::unique_ptr<std::ofstream>(new std::ofstream);
        ofs->rdbuf()->pubsetbuf(stream_buffer_.data(),
                                static_cast<std::streamsize>(stream_buffer_.size()));
        ofs->open(path, std::ios::binary | std::ios::app);
        if (!ofs->is_open()) {
            const int saved_errno = errno;
            std::stringstream ss;
            ss << "serialization: failed to open checkpoint path '" << path
               << "' for '" << model->GetComponentName()
               << "' at timestep " << ctx.current_time_step
               << " (" << ctx.timestamp << ") for feature id " << ctx.id
               << ": " << std::strerror(saved_errno);
            return make_unexpected<ProtocolError>( ProtocolError(
                is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING,
                ss.str()
            ));
        }
        out_ = std::move(ofs);
    }

    int size = 0;
    try {
        // Region (2) — (create, free) scope is RAII-managed so any throw
        // from GetValue, the archive path, or the file write still
        // releases the model-side capture. See ScopedCapture docstring.
        ScopedCapture capture(model);

        model->GetValue(SERIALIZATION_SIZE_NAME, &size);
        if (size <= 0) {
            // ScopedCapture's destructor will still call FREE on return.
            std::stringstream ss;
            ss << "serialization: non-positive state size (" << size
               << ") reported by '" << model->GetComponentName()
               << "' at timestep " << ctx.current_time_step
               << " (" << ctx.timestamp << ") for feature id " << ctx.id;
            return make_unexpected<ProtocolError>( ProtocolError(
                is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING,
                ss.str()
            ));
        }
        // Reusable per-instance buffer. Each protocol instance is bound
        // to exactly one model (each Bmi_Module_Formulation owns its
        // own NgenBmiProtocols), so the state size seen here is
        // stationary across timesteps for any given instance. After
        // the first growth to the model's state size, subsequent
        // `resize()` calls are no-ops — vector::resize does not
        // reallocate when the new size fits within capacity(). No
        // explicit `if (size > capacity())` guard is needed.
        payload_buffer_.resize(static_cast<size_t>(size));
        model->GetValue(SERIALIZATION_STATE_NAME, payload_buffer_.data());

        // Move the payload into a temporary record to drive the archive,
        // then move it back. Zero copies in the happy path.
        // The move dance preserves the vector's capacity for reuse
        SerializationRecord rec(
            ctx.id,
            ctx.current_time_step,
            parse_timestamp(ctx.timestamp),
            std::move(payload_buffer_)
        );
        write_record(*out_, rec);
        payload_buffer_ = std::move(rec.payload);

        // Flush per record so the file is tailable in real time and a
        // crash between records loses at most the last in-flight record,
        // not a buffer's worth. The persistent-stream optimization still
        // wins us the open/close syscall amortization — flush() triggers
        // a write() but not an open(), and the OS coalesces contiguous
        // writes into disk I/O asynchronously.
        out_->flush();
        if (!*out_) {
            std::stringstream ss;
            ss << "serialization: failed to write record for '" << model->GetComponentName()
               << "' at timestep " << ctx.current_time_step
               << " (" << ctx.timestamp << ") to '" << path << "'";
            return make_unexpected<ProtocolError>( ProtocolError(
                is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING,
                ss.str()
            ));
        }

        // Happy-path FREE: `release()` issues the BMI SetValue itself
        // and propagates any model-side exception. Exception paths fall
        // through to the destructor's best-effort fallback FREE.
        capture.release();
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "serialization: BMI or archive exchange failed for '"
           << model->GetComponentName()
           << "' at timestep " << ctx.current_time_step
           << " (" << ctx.timestamp << ") for feature id " << ctx.id
           << ": " << e.what();
        return make_unexpected<ProtocolError>( ProtocolError(
            is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING,
            ss.str()
        ));
    }

    return {};
}

auto NgenSerializationProtocol::check_support(const ModelPtr& model) -> expected<void, ProtocolError> {
    if (model == nullptr || !model->is_model_initialized()) {
        return make_unexpected<ProtocolError>( ProtocolError(
            Error::UNITIALIZED_MODEL,
            "Cannot check serialization support for uninitialized model. Disabling serialization protocol."
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
            return make_unexpected<ProtocolError>( ProtocolError(
                Error::INTEGRATION_ERROR, ss.str()
            ));
        }
        if (reported != ev.unit) {
            std::stringstream ss;
            ss << "serialization: model '" << model->GetComponentName()
               << "' exposes reserved variable '" << ev.name
               << "' but with unit '" << reported << "'; expected '" << ev.unit << "'.";
            return make_unexpected<ProtocolError>( ProtocolError(
                Error::INTEGRATION_ERROR, ss.str()
            ));
        }
    }
    this->supported = true;
    return {};
}

auto NgenSerializationProtocol::initialize(const ModelPtr& model, const Properties& properties) -> expected<void, ProtocolError> {
    auto top_it = properties.find(SERIALIZATION_CONFIGURATION_KEY);
    if (top_it == properties.end()) {
        // No serialization block at all — stay silent, stay disabled.
        check = false;
        return {};
    }
    geojson::PropertyMap top = top_it->second.get_values();

    // Shared path lives at the top level.
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
        check = true;  // save block present without check -> enable
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
        return error_or_warning( ProtocolError(
            Error::PROTOCOL_WARNING,
            "serialization: 'path' not specified at the top-level serialization block; disabling save protocol."
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
                throw ProtocolError(Error::PROTOCOL_ERROR,
                                    probe.error().get_message());
            }
            // Non-fatal (or non-conformance error of another kind):
            // warn, disable, make run() a no-op.
            error_or_warning(probe.error());
            check = false;
        }
    }
    return {};
}

bool NgenSerializationProtocol::is_supported() const {
    return this->supported;
}

}}} // end namespace models::bmi::protocols
