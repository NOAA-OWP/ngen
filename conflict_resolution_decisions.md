# Source integration decisions

Inputs: master `b43d2766cf98af3c0b3ea01772c24f93709ba101`, ngwpc HEAD
`cec6439dac5665ee90ab64bd579313cab5726f2b`, incoming
`e67ade533bb6250dc0b1d31a490ec0f77a6cab90`. All references were reconfirmed.
Only ngwpc is being edited. No commit, push, branch switch, build or test execution.

Pre-edit snapshot: `/apd_common/christopher.tubbs/tmp/ngwpc-conflict-20261014`.
Includes original index, conflict stage blobs, changed/untracked ordinary files,
dirty submodule files and indexes, binary diffs, inventories against both parents,
and populated submodule HEAD/status records. Extracted reference sources are in
`master/`, `HEAD/`, and `MERGE_HEAD/` beneath that snapshot. These originals and
Git history preserve displaced implementations; they are not active sources.

## Decisions (maintained during execution)

| Group / origin paths | Master contract retained | Contribution disposition and exact integration | Evidence / limitation |
| --- | --- | --- | --- |
| All master gitlinks | Exact 12 paths and commits | Restore index only from pinned master; no nested checkout | Manifest in snapshot `master-pins`; final verification required |
| Five add/add gitlinks | Absent from master | Retain existing stage-2 contribution revisions by default | Incoming-only revisions remain recoverable from MERGE_HEAD; no dependency fetch |
| Output configuration, HEAD and incoming formulation parsers | String lists, ordered defaults, explicit headers, header-count mismatch warning/fallback | Adapt name/header/units/index objects into one ordered OutputSelection sequence; preserve duplicates; reject malformed indices and missing names | Shared Bmi_Formulation::configure_output_selections; omit/empty units means native; valid explicit headers authoritative |
| Single BMI output | Typed values, current-timestep check, manager formatting | Adapt selections through get_value with alias resolution and checked vector bounds; schema uses same records and native-unit API | Bmi_Module_Formulation.hpp/.cpp; metadata errors alone do not block schema; conversions throw |
| Multi BMI output | availableData, deferred providers, final-module default delegation | Adapt explicit selections through supplying providers, retain master sampling time; obtain native units from provider rather than submodule display column | Bmi_Multi_Formulation.cpp; default schema retains final-module conversion metadata |
| Incoming CSV writers / fallback | Output managers own formatting | Ignore formulation CSV assembly and raw fallback on output conversion error; adapt contextual deduplicated diagnostics to typed retrieval and rethrow | Incoming originals in MERGE_HEAD same paths; no raw double mislabeled with requested units |

## Handoff scope

Build/runtime correctness remains unverified, as required by the plan.
Follow-up stabilization must assess
local EWTS installation/pinning, model serialization availability at fixed master
pins, CMake minimum/preset discrepancy, routing protocol compatibility, and restart
state completeness.

## Completed subsystem decisions

All origins below refer to the pinned master, HEAD and incoming revisions named
above, at the listed paths. No newer submodule checkout was used as source truth.

