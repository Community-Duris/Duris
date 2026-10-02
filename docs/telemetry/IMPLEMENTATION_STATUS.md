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
| #566: durable outage/loss evidence | Implemented and locally qualified | Worker registration before SQL initialization/admission; protected exclusive checksummed storage; bounded coherent samples; clean drain versus known abandonment and unknown tails; real restart/exec/SIGKILL and storage-failure tests. Offline read-only export preserves unknown ends. See [OUTAGE_STORAGE.md](OUTAGE_STORAGE.md). Report gap integration remains #567. |
| #567: reviewed incident coverage | Pending | Versioned incident registry, nullable unknown ends, first verified post-fix observation, backlog disposition and gap visibility in rollups/reports. Historical facts require evidence; synthetic fixtures cannot establish a real incident history. |
| Publish existing progression/encounter/combat observations | Pending | Versioned bounded projections and reports, with deduplication, coverage and read-only report permissions. |
| Character/account/confirmed controller association | Pending | Cached scoped tokens, authenticated account ownership, reviewed cross-account linkage and dated association versions. Unknown controllers remain unknown. |
| Shared battles and changing rosters | Pending | Link opponents and support actors to a shared battle, retain mode/roster segments, ownership-aware pets, compact context, observed outcomes and censored boundaries. |
| PvE zone attempts and committed rewards | Pending | Separate attempt identity, supported objective evidence, PvP interruptions, effort and exact operation-ID linkage. Generic zone entry or one kill must not imply a full clear. |
| Progression and portfolios | Pending | Rested/assistance provenance, earned/lost/restored/admin XP, milestone exposure and censoring, account/controller portfolio totals and union player-time. |
| Four balance report suites and study exports | Pending | Racewar, solo/group PvP, zone and progression reports from published aggregates; uncertainty, repeated-team influence and coverage visible. Preserve existing report definitions. |
| #487: economic projection compatibility | Pending audit and implementation | Reuse current accounting reconciliation where it meets acceptance; project canonical earned receipts without counting compatibility ledgers, transfers or openings as new rewards. |
| #258: local observational acceptance and final runbook | Pending | Dedicated roles/catalog, real session/progression readback, normal gameplay/save, failure/recovery and a single reproducible local qualification command. Cover players who log in before startup qualification, without inventing earlier observed time. |

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

This implementation does not establish battle balance, controller linkage
coverage, zone difficulty or progression speed. Those require the remaining
implementations above and the end-to-end local qualification. Outage evidence
is durable; reviewed incident registration and visible report gaps remain pending.
