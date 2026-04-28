#!/usr/bin/env python3
"""
Diff a working-tree GeoPackage fixture against its committed (HEAD)
version, producing a structured per-table summary.

The committed `.gpkg` files in this directory are produced by the
neighboring `make_*_fixture.py` scripts and re-running a generator
always produces a tracked diff (see test/data/geopackage/README.md).
`git diff` reports only "Binary files differ", so this tool exists to
let a maintainer verify, before committing, that a regen's diff matches
their intent.

Usage:
    python3 diff_fixture.py <path-to-fixture.gpkg>
    python3 diff_fixture.py example_v3_0.gpkg --aggressive
    python3 diff_fixture.py example_v3_0.gpkg --flat
    python3 diff_fixture.py example_v3_0.gpkg --all

Behavior:
    Default (structured) mode prints a per-table summary. For each table
    that differs, it lists schema changes (CREATE TABLE differences) and
    row-level changes (added / modified / removed, keyed by primary key
    or — if the table has none — by full-row tuple).

    Tables compared by default:
      * user feature tables (e.g. nexus, divides, flowpaths, ...)
      * GeoPackage metadata tables (gpkg_contents, gpkg_geometry_columns,
        gpkg_spatial_ref_sys, ...) — INCLUDING gpkg_contents, so the
        last_change timestamp drift introduced by every regen is
        explicitly visible.

    Tables ignored by default:
      * SQLite internal tables (sqlite_*, rtree_*).

Flags:
    --flat        Switch to a flat unified diff of `sqlite3 .dump`
                  output for both files. Useful when the structured
                  view obscures something you want to see verbatim.
    --aggressive  Also hide rows in `gpkg_contents` whose only
                  difference is the `last_change` column. Useful when
                  the timestamp drift is the only thing in the
                  metadata-table diff and you want to focus on data
                  changes.
    --all         Include SQLite internal tables in the comparison.

Dependencies: Python 3.6+, stdlib only (os, sys, argparse, sqlite3,
subprocess, tempfile, difflib, typing). Requires `git` in PATH and that
the fixture is tracked at HEAD in some ancestor directory's git repo.

Exit status:
    0 — no differences (or only differences hidden by an active flag)
    1 — meaningful differences were printed
    2 — usage error or environment problem (no git repo, file missing,
        etc.)
"""

import argparse
import difflib
import os
import sqlite3
import subprocess
import sys
import tempfile
from typing import Any, List, Optional, Sequence, Tuple


# Tables we never compare in default mode. (gpkg_* are intentionally
# *not* in this set — last_change drift is part of the default view.)
_INTERNAL_PREFIXES: Tuple[str, ...] = ("sqlite_", "rtree_")


# --- git plumbing -----------------------------------------------------

def find_repo_root(start: str) -> str:
    """Walk up from `start` until a directory containing .git/ is found.

    Args:
        start: Filesystem path to start searching from. May be a file
            path or a directory path; only its directory ancestry is
            consulted.

    Returns:
        Absolute path to the enclosing repository's root directory.

    Raises:
        RuntimeError: If no enclosing git repository exists.
    """
    path = os.path.abspath(start)
    while True:
        if os.path.isdir(os.path.join(path, ".git")):
            return path
        parent = os.path.dirname(path)
        if parent == path:
            raise RuntimeError(
                "could not find a git repository containing %r" % start
            )
        path = parent


def fetch_head_blob(repo_root: str, abs_path: str) -> bytes:
    """Run `git show HEAD:<rel>` and return the file's bytes at HEAD.

    Args:
        repo_root: Absolute path to the git repository's root, as
            returned by `find_repo_root`.
        abs_path: Absolute path to the file in the working tree whose
            HEAD version is wanted.

    Returns:
        The raw bytes of the file as it exists at HEAD.

    Raises:
        RuntimeError: If `git show` fails (most commonly because the
            file is not tracked at HEAD).
    """
    rel = os.path.relpath(abs_path, repo_root).replace(os.sep, "/")
    try:
        return subprocess.check_output(
            ["git", "-C", repo_root, "show", "HEAD:" + rel],
            stderr=subprocess.PIPE,
        )
    except subprocess.CalledProcessError as exc:
        msg = exc.stderr.decode("utf-8", "replace").strip()
        raise RuntimeError("git show failed: " + msg)


# --- schema / row introspection --------------------------------------

