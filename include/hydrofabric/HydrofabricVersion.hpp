#ifndef NGEN_HYDROFABRIC_VERSION_H
#define NGEN_HYDROFABRIC_VERSION_H

namespace ngen {
namespace hydrofabric {

/**
 * The kinds of hydrofabric input the loader can identify and read.
 *
 * Most of these name NOAA-OWP hydrofabric releases, told apart by what a nexus and a divide are
 * called and by how a divide names its downstream nexus: v2.2 identifies nexus rows via `id`,
 * while v4 renamed it to `nexus_id` (and `toid` to `nexus_toid`). The v4 family has two variants
 * for how a divide reaches its downstream nexus: V4_0_BETA1 carries no `flowpath_toid` and
 * synthesizes "toid" by joining divides to flowpaths on `flowpath_id`; V4_0 exposes
 * `flowpath_toid` natively and never consults flowpaths. Use is_v4() for checks that apply to the
 * whole family.
 *
 * V1_GEOJSON is the odd one, and the "V1" in it is a designation of ours rather than a release the
 * hydrofabric was ever published under: a GeoJSON hydrofabric carries no release marker to read, so
 * there is nothing to identify beyond the format itself. It names the vocabulary ngen's GeoJSON
 * path has always expected -- `id` and `toid` as ordinary feature properties, needing no
 * translation -- which predates every numbered release the loader distinguishes.
 *
 * UNRECOGNIZED is not a hydrofabric at all. It is the answer when the thing asked about turns out
 * to be something else, such as a plain GeoPackage of arbitrary tables. It never names a
 * hydrofabric whose release went unidentified: something that presents as a hydrofabric but
 * matches no known release is rejected outright rather than carried as a value every caller
 * downstream would have to remember to check.
 */
enum class HydrofabricVersion {
    V1_GEOJSON,
    V2_2,
    V4_0_BETA1,
    V4_0,
    UNRECOGNIZED
};

/**
 * Whether @p version belongs to the v4 family (either variant).
 *
 * @param[in] version Identified hydrofabric version
 * @return true for V4_0_BETA1 and V4_0, false for anything else
 */
inline bool is_v4(const HydrofabricVersion version)
{
    return version == HydrofabricVersion::V4_0_BETA1 || version == HydrofabricVersion::V4_0;
}

/**
 * Human-readable label for @p version, as used in log output.
 *
 * @param[in] version Identified hydrofabric version
 * @return Short label, e.g. "v4.0beta1"
 */
inline const char* version_label(const HydrofabricVersion version)
{
    switch (version) {
        case HydrofabricVersion::V1_GEOJSON:   return "v1 (GeoJSON)";
        case HydrofabricVersion::V2_2:         return "v2.2";
        case HydrofabricVersion::V4_0_BETA1:   return "v4.0beta1";
        case HydrofabricVersion::V4_0:         return "v4.0";
        case HydrofabricVersion::UNRECOGNIZED: return "not a hydrofabric";
    }
    return "unknown";
}

} // namespace hydrofabric
} // namespace ngen

#endif // NGEN_HYDROFABRIC_VERSION_H
