#ifndef NGEN_GEOPACKAGE_READER_H
#define NGEN_GEOPACKAGE_READER_H

#include <optional>
#include <string>
#include <vector>

#include "FeatureCollection.hpp"
#include "ngen_sqlite.hpp"

namespace ngen {
namespace geopackage {

/**
 * Reads feature collections out of the layers of a single GeoPackage.
 *
 * This class knows nothing about hydrofabrics or their schema versions, and nothing about any other
 * convention a particular GeoPackage might follow: it implements the generic "turn a GPKG table into
 * a FeatureCollection" algorithm and nothing else. Everything that varies between schemas is either
 * a parameter of read() or the caller's business afterward, so a caller with schema knowledge
 * composes this reader rather than specializing it.
 *
 * A reader owns its database connection for its lifetime, so a caller reading several layers of one
 * file constructs a single reader and calls read() once per layer.
 */
class GeoPackageReader
{
  public:
    /**
     * Take ownership of an open database.
     *
     * @param[in] db Open GeoPackage database to read from
     */
    explicit GeoPackageReader(sqlite::database db);

    // A reader owns a database connection, which is itself non-copyable but movable.
    GeoPackageReader(const GeoPackageReader&)            = delete;
    GeoPackageReader& operator=(const GeoPackageReader&) = delete;
    GeoPackageReader(GeoPackageReader&&)                 = default;
    GeoPackageReader& operator=(GeoPackageReader&&)      = default;

    /**
     * Build a feature collection from one layer of this GeoPackage.
     *
     * @param[in] layer Layer (table) name within the GeoPackage
     * @param[in] ids Optional subset of feature IDs to capture; if empty, the entire layer is
     *            converted. IDs absent from the layer are silently ignored, so the result is the
     *            intersection of the layer and @p ids.
     * @param[in] id_column Column to read each feature's id from, which is also the column @p ids
     *            is matched against. Defaults to the conventional "id"; a caller that knows the
     *            layer names it otherwise passes that name here.
     * @param[in] columns Columns to read besides @p id_column, each of which becomes a property,
     *            or no value to read every column. Geometry is decoded only when the layer's
     *            geometry column is read; otherwise each feature is an empty geometry collection
     *            with an empty bounding box, as a GeoJSON feature with no geometry is read, and the
     *            collection's bounding box is empty too.
     * @return std::shared_ptr<geojson::FeatureCollection> Collection of the layer's features, with
     *         a bounding box covering all of them
     * @throws std::runtime_error if @p layer is not a queryable table name or does not exist in
     *         this GeoPackage, or if it lacks any of @p columns
     */
    std::shared_ptr<geojson::FeatureCollection> read(
        const std::string& layer,
        const std::vector<std::string>& ids = {},
        const std::string& id_column = "id",
        const std::optional<std::vector<std::string>>& columns = std::nullopt
    ) const;

    /**
     * The database being read.
     *
     * @return Const reference to this reader's open database
     */
    const sqlite::database& db() const noexcept;

  private:
    sqlite::database db_;
};

/**
 * Open @p gpkg_path for generic reading.
 *
 * @param[in] gpkg_path Path to the GeoPackage file to read
 * @return GeoPackageReader Reader owning the opened database
 * @throws ngen::sqlite::sqlite_error if the file cannot be opened
 */
GeoPackageReader make_reader(const std::string& gpkg_path);

} // namespace geopackage
} // namespace ngen

#endif // NGEN_GEOPACKAGE_READER_H
