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
Path-template token resolver.

The BMI serialization protocol is intentionally MPI-agnostic: the
protocol itself knows nothing about ranks, hosts, or processes, and
will happily write multiple instances to the same file without
coordination. When a caller *does* want per-rank (or per-pid, or
per-host, or per-date) file naming to sidestep coordination, they
bake the discriminator into the configured path via these tokens and
let the engine/realization layer resolve them before handing the
path to the protocol.

Recognized tokens (double-braced, exactly — no whitespace inside):
    {{rank}}   MPI rank (0 for non-MPI or uninitialized), decimal.
    {{pid}}    POSIX process id, decimal.
    {{host}}   Host name (first label, truncated at 63 chars; empty if
               lookup fails).
    {{date}}   Calendar date at call time formatted YYYYMMDD (local
               time; not suitable as a cryptographic nonce).

Unknown tokens — anything else matching `{{...}}` — are left intact so
that a future addition doesn't silently swallow a typo. The caller is
expected to validate the final string however they see fit.

Keep this header free of MPI includes (guarded behind NGEN_WITH_MPI in
the .cpp). Callers include the header; link pulls in whichever MPI
state is available at build time.
*/
#pragma once

#include <string>

namespace utilities {

/** @brief Expand `{{rank}}`, `{{pid}}`, `{{host}}`, `{{date}}` tokens in
 *         @p path and return the resolved string.
 *
 * Safe to call from anywhere in the ngen lifecycle. The `{{rank}}`
 * token resolves to 0 when ngen was built without MPI or when MPI
 * has not been initialized yet at the call site.
 */
std::string resolve_path_tokens(const std::string& path);

} // namespace utilities
