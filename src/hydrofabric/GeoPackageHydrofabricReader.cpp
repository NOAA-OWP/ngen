#include "GeoPackageHydrofabricReader.hpp"
#include "JSONProperty.hpp"

#include <iostream>
#include <set>
#include <stdexcept>
#include <utility>

namespace ngen {
namespace hydrofabric {

namespace {

/**
 * Throw unless @p layer of @p reader carries @p column.
 *
 * @param[in] reader Reader for the GeoPackage holding the layer
 * @param[in] layer Layer to inspect
 * @param[in] column Column the caller's schema version requires
 * @param[in] version Version label to name in the error, e.g. "v4.0"
 * @throws std::runtime_error if the column is absent
 */
void require_column(
    const geopackage::GeoPackageReader& reader,
    const std::string& layer,
    const std::string& column,
    const std::string& version
)
{
    const std::set<std::string> columns = reader.db().columns(layer);
    if (columns.count(column) == 0) {
        throw std::runtime_error(
            version + " " + layer + " layer missing required '" + column + "' column"
        );
    }
}

/**
 * Copy the value of @p from onto @p to, leaving any existing entry alone.
 *
 * @param[in,out] properties Property map to update
 * @param[in] from Property name to read
 * @param[in] to Property name to publish the value under
 */
void alias_property(geojson::PropertyMap& properties, const std::string& from, const std::string& to)
{
    const geojson::PropertyMap::const_iterator source = properties.find(from);
    if (source != properties.end()) {
        properties.emplace(to, geojson::JSONProperty(to, source->second));
    }
}

} // namespace

// ngen::hydrofabric::GeoPackageHydrofabricReader ==============================

GeoPackageHydrofabricReader::GeoPackageHydrofabricReader(
    geopackage::GeoPackageReader divides_reader,
    std::optional<geopackage::GeoPackageReader> nexus_reader,
    const HydrofabricVersion version
)
  : divides_reader_(std::move(divides_reader))
  , nexus_reader_(std::move(nexus_reader))
  , version_(version)
{}

const geopackage::GeoPackageReader& GeoPackageHydrofabricReader::reader_for(const std::string& layer) const
{
    if (layer == "nexus" && nexus_reader_.has_value()) {
        return nexus_reader_.value();
    }
    return divides_reader_;
}

HydrofabricVersion GeoPackageHydrofabricReader::version() const noexcept
{
    return version_;
}

void GeoPackageHydrofabricReader::check_required_columns(const std::string& /* layer */) const
{}

geojson::GeoJSON GeoPackageHydrofabricReader::read_divides(const std::vector<std::string>& ids)
{
    check_required_columns("divides");

    const geopackage::GeoPackageReader& reader = reader_for("divides");
    const std::string id_column = get_layer_id_column(version_, "divides", reader.db());
    geojson::GeoJSON divides = reader.read("divides", ids, id_column);

    normalize_divides(*divides);
    return divides;
}

geojson::GeoJSON GeoPackageHydrofabricReader::read_nexus(const std::vector<std::string>& ids)
{
    check_required_columns("nexus");

    const geopackage::GeoPackageReader& reader = reader_for("nexus");
    const std::string id_column = get_layer_id_column(version_, "nexus", reader.db());
    geojson::GeoJSON nexus = reader.read("nexus", ids, id_column);

    normalize_nexus(*nexus);
    return nexus;
}

// ngen::hydrofabric::V2_2GeoPackageHydrofabricReader ==========================

V2_2GeoPackageHydrofabricReader::V2_2GeoPackageHydrofabricReader(
    geopackage::GeoPackageReader divides_reader,
    std::optional<geopackage::GeoPackageReader> nexus_reader
)
  : GeoPackageHydrofabricReader(
        std::move(divides_reader), std::move(nexus_reader), HydrofabricVersion::V2_2
    )
{}

void V2_2GeoPackageHydrofabricReader::normalize_divides(geojson::FeatureCollection& /* divides */) const
{
    // Nothing to translate: the property names ngen consumes are v2.2's own names, and v2.2 divides
    // carry a native "toid". The one thing that does vary -- whether the id column is "divide_id" or
    // the deprecated "id" -- is settled before the read, by get_layer_id_column().
}

void V2_2GeoPackageHydrofabricReader::normalize_nexus(geojson::FeatureCollection& /* nexus */) const
{
    // As above: v2.2 nexuses already carry "id" and "toid" under those names.
}

// ngen::hydrofabric::AbstractV4GeoPackageHydrofabricReader ============================

AbstractV4GeoPackageHydrofabricReader::AbstractV4GeoPackageHydrofabricReader(
    geopackage::GeoPackageReader divides_reader,
    std::optional<geopackage::GeoPackageReader> nexus_reader,
    const HydrofabricVersion version
)
  : GeoPackageHydrofabricReader(std::move(divides_reader), std::move(nexus_reader), version)
{}

void AbstractV4GeoPackageHydrofabricReader::check_required_columns(const std::string& layer) const
{
    if (layer == "nexus") {
        const geopackage::GeoPackageReader& reader = reader_for(layer);
        require_column(reader, layer, "nexus_id", "v4");
        require_column(reader, layer, "nexus_toid", "v4");
    }
}

void AbstractV4GeoPackageHydrofabricReader::normalize_nexus(geojson::FeatureCollection& nexus) const
{
    for (const geojson::Feature& feature : nexus) {
        if (feature->get_id().empty()) {
            throw std::runtime_error("v4 nexus row has empty 'nexus_id' value");
        }

        // v4 renamed nexus.id/toid -> nexus_id/nexus_toid; downstream consumers still key on
        // "id"/"toid", so alias them (additive).
        geojson::PropertyMap& properties = feature->get_properties();
        alias_property(properties, "nexus_id", "id");
        alias_property(properties, "nexus_toid", "toid");
    }
}

void AbstractV4GeoPackageHydrofabricReader::normalize_divides(geojson::FeatureCollection& divides) const
{
    std::size_t unlinked = 0;
    for (const geojson::Feature& feature : divides) {
        const std::string id = feature->get_id();
        if (id.empty()) {
            throw std::runtime_error("v4 divides row has empty 'divide_id' value");
        }

        attribute_divide_toid(feature->get_properties(), id);

        if (!feature->has_property("toid")) {
            ++unlinked;
        }
    }

    #ifndef NGEN_QUIET
    if (unlinked > 0) {
        std::cout << "WARN: " << unlinked
                  << " divide(s) have no toid (null downstream reference"
                  << " or divides -> flowpaths join miss); treated as"
                  << " terminal." << std::endl;
    }
    #endif
}

// ngen::hydrofabric::V4_0GeoPackageHydrofabricReader ==========================

V4_0GeoPackageHydrofabricReader::V4_0GeoPackageHydrofabricReader(
    geopackage::GeoPackageReader divides_reader,
    std::optional<geopackage::GeoPackageReader> nexus_reader
)
  : AbstractV4GeoPackageHydrofabricReader(
        std::move(divides_reader), std::move(nexus_reader), HydrofabricVersion::V4_0
    )
{}

void V4_0GeoPackageHydrofabricReader::check_required_columns(const std::string& layer) const
{
    AbstractV4GeoPackageHydrofabricReader::check_required_columns(layer);
    if (layer == "divides") {
        require_column(reader_for(layer), layer, "flowpath_toid", "v4.0");
    }
}

void V4_0GeoPackageHydrofabricReader::attribute_divide_toid(
    geojson::PropertyMap& properties,
    const std::string& /* id */
) const
{
    // v4.0 divides carry flowpath_toid natively, aliased straight to "toid". NULL arrives as the
    // placeholder string "null" (see build_properties); treat it as absent (terminal divide).
    const geojson::PropertyMap::const_iterator toid = properties.find("flowpath_toid");
    if (toid != properties.end() && toid->second.as_string() != "null") {
        properties.emplace("toid", geojson::JSONProperty("toid", toid->second));
    }
}

// ngen::hydrofabric::V4_0Beta1GeoPackageHydrofabricReader =====================

V4_0Beta1GeoPackageHydrofabricReader::V4_0Beta1GeoPackageHydrofabricReader(
    geopackage::GeoPackageReader divides_reader,
    std::optional<geopackage::GeoPackageReader> nexus_reader
)
  : AbstractV4GeoPackageHydrofabricReader(
        std::move(divides_reader), std::move(nexus_reader), HydrofabricVersion::V4_0_BETA1
    )
{}

void V4_0Beta1GeoPackageHydrofabricReader::check_required_columns(const std::string& layer) const
{
    AbstractV4GeoPackageHydrofabricReader::check_required_columns(layer);
    if (layer == "divides") {
        require_column(reader_for(layer), layer, "flowpath_id", "v4.0beta1");
    }
}

geojson::GeoJSON V4_0Beta1GeoPackageHydrofabricReader::read_divides(const std::vector<std::string>& ids)
{
    // Resolved once per reader rather than once per divide, and only when divides are actually
    // asked for, since the join costs a full scan of `flowpaths`.
    if (!divide_toid_lookup_built_) {
        divide_toid_lookup_ = build_divide_toid_lookup(
            version(), "divides", reader_for("divides").db()
        );
        divide_toid_lookup_built_ = true;
    }

    return GeoPackageHydrofabricReader::read_divides(ids);
}

void V4_0Beta1GeoPackageHydrofabricReader::attribute_divide_toid(
    geojson::PropertyMap& properties,
    const std::string& id
) const
{
    // No native toid column: synthesize it from the lookup built up front. A miss (null
    // flowpath_id, join miss, or no flowpaths table) leaves "toid" unset, matching v2.2
    // terminal-divide semantics.
    const std::unordered_map<std::string, std::string>::const_iterator toid =
        divide_toid_lookup_.find(id);
    if (toid != divide_toid_lookup_.end()) {
        properties.emplace("toid", geojson::JSONProperty("toid", toid->second));
    }
}

} // namespace hydrofabric
} // namespace ngen
