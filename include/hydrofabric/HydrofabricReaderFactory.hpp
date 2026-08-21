#ifndef NGEN_HYDROFABRIC_READER_FACTORY_H
#define NGEN_HYDROFABRIC_READER_FACTORY_H

#include <memory>
#include <string>

#include "HydrofabricReader.hpp"
#include "HydrofabricVersion.hpp"

namespace ngen {
namespace hydrofabric {

/**
 * Identify the hydrofabric a pair of data paths names.
 *
 * Both paths must name the same format: a hydrofabric is one thing, and half of one in GeoJSON
 * beside half in a GeoPackage is a mistake worth reporting rather than honoring.
 *
 * GeoJSON is identified from the path alone and always yields V1_GEOJSON -- the format carries no
 * release marker, so there is nothing further to read. A GeoPackage is opened and its schema
 * inspected, yielding the release it describes, or UNRECOGNIZED when it turns out not to be a
 * hydrofabric at all.
 *
 * @param[in] catchment_path Path to the file holding the divides
 * @param[in] nexus_path Path to the file holding the nexuses; may be the same file
 * @return HydrofabricVersion identified for the pair, or UNRECOGNIZED if the paths name
 *         GeoPackages that are not a hydrofabric
 * @throws std::runtime_error if the two paths name different formats, if a GeoPackage cannot be
 *         opened, if its schema matches no known release, or if the paths name GeoPackages and
 *         GeoPackage support was not built in
 */
HydrofabricVersion detect_hydrofabric(const std::string& catchment_path, const std::string& nexus_path);

/**
 * Open a hydrofabric whose divides and nexuses may live in two files.
 *
 * The format is taken from the paths and the release, where the format has one, from the files
 * themselves; the caller supplies paths and gets back something it can read both roles from.
 *
 * @param[in] catchment_path Path to the file holding the divides
 * @param[in] nexus_path Path to the file holding the nexuses; may be the same file, in which case
 *            it is opened once
 * @return std::unique_ptr<HydrofabricReader> Reader for the hydrofabric
 * @throws std::runtime_error under the same conditions as detect_hydrofabric(), or if the paths
 *         name GeoPackages that are not a hydrofabric
 */
std::unique_ptr<HydrofabricReader> make_hydrofabric_reader(
    const std::string& catchment_path,
    const std::string& nexus_path
);

/**
 * Open a hydrofabric held entirely in one file.
 *
 * @param[in] path Path to the file holding both the divides and the nexuses
 * @return std::unique_ptr<HydrofabricReader> Reader for the hydrofabric
 * @throws std::runtime_error under the same conditions as the two-path form
 */
std::unique_ptr<HydrofabricReader> make_hydrofabric_reader(const std::string& path);

} // namespace hydrofabric
} // namespace ngen

#endif // NGEN_HYDROFABRIC_READER_FACTORY_H
