# ngen BMI Serialization Protocol

> **Status**: version 0.1.

The **Serialization Protocol** is an opt-in convention that lets a BMI
model participate in a caller's state checkpoint/restore workflow.
A conforming model exposes four reserved BMI variables; the host engine
drives them through a save sequence (capture state to bytes) and a
restore sequence (write bytes back into state).

This document specifies what the **model author** has to provide. For
how ngen specifically drives the protocol (wire format, configuration, engine
integration, scaling, etc.), see [`NGEN_SERIALIZATION.md`](NGEN_SERIALIZATION.md).

## TODO or Not TODO
### Conformance is use-case dependent

Conformance is required when **the user of your model needs checkpoint
or restore**: a long-running simulation that wants to recover from a
crash, a multi-phase workflow that needs to resume from a stored state,
or any other use case that depends on capturing model state to bytes
and writing those bytes back later. If any of those matter for your
model's intended use, you should implement this protocol.

**For operational models, serialization support is a requirement.**

How a host engine reacts when a model fails to conform is
engine-specific. For ngen's behavior, see the "Engine behavior with
non-conforming models" section of
[`NGEN_SERIALIZATION.md`](NGEN_SERIALIZATION.md).

---

## Reserved BMI Variables

All four reserved names share the `ngen::` prefix to signal that they
belong to the ngen protocol vocabulary and are distinct from the model's
own physical inputs and outputs.

| Name | Driven via | BMI Type | Unit | Role |
|---|---|---|---|---|
| `ngen::serialization_create` | `SetValue` | `int` | `ngen::trigger` | Action: capture current state into an internal buffer |
| `ngen::serialization_free`   | `SetValue` | `int` | `ngen::trigger` | Action: release the internal buffer |
| `ngen::serialization_size`   | `GetValue` | `int` | `bytes`         | Current internal buffer size in bytes. Populated by `create`; may be `0` before any `create` (in particular at restore time). |
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
quantity". The host engine's support probe validates that a model
exposes the exact expected unit strings above; a mismatch is reported
as an integration error and disables the protocol for that module.

---

## Model Developer Implementation Guide

A BMI module wishing to support the protocol must:

1. **Expose the exact reserved units via `GetVarUnits`.** A conforming
   host engine probes support by calling `GetVarUnits` on each
   reserved name and verifying an exact match against `ngen::trigger`
   / `ngen::opaque` / `bytes` as listed in the Reserved BMI Variables
   table. This is the only introspection call the support probe
   makes — a mismatch here is the one and only signal the protocol
   uses to decide a model doesn't conform.

   `GetVarType`, `GetVarItemsize`, and `GetVarNbytes` must also
   resolve correctly for the reserved names (see the table for the
   declared types). The support probe does not exercise them, but
   BMI adapters on either side of the language boundary use these
   calls to marshal values into and out of `SetValue`/`GetValue` —
   so they're part of the protocol contract, not optional metadata.
   In particular, `GetVarNbytes(ngen::serialization_state)` is what a
   host engine queries to size the restore-side buffer (see step 5).
2. **`SetValue(ngen::serialization_create, _)`**: snapshot the model's
   current state into an internal buffer. The value passed is a trigger
   signal only and MUST be ignored.
3. **`GetValue(ngen::serialization_size, &int)`**: report the current
   internal buffer size in bytes — the number of bytes a subsequent
   `GetValue(state, ...)` will deliver. The caller invokes this after
   `create` to size the read destination; the model is permitted to
   return `0` before any `create` has fired (the dynamic-memory case
   where the snapshot size isn't known until `create` runs the
   serialize routine).

   `GetVarNbytes(ngen::serialization_state)` plays a separate role:
   BMI adapters that marshal `SetValue(state, ...)` through a
   pre-sized container — for example, a Python adapter wrapping raw
   bytes in a numpy array sized by `GetVarNbytes` — query it at
   restore time to decide how many bytes to marshal across the
   language boundary. Because restore happens before any `create`,
   `GetVarNbytes(ngen::serialization_state)` MUST return a valid
   length even when no buffer exists yet. Models in that situation
   should compute the expected snapshot size on demand — e.g. by
   running the same serialize routine `create` would run and
   measuring its output. Adapters that pass the raw `SetValue(state,
   ...)` pointer straight through to the model (C, C++, Fortran)
   don't need this — the model reads whatever the caller provides.
4. **`GetValue(ngen::serialization_state, dst)`**: copy the captured bytes
   from the internal buffer into `dst`. `dst` is sized to `size` bytes
   (as reported by `GetValue(size, ...)`).
