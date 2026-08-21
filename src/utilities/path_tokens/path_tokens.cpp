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
#include "path_tokens.hpp"

#include <NGenConfig.h>
#if NGEN_WITH_MPI
#include <mpi.h>
#endif

#include <unistd.h>
#include <sys/types.h>

#include <array>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace utilities {

namespace {

    // Scan for `{{token}}` at position `pos` and return the resolved
    // string when `pos..pos+token.size()+4` matches `{{token}}`. Empty
    // return means no match — caller advances by one character.
    bool try_match(const std::string& path, size_t pos,
                   const char* token, size_t& token_end_out) {
        const std::string needle = std::string("{{") + token + "}}";
        if (path.compare(pos, needle.size(), needle) != 0) return false;
        token_end_out = pos + needle.size();
        return true;
    }

    std::string mpi_rank_string() {
#if NGEN_WITH_MPI
        int initialized = 0;
        MPI_Initialized(&initialized);
        if (initialized) {
            int rank = 0;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            return std::to_string(rank);
        }
#endif
        return std::string("0");
    }

    std::string pid_string() {
        return std::to_string(static_cast<long>(::getpid()));
    }

    std::string host_string() {
        std::array<char, 256> buf{};
        if (::gethostname(buf.data(), buf.size() - 1) != 0) {
            return std::string();
        }
        // gethostname may return a FQDN; keep the first label.
        std::string host(buf.data());
        const auto dot = host.find('.');
        if (dot != std::string::npos) host.resize(dot);
        // POSIX caps HOST_NAME_MAX around 255; the 63-char truncation is
        // a pragmatic filename-friendliness bound rather than a POSIX
        // requirement. See header note.
        if (host.size() > 63) host.resize(63);
        return host;
    }

    std::string date_string() {
        using namespace std::chrono;
        const auto now_tt = system_clock::to_time_t(system_clock::now());
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &now_tt);
#else
        localtime_r(&now_tt, &tm);
#endif
        // `std::put_time` with the numeric specifiers `%Y%m%d` is
        // type-safe (no printf format/argument mismatch) and avoids
        // having to size a fixed `char` buffer. The numeric
        // specifiers produce ASCII digits regardless of locale.
        std::ostringstream os;
        os << std::put_time(&tm, "%Y%m%d");
        return os.str();
    }

} // namespace

std::string resolve_path_tokens(const std::string& path) {
    std::string out;
    out.reserve(path.size() + 32);

    size_t i = 0;
    while (i < path.size()) {
        // Cheap prefix filter: only pay the per-token compare when we
        // see the brace pair.
        if (i + 2 <= path.size() && path[i] == '{' && path[i + 1] == '{') {
            size_t end = 0;
            if (try_match(path, i, "rank", end)) {
                out += mpi_rank_string(); i = end; continue;
            }
            if (try_match(path, i, "pid", end)) {
                out += pid_string(); i = end; continue;
            }
            if (try_match(path, i, "host", end)) {
                out += host_string(); i = end; continue;
            }
            if (try_match(path, i, "date", end)) {
                out += date_string(); i = end; continue;
            }
            // Unknown token — leave the braces in place and advance a
            // single character so we don't swallow a typo. See header.
        }
        out.push_back(path[i]);
        ++i;
    }
    return out;
}

} // namespace utilities