| Paths / feature | Decision, retained master behavior and accepted contribution | Source evidence / limitations |
| --- | --- | --- |
| `src/forcing/NetCDFPerFeatureDataProvider.cpp`, corresponding header | Adapt incoming one-dimensional Time and either data-dimension order to master constructor, CF time metadata/epoch parsing, page cache, ID hints, bounds and resampling. Keep one file-open, raw-time buffer and constructor. Retain master time-stride acceptance rather than an unproven absolute-difference change. | `interpret_time_units`, `get_time_metadata`, `get_value`, `cache::*`, `align_cache_with_chunks`. Incoming bulk Time read could overflow a one-row buffer for old layouts; read one row for 2-D Time. Map time-first reads into the unchanged catchment-major cache layout. Remove an automatic-merge fragment that incorrectly changed the requested stop index when processing ID hints. |
| `DataProvider.hpp`, CSV, NetCDF, forcing-engine, BMI module/multi, wrapped/deferred/optional providers; test doubles | Adapt generic native-unit query; preserve discovery and aliases. CSV uses parsed/default units, NetCDF metadata aliases, engine BMI metadata with suffix resolution, module BMI name resolution, multi availableData, wrappers delegate. Unresolved wrappers reject metadata queries; missing metadata becomes unknown schema units. Null advertises nothing and rejects requests. | `get_provider_units_for_variable` implementations and `get_available_variable_names`; deferred/optional inherit wrapper API. Removed unused formulation unit maps after moving queries to suppliers, retaining provider discovery lists and availableData. |
| Conversion exceptions and diagnostics | Keep master UnitsHelper exception and provider conversion semantics; incoming `data_access::unit_conversion_exception` is an alias, not a competing exception type. Carry provider/requester/catchment/name/units context and deduplication into typed output; rethrow output failures. Existing master response/inter-model raw recovery remains separate and checks nonempty exception values before access. | DataProvider.hpp alias; Bmi_Module/Multi typed retrieval. No conversion-error raw fallback in file output. `none` means native in new selection objects; ambiguous dimensionless sentinel requests are rejected in favor of explicit `1`, preventing misleading schema units under the legacy converter. |
| `src/core/Layer.cpp`, `SurfaceLayer.cpp`, `include/core/DomainLayer.hpp`, layer headers | Keep master typed manager delivery, timestamps/markers, manager members and checked `.at()` routing indexes. Capture routing arrays only under BMI routing guards. Add state dispatch for catchments and domain formulations. | Layer/SurfaceLayer update_models and inline DomainLayer::update_models. Domain state methods explicitly dispatch to its formulation, since it has no base processing_units. |
| `src/NGen.cpp`, `include/core/NgenSimulation.hpp`, `src/core/NgenSimulation.cpp` | Adapt incoming run_ngen wrapper, hot starts, model snapshot and finalization lifecycle to master CLI/runtime information/JSON reader/hydrofabric/GeoPackage/layer/output setup. Keep one MPI init/finalize sequence and Python lifetime outside run_ngen. Restore --info stdout independent of logger settings. | main owns MPI/Python; run_ngen owns managers/features/simulation. Hot start after model initialization and before run_catchments; managers close once after catchments. End-of-run save follows routing, before explicit adapter/provider finalization. Exceptions unwind driver-owned managers while Python/MPI are alive; incoming Python exception-copy/rethrow behavior retained. Failure-path MPI termination remains unverified. |
| Routing adapter, CMake/test registration and fixtures | Retain both master legacy file routing and BMI per-timestep routing/collectives at the required master pin. Restore deleted legacy header, source, test registration and 2015 nexus fixtures/YAML. Retain incoming alternate hydrofabric/nexus files and preserve its YAML separately as `ngen_routing_config_bmi_unit_test.yaml`. | `run_routing` and `run_routing_bmi`; no duplicate post-run nexus CSV writer. BMI adapter is retained as a member so end-of-run serialization can access it, then finalize closes it. Incoming bulk unchecked catchment/nexus injection and one-shot Update are ignored because they replace the master pinned routing protocol; recoverable in incoming NgenSimulation.cpp. Independent framework state interfaces remain. |
| State save headers/sources, examples, BMI formulation/adapter methods, StateSaving.hpp | Accept contributed end-of-run FilePerUnit snapshots and hot-start load/reset APIs with source registration and Boost serialization. Correct single-module hot start FREE message to RESET (StateSaveNames protocol), initialize unused message arguments, reset formulation scheduler index. Retain multi archive serialization. | `save_end_of_run`, `load_hot_start`, Layer state dispatch, module/multi save/load/serialization, Python load_serialization_state and state_save_restore CMake. Master dependency models may lack these BMI messages. Mid-run scheduler/nexus state was TODO in incoming: simulation checkpoint/resume APIs explicitly fail rather than claim success; FirstOfMonth configuration explicitly fails for the same reason. No artificial successful serialization stubs. |
| Python adapter/local refactor | Preserve local type dispatch/copy macros, extended signed/unsigned types, shape handling and unchecked state transfer. Make Finalize idempotent so explicit finalization and destructor do not invoke Python finalize twice. | Bmi_Py_Adapter.hpp/.cpp. No compiled/runtime validation of pointer ownership or numpy buffers. C/C++/Fortran adapters retain existing type/pointer APIs and contributed diagnostics; Fortran pointer access remains unsupported. |
| `src/utilities/parallel_utils.cpp`, header/callers | Resolve add/add with master communicator-taking collectives/subdivision APIs; retain master Python and MPI feature guards. Ignore incoming global-world/rank-count API replacement. | parallel_utils.h/.cpp; NGen and NgenSimulation call with MPI_COMM_WORLD explicitly. Both original implementations in snapshot/history; no branch switch. |
| `src/partitionGenerator.cpp`, Partition_Parser | Keep master sentinel catchment treatment, partition validation, GeoPackage support, fatal failures and throws; incorporate EWTS diagnostics. Do not accept incoming removal of exit/error handling. | Destination validation and catchment/partition-count checks, hydrofabric read blocks. Remove obsolete local logger helper calls by explicit logging plus throw. |
| `src/utilities/bmi/mass_balance.cpp`, header | Retain initialize return discard and return of check_support expected value; incoming unused temporaries are not corrections. Keep end-of-run check at total_steps - 1. | Layer passes zero-based output_time_index and total_output_times; check frequency's negative sentinel therefore targets last computed index. Fatal/warning policy and expected error propagation remain. |
| `extern/cfe/CMakeLists.txt` | Retain supported master BMI/model sources at exact master pin. No logger.c, nash_cascade.c or bmi_serialization.cxx exists in that tree; ignore those incoming source substitutions and EWTS bridge dependency for this model. | `git -C extern/cfe/cfe ls-tree -r 6d08db7... src`; wrapper equals master. No CFE serialization source supported at this pin; framework state support retained but unsupported model combination recorded. |
| `extern/sac-sma/CMakeLists.txt` | One target/source/glob/install definition, retain Fortran middleware; guard shared iso_c_bmi target against duplicate add_subdirectory. Do not inject unverified EWTS linkage. | Neither candidate SAC object is locally available; source paths and EWTS compatibility remain provisional. Wrapper preserved from existing contribution, with incoming comment-only correction accepted. |
| Automatically merged NOAH/TOPMODEL wrappers | Match exact master wrapper/source revisions. Ignore TOPMODEL serialization source absent from pinned tree, and NOAH datetime/EWTS source assumptions absent from pinned source. | Master TOPMODEL tree contains only bmi_topmodel.c/topmodel.c/main.c under src; no bmi_serialization.cpp. NOAH pin has no EWTS references. Local framework Boost/state CMake edits remain. |
| `Formulation_Manager`, config groups/forcing config | Preserve master global output parsing and output-manager construction; remove contributed domain set_output_stream CSV opening. Accept independently useful formulation_groups/forcing_groups and engine init_config provider path. Restore full IDs for existing {{id}} placeholders; retain numeric-suffix helper through additive {{id_numeric}} for forcing patterns/nested init paths. | Manager read/construct/get_forcing_params and config clone support. Numeric-only replacement of existing placeholders would break master cat-N configs; it is not adopted as a default. No new formatting implementation. |
| `JSONProperty`, GeoPackage, HY_Features | Restore master List push_back/equality definitions lost in merge. Preserve public print_property output via contributed string formatter. Accept GeoPackage query batching in groups of 900 with bound parameters, preserving layer/ID/projection/error handling. Keep HY_Features behavior plus EWTS diagnostics. | JSONProperty.cpp/.hpp; geopackage/read.cpp and ngen_sqlite bound-span query API. Batching avoids SQLite variable-limit failure; repeated bbox scanning is an unverified performance concern. |
| MPI nexus | Restore master preposted receives/asynchronous sends, bounded outstanding sends and MPI_Handle_Error calls. Retain independent destructor deadline correction: prior loop could time out immediately at wait_time == 0; new steady-clock deadline drains before sleeping. | HY_PointHydroNexusRemote::post_receives/add_upstream_flow/get_downstream_flow/destructor and header declaration. Cancellation on timeout remains a follow-up defect; blocking incoming send replacement is recoverable in incoming source. |
| EWTS/CMake/logging | One active EWTS logger/bridge. Old HEAD Logger.hpp/Logger.cpp are explicitly marked inactive historical source and unregistered by logging CMake. Keep local installed-package preset and Boost/state CMake edits. Shared forcing-engine storage owns finalization, providers release references; match manager call to storage.finalize. | find_package(ewts CONFIG REQUIRED); ewts::ewts_ngen_bridge links; ForcingsEngineStorage::finalize and idempotent Python adapter. `extern/nwm-ewts/` and CMakePresets remain untracked. No rebuilding/copying dependencies. |
| Docker/CI | Retain contributed setup and master source/output capability. No workflow was run or published; existing workflow branch triggers do not include ngwpc. | .github/workflows/ngwpc-cicd.yml, Dockerfile, docker/CENTOS_TEST.dockerfile. Publishing/promoting/downstream logic remains an unverified contributed proposal, not an authorized release operation. |
| `full_exception` | Excluded. Remove only executable source registration; leave untracked experiment untouched and unstaged. | No active code include/call/reference; no exception redesign. |

