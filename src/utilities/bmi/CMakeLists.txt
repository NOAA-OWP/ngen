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
# Library Version 0.2
# BMI protocols library — mass balance checking, state serialization,
# state deserialization.

add_library(ngen_bmi_protocols protocols.cpp mass_balance.cpp serialization.cpp deserialization.cpp)
add_library(NGen::bmi_protocols ALIAS ngen_bmi_protocols)

target_include_directories(ngen_bmi_protocols PUBLIC
    ${PROJECT_SOURCE_DIR}/include/bmi
    ${PROJECT_SOURCE_DIR}/include/utilities/bmi
    ${PROJECT_SOURCE_DIR}/include/geojson
    ${NGEN_INC_DIR}
)

# Internal-only headers (byte_io.hpp, wire_format.hpp, etc.) live next to
# the .cpp files in src/utilities/bmi/ and are NOT exported to consumers —
# they encode wire-layout primitives that external callers should not
# bypass the record layer to use. The library's own translation units see
# them via this PRIVATE include path. The test_bmi_protocols target opts
# in via the same path to exercise the internals directly (see
# test/CMakeLists.txt).
target_include_directories(ngen_bmi_protocols PRIVATE
    ${PROJECT_SOURCE_DIR}/src/utilities/bmi
)

target_link_libraries(ngen_bmi_protocols
  PUBLIC
    ${CMAKE_DL_LIBS}
    Boost::boost           # Headers-only Boost (for boost::optional, etc.)
)

target_sources(ngen_bmi_protocols
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/bmi/Bmi_Adapter.cpp"
)
