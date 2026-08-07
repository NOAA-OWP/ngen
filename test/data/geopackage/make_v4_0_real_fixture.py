#!/usr/bin/env python3
"""
Derive example_v4_0_real.gpkg from a real NOAA-OWP v4.0 hydrofabric.

Unlike the other generators in this directory, this fixture is not synthesized
from scratch — it is a real gage subset with the tables ngen never reads
stripped out. That keeps genuine topology (a real confluence structure, a real
terminal nexus, real Albers geometry) in the test corpus at a fraction of the
source file's size.

Source used for the committed fixture:
    NOAA-OWP hydrofabric v4.0 gage subset for USGS 01031500
    (Piscataquis River, Maine), 1.9 MB as distributed.

What is kept:
    divides, nexus, flowpaths  — the only layers ngen's loader reads

What is dropped:
    flowlines, flowline-attributes, flowpath-attributes, network, lakes,
    pois, hydrolocations, lake_id_flagged_drops, and the r-tree index
    tables/triggers belonging to any dropped layer

Nothing inside the retained layers is modified: every column, value, and
geometry is carried through verbatim. The prefixed identifiers in the source
are already a dense 1..203 renumbering produced by the subsetter, so they
carry no trace of the parent VPU numbering.

Result: 190 divides, 75 nexuses (74 nex- plus 1 tnx- terminal nexus whose
nexus_toid is NULL), 190 flowpaths.

Usage:
    python3 make_v4_0_real_fixture.py /path/to/ngen_v4_01031500.gpkg

Output: example_v4_0_real.gpkg (sibling of this script).
Dependencies: Python 3.6+, stdlib only (os, shutil, sqlite3, sys).

Regenerating from the same source should be byte-stable, but VACUUM output
can drift across SQLite library versions. Treat regeneration as a deliberate,
maintainer-only action and verify intent with diff_fixture.py before
committing — see test/data/geopackage/README.md for the workflow.
"""

import os
import shutil
import sqlite3
import sys
from typing import List

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "example_v4_0_real.gpkg")

# The only layers ngen's GeoPackage loader reads. flowpaths is retained even
# though v4.0 does not need it for toid, so the fixture can also exercise the
# v4.0beta1 join path if a test ever wants to compare the two.
KEEP_LAYERS = ("divides", "nexus", "flowpaths")


def layer_tables(cur: sqlite3.Cursor) -> List[str]:
    """
    List user tables in the database, excluding SQLite/GeoPackage internals.

    :param cur: Open cursor on the fixture database.
    :return: Names of candidate layer tables.
    """
    rows = cur.execute(
        "SELECT name FROM sqlite_master WHERE type='table'"
        " AND name NOT LIKE 'gpkg_%' AND name NOT LIKE 'rtree_%'"
        " AND name NOT LIKE 'sqlite_%' ORDER BY name"
    ).fetchall()
    return [r[0] for r in rows]


def drop_layer(cur: sqlite3.Cursor, layer: str) -> None:
    """
    Drop a layer along with its r-tree index tables and triggers.

    Dropping the layer table alone leaves orphaned rtree_<layer>_geom* tables
    and triggers behind, which bloat the file and reference a table that no
    longer exists.

    :param cur: Open cursor on the fixture database.
    :param layer: Name of the layer table to remove.
    """
    triggers = cur.execute(
        "SELECT name FROM sqlite_master WHERE type='trigger' AND tbl_name=?", (layer,)
    ).fetchall()
    for (name,) in triggers:
        cur.execute(f'DROP TRIGGER IF EXISTS "{name}"')

    rtree_triggers = cur.execute(
        "SELECT name FROM sqlite_master WHERE type='trigger' AND name LIKE ?",
        (f"rtree_{layer}_%",),
    ).fetchall()
    for (name,) in rtree_triggers:
        cur.execute(f'DROP TRIGGER IF EXISTS "{name}"')

    rtree_tables = cur.execute(
        "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE ?",
        (f"rtree_{layer}_%",),
    ).fetchall()
    for (name,) in rtree_tables:
        cur.execute(f'DROP TABLE IF EXISTS "{name}"')

    cur.execute(f'DROP TABLE IF EXISTS "{layer}"')


