#include "fixture_builders.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>
#include <sqlite3.h>

namespace ngen {
namespace hydrofabric {
namespace fixtures {

namespace {

//! SRS every fixture geometry is written in.
constexpr int SRS_ID = 4326;

//! Fixed `gpkg_contents.last_change` value.
//!
//! Real GeoPackage writers stamp the wall clock here. A constant keeps a
//! rebuilt fixture byte-identical to the last one, so a test that behaves
//! differently between runs is never explained away as fixture drift.
constexpr const char* LAST_CHANGE = "2026-01-01T00:00:00.000Z";

using bytes = std::vector<unsigned char>;

/**
 * Append a 32-bit unsigned value in little-endian order.
 *
 * @param[in,out] out Buffer to append to
 * @param[in] value Value to encode
 */
void append_le(bytes& out, const std::uint32_t value)
{
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<unsigned char>((value >> (8 * i)) & 0xFF));
    }
}

/**
 * Append an IEEE 754 double in little-endian order.
 *
 * @param[in,out] out Buffer to append to
 * @param[in] value Value to encode
 */
void append_le(bytes& out, const double value)
{
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<unsigned char>((bits >> (8 * i)) & 0xFF));
    }
}

/**
 * Start a geometry blob: the GeoPackage binary header, then the WKB
 * byte-order flag and geometry type.
 *
 * @param[in] wkb_type WKB geometry type code (1 point, 2 linestring,
 *            3 polygon, 5 multilinestring)
 * @return Buffer holding the header, ready for the geometry's own contents
 */
bytes begin_geometry(const std::uint32_t wkb_type)
{
    bytes out{0x47, 0x50, 0x00, 0x01};   // "GP", version 0, flags: no envelope
    append_le(out, static_cast<std::uint32_t>(SRS_ID));
    out.push_back(0x01);                 // WKB little-endian
    append_le(out, wkb_type);
    return out;
}

//! A single coordinate pair, in the fixture's SRS.
struct point
{
    double x;
    double y;
};

/**
 * Encode a POINT geometry.
 *
 * @param[in] x Longitude
 * @param[in] y Latitude
 * @return GeoPackage geometry blob
 */
bytes point_blob(const double x, const double y)
{
    bytes out = begin_geometry(1);
    append_le(out, x);
    append_le(out, y);
    return out;
}

/**
 * Append a point sequence: its length, then each coordinate pair.
 *
 * @param[in,out] out Buffer to append to
 * @param[in] points Points of the sequence
 */
void append_points(bytes& out, const std::vector<point>& points)
{
    append_le(out, static_cast<std::uint32_t>(points.size()));
    for (const auto& p : points) {
        append_le(out, p.x);
        append_le(out, p.y);
    }
}

/**
 * Encode a LINESTRING geometry.
 *
 * @param[in] points Points of the line
 * @return GeoPackage geometry blob
 */
bytes linestring_blob(const std::vector<point>& points)
{
    bytes out = begin_geometry(2);
    append_points(out, points);
    return out;
}

/**
 * Encode a POLYGON geometry.
 *
 * @param[in] rings Rings of the polygon, each closed (first point repeated
 *            as the last)
 * @return GeoPackage geometry blob
 */
bytes polygon_blob(const std::vector<std::vector<point>>& rings)
{
    bytes out = begin_geometry(3);
    append_le(out, static_cast<std::uint32_t>(rings.size()));
    for (const auto& ring : rings) {
        append_points(out, ring);
    }
    return out;
}

/**
 * Encode a MULTILINESTRING geometry.
 *
 * @param[in] lines Component linestrings
 * @return GeoPackage geometry blob
 */
bytes multilinestring_blob(const std::vector<std::vector<point>>& lines)
{
    bytes out = begin_geometry(5);
    append_le(out, static_cast<std::uint32_t>(lines.size()));
    for (const auto& line : lines) {
        out.push_back(0x01);
        append_le(out, static_cast<std::uint32_t>(2));   // linestring
        append_points(out, line);
    }
    return out;
}

