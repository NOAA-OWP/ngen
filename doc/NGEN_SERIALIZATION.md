# ngen Serialization

> **Status**: version 0.1 — both save (`NgenSerializationProtocol`) and restore (`NgenDeserializationProtocol`) are implemented and tested in `test_bmi_protocols`.

This document covers ngen's **internals** for checkpoint/restore: the wire
format the engine writes, how the protocol is configured in a realization,
how it plugs into the simulation loop, and how it scales.

For the protocol spec itself — reserved BMI variables, unit conventions,
and what a model implementer needs to provide — see
[`BMI_SERIALIZATION_PROTOCOL.md`](BMI_SERIALIZATION_PROTOCOL.md).

For backend implementation details, see
[`include/utilities/serialization/README.md`](../include/utilities/serialization/README.md).

---

## Engine behavior with non-conforming models

How ngen handles a model that does not implement the protocol is fully
determined by the realization config:

- **No `serialization` block configured.** The engine's
  `checkpoint_state` and create-time restore calls still fire but
  short-circuit immediately on the protocol's "not configured" path.
  No warning, no error — non-conforming models run exactly as they
  did before the protocol existed.
- **`serialization` configured with `fatal: false`.** The engine
  emits a warning on stderr and silently disables checkpointing for
  that module. The rest of the simulation proceeds.
- **`serialization` configured with `fatal: true`** (the default).
  The engine fails fast: `check_support()`'s `INTEGRATION_ERROR` —
  the signal for "the model is loaded but its declared units don't
  match the protocol's reserved variables" — escalates to a
  `PROTOCOL_ERROR` that propagates out of `initialize()` so the
  driver surfaces the misconfiguration rather than silently
  disabling. Other probe failures (such as a null or uninitialized
  model) always stay on the warn-and-disable path and surface at
  the first real `run()` call as an `UNITIALIZED_MODEL` error — the
  escalation is scoped specifically to the conformance check.

The practical implication: a non-conforming model is still usable in
every other mode of operation; the only cost is that any user
requesting checkpoints from that model has to either disable the
protocol (`save.check: false` / `restore.check: false`) or clear
`fatal` to fall back to the warn-and-disable path.

---

## Wire Format

A checkpoint file is a concatenation of records, each laid out as a
fixed-size prefix followed by raw id bytes and raw payload bytes:

```
[ 39-byte prefix ][ id_length bytes of raw id ][ payload_length bytes of opaque payload ]
[ 39-byte prefix ][ id_length bytes of raw id ][ payload_length bytes of opaque payload ]
...
```

External tooling can identify a checkpoint file by inspection: every
record begins with the four ASCII bytes `N`, `G`, `S`, `R`. A hex dump
or `file(1)` matcher needs nothing else to recognize the format.

The prefix layout for `wire_version = 1` (39 bytes, little-endian,
no compiler padding — each field is written byte-by-byte; see
`src/utilities/bmi/wire_format.hpp` for the field-level
implementation):

```
offset    size  field                  type    notes
--------  ----  ---------------------  ------  --------------------------------
[0..4)      4   magic                  uint32  'NGSR' bytes-on-disk (N,G,S,R)
[4..5)      1   wire_version           uint8   prefix-layout version (= 1)
[5..13)     8   time_step              int64   simulation step counter
[13..21)    8   simulation_timestamp   int64   simulated time, signed seconds
                                                since the Unix epoch (1970-01-01 UTC)
[21..29)    8   checkpoint_epoch       int64   wall-clock time of the save event,
                                                signed seconds since Unix epoch
[29..31)    2   id_length              uint16  bytes of id that follow
[31..39)    8   payload_length         uint64  bytes of payload that follow
```

The in-memory record value type mirrors the wire fields:

```cpp
struct SerializationRecord {
    std::string       id;                       // Context::id (e.g. catchment compound id)
    int64_t           time_step            = 0; // simulation step counter
    int64_t           simulation_timestamp = 0; // sim time, signed seconds since Unix epoch
    int64_t           checkpoint_epoch     = 0; // wall-clock time of the save event
    std::vector<char> payload;                  // bytes from GetValue(ngen::serialization_state)
};
```