def list_tables(conn: sqlite3.Connection, include_internal: bool) -> List[str]:
    """Return the sorted list of table names in `conn`.

    Args:
        conn: Open SQLite connection to inspect.
        include_internal: If True, include SQLite internal tables
            (`sqlite_*`, `rtree_*`); if False, omit them.

    Returns:
        Alphabetically sorted list of table names.
    """
    rows = conn.execute(
        "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name"
    ).fetchall()
    names: List[str] = [r[0] for r in rows]
    if not include_internal:
        names = [
            n for n in names
            if not any(n.startswith(p) for p in _INTERNAL_PREFIXES)
        ]
    return names


def get_create_sql(conn: sqlite3.Connection, table: str) -> str:
    """Return the CREATE TABLE SQL recorded in sqlite_master for `table`.

    Args:
        conn: Open SQLite connection.
        table: Name of the table whose creation SQL is wanted.

    Returns:
        The SQL string from sqlite_master, or empty string if the
        table does not exist.
    """
    row = conn.execute(
        "SELECT sql FROM sqlite_master WHERE type='table' AND name=?",
        (table,),
    ).fetchone()
    return row[0] if row else ""


def get_columns(conn: sqlite3.Connection, table: str) -> List[str]:
    """Return the column names of `table` in declared order.

    Args:
        conn: Open SQLite connection.
        table: Name of the table to introspect.

    Returns:
        List of column names in the order PRAGMA table_info reports
        them (i.e. declared order).
    """
    rows = conn.execute("PRAGMA table_info('%s')" % table).fetchall()
    return [r[1] for r in rows]


def get_pk_indices(
    conn: sqlite3.Connection,
    table: str,
    columns: Sequence[str],
) -> Tuple[List[int], bool]:
    """Return indices into `columns` that form the primary key.

    Falls back to all columns if the table has no declared PK — this
    keys row identity by full content, which is correct for diff
    purposes (added/removed) but flags every modification as
    add+remove rather than as a modify. Acceptable degradation.

    Args:
        conn: Open SQLite connection.
        table: Name of the table to inspect.
        columns: The column names returned by `get_columns(conn, table)`,
            in declared order; used to translate PK column names back
            into positional indices.

    Returns:
        A pair `(indices, has_pk)`:
          * `indices`: positions within `columns` that comprise the PK
            (or `range(len(columns))` if the table has no PK).
          * `has_pk`: True iff a primary key was declared.
    """
    rows = conn.execute("PRAGMA table_info('%s')" % table).fetchall()
    # PRAGMA table_info returns: (cid, name, type, notnull, dflt_value, pk)
    pk_pairs = sorted(
        ((r[5], r[1]) for r in rows if r[5] > 0),
        key=lambda x: x[0],
    )
    if not pk_pairs:
        return list(range(len(columns))), False
    pk_names = [name for _, name in pk_pairs]
    return [columns.index(n) for n in pk_names], True


def fetch_rows(
    conn: sqlite3.Connection,
    table: str,
    columns: Sequence[str],
) -> List[Tuple[Any, ...]]:
    """Fetch all rows of `table`, projecting `columns` in order.

    Args:
        conn: Open SQLite connection.
        table: Name of the table to read.
        columns: Column names to select, in the desired output order.

    Returns:
        List of row tuples, in storage order. (No ORDER BY is applied;
        callers should not rely on row order.)
    """
    quoted = ", ".join('"%s"' % c for c in columns)
    return list(conn.execute("SELECT %s FROM %s" % (quoted, table)))


# --- rendering helpers -----------------------------------------------

def render_value(v: Any) -> str:
    """Render a single column value for human display."""
    if isinstance(v, (bytes, bytearray, memoryview)):
        return "<blob %d bytes>" % len(bytes(v))
    return repr(v)


def render_key(key: Tuple[Any, ...]) -> str:
    """Render a row's PK tuple as a compact identifier for diff output."""
    if len(key) == 1:
        return render_value(key[0])
    return "(" + ", ".join(render_value(v) for v in key) + ")"


# --- structured diff core --------------------------------------------

