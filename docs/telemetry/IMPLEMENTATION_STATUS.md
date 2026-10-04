# Telemetry expansion implementation status

The accepted scope is [BALANCE_EXPANSION_PLAN.md](BALANCE_EXPANSION_PLAN.md),
including the recovered requirements from PR #677. Implementation is in progress.
Technical acceptance uses disposable local databases and a personal local game
setup. Production or staging access is not a prerequisite for completing this
work. A future production deployment is a separate operational decision.

## Delivery record

| Requirement | State | Evidence or remaining work |
| --- | --- | --- |
| #561: full writer startup contract | Implemented and locally qualified | Validate every current writer column, type, signedness, width, nullable/default semantics, InnoDB engine and 16 required indexes. Exercise effective SELECT/INSERT/session UPDATE with zero-row statements and rollback. Reject admission until the worker qualifies; later transient outages retain buffering. |
| #565: shared serialization/schema descriptor | Implemented and locally qualified | `telemetry_columns.inc` supplies the column identities used by serializers and startup validation. Preserve all record kinds 1–13, admitted replay keys and column prefixes. Reserved/padding/absent union fields and sink-generated fields have explicit exclusions. |
| #566: durable outage/loss evidence | Implemented and locally qualified | Worker registration before SQL initialization/admission; protected exclusive checksummed storage; bounded coherent samples; clean drain versus known abandonment and unknown tails; real restart/exec/SIGKILL and storage-failure tests. Offline read-only export preserves unknown ends. See [OUTAGE_STORAGE.md](OUTAGE_STORAGE.md) and the qualified #567 report integration below. |
| #567: reviewed incident coverage | Implemented and locally qualified | Consecutive retained inventory versions, nullable unknown ends, committed first verified post-fix references, explicit backlog/reconstruction dispositions and atomic published coverage snapshots. Reports preserve gaps, source uncertainty and bounded private-role separation. Full local MariaDB/MySQL chains, capacity, digest/permission negatives, lost commit replies and unchanged v1 totals qualified. Historical facts require evidence; synthetic fixtures do not establish a real incident history. See [INCIDENT_COVERAGE.md](INCIDENT_COVERAGE.md). |
| Initial session qualification/capacity recovery | Implemented and locally qualified | Existing descriptor sweep and context/evidence adapters retry missing entry. Deferred copyover retains one handoff in descriptor memory; supplied keys/totals/revision survive. No earlier unobserved time or human activity is invented. True capacity refusal rolls back IDs; lifecycle queue loss retains admitted IDs. See [SESSION_LIFECYCLE.md](SESSION_LIFECYCLE.md). |
| Publish existing progression/encounter/combat observations | Implemented and locally qualified | Definition 2 publishes five bounded projections from typed kinds 6–8. Cumulative participant/actor facts replace earlier measurements, threshold consumption stays separate from XP, unknown tails remain NULL, and published read-only snapshots retain incident coverage. Both engines passed the full 55-step chain and replay, rollback, constraints and role negatives. See [OBSERVATION_PROJECTIONS.md](OBSERVATION_PROJECTIONS.md). |
| Native account lifetime and scoped token preparation | Implemented and locally qualified | Migration 0056 retains retired lifetimes, follows actual renames and issues one opaque token per scoped lifetime. Account-load caching, transaction/entropy/allocation failures, ambiguity, simultaneous preparation, deletion/recreation and restricted permissions passed on both local SQL engines. Preparation itself emits no authentication or participation evidence. See [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md). |
| Authenticated ownership observations | Implemented and locally qualified | Kind 9 observes scoped cached identity only on an authenticated playing descriptor with matching current account membership. Bounded admission recovery preserves original clock/payload, explicit loss spans and fresh transport key order. Reconnect/copyover, missing identity, logout retention, native writer replay/permissions, mixed-stream report compatibility and ownership outage export are qualified. See [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md). |
| Restricted reviewed identity registration and generation reservation | Implemented and locally qualified | Migration 0058 authenticates a provisioned SQL reviewer through the maintained registrar, verifies scoped issued account tokens, retains dated correction histories and reserves an immutable reviewed version/digest or explicit unknown identity per new balance generation. Both final 58-step chains, CLI, read-transaction release, capacity, permissions, rollback/ambiguity and exact fresh/restored fingerprints passed. Reservations are metadata and do not publish effort or imply complete identity. See [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md). |
| Copyover ownership context and ownership-loss review | Implemented and locally qualified | Outer copyover 18/telemetry-v2 retains last observed account context without importing an old monotonic clock or using the token as current authority. Legacy framing, overflow/unknown context and actual reloaded-token/scope/deletion changes passed native and ASan/UBSan fixtures. Migration 0059 adds independent incident schema-v2 inputs for families 1–9 and reuses existing snapshots; both 59-step chains, CLI, capacity, replay, private roles, atomic snapshot rollback and exact fresh/restored fingerprints passed. Definition-3 integration is qualified below. See [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md) and [INCIDENT_COVERAGE.md](INCIDENT_COVERAGE.md). |
| Atomic identity effort and observed XP portfolio publication | Implemented and locally qualified | Migration 0060 adds exact bounded source retention tied to the cursor, a conserved publication header and two reports. Definition 3 consumes the reserved dated review and incident schema 2 in the same publication transaction. Both 60-step chains qualified the actual CLI, corrections, old generations, explicit unknown identity, source/publication rollback, real committed writes with lost acknowledgements, private roles, budget refusal, guarded reruns and exact fresh/restored fingerprints. Definitions 1/2 retain their earlier amounts. See [IDENTITY_PUBLICATION.md](IDENTITY_PUBLICATION.md). |
| Durable shared-battle facts and loss review | Implemented and locally qualified | Migration 0061 maps all 70 canonical kind-10 fields with independent logical/transport replay, immutable configuration qualification and NULL family separation. Both 61-step chains qualified actual native packets, lost acknowledgements, header/scope/constraint refusals, exact quarantine evidence, guarded reruns, drift/restoration, private v3 review CLI and unchanged definitions 1/2/3. Outage v2 retains readable original v1 histories. Live capture and atomic battle publication remain separate requirements. See [BATTLES.md](BATTLES.md). |
| Bounded shared-battle history and contribution linkage | Implemented and locally qualified | The pure reducer checks complete revision/fact history, actual cross-component alias bridges, conservative rosters/graphs, cumulative effort, exact effective contribution context and lifecycle cuts. Twenty regressions cover 6,468 pure source facts/23 journeys and native missing/conflicting/context/lifecycle/incident/budget cases. Native normal/ASan/UBSan and both actual SQL readbacks qualify 123 facts/38 packets/28 verified contribution links, five canonical battles and conserved 112/112 damage. Migration 0064 consumes this reducer in the publisher described below. See [BATTLES.md](BATTLES.md#bounded-history-qualification-for-publication). |
| Historical battle exposure and retained-input/checkpoint contracts | Implemented and locally qualified | Thirty-nine regressions preserve the original history proof and add exact historical actor/scope/roster intervals, all eight native effort partitions, composition-aware coalescing, alias/inactivity/loss cuts and source count/digest/cursor/receipt/type/budget negatives. Normal, fresh ASan/UBSan and both private-writer SQL readbacks qualify the same source/exposure contracts and 28 contribution links. Actual SQL ingestion IDs/arrival labels survive source restoration; export-only arrival remains unknown. The original 151-input value-contract stage reserves 4,952,064 bytes internally; persisted source and atomic observation publication are qualified in the following rows. See [BATTLES.md](BATTLES.md#historical-exposure-and-retained-source-checkpoint). |
| Persisted battle source and cursor checkpoint | Implemented and locally qualified | Migration 0063 requires state and immutable identity reservation, retains exact kind-9/10/11 inputs/counts/digest plus the original ingestion boundary, and commits the header with the existing page cursor transaction. Forty-five focused regressions and both full-63-step native-writer SQL journeys qualify selected values, rollback/lost acknowledgements, receipt/constraint guards, original arrival clocks, cursor/origin conflicts, explicit empty building windows, raw retention, bounded consistent reads, CLI preparation, private roles, guarded reruns and drift/restoration. The native selected window contains 153 inputs, including two ownership observations; matching/foreign synthetic ownership cases preserve scope and expand selected evidence once. At that delivery definition 5 prepared private source only; migration 0064 adds publication below. See [BATTLES.md](BATTLES.md#persisted-battle-source-preparation). |
| Atomic battle observation publication | Implemented and locally qualified | Migration 0064 publishes source coverage, canonical/original battles, latest actors, disjoint contributions, dated exposure and every original association with reserved identity and independent schema-4 loss coverage in one transaction. Fifty-six focused regressions and both full-64-step native-writer SQL journeys qualify exact native values, ownership/review/day cuts, explicit unknowns, rollback, lost acknowledgements, immutable older generations, snapshot tamper refusal, bounds, restricted CLI/report roles and guarded drift/restoration. Definition 5 exposes five observation reports; definitions 1/2/3 retain their meanings. Complete balance suites and remaining native/personal-server evidence remain required. See [BATTLES.md](BATTLES.md#atomic-battle-observation-publication). |
| Character/account/confirmed controller association | In progress | Dated review/correction, ownership cuts, same-producer attribution, unknown linkage, native lifetime/token allocation, authenticated source capture, identity wire handoff, reviewed SQL registration and immutable generation reservations are qualified. Atomic identity effort/report publication now consumes the reserved review and qualified ownership incident snapshot. Real personal source journeys and linkage coverage in the complete balance suites remain required. See [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md). |
| Accepted blindness and stun applications | Implemented and locally qualified | Actual `blind`/`Stun` success boundaries feed the existing bounded kind-11 accumulator after effect mutation and before teardown. New segments declare producer mask 31; older unavailable control remains NULL. Normal/ASan/UBSan, 58 history/publication regressions and both full-64-step native-writer SQL journeys preserve eight applications/eight received through eight verified published segments. Source-service seams, partial producer coverage and unobserved duration/resistance remain explicit. See [BATTLES.md](BATTLES.md#accepted-blindness-and-stun-control-capture). |
| Expanded accepted status-control applications | Implemented and locally qualified | Actual major/minor paralysis, slow, sleep, silence and entangle success boundaries feed the existing kind-11 accumulator after mutation. The separate fresh journey preserves 17 accepted applications/17 received, 28 native source rows and eight published segments; the original blindness/Stun journey remains eight/eight. Normal/ASan/UBSan, 66 history/publication regressions and both full-66-step native-writer SQL journeys qualify the added producers and unchanged earlier publications. Counts include accepted refreshes; segment modifier flags are unions and cannot apportion self/external totals. Typed attempts, resistance, immunity, duration and complete producer coverage remain required. See [BATTLES.md](BATTLES.md#expanded-accepted-status-control-capture). |
| Typed control resolution and target-state storage | Partially implemented and locally qualified | Kind 13 preserves exact per-operation outcomes from the maintained selected-control helpers/spell bodies and separate overlap-safe target-state prefixes. The independent 76-field codec, bounded accumulator, private writer, full 67-step storage chains and schema-6 incident review are qualified. Native state coverage remains explicitly partial; complete mutation/removal inventory, retained control publication and personal-server duration journeys remain required. See [CONTROL_OBSERVATIONS.md](CONTROL_OBSERVATIONS.md). |
| Shared battles and changing rosters | In progress | Native callbacks capture shared hostile, effective-support and proven formal-presence graphs through kind 10, and disjoint damage/healing/accepted-control/casting/opponent-link contributions through kind 11. Shared clocks, context/owner/group/mode/configuration cuts, immutable aliases, distinct NPC lifetimes, retained pet teardown, exact session exit and observed-prefix inactivity preserve measured amounts. Definition 5 publishes these observations with exact association/alias/source and independent loss coverage. Kind-13 selected-control results now distinguish applied, saved, resisted, immune and other rejection boundaries. Complete effect producers/removals, qualified duration, retained control publication, prevention and faction exposure remain required. Complete build/power/arena comparisons, typed outcomes, the full balance suites and personal-server journeys remain required. See [BATTLES.md](BATTLES.md). |
| PvE zone attempts and committed rewards | Pending | Separate attempt identity, supported objective evidence, PvP interruptions, effort and exact operation-ID linkage. Generic zone entry or one kill must not imply a full clear. |
| Progression and portfolios | In progress | Observed XP/level projections and published exact account/controller effort unions and observed XP portfolio amounts are qualified. Rested/assistance provenance, milestone exposure/censoring, switching, canonical comparable rewards and rates remain required. |
| Four balance report suites and study exports | Pending | Racewar, solo/group PvP, zone and progression reports from published aggregates; uncertainty, repeated-team influence and coverage visible. Preserve existing report definitions. |
| #487: economic projection compatibility | Pending audit and implementation | Reuse current accounting reconciliation where it meets acceptance; project canonical earned receipts without counting compatibility ledgers, transfers or openings as new rewards. |
| #258: local observational acceptance and final runbook | In progress | Initial readiness/capacity recovery is implemented and qualified in executable fixtures. Remaining: dedicated roles/catalog, real session/progression readback, normal gameplay/save, failure/recovery and a single reproducible local qualification command. |
| Native compact build snapshot reader | Reader and cached capture qualified | Version-1 value reader preserves exact class masks/spec, base/effective resources/stats, fixed equipment declarations, learned epic skill fingerprints, bounded listed effects and independent arena room/roster observations. The 440-byte snapshot has explicit unavailable/partial families. A fixed cache gates entry/change/configuration/periodic/recovery reads before gear/epic scans, including combat callbacks. Normal/ASan/UBSan, lifetime/cap/queue-loss checks, both runtime variants, history/publication regressions and the server build pass. Exact point publication is qualified below; complete reviewed classification, comparison suites and actual personal-server evidence remain required. See [BATTLES.md](BATTLES.md#native-cached-build-capture). |
| Durable selected build observations and loss review | Storage, native capture and point publication qualified | Kind 12 has 110 typed fields, a 447-byte portable encoding and a 448-byte C++ payload. Migration 0065, exact logical replay, DMSTLJ04 and independent private incident schema 5 remain qualified. Native cached capture supplies fresh point keys and empty unavailable markers. Migration 0066 retains and publishes the exact points under independent definition 6, as described below. Existing definitions 1/2/3/5 retain their meanings. See [BATTLES.md](BATTLES.md#native-cached-build-capture). |
| Retained build point publication | Implemented and locally qualified | Definition 6 retains all 110 kind-12 fields, arrival labels and configuration evidence with the cursor; publishes exact points and independent schema-5 coverage through the existing bounded pipeline. Sixty-five history/publication regressions and both full-66-step native-writer SQL journeys qualify all 20 points, missing/stale/retired references, clock and configuration uncertainty, independent kind-10/12 loss, review corrections, old-generation immutability, rollback, lost acknowledgements, raw-retention independence, the restricted report CLI and guarded schema drift/restoration. Points establish no continuous build exposure or damage attribution. Full comparison suites and personal-server evidence remain required. See [BATTLES.md](BATTLES.md#retained-build-point-publication). |

## Qualified first-layer checks

The native control-state increment adds final mutation callbacks and nested
remove/reapply suppression to the existing bounded accumulator. The callback
fixture verifies 18 final transitions without build hashing or outside/inactive
enrollment. The maintained-affect ASan/UBSan journey verifies 26 final transitions
across all eight selected states, expiry, overlap, refresh, refused unlink,
`NOAPPLY`, equipment/save-style rebuilding and actual cure/broken-song callers;
10 short timers are canceled and the scope finishes before character destruction.
The full selected mutation/removal inventory, complete attempt denominators,
qualified duration/action restrictions, retained definition-7 comparisons and
personal-server qualification remain required. Live duration coverage stays zero
with `CONTEXT_UNKNOWN`; all seven final requirements in issue #258 remain open.

The SQL fixture loaded and verified the complete accounting migration manifest:
53 steps through `0053_craft_progression`, independently on MariaDB 10.11.14 and
MySQL 8.0.46. Each engine passed all eight record kinds, ten golden fixtures,
schema drift and effective permission negatives. A documented least privilege
writer can ingest; SELECT-only and missing session UPDATE cannot. Readiness
probes add no synthetic fact or session rows.

The focused checks passed:

```sh
python3 tests/async/test_telemetry_repository.py
python3 tests/async/test_telemetry_transport.py
python3 tests/async/test_telemetry_runtime_integration.py
python3 tests/async/test_telemetry_runtime_exhaustion.py
python3 tests/async/test_telemetry_gameplay_adapters.py
python3 tests/async/test_telemetry_gameplay_hooks.py
python3 tests/async/test_telemetry_transport_integration.py
python3 tests/async/test_telemetry_contract_headers.py
python3 tests/async/test_telemetry_fault_272.py
python3 tests/async/test_telemetry_capacity_272.py
python3 tests/async/test_telemetry_preflight.py
```

Transport normal, AddressSanitizer and UndefinedBehaviorSanitizer variants passed.
ThreadSanitizer could not initialize on this host: its trivial capability probe
failed with an unsupported address map. This is an unrun check, not a pass.
Capacity qualification passed 50, 200 and 1,000 record workloads with three
repetitions. The maintained SQL C++20 server built with `make -C src -j4`.
Touched C/C++ files passed the repository formatter.

The portable SQL wrapper for a Linux host with Docker creates and destroys its
own disposable fixture; it does not read checkout credentials:

```sh
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh
```

On this Windows host, equivalent task-owned containers shared an isolated network
namespace. The repository harness used explicit loopback fixture settings and
`--sql-fixture`; both engines loaded their own authoritative chain. No production,
staging, primary checkout environment file, player data or existing game process
was used.

## Qualified durable outage evidence

`test_telemetry_outage.py` passed native lifecycle, safe storage, checksum and
semantic corruption, ENOSPC/write/fsync/rename publication faults, idempotent
recovery, monotonic chain/history refusal, simultaneous process ownership, real
SIGKILL and exec, changed protection/ownership, full producer quota and offline
read-only export. `test_telemetry_runtime_outage.py` passed SQL-header and
client-free variants: registration before repository initialization/admission,
clean drain/restart, transient SQL recovery, unresolved commit shutdown, and
disk-full startup/checkpoints. Instrumented writes/fsync confirmed worker-only I/O.

## Qualified incident coverage and publication

The final incident fixture passed on MariaDB 10.11.14 and MySQL 8.0.46. Each loaded
and verified the complete 54-step accounting manifest through
`0054_telemetry_incident_coverage`. Engine-measured normalized fingerprints, all
three retained migration histories, compiled boot constants and the protected
lifecycle inventory are synchronized. Existing sealed SQL/verifier content and
checksums were preserved. The inventory now includes 229 SQL tables and 51
non-database stores; destructive rules remain disabled.

Thirteen offline incident tests passed. The actual SQL journeys qualified strict
and complete packets, committed post-fix reference/mismatch refusal, exact and
conflicting retries, retained corrections, explicit withdrawal, lost review and
publication commit acknowledgements, source-digest mutation refusal, full
64-incident capacity, byte-budget refusal before detail fetching, unknown and
reconstructed tails, incremental/ambiguous UTC windows, missing-inventory scope,
published report/cache visibility and distinct registrar/rollup/report permissions.
SQL constraints reject partial identities, invalid families and reversed times.
The migration verifier checks exact columns, indexes, check expressions, same-schema
foreign keys and effective check enforcement; boot fingerprints cover the new
metadata and constraints on both engines.

The existing administrator report contracts (14), pure rollup semantics (14),
rollup budget tests (7), immutable runner tests (24), runtime boot contracts (10),
lifecycle inventory (22), collector schema (4) and corpse repository contracts (7)
passed. The maintained SQL server build and formatting checks passed.

```sh
python3 tests/async/test_telemetry_incidents.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --incidents
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --incidents
```

These reliability and incident checks do not establish battle balance, controller
linkage coverage, zone difficulty or progression speed. Those require the
remaining implementations above and the end-to-end local qualification. Session
recovery and publication of existing progression/encounter/combat facts are
qualified below. No production or staging access is required for that work.

## Qualified session admission recovery

The focused gameplay adapter executable passed delayed repository qualification
followed by capture without reconnect, first command/context recovery, switched
player ownership, supplied and absent copyover handoffs, teardown cancellation,
invalid/partial identity negatives, state-capacity refusal followed by recovery,
and admitted lifecycle queue loss. No new interval starts before the successful
retry, repeated presence observations keep the same IDs, and input-free presence
does not create active time. Supplied copyover preserves the session key,
checkpoint revision and cumulative totals; absent copyover preserves unclosed-tail
quality. The fixture's stale world pointers are cleared before their stack storage
expires.

The 14 actual copyover framing/recovery seams, existing gameplay hook contracts,
standalone contract compilation, and SQL/client-free runtime lifecycle and outage
journeys passed. The gameplay executable also passed AddressSanitizer and
UndefinedBehaviorSanitizer after correcting the stale fixture pointers. The
maintained SQL server built and touched C/C++ passed
formatting. These proofs use executable local fixtures; the personal server
gameplay, save, persistence/readback and measured latency qualification remains
required by #258.

```sh
python3 tests/async/test_telemetry_gameplay_adapters.py
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
python3 tests/async/test_telemetry_copyover_format.py
python3 tests/async/test_telemetry_runtime_integration.py
python3 tests/async/test_telemetry_runtime_outage.py
```

## Qualified collected-observation projections

The final definition 2 fixture passed on MariaDB 10.11.14 and MySQL 8.0.46. Each
loaded and verified the complete 55-step accounting manifest through
`0055_telemetry_observation_projections`. Both measured metadata fingerprints,
all three retained migration histories, compiled boot constants and the lifecycle
inventory agree. That qualification covered 234 SQL tables and 51
non-database stores; destructive rules remain disabled. Previously sealed
migration SQL, verifiers and history prefixes remain unchanged.

The five published reports retain daily signed/source-separated XP, original
level transitions, original encounters, cumulative participant effort and latest
absolute actor contributions. Level modifier flags are retained; consumed level
thresholds cannot become XP rewards. Pet and NPC PIDs retain the canonical -1
unknown sentinel; pets keep their captured owner without adding a human. Captured
power remains a level proxy. Missing start, close, measured effort and UTC labels
retain their independent uncertainty. An original encounter supplies no shared
battle identity or full zone-clear evidence.

Twelve pure observation regressions and twelve source-review semantic regressions
passed. Real SQL journeys qualified mixed-stream pagination, all five published
report snapshots, restricted report/rollup permissions, absolute cumulative
replacement, lost commit replies, repeat consumption, unchanged definition 1
playtime, unknown tails/dates, and row-byte/fanout/statement budgets. A conflicting
later actor snapshot rolled back both the earlier XP write and the ingestion
cursor. Direct constraint negatives and exact migration-verifier drift checks
passed on both engines. Definition 1 now validates and advances through kinds
6–8 without assigning them playtime; unsupported or malformed active payloads
still refuse cursor acknowledgement.

The existing pure rollup semantics, administrator report contracts and budget
checks passed. Final immutable migration, runtime boot, lifecycle and maintained
SQL build checks passed; touched C/C++ passed formatting. ThreadSanitizer remains
unrun for the host limitation recorded above. The portable wrapper and the
equivalent Windows task-owned SQL fixture require no checkout credentials,
production or staging service.

```sh
python3 tests/async/test_telemetry_observations.py
python3 tests/async/telemetry_rollup_review_semantics.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --observations
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --observations
```

This completes publication of the facts already collected. Account/controller
history, shared battles and context segments, zone attempts and canonical rewards,
portfolio progression, the four balance suites and the final personal-server
qualification remain in the accepted scope. Rates and population claims require
those implementations and their covered denominators.

## Qualified offline identity and effort semantics

`test_telemetry_identity_history.py` passed 25 executable offline tests. Complete
review packets use scoped opaque tokens, consecutive versions and preceding
digests, known starts, nullable ends clipped to the reviewed window, explicit
unknown linkage and retained withdrawal. Corrections preserve earlier versions;
overlapping active associations, removed IDs, changed account identities, scope
changes and conflicting retries refuse the candidate.

Interval attribution preserves original session and observation producer keys.
It never backfills account ownership before its first capture, never compares
monotonic timestamps across copyover processes, and conserves measured duration
through observed ownership and reviewed linkage cuts. Unknown or ambiguous UTC
preserves character effort while withholding dated controller and complete union
claims. Six overlapping linked characters produce six units of character effort
and one unit of controller union; consecutive rotation produces the full
non-overlapping union. Unknown controllers cannot form one combined person.
Presence and active time remain independent, configurations remain separate, and
replay/overlap/mixed-version/overflow negatives passed.

The existing 13 incident tests passed after reusing its bounded evidence decoder;
incident packets retain their original 128 KiB limit. Both packet families now
classify excessive numeric encodings without exposing their private source or
path. Identity packets and attribution/effort inputs have explicit finite bounds.

These tests do not establish live account/controller collection or SQL publication.
The live source, handoff and restricted registry have their separate native/SQL
qualification below. Published effort generation integration and the final real
gameplay qualification remain required. Native account lifetime allocation and
account-load token caching are qualified below. No production or staging access
is required for those remaining checks.

## Qualified native account identity preparation

The final `test_telemetry_account_identity.py --sql-fixture` passed on MariaDB
10.11.14 and MySQL 8.0.46. Each loaded and verified the full 56-step canonical
chain through `0056_telemetry_account_identity`. All three retained migration
histories, compiled boot constants and lifecycle inventory are synchronized.
The current inventory is 236 SQL tables and 51 non-database stores. Earlier sealed
migrations and verifier checksums remain unchanged. The two new stores are
protected pseudonymous subject data with retained lifetime/token history;
destructive rules remain disabled.

The fixture executes the production C++ allocator against each real engine.
Repeated and case-alias loads reuse one token; scope changes prepare another;
renames preserve the lifetime, while deletion retires its binding and name reuse
allocates another lifetime. Two simultaneous account loads receive the same
committed token. Missing/fenced accounts, invalid scopes and oversized names
retain unknown identity; quoted names and injection-shaped missing names are
handled as escaped literals. Native allocation does not query historical
character ownership or infer a lifetime from creation timestamps.

Caller transactions remain owned by the caller. Zero/colliding entropy has a
fixed four-attempt ceiling, entropy and heap-allocation failures return unknown,
and later write/commit failures roll back earlier allocation. A lost commit reply
withholds the output token; retry reads the durable original without allocating
again. Minimum account-owner permissions qualify; missing MySQL account-lock
privilege refuses preparation without creating records. Identity-store
UPDATE/DELETE and private account fields are denied to the restricted preparation
fixture, and the SELECT-only report role cannot read accounts or either identity
store. Direct constraint negatives and strict check-expression drift passed.

Four local cache/schema tests passed, including execution of the actual runtime
helper in SQL-header and client-free variants and compilation of the actual
client-free allocator. Account reload clears stale cache values, including before
a failed read. Preparation failure does not change account-load success. Existing
account-character projection, runtime lifecycle/copyover and gameplay-adapter
checks passed. The 25 offline identity and 13 incident regressions also passed;
immutable history (24), runtime boot (10) and lifecycle inventory (22) checks
passed against the updated head. The maintained SQL C++20 server built after the
final fingerprint seal, and touched C/C++ passed the repository formatter.

```sh
python3 tests/async/test_telemetry_account_identity.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --identity
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --identity
```

Account-load preparation can occur before password verification and emits no
ownership fact. Allocator row counts cannot stand in for participating accounts.
Typed authenticated source capture, identity handoff and reviewed SQL association
registration are qualified below. Published portfolios, shared battles/zone attempts, four
balance suites, canonical economic rewards and final personal-server gate remain
in the accepted implementation scope. Production and staging access are not
prerequisites.

## Qualified authenticated ownership source

Migration `0057_telemetry_ownership_observations` adds two nullable raw columns
and a guarded payload check. Both MariaDB 10.11.14 and MySQL 8.0.46 passed the
full 57-step chain, all nine native writer record kinds, identical/conflicting
replays and effective restricted writer permissions. The canonical descriptor
now has 153 column identities and 193 mappings; its 12 required indexes are unchanged.
All three retained histories, compiled constants and engine-measured fingerprints
are synchronized. The inventory remains 236 SQL tables and 51 non-database stores.

Each engine's mixed SQL fixture consumes 21 facts across seven pages, including
known, unavailable and new-producer ownership. Existing definition-1/2 amounts
retain their meaning; ownership advances the cursor without adding duration,
XP or account/controller metrics. All five existing observation publications,
restricted report roles, ambiguity/retry, budget refusal and atomic rollback
passed. Malformed ownership combinations fail SQL checks, and changed column
widths or check expressions fail the new verifier. Kind 9 requires a full current
connection producer and rejects inactive union fields or contradictory UTC labels.

Native session fixtures preserve actual boundaries through backpressure and bounded
overflow, require fresh recovery anchors, and retain facts after logout. Admission
retries receive fresh ordered keys; admitted SQL retries retain immutable keys.
The actual gameplay adapter passes scope/membership/deletion-fence negatives,
cyclic-list bounds, known/unavailable changes, reconnect and a fresh copyover
producer. The real transport regression admits newer keys before retrying retained
ownership and verifies the original observation clocks. ASan/UBSan passed.

Durable outage and offline export now preserve ownership family masks. Native
outage/runtime restart and storage-failure checks, header contracts, health checks,
runtime lifecycle/copyover, 13 observation and 12 review semantic tests, seven
budget tests, 24 immutable-history tests, 10 boot contracts and 22 lifecycle checks
passed. The maintained SQL server build and touched formatting passed.

Reviewed incident publication version 1 retains its sealed families 1–8 contract.
Identity publication must integrate kind-9 incident/loss coverage and the reviewed
association version before making identity-dependent coverage claims. Identity
handoff is qualified below. Portfolios, shared battles, zone attempts, canonical rewards, four
balance suites and the final real personal-local gate remain required. The complete
accepted scope is unchanged; production and staging access remain unnecessary.

## Qualified reviewed identity registration and generation reservations

The final fixture passed on MySQL 8.0.46 and MariaDB 10.11.14 through all 58
immutable migrations, ending at `0058_telemetry_identity_review`. It authenticates
the actual SQL principal against enabled owner-provisioned scope/reviewer authority
through the maintained registrar. Scoped native account tokens are verified using
column-only access and a retained foreign key, without current account names or
private lifetime bindings. The restricted storage credential remains trusted to
use the registrar; table grants do not independently enforce row-level scope or
the packet validator. See the exact boundary in [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md).

The actual SQL journeys qualified absent/disabled/wrong-principal/wrong-token
reviewers, unissued/wrong-season tokens, future review dates, dated corrections,
withdrawal, historical exact retries, unknown and conflicting reservations,
corrupted retained evidence, rollback and lost commit acknowledgements. Full
1,024-association capacity and simultaneous retries passed. CLI registration and
reservation executed through dedicated credentials with private error output.
Report metadata reads release their transactions instead of retaining schema locks;
the fixture asserts the real connection's transaction status. Review, rollup,
report and writer permission negatives passed, including refusal to read account
lifetime/name mappings through the review credential.

Unsigned widths, SQL enum/time/sentinel/digest boundaries, nullable unknown
reservations, guarded DDL reruns and exact verifier drift passed. MySQL and MariaDB
use their equivalent byte-length function spellings, and binary zero-digest
comparisons avoid connection-collation dependence. The signed UTC sentinel check
uses a symbolic field conversion that remains stable under MySQL schema alteration.
After restoring deliberate constraint drift, the full metadata fingerprint equals
the fresh migrated schema on each engine. The measured final fingerprints are:

- MySQL 8.0.46: `0ad566dfdf29fc6d3042a0ccb1879587851e9500e70219d3653a39c79af0b81f`.
- MariaDB 10.11.14: `5105c935e88237149f55d786378dfbf94116d90090673847b2a1861c33f0b4cd`.

All three migration histories, compiled boot constants, exact metadata queries
and protected lifecycle inventory are synchronized. The inventory covers 240 SQL
tables, 51 non-database stores and 42 Redis surfaces; destructive rules remain
disabled. Earlier sealed migrations 0054–0057 remain unchanged. Twenty-seven
identity, 13 incident, 13 observation, 14 report and seven rollup-budget checks
passed, together with 24 immutable-history, boot preflight and 22 lifecycle checks.
The existing incident SQL publication fixture also passed through migration 58
on both engines. Qualification used explicit disposable task-owned databases.
The final maintained SQL server build and touched formatting passed. Its first
link attempt exhausted local memory; stopping the completed task-owned database
fixture freed memory and the same build passed without changing compiler/linker
options or server code.

A generation reservation pins reviewed metadata or explicit unknown identity.
It reports that a balance report has not been published and that complete identity
coverage is not implied. Identity wire handoff and kind-9 incident coverage are
qualified below. Atomic effort/portfolio publication must consume those facts
and the reservation. Shared battles, zone objectives and canonical
rewards, all four balance suites and the final real personal-local gate remain
required. This increment preserves the complete accepted scope and the single
completion tracker; production and staging access remain unnecessary.

## Qualified copyover account context and ownership-loss review

The copyover wire now retains the last observed scoped account context in 24
additional bytes per handoff. Outer version 18 writes telemetry-v2 framing;
versions 15–17 read the exact sealed telemetry-v1 width and import absent ownership
context. Session identity, cumulative counters, checkpoint revision and source
quality survive those legacy reads. Version/header mismatch or malformed metadata
cannot silently reinterpret a following world section. Existing absent/undurable/
allocation-pressure recovery remains bounded.

The native handoff retains known, absent, unavailable and overflow context.
Overflow without a fresh ownership anchor stays unavailable with explicit
cardinality/drop/sequence quality; pending raw ownership still gates handoff.
Resume stores historical context without seeding it as a new-process ownership
observation. The actual gameplay adapter qualifies reloaded token changes,
missing tokens, wrong scope and deletion fences while preserving accepted
gameplay and fresh-producer ownership facts. The 16 actual wire scenarios,
SQL/client-free state and golden harnesses, standalone header contracts and
ASan/UBSan gameplay/capacity/transport fixtures passed. Existing shopkeeper and
death-retry/copyover-save contracts passed.

Migration 0059 adds two independent incident-schema-v2 input tables for families
1–9. Sealed migrations 0054–0058 and the v1 incident history remain unchanged.
The shared validator/registrar/snapshot/read code selects only validated schema
constants and keeps schema identity in the packet digest. Stored binary digests
require exactly 32 bytes before conversion. The 64-incident and 128-KiB review
bounds remain; unknown tails, separate reconstruction, committed-reference
validation, append-only corrections and explicit withdrawals retain their meaning.

Both MariaDB 10.11.14 and MySQL 8.0.46 passed the full 59-step migration chain and
actual dedicated-role CLI registration. Tests exercised kind-9 committed keys and
kind/scope/missing-reference refusals, insert rollback, old exact retries, retained
corrections, a real committed write with a lost acknowledgement and socket drop,
64 incidents, narrow byte-budget refusal, withdrawal, role and SQL CHECK negatives,
guarded reruns, altered-family-constraint refusal, and the same whole-schema
fingerprint before and after restoring deliberate drift.

The existing incident snapshot tables atomically retain v2 metadata and details
without widening the report role into either private review history or raw facts.
This proof executed the common snapshot/read seam at definition-3 scope inside
the bounded transaction. At this qualification point, definition 3 was disabled
in the public rollup/report catalog. That incident-only proof did not constitute
atomic account/controller effort publication; migration 0060 supplies it below.
The three histories, compiled runtime metadata and lifecycle inventory now target
head 59 and 242 database tables, with both measured engine fingerprints sealed.

The measured fingerprints are
`32fd376d0359d8acca79f6462b8bc3ab74a72f83e9b7265ea347ed30d4e1a043`
for MySQL 8.0.46 and
`f699a5487279e561b8d1705b76260bdcea5a57cc1eb820de1aa871d3e1285c1e`
for MariaDB 10.11.14. The maintained `make -C src -j2` build passed with these
compiled constants. Changed C/C++ formatting, ten boot compatibility tests,
boot schema preflight, 24 immutable-history tests and 22 lifecycle tests passed.
The focused Python checks passed: 27 identity, 16 incident, 14 report and seven
rollup-budget tests. Qualification used disposable task-owned local databases.

The identity publisher is qualified below. The real personal-local gameplay/save/
readback gate and the other complete balance requirements remain in issue #258.
No production or staging access is required.

## Qualified atomic identity effort and observed XP portfolios

Definition 3 retains selected activity, progression and authenticated ownership
facts in the existing cursor transaction. The ordered count and rolling digest
bind those facts to its processed watermark. Publication verifies that retained
source, consumes the reserved exact identity review and incident-schema-v2
snapshot, writes both reports and seals the coverage header in the same commit.
Published source windows freeze; corrections or later input require a new
generation. Explicit reads preserve superseded generations and their reviews.

Both MySQL 8.0.46 and MariaDB 10.11.14 passed the full 60-step immutable chain
through `0060_telemetry_identity_publication`. Actual dedicated-role CLI journeys
qualified ownership transfers, revised reviews, preserved old generations,
explicit unknown identity, known/unknown ownership-loss windows, complete source
digest checks, refusal before an unreserved buffering fetch, and read-only/private
role boundaries. Later-family publication failure rolls back outputs, incident
snapshot, header and published state; source-page failure rolls back retained
inputs, header and cursor. Real committed source and publication writes with
lost replies reconcile without duplication. Invalid SQL CHECK combinations fail.
Guarded DDL reruns preserve inputs, the exact verifier refuses changed family
constraints, and restoring drift returns the fresh whole-schema fingerprint.

The same real mixed raw window also qualified definitions 1 and 2: their observed
activity and XP amounts retain their earlier meanings, and no identity metadata
is added. The five observation reports remain definition 2; definition 3 offers
the new identity reports and established session/cohort activity reports. The
separate administrator `report.py` catalog remains definition 1.

Fifteen focused publication tests and the existing 27 identity tests passed,
including six overlapping characters yielding six character effort units and
one confirmed-controller union, transfers, review/day cuts, separate source and
status XP cells, unknown clock/identity, threshold exclusion, capacity, replay,
and conflicting clocks that cannot close a loss window. Existing incident,
observation, report, rollup-engine and budget checks passed. These tests establish
observations and atomicity; canonical economics, causal rotation effects,
milestones and complete controller populations are not inferred.

The measured fingerprints are:

- MySQL 8.0.46: `df6b5bac2c6fd755651fd1be0e3f0c34f0e567fb679c5cdc9a1a7fbc453b10c6`.
- MariaDB 10.11.14: `3f0b33ab825cca6506fbc4b7ea73b19e47bbe12e7ed7da71dcb2710a5abff221`.

All three retained histories and the compiled runtime contract target head 60.
The protected inventory covers 246 database tables and retains both coverage
foreign-key dependencies; destructive policies remain disabled. Sealed migrations
0054–0059 are unchanged. The focused runner is
`bash tests/async/run_telemetry_repository_sql.sh --identity-publication` with
either supported disposable engine image; exact commands and permissions are in
[IDENTITY_PUBLICATION.md](IDENTITY_PUBLICATION.md). The maintained
`make -C src -j2` server build, changed C/C++ formatting, runtime compatibility
validation, nine telemetry preflight checks, 24 immutable-history checks,
ten boot compatibility checks and 22 lifecycle checks passed with the final
head and fingerprints.

The complete accepted expansion remains active. Shared battles and changing
rosters/context/support, PvE objectives and canonical rewards, rested/assistance/
milestones/switching, four balance suites and study exports, #487 compatibility,
and the final real personal-local gameplay/persistence/performance qualification
remain required in the single issue #258 expectation. Production and staging
access are unnecessary.

## Qualified group lifetimes and pure shared association

Formal group generations now survive actual appointment and leader departure.
Both pool allocation sites clear earlier metadata; recreation allocates another
generation within the current producer. Accepted combat entry observes actual
PCs on both sides, including an attacked PC when the source is an NPC, while
pet ownership supplies PvP context without manufacturing PC participants. Every
encounter participant adapter checks the NPC flag before reading PC union data.
The existing group hook's initial source scan is bounded even for an NPC-only
cycle. Native mutation/adapter fixtures qualified these behaviors, callback
ordering and owner/player denominators.

The new pure `telemetry_battle` module is built into the maintained server and
qualified independently of its pending runtime/storage adapters. Its fixed
state is 1,719,392 bytes; a fact is 392 bytes and fits the 512-byte envelope with
the current header. It qualifies hostile/support/proven-presence association,
changing roster/context/mode, unique NPC generations and ownership changes,
conservative sides, retained aliases, measured presence/contribution/owner
outnumbering, and censored closure. Complete packet and duration conservation,
allocation traps, exact/oversized merges, queue loss, clock discontinuity,
inactivity bounds, inline-expiry accounting and row/actor/slot/sequence limits
passed normal and AddressSanitizer/UndefinedBehaviorSanitizer execution.

The actual group/combat mutation fixtures, gameplay adapter sanitizer executable,
existing encounter/combat summary/gameplay hook contracts, standalone/golden
record contracts, and transport normal/ASan/UBSan regressions passed. Changed
C/C++ and all new source files passed repository formatting. The final maintained
`make -C src -j1` server build passed with the new module registered. ThreadSanitizer
remains unsupported on this host. No new schema or report definition is included
in this increment; migration head 60 and existing persistent kinds 1–9 remain.

Shared native capture wiring, reviewed support/control/prevention and
population producers, durable shared facts/contribution linkage, qualified
publication and the final real personal-local gate remain required. Enabling
telemetry currently emits the improved existing encounter observations; it does
not emit the pure module's shared facts. [BATTLES.md](BATTLES.md) records the next
integration gate and executable commands. This is progress on the complete
accepted expansion, whose seven completion requirements remain open in #258.


## Qualified native battle source values

The native actor adapter reuses the maintained runtime-ID allocator instead of
adding a second identity registry. NPCs with the same prototype and reused
addresses receive distinct values when their runtime lifetimes change; pet
ownership changes preserve the live actor. Zero/high-half lifetimes and invalid
current PC owners are refused. The actual native group hook advances one roster
revision before the post-mutation callbacks. Accepted appointment/departure
retain the generation, refused operations supply no mutation callback, both pool
allocation sites clear revision metadata, exhaustion remains unknown and a new
observing producer allocates a fresh lifetime.

Native party presence requires one exact bounded formal roster, distinct actors
and a known shared room/zone/revision. A different room in the same zone fails;
a cyclic list supplies unknown/cardinality quality. Optional PC session links
must match admitted, unclosed state, including linkdead and a retained logical
copyover session. Active current-producer encounters can be linked; closed or
inactive history and forged native tuples cannot. Pets inherit no owner's PC
session or encounter. Snapshot failure clears the returned values.

The actual native adapters and maintained runtime-ID allocator passed normal
and AddressSanitizer/UndefinedBehaviorSanitizer fixtures. Native presence values
fed into the pure association engine retained presence-only effort separately
from contribution and authentication. Ownership-loss context retained the same
actor and conserved the preceding PvP/following PvE segments. Actual extracted
group mutation fixtures qualified exactly one revision callback before roster/
context notification, rejected operations and leader-departure metadata transfer.
Active encounter lookup, existing combat boundaries, runtime lifecycle/copyover
integration, standalone record/golden contracts and pure battle sanitizers passed.
Touched C/C++ formatting and the final maintained `make -C src -j1` build passed.

These are native value adapters, not an activated persistent shared collector.
Migration head 60, kinds 1–9 and earlier published definitions retain their
meanings. This increment introduces no SQL schema change. Its durable/native
activation gate remains registering the adapters at reviewed hostile/support/
control/prevention/context/leave/lifecycle boundaries, preserving attribution
cuts and uncertainty, explicit shared contribution linkage, additive writer/
schema/permissions/incident/lifecycle contracts and bounded atomic publication.
The full #258 checklist and real personal-local gameplay/persistence/performance
qualification remain required; no production or staging access is needed.

## Qualified portable shared-battle fact and packet contract

The definition-1 actor/battle value types are now independent of the pure
association state. Intrinsic validators qualify producer/lifetime identity,
optional session/encounter references, formal revisions, conserved effort,
cardinality, clocks, loss and censored terminal boundaries.
`telemetry_battle_fields.inc` declares 70 canonical numeric fields, encoded in
366 bytes independently of C++ padding. Native/Python layouts and a retained
semantic digest agree. Actor reserved bytes cannot be serialized as valid facts.

Normal packets require their final cut; terminal packets require unique complete
rosters and consistent active/owner counts. Scope, clocks, revision, counts and
contiguous fact sequences agree across frames. Loss quality may increase during
a packet but cannot disappear. The fixed native receiver retains at most 65 facts
in less than 32 KiB, accepts reordered/identical retries and latches conflicting
logical facts. Missing frames remain pending, and another packet cannot silently
replace them. Python receivers preserve the same boundaries and copy their
accepted/returned values.

Ten cross-language contract regressions passed using actual native battle
journeys, including all 65 terminal ordinals, callback loss, immutable encoding,
negative clocks/PIDs, changing pet ownership, alias facts, width/type/length
refusal, malformed packets and replay conflicts. The pure battle executable and
native gameplay adapter executable passed AddressSanitizer/UndefinedBehaviorSanitizer.
Standalone C++20 public headers and the existing 40-record/9-configuration/
3-transition golden bridge passed.
The maintained SQL server built with `make -C src -j1`, including the new
contract object. Runtime lifecycle/copyover integration passed both SQL-header
and client-free variants. All touched C/C++ and the canonical descriptor passed
the repository formatter; the codec also asserts each field's actual native
width and signedness at compile time.

```sh
python3 tests/async/test_telemetry_battle_contract.py
python3 tests/async/test_telemetry_battles.py --sanitize
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
python3 tests/async/test_telemetry_contract_headers.py
python3 tests/async/test_telemetry_runtime_integration.py
make -C src
```

This portable-contract increment prepared the durable integration qualified
below. Packet completeness does not repair cross-packet source gaps or prove
ownership, outcome or alias-history coverage. All seven full completion
requirements in #258 remain open, including the four balance suites and real
personal-local gameplay/persistence/performance qualification.

## Qualified durable shared-battle facts and independent loss review

Migration `0061_telemetry_shared_battle_facts` preserves sealed migrations 1–60
and the earlier report definitions. It adds all 70 canonical numeric kind-10
columns and their separate logical unique key, exact family/header binding,
immutable capture-configuration qualification and independent incident schema
v3 histories. The retained lifecycle inventory has 248 SQL tables and 51
non-database stores; destructive rules remain disabled.

Both full 61-step local chains passed on MariaDB 10.11.14 and MySQL 8.0.46. The
native repository generates actual pure-battle packets with hostile PCs, support,
a pet, group/zone context change and copyover closure, verifies every typed
field, then exercises a real committed write with an injected lost reply and
exact replay. Different transport receipts for one logical fact conflict.
Producer/time/configuration mismatches, reserved pending-value substitution and
record-specific SQL quarantine/retry evidence are qualified. Startup schema and
effective permission negatives cover all ten native record families.

The v3 review journey qualifies the actual dedicated CLI, committed battle scope
references, atomic parent/detail rollback, exact retries, corrections, lost
acknowledgements, immutable old snapshots, unknown/reconstructed tails, 64-row
capacity, byte reservation and writer/reviewer/publisher/report role separation.
Original v1/v2 review histories retain their own limits. Guarded SQL reruns retain
facts/reviews and never repair altered defaults or widened checks. After explicit
restoration the full schema fingerprint equals the fresh chain on each engine:

- MySQL 8.0.46: `0ef431d622156d6e81c71879e338ea25c727b738829047ea0f89101ed5975a91`.
- MariaDB 10.11.14: `4450fab94c8e0ae9a9e90817849d6a4a6ffca59443f3f59bb985c18f95b51b6b`.

Outage v2 (`DMSTLJ02`) includes family 10 within the unchanged 256-producer/
81,984-byte bounds. Native/offline readers preserve original v1 bytes and family
limits; atomic worker upgrade retains old observations. The full lifecycle,
publication fault, SIGKILL/exec, quota and actual worker outage suites passed.
Transport normal/AddressSanitizer/UndefinedBehaviorSanitizer qualification passed;
ThreadSanitizer remains unrun because this host's runtime probe cannot initialize.

Twelve native/Python battle contract tests passed, including complete packets,
loss, all 65 terminal ordinals, exact raw family/header binding, unchanged
definition-1/2/3 contributions and malformed/over-budget refusal before cursor
acknowledgement. All three definitions also processed/published the real SQL
mixed stream, with an explicit unknown identity reservation for definition 3.
The frozen 40-record/9-configuration/3-transition bridge and ten golden fixtures
retain their original expectations. Runtime/lifecycle/immutable manifest
validation is synchronized with the measured fingerprints and head 61.
All touched C/C++ passed the repository formatter, and the maintained server
built successfully with `make -C src -j1` against the current schema contract.

```sh
python3 tests/async/test_telemetry_battle_contract.py
python3 tests/async/test_telemetry_incidents.py
python3 tests/async/test_telemetry_outage.py
python3 tests/async/test_telemetry_runtime_outage.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --battle-storage
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --battle-storage
make -C src
```

The runtime capture qualification below connects the battle callbacks. Compact
context/control/prevention/faction sources, exact shared contribution linkage
and bounded atomic projections remain required. This layer alone publishes no
new balance suite. The complete identity, PvE
attempt/reward, progression/rested/milestone/switching, four-suite/statistical
export, #487 compatibility and actual personal-local gameplay/persistence/
performance requirements remain on #258's full checklist. There is no production
or staging access dependency.

## Qualified runtime shared-battle association capture

Native hostile observations and positive effective healing support now feed the
bounded shared-battle state through the existing queue and worker. Actual formal
party presence remains separate from contribution and authenticated activity;
a repeated hostile relationship can establish an arriving member's presence.
Owned pets retain unique live identities without adding an owner's session or
duplicating their owner-character denominator. Context and roster changes cut
known state; accepted room removal, death/flee, extraction and session exit end
observed presence. Session exit uses one actual source clock for both its session
and battle boundaries. Copyover/shutdown remain censored closures.

Admitted configuration changes preserve battle identity, actor roster, measured
effort and the hostile-activity clock. Bounded actor-pair checkpoints seal the
preceding context. Effective-property withdrawal preserves the known prefix,
suspends relationships/context and records unknown-mode effort until qualified
recovery; actor teardown still operates during the gap. Clock/revision refusal,
callback loss and mutation-capacity exhaustion cannot repair historical coverage.
The fixed state remains 1,719,392 bytes, with the same 128-battle, 64-actor and
4,096-mutation-fact bounds.

The actual runtime/queue/worker/native-writer fixture passed on both disposable
full-61-step MariaDB 10.11.14 and MySQL 8.0.46 chains: 80 battle facts in 27 complete
packets per engine, all 70 canonical values equal to SQL, correct absent-family
NULLs and configuration qualification, empty quarantine and restricted writer
denials. Temporary fixture databases and writer roles were removed. The fixture
uses synthetic gameplay objects and a private connection-factory seam; it starts
no real game server and leaves account identity unknown.

Native gameplay and pure battle AddressSanitizer/UndefinedBehaviorSanitizer
journeys passed, along with the 12 native/Python contract regressions. Executable
accepted/refused room removal and group/member-removal journeys passed; native
configuration withdrawal/recovery, copyover/exhaustion and durable outage fixtures
passed in both client-free and SQL-header builds. Existing encounter/combat and
gameplay source contracts, standalone public headers and frozen golden fixtures
passed. The maintained server built with `make -C src -j1`; changed C/C++ lines
passed the repository formatter. ThreadSanitizer remains unrun because its
previous host capability probe could not initialize.

```sh
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
python3 tests/async/test_telemetry_battles.py --sanitize
python3 tests/async/test_telemetry_battle_contract.py
python3 tests/async/test_telemetry_room_hooks.py
python3 tests/async/test_member_removal_runtime.py
python3 tests/async/test_telemetry_runtime_integration.py
python3 tests/async/test_telemetry_runtime_exhaustion.py
python3 tests/async/test_telemetry_runtime_outage.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
make -C src
```

This completes association capture and its source/configuration boundaries, not
the complete battle balance requirement. Compact power/build/arena context,
actual control/prevention producer coverage, faction exposure, exact shared
contribution linkage and atomic battle/report publication remain required.
All seven accepted checklist items retain their remaining identity source/suite,
PvE attempt/objective/reward, progression/rested/assistance/milestone/switching,
four-suite/statistical, #487 compatibility and actual personal-local
gameplay/persistence/performance requirements. Technical acceptance requires no
production or staging access.

## Qualified disjoint shared-battle contribution contract

The pure contribution accumulator seals amounts under their exact effective
battle/live actor/owner, native context, mode/side and configuration. First/last
association references are retained; a changed context requires an advancing
reference. The process-wide segment sequence supplies a separate logical replay
key. Battle aliases never copy accumulated totals into another segment. Paired
source/recipient events preflight capacity before either counter changes, and
release both changed streams before creating replacements.

Ten executable journeys passed normal and AddressSanitizer/UndefinedBehaviorSanitizer
qualification. They cover source/recipient conservation, actual complete pure
association packets/merges, PvP-to-mixed mode/configuration cuts, pets changing
owners/kinds, distinct live NPC identities, casting completion/abort/unresolved
partitions, opponent-link duration and censored observed prefixes, stale clocks/
references, 64-actor and 128-battle atomic capacity, sequence exhaustion,
saturation, source gaps and sink refusal. Allocation traps cover every exercised
event and codec path. The separate fixed contribution state measures 3,674,176
bytes; a payload measures 400 bytes, and its 65 canonical fields encode in 385
bytes. Compile-time guards retain a 4 MiB module budget and the existing 512-byte
payload/header bound.

Nine independent native/Python contract regressions passed exact source-row
round trips, immutable layout, domain keys, all declared numeric boundaries,
strict types/widths/lengths and semantic corruption. Unknown producer families
remain unknown rather than measured zero. Unknown UTC retains `INT64_MIN`;
negative/zero epoch labels remain real labels. An interrupted cast remains
unresolved rather than an invented abort. Opponent-link time cannot be
relabeled as prevention or tanking. A close boundary supplies no victory.
Intermediate UTC reversal/unknown observations retain uncertainty even when
their final labels recover.

The existing 12 battle packet regressions and pure association ASan/UBSan
journeys passed. Legacy combat reconciliation, standalone headers and the frozen
40-record/9-configuration/3-transition bridge passed. The maintained server
built with `make -C src -j1`; new C/C++ code follows the repository formatter.

```sh
python3 tests/async/test_telemetry_battle_contributions.py
python3 tests/async/test_telemetry_battle_contributions.py --sanitize
python3 tests/async/test_telemetry_battle_contribution_contract.py
python3 tests/async/test_telemetry_contract_headers.py
make -C src
```

The accumulator is now owned and called by the runtime; its native connection
is described below. The kind-11 durable family preserves earlier record meanings
and report definitions 1/2/3. Complete published association/alias/loss coverage
and atomic battle publication remain required. The pure qualification in this
section establishes the collector contract; the native fixture establishes the
specific connected callbacks. Actual control/prevention sources and running
personal-server/performance coverage remain required.
All seven accepted identity/source-suite, full battle/context/population,
PvE attempt/objective/interruption/reward, progression/rested/assistance/
milestone/switching/comparable portfolio, four-suite/statistical, #487
compatibility and personal-local gameplay/persistence/performance requirements
retain their incomplete portions under #258. No production or staging access
is needed for technical completion.

## Qualified durable battle contribution family

Record kind 11 now admits the sealed contribution payload through the canonical
writer. All 65 fields retain their declared SQL widths and signedness; the
complete tagged record remains within 512 bytes. The transport receipt is bound
to the battle producer and the decision UTC label. Its domain key is the
independent process-wide segment sequence, so a battle or actor alias cannot
replay a contribution under a second receipt. Exact retries, conflicting
receipts and uncertain pending representations retain the existing repository
semantics. Reserved bytes are included in quarantine identity; unrelated union
bytes and ABI padding are excluded. Configuration scope and captured semantic
versions are qualified against the immutable configuration projection.

The additive `0062_telemetry_battle_contributions` migration requires every
kind-11 field and NULL columns for other families. Direct SQL checks retain
native actor/context/association identity, metric availability, clock ordering,
explicit uncertainty and healing/casting partitions, including unsigned 64-bit
saturation boundaries. Earlier migrations and their manifest entries remain
byte-identical. All three current migration histories, protected runtime
inventory, boot constants and exact fresh/restored engine fingerprints agree at
head 62 with 250 database tables and 51 non-database stores.

Both disposable MariaDB 10.11.14 and MySQL 8.0.46 full-chain qualifications passed.
The native writer generated actual disjoint damage, healing, control, casting
and engagement segments with changed zone/association context. It verified all
65 fields, source totals, unresolved casting, unknown/signed clocks,
NULL-family separation, exact/conflicting domain and transport retries, a lost
commit acknowledgement, configuration/header refusals, protected pending
values and exact quarantine evidence. All eleven native record families and
the frozen golden fixtures retain their behavior.

The separate storage fixture also passed actual emitted pet/NPC, saturated
counter, clock-uncertainty and source-gap rows, declared unavailable metrics,
direct constraint refusals, private role negatives, the maintained incident
registration CLI, retained corrections, lost reply recovery, atomic review
rollback and immutable coverage snapshot retries. Guarded reruns preserve raw
facts and reviews; altered column defaults and widened family checks are
refused until explicitly restored. The restored metadata fingerprint equals
the freshly migrated schema on each engine. New mixed-stream tests preserve
definitions 1/2/3 amounts while strictly validating kind 11 and advancing their
input cursor.

Outage wire v3 includes families 1–11 without changing its 81,984-byte bound.
Native and offline readers preserve v1/v2 bytes and enforce each earlier
version's original family limit for both observed/failure masks. Private
incident review schema v4 has an independent retained history for contribution
loss. The existing snapshot seam selects v4 for future definitions beginning
with 5, keeps v3 for definition 4 and preserves older selections. This seam
does not add a definition to the current report catalog or publish a battle
suite.

```sh
python3 tests/async/test_telemetry_battle_contribution_contract.py
python3 tests/async/test_telemetry_battle_contributions.py --sanitize
python3 tests/async/test_telemetry_transport.py
python3 tests/async/test_telemetry_outage.py
python3 tests/async/test_telemetry_incidents.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --contribution-storage
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --contribution-storage
python3 scripts/validate_runtime_compatibility.py
python3 scripts/validate_data_lifecycle.py
make -C src
```

The new queue journey passed normal, ASan and UBSan execution and retained the
admitted immutable kind-11 payload through control-reserve admission and worker
delivery. ThreadSanitizer could not start its trivial host capability probe;
the harness was not run under TSAN and no TSAN pass is claimed.

The native contribution connection described below retains exact complete-packet
references and disjoint boundary amounts. Complete published association/alias/
loss linkage remains required. Full compact context/control/
prevention/faction-population sources, typed death/escape/objective evidence,
bounded atomic battle publication and actual personal-server source journeys
remain open. All seven accepted identity/source-suite, PvE attempt/objective/
interruption/reward, progression/rested/assistance/milestone/switching/comparable
portfolio, four-suite/statistical, #487 compatibility and personal-local
gameplay/persistence/performance requirements retain their incomplete portions
under #258. Technical completion requires no production or staging access.

## Qualified native disjoint contribution capture

The runtime now owns the bounded contribution accumulator and connects native
damage, healing, casting and actual opponent-link observations to its kind-11
writer path. Each combat observation uses one clock across its hostile/support
relationship, formal party presence and metric update. The contribution context
comes from the committed shared battle state, with its actual actor/owner,
current scope, group revision, mode/side and final complete-packet reference.
The native availability mask is 27: damage, healing, casting and engagement.
Control remains unavailable until a real native effect producer is connected.
An availability bit alone does not prove complete lifecycle coverage, and an
absent row cannot establish a measured zero.

Association mutations seal affected retained streams before further activity.
Battle aliases, mode/side, ownership/kind, group revision, dimensions and
configuration changes retain old disjoint amounts under their original identity.
Teardown uses the retained pet kind even if current owner context is missing.
Session exit and battle leave share one exact clock. Useful support requires
positive effective healing; ineffective attempts count only inside an existing
shared battle. Formal presence supplies no metric stream. Pending casts become
unresolved at a context boundary and later terminal callbacks cannot copy their
attempts or elapsed time into a replacement.

The battle close callback seals contributions before the association slot is
released. Inactivity preserves the actual observed prefix with a later decision;
copyover/shutdown retain their observed source cut. Casting/opponent observations
extend an active prefix without refreshing hostility or emitting duplicate
packets. Inactive actors and unrelated battles cannot extend that prefix.
Configuration withdrawal seals measured source-gap prefixes. Recovery keeps
partial association coverage while admitting the newly observed amounts.
An actual opponent pointer without a usable native identity seals a source gap
and preserves uncertainty instead of reporting zero engagement.
Capture/admission/capacity/clock loss remains explicit.

Both full 62-step disposable SQL chains passed the actual native runtime, queue,
worker and canonical writer journey: **123 kind-10 facts in 38 complete packets
and 28 kind-11 segments per engine**. Every one of the 65 contribution fields
matched its emitted source, including both first/last references to complete
association packets, configuration versions, actor lifetimes, clocks and
availability. The source retained 112 damage dealt and 112 damage taken,
45 attempted healing partitioned into 15 effective and 30 overhealing, and
six casting attempts partitioned into one completion, one abort and four
unresolved attempts. Private writer negatives, absent-family NULLs and an empty
quarantine passed. The MySQL 8.0.46 fingerprint remains
`3a979c1d7b66f57e820db1087ade7f84bdbf2dc6b4ac988280aa3360eda8ed68`;
MariaDB 10.11.14 remains
`90a2741fc2425a0d2e06809fdbaa3ebd8f098325d4e6805b82b2cb14b6fdc520`.
Temporary databases/roles were removed and both fixture engines were stopped.

The gameplay journey checks party arrival/departure, two-sided metrics, pet
ownership/kind and retained teardown, distinct NPC generations, configuration
cuts, an alias, terminal casting retries and exact session exit. A separate
native journey supplies a future pulse clock for two independent inactivity
closures and verifies their different measured prefixes at one later decision.
It is a controlled fixture clock. A third native fixture validates the unavailable
opponent source-gap segment through actual SQL, preserving known damage and an
unresolved cast. Each actual copyover flush keeps its 250 ms
cap; the SQL correctness fixture allows bounded generation retries within five
seconds and verifies conserved amounts and one censored close. Neither fixture
establishes a running personal-server performance budget.

Normal and ASan/UBSan gameplay and pure battle checks passed. Both runtime
variants passed configuration withdrawal/recovery, copyover, sequence exhaustion
and durable outage checks. The twelve battle and nine contribution cross-language
contracts, legacy combat/encounter checks, standalone headers and frozen
40-record/9-configuration/3-transition bridge passed. The maintained server built
with `make -C src -j1`; touched C/C++ passed the repository formatter. The unchanged
schema remains at head 62 with fourteen qualified writer indexes. The earlier
TSAN limitation remains: its host capability probe could not initialize, so no
TSAN harness pass is claimed.

Reproducible focused commands are in [BATTLES.md](BATTLES.md#native-contribution-capture).
Full native compact context/control/prevention/faction exposure and typed death/
escape/objective evidence, complete published association/alias/loss coverage
and bounded atomic battle publication remain required. All seven accepted
identity/source-suite, zone attempt/objective/interruption/reward, progression/
rested/assistance/milestone/switching/comparable portfolio, four-suite/statistical,
#487 compatibility and actual personal-local gameplay/persistence/performance
requirements retain their incomplete portions under #258. The report catalog
remains definitions 1/2/3, and this connection publishes no automatic balance
change. Technical completion requires no production or staging access.

## Qualified bounded battle history and linkage

The history reducer now checks the native facts as a bounded, ordered history,
using the sealed kind-10/11 validators. Initial/whole/middle/terminal packet loss,
gaps, conflicting receipts, source-quality regression, false alias bridges,
effective-context changes and unproven lifecycle cuts cannot become a verified
contribution link. Conservative graph replay verifies declared actor/owner counts
and observed sides/modes and reconciles absolute actor effort. Aliases preserve
their separate source identities and replace inherited absolute effort; their
measured segments contribute once to the canonical total. Later complete packets
cannot repair earlier missing history.

The source fixture retains **123 association facts in 38 complete packets, 28
contribution segments and five canonical battles**. Every contribution has a
verified effective-context/lifecycle link. Normal and ASan/UBSan native exports
and the actual private SQL writer/readback journeys passed. Both full 62-step
MySQL 8.0.46/MariaDB 10.11.14 chains retain their exact sealed fingerprints,
empty quarantine, absent-family NULLs and writer permission denials. Temporary
databases/roles were removed and both engines stopped. Twenty history regressions
also consume 6,468 facts from 23 existing pure association journeys. The twelve
older cross-language packet contracts, maintained build and repository formatting
passed.

Unknown control metrics remain NULL. Reviewed schema-4 family/producer/sequence/
time loss and unknown UTC remain separate from measured amounts. The native
reducer fixture's 2,230,272-byte internal reservation is accounting for its
bounded value work; it does not measure buffering SQL, the Python heap or a game
server's performance. Actor outputs retain latest absolute cumulative effort,
whose latest context cannot attribute its whole history to one configuration,
class, faction or group. Copyover and future-pulse fixtures retain their prior
limits and establish no actual personal-server journey.

The next integration connects this reducer to an immutable retained source/cursor
digest and the bounded atomic generation transaction, adds dated identity and
independent incident snapshots, and publishes the required battle exposure/
contribution projections. The reducer currently writes no SQL and the catalog
remains 1/2/3. Remaining native context/control/prevention/faction and typed
death/escape/objective evidence, all other identity/zone/progression/suite/#487
requirements, and the one actual personal-local gameplay/persistence/performance
gate remain open under all seven accepted completion items. No production or
staging access is required.

## Qualified historical battle exposure and retained source values

Historical exposure now records the exact actor, role, mode, conservative side,
observed owner counts and captured configuration/context for each positive
observed interval. All eight exposure amounts reconcile to the native cumulative
actor values in the fully replay-verified fixture. Presence-only participants
retain presence without invented contribution streams. Aliases retain original
prefixes once; inactivity ends at the observed cut. Missing mutation evidence
retains earlier verified time and cannot extend it. Unknown clocks preserve
monotonic effort; unknown sides retain NULL owner denominators.

The observed active roster/relationship digest prevents composition changes from
being hidden in coalesced intervals. Even an opponent's class change splits the
unchanged actor's exposure. Adjacent intervals combine only with identical scope,
actor context, roles/sides, owner counts, roster and clock quality. Association-only
actor overlap refuses reduction, and explicit row/byte/deadline budgets remain.

The retained-source value contract preserves kinds 9/10/11, original ingestion
IDs/receipts, typed source values and separate occurrence/arrival labels. Exact
canonical payload SHA-256, a scoped rolling digest, family counts and the expected
generation cursor qualify the whole window. Missing/reordered/repeated receipts,
changed source, envelope/type/JSON faults, checkpoint/count/quality mismatches,
capacity/deadline failure and extension of a published checkpoint refuse results.
No caller source/header values are mutated. Export-only arrival is explicitly
unknown; it never borrows occurrence time.

**Thirty-nine focused tests passed**, including all original history regressions,
6,468 facts/23 pure association journeys and the exposure/source negatives.
Normal and a fresh ASan/UBSan gameplay export passed the same reducers. Both
actual native private-writer SQL readbacks preserve 123 kind-10 facts/38 packets,
28 kind-11 segments and every verified contribution link, with five canonical
battles and conserved 112/112 damage plus exact healing/casting partitions.
Restoring encoded SQL inputs reproduces every battle, actor, exposure and
contribution value using actual ingestion IDs and arrival labels. The standalone
export proof uses synthetic ingestion IDs because those values have no SQL receipt.
The SQL source-checkpoint contract is evaluated in memory; no persisted checkpoint
or atomic battle publication is claimed by this readback.

MySQL 8.0.46's final readback used an owned, auto-removed disposable fixture with
`mysqld --skip-innodb-use-native-aio` after the shared host kernel AIO quota
prevented the default fixture from starting. MariaDB 10.11.14 used its normal
disposable setup. Both full 62-step chains retain their sealed fingerprints;
temporary databases/roles were removed and both fixtures stopped. This fixture
setting proves functional SQL behavior, not default-engine or game-server
performance.

The 151-input source stage reserves 4,952,064 bytes internally. History/exposure
reservations vary with positive source clock cuts and coalescing. Neither figure
measures Python heap, SQL buffering or real gameplay overhead. Migration head 62,
sealed fingerprints, native wire/state and catalog definitions 1/2/3 remain
unchanged. No source/public table or migration is added by this increment.

The following work connects these qualified values to the existing bounded atomic
transaction: persist selected source with the cursor/count/digest, consume the
reserved dated identity review and independent schema-4 incident snapshot, publish
dated exposure/contribution/coverage stores and verify immutable generations,
rollback/lost acknowledgements and restricted report reads. Definition 5 names
the schema-4 contract seam but remains disabled in the catalog. All seven final
requirements retain their unfinished native, identity, zone, progression, suite,
#487 and actual personal-local gameplay/persistence/performance portions under
#258. Technical acceptance continues to require no production or staging access.

## Qualified persisted battle source preparation

Migration 0063 now retains the actual selected kind-9/10/11 window in private
tables. Its scoped unique producer receipt, exact canonical payload digest,
selected family counts and rolling digest qualify the original source window
against the locked generation cursor and immutable rebuild boundary. The version-2
digest seed binds the original boundary as well as the four-part scope; changing
the origin cannot relabel existing retained evidence as another verified window.
Restrictive foreign keys require both
state and immutable identity reservation. Explicit unknown identity is preserved
as preparation and assigns no account/controller effort.

Source inserts, count/digest/header updates and cursor advancement share the
existing bounded page transaction, advisory lock and fixed keyset/retry/deadline
path. Faults after source writes but before cursor update roll everything back.
Lost acknowledgements before and after commit reconcile without duplicate
receipts. Retained source survives raw retention. Consistent private reads
reserve encoded buffering, decoded values and a sentinel before selecting rows,
validate the exact state origin/cursor and whole source window, cap payload
selection
at one byte beyond its declared maximum, and release the transaction on every
outcome. General report/game-writer roles cannot read either source table; the
rollup role cannot update or delete retained inputs.

Forty-five focused history/exposure/source tests passed. Both full-63-step
MySQL 8.0.46 and MariaDB 10.11.14 native runtime/private-writer journeys passed
the same persistence checks: the actual selected window contains two ownership
observations, 123 association facts and 28 contribution segments (**153 inputs**).
Every original field and arrival label restores exactly and reproduces every
battle/history/actor/contribution/exposure value, with all 28 links verified and
all eight effort measures conserved. An additional matching synthetic ownership
observation is retained once (**154 inputs**); the foreign-scope observation
advances only the global cursor. These storage fixtures establish no actual
personal-server login or historical controller association.

Both engines qualified missing reservation, cursor rollback, exact retry,
lost acknowledgements, missing/changed payload and cursor/origin conflicts,
an explicit empty building window with a nonzero original boundary, receipt/
count/kind/foreign-key constraints, bounded reads, raw retention, maintained CLI
preparation/publication refusal, private-role denials, guarded migration reruns,
weakened payload/kind schema drift, read-transaction release and exact restored
metadata. Engine-measured fingerprints are sealed for 252 protected tables and
head 63 in all three migration histories and the compiled boot contract. Earlier
sealed migration bytes/entries retain their original meanings. MySQL used the
owned, auto-removed native-AIO workaround documented above; this remains SQL
correctness proof, not a measured default-engine or game-server performance gate.

At migration 0063's delivery, definition 5 accepted source preparation through the maintained `run` command.
The report catalog remains 1/2/3 and definition-5 publication explicitly refuses
until its battle projection exists. The next integration consumes the reserved
dated identity review and independent schema-4 incident snapshot, publishes dated
exposure/contribution/coverage stores atomically and qualifies immutable report
generations, rollback/lost acknowledgements and restricted public reads. All
seven completion requirements retain their unfinished native, identity, zone,
progression, suite, #487 and actual personal-local gameplay/persistence/performance
portions under #258. No production or staging access is needed for technical
acceptance.

## Atomic battle observation publication

Migration `0064_telemetry_battle_publication` completes the observation publication
transaction on the retained source/cursor foundation. Definition 5 now exposes
`battle_observations`, `battle_actors`, `battle_contributions`, `battle_exposure`
and `battle_associations` through the maintained `rollup.py definitions`, `run`,
`publish` and `report` commands. Earlier report versions retain their meanings.
The original association and contribution contracts remain sealed and unchanged.

The existing locked transaction reads the whole bounded source window and exact
reserved identity version, validates the independent schema-4 loss snapshot,
builds all projections, inserts public coverage/rows, marks source complete and
publishes/supersedes generations together. Source/cursor counts and digests,
canonical/alias lineage, original producer and SQL arrival labels, context and
complete contribution references remain exact. Missing/changed evidence cannot
become a verified result. Reports validate a bounded complete snapshot receipt
digest and row counts before selecting canonical typed payloads, retain consistent
state/identity/loss metadata, and release their read-only transaction on every
outcome. Restricted report accounts cannot read private ownership/source data.

Presence is divided only at observed ownership changes, reviewed identity
boundaries and comparable UTC midnight. All native presence/effort amounts are
conserved. Missing sessions, unproven pet owners, unknown UTC and ownership-loss
gaps retain explicit uncertainty. A reviewed loss end requires a fresh observed
matching anchor before ownership recovers. A correction changes only a new
generation. Contribution amounts are assigned only to uniform observed identity;
changing identity never divides the amount. A uniform account can survive a
controller-review change. Unavailable native control stays NULL. Actor snapshots,
segments and historical exposure are alternative projections; actor time is not
unique controller effort or continuous human attention.

**56 focused history/source/publication regressions passed**, preserving the
6,468-fact/23-journey association proof. Both disposable **MySQL 8.0.46** and
**MariaDB 10.11.14** passed the complete **64-step** migration chain and actual
runtime/queue/worker/private-writer journey. The original selected window contains
two ownership observations, **123 association facts in 38 complete packets** and
**28 contribution segments**. Every original association field and arrival label
publishes exactly, all 28 links remain verified, damage remains **112/112**, and
all measured effort partitions are conserved. The matching/foreign source cases
retain **154 selected inputs** while preserving the original source proof.

Both engines qualified row/output/byte refusal, unfinished fixed bounds,
rollback after an inserted public detail, before/after-commit lost acknowledgements,
exact retries, older superseded report/source readability, missing/changed public
rows and source-completion conflicts, limited/truncated reports, maintained CLI
publication and restricted report reads, public SQL constraints, guarded reruns,
weakened payload drift, capped reads, read-transaction release and exact restored
metadata. Separate coherent-UTC fixtures qualify registered issued tokens and
scoped reviewer authority, account transfers, immutable identity corrections,
independent schema-4 loss versions and recovery at a fresh ownership anchor.
Those fixtures preserve native amounts/context/references but deliberately replace
UTC labels; they do not establish real personal-server authentication history.

All three immutable histories, protected inventory and compiled boot contract
target head **64 / 254 tables**. Earlier migration bytes and entries are preserved.
Fresh and exactly restored engine fingerprints are:

| Engine | Normalized metadata fingerprint |
| --- | --- |
| MySQL 8.0.46 | `2fd0ca1c7b764521ee14db4cfaa577b22fc1beb5cb1adbc1b570170681dd1743` |
| MariaDB 10.11.14 | `eabbf995f7087832febf52bbd84a2fff56bf74d7bfcc38c28c45c22bb4fba840` |

The local MariaDB fixture was stopped and the owned MySQL fixture stopped and
removed. MySQL used the previously documented native-AIO workaround; this
qualifies SQL correctness and does not measure default-engine or game-server
performance. The current increment changes Python publication and boot/schema
metadata, with no native capture/wire semantics change. Existing 14 engine and
15 identity-publication regressions, 24 immutable migration tests and 22 lifecycle
tests passed; POSIX-dependent checks ran in the owned local test container. The
final ten boot/runtime contract tests, offline runtime validator, maintained
`make -C src -j1` build, touched C/C++ formatter checks and `git diff --check`
also passed with the sealed head-64 fingerprints.

The focused local commands remain in
[BATTLES.md](BATTLES.md#atomic-battle-observation-publication). These five reports
are observation primitives for the full accepted suites. Compact build/power/
arena context, native control/prevention/faction exposure, typed outcomes, real
authenticated source journeys, PvE attempts, progression context, the four
complete suites, #487 compatibility and the final actual personal-local gameplay/
persistence/performance gate remain required under #258. All seven completion
requirements retain their unfinished portions. Production and staging access
remain unnecessary for technical acceptance.

## Accepted blindness and stun control capture

The maintained `blind()` helper and all three successful `Stun()` branches now
feed the runtime's existing control adapter after effect mutation and before
combat teardown. The adapter records both actual sides through the bounded
kind-11 accumulator before checking legacy kind-8 IDs. Self effects use the
existing battle and SELF modifier and cannot start a hostile edge. Invalid
modifier bits are refused before association mutation; zero applications create
no native contribution stream. Existing gameplay immunity, saving throws,
already-active gates, duration calculation and legacy summary rules are preserved.

New native contribution segments declare producer availability mask **31**.
This identifies accepted `blind`/`Stun` observations, including measured segment
zeros when these hooks observed no application. It does not establish all effect
coverage, resistance or duration. Older mask-27 inputs still publish NULL control
counters, and `complete_metric_coverage_implied` remains false. Effect types,
attempts, rejection reasons, duration/overlap/removal and other direct affect
producers remain unfinished. No wire fields, versions, migrations, SQL shapes or
report definitions change.

Normal and fresh **ASan/UBSan gameplay adapter journeys passed**. The fixture
executes the maintained blindness/stun/clamp source bodies with isolated affect,
save, random, message and teardown services, linked to the real runtime/queue/
worker. It covers accepted full/half stuns, blindness, rejected effects, unrelated
self effects, an in-battle self effect, pet ownership and a native NPC lacking a
legacy ID. **58 history/source/publication tests passed**, including the new
accepted-control and older-unavailable-NULL cases, while preserving the original
6,468-fact/23-journey association proof. The ten pure contribution journeys,
nine cross-language contribution contract tests, existing accepted-start/stop
hook test, repository C/C++ formatting and maintained `make -C src -j1` build
also passed.

Both **MariaDB 10.11.14** and **MySQL 8.0.46** passed the complete **64-step**
native writer/source/publication journey. The original 123-fact/38-packet/
28-contribution source proof still conserves damage **112/112**, healing and
casting partitions. A separate fresh source process adds **20 association facts
and 8 control contribution segments**. Each original field matches SQL; every
link verifies; source retention, atomic publication, restricted reports and exact
publication retry conserve **8 applications / 8 received**. Missing sessions and
account/controller identity remain unknown. Both engines retain the sealed
head-64 fingerprints listed above after the source/publication drift tests.

Temporary databases/roles were removed, MariaDB stopped, and the owned MySQL
fixture stopped and removed. MySQL used the existing no-native-AIO workaround.
These tests do not prove actual personal-server effect application or performance;
the affect/service seams are explicitly isolated. TSan retains its earlier unrun
host limitation. The focused commands are in
[BATTLES.md](BATTLES.md#accepted-blindness-and-stun-control-capture).

This supplies accepted application producers within the second completion
requirement. All seven accepted requirements retain their unfinished portions:
complete native context and typed evidence/control/prevention/faction coverage,
real authenticated source journeys, distinct PvE attempts, progression/portfolio
context, the four suites/statistical exports, #487 compatibility and one actual
personal-local gameplay/persistence/performance command/runbook. Production and
staging access remain unnecessary for technical acceptance.

## Native combat build snapshot foundation

The compact context work now has an executable native value reader,
`telemetry_runtime_game_battle_build_context`, with an independently versioned
snapshot contract. It preserves exact class masks rather than projecting a
multiclass character into a single class ID. Base/effective stats and resource
fields remain separate. Saving values are the actual signed modifiers; gear
fields are selected fixed loaded declarations. Neither effective-minus-base nor
the gear fingerprint is presented as an applied-equipment contribution or a
universal power score.

The snapshot is **440 bytes**, with a **448-byte** compile-time ceiling.
Equipment has a fixed **43-slot** traversal; all **309 compiled skill IDs** are
bounded. Learned epic skill IDs/ranks and selected fixed equipment features have
explicit canonical SHA-256 fingerprints tied to content version. Identity,
item naming/prices, wealth and unspent points do not change these fingerprints.
The reader retains at most **64 unique affect nodes**, detects cycles and cap
truncation, separates metadata-only affects, and preserves unknown support
origin. Room arena flags and unique membership in the fixed **3-by-20** roster
remain separate. Duplicate roster/item pointers and missing/invalid catalogs do
not silently produce a known empty build or a definitive arena membership.

The normal and fresh **ASan/UBSan gameplay adapter journeys passed**, including
all occupied equipment slots, the full skill range, exact signed/base/effective
values, fingerprint stability/change cases, NPC reuse/pet identity, source
unavailability, affect bounds/cycles and configuration version changes.
Independent Python encoders match both native SHA-256 digests. OpenSSL memory
callbacks observe **zero crypto heap calls during native reads**, including
the saturated equipment/skill cases. Fingerprinting uses stack-owned SHA state
with checked canonical buffers. Both flatfile and SQL-stub runtime lifecycle
variants also pass with absent weak native skill/arena symbols. **58 existing
battle history/source/publication regressions pass**, preserving the original
123-fact/38-packet/28-contribution source, accepted 8/8 control and unavailable
NULL behavior. Changed C/C++ formatting and maintained `make -C src -j1` pass.

This is a tested native reader, **not durable context capture or a report suite**.
It emits no record and is not wired into per-hit callbacks. No record kind,
schema, outage inventory, migration or published definition changes. Sealed
0063/0064 and the earlier MySQL/MariaDB writer/publication proofs retain their
existing contracts; the SQL engines were not rerun for this reader-only change.
Prototype procs, dynamic equipment effects, complete resistance mechanics,
buff/support ownership, match generations/outcomes and the complete effective
property catalog remain outside this observed subset.

The separately versioned selected record/writer/outage/replay contract and
bounded cached capture are implemented in the following deliveries. The
remaining expectation is to retain and publish matching values and run the
actual personal-local gameplay/readback/performance journey. Use the
existing bounded telemetry transport, private writer, loss inventory and
publication machinery. Commands and field mappings are in
[BATTLES.md](BATTLES.md#native-combat-build-snapshot-reader). All seven accepted
completion requirements remain open for their unfinished work. Production and
staging access remain unnecessary.

## Durable selected combat build contract

The native reader now has a separately versioned selected point-observation
contract, registered as kind **12**, `battle_build`. Definition **1** preserves
**110 fields**, including two exact 32-byte fingerprints, in a **447-byte**
portable encoding. The C++ payload is **448 bytes**; the full fixed record is
**488 bytes**, below the existing 512-byte record limit. The native reader's
440-byte layout remains unchanged. Its raw listed-affect flag banks are excluded
from this persisted subset; bounded counts and partial/origin uncertainty remain.

Battle/live-actor identity, original producer, exact association revision/fact,
one clock pair and configuration/build/content versions qualify every point.
The process-wide context sequence has independent logical replay identity.
Factory and raw validators refuse mismatches rather than borrowing another
actor's build. Availability remains separate by family. Rate/source/configuration
unavailability is an empty point marker with explicit unknown quality; it never
reuses the previous gear/epic digest. No uninterrupted validity, applied gear
contribution, complete power score, caster origin or arena result is inferred.

Migration **0065** adds **110 typed nullable columns**, the logical unique key,
**nine CHECK constraints** and the two independent private incident-schema-**5**
tables. The current lifecycle inventory contains **256 database tables**. Earlier
manifest entries and sealed migrations retain their exact bytes. The compiled
boot contract and all three histories move to head 65. The measured fresh and
restored normalized metadata fingerprints are:

| Engine | Fingerprint at head 65 |
| --- | --- |
| MySQL 8.0.46 | `c344d46b902d27a272cd8567b19b174d51203008be9184611fe7cf66bbe3a74e` |
| MariaDB 10.11.14 | `80b328c879fa3181f532fe0b0e0a1475a8b8998d572a98dc95120f9d81417369` |

The existing private writer serializes and compares every selected field and
digest. Header/producer/UTC/configuration conflicts refuse; identical transport
retries remain one receipt, including after a lost commit reply. A different
receipt cannot replace the same logical point. Invalid records retain canonical
SHA-256 and fixed-record quarantine evidence. Known values from older families
remain separate: older rows have NULL build columns, and kind 12 has NULL older
payload columns. No alternate worker or transport is introduced.

Durable outage version **4**, `DMSTLJ04`, includes kind 12. Native and offline
readers preserve earlier v1/v2/v3 inventories with their original family ceilings
for both observed and failure masks; atomic upgrades preserve previous
observations. The maintained incident CLI exposes schema-5 templates while
preserving its original default. Independent schema-5 registration verifies the
committed post-fix receipt and explicit scope/UTC, rolls metadata and detail back
together on failure and retains exact retries. Current published definition 5
still selects incident schema 4. Definitions 1/2/3/5 validate and skip kind 12
without changing their input totals, retained-source or quality meaning. There is
no newly activated build report.

Local qualification passed **10 native/Python contract tests**, normally and
with fresh **ASan/UBSan**, including exact field order/widths, independent portable
encoding, signed extremes, digest bytes, partial families, empty markers,
corruption, strict row shape and sealed-definition compatibility. The native
queue preserves admitted values and exact association references through control
reserve; loss stays explicit when that reserve is full. **20 incident tests**,
native/offline outage tests, both durable runtime-outage variants, the gameplay
adapter and both runtime lifecycle variants pass. TSan remains unsupported after
its trivial runtime probe failed; the transport harness did not run under TSan.

Both disposable **full-65-step MySQL and MariaDB** chains passed the selected
field/constraint/NULL/replay journey, independent restricted schema-5 registration,
guarded reruns, incompatible-width/reordered-index/weakened-check refusal and
exact restored metadata. Both all-twelve-family writer journeys passed, including
lost acknowledgements, logical conflicts, empty configuration gaps and exact
quarantine evidence. The MySQL fixture temporarily preserves/restores the new
family-isolation CHECK around its pre-existing missing-column startup test,
because MySQL refuses renaming a checked column. The actual migration keeps that
constraint enforced.

The established native source/definition-5 publication journey also passes on
both engines at head 65, preserving **123 association facts / 38 packets / 28
verified contribution links**, **112/112 damage** and the fresh **8/8 accepted
control** publication. Retained source, rollback, lost acknowledgements, private
roles, CLI, original generations, drift/restoration and snapshot verification
remain qualified. **58 history/source/publication**, **14 engine**, **15 identity
publication**, **14 report**, **24 immutable migration**, **22 lifecycle** and
**10 boot/runtime** regressions pass. Header/golden contracts, Python/shell syntax,
changed C/C++ formatting, manifest validators and maintained `make -C src -j1`
also pass. SQL databases/reviewer roles are removed and owned database fixtures
stopped; MySQL retains the existing no-native-AIO fixture workaround.

The bounded native capture expectation is qualified by the following delivery.
Retained-source/publication, the full balance suites and actual personal-server
acceptance remain open. Commands and field meaning are in
[BATTLES.md](BATTLES.md#durable-selected-combat-build-observations).

## Native cached build capture

The runtime now connects the selected native reader to actual shared battle
associations. Accepted hostile/support observations, proven same-room formal
presence and existing-actor context callbacks capture entry/change/configuration,
periodic and recovery points. The original kind-12 writer, portable layout,
migration 0065, outage version 4 and private incident schema 5 are unchanged.
This delivery adds no migration or report definition.

A **512-entry fixed cache** binds opaque addresses to native runtime lifetimes
and live actor/battle identities. Full reads are limited before gear/epic hashing
to **16 per second** globally and the qualified per-entry context cap, normally
**8 per 60-second window**. Equal dirty reads also consume this budget. Dirty
marks coalesce; named-field comparison suppresses equal selected profiles.
Periodic capture uses `max(config.interval_usec, 10 seconds)`, normally **60
seconds**. It selects at most **16 cache entries** and resolves them through one
live-world pass of at most **4,096 nodes**, verifying runtime and association
identity without dereferencing cached pointers. It runs after expiry and never
extends measured engagement or participation.

Successful equipment changes and completed `affect_total` recalculation mark
existing cache entries. The hook copies no build and emits no record; the next
observation samples current state. Deferred scheduling alone does not claim
completed values. Snapshot clocks are observation times, not exact mutation
times or continuous validity. Source, rate and configuration gaps clear profiles
and digests. Configuration gaps clear configuration/build/content IDs as well.
Empty markers have their own **16-per-second** admission budget; suppression
latches context-overflow quality. Build-only queue loss retains independent
quality and cannot degrade otherwise measured damage/control coverage. Every
attempted point obtains a fresh process-wide sequence; recovery reads current
values with a fresh key.

The existing gameplay suite now qualifies gear/epic changes, equal coalescing,
invalid gear with known stats retained, configuration changes and withdrawal,
NPC/pet lifetime separation at a reused address, a freed cached character,
temporary world-list loss and recovery, periodic samples, expiry, actor/global
read and marker caps, and saturated-queue recovery without key reuse. The
equipment fixture qualifies accepted versus rejected mutation marking. Normal
and fresh **ASan/UBSan** gameplay runs pass, along with both runtime
lifecycle/outage/exhaustion variants, **58** history/source/publication tests,
standalone header/golden contracts, changed-line formatting and the maintained
server `make -C src -j1` build.

Both full-65-step disposable **MariaDB 10.11.14** and **MySQL 8.0.46** chains
qualify **20 actual native reader/runtime/worker/private-writer points**. All
**110 selected fields**, digests, signed values, partial families, empty gaps and
association/configuration references match exactly. Existing published rows
remain unchanged. The previous source/publication journey preserves **123 facts
/ 38 packets / 28 verified links**, **112/112 damage**, and fresh **8/8 accepted
control**, including bounded retained source, private roles, rollback, lost
acknowledgements and drift/restoration. Schema fingerprints remain the head-65
values above. Owned databases and reviewer roles are removed, database fixtures
stopped and the existing MySQL no-native-AIO workaround retained.

This qualification still uses gameplay objects and game-service seams in an
executable fixture, including explicit future pulse clocks. It does not establish
a full personal-server session/save/readback or performance journey, complete
native effect classification, continuous build exposure or a new build report.
TSan remains unsupported on the local runner as previously recorded.

**Historical expectation at native-capture delivery (superseded below):** retain exact kind-12 observations and publish
them under a separately versioned comparison definition with independent
schema-5 loss coverage. Verify packet/alias, point-time, configuration,
availability and sampling-gap evidence before comparisons; preserve current
definition-5 meaning and published generations. Then qualify actual
personal-local gameplay/readback/performance. Complete native context and typed
outcomes, distinct PvE attempts/objectives/reward linkage, rested/assistance and
portfolio additions, four complete balance suites/statistical exports and #487
canonical economic compatibility remain required. All seven final accepted
requirements retain their unfinished portions; production or staging access is
unnecessary. See [BATTLES.md](BATTLES.md#native-cached-build-capture) for exact
boundaries, limits and maintained local commands.

## Retained build point publication

The retained kind-12 publication step is implemented under independent report
definition **6**, with its own exact source/publication stores and schema-5 loss
coverage. Definition 5's kinds 9–11, schema-4 reviews, tables, row shapes and
published generations retain their meaning. All 110 fields and their digests
are retained with configuration evidence at the input page/cursor transaction.
Publication verifies association packets, live actor kind, original/canonical
alias lineage, producer clocks, stale references and closed observed prefixes.
The `battle_build_points` report retains unknown families and partial/gap points.

Point context qualification is distinct from family availability and from
verified packet links. Unknown buff origin and unavailable families remain
visible. A qualified point has no inferred continuous exposure, contribution
amount, human identity or arena result. Those limits prevent a later gear change
or a grace-tail read from rewriting earlier battle evidence.

Migration **0066_telemetry_build_publication** adds four protected stores, with
all three prefixes and boot/lifecycle consumers updated to **66 steps / 260
tables**. Earlier immutable migrations are unchanged. Measured fresh/restored
normalized fingerprints are:

- MariaDB 10.11.14:
  `292d480353901e4b197a89a73667e3cd81b7beb747b343e9bc0acd1fff0fa3f3`.
- MySQL 8.0.46:
  `22cdbdb0f767e1bf3dd62405692c9f180c15b3865ba24cfddbe7c2ea3b0d3e29`.

The expanded native history suite has **65 passing tests**, including exact
retained fields/configuration, partial families, missing packet/configuration,
invalid clocks, alias and stale references, independent kind-10/kind-12 loss,
logical conflicts, tampering and bounded refusal. Both full-chain SQL journeys
qualify **20 actual native reader/runtime/worker/private-writer points** through
retention and atomic publication. Restricted report roles, source/publication
lost acknowledgements, rollback, CLI access, raw retention/catalog changes,
guarded reruns and column/index/check drift with exact restoration pass. A
separate corrected schema-5 review publishes actual kind-12 loss under a new
generation while preserving the earlier generation's frozen inventory and rows.
The 123-fact/38-packet/28-link, 112/112 damage and 8/8 accepted-control journeys
remain qualified. Coverage durations and qualification counts depend on their
actual captured clocks; controlled dated fixtures are separately labeled.

The maintained server build, changed-line formatting, rollup, incident,
reservation/budget, migration, lifecycle and boot contract checks qualify this
increment. These executable gameplay objects and service seams still do not
establish personal-server authentication/save/readback or measured performance.
All seven final accepted requirements retain unfinished portions.

**Historical expectation at retained-build delivery (superseded below):** complete typed control attempts, resistance,
immunity and overlap-safe duration, the remaining native prevention/faction and
typed death/escape/objective producers and battle reports, then qualify actual
personal-local gameplay/readback/performance.
Distinct PvE attempts/objectives/recovery/reward linkage, rested/assistance and
milestone/switching/portfolio additions, the four complete suites/statistical
exports and #487 canonical economic compatibility remain required under #258.
Production/staging access is unnecessary. See
[BATTLES.md](BATTLES.md#retained-build-point-publication) for exact semantics,
limits, permissions and maintained local commands.

## Expanded accepted status-control capture

The maintained major/minor paralysis, slow, sleep, silence and entangle spells
now capture an application only after their actual accepted mutation. All four
silence duration branches and both temporary-paralysis/direct-binding entangle
branches are covered. Existing immunity, resistance, saving, movement freedom,
location, already-active and special bypass behavior remains exact. Sleep
refreshes are accepted application operations, not additional disabled time.

The existing kind-11 accumulator, retained source and atomic battle publisher
conserve **17 applications / 17 received** from this fresh native journey. The
original `blind`/`Stun` **8/8** journey remains independent. Normal and fresh
ASan/UBSan gameplay fixtures, **66 history/publication tests**, formatting and the
maintained server build pass. Both complete local MariaDB 10.11.14/MySQL 8.0.46
chains qualify exact native source/SQL values, private-role retention, published
reports, retries, unknown identity and unchanged older generations. The sealed
schema head remains **66 / 260 tables** with its prior fingerprints; wire fields,
availability masks and earlier published meanings are unchanged. Existing exact
native battle and definition-6 build-point publication proofs pass.

The accepted counts aggregate reviewed producers. Effect-family attempts,
resistance/immunity, removal and effective duration remain unavailable. Modifier
flags are segment unions and cannot apportion self versus other applications.
These fixtures use game-service seams; actual personal-server effect, save,
readback and performance journeys remain required. All seven final requirements
retain unfinished portions. See [BATTLES.md](BATTLES.md#expanded-accepted-status-control-capture)
for the exact source catalog, semantics and maintained local commands.

## Typed control resolutions and target-state storage

Record family 13 separates individual resolved operations from observed target
state. Its exact 76-field encoding is 368 bytes, with a 408-byte payload inside
the unchanged 488-byte telemetry record. The fixed 512-target accumulator uses
213,048 bytes, allocates no memory on events and retains no game pointers.
Overlaps share one target timeline. Entry, change, context/configuration cuts,
departure, battle closure and explicit loss seal only actual observed prefixes.
Delivery recovery starts from a cleared gap and fresh baseline. Sequence keys
never wrap or repeat, and a UTC regression remains marked after UTC recovers.

The native `blind`/`Stun` helpers and major/minor paralysis, slow, sleep, silence
and entangle spell bodies preserve the branch that actually ran. The expanded
fixture captures **56 typed resolutions**, **18 accepted operations** and
**12 distinct rejection reasons**, including saving, resistance, immunity,
movement protection, refresh, bypass and the untimed bound branch. Existing
kind-11 **17/17** and original **8/8** accepted-application journeys keep their
meanings. Rejected outside targets remain unassociated; those result points
create no battle edge, enrollment or merge. Configured ticks are declarations,
including signed native values, and do not establish elapsed time.

Live target-state reads use the existing bounded cached build pass, with no
additional world traversal. They have zero duration coverage and explicit
`CONTEXT_UNKNOWN` while the full native mutation/removal inventory is incomplete.
Blindness, slow and other selected flags have different action restrictions;
their union cannot yet be published as time unable to act. Outer blindness
spell gates and the remaining selected-status sources still need qualification.

Migration `0067_telemetry_typed_control` adds nullable columns, the logical
sequence index and exact insert/update validation triggers. Moving the eight
specific validation rules into triggers avoids MariaDB's table-definition
metadata limit. Verification and runtime fingerprints cover exact trigger bodies
while preserving operator grouping, SQL mode, event, timing and order. Both
**MariaDB 10.11.14** and **MySQL 8.0.46** pass the **67-step / 262-table** storage
chain, all **455 payload-column** checks, exact cross-language values, inactive
NULL separation, logical uniqueness, scoped private incident review, atomic
rollback, guarded reruns and column/index/trigger drift with exact restoration.
Migrations 1–66 and all three previously supported history prefixes are unchanged.
The independently reviewed family-13 inventory is incident schema 6; old
schemas 1–5 stay sealed. New outage journals use `DMSTLJ05`, preserving read-only
compatibility with versions 1–4 and the protected lifecycle queue reserve.

Fresh and restored metadata fingerprints are:

- MariaDB 10.11.14:
  `b10fdf14f7f327e33d1b01e18ec5ce40d907d116e0f13d9f5bc323895d8ee559`.
- MySQL 8.0.46:
  `0d96a65461760b07fdd068e20a47e0289d3e704baa769ad0a1b6b069596687b8`.

The **164-test** focused Python suite passes. Its new mixed-stream regression
validates control rows before advancing existing report cursors, preserves
definitions 1/2/3/5/6 and rejects malformed controls, unknown families, changed
receipts and exceeded byte budgets. Pure and actual gameplay fixtures pass
normally and under fresh ASan/UBSan. Runtime lifecycle/outage/exhaustion,
standalone header contracts, changed-line formatting and the maintained server
build pass. Definition 7 is still unavailable; this increment does not retain
or publish control comparisons.

Both complete **67-step** private-writer and native runtime/queue/worker/SQL
journeys also pass. Exact readback preserves every control field for the
**56/18/12** resolution fixture and explicitly partial target states. Existing
**123 association facts / 38 packets / 28 contribution links**, **112/112 damage**,
accepted-control **8/8 and 17/17**, and **20 native build points / 110 fields**
retain their qualified meanings through source retention and atomic publication.
Private roles, bounded reports, rollback, lost acknowledgements, corrected
reviews, immutable old generations and exact schema restoration pass. The dated
fixture preserves and restores the complete mixed receipt namespace. Owned
temporary databases/reviewer users were removed and SQL fixtures stopped.

Accounting's alchemy publication, room item payload and ward durability changes
are integrated through base commit `f7d26eaa721cd3b675c0b0c65009a2535813f400`.
All **144 sealed migration files** from both parents retain their original bytes.
The three default histories preserve accounting's first 56 receipts and append
14 telemetry steps. Three explicit telemetry histories preserve their first 67
receipts and append the three accounting steps. Both database engines qualified
all **six histories at 70 steps / 263 tables**, including reruns, retained data,
native migration-session faults, compiled boot and restore selection, and
rejection of altered receipts, mixed state and changed generated expressions.
See [runtime compatibility](../persistence/RUNTIME_COMPATIBILITY.md) for the
exact manifest selection and common schema contract.

The combined writer uses **portable copyover 19**. It reads accounting's portable
18 format and telemetry's native 12–18 formats with their declared ABI checks.
Version 19 preserves the telemetry-v2 ownership context alongside bounded world,
descriptor and custody data. Portable golden bytes, framing/CRC/allocation
faults, invalid optional ownership, deferred telemetry qualification, native
custody recovery and save/exec failures pass focused ASan/UBSan fixtures.
See [copyover format](../persistence/COPYOVER_FORMAT.md).

Interactive account loading now carries a game-thread scope snapshot to the
account worker. After its required account transaction commits, the shared
lifetime/token allocator optionally runs on that worker's own connection. A
committed token and scope enter the returned snapshot; unavailable preparation,
exceptions and ambiguous commits retain unknown identity and the valid account
snapshot. Dirty or failed worker handles are retired. The actual worker/session
regressions pass ASan/UBSan, five scope/cache/client-free tests pass, and both
70-step native SQL fixtures qualify owned connection use, deadlines, optional
failure behavior, retries, ambiguity, rename, recreation and private grants.
Token preparation supplies no authentication evidence. The maintained server
build and changed-line formatting pass. See
[interactive account loading](../persistence/ASYNC_ACCOUNT_LOAD.md).

Both complete **70-step** common-schema storage, private repository and native
runtime/queue/worker/writer/publication journeys pass. Native SQL readback retains
**56 typed resolutions / 18 accepted / 12 rejection reasons**, all **76 control
fields**, explicit partial duration coverage and rejected-target isolation.
Storage checks retain all **455 payload columns**, independent schema-6 loss
review, guarded reruns and exact trigger/operator-grouping drift refusal and
restoration. Earlier **123 association facts / 38 packets / 28 links**, **112/112
damage**, accepted-control **8/8 and 17/17**, and **20 build points / 110 fields**
remain qualified, including private roles, bounded reports, retries, lost
acknowledgements, corrected reviews and unchanged older generations. Temporary
fixture databases/users were removed and SQL servers stopped.

The common fresh/restored normalized metadata fingerprints are:

| Engine | Fingerprint |
| --- | --- |
| MariaDB 10.11.14 | `5b2476e2551ea87a13adcf5d702412262a35fb9a987f73f3bfe851d09f069123` |
| MySQL 8.0.46 | `752c4b0485fcd9b5daf796a32f4b7cfc4f7ce573c992b5f7abf7f9476f59060b` |

Actual personal-server authentication/effect/save/readback and measured
performance remain required. All seven final #258 requirements remain open.

**Next executable expectation:** complete the native selected-status mutation/removal
inventory and duration qualification, retain exact controls and publish reviewed
comparisons under a new definition, then finish native prevention/faction,
typed death/escape/objective evidence and complete battle reports. Distinct PvE
attempts/objectives/failure/recovery/interruption and committed rewards,
rested/assistance/milestone/switching/portfolio additions, all four balance suites
and statistical exports, #487 economic compatibility and the real personal-local
gameplay/persistence/performance gate remain required. Production or staging
access is unnecessary. See [CONTROL_OBSERVATIONS.md](CONTROL_OBSERVATIONS.md)
for exact semantics, limits and maintained local commands.
