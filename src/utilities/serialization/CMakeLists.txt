# Author: Nels Frazier
# Copyright (C) 2025 Lynker
# ------------------------------------------------------------------------
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ------------------------------------------------------------------------
# Library Version 0.1
# Engine-agnostic serialization abstractions: the Record value type,
# the RecordBackend interface (with nested Writer/Reader sub-handles),
# the BackendError taxonomy, and the IdPredicate kit. The BMI protocols
# library is the first consumer and ships its own built-in file
# backend; future HDF5 / S3 / etc. backends will live in their own
# targets that depend on this one.
#
# Header-only for now (INTERFACE library); .cpp sources land when
# concrete utilities (e.g. backend registry) need them.
#
# The library API version is also exposed at compile time via
# constants in `include/utilities/serialization/version.hpp` —
# consumers that need to feature-detect can `static_assert` against
# them. See that header for the versioning policy.

add_library(ngen_serialization INTERFACE)
add_library(NGen::serialization ALIAS ngen_serialization)

target_link_libraries(ngen_serialization INTERFACE
    # Headers-only Boost.Core for `boost::span`
    Boost::boost
)

target_include_directories(ngen_serialization INTERFACE
    ${PROJECT_SOURCE_DIR}/include
    # nonstd::expected currently lives under include/utilities/bmi/nonstd/.
    # It is shared infrastructure, not BMI-specific; expose its parent
    # directory here so `<nonstd/expected.hpp>` resolves for any consumer
    # of ngen_serialization. (A future cleanup will relocate the header
    # to a top-level include/nonstd/ and drop this line.)
    ${PROJECT_SOURCE_DIR}/include/utilities/bmi
)
