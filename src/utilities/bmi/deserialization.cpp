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
Version 0.2 (see deserialization.hpp)
*/

#include "deserialization.hpp"
#include "file_backend.hpp"
#include "serialization.hpp" // reserved variable name constants + keys
#include "serialization_record.hpp" // re-exports parse_timestamp into this namespace
#include "utilities/serialization/id_predicates.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace models {
namespace bmi {
namespace protocols {

namespace {

// Translate the v0.1 `id_subset` (vector of primary-identity strings)
// into the v0.2 IdPredicate the backend's reader() expects. An empty
// subset becomes the default-constructed empty predicate, meaning
// "no scope" — find any record. A non-empty subset becomes
// `any_of({primary_prefix(s) for s in subset})` to preserve the v0.1
// behavior where the subset matched on primary-identity prefix.
::ngen::serialization::IdPredicate make_scope(const std::vector<std::string>& id_subset) {
    if (id_subset.empty()) return {};

    std::vector<::ngen::serialization::IdPredicate> per_id;
    per_id.reserve(id_subset.size());
    for (const auto& s : id_subset) {
        per_id.push_back(::ngen::serialization::primary_prefix(s));
    }
    return ::ngen::serialization::any_of(std::move(per_id));
}

} // namespace

NgenDeserializationProtocol::NgenDeserializationProtocol(
    const ModelPtr& model,
    const Properties& properties
)
    : check(false)
    , is_fatal(true) {
    (void)initialize(model, properties);
}

NgenDeserializationProtocol::NgenDeserializationProtocol()
    : check(false)
    , is_fatal(true) {
}

NgenDeserializationProtocol::~NgenDeserializationProtocol() = default;

auto NgenDeserializationProtocol::run(const ModelPtr& model, const Context& ctx) const
    -> expected<void, ProtocolError> {
    if (model == nullptr) {
        return make_unexpected<ProtocolError>(ProtocolError(
            Error::UNITIALIZED_MODEL,
            "Cannot run deserialization protocol with null model."
        ));
    }
    if (!check || !supported) {
        return {};
    }

    try {
        // Open a Reader scoped to exactly THIS feature's id. Two
        // layers of filtering apply:
        //   - Construction scope (on the FileBackend): bounds the
        //     index to the realization's id_subset. Set once at
        //     backend create-time.
        //   - Read scope (on this Reader): bounds THIS lookup to
        //     the one feature id the engine handed us via ctx.id.
        //     Per-run, single-feature. A Reader scoped this way
        //     cannot return any other id's record even if
        //     accidentally asked.
        // The Reader is short-lived: open, do one find_*, drop.
        auto ro = backend_->reader(::ngen::serialization::exact_id(ctx.id));
        if (!ro) {
            std::stringstream ss;
            ss << "deserialization: " << ro.error().message << " for feature id " << ctx.id;
            return make_unexpected<ProtocolError>(
                ProtocolError(is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING, ss.str())
            );
        }
        auto& reader = *ro.value();

        // Dispatch on match mode.
        ::nonstd::expected<::ngen::serialization::Record, ::ngen::serialization::BackendError>
            loaded;
        if (by_timestamp) {
            loaded = reader.find_at_simulation_timestamp(ctx.id, target_simulation_timestamp);
        } else if (step_latest) {
            loaded = reader.find_latest(ctx.id);
        } else {
            loaded = reader.find_at_step(ctx.id, target_step);
        }

        if (!loaded) {
            // NotFound is the typical case (no matching record);
            // Corrupted and IOError fall through to the same
            // protocol-level error path. The backend's message
            // already includes the lookup criteria and path.
            std::stringstream ss;
            ss << "deserialization: " << loaded.error().message;
            return make_unexpected<ProtocolError>(
                ProtocolError(is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING, ss.str())
            );
        }

        // Feed the payload back through the reserved SetValue name; the model
        // owns the exact inverse of its save() machinery.
        model->SetValue(SERIALIZATION_STATE_NAME, loaded.value().payload.data());
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "deserialization: BMI or archive exchange failed for '" << model->GetComponentName()
           << "' at timestep " << ctx.current_time_step << " (" << ctx.timestamp
           << ") for feature id " << ctx.id << ": " << e.what();
        return make_unexpected<ProtocolError>(
            ProtocolError(is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING, ss.str())
        );
    }

    return {};
}

auto NgenDeserializationProtocol::check_support(const ModelPtr& model)
    -> expected<void, ProtocolError> {
    if (model == nullptr || !model->is_model_initialized()) {
        return make_unexpected<ProtocolError>(ProtocolError(
            Error::UNITIALIZED_MODEL,
            "Cannot check deserialization support for uninitialized model. Disabling "
            "deserialization protocol."
        ));
    }

    // Metadata-only probe, same policy as NgenSerializationProtocol: verify
    // each reserved variable's name is recognized AND that the reported unit
    // string matches the protocol convention exactly. No SetValue or
    // GetValue(state) is performed during support detection — the engine
    // owns model state mutation. The restore-specific SetValue(state, ...)
    // cannot be safely probed, and doing so at init time would effectively
    // pre-execute run() on whatever record check_support's companion save
    // left behind; the trade-off is that a model which registers the names
    // with the right units but has a broken SetValue(state) implementation
    // will only fail at the first real run() call.
    // The reserved-variable table is defined in serialization.hpp and
    // shared with the save protocol so the two agree on the exact set
    // of variables a conforming model must expose.
    for (const auto& ev : RESERVED_VARS) {
        std::string reported;
        try {
            reported = model->GetVarUnits(ev.name);
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << "deserialization: model '" << model->GetComponentName()
               << "' does not expose reserved variable '" << ev.name
               << "' (GetVarUnits failed: " << e.what() << ").";
            return make_unexpected<ProtocolError>(
                ProtocolError(Error::INTEGRATION_ERROR, ss.str())
            );
        }
        if (reported != ev.unit) {
            std::stringstream ss;
            ss << "deserialization: model '" << model->GetComponentName()
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

auto NgenDeserializationProtocol::initialize(const ModelPtr& model, const Properties& properties)
    -> expected<void, ProtocolError> {
    auto top_it = properties.find(SERIALIZATION_CONFIGURATION_KEY);
    if (top_it == properties.end()) {
        check = false;
        return {};
    }
    geojson::PropertyMap top = top_it->second.get_values();

    // The path is local-only: handed to the backend factory below
    // and owned by the backend thereafter. The protocol does not
    // need its own copy.
    std::string path;
    auto path_it = top.find(SERIALIZATION_PATH_KEY);
    if (path_it != top.end()) {
        path = path_it->second.as_string();
    }

    auto restore_it = top.find(DESERIALIZATION_RESTORE_KEY);
    if (restore_it == top.end()) {
        check = false;
        return {};
    }
    geojson::PropertyMap restore = restore_it->second.get_values();

    auto _it = restore.find(SERIALIZATION_CHECK_KEY);
    if (_it != restore.end()) {
        check = _it->second.as_boolean();
    } else {
        check = true;
    }

    _it = restore.find(SERIALIZATION_FATAL_KEY);
    if (_it != restore.end()) {
        is_fatal = _it->second.as_boolean();
    }

    // Optional id_subset: scope the per-Reader index to a known set
    // of ids. Callers that omit this key pay the full-file index cost
    // on every restore but don't have to enumerate their features up
    // front; callers that supply it get a smaller in-memory index
    // proportional to the subset.
    //
    // We translate the parsed strings into the backend's IdPredicate
    // here, once, and store the resulting predicate as `id_scope`.
    // The predicate captures the necessary strings internally (one
    // `primary_prefix(s)` per entry, composed via `any_of`), so the
    // raw vector is not kept around.
    auto subset_it = restore.find(DESERIALIZATION_ID_SUBSET_KEY);
    if (subset_it != restore.end()) {
        try {
            std::vector<std::string> entries_str;
            auto entries = subset_it->second.as_list();
            entries_str.reserve(entries.size());
            for (const auto& e : entries) {
                entries_str.push_back(e.as_string());
            }
            id_scope = make_scope(entries_str);
        } catch (const std::exception&) {
            // A malformed id_subset is non-fatal — fall through to
            // "index everything" semantics. The property tree layer
            // throws on type mismatch; we'd rather surface that as a
            // warning than abort restore entirely.
            id_scope = {};
        }
    }

    // Match-mode precedence: `timestamp` beats `step` if both are specified.
    // If only `step` is set, use step-based matching. If neither, fall back to
    // "latest" step for this id.
    auto ts_it = restore.find(DESERIALIZATION_TIMESTAMP_KEY);
    if (ts_it != restore.end()) {
        const std::string ts_str = ts_it->second.as_string();
        const int64_t     parsed = parse_timestamp(ts_str);
        if (parsed == ::ngen::serialization::UNPARSEABLE_TIMESTAMP_SENTINEL) {
            // User asked for restore-by-timestamp with a string we can't
            // parse — refuse to enable restore rather than silently
            // looking up a meaningless `simulation_timestamp` value.
            check = false;
            return error_or_warning(ProtocolError(
                is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING,
                "deserialization: 'timestamp' value '" + ts_str
                    + "' is not parseable; disabling restore. See record.hpp::parse_timestamp "
                      "for accepted forms."
            ));
        }
        by_timestamp                = true;
        target_simulation_timestamp = parsed;
    } else {
        by_timestamp = false;
        _it          = restore.find(DESERIALIZATION_STEP_KEY);
        if (_it != restore.end()) {
            // The property layer reports the underlying JSON type; accept either
            // a string (expected value: "latest") or an integer.
            try {
                std::string s = _it->second.as_string();
                if (s == DESERIALIZATION_STEP_LATEST) {
                    step_latest = true;
                } else {
                    // Numeric string fallback (e.g. "3").
                    step_latest = false;
                    target_step = std::stoi(s);
                }
            } catch (const std::exception&) {
                step_latest = false;
                target_step = static_cast<int>(_it->second.as_natural_number());
            }
        } else {
            step_latest = true;
        }
    }

    if (check && path.empty()) {
        check = false;
        return error_or_warning(ProtocolError(
            Error::PROTOCOL_WARNING,
            "deserialization: 'path' not specified at the top-level serialization block; disabling "
            "restore protocol."
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
    // Default backend is FileBackend. Pass `id_scope` as the
    // construction-time scope so the FIRST protocol that creates
    // the path-keyed backend bounds the index to the configs
    // intended subset. Subsequent protocols on the same path
    // receive the cached backend (the scope arg they pass is
    // ignored at the cache hit.
    auto be = ::ngen::serialization::FileBackend::create(path, id_scope);
    if (!be) {
        check = false;
        std::stringstream ss;
        ss << "deserialization: failed to construct backend at '" << path
           << "': " << be.error().message;
        return error_or_warning(
            ProtocolError(is_fatal ? Error::PROTOCOL_ERROR : Error::PROTOCOL_WARNING, ss.str())
        );
    }
    backend_ = std::move(be.value());
    return {};
}

bool NgenDeserializationProtocol::is_supported() const {
    return this->supported;
}

} // namespace protocols
} // namespace bmi
} // namespace models
