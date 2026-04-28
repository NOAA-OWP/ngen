#!/usr/bin/env python3
"""
Generate example_v3_0.gpkg — a minimal v3.0 GeoPackage used by the
detect_version, nexus-remap, toid-synthesis, and v3.0 regression tests.

Topology (3 catchments, 2 nexuses) — mirrors example_v2_2.gpkg:

    cat-1 (fp-1) ─┐
                    ├─> nex-1 ─> fp-2 ─> cat-2 (fp-2) ─> nex-2 ─> coastal-000001
    cat-3 (fp-3) ─┘

- cat-1 and cat-3 both drain to nex-1 (confluence), via fp-1 / fp-3.
- nex-1 drains into cat-2 (nexus_toid = fp-2, the flowpath of cat-2).
- cat-2 drains to nex-2 (via fp-2 → nex-2).
- nex-2 is terminal (nexus_toid = coastal-000001).

Feature IDs:
  Divides:   cat-1, cat-2, cat-3   (divide_id)
  Nexuses:   nex-1, nex-2          (nexus_id)
  Flowpaths: fp-1, fp-2, fp-3      (flowpath_id)

Schema (v3.0):
  nexus    (fid, geom POINT,          nexus_id, nexus_toid, vpuid)
  divides  (fid, geom POLYGON,        divide_id, areasqkm, has_flowline, ds_id,
                                       type, vpuid, flowpath_id)
  flowpaths(fid, geom MULTILINESTRING, flowpath_id, flowpath_toid, flowline_id,
                                        divide_id, mainstem, hydroseq, lengthkm,
                                        areasqkm, tot_drainage_areasqkm,
                                        has_divide, vpuid, ibt, poi_id,
                                        member_comid)

Join invariant (toid synthesis):
  divide.flowpath_id -> flowpaths.flowpath_id -> flowpaths.flowpath_toid
  All three divides resolve to a nexus via this join.

Usage:
    python3 make_v3_0_fixture.py

Output: example_v3_0.gpkg (sibling of this script).
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

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "example_v3_0.gpkg")
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

    # ── nexus (v3.0 schema) ───────────────────────────────────────────────────
    # nexus_id column (not 'id') is the v3.0 signature detected by detect_version.
    # nexus_toid points to the downstream flowpath (or coastal-* for terminal).
    cur.execute("""
        CREATE TABLE nexus (
            fid        INTEGER PRIMARY KEY AUTOINCREMENT,
            geom       POINT,
            nexus_id   TEXT NOT NULL,
            nexus_toid TEXT,
            vpuid      TEXT
        )
    """)
    cur.executemany(
        "INSERT INTO nexus (geom, nexus_id, nexus_toid, vpuid) VALUES (?,?,?,?)",
        [
            (point_blob(-81.0, 30.0), "nex-1", "fp-2",           "03"),
            (point_blob(-80.0, 30.0), "nex-2", "coastal-000001", "03"),
        ],
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES"
        " ('nexus','features','nexus','',datetime('now'),-82.0,29.0,-80.0,31.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns VALUES ('nexus','geom','POINT',4326,0,0)"
    )

    # ── divides (v3.0 schema) ─────────────────────────────────────────────────
    # divide_id  = catchment identifier (cat-*)
    # flowpath_id = associated flowpath; used by the toid-synthesis JOIN
    cat1 = [(-82.0, 29.0), (-81.0, 29.0), (-81.0, 30.0), (-82.0, 30.0), (-82.0, 29.0)]
    cat2 = [(-81.0, 29.0), (-80.0, 29.0), (-80.0, 31.0), (-81.0, 31.0), (-81.0, 29.0)]
    cat3 = [(-82.0, 30.0), (-81.0, 30.0), (-81.0, 31.0), (-82.0, 31.0), (-82.0, 30.0)]
    cur.execute("""
        CREATE TABLE divides (
            fid          INTEGER PRIMARY KEY AUTOINCREMENT,
            geom         POLYGON,
            divide_id    TEXT NOT NULL,
            areasqkm     REAL,
            has_flowline BOOLEAN,
            ds_id        BOOLEAN,
            type         TEXT,
            vpuid        TEXT,
            flowpath_id  TEXT
        )
    """)
    cur.executemany(
        "INSERT INTO divides"
        " (geom, divide_id, areasqkm, has_flowline, ds_id, type, vpuid, flowpath_id)"
        " VALUES (?,?,?,?,?,?,?,?)",
        [
            (polygon_blob([cat1]), "cat-1", 100.0, 1, 0, "divide", "03", "fp-1"),
            (polygon_blob([cat2]), "cat-2", 200.0, 1, 0, "divide", "03", "fp-2"),
            (polygon_blob([cat3]), "cat-3", 100.0, 1, 0, "divide", "03", "fp-3"),
        ],
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES"
        " ('divides','features','divides','',datetime('now'),-82.0,29.0,-80.0,31.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns VALUES ('divides','geom','POLYGON',4326,0,0)"
    )

    # ── flowpaths (v3.0 schema) ───────────────────────────────────────────────
    # flowpath_id   = fp-* identifier
    # flowpath_toid = downstream nexus (nex-*)
    # divide_id     = back-reference to associated divide
    cur.execute("""
        CREATE TABLE flowpaths (
            fid                   INTEGER PRIMARY KEY AUTOINCREMENT,
            geom                  MULTILINESTRING,
            flowpath_id           TEXT NOT NULL,
            flowpath_toid         TEXT,
            flowline_id           TEXT,
            divide_id             TEXT,
            mainstem              MEDIUMINT,
            hydroseq              MEDIUMINT,
            lengthkm              REAL,
            areasqkm              REAL,
            tot_drainage_areasqkm REAL,
            has_divide            BOOLEAN,
            vpuid                 TEXT,
            ibt                   BOOLEAN,
            poi_id                REAL,
            member_comid          TEXT
        )
    """)
    cur.executemany(
        "INSERT INTO flowpaths"
        " (geom, flowpath_id, flowpath_toid, flowline_id, divide_id, mainstem,"
        "  hydroseq, lengthkm, areasqkm, tot_drainage_areasqkm, has_divide,"
        "  vpuid, ibt, poi_id, member_comid)"
        " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        [
            (multilinestring_blob([[(-81.5, 29.5), (-81.0, 30.0)]]),
             "fp-1", "nex-1", "fl-1", "cat-1", 0, 3, 10.0, 100.0, 200.0, 1, "03", 0, None, None),
            (multilinestring_blob([[(-80.5, 30.0), (-80.0, 30.0)]]),
             "fp-2", "nex-2", "fl-2", "cat-2", 1, 1, 15.0, 200.0, 200.0, 1, "03", 0, 1.0, None),
            (multilinestring_blob([[(-81.5, 30.5), (-81.0, 30.0)]]),
             "fp-3", "nex-1", "fl-3", "cat-3", 0, 2, 10.0, 100.0, 100.0, 1, "03", 0, None, None),
        ],
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES"
        " ('flowpaths','features','flowpaths','',datetime('now'),-81.5,29.5,-80.0,30.5,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns"
        " VALUES ('flowpaths','geom','MULTILINESTRING',4326,0,0)"
    )

    db.commit()
    db.close()
    print(f"Written: {OUT}  ({os.path.getsize(OUT)} bytes)")

    # Verify the toid-synthesis join resolves for all 3 divides
    db2 = sqlite3.connect(OUT)
    rows = list(db2.execute(
        "SELECT d.divide_id, f.flowpath_toid"
        " FROM divides d JOIN flowpaths f ON d.flowpath_id = f.flowpath_id"
    ))
    db2.close()
    assert len(rows) == 3, f"Expected 3 join rows, got {len(rows)}"
    expected = {("cat-1", "nex-1"), ("cat-2", "nex-2"), ("cat-3", "nex-1")}
    assert set(rows) == expected, f"Unexpected join result: {rows}"
    print("Join invariant verified: all 3 divides resolve to a nexus.")


if __name__ == "__main__":
    main()
