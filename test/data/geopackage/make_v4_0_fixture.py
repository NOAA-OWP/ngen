#!/usr/bin/env python3
"""
Generate example_v4_0.gpkg — a minimal v4.0 GeoPackage used by the
detect_version and native-toid tests.

This is the release-schema counterpart to example_v4_0beta1.gpkg. Both
fixtures describe the SAME topology, so a test can load each and assert
that the resulting "toid" properties are identical regardless of how the
loader obtained them:

    cat-1 (fp-1) ─┐
                    ├─> nex-1 ─> fp-2 ─> cat-2 (fp-2) ─> nex-2 ─> coastal-000001
    cat-3 (fp-3) ─┘

What makes this v4.0 rather than v4.0beta1 is the `divides.flowpath_toid`
column. beta1 has no such column, so the loader must synthesize "toid" by
joining divides to flowpaths on flowpath_id; v4.0 carries the downstream
nexus natively and the loader reads it straight off the row, never touching
the flowpaths table.

Feature IDs:
  Divides:   cat-1, cat-2, cat-3   (divide_id)
  Nexuses:   nex-1, nex-2          (nexus_id)
  Flowpaths: fp-1, fp-2, fp-3      (flowpath_id)

Schema (v4.0):
  nexus    (fid, geom POINT,          nexus_id, nexus_toid, vpuid)
  divides  (fid, geom POLYGON,        divide_id, areasqkm, has_flowline, ds_id,
                                       type, flowpath_id, flowpath_toid, vpuid,
                                       has_divide)
  flowpaths(fid, geom MULTILINESTRING, flowpath_id, flowpath_toid, rec_id_list,
                                        mainstem_id, member_comid, poi_id,
                                        hydroseq, lengthkm, areasqkm,
                                        total_dasqkm, has_divide, divide_id,
                                        vpuid, streamorder, slope)

The flowpaths column set mirrors the real v4.0 release schema, which renamed
tot_drainage_areasqkm -> total_dasqkm and mainstem -> mainstem_id relative to
beta1. ngen reads none of those columns; they are present so the fixture is a
faithful sample of the format.

Usage:
    python3 make_v4_0_fixture.py

Output: example_v4_0.gpkg (sibling of this script).
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
from typing import List, Sequence, Tuple

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "example_v4_0.gpkg")
SRS_ID = 4326

Point = Tuple[float, float]


def _gpkg_hdr() -> bytes:
    """
    Build the GeoPackage binary header prefixed to every geometry blob.

    :return: The 8-byte GPKG header for this fixture's SRS.
    """
    return b"\x47\x50\x00\x01" + struct.pack("<i", SRS_ID)


def point_blob(x: float, y: float) -> bytes:
    """
    Encode a POINT geometry as a GeoPackage blob.

    :param x: Longitude of the point.
    :param y: Latitude of the point.
    :return: GPKG-encoded POINT geometry.
    """
    wkb = b"\x01" + struct.pack("<I", 1) + struct.pack("<dd", x, y)
    return _gpkg_hdr() + wkb


def polygon_blob(rings: Sequence[Sequence[Point]]) -> bytes:
    """
    Encode a POLYGON geometry as a GeoPackage blob.

    :param rings: Rings of the polygon, each a closed sequence of points.
    :return: GPKG-encoded POLYGON geometry.
    """
    wkb = b"\x01" + struct.pack("<I", 3) + struct.pack("<I", len(rings))
    for ring in rings:
        wkb += struct.pack("<I", len(ring))
        for x, y in ring:
            wkb += struct.pack("<dd", x, y)
    return _gpkg_hdr() + wkb


def multilinestring_blob(lines: Sequence[Sequence[Point]]) -> bytes:
    """
    Encode a MULTILINESTRING geometry as a GeoPackage blob.

    :param lines: Linestrings, each a sequence of points.
    :return: GPKG-encoded MULTILINESTRING geometry.
    """
    wkb = b"\x01" + struct.pack("<I", 5) + struct.pack("<I", len(lines))
    for pts in lines:
        wkb += b"\x01" + struct.pack("<I", 2) + struct.pack("<I", len(pts))
        for x, y in pts:
            wkb += struct.pack("<dd", x, y)
    return _gpkg_hdr() + wkb


def write_metadata_tables(cur: sqlite3.Cursor) -> None:
    """
    Create and populate the GeoPackage-required metadata tables.

    :param cur: Open cursor on the fixture database.
    """
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


def write_nexus(cur: sqlite3.Cursor) -> None:
    """
    Create and populate the v4.0 `nexus` layer.

    The nexus_id column (not 'id') is the v4-family signature detected by
    detect_version; nexus_toid points at the downstream flowpath, or a
    coastal-* sentinel for a terminal nexus.

    :param cur: Open cursor on the fixture database.
    """
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
            (point_blob(-81.0, 30.0), "nex-1", "fp-2", "03"),
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


def write_divides(cur: sqlite3.Cursor) -> None:
    """
    Create and populate the v4.0 `divides` layer.

    The native flowpath_toid column is what distinguishes v4.0 from
    v4.0beta1: the loader aliases it straight to "toid" with no join.
    flowpath_id is retained because real v4.0 files carry it, but the
    loader no longer requires it on this variant.

    :param cur: Open cursor on the fixture database.
    """
    cat1 = [(-82.0, 29.0), (-81.0, 29.0), (-81.0, 30.0), (-82.0, 30.0), (-82.0, 29.0)]
    cat2 = [(-81.0, 29.0), (-80.0, 29.0), (-80.0, 31.0), (-81.0, 31.0), (-81.0, 29.0)]
    cat3 = [(-82.0, 30.0), (-81.0, 30.0), (-81.0, 31.0), (-82.0, 31.0), (-82.0, 30.0)]
    cur.execute("""
        CREATE TABLE divides (
            fid           INTEGER PRIMARY KEY AUTOINCREMENT,
            geom          POLYGON,
            divide_id     TEXT NOT NULL,
            areasqkm      REAL,
            has_flowline  BOOLEAN,
            ds_id         TEXT,
            type          TEXT,
            flowpath_id   TEXT,
            flowpath_toid TEXT,
            vpuid         TEXT,
            has_divide    INTEGER
        )
    """)
    cur.executemany(
        "INSERT INTO divides"
        " (geom, divide_id, areasqkm, has_flowline, ds_id, type, flowpath_id,"
        "  flowpath_toid, vpuid, has_divide)"
        " VALUES (?,?,?,?,?,?,?,?,?,?)",
        [
            (polygon_blob([cat1]), "cat-1", 100.0, 1, None, "network", "fp-1", "nex-1", "03", 1),
            (polygon_blob([cat2]), "cat-2", 200.0, 1, None, "network", "fp-2", "nex-2", "03", 1),
            (polygon_blob([cat3]), "cat-3", 100.0, 1, None, "network", "fp-3", "nex-1", "03", 1),
        ],
    )
    cur.execute(
        "INSERT INTO gpkg_contents VALUES"
        " ('divides','features','divides','',datetime('now'),-82.0,29.0,-80.0,31.0,4326)"
    )
    cur.execute(
        "INSERT INTO gpkg_geometry_columns VALUES ('divides','geom','POLYGON',4326,0,0)"
    )


def write_flowpaths(cur: sqlite3.Cursor) -> None:
    """
    Create and populate the v4.0 `flowpaths` layer.

    ngen never reads this table on a v4.0 file — toid comes from divides.
    It is present so the fixture is a faithful sample of the format, and so
    a test can prove the loader's answer matches what a divides-to-flowpaths
    join would have produced.

    :param cur: Open cursor on the fixture database.
    """
    cur.execute("""
        CREATE TABLE flowpaths (
            fid           INTEGER PRIMARY KEY AUTOINCREMENT,
            geom          MULTILINESTRING,
            flowpath_id   TEXT NOT NULL,
            flowpath_toid TEXT,
            rec_id_list   TEXT,
            mainstem_id   REAL,
            member_comid  TEXT,
            poi_id        REAL,
            hydroseq      REAL,
            lengthkm      REAL,
            areasqkm      REAL,
            total_dasqkm  REAL,
            has_divide    REAL,
            divide_id     TEXT,
            vpuid         TEXT,
            streamorder   REAL,
            slope         REAL
        )
    """)
    cur.executemany(
        "INSERT INTO flowpaths"
        " (geom, flowpath_id, flowpath_toid, rec_id_list, mainstem_id, member_comid,"
        "  poi_id, hydroseq, lengthkm, areasqkm, total_dasqkm, has_divide, divide_id,"
        "  vpuid, streamorder, slope)"
        " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        [
            (multilinestring_blob([[(-81.5, 29.5), (-81.0, 30.0)]]),
             "fp-1", "nex-1", "1", 1.0, "1000001", None, 3.0, 10.0, 100.0, 200.0, 1.0,
             "cat-1", "03", 1.0, 0.001),
            (multilinestring_blob([[(-80.5, 30.0), (-80.0, 30.0)]]),
             "fp-2", "nex-2", "2", 1.0, "1000002", 1.0, 1.0, 15.0, 200.0, 200.0, 1.0,
             "cat-2", "03", 2.0, 0.002),
            (multilinestring_blob([[(-81.5, 30.5), (-81.0, 30.0)]]),
             "fp-3", "nex-1", "3", 1.0, "1000003", None, 2.0, 10.0, 100.0, 100.0, 1.0,
             "cat-3", "03", 1.0, 0.001),
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


def verify(path: str) -> None:
    """
    Assert the native toid column agrees with the beta1 join it replaces.

    This is the invariant that lets example_v4_0.gpkg and
    example_v4_0beta1.gpkg be compared directly in tests.

    :param path: Path to the freshly written fixture.
    :raises AssertionError: If the native column and the join disagree.
    """
    db = sqlite3.connect(path)
    native = dict(db.execute("SELECT divide_id, flowpath_toid FROM divides"))
    joined = dict(db.execute(
        "SELECT d.divide_id, f.flowpath_toid"
        " FROM divides d JOIN flowpaths f ON d.flowpath_id = f.flowpath_id"
    ))
    db.close()

    expected = {"cat-1": "nex-1", "cat-2": "nex-2", "cat-3": "nex-1"}
    assert native == expected, f"Unexpected native toids: {native}"
    assert native == joined, f"Native toid disagrees with join: {native} vs {joined}"
    print("Native toid verified: matches the beta1 divides->flowpaths join.")


def main() -> None:
    """Write the fixture to OUT, replacing any existing file."""
    if os.path.exists(OUT):
        os.remove(OUT)

    db = sqlite3.connect(OUT)
    db.execute("PRAGMA page_size=512")
    cur = db.cursor()

    write_metadata_tables(cur)
    write_nexus(cur)
    write_divides(cur)
    write_flowpaths(cur)

    db.commit()
    db.close()
    print(f"Written: {OUT}  ({os.path.getsize(OUT)} bytes)")

    verify(OUT)


if __name__ == "__main__":
    main()