## Existing test/specification reconciliation

No tests were executed. These are source specifications for later validation:

- CSV: retain master recognized and non-parsing header cases. Reject the incoming
  `#if 0` disabling non-parsing cases; preserve master exception expectations.
- NetCDF: retain master 2015 fixture dates/numeric expectations, ID hint order,
  resampling/range/variable cases, synthetic cache-page and time-metadata fixtures.
  The incoming 2024 engine file is a separate scenario with its own dates/IDs,
  checking native metadata and time/layout reading rather than overwriting 2015
  expected values. Incoming claimed 17743/17842.5 values and scale/offset assumptions
  remain recoverable in incoming source; not established as corrections here.
- Multi: eight distinct scenarios, keeping vector case at 6 and moved composition
  case at 7 (depth three with C, two without C). Deduplicate repeated case definitions
  and replace exact CSV-string assertions with typed [1,2,3] values plus schema width.
  Legacy order and echoed forcing assertions remain active.
- Single: extend existing BMI C fixture with alias/duplicate selections, native m
  versus requested km, authoritative explicit headers, current-timestep validation
  and rejection of a negative index. Existing master string-order specs remain.
- Restore master Formulation_Manager cases silently deleted/mislabelled by merge
  (`basic_run_9`, both init_config pattern tests, and the proper `basic_run_3`). No
  incoming additional test scenario was present in that diff to preserve.
