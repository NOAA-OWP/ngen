# GeoPackage Test Fixtures

Most of the GeoPackages the geopackage tests read are **not** in this
directory. They are built in C++ at test time by
`test/geopackage/fixture_builders.cpp`, written into the test temp directory,
and thrown away with it. Only three fixtures are committed here, each because
it cannot be built from scratch.

## Hydrofabric versions and variants

`detect_version` classifies a hydrofabric GeoPackage into one of three schemas:

| Version      | Nexus id column | Divides downstream reference        |
|--------------|-----------------|-------------------------------------|
| `V2_2`       | `id`            | native `toid`                       |
| `V4_0_BETA1` | `nexus_id`      | none — synthesized via a join to `flowpaths` on `flowpath_id` |
| `V4_0`       | `nexus_id`      | native `flowpath_toid`              |

`nexus.nexus_id` marks the whole v4 family; the presence of a native
`divides.flowpath_toid` column is what separates the v4.0 release schema from
v4.0beta1. Fixtures are named for the variant they instantiate, so the
`example_v4_0beta1*` files all lack `flowpath_toid` by design.

A GeoPackage with no `nexus` layer is not a hydrofabric at all. `detect_hydrofabric`
reports that as `(false, UNRECOGNIZED)`, and `UNRECOGNIZED` never names a schema: a
`nexus` layer that matches none of the three is rejected outright rather than carried
as a value.

---

## Generated at test time

`test/geopackage/fixture_builders.hpp` declares one builder per fixture.
Each writes its GeoPackage into `testing::TempDir()`, replacing whatever was
there, and returns the path; a test suite calls the builders it needs from
`SetUpTestSuite`. The writes are deterministic — including a constant
`gpkg_contents.last_change` — so repeated runs produce identical files.

Nothing about these fixtures can drift from the tests that read them: change a
builder and the next test run uses the change. There is no regeneration step,
no committed binary to refresh, and no Python.

| Builder | Describes |
|---------|-----------|
| `write_v2_2()` | v2.2: 3 divides, 2 nexuses, 3 flowpaths |
| `write_v4_0beta1()` | Same topology, v4.0beta1 schema |
| `write_v4_0()` | Same topology, v4.0 release schema |
| `write_v4_0beta1_minimal()` | v4.0beta1 with only `nexus`, `divides`, `flowpaths` |
| `write_v4_0beta1_dangling()` | v4.0beta1 where one divide names a missing flowpath |
| `write_v4_0beta1_extra_col()` | v4.0beta1 with auxiliary tables carrying odd schemas |
| `write_v4_0_nexus_only()` | The `nexus` half of a v4.0 hydrofabric split across two files |
| `write_v4_0_divides_only()` | The `divides` half of that same split hydrofabric |

The last two exist as a pair. Neither can be identified alone — `nexus` marks the v4
family, and only `divides.flowpath_toid` separates v4.0 from beta1 — so together they
cover detecting a hydrofabric whose layers live in two files, which the drivers have
always accepted.

The first three describe the **same topology**, so a test can load two of them
and compare the results directly even though the loader reaches `toid`
differently in each:

```
cat-1 (fp-1) ─┐
               ├─> nex-1 ─> fp-2 ─> cat-2 (fp-2) ─> nex-2 ─> coastal-000001
cat-3 (fp-3) ─┘
```

`geopackage_v4_0_matches_beta1_toids` asserts exactly that for the two v4
variants: beta1 joins `divides` to `flowpaths`, while v4.0 reads
`divides.flowpath_toid` directly and never opens `flowpaths`.

---

## Committed here

### example_v4_0_real.gpkg

A **real** NOAA-OWP v4.0 hydrofabric — a gage subset for USGS 01031500
(Piscataquis River, Maine) with every layer ngen never reads stripped out.
It is committed because it is derived from a real hydrofabric that is not in
this repository, so there is nothing to derive it from at test time.

190 divides, 75 nexuses (74 `nex-` plus one `tnx-` terminal nexus whose
`nexus_toid` is NULL), 190 flowpaths. `divides`, `nexus`, and `flowpaths` are
verbatim — no column, value, or geometry inside them is modified. Dropped:
`flowlines`, `flowline-attributes`, `flowpath-attributes`, `network`, `lakes`,
`pois`, `hydrolocations`, and the r-tree tables of each.

This is the only fixture with genuine topology and geometry, and the only one
carrying a terminal nexus. It is larger than the generated fixtures (~650 KB)
because real divide polygons in EPSG:5070 are heavy.

The prefixed identifiers are a dense 1..203 renumbering produced by the
upstream subsetter, so they carry no trace of the parent VPU numbering.

### example.gpkg / example_3857.gpkg

Non-hydrofabric fixtures used by the original GeoPackage read tests, one in
EPSG:4326 and one in EPSG:3857. They predate this work, have no generator, and
are opaque committed blobs. Neither has a `nexus` table, so neither is a
hydrofabric; they are read through `ngen::geopackage::GeoPackageReader` directly,
which is the generic path and needs no hydrofabric knowledge to read them.
