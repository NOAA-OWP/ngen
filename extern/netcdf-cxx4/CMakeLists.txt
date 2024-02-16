###
# Set up headers and sources
###

file(GLOB CXX_HEADERS netcdf-cxx4/cxx4/*.h)
file(GLOB CXX_SOURCES netcdf-cxx4/cxx4/nc*.cpp)

ADD_LIBRARY(netcdf-cxx4 ${CXX_SOURCES})
TARGET_INCLUDE_DIRECTORIES(netcdf-cxx4 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/netcdf-cxx4/cxx4")
TARGET_LINK_LIBRARIES(netcdf-cxx4 NetCDF::C)