### Two timestamps, not one

`simulation_timestamp` is the moment in the model's universe being
captured. `checkpoint_epoch` is the wall-clock time at which the
operator triggered the save event. The two values diverge in every
nontrivial run: a reanalysis modeling 1950 saved in 2026 has
`simulation_timestamp` in the past and `checkpoint_epoch` in the
present; a forecast run sees the reverse; a restart re-run shares
`simulation_timestamp` with the original records but carries a later
`checkpoint_epoch`. Storing both per-record makes any single record
self-describing — it can answer both "what simulated moment is this?"
and "when did the operator save this?" without consulting the
surrounding filesystem layout (which not every backend preserves
identically across copy, concatenation, or format migration).

### Wire-format version

The prefix carries exactly one version field, `wire_version`, covering
the prefix-layout itself. Any change to the prefix's fields, widths,
or field semantics (e.g. switching `simulation_timestamp` from seconds
to milliseconds) bumps `wire_version` and readers branch on it to
select the appropriate parser. Currently `wire_version == 1`.

The wire format does **not** carry a payload-schema version. The
payload is opaque to the protocol library — the library never
interprets the bytes, so it cannot enforce or detect a payload-schema
mismatch on behalf of the model. Model authors who want payload
versioning embed their own header inside the payload bytes; see the
"Recommended: defensive payload format" section of
`BMI_SERIALIZATION_PROTOCOL.md`.

### Key properties

- **Single shared file, multiple writers.** All protocol instances
  pointed at the same `path` append to the same file. Cross-instance
  coordination (threads or MPI ranks writing to one file) is not the
  protocol's responsibility — see the Thread and Process Safety
  discussion in the Scaling Considerations section.
- **Self-contained records.** Each `write_record()` call produces
  one prefix + id + payload sequence. The prefix's length fields
  tell a reader exactly how many bytes belong to this record, so the
  next record's prefix is always at a known offset from the current
  one.
- **Truncation tolerance.** A partially-written trailing record —
  from a crashed writer or a torn filesystem append — is detected via
  a short read at any field boundary (anywhere inside the prefix, the
  id bytes, or the payload bytes) and surfaces as a clean
  end-of-stream, not as an exception. Every complete record preceding
  the tear is readable.
- **Magic-byte file identification.** The four bytes "NGSR" at offset 0
  of every prefix make checkpoint files identifiable by external
  tooling without any library dependency. A reader encountering a
  non-NGSR magic at a record boundary throws with a clear "this is not
  a v0.2 record stream" diagnostic — corruption is distinguished from
  truncation.
- **Cross-host portability.** The prefix's integer fields are written
  byte-by-byte in little-endian order, so the bytes on disk are
  identical regardless of host endianness or word size. The signed
  fields assume two's-complement representation; two canary tests in
  `test_bmi_protocols` verify this at run time on each deployment
  host. Payload portability is the model's responsibility — see
  `BMI_SERIALIZATION_PROTOCOL.md`.
- **Uniqueness is a caller convention, not a protocol guarantee.** The
  save protocol does not check for, or prevent, duplicate
  `(id, time_step)` records — callers that fire `run()` twice at the
  same step for the same id will write two records. Well-behaved
  drivers produce exactly one record per `(id, time_step)` pair;
  restore tooling that encounters duplicates returns the first match
  found (for fixed-step and timestamp modes) or the highest step seen
  (for "latest" mode).
- **Timestamp matching.** Restore may key on `simulation_timestamp`
  instead of `time_step`; the match is numeric (not string), done
  against the `int64_t` value parsed from `Context::timestamp`. The
  save and restore sides both parse their respective `timestamp`
  configuration strings through the same rule (`std::stoll`, with
  unparseable strings mapping to 0), so as long as the caller emits
  the same encoding on both sides, the round-trip is exact.

