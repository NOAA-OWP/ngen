# C and C++ style for ngen

## Scope and priorities

Apply this guide to new and materially changed first-party C and C++ code,
including headers, private helpers, templates, and tests. Preserve existing public
names and ABI contracts unless the task authorizes a change. Bring changed code
into conformance within scope. **Absolutely no formatting changes shall be made
to unrelated code, including untouched code within an otherwise modified file.**
Do not reorder unrelated includes, rewrap comments, normalize whitespace, or
perform incidental cleanup. External dependencies retain their own conventions.

**Direct changes to `extern/` and all submodules are deprecated.** Prefer
first-party adapters, wrappers, and integration changes outside these locations.
Do not introduce direct dependency edits, vendor patches, or submodule revision
changes during routine development. Pursue upstream fixes or a maintained
dependency-update path when necessary. Changes within deprecated locations require
explicit user scope and a documented, narrow reason; do not undertake a migration
or removal of existing code merely because this policy exists.

Prefer explicit, unsurprising, short code with precise contracts. Make ownership,
units, numerical assumptions, failure behavior, synchronization, and extension
points visible. A reader should not need to infer them from call-site accidents.

## Formatting and static analysis

`.clang-format` defines mechanical formatting. Its current settings specify:

- LLVM base, four spaces, no tabs, and a 100-column limit.
- Attached opening braces; no namespace indentation; separated definition blocks.
- No single-line function definitions and no nonempty single-line blocks.
- Arguments and parameters are not bin-packed; template declarations break.
- Consecutive assignments, macros, and bit fields align; declarations do not.
- Pointer alignment is derived from the file; do not impose a conflicting rule.
- Trailing comments are not aligned. Keep comments focused on the symbol or rule.

The formatter permits short loops and lambdas on one line. For new control flow,
prefer braced, multiline bodies, including single-statement branches and loops.
Use early returns to reduce nesting, with explicit error checks. Let a compatible
`clang-format` determine exact wrapping, alignment, include layout, and spacing;
do not change its settings merely to accommodate generated code. Some settings
require newer Clang versions; report unsupported keys instead of silently dropping
them or reformatting with a different style.

`.clang-tidy` enables diagnostics, analyzer, C++ Core Guidelines, modernization,
MPI, bug-prone, Boost, performance, HIC++, Google, readability, selected miscellaneous,
and const-correctness checks. It disables `modernize-use-trailing-return-type` and
`cppcoreguidelines-special-member-functions`; those exclusions neither require
trailing returns nor waive correct ownership semantics. `FormatStyle: file` uses
the repository formatter. The configuration does not reset all tool-default checks;
the effective set also depends on the tool version. Inspect actual diagnostics.

For example, after configuring a suitable compilation database:

```sh
clang-format --dry-run --Werror path/to/changed_file.cpp
clang-tidy -p path/to/build path/to/changed_file.cpp
```

These paths are placeholders, not executable repository targets. Run on actual
changed files. Use the configured compiler arguments and required generated headers.
The example formatter command is read-only and may report pre-existing issues;
those findings do not authorize unrelated edits. For formatting edits, use changed
line ranges or a diff-based workflow and inspect the resulting diff. Do not run
whole-file `clang-format -i` on an existing file when it would change unrelated code.
Do not run mass fixes or weaken lint configuration. A narrowly scoped `NOLINT`
requires the check name and a concrete reason; unresolved diagnostics must be
reported. This guide supplies semantic rules the Clang tools cannot enforce.

## Names, types, and explicit behavior

- Use full, descriptive words for variables, parameters, functions, classes, type
  parameters, and test names. Avoid single-character identifiers, including loop
  indices, and unexplained abbreviations. Prefer `catchment_index`,
  `simulation_time_step_seconds`, and `destination_process_rank` to `i`, `dt`, or
  `dst`. Names should convey purpose and relevant scope or units.
- Use `UpperCamelCase` for new C++ classes/structs and `lower_snake_case` for new
  functions, variables, and parameters. Preserve required names when overriding
  existing interfaces. Give C functions/types a descriptive component prefix where
  needed to avoid global symbol collisions. Preserve standardized boundary names
  such as MPI/BMI entry points; spell out terminology in their documentation.
- Provenance belongs in documentation, not model names embedded in identifiers.
  Do not rename public symbols merely to record an author or tool version.
