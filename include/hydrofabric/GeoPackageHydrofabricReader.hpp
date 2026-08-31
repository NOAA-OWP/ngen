#ifndef NGEN_GEOPACKAGE_HYDROFABRIC_READER_H
#define NGEN_GEOPACKAGE_HYDROFABRIC_READER_H

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "GeoPackageReader.hpp"
#include "HydrofabricReader.hpp"
#include "GeoPackageHydrofabricSchema.hpp"

namespace ngen {
namespace hydrofabric {

/**
 * Reads a hydrofabric stored as one or two GeoPackages.
 *
 * A GeoPackage hydrofabric keeps its divides and nexuses in named layers, so this reader owns
 * generic ngen::geopackage::GeoPackageReader instances and mediates between them and the two
 * hydrofabric roles: it resolves which column carries each layer's id, checks that the columns its
 * schema version requires are present, and translates what the file names into what the rest of
 * ngen expects. The generic readers stay unaware of all of it.
 *
 * The two roles may share one GeoPackage or come from two, which is why the second reader is
 * optional rather than a second mandatory handle: absent means "the same file the divides came
 * from", and the file is opened once.
 *
 * Schema versions differ only in that translation, so this class is abstract and each supported
 * version is a subclass. Instances come from make_hydrofabric_reader(), which detects the version.
 */
class GeoPackageHydrofabricReader : public HydrofabricReader
{
  public:
    geojson::GeoJSON read_divides(const std::vector<std::string>& ids = {}) override;

    geojson::GeoJSON read_nexus(const std::vector<std::string>& ids = {}) override;

  protected:
    /**
     * @param[in] divides_reader Reader for the GeoPackage holding the `divides` layer
     * @param[in] nexus_reader Reader for the GeoPackage holding the `nexus` layer, or no value when
     *            that layer lives in the same file as the divides
     * @param[in] version Hydrofabric schema version detected for this hydrofabric
     */
    GeoPackageHydrofabricReader(
        geopackage::GeoPackageReader divides_reader,
        std::optional<geopackage::GeoPackageReader> nexus_reader,
        HydrofabricVersion version
    );

    /**
     * The reader for the file holding @p layer.
     *
     * @param[in] layer Hydrofabric layer name, "divides" or "nexus"
     * @return Const reference to the reader that layer is read through
     */
    const geopackage::GeoPackageReader& reader_for(const std::string& layer) const;

    /**
     * The detected schema version, for the version-keyed schema utilities.
     *
     * @return Hydrofabric version this reader was constructed for
     */
    HydrofabricVersion version() const noexcept;

    /**
     * Verify up front that @p layer carries the columns this version needs.
     *
     * Checking before the read means a file missing a required column is reported as the schema
     * problem it is, rather than as a failure to find a column mid-row. The default checks nothing.
     *
     * @param[in] layer Hydrofabric layer about to be read
     * @throws std::runtime_error if a required column is absent
     */
    virtual void check_required_columns(const std::string& layer) const;

    /**
     * Translate a freshly read divides collection into the property names ngen consumes.
     *
     * @param[in,out] divides Divides collection to update in place
     * @throws std::runtime_error if a row cannot be translated
     */
    virtual void fixup_divides(geojson::FeatureCollection& divides) const = 0;

    /**
     * Translate a freshly read nexus collection into the property names ngen consumes.
     *
     * @param[in,out] nexus Nexus collection to update in place
     * @throws std::runtime_error if a row cannot be translated
     */
    virtual void fixup_nexus(geojson::FeatureCollection& nexus) const = 0;

  private:
    geopackage::GeoPackageReader divides_reader_;
    std::optional<geopackage::GeoPackageReader> nexus_reader_;
    const HydrofabricVersion version_;
};

/**
 * Reads a hydrofabric v2.2 GeoPackage.
 *
 * v2.2 is the schema ngen's property names were taken from, so nothing needs translating; the only
 * version-specific behavior is resolving the divides id column, which
 * ngen::get_layer_id_column() already handles.
 */
class V2_2GeoPackageHydrofabricReader : public GeoPackageHydrofabricReader
{
  public:
    /**
     * @param[in] divides_reader Reader for the GeoPackage holding the `divides` layer
     * @param[in] nexus_reader Reader for the `nexus` layer's GeoPackage, if a separate file
     */
    V2_2GeoPackageHydrofabricReader(
        geopackage::GeoPackageReader divides_reader,
        std::optional<geopackage::GeoPackageReader> nexus_reader
    );