def diff_table(
    name: str,
    head_conn: sqlite3.Connection,
    work_conn: sqlite3.Connection,
    aggressive: bool,
) -> List[str]:
    """Diff one table between HEAD and the working tree.

    Args:
        name: Table name (must exist in at least one of the two
            connections).
        head_conn: Connection to the HEAD-version copy of the fixture.
        work_conn: Connection to the working-tree fixture.
        aggressive: If True, hide `gpkg_contents` rows whose only
            differing column is `last_change`.

    Returns:
        A list of human-readable diff lines (without trailing newlines).
        An empty list means no differences were observed. Schema
        differences and row differences are reported in that order.
    """
    out: List[str] = []

    head_sql = get_create_sql(head_conn, name)
    work_sql = get_create_sql(work_conn, name)

    if head_sql and not work_sql:
        return ["  Table '%s': REMOVED" % name]
    if work_sql and not head_sql:
        return ["  Table '%s': ADDED" % name]

    schema_changed = head_sql != work_sql

    head_cols = get_columns(head_conn, name)
    work_cols = get_columns(work_conn, name)

    if schema_changed:
        out.append("  Table '%s': schema changed" % name)
        for line in difflib.unified_diff(
            head_sql.splitlines(),
            work_sql.splitlines(),
            fromfile="HEAD",
            tofile="working",
            lineterm="",
        ):
            out.append("    " + line)

    # Row diff requires a stable column set. If columns differ between
    # head and working, fall back to advising --flat for this table.
    if head_cols != work_cols:
        out.append(
            "    (column set differs; rerun with --flat for verbatim diff)"
        )
        return out

    columns = work_cols
    pk_idx, has_pk = get_pk_indices(work_conn, name, columns)

    head_rows = fetch_rows(head_conn, name, columns)
    work_rows = fetch_rows(work_conn, name, columns)

    def key_of(row: Tuple[Any, ...]) -> Tuple[Any, ...]:
        return tuple(row[i] for i in pk_idx)

    head_by_key = {key_of(r): r for r in head_rows}
    work_by_key = {key_of(r): r for r in work_rows}

    added = [k for k in work_by_key if k not in head_by_key]
    removed = [k for k in head_by_key if k not in work_by_key]
    common = [k for k in work_by_key if k in head_by_key]

    modified: List[Tuple[Tuple[Any, ...], List[Tuple[int, str]]]] = []
    last_change_only: List[Tuple[Any, ...]] = []
    for k in common:
        h = head_by_key[k]
        w = work_by_key[k]
        if h == w:
            continue
        diffs = [(i, columns[i]) for i in range(len(columns)) if h[i] != w[i]]
        if (
            aggressive
            and name == "gpkg_contents"
            and all(c == "last_change" for _, c in diffs)
        ):
            last_change_only.append(k)
            continue
        modified.append((k, diffs))

    if not (added or removed or modified or schema_changed):
        return []

    if added or removed or modified:
        out.append(
            "  Table '%s': +%d -%d ~%d (rows added/removed/modified)"
            % (name, len(added), len(removed), len(modified))
        )
        for k in sorted(added):
            row = work_by_key[k]
            out.append(
                "    + %s -> %s"
                % (render_key(k), tuple(render_value(v) for v in row))
            )
        for k in sorted(removed):
            row = head_by_key[k]
            out.append(
                "    - %s -> %s"
                % (render_key(k), tuple(render_value(v) for v in row))
            )
        for k, diffs in sorted(modified, key=lambda x: x[0]):
            h = head_by_key[k]
            w = work_by_key[k]
            for col_i, col_name in diffs:
                out.append(
                    "    ~ %s.%s: %s -> %s" % (
                        render_key(k),
                        col_name,
                        render_value(h[col_i]),
                        render_value(w[col_i]),
                    )
                )

    if last_change_only:
        out.append(
            "    (suppressed %d gpkg_contents row(s) whose only diff was "
            "last_change; rerun without --aggressive to see them)"
            % len(last_change_only)
        )

    if not has_pk and (added or removed or modified):
        out.append(
            "    (note: '%s' has no primary key; rows keyed by "
            "full-content tuple)" % name
        )

    return out


