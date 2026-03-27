# Created by Robert Bartel on 3/26/26.
# Copyright (C) 2026 Lynker
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

add_library(ngen_output
        PerNexusCsvOutputMgr.cpp
)
add_library(NGen::output ALIAS ngen_output)

target_include_directories(ngen_output PUBLIC
        ${PROJECT_SOURCE_DIR}/include/utilities/output
)

if(NGEN_WITH_NETCDF)
    target_compile_definitions(ngen_output PUBLIC NGEN_WITH_NETCDF)
    target_sources(ngen_output PRIVATE "${CMAKE_CURRENT_LIST_DIR}/PerFormulationNexusOutputMgr.cpp")
    target_link_libraries(ngen_output PUBLIC NetCDF)
endif()