- Restore legacy routing test and registration alongside contributed routing inputs.

## Output ownership and source trace

The pinned master output directories and global output configuration headers remain
unchanged. CatchmentCsvOutputMgr still owns row/header serialization, per-feature or
per-formulation grouping and precision; PerNexusCsvOutputMgr / PerFormulationNexusOutputMgr
own nexus backends. No alternative formulation CSV method or post-run writer remains.

1. Master string example `["OUTPUT_VAR_2", "OUTPUT_VAR_1"]`: module
   `inner_create_formulation` calls `configure_output_selections`; records retain that
   order with native-unit requests. `get_output_fields` resolves native units for
   those sources; `get_output_values_for_timestep` validates current time and calls
   get_value in the identical record order. NGen descriptors are made from that
   schema; Layer::update_models supplies the values and time_marker to
   CatchmentOutputsMgr::receive_data_entry. CatchmentCsvOutputMgr writes configured
   output names and doubles at manager precision, then NGen closes it.
2. Contributed object example (existing multi vector fixture): three OUTPUT_VAR_3
   records with indices 0,1,2 become three columns, not one map entry. Multi parser
   uses the same normalizer, availableData supplies the chosen provider, master
   model epoch/forcing-offset sampling is retained, and units/index are passed in
   CatchmentAggrDataSelector. Multi schema queries the supplying provider's native
   units, not a nested module's potentially converted display column. The same
   Layer/manager trace serializes the three values. The single BMI C object fixture
   additionally traces alias echo and actual m-to-km requested conversion, with the
   corresponding m/km schema and outer explicit header labels.
