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
Application-defined predicates over SerializationRecord ids.

The serialization library treats `SerializationRecord::id` as an
opaque string — the producer composes it according to whatever
identity convention the application uses, and the consumer parses
it the same way. When a Reader is opened with an `IdPredicate`,
the library invokes the closure on each candidate record's id
during the index walk to decide eligibility. The library never
parses, splits, or interprets the id itself; the closure is the
only place where id semantics get applied.

This header provides the calling-convention type alias and a
small kit of stock helpers + combinators for common id-composition
schemes. Applications may freely supply their own closures —
the library calls them as opaque truth.

Closure contract
----------------
- *Pure-ish.* Called once per record during the index walk; no
  order-sensitive side effects beyond logging / telemetry.
- *Cheap-ish.* On the hot index-build path; a slow predicate
  slows the walk linearly in record count.
- *Stable for the Reader's lifetime.* The library may memoize
  the matched-id set once the walk completes — don't mutate
  captures in ways that would retroactively change verdicts.
*/
#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace ngen{ namespace serialization{

/** @brief Eligibility predicate over a SerializationRecord's id
 *  string. The library invokes this once per record during the
 *  index walk and treats the returned bool as authoritative. */
using IdPredicate = std::function<bool(const std::string& id)>;

// ----- Stock helpers ------------------------------------------------

/** Match: the id equals @p target exactly. The default common
 *  case for restore-by-known-id. */
inline IdPredicate exact_id(std::string target) {
    return [t = std::move(target)](const std::string& id) {
        return id == t;
    };
}

/** Match: the first `:`/`.`-delimited segment of the id equals
 *  @p prefix. Encodes the ngen v0.2 composition convention where
 *  `"cat-1"` matches both `"cat-1:cfe"` and `"cat-1.0:cfe:multi"`. */
inline IdPredicate primary_prefix(std::string prefix) {
    return [p = std::move(prefix)](const std::string& id) {
        const auto d = id.find_first_of(":.");
        const auto head = (d == std::string::npos) ? id : id.substr(0, d);
        return head == p;
    };
}

/** Match: @p segment appears as any `:`/`.`-delimited token in
 *  the id. Exact-token match — `"key_1"` does not match
 *  `"key_10"`. */
inline IdPredicate contains_segment(std::string segment) {
    return [s = std::move(segment)](const std::string& id) {
        std::size_t start = 0;
        while (start <= id.size()) {
            const auto d   = id.find_first_of(":.", start);
            const auto end = (d == std::string::npos) ? id.size() : d;
            if (id.compare(start, end - start, s) == 0) return true;
            if (d == std::string::npos) break;
            start = d + 1;
        }
        return false;
    };
}

/** Match: the @p n'th (zero-indexed) `:`/`.`-delimited segment of
 *  the id equals @p value. Returns false if the id has fewer than
 *  @p n + 1 segments. */
inline IdPredicate at_position(std::size_t n, std::string value) {
    return [n, v = std::move(value)](const std::string& id) {
        std::size_t start = 0;
        std::size_t seg   = 0;
        while (start <= id.size()) {
            const auto d   = id.find_first_of(":.", start);
            const auto end = (d == std::string::npos) ? id.size() : d;
            if (seg == n) {
                return id.compare(start, end - start, v) == 0;
            }
            if (d == std::string::npos) return false;
            start = d + 1;
            ++seg;
        }
        return false;
    };
}

/** Match: the id begins with the substring @p prefix. Escape
 *  hatch for ids that are not segment-structured. */
inline IdPredicate starts_with(std::string prefix) {
    return [p = std::move(prefix)](const std::string& id) {
        return id.size() >= p.size() &&
               id.compare(0, p.size(), p) == 0;
    };
}

// ----- Combinators --------------------------------------------------

/** All sub-predicates must be true. Short-circuits on the first
 *  false. */
inline IdPredicate all_of(std::vector<IdPredicate> preds) {
    return [p = std::move(preds)](const std::string& id) {
        for (const auto& f : p) if (!f(id)) return false;
        return true;
    };
}

/** Any sub-predicate true. Short-circuits on the first true. */
inline IdPredicate any_of(std::vector<IdPredicate> preds) {
    return [p = std::move(preds)](const std::string& id) {
        for (const auto& f : p) if (f(id)) return true;
        return false;
    };
}

/** Inverse of @p p. Named after `std::not_fn` from `<functional>`. */
inline IdPredicate not_fn(IdPredicate p) {
    return [inner = std::move(p)](const std::string& id) {
        return !inner(id);
    };
}

}}  // namespace ngen::serialization