/**
 * A GeoPackage being written.
 *
 * Wraps an open, writable sqlite3 handle and turns any SQLite failure into
 * an exception, so a builder reads as a list of statements rather than a
 * list of return-code checks.
 */
class writer
{
  public:
    /**
     * Create (or replace) the database at @p path and populate the
     * GeoPackage metadata tables every fixture needs.
     *
     * @param[in] path File to write
     * @throws std::runtime_error if the file cannot be opened for writing
     */
    explicit writer(const std::string& path)
      : path_(path)
    {
        std::remove(path.c_str());
        if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
            const std::string msg = db_ == nullptr ? "out of memory" : sqlite3_errmsg(db_);
            sqlite3_close(db_);
            throw std::runtime_error("fixture " + path + ": cannot open for writing: " + msg);
        }
        write_metadata_tables();
    }

    ~writer()
    {
        sqlite3_close(db_);
    }

    writer(const writer&)            = delete;
    writer& operator=(const writer&) = delete;

    /**
     * Run one statement with no parameters.
     *
     * @param[in] sql Statement to execute
     * @throws std::runtime_error if SQLite rejects the statement
     */
    void exec(const std::string& sql)
    {
        char* errmsg = nullptr;
        if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
            const std::string msg = errmsg == nullptr ? "unknown error" : errmsg;
            sqlite3_free(errmsg);
            throw std::runtime_error("fixture " + path_ + ": " + msg + " in: " + sql);
        }
    }

    /**
     * Run one INSERT whose single bound parameter is a geometry blob.
     *
     * Everything but the geometry is written as a SQL literal, so each row
     * in a builder reads in the same order as the schema above it.
     *
     * @param[in] sql INSERT statement with exactly one `?` placeholder, for
     *            the geometry column
     * @param[in] geometry Geometry blob to bind
     * @throws std::runtime_error if SQLite rejects the statement
     */
    void insert(const std::string& sql, const bytes& geometry)
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(
                "fixture " + path_ + ": " + sqlite3_errmsg(db_) + " in: " + sql
            );
        }

        const int bind_code = sqlite3_bind_blob(
            stmt, 1, geometry.data(), static_cast<int>(geometry.size()), SQLITE_TRANSIENT
        );
        if (bind_code != SQLITE_OK) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("fixture " + path_ + ": cannot bind geometry in: " + sql);
        }

        const int step_code = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (step_code != SQLITE_DONE) {
            throw std::runtime_error(
                "fixture " + path_ + ": " + sqlite3_errmsg(db_) + " in: " + sql
            );
        }
    }

    /**
     * Register a layer in the GeoPackage metadata tables.
     *
     * A layer the loader can find has a row in both `gpkg_contents` and
     * `gpkg_geometry_columns`; the latter is where the loader looks up which
     * column holds the geometry.
     *
     * @param[in] table Layer's table name
     * @param[in] geometry_type Declared geometry type, e.g. "POLYGON"
     * @param[in] min_x Bounding box minimum longitude
     * @param[in] min_y Bounding box minimum latitude
     * @param[in] max_x Bounding box maximum longitude
     * @param[in] max_y Bounding box maximum latitude
     */
    void register_layer(
        const std::string& table,
        const std::string& geometry_type,
        const double min_x,
        const double min_y,
        const double max_x,
        const double max_y
    )
    {
        exec(
            "INSERT INTO gpkg_contents VALUES ('" + table + "','features','" + table + "','','"
            + LAST_CHANGE + "'," + std::to_string(min_x) + "," + std::to_string(min_y) + ","
            + std::to_string(max_x) + "," + std::to_string(max_y) + ",4326)"
        );
        exec(
            "INSERT INTO gpkg_geometry_columns VALUES ('" + table + "','geom','"
            + geometry_type + "',4326,0,0)"
        );
    }

    /**
     * The path this writer is writing to.
     *
     * @return Fixture file path
     */
    const std::string& path() const
    {
        return path_;
    }

  private:
    /**
     * Create and populate the metadata tables the GeoPackage spec requires.
     */
    void write_metadata_tables()
    {
        exec(
            "CREATE TABLE gpkg_spatial_ref_sys ("
            "  srs_name TEXT NOT NULL,"
            "  srs_id INTEGER NOT NULL PRIMARY KEY,"
            "  organization TEXT NOT NULL,"
            "  organization_coordsys_id INTEGER NOT NULL,"
            "  definition TEXT NOT NULL,"
            "  description TEXT"
            ")"
        );
        exec(
            "INSERT INTO gpkg_spatial_ref_sys VALUES"
            " ('Undefined Cartesian SRS',-1,'NONE',-1,'undefined',NULL),"
            " ('Undefined geographic SRS',0,'NONE',0,'undefined',NULL),"
            " ('WGS 84 geodetic',4326,'EPSG',4326,"
            "  'GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\","
            "SPHEROID[\"WGS 84\",6378137,298.257223563]],"
            "PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]',NULL)"
        );
        exec(
            "CREATE TABLE gpkg_contents ("
            "  table_name TEXT NOT NULL PRIMARY KEY,"
            "  data_type TEXT NOT NULL,"
            "  identifier TEXT,"
            "  description TEXT,"
            "  last_change TEXT,"
            "  min_x REAL, min_y REAL, max_x REAL, max_y REAL,"
            "  srs_id INTEGER"
            ")"
        );
        exec(
            "CREATE TABLE gpkg_geometry_columns ("
            "  table_name TEXT NOT NULL,"
            "  column_name TEXT NOT NULL,"
            "  geometry_type_name TEXT NOT NULL,"
            "  srs_id INTEGER NOT NULL,"
            "  z TINYINT NOT NULL,"
            "  m TINYINT NOT NULL,"
            "  CONSTRAINT pk_geom_cols PRIMARY KEY (table_name, column_name)"
            ")"
        );
    }

    std::string path_;
    sqlite3*    db_ = nullptr;
};