3. Default multi output delegates values and fields to the final module (including
   its selected units/indices); outer headers rename those fields positionally.
   DomainLayer uses its own feature descriptor/time_marker and the same manager
   type. SurfaceLayer sends nexus values on each timestep and commits the nexus
   manager. NGen passes all manager arguments to layers and closes each once.

Master defaults: enabled CSV, per-feature grouping, configured output root/precision,
rank-root policy, native-unit string selections, master default variable order,
header-count warning/fallback, final-module multi delegation, and manager-owned
formatting are retained. No new output backend or changed default is introduced.

## Dependency pins and preserved local work

All 12 required master pins in the index:

| Path | Commit |
| --- | --- |
| `extern/SoilFreezeThaw/SoilFreezeThaw` | `bddfef82b6377fc17a1e51d4206e68e91912c26e` |
| `extern/SoilMoistureProfiles/SoilMoistureProfiles` | `87631bbd310d8a99fab144356f1f98ece289dde5` |
| `extern/bmi-cxx` | `7c9a1d07bfb566027df337bbf83051da7fac4004` |
| `extern/cfe/cfe` | `6d08db7bec2ec6cc2cc2446300c9e0d2076e0fc7` |
| `extern/evapotranspiration/evapotranspiration` | `1eb7bd765f1810419923aad30ae95926089c0aac` |
| `extern/netcdf-cxx4/netcdf-cxx4` | `a43d6d4d415d407712c246faca553bd951730dc1` |
| `extern/noah-owp-modular/noah-owp-modular` | `30d0f53e8c14acc4ce74018e06ff7c9410ecc13c` |
| `extern/pybind11` | `5b0a6fc2017fcc176545afe3e09c9f9885283242` |
| `extern/sloth` | `48cd8082878fdb8a6f0e2337a5b375be5b71384c` |
| `extern/t-route` | `6f8033b713f5c8ce4a59ef20b93851500a79c12c` |
| `extern/topmodel/topmodel` | `a4cb3ba3d64e41a506ebbdd49c4215f8041b88e1` |
| `test/googletest` | `58d77fa8070e8cec2dc1ed015d66b454c8d78850` |

Contribution-only stage-0 pins (existing stage 2, or existing staged addition):

| Path | Commit |
| --- | --- |
| `extern/LASAM` | `d20e7e357c23c1230b27a4d0b896f1979638f9c7` |
| `extern/lstm` | `341be45ed854442459ad2f4ed05532b5eb5406fe` |
| `extern/sac-sma/sac-sma` | `b40f61ca9e82d4c4d0fc6171b314714af0160ab3` |
| `extern/snow17` | `ee83d3d659f105377a0f276952dbf408a98812b6` |
| `extern/topoflow-glacier` | `3844fba45952675407401b3247d4177542b1ed7b` |
| `extern/ueb-bmi` | `6c70fe9e7386a0607b07d54e8f874bd6faa76b9e` |

Stage-2/stage-3 histories could not be inspected for LASAM, lstm, SAC and snow17:
objects/checkouts are unavailable locally. Retaining stage 2 is the prescribed
provisional default, not a claim those commits are published or obtainable. UEB
history is available; incoming changes include warning throttling, EWTS and units
work, but no established requirement justifies replacing the existing holding pin.
Its incoming revision stays recoverable from MERGE_HEAD. t-route is unconditionally
pinned to master; its incoming history replaces large parts of the routing package
and does not authorize changing that pin. Contribution-only .gitmodules entries are
added from incoming; every master module setting is retained verbatim. Retrieval
from those remotes, including stage-2 commit availability, is unverified.

Populated nested repository HEADs, indexes and dirty/untracked work were not changed.
Parent status can show submodule differences because the checkout still differs
from the mandatory index pin. Snapshot `submodule-status.json` records original
HEAD/status; `gitlink-provenance.json` records candidate object availability.

The original `.clang-tidy` edit remains unstaged. CMakePresets.json, scripts/,
extern/nwm-ewts/, both forcing-provider notes, conflict_resolution.md and the
experimental include/full_exception.hpp remain untracked and unstaged. The preset
continues to use `${sourceDir}/cmake-build-ewts/install/lib64/cmake/ewts`. No local
installation, binary, branch reference or nested checkout was replaced. The
original Python adapter and local Boost/state CMake proposals were reviewed and
retained in the integration. No blanket staging command was used.