See `include/utilities/bmi/serialization_record.hpp` for the
`SerializationRecord` definition and the `write_record` /
`read_next_record` / `read_record_length` / `read_record_metadata`
helpers. The prefix layout itself lives in the library-internal
header `src/utilities/bmi/wire_format.hpp`.

---

## Save Protocol: `NgenSerializationProtocol`

### Configuration

```json
"serialization": {
    "path": "/path/to/shared.ckpt",
    "save": {
        "check":     true,
        "frequency": 1,
        "fatal":     true
    }
}
```

Keys:

- `path` — *string, shared at the top level.* Output file into which
  all protocol instances append their records. Required when
  `save.check` is true.
- `save.check` — *bool, default true when the `save` block is present.*
  Master enable for the save-side protocol.
- `save.frequency` — *int, default 1.*
    - `> 0` : checkpoint every N time steps (step % N == 0 fires).
    - `< 0` : checkpoint only at the final time step.
    - `0`   : disabled (equivalent to `check: false`).
- `save.fatal` — *bool, default true.* On a non-conforming model at
  load time, or an I/O or BMI failure during `run()`, `true` raises a
  `PROTOCOL_ERROR` (fatal); `false` logs a `PROTOCOL_WARNING` and
  disables the protocol for that module.

Absent `serialization` block, or absent `save` sub-block, disables the
save protocol silently.

### Support detection

`check_support()` is a **metadata-only** probe that validates, for each
of the four reserved names:

1. The model recognizes the name (`GetVarUnits(name)` succeeds).
2. The returned unit string matches the expected value above exactly.

Either failure is reported as `INTEGRATION_ERROR` naming the specific
variable that failed and the mode of failure (unknown vs
wrong-unit). The probe does **not** invoke any `SetValue` or
`GetValue(state)` — it performs no mutation or state read on the model.
This honors the principle that the ngen engine is the sole coordinator
of BMI state mutation.

### Operation

Each `run()` call gated by the configured frequency drives the capture
sequence against the model:

```
SetValue(ngen::serialization_create, &trigger)
GetValue(ngen::serialization_size,   &size)       // expect size > 0
GetValue(ngen::serialization_state,  buffer)      // buffer.size() == size
--- append SerializationRecord{ctx.id, ctx.current_time_step, buffer} to `path` ---
SetValue(ngen::serialization_free,   &trigger)
```

The `id` and `time_step` come from the `Context` provided by the engine,
so multiple entities driving the same protocol instance write records
keyed by their respective feature ids.

On any BMI or I/O failure, control returns early with the appropriate
`PROTOCOL_ERROR` / `PROTOCOL_WARNING`. A best-effort `SetValue(free)` is
attempted to avoid leaking the internal buffer even in error paths.

---

## Restore Protocol: `NgenDeserializationProtocol`

### Configuration

```json
"serialization": {
    "path": "/path/to/shared.ckpt",
    "restore": {
        "check":     true,
        "step":      "latest",
        "timestamp": "1448928000",
        "fatal":     true
    }
}
```

Keys:

- `path` — *string, shared at the top level.* The same file the save
  protocol writes into. Required when `restore.check` is true.
- `restore.check` — *bool, default true when the `restore` block is
  present.* Master enable for the restore-side protocol.
- `restore.step` — *string, default `"latest"`.* Either the literal
  sentinel `"latest"` (the highest step present for the entity's id in
  the file) or a numeric step string (e.g. `"5"`). An integer is also
  accepted.
- `restore.timestamp` — *string, optional.* If set, restore matches by
  numeric compare against `SerializationRecord::timestamp` (instead of
  `time_step`). The string is parsed through the same rule the save
  protocol uses — `std::stoll`, with unparseable strings mapping to 0
  — so callers who want timestamp-keyed round-trip should emit the
  same integer encoding (typically seconds since the Unix epoch) on
  both sides. **If both `timestamp` and `step` are specified,
  `timestamp` wins.**
- `restore.fatal` — *bool, default true.* On a non-conforming model at
  load time, or a missing record, missing file, or BMI failure during
  `run()`, `true` raises a `PROTOCOL_ERROR` (fatal); `false` logs a
  `PROTOCOL_WARNING` and disables the protocol for that module.