def verify(path: str) -> None:
    """
    Assert the derived fixture is a well-formed v4.0 hydrofabric.

    :param path: Path to the freshly written fixture.
    :raises AssertionError: If shape, linkage, or integrity is wrong.
    """
    db = sqlite3.connect(path)

    tables = set(
        r[0] for r in db.execute(
            "SELECT name FROM sqlite_master WHERE type='table'"
            " AND name NOT LIKE 'gpkg_%' AND name NOT LIKE 'rtree_%'"
            " AND name NOT LIKE 'sqlite_%'"
        )
    )
    assert tables == set(KEEP_LAYERS), f"Unexpected layers retained: {sorted(tables)}"

    divides_cols = set(r[1] for r in db.execute("PRAGMA table_info(divides)"))
    assert "flowpath_toid" in divides_cols, "divides.flowpath_toid missing; not a v4.0 file"

    nexus_cols = set(r[1] for r in db.execute("PRAGMA table_info(nexus)"))
    assert "nexus_id" in nexus_cols, "nexus.nexus_id missing; not a v4 file"

    (dangling,) = db.execute(
        "SELECT COUNT(*) FROM divides d"
        " LEFT JOIN nexus n ON d.flowpath_toid = n.nexus_id"
        " WHERE n.nexus_id IS NULL"
    ).fetchone()
    assert dangling == 0, f"{dangling} divides point at a nexus not present in the file"

    (mismatch,) = db.execute(
        "SELECT COUNT(*) FROM divides d"
        " JOIN flowpaths f ON d.flowpath_id = f.flowpath_id"
        " WHERE f.flowpath_toid IS NOT d.flowpath_toid"
    ).fetchone()
    assert mismatch == 0, f"{mismatch} rows where native toid disagrees with the join"

    (n_div,) = db.execute("SELECT COUNT(*) FROM divides").fetchone()
    (n_nex,) = db.execute("SELECT COUNT(*) FROM nexus").fetchone()
    (n_term,) = db.execute(
        "SELECT COUNT(*) FROM nexus WHERE nexus_toid IS NULL"
    ).fetchone()
    db.close()

    assert n_term > 0, "expected at least one terminal nexus with NULL nexus_toid"
    print(
        f"Verified: {n_div} divides, {n_nex} nexuses ({n_term} terminal),"
        " native toid == join, no dangling references."
    )


def main() -> None:
    """Derive the fixture from the source path given on the command line."""
    if len(sys.argv) != 2:
        print(f"usage: {os.path.basename(__file__)} <source-v4.0-hydrofabric.gpkg>")
        raise SystemExit(2)

    src = sys.argv[1]
    if not os.path.exists(src):
        print(f"source not found: {src}")
        raise SystemExit(1)

    if os.path.exists(OUT):
        os.remove(OUT)
    shutil.copyfile(src, OUT)

    db = sqlite3.connect(OUT)
    cur = db.cursor()

    for layer in layer_tables(cur):
        if layer not in KEEP_LAYERS:
            drop_layer(cur, layer)

    placeholders = ",".join("?" for _ in KEEP_LAYERS)
    cur.execute(
        f"DELETE FROM gpkg_contents WHERE table_name NOT IN ({placeholders})", KEEP_LAYERS
    )
    cur.execute(
        f"DELETE FROM gpkg_geometry_columns WHERE table_name NOT IN ({placeholders})",
        KEEP_LAYERS,
    )
    has_ogr = cur.execute(
        "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='gpkg_ogr_contents'"
    ).fetchone()[0]
    if has_ogr:
        cur.execute(
            f"DELETE FROM gpkg_ogr_contents WHERE table_name NOT IN ({placeholders})",
            KEEP_LAYERS,
        )

    db.commit()
    cur.execute("VACUUM")
    db.close()

    print(f"Written: {OUT}  ({os.path.getsize(OUT)} bytes)")
    verify(OUT)


if __name__ == "__main__":
    main()
