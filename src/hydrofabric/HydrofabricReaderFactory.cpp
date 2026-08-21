#include "HydrofabricReaderFactory.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>

#include "GeoJSONHydrofabricReader.hpp"

#if NGEN_WITH_SQLITE3
#include "GeoPackageHydrofabricReader.hpp"
#include "GeoPackageHydrofabricSchema.hpp"
#endif

namespace ngen {
namespace hydrofabric {

namespace {

//! Every SQLite database begins with this, trailing NUL included.
const char SQLITE_MAGIC[] = "SQLite format 3";

/**
 * Whether @p path holds a SQLite database, which for a hydrofabric means a GeoPackage.
 *
 * Detect whether this is a SQLite database file by reading the beginning of the file, rather than
 * simply reading the file name.  This probably means, if we assume it is a hydrofabric, that it
 * is a GeoPackage hydrofabric (versus something else like a GeoJson hydrofabric), though specific
 * validity checks of that nature are the responsibility of something else.
 *
 * @param[in] path Hydrofabric data path
 * @return true if the file begins with the SQLite header
 * @throws std::runtime_error if the file cannot be opened for reading
 */
bool is_sqlite_file(const std::string& path)
{
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        throw std::runtime_error("cannot open hydrofabric data file " + path);
    }

    char header[sizeof(SQLITE_MAGIC)];
    file.read(header, sizeof(header));
    if (file.gcount() < static_cast<std::streamsize>(sizeof(header))) {
        // Too short to be a SQLite database, whatever else it may be.
        return false;
    }

    return std::memcmp(header, SQLITE_MAGIC, sizeof(SQLITE_MAGIC)) == 0;
}

/**
 * The result of identifying a hydrofabric: which one it is, and any database opened to find out.
 *
 * Structure to maintain handles for opening a database as needed to identify it as a hydrofabric
 * of a particular schema, so that later reads of the database can reuse these handles.
 */
struct Identified
{
    HydrofabricVersion version = HydrofabricVersion::UNRECOGNIZED;