/**
 * Absolute path to write a fixture of the given name to.
 *
 * @param[in] name File name of the fixture
 * @return Path under the test temp directory
 */
std::string fixture_path(const std::string& name)
{
    return std::string(::testing::TempDir()) + "/" + name;
}

// The shared base topology. cat-1 and cat-3 meet at nex-1, which drains
// through cat-2 to the terminal nex-2; the v2.2, v4.0beta1 and v4.0 fixtures
// all describe it, so their loaded toids can be compared to each other.
const std::vector<point> CAT_1{{-82.0, 29.0}, {-81.0, 29.0}, {-81.0, 30.0}, {-82.0, 30.0}, {-82.0, 29.0}};
const std::vector<point> CAT_2{{-81.0, 29.0}, {-80.0, 29.0}, {-80.0, 31.0}, {-81.0, 31.0}, {-81.0, 29.0}};
const std::vector<point> CAT_3{{-82.0, 30.0}, {-81.0, 30.0}, {-81.0, 31.0}, {-82.0, 31.0}, {-82.0, 30.0}};

const std::vector<point> FP_1{{-81.5, 29.5}, {-81.0, 30.0}};
const std::vector<point> FP_2{{-80.5, 30.0}, {-80.0, 30.0}};
const std::vector<point> FP_3{{-81.5, 30.5}, {-81.0, 30.0}};

/**
 * Write the `nexus` layer shared by every v4 fixture with the base topology.
 *
 * @param[in,out] db Fixture being written
 */
void write_v4_nexus(writer& db)
{
    // nexus_id (rather than id) is the v4-family signature the loader detects
    // on; nexus_toid names the downstream flowpath, or a coastal-* sentinel
    // where the nexus is terminal.
    db.exec(
        "CREATE TABLE nexus ("
        "  fid        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom       POINT,"
        "  nexus_id   TEXT NOT NULL,"
        "  nexus_toid TEXT,"
        "  vpuid      TEXT"
        ")"
    );
    db.insert(
        "INSERT INTO nexus (geom, nexus_id, nexus_toid, vpuid)"
        " VALUES (?,'nex-1','fp-2','03')",
        point_blob(-81.0, 30.0)
    );
    db.insert(
        "INSERT INTO nexus (geom, nexus_id, nexus_toid, vpuid)"
        " VALUES (?,'nex-2','coastal-000001','03')",
        point_blob(-80.0, 30.0)
    );
    db.register_layer("nexus", "POINT", -82.0, 29.0, -80.0, 31.0);
}

