-- Derivation script for test/data/geopackage/example_aux.gpkg
--
-- example_aux.gpkg is a copy of example.gpkg (a two-feature "test" layer holding "First"
-- and "Second") with two auxiliary attribute tables added, covering prefixing, per-column
-- type mapping, NULL cells, features without a row, rows without a feature, a non-default
-- key column, and column names shared between two joined tables.
--
-- The tables have the shape of the regionalization tables in the Aug 2026 hydrofabric
-- preview: keyed on a text divide identifier, no "fid" column, and registered in
-- gpkg_contents as 'attributes' with a NULL srs_id and no bounding box.
--
-- To regenerate the fixture from the source directory:
--
--     cp test/data/geopackage/example.gpkg test/data/geopackage/example_aux.gpkg
--     sqlite3 test/data/geopackage/example_aux.gpkg < test/data/geopackage/example_aux.sql

-- Default key column, one column of each supported type, a NULL cell for "First", no row
-- at all for "Second", and a row for "Third", which is not a feature of the layer.
CREATE TABLE "aux_params_one" (
  "divide_id"    TEXT,
  "int_value"    INTEGER,
  "real_value"   REAL,
  "text_value"   TEXT,
  "sparse_value" REAL
);

INSERT INTO "aux_params_one" ("divide_id", "int_value", "real_value", "text_value", "sparse_value")
VALUES
  ('First', 42, 3.5,  'alpha', NULL),
  ('Third', 7,  1.25, 'gamma', 9.75);

-- Non-default key column, full coverage of the layer's features, and two column names
-- shared with the table above, so joining both is only unambiguous because of prefixing.
CREATE TABLE "aux_params_two" (
  "catchment_id" TEXT,
  "real_value"   REAL,
  "text_value"   TEXT,
  "donor_id"     TEXT
);

INSERT INTO "aux_params_two" ("catchment_id", "real_value", "text_value", "donor_id")
VALUES
  ('First',  1.5, 'beta',  'gauge-01'),
  ('Second', 2.5, 'delta', 'gauge-02');

INSERT INTO "gpkg_contents" ("table_name", "data_type", "identifier", "description", "last_change")
VALUES
  ('aux_params_one', 'attributes', 'aux_params_one', 'Auxiliary attribute table fixture', '2026-08-19T00:00:00.000Z'),
  ('aux_params_two', 'attributes', 'aux_params_two', 'Auxiliary attribute table fixture', '2026-08-19T00:00:00.000Z');
