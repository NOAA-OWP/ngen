#!/usr/bin/env python3
"""
Generate example_v3_0_extra_col.gpkg — a minimal v3.0 GeoPackage used by the
extra-column tolerance regression test.

Topology: cat-1 -> fp-1 -> nex-1 -> coastal-000001 (terminal)

Tables present:
  nexus     (nexus_id, nexus_toid, geom POINT, vpuid)
  divides   (divide_id, geom POLYGON, flowpath_id, vpuid, areasqkm, type,
             has_flowline, ds_id)
  flowpaths (flowpath_id, flowpath_toid, geom LINESTRING, vpuid)
  flowlines (flowline_id, geom MULTILINESTRING, lengthkm)   <- extra column
  pois      (poi_id, geom GEOMETRY, flowpath_id, vpuid)     <- GEOMETRY, not POINT

The loader must not read flowlines or pois. The extra 'lengthkm' column on
flowlines and the GEOMETRY column type on pois verify that auxiliary-table
schema drift does not break the nexus/divides load.
"""

import os
import sqlite3
import struct

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "example_v3_0_extra_col.gpkg")
SRS_ID = 4326


def _hdr():
    return b"\x47\x50\x00\x01" + struct.pack("<i", SRS_ID)


def point_blob(x, y):
    wkb = b"\x01" + struct.pack("<I", 1) + struct.pack("<dd", x, y)
    return _hdr() + wkb


def polygon_blob(rings):
    wkb = b"\x01" + struct.pack("<I", 3) + struct.pack("<I", len(rings))
    for ring in rings:
        wkb += struct.pack("<I", len(ring))
        for x, y in ring:
            wkb += struct.pack("<dd", x, y)
    return _hdr() + wkb


def linestring_blob(points):
    wkb = b"\x01" + struct.pack("<I", 2) + struct.pack("<I", len(points))
    for x, y in points:
        wkb += struct.pack("<dd", x, y)
    return _hdr() + wkb


def multilinestring_blob(lines):
    wkb = b"\x01" + struct.pack("<I", 5) + struct.pack("<I", len(lines))
    for pts in lines:
        wkb += b"\x01" + struct.pack("<I", 2) + struct.pack("<I", len(pts))
        for x, y in pts:
            wkb += struct.pack("<dd", x, y)
    return _hdr() + wkb


def main():
    if os.path.exists(OUT):
        os.remove(OUT)

    db = sqlite3.connect(OUT)
    cur = db.cursor()

    # GPKG required metadata tables
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

    # nexus (v3.0 schema)
    cur.execute("""
        CREATE TABLE nexus (
            fid INTEGER PRIMARY KEY AUTOINCREMENT,
            geom POINT,
            nexus_id TEXT NOT NULL,
            nexus_toid TEXT,
            vpuid TEXT
        )
    """)
    cur.execute(
        "INSERT INTO nexus (geom, nexus_id, nexus_toid, vpuid) VALUES (?,?,?,?)",
        (point_blob(-81.0, 30.0), "nex-1", "coastal-000001", "03"),
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES ('nexus','features','nexus','',"
        "'2026-01-01 00:00:00',-82.0,29.0,-80.0,31.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns VALUES ('nexus','geom','POINT',4326,0,0)"
    )

    # divides (v3.0 schema)
    ring = [
        (-82.0, 29.0), (-81.0, 29.0), (-81.0, 30.0),
        (-82.0, 30.0), (-82.0, 29.0),
    ]
    cur.execute("""
        CREATE TABLE divides (
            fid INTEGER PRIMARY KEY AUTOINCREMENT,
            geom POLYGON,
            divide_id TEXT NOT NULL,
            areasqkm REAL,
            type TEXT,
            has_flowline INTEGER,
            ds_id INTEGER,
            flowpath_id TEXT,
            vpuid TEXT
        )
    """)
    cur.execute(
        "INSERT INTO divides (geom, divide_id, areasqkm, type, has_flowline,"
        " ds_id, flowpath_id, vpuid) VALUES (?,?,?,?,?,?,?,?)",
        (polygon_blob([ring]), "cat-1", 100.0, "divide", 1, 0, "fp-1", "03"),
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES ('divides','features','divides','',"
        "'2026-01-01 00:00:00',-82.0,29.0,-81.0,30.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns VALUES ('divides','geom','POLYGON',4326,0,0)"
    )

    # flowpaths (needed for v3.0 toid synthesis JOIN)
    cur.execute("""
        CREATE TABLE flowpaths (
            fid INTEGER PRIMARY KEY AUTOINCREMENT,
            geom LINESTRING,
            flowpath_id TEXT NOT NULL,
            flowpath_toid TEXT,
            vpuid TEXT
        )
    """)
    cur.execute(
        "INSERT INTO flowpaths (geom, flowpath_id, flowpath_toid, vpuid)"
        " VALUES (?,?,?,?)",
        (linestring_blob([(-81.5, 29.5), (-81.0, 30.0)]), "fp-1", "nex-1", "03"),
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES ('flowpaths','features','flowpaths','',"
        "'2026-01-01 00:00:00',-81.5,29.5,-81.0,30.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns"
        " VALUES ('flowpaths','geom','LINESTRING',4326,0,0)"
    )

    # flowlines — auxiliary table, loader must NOT open this.
    # Extra column 'lengthkm' exercises extra-column tolerance.
    cur.execute("""
        CREATE TABLE flowlines (
            fid INTEGER PRIMARY KEY AUTOINCREMENT,
            geom MULTILINESTRING,
            flowline_id TEXT,
            lengthkm REAL
        )
    """)
    cur.execute(
        "INSERT INTO flowlines (geom, flowline_id, lengthkm) VALUES (?,?,?)",
        (multilinestring_blob([[(-81.5, 29.5), (-81.0, 30.0)]]), "fl-1", 5.0),
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES ('flowlines','features','flowlines','',"
        "'2026-01-01 00:00:00',-81.5,29.5,-81.0,30.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns"
        " VALUES ('flowlines','geom','MULTILINESTRING',4326,0,0)"
    )

    # pois — auxiliary table, geom declared as GEOMETRY (not POINT).
    # Verifies the loader does not fail on GEOMETRY-typed geom columns
    # in tables it does not read.
    cur.execute("""
        CREATE TABLE pois (
            fid INTEGER PRIMARY KEY AUTOINCREMENT,
            geom GEOMETRY,
            poi_id REAL,
            flowpath_id TEXT,
            vpuid TEXT
        )
    """)
    cur.execute(
        "INSERT INTO pois (geom, poi_id, flowpath_id, vpuid) VALUES (?,?,?,?)",
        (point_blob(-81.0, 30.0), 1.0, "fp-1", "03"),
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES ('pois','features','pois','',"
        "'2026-01-01 00:00:00',-81.0,30.0,-81.0,30.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns"
        " VALUES ('pois','geom','GEOMETRY',4326,0,0)"
    )

    db.commit()
    db.close()
    print(f"Written: {OUT}")


if __name__ == "__main__":
    main()