/**
 * Write the `divides` layer shared by the v4.0beta1 fixtures with the base
 * topology.
 *
 * @param[in,out] db Fixture being written
 */
void write_v4_0beta1_divides(writer& db)
{
    // No downstream column of its own: a divide names the flowpath it
    // contains, and the loader follows that to the nexus.
    db.exec(
        "CREATE TABLE divides ("
        "  fid          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom         POLYGON,"
        "  divide_id    TEXT NOT NULL,"
        "  areasqkm     REAL,"
        "  has_flowline BOOLEAN,"
        "  ds_id        BOOLEAN,"
        "  type         TEXT,"
        "  vpuid        TEXT,"
        "  flowpath_id  TEXT"
        ")"
    );
    const std::string insert_sql =
        "INSERT INTO divides"
        " (geom, divide_id, areasqkm, has_flowline, ds_id, type, vpuid, flowpath_id)"
        " VALUES (?,";
    db.insert(insert_sql + "'cat-1',100.0,1,0,'divide','03','fp-1')", polygon_blob({CAT_1}));
    db.insert(insert_sql + "'cat-2',200.0,1,0,'divide','03','fp-2')", polygon_blob({CAT_2}));
    db.insert(insert_sql + "'cat-3',100.0,1,0,'divide','03','fp-3')", polygon_blob({CAT_3}));
    db.register_layer("divides", "POLYGON", -82.0, 29.0, -80.0, 31.0);
}

/**
 * Write the `divides` layer shared by the v4.0 fixtures with the base topology.
 *
 * @param[in,out] db Fixture being written
 */
void write_v4_0_divides(writer& db)
{
    // The native flowpath_toid column is what separates v4.0 from beta1: the
    // loader aliases it straight to "toid" and never opens flowpaths.
    db.exec(
        "CREATE TABLE divides ("
        "  fid           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom          POLYGON,"
        "  divide_id     TEXT NOT NULL,"
        "  areasqkm      REAL,"
        "  has_flowline  BOOLEAN,"
        "  ds_id         TEXT,"
        "  type          TEXT,"
        "  flowpath_id   TEXT,"
        "  flowpath_toid TEXT,"
        "  vpuid         TEXT,"
        "  has_divide    INTEGER"
        ")"
    );
    const std::string divides_sql =
        "INSERT INTO divides"
        " (geom, divide_id, areasqkm, has_flowline, ds_id, type, flowpath_id,"
        "  flowpath_toid, vpuid, has_divide)"
        " VALUES (?,";
    db.insert(divides_sql + "'cat-1',100.0,1,NULL,'network','fp-1','nex-1','03',1)",
              polygon_blob({CAT_1}));
    db.insert(divides_sql + "'cat-2',200.0,1,NULL,'network','fp-2','nex-2','03',1)",
              polygon_blob({CAT_2}));
    db.insert(divides_sql + "'cat-3',100.0,1,NULL,'network','fp-3','nex-1','03',1)",
              polygon_blob({CAT_3}));
    db.register_layer("divides", "POLYGON", -82.0, 29.0, -80.0, 31.0);
}

} // namespace

