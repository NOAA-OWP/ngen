#ifndef NGEN_TEST_HYDROFABRIC_FIXTURE_BUILDERS_H
#define NGEN_TEST_HYDROFABRIC_FIXTURE_BUILDERS_H

#include <string>

namespace ngen {
namespace hydrofabric {

/**
 * Synthetic GeoPackage fixtures, built at test time.
 *
 * Each builder writes one small GeoPackage into the test temp directory,
 * replacing anything already there, and returns the path it wrote. Nothing is
 * written into the source tree and no fixture is committed, so a fixture can
 * never drift from the test that depends on it.
 *
 * The three synthetic hydrofabrics -- v2.2, v4.0beta1 and v4.0 -- all describe
 * the same topology, so a test can load two of them and compare directly:
 *
 *     cat-1 -+
 *            +-> nex-1 -> cat-2 -> nex-2 -> coastal-000001
 *     cat-3 -+
 *
 * cat-1 and cat-3 meet at nex-1, which drains through cat-2 to the terminal
 * nex-2. The remaining builders vary that base in one respect each, for the
 * regression tests that need the variation.
 *
 * Every builder throws std::runtime_error if SQLite refuses any step of the
 * write, so a broken fixture fails loudly at setup rather than as a puzzling
 * assertion later.
 */
namespace fixtures {

/**
 * Write the v2.2 fixture: 3 divides, 2 nexuses, 3 flowpaths.
 *
 * Nexuses are keyed by `id` (not `nexus_id`), which is what marks the file
 * v2.2, and divides carry a native `toid`.
 *
 * @return Path to the written GeoPackage
 */
std::string write_v2_2();

/**
 * Write the v4.0beta1 fixture: 3 divides, 2 nexuses, 3 flowpaths.
 *
 * Nexuses are keyed by `nexus_id`, marking the v4 family, and `divides` has
 * no `flowpath_toid` column, which is what makes it beta1 rather than v4.0:
 * a divide reaches its nexus only through the flowpath it names.
 *
 * @return Path to the written GeoPackage
 */
std::string write_v4_0beta1();

/**
 * Write the v4.0 fixture: the release-schema counterpart of the beta1
 * fixture, with identical topology.
 *
 * `divides.flowpath_toid` holds the downstream nexus natively, so the loader
 * never needs the `flowpaths` table. That table is still present, with the
 * release column names, so the fixture stays a faithful sample of the format
 * and a test can check the native answer against the join it replaces.
 *
 * @return Path to the written GeoPackage
 */
std::string write_v4_0();

/**
 * Write the `nexus` half of a v4.0 hydrofabric, with no `divides` layer.
 *
 * Paired with write_v4_0_divides_only(), this is a v4.0 hydrofabric split across two files, which
 * the drivers have always accepted: neither half can be identified alone, since `nexus` marks the
 * v4 family and only `divides.flowpath_toid` separates v4.0 from beta1.
 *
 * @return Path to the written GeoPackage
 */
std::string write_v4_0_nexus_only();

/**
 * Write the `divides` half of a v4.0 hydrofabric, with no `nexus` layer.
 *
 * The counterpart to write_v4_0_nexus_only(); on its own it is not a hydrofabric at all.
 *
 * @return Path to the written GeoPackage
 */
std::string write_v4_0_divides_only();

/**
 * Write a v4.0beta1 fixture holding only `nexus`, `divides` and `flowpaths`.
 *
 * Real files carry auxiliary layers the loader never reads; this one proves
 * none of them is quietly required.
 *
 * @return Path to the written GeoPackage
 */
std::string write_v4_0beta1_minimal();

/**
 * Write a v4.0beta1 fixture whose second divide names a flowpath that does
 * not exist.
 *
 * cat-1 resolves to nex-1 as usual; cat-2 names `fp-DANGLING`, which is
 * absent from `flowpaths`, so it must load with no `toid` at all rather than
 * failing the read or inventing one.
 *
 * @return Path to the written GeoPackage
 */
std::string write_v4_0beta1_dangling();

/**
 * Write a single-divide v4.0beta1 fixture carrying auxiliary tables with
 * unusual schemas: a `flowlines` table with an extra column, and a `pois`
 * table whose geometry column is declared `GEOMETRY` rather than `POINT`.
 *
 * Neither is read, so neither may affect the `nexus` and `divides` loads.
 *
 * @return Path to the written GeoPackage
 */
std::string write_v4_0beta1_extra_col();

} // namespace fixtures
} // namespace hydrofabric
} // namespace ngen

#endif // NGEN_TEST_HYDROFABRIC_FIXTURE_BUILDERS_H
