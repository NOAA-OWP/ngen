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
# BMI mass balance checking protocol

add_library(ngen_bmi_protocols protocols.cpp mass_balance.cpp)
add_library(NGen::bmi_protocols ALIAS ngen_bmi_protocols)

target_include_directories(ngen_bmi_protocols PUBLIC 
    ${PROJECT_SOURCE_DIR}/include/bmi
    ${PROJECT_SOURCE_DIR}/include/utilities/bmi
    ${PROJECT_SOURCE_DIR}/include/geojson
    ${NGEN_INC_DIR}
)

target_link_libraries(ngen_bmi_protocols
  PUBLIC
    ${CMAKE_DL_LIBS}
    Boost::boost # Headers-only Boost
)

target_sources(ngen_bmi_protocols
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/bmi/Bmi_Adapter.cpp"
)