std::string write_v2_2()
{
    writer db{fixture_path("example_v2_2.gpkg")};

    // The v2.2 nexus is keyed by plain 'id', which is what tells the loader
    // this is not a v4 file.
    db.exec(
        "CREATE TABLE nexus ("
        "  fid    INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom   POINT,"
        "  id     TEXT NOT NULL,"
        "  toid   TEXT,"
        "  type   TEXT,"
        "  vpuid  TEXT,"
        "  poi_id REAL"
        ")"
    );
    db.insert(
        "INSERT INTO nexus (geom, id, toid, type, vpuid, poi_id)"
        " VALUES (?,'nex-1','wb-2','nexus','03',NULL)",
        point_blob(-81.0, 30.0)
    );
    db.insert(
        "INSERT INTO nexus (geom, id, toid, type, vpuid, poi_id)"
        " VALUES (?,'nex-2','coastal-000001','nexus','03',1.0)",
        point_blob(-80.0, 30.0)
    );
    db.register_layer("nexus", "POINT", -82.0, 29.0, -80.0, 31.0);

    // v2.2 divides carry their downstream nexus directly in 'toid'; 'id' is
    // the flowpath (wb-*) within the divide.
    db.exec(
        "CREATE TABLE divides ("
        "  fid                   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom                  POLYGON,"
        "  divide_id             TEXT NOT NULL,"
        "  id                    TEXT,"
        "  toid                  TEXT,"
        "  type                  TEXT,"
        "  ds_id                 REAL,"
        "  areasqkm              REAL,"
        "  vpuid                 TEXT,"
        "  lengthkm              REAL,"
        "  tot_drainage_areasqkm REAL,"
        "  has_flowline          INTEGER"
        ")"
    );
    const std::string divides_sql =
        "INSERT INTO divides"
        " (geom, divide_id, id, toid, type, ds_id, areasqkm, vpuid, lengthkm,"
        "  tot_drainage_areasqkm, has_flowline)"
        " VALUES (?,";
    db.insert(divides_sql + "'cat-1','wb-1','nex-1','network',NULL,100.0,'03',10.0,200.0,1)",
              polygon_blob({CAT_1}));
    db.insert(divides_sql + "'cat-2','wb-2','nex-2','network',NULL,200.0,'03',15.0,200.0,1)",
              polygon_blob({CAT_2}));
    db.insert(divides_sql + "'cat-3','wb-3','nex-1','network',NULL,100.0,'03',10.0,100.0,1)",
              polygon_blob({CAT_3}));
    db.register_layer("divides", "POLYGON", -82.0, 29.0, -80.0, 31.0);

    db.exec(
        "CREATE TABLE flowpaths ("
        "  fid                   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom                  MULTILINESTRING,"
        "  id                    TEXT NOT NULL,"
        "  toid                  TEXT,"
        "  mainstem              INTEGER,"
        "  hydroseq              INTEGER,"
        "  lengthkm              REAL,"
        "  areasqkm              REAL,"
        "  tot_drainage_areasqkm REAL,"
        "  has_divide            INTEGER,"
        "  divide_id             TEXT,"
        "  poi_id                TEXT,"
        "  vpuid                 TEXT"
        ")"
    );
    const std::string flowpaths_sql =
        "INSERT INTO flowpaths"
        " (geom, id, toid, mainstem, hydroseq, lengthkm, areasqkm,"
        "  tot_drainage_areasqkm, has_divide, divide_id, poi_id, vpuid)"
        " VALUES (?,";
    db.insert(flowpaths_sql + "'wb-1','nex-1',0,3,10.0,100.0,200.0,1,'cat-1',NULL,'03')",
              multilinestring_blob({FP_1}));
    db.insert(flowpaths_sql + "'wb-2','nex-2',1,1,15.0,200.0,200.0,1,'cat-2','1','03')",
              multilinestring_blob({FP_2}));
    db.insert(flowpaths_sql + "'wb-3','nex-1',0,2,10.0,100.0,100.0,1,'cat-3',NULL,'03')",
              multilinestring_blob({FP_3}));
    db.register_layer("flowpaths", "MULTILINESTRING", -81.5, 29.5, -80.0, 30.5);

    return db.path();
}