- Prefer explicit types where they clarify units, signedness, width, ownership,
  or conversions. Use `auto` only when the type is evident and its omission hides
  no material contract. Avoid implicit narrowing, unchecked casts, uninitialized
  state, magic sentinels, and booleans whose meaning depends on argument position.
- Use named constants, scoped enums, or small value types for meaningful domains.
  Separate byte counts, element counts, simulation timestamps, durations, and
  process ranks. Validate ranges and sizes before arithmetic, allocation, indexing,
  or conversion; consider overflow, nonfinite floating-point values, and empty data.
- Make state immutable where practical. Apply `const` to read-only objects and
  accessors; avoid hidden global mutable state. Const access alone is not a
  concurrency guarantee.
- Use normal `if`/`else` or `switch` statements for branching. Avoid nested ternaries,
  macro-generated control flow, boolean arithmetic that encodes decisions, and
  hidden environment-variable behavior. Do not introduce runtime feature flags to
  conceal competing implementations or select scattered algorithm fragments.
- Keep functions focused on one responsibility, ideally a few dozen executable
  lines. This is a design target, not a reason to split a readable operation into
  trivial forwarding helpers. Limit nesting and argument lists; introduce a
  cohesive typed request only when parameters describe a real shared concept.

## Required LLM provenance

Every LLM-generated function and class must have a per-symbol documentation record,
including private helpers, overloads, constructors, destructors, templates,
generated test cases/fixtures, and behavior-bearing lambdas. Apply this to structs
as well. Prefer a named documented function over a substantial anonymous lambda.
An adjacent comment before a test macro or lambda is acceptable. A file-level
credit alone does not satisfy this requirement.

Use the following fields in the symbol's Doxygen block:

```text
@par LLM provenance
Authorship: Written by <provider and model>, version <reported model version>.
Reasoning configuration: <reported setting, or "not exposed by the environment">.
Design rationale: <concise explanation of the chosen approach and relevant tradeoff>.
Review status: <not yet reviewed, or a verifiable review reference>.
```

For material edits to existing human-written code, use
`Authorship: Modified by ...; scope: ...` and preserve prior authorship and licenses.
Preserve earlier provenance records when multiple models materially contribute.
Each overload and special member needs its own record; class provenance does not
automatically cover its methods. Trivial defaulted members still need a brief
contract and provenance when generated.

Identify the actual model and version supplied by the environment. Record the
exact model identifier/build when available; otherwise state the available model
family/version and explicitly identify missing build details. An application name
such as "Codex" alone is insufficient. Never invent a model version, reasoning
level, human reviewer, or claim of verification. Reasoning means an auditable
design rationale, assumptions, and tradeoffs, not a private chain-of-thought trace.
Use `not exposed by the environment` when a reasoning setting is unavailable.

Do not add provenance retroactively to unchanged human code or imply LLM authorship
of external libraries. Mechanical formatting alone does not establish authorship.
Review status records review, not an instruction to leave code unfinished.

## Required contracts and documentation

Use Doxygen `/** ... */` blocks for every function and class, including internal
and test code. Put the authoritative contract at the declaration when declaration
and definition are separate; the definition may link to it instead of duplicating
it. Document implementation-specific rationale beside the implementation. Keep
documentation consistent with actual behavior and tests.

For every function, describe:

- `@brief`: the operation's purpose and observable behavior.
- `@tparam`: every template parameter and constraints, when applicable.
- `@param[in]`, `@param[out]`, or `@param[in,out]`: every named parameter, including
  callback/context arguments. Specify meaning, units, allowed ranges, default
  behavior, nullability, buffer lengths, encoding, aliasing restrictions, ownership,
  lifetime, and whether an argument is retained, where relevant.
- `@return`/`@retval`: all outcomes, including empty/optional results, sentinel or
  status values, result units, ownership, and lifetime. For `void`, explicitly
  state that no value is returned and describe output/state changes. Constructors
  and destructors have no return value; say so without inventing a result.
- `@throws`: anticipated exception types and the conditions that produce them,
  including propagated library or callback errors and allocation failures where
  applicable. For nonthrowing functions, state that no exceptions escape and
  explain error reporting. For C, document status/error outputs and `errno` only
  if the implementation actually uses it; C functions do not throw C++ exceptions.
- `@pre`: caller obligations, valid state, resource ownership, synchronization,
  MPI initialization/thread level, or collective participation when applicable.
  Distinguish checked invalid inputs from unchecked programmer obligations.
