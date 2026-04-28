#!/usr/bin/env python3
"""
Generate example_v3_0_dangling.gpkg — a v3.0 GeoPackage used by the
divides toid-synthesis unit test for the "join miss / dangling flowpath_id"
case.

Contains 2 divides:
  cat-1  flowpath_id = fp-1  (fp-1 IS in flowpaths -> toid synthesized)
  cat-2  flowpath_id = fp-DANGLING  (NOT in flowpaths -> no toid)

Topology:
  cat-1 (fp-1) --> nex-1
  cat-2 (fp-DANGLING) --> [unresolvable]

flowpaths table has only fp-1 (flowpath_toid = nex-1).

Usage:
    python3 make_v3_0_dangling_fixture.py

Output: example_v3_0_dangling.gpkg (sibling of this script).
Dependencies: Python 3.6+, stdlib only (os, sqlite3, struct). The CPython
`_sqlite3` extension is required; every standard CPython build has it.

Regenerating ALWAYS produces a tracked diff: gpkg_contents.last_change is
written via SQLite's datetime('now'), so the new file's timestamp differs
from whatever is committed. Bytes can also drift across SQLite library
versions for the same logical SQL. Treat regeneration as a deliberate,
maintainer-only action and verify intent with diff_fixture.py before
committing — see test/data/geopackage/README.md for the workflow.
"""

import os
import sqlite3
import struct

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "example_v3_0_dangling.gpkg")
SRS_ID = 4326


def _gpkg_hdr():
    return b"\x47\x50\x00\x01" + struct.pack("<i", SRS_ID)


def point_blob(x, y):
    wkb = b"\x01" + struct.pack("<I", 1) + struct.pack("<dd", x, y)
    return _gpkg_hdr() + wkb


def polygon_blob(rings):
    wkb = b"\x01" + struct.pack("<I", 3) + struct.pack("<I", len(rings))
    for ring in rings:
        wkb += struct.pack("<I", len(ring))
        for x, y in ring:
            wkb += struct.pack("<dd", x, y)
    return _gpkg_hdr() + wkb


def multilinestring_blob(lines):
    wkb = b"\x01" + struct.pack("<I", 5) + struct.pack("<I", len(lines))
    for pts in lines:
        wkb += b"\x01" + struct.pack("<I", 2) + struct.pack("<I", len(pts))
        for x, y in pts:
            wkb += struct.pack("<dd", x, y)
    return _gpkg_hdr() + wkb