std::string write_v4_0beta1()
{
    writer db{fixture_path("example_v4_0beta1.gpkg")};

    write_v4_nexus(db);
    write_v4_0beta1_divides(db);

    // flowpath_toid is the divides layer's only route to a nexus on this
    // variant, reached by joining on flowpath_id.
    db.exec(
        "CREATE TABLE flowpaths ("
        "  fid                   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom                  MULTILINESTRING,"
        "  flowpath_id           TEXT NOT NULL,"
        "  flowpath_toid         TEXT,"
        "  flowline_id           TEXT,"
        "  divide_id             TEXT,"
        "  mainstem              MEDIUMINT,"
        "  hydroseq              MEDIUMINT,"
        "  lengthkm              REAL,"
        "  areasqkm              REAL,"
        "  tot_drainage_areasqkm REAL,"
        "  has_divide            BOOLEAN,"
        "  vpuid                 TEXT,"
        "  ibt                   BOOLEAN,"
        "  poi_id                REAL,"
        "  member_comid          TEXT"
        ")"
    );
    const std::string flowpaths_sql =
        "INSERT INTO flowpaths"
        " (geom, flowpath_id, flowpath_toid, flowline_id, divide_id, mainstem,"
        "  hydroseq, lengthkm, areasqkm, tot_drainage_areasqkm, has_divide,"
        "  vpuid, ibt, poi_id, member_comid)"
        " VALUES (?,";
    db.insert(flowpaths_sql + "'fp-1','nex-1','fl-1','cat-1',0,3,10.0,100.0,200.0,1,'03',0,NULL,NULL)",
              multilinestring_blob({FP_1}));
    db.insert(flowpaths_sql + "'fp-2','nex-2','fl-2','cat-2',1,1,15.0,200.0,200.0,1,'03',0,1.0,NULL)",
              multilinestring_blob({FP_2}));
    db.insert(flowpaths_sql + "'fp-3','nex-1','fl-3','cat-3',0,2,10.0,100.0,100.0,1,'03',0,NULL,NULL)",
              multilinestring_blob({FP_3}));
    db.register_layer("flowpaths", "MULTILINESTRING", -81.5, 29.5, -80.0, 30.5);

    return db.path();
}

std::string write_v4_0()
{
    writer db{fixture_path("example_v4_0.gpkg")};

    write_v4_nexus(db);

    write_v4_0_divides(db);

    // Unread by the loader on this variant, and present only so the fixture
    // stays a faithful sample. The release schema renamed beta1's
    // tot_drainage_areasqkm and mainstem to total_dasqkm and mainstem_id.
    db.exec(
        "CREATE TABLE flowpaths ("
        "  fid           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom          MULTILINESTRING,"
        "  flowpath_id   TEXT NOT NULL,"
        "  flowpath_toid TEXT,"
        "  rec_id_list   TEXT,"
        "  mainstem_id   REAL,"
        "  member_comid  TEXT,"
        "  poi_id        REAL,"
        "  hydroseq      REAL,"
        "  lengthkm      REAL,"
        "  areasqkm      REAL,"
        "  total_dasqkm  REAL,"
        "  has_divide    REAL,"
        "  divide_id     TEXT,"
        "  vpuid         TEXT,"
        "  streamorder   REAL,"
        "  slope         REAL"
        ")"
    );
    const std::string flowpaths_sql =
        "INSERT INTO flowpaths"
        " (geom, flowpath_id, flowpath_toid, rec_id_list, mainstem_id, member_comid,"
        "  poi_id, hydroseq, lengthkm, areasqkm, total_dasqkm, has_divide, divide_id,"
        "  vpuid, streamorder, slope)"
        " VALUES (?,";
    db.insert(
        flowpaths_sql + "'fp-1','nex-1','1',1.0,'1000001',NULL,3.0,10.0,100.0,200.0,1.0,"
                        "'cat-1','03',1.0,0.001)",
        multilinestring_blob({FP_1})
    );
    db.insert(
        flowpaths_sql + "'fp-2','nex-2','2',1.0,'1000002',1.0,1.0,15.0,200.0,200.0,1.0,"
                        "'cat-2','03',2.0,0.002)",
        multilinestring_blob({FP_2})
    );
    db.insert(
        flowpaths_sql + "'fp-3','nex-1','3',1.0,'1000003',NULL,2.0,10.0,100.0,100.0,1.0,"
                        "'cat-3','03',1.0,0.001)",
        multilinestring_blob({FP_3})
    );
    db.register_layer("flowpaths", "MULTILINESTRING", -81.5, 29.5, -80.0, 30.5);

    return db.path();
}

