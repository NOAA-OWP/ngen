#!/usr/bin/env python3
"""
Generate example_v2_2.gpkg — a minimal v2.2 GeoPackage used by the
detect_version, nexus-remap, and v2.2 regression tests.

Topology (3 catchments, 2 nexuses):

    cat-1 (wb-1) ─┐
                    ├─> nex-1 ─> wb-2 ─> cat-2 (wb-2) ─> nex-2 ─> coastal-000001
    cat-3 (wb-3) ─┘

- cat-1 and cat-3 both drain to nex-1 (confluence).
- nex-1 drains into cat-2 (via wb-2, the flowpath within cat-2).
- cat-2 drains to nex-2.
- nex-2 is terminal (toid = coastal-000001).

Feature IDs (as returned by the loader):
  Divides:  cat-1, cat-2, cat-3   (loader uses divide_id column)
  Nexuses:  nex-1, nex-2          (loader uses id column)
  Flowpaths: wb-1, wb-2, wb-3    (id column)

Schema (v2.2):
  nexus    (fid, geom POINT,          id, toid, type, vpuid, poi_id)
  divides  (fid, geom POLYGON,        divide_id, id, toid, type, ds_id, areasqkm,
                                       vpuid, lengthkm, tot_drainage_areasqkm, has_flowline)
  flowpaths(fid, geom MULTILINESTRING, id, toid, mainstem, hydroseq, lengthkm,
                                       areasqkm, tot_drainage_areasqkm, has_divide,
                                       divide_id, poi_id, vpuid)
"""

import os
import sqlite3
import struct

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "example_v2_2.gpkg")
SRS_ID = 4326


def _gpkg_hdr():
    """Minimal GeoPackage WKB envelope header (no envelope flags)."""
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

    # ── nexus (v2.2 schema) ───────────────────────────────────────────────────
    # id column (not nexus_id) is the v2.2 signature detected by detect_version.
    cur.execute("""
        CREATE TABLE nexus (
            fid     INTEGER PRIMARY KEY AUTOINCREMENT,
            geom    POINT,
            id      TEXT NOT NULL,
            toid    TEXT,
            type    TEXT,
            vpuid   TEXT,
            poi_id  REAL
        )
    """)
    cur.executemany(
        "INSERT INTO nexus (geom, id, toid, type, vpuid, poi_id) VALUES (?,?,?,?,?,?)",
        [
            (point_blob(-81.0, 30.0), "nex-1", "wb-2",           "nexus", "03", None),
            (point_blob(-80.0, 30.0), "nex-2", "coastal-000001", "nexus", "03", 1.0),
        ],
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES"
        " ('nexus','features','nexus','',datetime('now'),-82.0,29.0,-80.0,31.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns VALUES ('nexus','geom','POINT',4326,0,0)"
    )

    # ── divides (v2.2 schema) ─────────────────────────────────────────────────
    # divide_id  = catchment identifier (cat-*)
    # id         = associated flowpath identifier (wb-*)
    # toid       = downstream nexus
    cat1 = [(-82.0, 29.0), (-81.0, 29.0), (-81.0, 30.0), (-82.0, 30.0), (-82.0, 29.0)]
    cat2 = [(-81.0, 29.0), (-80.0, 29.0), (-80.0, 31.0), (-81.0, 31.0), (-81.0, 29.0)]
    cat3 = [(-82.0, 30.0), (-81.0, 30.0), (-81.0, 31.0), (-82.0, 31.0), (-82.0, 30.0)]
    cur.execute("""
        CREATE TABLE divides (
            fid                    INTEGER PRIMARY KEY AUTOINCREMENT,
            geom                   POLYGON,
            divide_id              TEXT NOT NULL,
            id                     TEXT,
            toid                   TEXT,
            type                   TEXT,
            ds_id                  REAL,
            areasqkm               REAL,
            vpuid                  TEXT,
            lengthkm               REAL,
            tot_drainage_areasqkm  REAL,
            has_flowline           INTEGER
        )
    """)
    cur.executemany(
        "INSERT INTO divides"
        " (geom, divide_id, id, toid, type, ds_id, areasqkm, vpuid,"
        "  lengthkm, tot_drainage_areasqkm, has_flowline)"
        " VALUES (?,?,?,?,?,?,?,?,?,?,?)",
        [
            (polygon_blob([cat1]), "cat-1", "wb-1", "nex-1", "network", None, 100.0, "03", 10.0, 200.0, 1),
            (polygon_blob([cat2]), "cat-2", "wb-2", "nex-2", "network", None, 200.0, "03", 15.0, 200.0, 1),
            (polygon_blob([cat3]), "cat-3", "wb-3", "nex-1", "network", None, 100.0, "03", 10.0, 100.0, 1),
        ],
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES"
        " ('divides','features','divides','',datetime('now'),-82.0,29.0,-80.0,31.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns VALUES ('divides','geom','POLYGON',4326,0,0)"
    )

    # ── flowpaths (v2.2 schema) ───────────────────────────────────────────────
    # id = wb-* identifier; toid = downstream nexus (same as the divide's toid)
    cur.execute("""
        CREATE TABLE flowpaths (
            fid                    INTEGER PRIMARY KEY AUTOINCREMENT,
            geom                   MULTILINESTRING,
            id                     TEXT NOT NULL,
            toid                   TEXT,
            mainstem               INTEGER,
            hydroseq               INTEGER,
            lengthkm               REAL,
            areasqkm               REAL,
            tot_drainage_areasqkm  REAL,
            has_divide             INTEGER,
            divide_id              TEXT,
            poi_id                 TEXT,
            vpuid                  TEXT
        )
    """)
    cur.executemany(
        "INSERT INTO flowpaths"
        " (geom, id, toid, mainstem, hydroseq, lengthkm, areasqkm,"
        "  tot_drainage_areasqkm, has_divide, divide_id, poi_id, vpuid)"
        " VALUES (?,?,?,?,?,?,?,?,?,?,?,?)",
        [
            (multilinestring_blob([[(-81.5, 29.5), (-81.0, 30.0)]]),
             "wb-1", "nex-1", 0, 3, 10.0, 100.0, 200.0, 1, "cat-1", None, "03"),
            (multilinestring_blob([[(-80.5, 30.0), (-80.0, 30.0)]]),
             "wb-2", "nex-2", 1, 1, 15.0, 200.0, 200.0, 1, "cat-2", "1",  "03"),
            (multilinestring_blob([[(-81.5, 30.5), (-81.0, 30.0)]]),
             "wb-3", "nex-1", 0, 2, 10.0, 100.0, 100.0, 1, "cat-3", None, "03"),
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


if __name__ == "__main__":
    main()