- `@post`: observable success guarantees and state after failure, including which
  outputs remain unchanged or become partially updated. Specify the exception
  guarantee when relevant. State no additional pre/postconditions if there are none.
- Side effects and concurrency: I/O, blocking, callbacks, retained references,
  external state changes, reentrancy, and same-instance versus separate-instance
  concurrent access. Document lock ownership/order and buffer validity through
  asynchronous completion when applicable.
- The LLM provenance record above for generated or materially modified code.

For every class/struct, describe its responsibility, invariants, ownership,
construction/destruction lifecycle, copy/move policy, concurrency contract, and
extension contract, plus provenance when applicable. Document important data
members' meaning, units, and guarding lock. C structs and opaque handles require
equivalent lifecycle and ownership documentation.

Do not manufacture a claim such as "throws nothing" merely because there is no
explicit `throw`. Allocations, filesystem operations, containers, and callbacks
can fail. Use `noexcept` only when the complete operation can uphold it; explain
what happens if a callback violates a nonthrowing contract. Preconditions and
assertions do not replace runtime validation of external inputs.

## Ownership, RAII, and failure handling

- In C++, give each resource a scoped owner: standard containers, `std::unique_ptr`
  with an appropriate deleter, scoped locks, file/handle wrappers, and joining
  thread owners. Prefer unique ownership; justify shared ownership with an actual
  lifetime requirement. Raw pointers and views are non-owning unless explicitly
  required and documented at an external boundary.
- Follow the rule of zero where practical. When custom resource management needs
  special members, define/delete copy and move operations deliberately. Polymorphic
  interfaces deleted through a base pointer need a virtual destructor.
- Destructors must not let exceptions escape. For fallible flush/commit/shutdown,
  offer an explicit operation that reports errors and retain safe cleanup on all
  exits. Never pretend a destructor proves a durable commit succeeded.
- C has no RAII: initialize handles to safe states, check acquisition results,
  pair every acquire/release, and use one clear cleanup path when needed. A small
  `goto cleanup` is acceptable for resource unwinding; avoid duplicated, incomplete
  release logic. Match allocators and deallocators across language/library boundaries.
- At C ABI boundaries, catch C++ exceptions and translate them to documented
  status/error channels. Do not throw through C or Fortran frames or expose C++
  implementation types in C interfaces.
- Use existing component error conventions, including structured `expected` results
  where established. Do not silently swallow exceptions, ignore return codes, parse
  diagnostics to determine error categories, or substitute a success-shaped default.
  Preserve enough context for the caller to identify the failed operation.

## Thread safety and asynchronous lifecycle

Design new code to be safe for its intended concurrency. Prefer immutable data,
local state, and explicit ownership before adding locks. Never claim thread safety
without identifying shared resources and the synchronization that protects them.

- Protect every access to shared mutable state, including reads; document the
  guarding mutex or justified atomic protocol. Do not use `volatile` as a lock.
  Prefer simple locks to lock-free algorithms or custom memory-order schemes.
- Use scoped locking, consistent lock order, and the smallest correct critical
  section. Avoid calling unknown callbacks or blocking external operations while
  holding internal locks; document unavoidable locking around unsafe libraries.
- Check third-party library guarantees. Use the existing process-wide library
  mutex facility where applicable, including NetCDF/HDF5 calls. Per-object locks
  cannot protect process-global library state, and cross-process access requires
  a separate protocol. Avoid nesting the same nonrecursive library mutex.
- Define completion, cancellation, timeout, error propagation, queue bounds,
  backpressure, and shutdown where asynchronous work exists. Drain or cancel
  operations and join workers before destroying state or buffers they use.
  Do not detach threads or let tasks capture dangling references.
- Respect the MPI thread level actually provided, communicator ownership,
  collective ordering, and finalize lifecycle. Keep send/receive buffers alive
  until completion. A mutex around local calls does not repair mismatched
  collectives, unsupported MPI thread access, or distributed deadlocks.
- When a dependency or interface requires thread confinement, state it explicitly
  and enforce it at the boundary; do not advertise unrestricted concurrency.
  Prefer reentrant time/error helpers over APIs with shared static result buffers.

## Interchangeable components and conditional logic