std::string write_v4_0_nexus_only()
{
    writer db{fixture_path("example_v4_0_nexus_only.gpkg")};

    write_v4_nexus(db);

    return db.path();
}

std::string write_v4_0_divides_only()
{
    writer db{fixture_path("example_v4_0_divides_only.gpkg")};

    write_v4_0_divides(db);

    return db.path();
}

std::string write_v4_0beta1_minimal()
{
    writer db{fixture_path("example_v4_0beta1_minimal.gpkg")};

    write_v4_nexus(db);
    write_v4_0beta1_divides(db);

    // Only the columns the join actually needs, and no auxiliary layers at
    // all: whatever a real file carries beyond this is not required.
    db.exec(
        "CREATE TABLE flowpaths ("
        "  fid           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom          MULTILINESTRING,"
        "  flowpath_id   TEXT NOT NULL,"
        "  flowpath_toid TEXT,"
        "  divide_id     TEXT,"
        "  vpuid         TEXT"
        ")"
    );
    const std::string flowpaths_sql =
        "INSERT INTO flowpaths (geom, flowpath_id, flowpath_toid, divide_id, vpuid)"
        " VALUES (?,";
    db.insert(flowpaths_sql + "'fp-1','nex-1','cat-1','03')", multilinestring_blob({FP_1}));
    db.insert(flowpaths_sql + "'fp-2','nex-2','cat-2','03')", multilinestring_blob({FP_2}));
    db.insert(flowpaths_sql + "'fp-3','nex-1','cat-3','03')", multilinestring_blob({FP_3}));
    db.register_layer("flowpaths", "MULTILINESTRING", -81.5, 29.5, -80.0, 30.5);

    return db.path();
}

std::string write_v4_0beta1_dangling()
{
    writer db{fixture_path("example_v4_0beta1_dangling.gpkg")};

    // One nexus, terminal, present mainly so the file is detected as v4.
    db.exec(
        "CREATE TABLE nexus ("
        "  fid        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom       POINT,"
        "  nexus_id   TEXT NOT NULL,"
        "  nexus_toid TEXT,"
        "  vpuid      TEXT"
        ")"
    );
    db.insert(
        "INSERT INTO nexus (geom, nexus_id, nexus_toid, vpuid)"
        " VALUES (?,'nex-1','coastal-000001','03')",
        point_blob(-81.0, 30.0)
    );
    db.register_layer("nexus", "POINT", -81.0, 30.0, -81.0, 30.0);

    // cat-1 names a flowpath that exists; cat-2 names one that does not.
    const std::vector<point> cat_2{{-81.0, 29.0}, {-80.0, 29.0}, {-80.0, 30.0}, {-81.0, 30.0}, {-81.0, 29.0}};
    db.exec(
        "CREATE TABLE divides ("
        "  fid         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom        POLYGON,"
        "  divide_id   TEXT NOT NULL,"
        "  areasqkm    REAL,"
        "  vpuid       TEXT,"
        "  flowpath_id TEXT"
        ")"
    );
    const std::string divides_sql =
        "INSERT INTO divides (geom, divide_id, areasqkm, vpuid, flowpath_id) VALUES (?,";
    db.insert(divides_sql + "'cat-1',100.0,'03','fp-1')", polygon_blob({CAT_1}));
    db.insert(divides_sql + "'cat-2',100.0,'03','fp-DANGLING')", polygon_blob({cat_2}));
    db.register_layer("divides", "POLYGON", -82.0, 29.0, -80.0, 30.0);

    db.exec(
        "CREATE TABLE flowpaths ("
        "  fid           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom          MULTILINESTRING,"
        "  flowpath_id   TEXT NOT NULL,"
        "  flowpath_toid TEXT,"
        "  vpuid         TEXT"
        ")"
    );
    db.insert(
        "INSERT INTO flowpaths (geom, flowpath_id, flowpath_toid, vpuid)"
        " VALUES (?,'fp-1','nex-1','03')",
        multilinestring_blob({FP_1})
    );
    db.register_layer("flowpaths", "MULTILINESTRING", -81.5, 29.5, -81.0, 30.0);

    return db.path();
}