Both `save` and `restore` sub-blocks may appear together; they share
the `path`. Either sub-block may be absent to disable that direction.

### Support detection

`check_support()` uses the same **metadata-only** probe as the save
protocol: for each reserved name, confirm the model recognizes it and
reports the exact expected unit string. No `SetValue` or `GetValue`
is performed against the model during support detection.

In particular, `SetValue(ngen::serialization_state, ...)` — the one
operation the restore protocol actually drives at `run()` time — is
*not* probed. Doing so would mutate the model's computed state at init,
which is the engine's responsibility, not a support probe's. The
trade-off is that a model whose state-write implementation is broken
will only surface the failure at the first real `run()` call.

### Operation

Each `run()` scans `path` for a `SerializationRecord` matching the
entity's `ctx.id` and the configured match mode:

- **Timestamp** (if `restore.timestamp` is set): the record whose
  `timestamp` field — an `int64_t` — numerically equals the parsed
  value of the `restore.timestamp` configuration string. Wins over
  `step` if both are configured.
- **`"latest"`** (default when only `step` is set, or neither key is
  specified): the record with the highest `time_step` for that id.
- **Fixed step** (when `step` is a numeric value): the record whose
  `time_step` exactly equals the value.

On a match, the protocol calls:

```
SetValue(ngen::serialization_state, record.payload.data())
```

The model reads the bytes and restores its own computed state.
`ctx.current_time_step` is not used by this protocol; the step that
matters is the one embedded in the record, controlled by
`restore.step`.

Missing-id, missing-step, and missing-file are all reported via
`PROTOCOL_{ERROR,WARNING}` depending on `fatal`, with a message naming
the id (and step if pinned).

---

## Combined save + restore example

A realization configuration for an ensemble that both checkpoints
every step and restores from the latest record at start:

```json
"serialization": {
    "path": "/var/run/ngen/checkpoint.ckpt",
    "save": {
        "check":     true,
        "frequency": 1,
        "fatal":     true
    },
    "restore": {
        "check": true,
        "step":  "latest",
        "fatal": true
    }
}
```

Because both directions share `path`, records appended by save are the
same ones that restore reads back. Per-entity isolation is maintained
by tagging each record with `ctx.id` (an engine-level *compound
identity*, described below); multiple entities sharing one file
round-trip independently.

---

## Identity Convention: `compound_id()`

Every BMI formulation has a `compound_id()` accessor (defined on
`Bmi_Formulation`) that yields a string the engine uses to tag
protocol records. The convention:

| Formulation shape | `compound_id()` format | Example |
|---|---|---|
| Single-BMI formulation | `<feature-id>:<model_type_name>` | `cat-1:bmi_c_cfe` |
| Multi-BMI submodule    | `<feature-id>.<submodule-index>:<submodule-mtn>:<multi-mtn>` | `cat-1.0:bmi_c_cfe:bmi_multi_noahowp_cfe` |

Where:

- **feature-id** — the catchment / feature identifier (e.g. `cat-1`).
- **submodule-index** — the `.0`, `.1`, ... suffix
  `Bmi_Multi_Formulation` appends to each submodule's id.
- **submodule-mtn** — the submodule's own `model_type_name` from its
  `params` block.
- **multi-mtn** — the enclosing multi formulation's `model_type_name`.

Structural notes:

- Two colon-separated parts → single-BMI formulation.
- Three colon-separated parts → multi submodule. The presence of a
  `.` in the left-most part reflects the submodule index;
  single-formulation keys have no `.` component.
- The multi's `model_type_name` (e.g. `bmi_multi_noahowp_cfe`) is
  user-controlled in the realization JSON, so it naturally
  distinguishes different multi-formulation setups without the
  protocol having to derive a signature. Rename the multi and its
  submodule records become unreachable — intentional composition
  safety.

### How the compound gets set