## Prioritized defects for the stabilization-planning session

1. Establish compatible serialization support per model at the fixed master pins.
   CFE and TOPMODEL pins have no contributed serialization sources; Fortran pointer
   access is unsupported. Routing serialization/reset messages are unverified at
   pinned t-route. Unsupported models must fail, not fabricate state.
2. Design actual mid-run scheduler/layer/model/nexus checkpoint state and triggers
   before enabling resume or FirstOfMonth. Current end-of-run/hot-start support
   resets a new run to timestep zero; it does not resume partial output files.
   Incoming full-resume methods had unsaved scheduler/nexus TODOs, so those APIs now
   fail explicitly. Do not silently enable them based on the archive interfaces.
3. Audit FilePerUnit error detection and durability: streams do not currently enable
   exceptions/check all status bits, and strict fsync is TODO. NGen now calls snapshot
   finish_saving and loader/saver finalize, but these hooks do not guarantee durable
   disk state. Validate shared-filesystem MPI writes and unit-name uniqueness.
4. Make EWTS installation reproducible with a dependency source/pin. The current
   untracked dependency tree and installed-package preset are local proposals.
   Confirm target names/ABI and all optional-build link closures. Root Boost minimum
   and some contributed component searches differ; CMake minimum versus preset
   schema also needs a workability plan. No configure/build gate was imposed here.
5. Validate lifecycle failure paths, Python destructor behavior, reference ownership
   and numpy state-buffer sizes. Explicit Python Finalize is now idempotent on the
   successful path, but failed finalize/destructors remain unverified. Check MPI
   exception shutdown and outstanding nexus request cancellation on timeout.
6. Validate unit conversions, aliases, output schema/value alignment and vector bounds
   with real model fixtures, including duplicate columns and echoed forcings.
   Master response/inter-model raw recovery policy remains distinct from output
   failure policy and needs a separate review. Deduplication's shared incoming set
   has no new thread-safety guarantee. CSV range/clamping and multi's fixed 3600-second
   output sampling retain master behavior; do not infer correctness for other steps.
7. Verify old and engine NetCDF layouts, cache page boundaries, time-unit fallback,
   scale/offset conventions and ID hints against actual files. Incoming 2024 numeric
   assertions were not evidence of a correction to the 2015 fixture. GeoPackage
   batching and wrapper/source availability need validation. SAC and several added
   model objects are unavailable locally.
8. Review contributed Docker/CI publish/promote/downstream logic separately before
   any release use. Nothing was run, committed, pushed or deployed in this session.
   Optional coastal hooks remain unverified; this effort makes no readiness claim.

## Completion verification

Only static/source and Git index checks are used. Results are appended after staging.
Compilation, linking, dependency installation, execution and passing tests were not
required and were not used to certify this branch. The next session plans
stabilization; it does not inherit an assertion that this source will build or run.

### Final review (2026-10-15)

- All 19 recorded conflict paths are resolved: no unmerged index entries remain.
- The supplied exact comparison confirms all 12 master gitlinks at stage 0.
  Contribution-only resolutions retain their documented pins.
- Static source review found no conflict markers, duplicate test names or unbalanced
  braces/conditional directives in integrated code. Public typed output declarations,
  implementations and layer/manager callers agree; legacy CSV methods are absent.
- Staging uses the explicit 70-path manifest at the snapshot's `stage-paths.txt`;
  pre-existing staged changes were retained. The experiment and other listed
  untracked proposals were excluded. Branch pointers remain exactly as recorded.
- Final cached whitespace review identified whitespace in restored legacy routing
  files; cleaned those lines without behavioral changes. CSV conversion diagnostics
  now use the selector's catchment ID rather than an uninitialized contributed field.
- No branch switch, commit, push, dependency install, configure, build, test execution
  or publication was performed. The merge remains in progress, ready for review as
  staged source integration; recording a merge commit is deliberately left undone.
- Prioritized unresolved technical limitations above are the handoff to the separate
  stabilization-planning session. Static integration checks do not certify runtime
  behavior or buildability.
