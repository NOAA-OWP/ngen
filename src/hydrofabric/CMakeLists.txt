add_library(hydrofabric STATIC
        GeoJSONHydrofabricReader.cpp
        HydrofabricReaderFactory.cpp
        )
add_library(NGen::hydrofabric ALIAS hydrofabric)
target_include_directories(hydrofabric PUBLIC
        ${PROJECT_SOURCE_DIR}/include/hydrofabric
        )
target_link_libraries(hydrofabric PUBLIC
        NGen::geojson
        Boost::boost                # Headers-only Boost
        )

# The GeoPackage implementation is the only part of this library that needs SQLite, so it is the
# only part that is conditional. Without it, the factory still builds and still reads GeoJSON
# hydrofabrics; it just refuses GeoPackage paths.
if(NGEN_WITH_SQLITE)
    target_sources(hydrofabric PRIVATE
            GeoPackageHydrofabricSchema.cpp
            GeoPackageHydrofabricReader.cpp
            )
    target_link_libraries(hydrofabric PUBLIC NGen::geopackage)
endif()