`Bmi_Multi_Formulation::init_nested_module` reads each submodule's
`model_type_name` from its config map and calls
`set_compound_id(<identifier>:<sub-mtn>:<multi-mtn>)` on the
submodule **before** `create_formulation()` runs. That ordering
matters — the restore hook at the end of `create_model()` (see
below) needs the compound already in place.

Single-BMI formulations don't call `set_compound_id`; the default
composition (`id + ":" + model_type_name`) wins inside
`compound_id()`.

`this->id` is not rewritten by any of this. It remains the feature
identifier used by non-protocol code paths (output file naming,
forcing lookup, logs). The compound is a derived view.

---

## Engine Integration

The protocol plugs into two well-defined ngen lifecycle points.

### Save — per-timestep from `Layer`

`Catchment_Formulation` grows a virtual `checkpoint_state(iteration,
total_steps, timestamp)` alongside `check_mass_balance`, overridden
on:

- `Bmi_Module_Formulation` — builds a `Context` with
  `ctx.id = compound_id()` and runs
  `Protocol::SERIALIZATION`.
- `Bmi_Multi_Formulation` — `final` override that iterates its
  submodules and forwards the call. The enclosing multi has no state
  of its own to checkpoint.

The simulation driver — `ngen::Layer::update_models` defined in
`src/core/Layer.cpp` (the declaration lives in
`include/core/Layer.hpp`) — invokes it once per timestep, immediately
after `check_mass_balance`:

```cpp
response = r_c->get_response(output_time_index,
                             simulation_time.get_output_interval_seconds());
r_c->check_mass_balance(output_time_index,
                        simulation_time.get_total_output_times(),
                        current_timestamp);
r_c->checkpoint_state(output_time_index,
                      simulation_time.get_total_output_times(),
                      current_timestamp);
```

When `serialization.save` is unconfigured or disabled, the protocol's
`run()` short-circuits; the steady-state cost is one virtual dispatch
per formulation per timestep.

### Restore — once at the end of `create_model`

Restore is not a per-timestep hook — it fires exactly once, inside
`Bmi_Module_Formulation::inner_create_formulation`, right after the
`NgenBmiProtocols` container is constructed:

```cpp
bmi_protocols = models::bmi::protocols::NgenBmiProtocols(
    get_bmi_model(), properties);

// Engine-driven restore.
models::bmi::protocols::Context restore_ctx{
    /*current_time_step*/ 0,
    /*total_steps*/       0,
    /*timestamp*/         "create_model",
    /*id*/                compound_id()
};
bmi_protocols.run(
    models::bmi::protocols::Protocol::DESERIALIZATION,
    restore_ctx);
```

No additional virtual on the formulation base is needed — the engine
drives restore implicitly as part of the formulation's own
construction. Multi submodules inherit this behavior: each
submodule's `inner_create_formulation` is called from
`init_nested_module`, and by that point `compound_id()` already
returns the injected three-part key.

`ctx.current_time_step`, `ctx.total_steps`, and `ctx.timestamp` are
not used for record matching — the record to restore is selected by
`serialization.restore.step` or `serialization.restore.timestamp`
from config. The placeholder values here exist only for error-message
context should restore fail.

### Lifecycle summary

```
Formulation_Manager config parsing
  │
  ├─ for each formulation (catchment or multi):
  │    │
  │    ├─ Bmi_Module_Formulation::create_formulation
  │    │    ├─ BMI model Initialize()
  │    │    ├─ NgenBmiProtocols constructed (parses serialization cfg)
  │    │    └─ Protocol::DESERIALIZATION run()       ← restore here
  │    │
  │    └─ (Bmi_Multi_Formulation: above runs once per submodule)
  │
  └─ Layer::update_models per timestep:
       │
       └─ for each catchment:
            ├─ get_response()
            ├─ check_mass_balance()
            └─ checkpoint_state()                     ← save here
                 └─ Protocol::SERIALIZATION run()
```

---

## Worked Example — multi-BMI formulation with save + restore