def main():
    if os.path.exists(OUT):
        os.remove(OUT)

    db = sqlite3.connect(OUT)
    db.execute("PRAGMA page_size=512")
    cur = db.cursor()

    # ── GPKG required metadata tables ────────────────────────────────────────
    cur.execute("""
        CREATE TABLE gpkg_spatial_ref_sys (
            srs_name TEXT NOT NULL,
            srs_id INTEGER NOT NULL PRIMARY KEY,
            organization TEXT NOT NULL,
            organization_coordsys_id INTEGER NOT NULL,
            definition TEXT NOT NULL,
            description TEXT
        )
    """)
    cur.executemany(
        "INSERT INTO gpkg_spatial_ref_sys VALUES (?,?,?,?,?,?)",
        [
            ("Undefined Cartesian SRS", -1, "NONE", -1, "undefined", None),
            ("Undefined geographic SRS", 0, "NONE", 0, "undefined", None),
            (
                "WGS 84 geodetic", 4326, "EPSG", 4326,
                'GEOGCS["WGS 84",DATUM["WGS_1984",'
                'SPHEROID["WGS 84",6378137,298.257223563]],'
                'PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]]',
                None,
            ),
        ],
    )

    cur.execute("""
        CREATE TABLE gpkg_contents (
            table_name TEXT NOT NULL PRIMARY KEY,
            data_type TEXT NOT NULL,
            identifier TEXT,
            description TEXT,
            last_change TEXT,
            min_x REAL, min_y REAL, max_x REAL, max_y REAL,
            srs_id INTEGER
        )
    """)

    cur.execute("""
        CREATE TABLE gpkg_geometry_columns (
            table_name TEXT NOT NULL,
            column_name TEXT NOT NULL,
            geometry_type_name TEXT NOT NULL,
            srs_id INTEGER NOT NULL,
            z TINYINT NOT NULL,
            m TINYINT NOT NULL,
            CONSTRAINT pk_geom_cols PRIMARY KEY (table_name, column_name)
        )
    """)

    # ── nexus (v3.0 schema — provides the nexus_id column detect_version needs) ─
    cur.execute("""
        CREATE TABLE nexus (
            fid        INTEGER PRIMARY KEY AUTOINCREMENT,
            geom       POINT,
            nexus_id   TEXT NOT NULL,
            nexus_toid TEXT,
            vpuid      TEXT
        )
    """)
    cur.execute(
        "INSERT INTO nexus (geom, nexus_id, nexus_toid, vpuid) VALUES (?,?,?,?)",
        (point_blob(-81.0, 30.0), "nex-1", "coastal-000001", "03"),
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES"
        " ('nexus','features','nexus','',datetime('now'),-81.0,30.0,-81.0,30.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns VALUES ('nexus','geom','POINT',4326,0,0)"
    )

    # ── divides ──────────────────────────────────────────────────────────────
    # cat-1: flowpath_id=fp-1  (fp-1 exists -> toid will be synthesized)
    # cat-2: flowpath_id=fp-DANGLING  (not in flowpaths -> no toid)
    cat1 = [(-82.0, 29.0), (-81.0, 29.0), (-81.0, 30.0), (-82.0, 30.0), (-82.0, 29.0)]
    cat2 = [(-81.0, 29.0), (-80.0, 29.0), (-80.0, 30.0), (-81.0, 30.0), (-81.0, 29.0)]
    cur.execute("""
        CREATE TABLE divides (
            fid          INTEGER PRIMARY KEY AUTOINCREMENT,
            geom         POLYGON,
            divide_id    TEXT NOT NULL,
            areasqkm     REAL,
            vpuid        TEXT,
            flowpath_id  TEXT
        )
    """)
    cur.executemany(
        "INSERT INTO divides (geom, divide_id, areasqkm, vpuid, flowpath_id) VALUES (?,?,?,?,?)",
        [
            (polygon_blob([cat1]), "cat-1", 100.0, "03", "fp-1"),
            (polygon_blob([cat2]), "cat-2", 100.0, "03", "fp-DANGLING"),
        ],
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES"
        " ('divides','features','divides','',datetime('now'),-82.0,29.0,-80.0,30.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns VALUES ('divides','geom','POLYGON',4326,0,0)"
    )

    # ── flowpaths (only fp-1; fp-DANGLING intentionally absent) ─────────────
    cur.execute("""
        CREATE TABLE flowpaths (
            fid           INTEGER PRIMARY KEY AUTOINCREMENT,
            geom          MULTILINESTRING,
            flowpath_id   TEXT NOT NULL,
            flowpath_toid TEXT,
            vpuid         TEXT
        )
    """)
    cur.execute(
        "INSERT INTO flowpaths (geom, flowpath_id, flowpath_toid, vpuid) VALUES (?,?,?,?)",
        (multilinestring_blob([[(-81.5, 29.5), (-81.0, 30.0)]]), "fp-1", "nex-1", "03"),
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES"
        " ('flowpaths','features','flowpaths','',datetime('now'),-81.5,29.5,-81.0,30.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns VALUES ('flowpaths','geom','MULTILINESTRING',4326,0,0)"
    )

    db.commit()
    db.close()
    print(f"Written: {OUT}  ({os.path.getsize(OUT)} bytes)")

    # Verify the toid-synthesis join
    db2 = sqlite3.connect(OUT)
    rows = list(db2.execute(
        "SELECT d.divide_id, f.flowpath_toid"
        " FROM divides d JOIN flowpaths f ON d.flowpath_id = f.flowpath_id"
        " WHERE f.flowpath_toid IS NOT NULL"
    ))
    db2.close()

    assert len(rows) == 1, f"Expected 1 join row (cat-1 only), got {len(rows)}: {rows}"
    assert rows[0] == ("cat-1", "nex-1"), f"Unexpected join result: {rows[0]}"
    print("Join invariant verified: only cat-1 resolves; cat-2 (fp-DANGLING) has no match.")


if __name__ == "__main__":
    main()