Separate domain behavior from execution and transport mechanisms. Prefer composition
and a small cohesive interface to inheritance used only for code reuse. Use the
Strategy/Adapter pattern when interchangeable behavior has a concrete need; use a
factory at the composition boundary when construction depends on validated input.
Do not create registries, plugin loaders, or universal task frameworks speculatively.

For example, hydrologic flow calculations should not choose MPI ranks, create broker
queues, or spawn workers. An execution/communication boundary can have an MPI
adapter today and a RabbitMQ, thread, or worker-process adapter later. Keep native
transport handles and dependency headers out of domain interfaces where practical.
Use transport-neutral request/result values with explicit ownership and errors.

Define the semantics every implementation must preserve: ordering, completion,
delivery guarantees, cancellation, failures, serialization boundaries, and shutdown
as applicable. Local threads and distributed messaging are not automatically
equivalent. Document limitations rather than hiding them behind a common method
name. Verify shared behavior with small contract tests and isolate adapter-specific
integration tests. Add an interface only where a real substitution boundary exists;
leave future implementations unimplemented until requested.

Choose a concrete implementation once during composition, then call its interface.
Use ordinary conditional statements for actual runtime decisions. Preserve existing
`NGEN_WITH_*` build options for optional dependencies and unavailable APIs; confine
necessary preprocessor guards to integration boundaries. Do not scatter MPI or
backend feature checks throughout domain algorithms or introduce combinations of
boolean flags to represent mutually exclusive execution strategies.

## Portability and numerical correctness

- Use the repository's C++20 baseline and each C target's configured language
  standard. Do not introduce C++23-only facilities, compiler extensions, variable
  length arrays, or compiler-specific builtins without a demonstrated need and a
  documented portable fallback or bounded supported-compiler requirement.
- Target supported Unix-like systems with GCC and Clang; Windows is unsupported.
  Prefer standard library facilities, then isolated POSIX adapters when needed.
  Avoid speculative Windows branches and assumptions about Linux-only behavior.
- Write self-contained headers with include guards or established `#pragma once`;
  include what is used, avoid namespace imports in headers, and keep implementation
  details private. Do not add overloads to `namespace std`.
- Use fixed-width integers for specified wire/ABI widths and size types for memory
  lengths. Check conversions and overflow. Do not assume `long`, pointers, enum
  layout, struct padding, byte order, or object representation is a wire format.
  Serialize fields explicitly and validate lengths and versions on input.
- Preserve units, time origins, valid ranges, and hydrologic conservation rules.
  Document precision/tolerances and deterministic ordering requirements. Do not
  enable unsafe floating-point transformations or change reduction order silently.
- Keep CMake changes target-scoped, respect existing dependency discovery, avoid
  machine-specific paths, and test affected configurations on supported compilers.

## Highly secure and hardened environments

The application must be able to run in highly secure, hardened environments.
Treat restricted capabilities as deployment constraints, not reasons to weaken
host security. Document required capabilities and configuration explicitly.

- Run with least privilege. Do not require root, privilege escalation, broad
  filesystem permissions, writable installation/source/home directories, or
  disabling sandbox, access-control, or security policy enforcement.
- Keep runtime dependencies pre-provisioned. Do not download packages, update
  submodules, fetch executable code, emit telemetry, or contact external services
  implicitly at startup or during simulation. Support offline operation when the
  selected execution mode does not require a network. MPI or explicitly configured
  messaging backends may use authorized endpoints; report denied connections
  without falling back to unapproved destinations or insecure protocols.
- Do not assume `/tmp` is writable or executable. Use explicit approved paths when
  temporary storage is necessary, restrictive permissions, exclusive creation,
  and bounded cleanup. Never execute generated files from temporary storage or
  construct shell commands from untrusted inputs. Prefer direct library APIs.
- Validate input/configuration sizes, paths, and serialized data before using them.
  Do not traverse outside configured storage roots or follow untrusted symlinks
  when operating on security-sensitive files. Avoid check-then-open races; use
  appropriate handle-based or atomic filesystem operations at the platform boundary.
- Never embed credentials or log secrets or sensitive payloads. Receive credentials
  through the deployment's configured mechanism and expose only necessary
  diagnostics. Fail clearly and safely on denied permissions, unavailable resources,
  or invalid configuration; never silently relax permissions or security settings.
- Preserve supported compiler/linker hardening settings and compatibility with
  restricted loaders and process policies. Avoid runtime code generation or
  unconfigured executable/plugin discovery. Document essential dynamic-library
  loading for existing BMI/model integrations and require explicit trusted paths.
