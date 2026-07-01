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
Version constants for the `ngen_serialization` library.

These track the *library API* — the public abstract surface in
`record_backend.hpp`, `record.hpp`, and `id_predicates.hpp`. They
do NOT track on-disk wire formats; concrete backends carry their
own wire-format versions independent of this number.

Versioning policy
-----------------
**Pre-1.0 (the current 0.X range): no API compatibility is
guaranteed.** Any release in the 0.X series may change the public
abstract surface — renames, removed methods, reshaped signatures,
new `BackendError::Kind` values, behavioral-contract changes —
without a major-version bump. The library is in active design;
consumers should pin to an exact 0.X.Y release and expect to
update on each new release. Once the surface settles enough to
promise stability, a 1.0.0 release will lock in the contracts
documented in `record_backend.hpp` and this directory's README.

The 0.X minor/patch numbers are still meaningful as release
markers — a 0.2.0 ships *something* materially different from
0.1.0 — but the distinction between MAJOR / MINOR / PATCH only
becomes a compatibility guarantee at 1.0 and beyond.

Post-1.0 policy (what the numbers will mean once we get there):

  - **MAJOR** — incompatible changes to the public abstract
    surface: renames, removed methods, signature changes on
    virtual methods, or behavioral contract changes that existing
    backend implementers would have to react to.
  - **MINOR** — additive changes: new methods with default
    implementations, new helpers in the predicate kit, new
    `BackendError::Kind` values, new optional sub-handle methods.
    Existing backend implementations continue to compile and run.
  - **PATCH** — non-API changes: documentation, internal
    refactors, test additions, build-system tweaks.

The version is informational. Consumers MAY `static_assert` a
minimum version if they depend on a feature added in a particular
release, but the library doesn't enforce or read the version
itself.
*/
#pragma once

namespace ngen{ namespace serialization{

inline constexpr int LIBRARY_VERSION_MAJOR = 0;
inline constexpr int LIBRARY_VERSION_MINOR = 1;
inline constexpr int LIBRARY_VERSION_PATCH = 0;

/** @brief Human-readable "MAJOR.MINOR.PATCH" string. */
inline constexpr const char* LIBRARY_VERSION_STRING = "0.1.0";

}}  // namespace ngen::serialization
