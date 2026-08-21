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

**Pre-1.0 (the current 0.X range): no API compatibility is
guaranteed.** Any release in the 0.X series may change the public
abstract surface without a major-version bump. Consumers should
pin to an exact 0.X.Y release and expect to update on each new
release. A 1.0.0 will lock in the surface as it stands then, and
MAJOR / MINOR / PATCH will carry their conventional SemVer
meanings from that point on. See this directory's README for the
policy.

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