std::string write_v4_0beta1_extra_col()
{
    writer db{fixture_path("example_v4_0beta1_extra_col.gpkg")};

    db.exec(
        "CREATE TABLE nexus ("
        "  fid        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom       POINT,"
        "  nexus_id   TEXT NOT NULL,"
        "  nexus_toid TEXT,"
        "  vpuid      TEXT"
        ")"
    );
    db.insert(
        "INSERT INTO nexus (geom, nexus_id, nexus_toid, vpuid)"
        " VALUES (?,'nex-1','coastal-000001','03')",
        point_blob(-81.0, 30.0)
    );
    db.register_layer("nexus", "POINT", -82.0, 29.0, -80.0, 31.0);

    db.exec(
        "CREATE TABLE divides ("
        "  fid          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom         POLYGON,"
        "  divide_id    TEXT NOT NULL,"
        "  areasqkm     REAL,"
        "  type         TEXT,"
        "  has_flowline INTEGER,"
        "  ds_id        INTEGER,"
        "  flowpath_id  TEXT,"
        "  vpuid        TEXT"
        ")"
    );
    db.insert(
        "INSERT INTO divides (geom, divide_id, areasqkm, type, has_flowline,"
        " ds_id, flowpath_id, vpuid) VALUES (?,'cat-1',100.0,'divide',1,0,'fp-1','03')",
        polygon_blob({CAT_1})
    );
    db.register_layer("divides", "POLYGON", -82.0, 29.0, -81.0, 30.0);

    // Declared LINESTRING here rather than MULTILINESTRING, which the loader
    // must take as it finds it.
    db.exec(
        "CREATE TABLE flowpaths ("
        "  fid           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom          LINESTRING,"
        "  flowpath_id   TEXT NOT NULL,"
        "  flowpath_toid TEXT,"
        "  vpuid         TEXT"
        ")"
    );
    db.insert(
        "INSERT INTO flowpaths (geom, flowpath_id, flowpath_toid, vpuid)"
        " VALUES (?,'fp-1','nex-1','03')",
        linestring_blob(FP_1)
    );
    db.register_layer("flowpaths", "LINESTRING", -81.5, 29.5, -81.0, 30.0);

    // Auxiliary, never read. The extra 'lengthkm' column is the drift being
    // tolerated here.
    db.exec(
        "CREATE TABLE flowlines ("
        "  fid         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom        MULTILINESTRING,"
        "  flowline_id TEXT,"
        "  lengthkm    REAL"
        ")"
    );
    db.insert(
        "INSERT INTO flowlines (geom, flowline_id, lengthkm) VALUES (?,'fl-1',5.0)",
        multilinestring_blob({FP_1})
    );
    db.register_layer("flowlines", "MULTILINESTRING", -81.5, 29.5, -81.0, 30.0);

    // Auxiliary, never read. Its geometry column is declared GEOMETRY rather
    // than POINT, which the loader must not trip over in a table it skips.
    db.exec(
        "CREATE TABLE pois ("
        "  fid         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  geom        GEOMETRY,"
        "  poi_id      REAL,"
        "  flowpath_id TEXT,"
        "  vpuid       TEXT"
        ")"
    );
    db.insert(
        "INSERT INTO pois (geom, poi_id, flowpath_id, vpuid) VALUES (?,1.0,'fp-1','03')",
        point_blob(-81.0, 30.0)
    );
    db.register_layer("pois", "GEOMETRY", -81.0, 30.0, -81.0, 30.0);

    return db.path();
}

} // namespace fixtures
} // namespace hydrofabric
} // namespace ngen
