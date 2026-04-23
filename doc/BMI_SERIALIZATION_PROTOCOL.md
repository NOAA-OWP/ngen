# ngen BMI Serialization Protocol

> **Status**: version 0.1 — both save (`NgenSerializationProtocol`) and restore (`NgenDeserializationProtocol`) are implemented and tested in `test_bmi_protocols`.

The **Serialization Protocol** is an opt-in convention that lets a BMI model
participate in ngen's state checkpoint/restore workflow. A conforming model
exposes four reserved BMI variables; the ngen engine drives them via the
`NgenSerializationProtocol` (save) and `NgenDeserializationProtocol`
(restore) classes registered under `NgenBmiProtocols`.

Conformance is optional. Models that do not implement the protocol run
unchanged; the engine silently skips checkpointing for those modules.

---

## Reserved BMI Variables

All four reserved names share the `ngen::` prefix to signal that they
belong to the ngen protocol vocabulary and are distinct from the model's
own physical inputs and outputs.

| Name | Driven via | BMI Type | Unit | Role |
|---|---|---|---|---|
| `ngen::serialization_create` | `SetValue` | `int` | `ngen::trigger` | Action: capture current state into an internal buffer |
| `ngen::serialization_free`   | `SetValue` | `int` | `ngen::trigger` | Action: release the internal buffer |
| `ngen::serialization_size`   | `GetValue` | `int` | `bytes`         | Current internal buffer size in bytes; 0 before `create` |
| `ngen::serialization_state`  | `GetValue` / `SetValue` | `char` | `ngen::opaque` | Raw state bytes: read to serialize, write to restore |

### Item list behavior

The four reserved names **MUST NOT** be returned by `GetInputVarNames()` or
`GetOutputVarNames()`. They are discovered by protocol probing (by name),
not by enumeration. This keeps the model's public item list free of
protocol plumbing and matches the pattern used by the Mass Balance
Protocol's `ngen::mass_*` variables.

They MUST however be resolvable by the standard variable-introspection
calls: `GetVarType`, `GetVarUnits`, `GetVarItemsize`, and `GetVarNbytes`.

---

## Unit Conventions for Non-Physical Variables

Three of the four reserved variables are not physical quantities: two are
action signals and one is an opaque byte blob. Standard UDUnits strings
are not meaningful for these.

The protocol reserves two `ngen::`-namespaced unit strings for this
purpose:

- **`ngen::trigger`** — the `SetValue` interaction is an *action*; the
  value passed is a signal only and MUST be ignored by the model. Used
  for `ngen::serialization_create` and `ngen::serialization_free`.

- **`ngen::opaque`** — the value is a byte bag with no physical
  dimension. Implementations MUST NOT attempt unit conversion on these
  bytes. Used for `ngen::serialization_state`.

These non-UDUnits strings exist so protocol machinery and model
implementers can disambiguate "semantic signal" from "dimensioned
quantity". `NgenSerializationProtocol::check_support()` validates that
a model exposes the exact expected unit strings above; a mismatch is
reported as an integration error and disables the protocol for that
module.

---

## Wire Format: `SerializationRecord`

The save protocol writes state snapshots as a sequence of self-contained
Boost binary archives, each holding one `SerializationRecord`:

```cpp
struct SerializationRecord {
    static constexpr uint8_t CURRENT_VERSION = 1;

    uint8_t           version;    // schema version; CURRENT_VERSION at write time
    std::string       id;         // Context::id (e.g. catchment id)
    int               time_step;  // simulation time step at capture
    std::string       timestamp;  // Context::timestamp at capture (exact string)
    std::vector<char> payload;    // bytes from GetValue(ngen::serialization_state)
};
```

The leading `version` byte makes each record self-describing independent
of the archive format: future schema changes bump
`SerializationRecord::CURRENT_VERSION` and the paired
`BOOST_CLASS_VERSION`, and reading code can branch on `rec.version` or
the archive version argument in `serialize()`.

Key properties:

- **Single shared file, multiple writers.** All protocol instances
  pointed at the same `path` append to the same file.
- **Self-contained records.** Each `write_record()` call produces one
  complete binary archive (with its own boost header); readers consume
  one archive at a time via a fresh `binary_iarchive` per record.
- **Uniqueness.** Each `(id, time_step)` pair is unique within a file.
  One entity may have records at multiple steps, but at most one
  record per step.
- **Timestamp matching.** Restore may key on `timestamp` (exact-string
  compare) instead of `time_step`; the timestamp written by the save
  protocol is whatever the engine passed as `Context::timestamp` at
  capture time. Save/restore timestamp formats must therefore agree —
  they're compared as raw strings, not parsed.

See `include/utilities/bmi/serialization_record.hpp` for the
`SerializationRecord` definition and the `write_record` /
`read_next_record` helpers.

---

## Model Developer Implementation Guide

A BMI module wishing to support the protocol must:

1. **Recognize the four reserved names** in `GetVarType`, `GetVarUnits`,
   `GetVarItemsize`, and `GetVarNbytes`, returning the declared type and
   unit strings above verbatim.
2. **`SetValue(ngen::serialization_create, _)`**: snapshot the model's
   current state into an internal buffer. The value passed is a trigger
   signal only and MUST be ignored.
3. **`GetValue(ngen::serialization_size, &int)`**: report the current
   buffer size in bytes. Before any `create` has been performed, report
   `0`. The protocol uses this to allocate the read destination before
   `GetValue(state, ...)`.
4. **`GetValue(ngen::serialization_state, dst)`**: copy the captured bytes
   from the internal buffer into `dst`. `dst` is sized to `size` bytes
   (as reported by `GetValue(size, ...)`).
5. **`SetValue(ngen::serialization_state, src)`**: restore the model's
   computed state from the `src` bytes. The number of bytes is implied
   by the format encoded in the bytes themselves — the protocol does
   not pass a size.
6. **`SetValue(ngen::serialization_free, _)`**: release the internal
   buffer. Value is a trigger signal only.

### Constraints and non-goals

- `SetValue(create)` and `SetValue(free)` touch only the internal
  snapshot buffer; they **must not** alter the model's computed state.
  Conceptually this is double-buffering — the model's user-facing state
  is unaffected by a capture.
- `GetValue` on any reserved name is non-mutating; the protocol relies
  on this to probe support without side effects.
- Reserved names MUST NOT appear in `GetInputVarNames` /
  `GetOutputVarNames`.
- Unit strings for the three non-byte-count reserved variables MUST use
  `ngen::trigger` or `ngen::opaque` as described above. Do not substitute
  `"-"` or other dimensionless markers — the protocol validates the
  exact string.

### Reference implementations

`extern/test_bmi_c/`, `extern/test_bmi_cpp/`, and `extern/test_bmi_fortran/`
each include a minimal but complete reference implementation suitable
as a template for adaptation into real models.

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
- `save.fatal` — *bool, default true.* On I/O or BMI failure during
  `run()`, `true` raises a `PROTOCOL_ERROR` (fatal); `false` logs a
  `PROTOCOL_WARNING` (non-fatal).

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
        "timestamp": "2015-12-01 00:00:00",
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
  exact-string compare against `SerializationRecord::timestamp`
  (instead of `time_step`). **If both `timestamp` and `step` are
  specified, `timestamp` wins.** The format must match exactly what
  the save protocol wrote — the save protocol persists
  `Context::timestamp` verbatim, so whatever the engine supplies on
  save must be what the user supplies on restore.
- `restore.fatal` — *bool, default true.* On missing record, missing
  file, or BMI failure during `run()`, `true` raises a
  `PROTOCOL_ERROR` (fatal); `false` logs a `PROTOCOL_WARNING`.

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
  `timestamp` field is an exact string match. Wins over `step` if both
  are configured.
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
by tagging each record with `ctx.id`; multiple entities sharing one
file round-trip independently.