```json
{
    "catchments": {
        "cat-1": {
            "formulations": [
                {
                    "name": "bmi_multi",
                    "params": {
                        "model_type_name": "bmi_multi_noahowp_cfe",
                        "modules": [
                            { "name": "bmi_fortran",
                              "params": { "model_type_name": "bmi_fortran_noahowp",
                                          /* ... */ } },
                            { "name": "bmi_c",
                              "params": { "model_type_name": "bmi_c_cfe",
                                          /* ... */ } }
                        ],
                        "serialization": {
                            "path": "/var/run/ngen/cat-1.ckpt",
                            "save":    { "check": true, "frequency": 1, "fatal": true },
                            "restore": { "check": true, "step": "latest", "fatal": true }
                        }
                    }
                }
            ]
        }
    }
}
```

Running this realization will produce checkpoint records keyed:

- `cat-1.0:bmi_fortran_noahowp:bmi_multi_noahowp_cfe`
- `cat-1.1:bmi_c_cfe:bmi_multi_noahowp_cfe`

Each submodule of `cat-1` saves every timestep and, on restart from
the same file, restores the latest record matching its compound id.

If the same realization contained a second catchment `cat-2` with a
different formulation lineup, its records would use its own
model_type_name(s) — so `cat-1.ckpt` and `cat-2.ckpt` may either be
separate files or share a single path; tagging by compound id keeps
them disambiguated either way.

---

## Realization-Level (Global) Configuration

The `serialization` block in the example above lives inside a single
catchment's formulation params, which is legal but noisy — a
hydrofabric with hundreds or thousands of catchments shouldn't need
that many copies of the same checkpoint settings, because nothing
about serialization is actually per-feature (file path, frequency,
save/restore semantics are all realization-wide decisions).

### Top-level placement

Lift the `serialization` block to the **top of the realization JSON**,
as a sibling to `global`, `time`, and `catchments`:

```json
{
    "global":  { "formulations": [ /* ... */ ], "forcing": { /* ... */ } },
    "time":    { "start_time": "2015-12-01 00:00:00",
                 "end_time":   "2015-12-31 23:00:00",
                 "output_interval": 3600 },

    "serialization": {
        "path": "/var/run/ngen/checkpoint.ckpt",
        "save":    { "check": true, "frequency": 1, "fatal": true },
        "restore": { "check": true, "step": "latest", "fatal": true }
    },

    "catchments": {
        "cat-1": { "formulations": [ /* no serialization block needed */ ] },
        "cat-2": { "formulations": [ /* ... */ ] },
        "cat-3": { "formulations": [ /* ... */ ] }
    }
}
```

`Formulation_Manager` parses this top-level block once at startup.
For every catchment it constructs — whether through a per-catchment
formulation config or the global-fallback path — the block is
injected into that formulation's params right before
`create_formulation()` runs. `Bmi_Multi_Formulation` then propagates
it into each submodule's params during submodule setup (each submodule
owns its own `NgenBmiProtocols` and therefore needs the config
locally).

### Precedence

Per-formulation entries always win. If a catchment explicitly sets
`params.serialization`, the realization-level block is ignored for
that catchment — useful for one-off overrides (e.g. a debugging path,
or disabling checkpoint for a single problematic module).

```json
{
    "serialization": {
        "path": "/var/run/ngen/global.ckpt",
        "save": { "check": true, "frequency": 1, "fatal": true }
    },
    "catchments": {
        "cat-noisy": {
            "formulations": [{
                "name": "bmi_c",
                "params": {
                    "model_type_name": "bmi_c_cfe",
                    "serialization": { "save": { "check": false } }
                }
            }]
        }
    }
}
```

Here `cat-noisy` opts out of checkpointing while every other catchment
inherits the realization default.

### Absent block

If no `serialization` block is present at the top level and no
catchment supplies one in its params, no serialization happens —
identical to the pre-feature behavior of ngen. The engine's
`checkpoint_state` and `create_model` restore calls still fire, but
the protocols inside are silently disabled and return immediately.

### Path templating

The realization-level `serialization.path` is passed through
`utilities::resolve_path_tokens` before being injected into any
formulation's params. The following tokens, written with exact
double-brace syntax and no whitespace, are expanded at config-load
time:

| Token       | Resolves to                                         |
| ----------- | --------------------------------------------------- |
| `{{rank}}`  | MPI rank (0 for non-MPI builds or uninitialized MPI)|
| `{{pid}}`   | POSIX process id                                    |
| `{{host}}`  | First label of the host name (max 63 chars)         |
| `{{date}}`  | Local date at startup, `YYYYMMDD`                   |

Unknown `{{...}}` tokens are left untouched so a typo doesn't silently
vanish.

Example — per-rank, per-day checkpoint file:

```json
{
    "serialization": {
        "path": "/scratch/run_{{date}}/ckpt.rank_{{rank}}.bin",
        "save":    { "check": true, "frequency": 1 },
        "restore": { "check": true, "step": "latest" }
    }
}
```

Templating happens on the **realization layer**, not inside the
protocol. The protocol is deliberately MPI-agnostic: it sees only the
concrete resolved string and neither knows nor cares that `{{rank}}`
was involved.

---

## Scaling Considerations

The protocol is designed to handle a checkpoint file shared by up to
the low millions of features per simulation. This section documents
where the ceiling is, what wins at each scale tier, and what is
explicitly not solved in-protocol.

### On-disk format

Each record is a fixed-size prefix followed by raw id and payload
bytes (see *Wire Format* above for the field-level layout):

```
[ 39-byte prefix ][ id_length bytes ][ payload_length bytes ]
```

The fixed-prefix layout exists for three reasons:

1. **Truncation tolerance.** A crashed writer can leave a torn
   trailing record; the reader detects the short tail at any field
   boundary (prefix, id, or payload) and stops cleanly at the last
   intact record rather than throwing. Checkpoint files remain
   usable after an unclean shutdown.
2. **Fast index skipping.** The restore-side `CheckpointIndex` walks
   the file via `read_record_metadata` — read the prefix and id,
   `seekg` past the payload, never read payload bytes. Per-record
   cost is O(prefix + id_size) regardless of payload size. The v0.1
   format required reading and parsing the full boost archive
   (payload included) to extract metadata; the v0.2 separation makes
   index builds independent of payload sizes. For multi-MB or
   multi-GB ML-state payloads, this is orders of magnitude faster.
3. **Append-only safety.** Records are individually complete, so
   multiple save protocol instances can append to the same file
   without format-level coordination. Writer-level coordination (for
   intra-process threads or inter-process MPI ranks) is not the
   protocol's responsibility — see *Thread and process safety*.

Both timestamps on disk (`simulation_timestamp` and
`checkpoint_epoch`) are stored as `int64_t` signed seconds since the
Unix epoch. The fixed 8-byte width keeps record headers predictable
and makes `CheckpointIndex` entries compact.

The format does not use `boost::serialization` — every field is
written byte-by-byte in little-endian order via the library-internal
`byte_io` primitives. This makes the bytes identical across hosts
(no host-endian or word-size assumptions embedded in the file) and
removes any boost dependency from the I/O path. The two's-complement
assumption for the three signed fields is verified at run time by
two canary tests in `test_bmi_protocols` (see
`src/utilities/bmi/wire_format.hpp` for the full discussion).

### Save cost model

Per call to `NgenSerializationProtocol::run()` at steady state:

- One `SetValue(create)` → one `GetValue(size)` → one `GetValue(state)`
  → one record write → one `SetValue(free)`.
- The payload buffer is reused across calls; no heap allocations after
  the buffer has grown to steady state.
- The output file handle is opened lazily on first fire and held open
  for the life of the protocol instance. Writes are flushed per record
  so the file is tailable in real time and a crash between records
  loses at most the current in-flight record.
- An explicit per-instance stream buffer (64 KB default) reduces the
  OS write syscall rate under dense checkpointing.

At 1 M features × 24 checkpoints/day with ~100 KB per record, that's
~240 GB/day of write throughput and ~24 M records/day on disk —
within the budget of a single shared NFS volume for most workloads.

### Restore cost model