5. **`SetValue(ngen::serialization_state, src)`**: restore the model's
   computed state from the `src` bytes. The protocol hands back the
   exact byte sequence the model produced at save time
   (`GetValue(state, ...)` after `create`); the model is responsible
   for knowing how many bytes that is, since the model controls the
   format. The BMI surface here does not pass a length argument — at
   restore, `ngen::serialization_size` may still read 0 because no
   `create` has fired (see step 3).

   In languages whose BMI adapter wraps `src` in a pre-sized container
   (e.g. a Python adapter wrapping the bytes in a numpy array sized by
   `GetVarNbytes(ngen::serialization_state)`), the model keeps the
   correct length flowing through by making `GetVarNbytes` return the
   snapshot size at all times. In languages whose adapter passes the
   raw pointer through (C, C++, Fortran), the model simply reads its
   own format directly from `src`.
6. **`SetValue(ngen::serialization_free, _)`**: release the internal
   buffer. Value is a trigger signal only.

### Recommended: defensive payload format

The protocol shuttles raw bytes without any framing, validation, or
schema check of its own. Model implementers are strongly encouraged
to encode their own header in the payload — a version number, magic
bytes, or both — so that `SetValue(state, src)` can detect format
drift before applying the bytes to the model's computed state. A
buffer from an older model version, from a different model entirely,
or from a truncated record will then be rejected at restore time
rather than silently corrupting state.

This matters whenever:

- Multiple model versions share a checkpoint file or write into the
  same format over time.
- Schema evolution across releases is expected.
- Robust fail-fast behavior on a malformed buffer is preferable to
  best-effort interpretation.

This is a model-level concern. The host engine's own record header
(see [`NGEN_SERIALIZATION.md`](NGEN_SERIALIZATION.md)) operates at a
different layer — it versions the framing the engine wraps around
your payload, not the payload itself.

### Constraints and non-goals

- `SetValue(create)` and `SetValue(free)` touch only the internal
  snapshot buffer; they **must not** alter the model's computed state.
  Conceptually this is double-buffering — the model's user-facing
  state is unaffected by a capture.

  The capture sequence (`create` → `GetValue(size)` → `GetValue(state)`
  → `free`) is driven by the caller, and the caller is responsible
  for pairing every `create` with a matching `free` — including on
  error paths between the two. The model is responsible for keeping
  `free` safe to invoke whenever the caller issues it (including
  after a failed intermediate `GetValue`), so the buffer is reliably
  released and not leaked into subsequent capture cycles.
- `GetValue` on any reserved name is non-mutating. This is a
  correctness property of the model, not a probing mechanism: the
  support probe uses `GetVarUnits` only and does not invoke
  `SetValue` or `GetValue(state)` against the model. BMI state
  mutation is reserved for the host engine.
- Reserved names MUST NOT appear in `GetInputVarNames` /
  `GetOutputVarNames`. They are discovered by name, not enumeration.
- Unit strings for the three non-byte-count reserved variables MUST use
  `ngen::trigger` or `ngen::opaque` as described above. Do not substitute
  `"-"` or other dimensionless markers — the support probe performs an
  exact-string comparison.
- **Spatial introspection calls are NOT expected to resolve these
  names.** The reserved variables have no spatial semantics — two are
  action triggers, one is a byte count, and one is an opaque byte
  buffer — so a model SHOULD leave `GetVarLocation` and `GetVarGrid`
  to return their standard "unknown variable" signal (BMI_FAILURE /
  throw) for the reserved names rather than invent placeholder values
  like `"none"`. The reference implementations in `extern/test_bmi_*`
  follow this policy. The protocol itself never invokes either of
  these calls against the reserved names, and any caller that does is
  reaching for metadata the protocol doesn't promise exists.
- The minimum BMI surface area a model must expose for protocol
  participation is:
    - `GetVarUnits`, `GetVarType`, `GetVarItemsize`, and `GetVarNbytes`
      for all four reserved names;
    - `SetValue` for `ngen::serialization_create`,
      `ngen::serialization_free`, and `ngen::serialization_state`;
    - `GetValue` for `ngen::serialization_size` and
      `ngen::serialization_state`.

  The support probe validates `GetVarUnits` against the exact
  expected strings; BMI adapters use the other introspection calls to
  marshal values across the language boundary, so all four must
  resolve correctly.

### Reference implementations

`extern/test_bmi_c/`, `extern/test_bmi_cpp/`, `extern/test_bmi_fortran/`,
and `extern/test_bmi_py/` each include a minimal but complete reference
implementation suitable as a template for adaptation into real models.
