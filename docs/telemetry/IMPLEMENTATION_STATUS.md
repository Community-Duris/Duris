# Telemetry expansion implementation status

The accepted scope is [BALANCE_EXPANSION_PLAN.md](BALANCE_EXPANSION_PLAN.md),
including the recovered requirements from PR #677. Implementation is in progress.
Technical acceptance uses disposable local databases and a personal local game
setup. Production or staging access is not a prerequisite for completing this
work. A future production deployment is a separate operational decision.

## Delivery record

| Requirement | State | Evidence or remaining work |
| --- | --- | --- |
| #561: full writer startup contract | Implemented and locally qualified | Validate every current writer column, type, signedness, width, nullable/default semantics, InnoDB engine and 12 required indexes. Exercise effective SELECT/INSERT/session UPDATE with zero-row statements and rollback. Reject admission until the worker qualifies; later transient outages retain buffering. |
| #565: shared serialization/schema descriptor | Implemented and locally qualified | `telemetry_columns.inc` supplies the column identities used by serializers and startup validation. Preserve all record kinds 1–9, admitted replay keys and column prefixes. Reserved/padding/absent union fields and sink-generated fields have explicit exclusions. |
| #566: durable outage/loss evidence | Implemented and locally qualified | Worker registration before SQL initialization/admission; protected exclusive checksummed storage; bounded coherent samples; clean drain versus known abandonment and unknown tails; real restart/exec/SIGKILL and storage-failure tests. Offline read-only export preserves unknown ends. See [OUTAGE_STORAGE.md](OUTAGE_STORAGE.md) and the qualified #567 report integration below. |
| #567: reviewed incident coverage | Implemented and locally qualified | Consecutive retained inventory versions, nullable unknown ends, committed first verified post-fix references, explicit backlog/reconstruction dispositions and atomic published coverage snapshots. Reports preserve gaps, source uncertainty and bounded private-role separation. Full local MariaDB/MySQL chains, capacity, digest/permission negatives, lost commit replies and unchanged v1 totals qualified. Historical facts require evidence; synthetic fixtures do not establish a real incident history. See [INCIDENT_COVERAGE.md](INCIDENT_COVERAGE.md). |
| Initial session qualification/capacity recovery | Implemented and locally qualified | Existing descriptor sweep and context/evidence adapters retry missing entry. Deferred copyover retains one handoff in descriptor memory; supplied keys/totals/revision survive. No earlier unobserved time or human activity is invented. True capacity refusal rolls back IDs; lifecycle queue loss retains admitted IDs. See [SESSION_LIFECYCLE.md](SESSION_LIFECYCLE.md). |
| Publish existing progression/encounter/combat observations | Implemented and locally qualified | Definition 2 publishes five bounded projections from typed kinds 6–8. Cumulative participant/actor facts replace earlier measurements, threshold consumption stays separate from XP, unknown tails remain NULL, and published read-only snapshots retain incident coverage. Both engines passed the full 55-step chain and replay, rollback, constraints and role negatives. See [OBSERVATION_PROJECTIONS.md](OBSERVATION_PROJECTIONS.md). |
| Native account lifetime and scoped token preparation | Implemented and locally qualified | Migration 0056 retains retired lifetimes, follows actual renames and issues one opaque token per scoped lifetime. Account-load caching, transaction/entropy/allocation failures, ambiguity, simultaneous preparation, deletion/recreation and restricted permissions passed on both local SQL engines. Preparation itself emits no authentication or participation evidence. See [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md). |
| Authenticated ownership observations | Implemented and locally qualified | Kind 9 observes scoped cached identity only on an authenticated playing descriptor with matching current account membership. Bounded admission recovery preserves original clock/payload, explicit loss spans and fresh transport key order. Reconnect/copyover, missing identity, logout retention, native writer replay/permissions, mixed-stream report compatibility and ownership outage export are qualified. See [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md). |
| Restricted reviewed identity registration and generation reservation | Implemented and locally qualified | Migration 0058 authenticates a provisioned SQL reviewer through the maintained registrar, verifies scoped issued account tokens, retains dated correction histories and reserves an immutable reviewed version/digest or explicit unknown identity per new balance generation. Both final 58-step chains, CLI, read-transaction release, capacity, permissions, rollback/ambiguity and exact fresh/restored fingerprints passed. Reservations are metadata and do not publish effort or imply complete identity. See [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md). |
| Copyover ownership context and ownership-loss review | Implemented and locally qualified | Outer copyover 18/telemetry-v2 retains last observed account context without importing an old monotonic clock or using the token as current authority. Legacy framing, overflow/unknown context and actual reloaded-token/scope/deletion changes passed native and ASan/UBSan fixtures. Migration 0059 adds independent incident schema-v2 inputs for families 1–9 and reuses existing snapshots; both 59-step chains, CLI, capacity, replay, private roles, atomic snapshot rollback and exact fresh/restored fingerprints passed. Definition-3 integration is qualified below. See [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md) and [INCIDENT_COVERAGE.md](INCIDENT_COVERAGE.md). |
| Atomic identity effort and observed XP portfolio publication | Implemented and locally qualified | Migration 0060 adds exact bounded source retention tied to the cursor, a conserved publication header and two reports. Definition 3 consumes the reserved dated review and incident schema 2 in the same publication transaction. Both 60-step chains qualified the actual CLI, corrections, old generations, explicit unknown identity, source/publication rollback, real committed writes with lost acknowledgements, private roles, budget refusal, guarded reruns and exact fresh/restored fingerprints. Definitions 1/2 retain their earlier amounts. See [IDENTITY_PUBLICATION.md](IDENTITY_PUBLICATION.md). |
| Character/account/confirmed controller association | In progress | Dated review/correction, ownership cuts, same-producer attribution, unknown linkage, native lifetime/token allocation, authenticated source capture, identity wire handoff, reviewed SQL registration and immutable generation reservations are qualified. Atomic identity effort/report publication now consumes the reserved review and qualified ownership incident snapshot. Real personal source journeys and linkage coverage in the complete balance suites remain required. See [IDENTITY_HISTORY.md](IDENTITY_HISTORY.md). |
| Shared battles and changing rosters | In progress | Native prerequisites retain producer-scoped formal group generations through appointment/departure and observe actual PCs on both accepted combat sides, including pet-owned PvP context and explicit NPC union guards. The pure bounded association module qualifies shared hostile/support/presence graphs, roster/mode/context cuts, pets, aliases, uncertainty and censored closure. Native value adapters qualify existing live NPC lifetimes, actual formal-roster revisions/presence and admitted current session/encounter links. Portable definition-1 facts, canonical numeric fields, complete mutation packets and bounded replay receivers are now qualified across native/Python fixtures. Native battle callbacks, compact context/control/prevention/population sources, durable facts/contributions and atomic published projections remain required. See [BATTLES.md](BATTLES.md). |
| PvE zone attempts and committed rewards | Pending | Separate attempt identity, supported objective evidence, PvP interruptions, effort and exact operation-ID linkage. Generic zone entry or one kill must not imply a full clear. |
| Progression and portfolios | In progress | Observed XP/level projections and published exact account/controller effort unions and observed XP portfolio amounts are qualified. Rested/assistance provenance, milestone exposure/censoring, switching, canonical comparable rewards and rates remain required. |
| Four balance report suites and study exports | Pending | Racewar, solo/group PvP, zone and progression reports from published aggregates; uncertainty, repeated-team influence and coverage visible. Preserve existing report definitions. |
| #487: economic projection compatibility | Pending audit and implementation | Reuse current accounting reconciliation where it meets acceptance; project canonical earned receipts without counting compatibility ledgers, transfers or openings as new rewards. |
| #258: local observational acceptance and final runbook | In progress | Initial readiness/capacity recovery is implemented and qualified in executable fixtures. Remaining: dedicated roles/catalog, real session/progression readback, normal gameplay/save, failure/recovery and a single reproducible local qualification command. |

## Qualified first-layer checks

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

This increment prepares durable integration. It does not activate a new
persistent record kind or native battle callbacks, change sealed migrations,
or publish balance results. The next step uses this descriptor through the
existing queue/writer, with independent logical/transport replay identities,
additive schema and versioned outage/incident handling, followed by atomic
projection. Packet completeness does not repair cross-packet source gaps or
prove ownership, outcome or alias-history coverage. All seven remaining full
completion requirements in #258 remain open, including the four balance suites
and real personal-local gameplay/persistence/performance qualification.