    #if NGEN_WITH_SQLITE3
    //! Engaged only for a GeoPackage hydrofabric.
    std::optional<geopackage::GeoPackageReader> divides_reader;
    //! Engaged only when the nexuses live in a second file.
    std::optional<geopackage::GeoPackageReader> nexus_reader;
    #endif
};

/**
 * Identify the hydrofabric @p catchment_path and @p nexus_path name.
 *
 * @param[in] catchment_path Path to the file holding the divides
 * @param[in] nexus_path Path to the file holding the nexuses; may be the same file
 * @return Identified The release identified, plus any GeoPackages opened to identify it
 * @throws std::runtime_error if the paths name different formats, if a GeoPackage cannot be
 *         opened or matches no known release, or if the paths name GeoPackages and GeoPackage
 *         support was not built in
 */
Identified identify(const std::string& catchment_path, const std::string& nexus_path)
{
    // One path naming both roles is the common case, and a file is trivially in the same format as
    // itself, so the second look is only worth taking when there is a second file.
    const bool catchment_is_gpkg = is_sqlite_file(catchment_path);
    if (nexus_path != catchment_path && catchment_is_gpkg != is_sqlite_file(nexus_path)) {
        throw std::runtime_error(
            "a hydrofabric must be in one format, but catchment data " + catchment_path +
            " and nexus data " + nexus_path + " are not"
        );
    }

    Identified identified;

    // GeoJSON carries no release marker, so the path is the whole answer and nothing is opened.
    if (!catchment_is_gpkg) {
        identified.version = HydrofabricVersion::V1_GEOJSON;
        return identified;
    }

    #if NGEN_WITH_SQLITE3
    identified.divides_reader.emplace(geopackage::make_reader(catchment_path));

    // One file is the common case, and opening it twice would buy nothing but a second handle.
    if (nexus_path != catchment_path) {
        identified.nexus_reader.emplace(geopackage::make_reader(nexus_path));
    }

    const geopackage::GeoPackageReader& nexus_source =
        identified.nexus_reader.has_value() ? identified.nexus_reader.value()
                                            : identified.divides_reader.value();

    identified.version = detect_hydrofabric(
        nexus_source.db(), identified.divides_reader.value().db()
    );

    return identified;
    #else
    throw std::runtime_error("SQLite3 support required to read GeoPackage files.");
    #endif
}

#if NGEN_WITH_SQLITE3

/**
 * Build the reader for a GeoPackage hydrofabric of @p version, over already-open GeoPackages.
 *
 * The databases arrive open because identifying the release required opening them. Handing those
 * same handles on saves opening the files again, and means the release this reader is built for is
 * certain to be the release of the database it goes on to read, rather than of one that was closed
 * and reopened in between.
 *
 * @param[in] divides_reader Reader for the GeoPackage holding the `divides` layer
 * @param[in] nexus_reader Reader for the `nexus` layer's GeoPackage, if a separate file
 * @param[in] version Release identified for the pair
 * @return std::unique_ptr<HydrofabricReader> Reader for that release
 * @throws std::runtime_error if @p version names no readable GeoPackage release
 */
std::unique_ptr<HydrofabricReader> make_geopackage_reader(
    geopackage::GeoPackageReader divides_reader,
    std::optional<geopackage::GeoPackageReader> nexus_reader,
    const HydrofabricVersion version
)
{
    // Deliberately without a default case, so that adding a release without adding its reader here
    // is a compiler warning rather than a surprise.
    switch (version) {
        case HydrofabricVersion::V4_0:
            return std::make_unique<V4_0GeoPackageHydrofabricReader>(
                std::move(divides_reader), std::move(nexus_reader)
            );
        case HydrofabricVersion::V4_0_BETA1:
            return std::make_unique<V4_0Beta1GeoPackageHydrofabricReader>(
                std::move(divides_reader), std::move(nexus_reader)
            );
        case HydrofabricVersion::V2_2:
            return std::make_unique<V2_2GeoPackageHydrofabricReader>(
                std::move(divides_reader), std::move(nexus_reader)
            );
        case HydrofabricVersion::V1_GEOJSON:
        case HydrofabricVersion::UNRECOGNIZED:
            break;
    }

    throw std::runtime_error(
        std::string("cannot read a GeoPackage hydrofabric identified as ") + version_label(version)
    );
}

#endif // NGEN_WITH_SQLITE3

} // namespace

HydrofabricVersion detect_hydrofabric(const std::string& catchment_path, const std::string& nexus_path)
{
    // Anything identify() opened closes with the value it returned, which is what a caller asking
    // only for the answer wants. make_hydrofabric_reader() keeps those handles instead.
    return identify(catchment_path, nexus_path).version;
}

std::unique_ptr<HydrofabricReader> make_hydrofabric_reader(
    const std::string& catchment_path,
    const std::string& nexus_path
)
{
    Identified identified = identify(catchment_path, nexus_path);

    #ifndef NGEN_QUIET
    if (identified.version != HydrofabricVersion::UNRECOGNIZED) {
        std::cout << "INFO: hydrofabric detected: " << version_label(identified.version) << std::endl;
    }
    #endif

    switch (identified.version) {
        case HydrofabricVersion::V1_GEOJSON:
            return std::make_unique<GeoJSONHydrofabricReader>(catchment_path, nexus_path);
        case HydrofabricVersion::V2_2:
        case HydrofabricVersion::V4_0_BETA1:
        case HydrofabricVersion::V4_0:
            #if NGEN_WITH_SQLITE3
            return make_geopackage_reader(
                std::move(identified.divides_reader.value()),
                std::move(identified.nexus_reader),
                identified.version
            );
            #else
            // Unreachable: detect_hydrofabric() refuses GeoPackage paths in this build.
            throw std::runtime_error("SQLite3 support required to read GeoPackage files.");
            #endif
        case HydrofabricVersion::UNRECOGNIZED:
            break;
    }

    throw std::runtime_error(
        "GeoPackage " + nexus_path + " is not a hydrofabric: it has no `nexus` layer"
    );
}

std::unique_ptr<HydrofabricReader> make_hydrofabric_reader(const std::string& path)
{
    return make_hydrofabric_reader(path, path);
}

} // namespace hydrofabric
} // namespace ngen