def structured_diff(
    work_path: str,
    head_blob: bytes,
    aggressive: bool,
    include_internal: bool,
) -> int:
    """Print a structured per-table diff and return an exit status.

    Args:
        work_path: Path to the working-tree `.gpkg` file.
        head_blob: Bytes of the same path's HEAD version (as returned
            by `fetch_head_blob`); written to a tempfile and opened
            read-only as the "before" side.
        aggressive: If True, hide `gpkg_contents` rows whose only
            differing column is `last_change`.
        include_internal: If True, also compare SQLite internal tables.

    Returns:
        0 if nothing meaningful differed (either truly identical, or
        the only differences were suppressed by `aggressive`); 1 if
        any differences were printed.
    """
    head_fd, head_path = tempfile.mkstemp(suffix=".gpkg")
    os.close(head_fd)
    head_conn: Optional[sqlite3.Connection] = None
    work_conn: Optional[sqlite3.Connection] = None
    try:
        with open(head_path, "wb") as f:
            f.write(head_blob)

        head_conn = sqlite3.connect(head_path)
        work_conn = sqlite3.connect(work_path)

        head_tables = set(list_tables(head_conn, include_internal))
        work_tables = set(list_tables(work_conn, include_internal))
        all_tables = sorted(head_tables | work_tables)

        all_lines: List[str] = []
        for t in all_tables:
            all_lines.extend(diff_table(t, head_conn, work_conn, aggressive))

        if not all_lines:
            print("No differences in compared tables.")
            return 0

        print("Differences vs HEAD (working tree → committed):")
        for line in all_lines:
            print(line)
        return 1
    finally:
        if head_conn is not None:
            head_conn.close()
        if work_conn is not None:
            work_conn.close()
        try:
            os.unlink(head_path)
        except OSError:
            pass


# --- flat diff -------------------------------------------------------

def _sqlite3_dump(path: str) -> str:
    """Return a deterministic SQL dump of the database at `path`.

    Uses the python sqlite3 module's iterdump() so that no `sqlite3`
    CLI binary needs to be installed.

    Args:
        path: Filesystem path to a SQLite database file.

    Returns:
        The full dump as a single string with `\\n` separators between
        statements.
    """
    conn = sqlite3.connect(path)
    try:
        return "\n".join(conn.iterdump())
    finally:
        conn.close()


def flat_diff(work_path: str, head_blob: bytes) -> int:
    """Print a flat unified diff of full SQL dumps and return exit status.

    Args:
        work_path: Path to the working-tree `.gpkg`.
        head_blob: Bytes of the HEAD version of the same path.

    Returns:
        0 if dumps are identical, 1 otherwise.
    """
    head_fd, head_path = tempfile.mkstemp(suffix=".gpkg")
    os.close(head_fd)
    try:
        with open(head_path, "wb") as f:
            f.write(head_blob)
        head_dump = _sqlite3_dump(head_path).splitlines()
        work_dump = _sqlite3_dump(work_path).splitlines()
        diff = list(difflib.unified_diff(
            head_dump, work_dump,
            fromfile="HEAD",
            tofile="working",
            lineterm="",
        ))
        if not diff:
            print("No differences (flat dump).")
            return 0
        for line in diff:
            print(line)
        return 1
    finally:
        try:
            os.unlink(head_path)
        except OSError:
            pass


# --- entry point -----------------------------------------------------

def main(argv: Optional[Sequence[str]] = None) -> int:
    """Parse command-line args and dispatch to the chosen diff mode.

    Args:
        argv: Argument list (defaults to `sys.argv[1:]` when None).

    Returns:
        Process exit status (0/1/2 per the module docstring).
    """
    parser = argparse.ArgumentParser(
        description=(
            "Diff a working-tree GeoPackage fixture against its "
            "committed (HEAD) version, table-by-table."
        )
    )
    parser.add_argument(
        "path",
        help="path to a working-tree .gpkg fixture (must be tracked in git)",
    )
    parser.add_argument(
        "--flat", action="store_true",
        help="use a flat unified diff of full SQL dumps instead of the "
             "structured per-table view",
    )
    parser.add_argument(
        "--aggressive", action="store_true",
        help="hide gpkg_contents rows whose only difference is last_change",
    )
    parser.add_argument(
        "--all", action="store_true", dest="include_internal",
        help="also compare SQLite internal tables (sqlite_*, rtree_*)",
    )
    args = parser.parse_args(argv)

    if not os.path.isfile(args.path):
        print("error: file not found: %s" % args.path, file=sys.stderr)
        return 2

    abs_path = os.path.abspath(args.path)
    try:
        repo_root = find_repo_root(os.path.dirname(abs_path))
    except RuntimeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 2

    try:
        head_blob = fetch_head_blob(repo_root, abs_path)
    except RuntimeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 2

    if args.flat:
        return flat_diff(abs_path, head_blob)
    return structured_diff(
        abs_path,
        head_blob,
        aggressive=args.aggressive,
        include_internal=args.include_internal,
    )


if __name__ == "__main__":
    sys.exit(main())
