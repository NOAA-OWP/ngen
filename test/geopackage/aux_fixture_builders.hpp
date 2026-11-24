#ifndef NGEN_TEST_GEOPACKAGE_AUX_FIXTURE_BUILDERS_H
#define NGEN_TEST_GEOPACKAGE_AUX_FIXTURE_BUILDERS_H

#include <string>

namespace ngen {
namespace geopackage {

/**
 * Synthetic GeoPackage fixtures for the auxiliary attribute table tests, built at test time.
 *
 * The builder writes into the test temp directory, replacing anything already there, and returns
 * the path it wrote. Nothing is written into the source tree and no fixture is committed, so a
 * fixture can never drift from the tests that depend on it.
 *
 * Unlike the hydrofabric fixtures, which describe whole hydrofabrics and are written from nothing,
 * these start from the committed `example.gpkg` and add tables to a copy of it. That file is an
 * opaque blob with no generator (see test/data/geopackage/README.md), and reproducing its two-point
 * "test" layer here would duplicate the geometry-writing machinery in
 * test/hydrofabric/fixture_builders.cpp to no purpose: what these tests need from it is a feature
 * layer to join onto, not a particular geometry.
 */
namespace fixtures {

/**
 * Write a copy of `example.gpkg` carrying auxiliary attribute tables.
 *
 * The layer joined onto is `example.gpkg`'s own two-feature "test" layer, holding "First" and
 * "Second". The tables added to it have the shape of the regionalization tables in the Aug 2026
 * hydrofabric preview: keyed on a text divide identifier, no "fid" column, and registered in
 * `gpkg_contents` as 'attributes' with a NULL srs_id and no bounding box.
 *
 * Between them the tables cover prefixing, per-column type mapping, NULL cells, a feature with no
 * row, a row for no feature, a non-default key column, column names shared between two joined
 * tables, cells of a type that cannot become a property, and a table keyed ambiguously.
 *
 * @return Path to the written GeoPackage
 * @throws std::runtime_error if `example.gpkg` cannot be found or copied, or if SQLite refuses any
 *         step of the write
 */
std::string write_example_aux();

} // namespace fixtures
} // namespace geopackage
} // namespace ngen

#endif // NGEN_TEST_GEOPACKAGE_AUX_FIXTURE_BUILDERS_H