The restore protocol uses a process-global `CheckpointIndex` keyed by
file path. The first restore against a path builds an
`id → [(time_step, timestamp, file_offset, length)]` map in a single
O(records) pass. Subsequent restores are O(entries-for-that-id), which
is O(1) in the common "one record per feature" case.

This matters because the naive "every feature scans the whole file"
approach is O(features × records). At 1 M features with 1 M records in
the file that's `1e12` record reads — unfeasible. The indexed approach
is `1e6` reads for the build plus `1e6` O(1) lookups, total `2e6`
record reads, which is ~six orders of magnitude better.

Payload bytes are not cached in memory: the index stores `(offset,
length)` pairs and the restore issues a `seek + read` per feature.
That keeps the index size proportional to the number of records
rather than total payload bytes.

The cache survives every protocol instance by default — restores that
happen mid-run (e.g. a multi-phase driver that restores and then
keeps running against the same file) reuse the indexes without a
second build. In memory-constrained runs where restores happen only
during initialization and the index is dead weight afterward, call
`models::bmi::protocols::clear_checkpoint_indexes()` (declared in
`include/utilities/bmi/deserialization.hpp`) once the
initialization-phase restore sequence completes. The cache is
released immediately; any later restore relazily rebuilds from disk.

### `id_subset` for memory-bounded restores

When a run only intends to restore a known subset of the features
present in the checkpoint file, set
`serialization.restore.id_subset` to the list of feature ids:

```json
{
    "serialization": {
        "path": "run_042.ckpt",
        "restore": {
            "id_subset": ["cat-1", "cat-2", "cat-17"]
        }
    }
}
```

The `CheckpointIndex` build drops records whose id is not in the
subset, keeping the index size proportional to the subset size rather
than the file size. For a run restoring 10 k features from a 10 M
feature checkpoint, the subset cuts index memory by ~1000×.

**Auto-derivation from the hydrofabric.** When `id_subset` is not
supplied but a realization-level `serialization.restore` block *is*
configured, `Formulation_Manager` populates `id_subset` with the
catchment ids present in the hydrofabric before any formulation is
constructed. This means a partitioned run (e.g. one MPI rank operating
on a subset of features) gets the memory-optimal index for free — no
need to enumerate features by hand. A caller-supplied `id_subset`
always wins over the derived default.

If the key is omitted *and* no catchments are known (e.g. a lone
protocol instance outside the realization manager), the index covers
all ids in the file.

### Thread and process safety

The protocol draws three boundaries, each documented inline in the
source:

- **Per-instance I/O** (save-side `ofstream` and payload buffer): guarded
  by an `io_mutex_` on the protocol instance. A future threaded driver
  calling `run()` for different feature ids on the *same* protocol
  instance is safe. See `include/utilities/bmi/serialization.hpp`,
  region (1).
- **Per-call BMI capture scope** (`SetValue(create) ... SetValue(free)`):
  managed by an RAII guard so an exception partway through still
  releases the model's capture. See region (2).
- **Cross-instance coordination** (multiple protocol instances pointed at
  the same `path`): *not* coordinated by this protocol. If a driver
  fans out writes across threads or processes to one file, that
  driver owns the coordination. See region (3).

For MPI runs, the common pattern is per-rank file naming rather than
a shared file — this sidesteps cross-process coordination entirely.
See the path templating section in *Configuration* for the
`{{rank}}`-style tokens the realization layer supports.

### When the protocol is *not* the right tool

- **More features than can be indexed in memory.** At ~80 bytes per
  index entry (id string + metadata), 100 M records ≈ 8 GB of index.
  Above that, callers want a tiered or on-disk index — not something
  the current protocol attempts.
- **Concurrent writers to one file across hosts.** POSIX append-mode
  semantics on shared filesystems (NFS, Lustre) do not guarantee
  atomicity for large writes. Either use per-rank files or layer a
  durable object store in front.
- **Per-record compression or encryption.** Not addressed by the
  current protocol. The payload bytes written are the bytes the
  model's BMI adapter handed back from `GetValue(state)`; any
  transformation is the adapter's responsibility.
