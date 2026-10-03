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
| #565: shared serialization/schema descriptor | Implemented and locally qualified | `telemetry_columns.inc` supplies the column identities used by serializers and startup validation. Preserve all record kinds 1–8, replay keys and column prefixes. Reserved/padding/absent union fields and sink-generated fields have explicit exclusions. |
| #566: durable outage/loss evidence | Implemented and locally qualified | Worker registration before SQL initialization/admission; protected exclusive checksummed storage; bounded coherent samples; clean drain versus known abandonment and unknown tails; real restart/exec/SIGKILL and storage-failure tests. Offline read-only export preserves unknown ends. See [OUTAGE_STORAGE.md](OUTAGE_STORAGE.md) and the qualified #567 report integration below. |
| #567: reviewed incident coverage | Implemented and locally qualified | Consecutive retained inventory versions, nullable unknown ends, committed first verified post-fix references, explicit backlog/reconstruction dispositions and atomic published coverage snapshots. Reports preserve gaps, source uncertainty and bounded private-role separation. Full local MariaDB/MySQL chains, capacity, digest/permission negatives, lost commit replies and unchanged v1 totals qualified. Historical facts require evidence; synthetic fixtures do not establish a real incident history. See [INCIDENT_COVERAGE.md](INCIDENT_COVERAGE.md). |
| Initial session qualification/capacity recovery | Implemented and locally qualified | Existing descriptor sweep and context/evidence adapters retry missing entry. Deferred copyover retains one handoff in descriptor memory; supplied keys/totals/revision survive. No earlier unobserved time or human activity is invented. True capacity refusal rolls back IDs; lifecycle queue loss retains admitted IDs. See [SESSION_LIFECYCLE.md](SESSION_LIFECYCLE.md). |
| Publish existing progression/encounter/combat observations | Implemented and locally qualified | Definition 2 publishes five bounded projections from typed kinds 6–8. Cumulative participant/actor facts replace earlier measurements, threshold consumption stays separate from XP, unknown tails remain NULL, and published read-only snapshots retain incident coverage. Both engines passed the full 55-step chain and replay, rollback, constraints and role negatives. See [OBSERVATION_PROJECTIONS.md](OBSERVATION_PROJECTIONS.md). |
| Character/account/confirmed controller association | Pending | Cached scoped tokens, authenticated account ownership, reviewed cross-account linkage and dated association versions. Unknown controllers remain unknown. |
| Shared battles and changing rosters | Pending | Link opponents and support actors to a shared battle, retain mode/roster segments, ownership-aware pets, compact context, observed outcomes and censored boundaries. |
| PvE zone attempts and committed rewards | Pending | Separate attempt identity, supported objective evidence, PvP interruptions, effort and exact operation-ID linkage. Generic zone entry or one kill must not imply a full clear. |
| Progression and portfolios | Pending | Rested/assistance provenance, earned/lost/restored/admin XP, milestone exposure and censoring, account/controller portfolio totals and union player-time. |
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
inventory agree. The current inventory contains 234 SQL tables and 51
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