  protected:
    void fixup_divides(geojson::FeatureCollection& divides) const override;

    void fixup_nexus(geojson::FeatureCollection& nexus) const override;
};

/**
 * Common behavior of the hydrofabric v4 readers.
 *
 * Both v4 variants renamed the nexus identifier columns (`id`/`toid` to `nexus_id`/`nexus_toid`),
 * so nexus translation lives here, as does the summary warning about divides left without a
 * downstream reference. What the variants do not share is how a divide reaches its nexus, which is
 * left to attribute_divide_toid() in the concrete subclasses.
 */
class AbstractV4GeoPackageHydrofabricReader : public GeoPackageHydrofabricReader
{
  protected:
    /**
     * @param[in] divides_reader Reader for the GeoPackage holding the `divides` layer
     * @param[in] nexus_reader Reader for the `nexus` layer's GeoPackage, if a separate file
     * @param[in] version Which v4 variant was detected
     */
    AbstractV4GeoPackageHydrofabricReader(
        geopackage::GeoPackageReader divides_reader,
        std::optional<geopackage::GeoPackageReader> nexus_reader,
        HydrofabricVersion version
    );

    void check_required_columns(const std::string& layer) const override;

    void fixup_nexus(geojson::FeatureCollection& nexus) const override;

    /**
     * Alias each divide's downstream nexus to "toid", then warn once with a count of the divides
     * that ended up without one, rather than once per divide.
     *
     * @param[in,out] divides Divides collection to update in place
     * @throws std::runtime_error if a row cannot be translated
     */
    void fixup_divides(geojson::FeatureCollection& divides) const override;

    /**
     * Attribute "toid" on one divide, however this variant expresses it.
     *
     * Leaving "toid" unset marks the divide terminal, which is what an absent downstream reference
     * means to every consumer downstream of the loader.
     *
     * @param[in,out] properties Property map for the divide
     * @param[in] id Resolved divide id
     * @throws std::runtime_error if a column this variant requires is absent from the row
     */
    virtual void attribute_divide_toid(geojson::PropertyMap& properties, const std::string& id) const = 0;
};

/**
 * Reads a hydrofabric v4.0 GeoPackage.
 *
 * v4.0 divides carry their downstream nexus natively in `flowpath_toid`, so this reader never
 * consults the `flowpaths` table.
 */
class V4_0GeoPackageHydrofabricReader : public AbstractV4GeoPackageHydrofabricReader
{
  public:
    /**
     * @param[in] divides_reader Reader for the GeoPackage holding the `divides` layer
     * @param[in] nexus_reader Reader for the `nexus` layer's GeoPackage, if a separate file
     */
    V4_0GeoPackageHydrofabricReader(
        geopackage::GeoPackageReader divides_reader,
        std::optional<geopackage::GeoPackageReader> nexus_reader
    );

  protected:
    void check_required_columns(const std::string& layer) const override;

    void attribute_divide_toid(geojson::PropertyMap& properties, const std::string& id) const override;
};

/**
 * Reads a hydrofabric v4.0beta1 GeoPackage.
 *
 * beta1 divides have no downstream column of their own and reach their nexus indirectly, through
 * the flowpath they contain. This reader resolves that join once per read into a lookup table,
 * rather than once per divide.
 */
class V4_0Beta1GeoPackageHydrofabricReader : public AbstractV4GeoPackageHydrofabricReader
{
  public:
    /**
     * @param[in] divides_reader Reader for the GeoPackage holding the `divides` layer
     * @param[in] nexus_reader Reader for the `nexus` layer's GeoPackage, if a separate file
     */
    V4_0Beta1GeoPackageHydrofabricReader(
        geopackage::GeoPackageReader divides_reader,
        std::optional<geopackage::GeoPackageReader> nexus_reader
    );

    geojson::GeoJSON read_divides(const std::vector<std::string>& ids = {}) override;

  protected:
    void check_required_columns(const std::string& layer) const override;

    void attribute_divide_toid(geojson::PropertyMap& properties, const std::string& id) const override;

  private:
    std::unordered_map<std::string, std::string> divide_toid_lookup_;
    bool divide_toid_lookup_built_ = false;
};

} // namespace hydrofabric
} // namespace ngen

#endif // NGEN_GEOPACKAGE_HYDROFABRIC_READER_H