- For changes to these boundaries, test relevant read-only paths, permission
  failures, unavailable temporary storage, and blocked/unavailable networking.
  Record which hardened deployment constraints were actually verified.

## Tightly controlled disk I/O on distributed filesystems

Disk I/O must be explicit, limited, and bounded. Distributed filesystems can impose
high costs for metadata operations, concurrent access, and many small files even
when the total byte count is low. Keep domain calculations independent of storage
so they do not introduce hidden reads, writes, or logging in hot paths.

- Define which component/rank owns each output and which configured directories
  may be read or written. Document expected operation frequency, file counts,
  volume, buffer/queue limits, flush policy, durability, and failure behavior.
  Do not introduce new caches, checkpoints, debug dumps, or scratch files implicitly.
- Read inputs once where practical and reuse immutable validated data. Use bounded
  caches or chunked streaming for large datasets; do not replace disk traffic with
  unbounded memory growth or unnecessary duplication across workers.
- Batch and buffer reads/writes within explicit memory and latency limits. Prefer
  suitably sized sequential transfers and bounded handle reuse to repeated small
  operations. Avoid per-element/per-time-step open/close cycles and redundant reads.
- Coordinate distributed access. Prefer designated writers, bounded aggregation,
  or existing collective/parallel I/O facilities when appropriate. Avoid every
  rank independently reading identical metadata or writing to the same file without
  a defined protocol. Do not make one writer a mandatory bottleneck; choose the
  ownership strategy using the actual data volume, memory limits, and backend.
- Avoid file-per-catchment/per-time-step designs, repeated `stat` calls, directory
  scans, filesystem polling, and small-file fan-out unless required by an existing
  contract. If compatibility requires such behavior, contain it behind the storage
  interface and document its cost instead of silently changing output semantics.
- Keep logs bounded, configurable, and aggregated or rate-limited where appropriate.
  Avoid per-iteration/per-rank disk logging and eager debug payload construction.
  Use the configured logging destination rather than creating additional log files.
- Flush, synchronize, and checkpoint at documented boundaries. Avoid unconditional
  per-record flushing or `fsync`, but never omit synchronization required by the
  selected durability contract. Use a backend-appropriate commit/publication
  protocol; do not assume local-filesystem atomicity or visibility on every
  distributed filesystem. Report partial writes, quota exhaustion, and failures.
- Use approved local scratch only when explicitly configured and available. Keep
  it optional where possible; define size limits, cleanup, and required transfer
  back to persistent storage. Do not assume a writable local disk on compute nodes.
- Verify changed I/O behavior with deterministic counting fakes or instrumentation
  at the storage boundary: operation counts, byte volume, file counts, and bounds.
  Check required error paths. Use representative distributed-filesystem measurements
  when a performance claim depends on them; local timings are not equivalent.

## Clear, simple tests

- Use Google Test for C++ tests and the existing harness for C integration tests.
  Add tests to `test/CMakeLists.txt` using its current conventions and required
  feature/dependency declarations. Mirror the component's directory where practical.
- Give suites and cases descriptive names such as
  `FlowExchangeTest.RejectsNegativeTimeStep`. Test one observable behavior per case
  with a clear arrange/act/assert structure. Document case purpose, inputs,
  expected outcome, failure expectations, applicable conditions, and LLM provenance
  in an adjacent comment. Test fixtures and helpers follow the same class/function
  contracts as production code.
- Prefer explicit representative inputs and expected outputs computed independently
  of the implementation. Keep helpers, parameterization, mocks, and fixtures small;
  do not duplicate the algorithm or build a miniature framework to test it.
- Cover meaningful boundaries, invalid inputs, error propagation, partial failures,
  ownership cleanup, and any changed concurrency contract. Use `ASSERT_*` for
  prerequisites and `EXPECT_*` for independent observations. Use justified numeric
  tolerances rather than arbitrary precision or exact comparisons of inexact values.
- Use controlled synchronization for concurrency tests rather than sleeps. Bound
  waits, avoid shared filenames/ports and real brokers in unit tests, and use
  scoped temporary resources. Separate adapter integration tests with explicit
  dependencies, process counts, environment assumptions, and timeouts.
- Make failures explain the violated contract. Never weaken assertions to hide
  nondeterminism, skip a required check without disclosure, or treat an unavailable
  integration test as proof of correctness.
