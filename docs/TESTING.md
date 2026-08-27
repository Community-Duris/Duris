# Testing

The project uses focused regression tests in `tests/async/`, plus
schema/migration checks at `tests/` root. The root `Makefile` provides a single
developer and CI gate while retaining fast commands for focused work.

## Layout

```
tests/
├── async/                       # focused regression + source-contract tests
│   ├── test_*.py                # plain python3 regressions; no framework
│   ├── run_*.sh                 # special-purpose and legacy thin wrappers
│   └── run_*_mysql.sh           # MySQL-backed schema-contract tests (need a live DB)
├── run_regression_tests.py      # discovery, bounded parallelism, failure summary
├── compare_bootstrap_mud_schema.sh   # diff live schema vs bootstrap baseline
├── test_migration_replay_safety.sh   # migration re-run safety
└── test_run_migration_persistence_schema.sh
```

## Test styles

**Source-contract tests** — read the C sources as text and assert structural
invariants (a guard exists, a call site was not reintroduced, an ordering
holds). Example (`tests/async/test_sql_pool_shutdown.py`): slices out
`sql_pool_acquire`/`sql_pool_shutdown` from `src/sql_pool.c` and asserts the
closing-pool checks are present. These need no database and no build.

**Behavioral tests** — boot or exercise server logic where feasible; most
regression coverage for past crashes is contract-style because full boots are
expensive.

**Schema tests** - verify migrations/persistence contracts against disposable
MySQL or MariaDB instances (development databases only).

## Running

```bash
# Complete safe developer/CI gate: builds all maintained binaries and tools,
# generates world data, then runs Python and native regression tests.
make test-all

# Regression tests without rebuilding the server or area editor:
make test

# Limit concurrency, filter by filename, or inspect discovery:
make test TEST_JOBS=1
make test-python TEST_MATCH=wear
make test-list

# Single test (preferred while iterating):
python3 tests/async/test_wear_all_regression.py
# or via its wrapper:
tests/async/run_sql_pool_shutdown.sh

# Isolated Docker/MySQL schema suites (Docker is an optional prerequisite):
make test-db

# Runtime schema compatibility on both supported variants:
tests/async/run_runtime_compatibility_mysql.sh
RUNTIME_DB_IMAGE=mariadb:10.11 tests/async/run_runtime_compatibility_mysql.sh
```

`TEST_JOBS=0` is the default and selects up to eight workers based on available
CPUs. Test output is buffered per process so parallel failures remain readable.
The runner executes every discovered `test_*.py` in a separate process and
returns nonzero if any test fails.

`make test-all` deliberately excludes Docker and externally provisioned
database checks. `make test-db` creates and destroys isolated MySQL containers;
the three scripts at the root of `tests/` require explicitly named disposable
or read-only databases and are manual migration-verification tools. Never point
them at production.

## Validation matrix

No single command proves release readiness. Use the narrowest applicable row while
iterating, then run every row required by the session or release gate.

| Evidence boundary | Command | What it proves | What it does not prove |
|---|---|---|---|
| Documentation | `python3 tests/async/test_documentation_contract.py` | Maintained links, paths, commands, configuration names, safety language, and diagram contracts | Runtime or database behavior |
| Focused source/runtime | `python3 tests/async/test_<feature>.py` | One named invariant or compiled harness | Unrelated domains or integrated load |
| Server build | `make -C src` | C++20 server compiles under the warning profile | Schema compatibility or runtime readiness |
| Repository gate | `make test-all` | Maintained builds, generated world inputs, all discovered Python tests, and native signal tests | Docker database suites, representative data, or a 200-player hold |
| Disposable schema | `make test-db` | Listed schema contracts on isolated Docker MySQL | MariaDB parity or configured database state |
| Dual-engine boot contract | `tests/async/run_runtime_compatibility_mysql.sh` and `RUNTIME_DB_IMAGE=mariadb:10.11 tests/async/run_runtime_compatibility_mysql.sh` | Fresh bootstrap, immutable head, drift rejection, and boot compatibility on MySQL 8 and MariaDB 10.11 | A configured or production upgrade |
| Lifecycle/privacy | commands below | Pending-policy fail-closed behavior, synthetic archive/export/erasure contracts, and disposable schemas | Controller approval, legal compliance, or enabled canonical mutation |
| Capacity/fault precursors | commands below | Bounded 25/50/100/200 logical-client codecs and named crash/fault invariants | Representative eight-profile 30-minute 200-player readiness |
| Final Phase 03 gate | Session 14 specification and its future sanitized report | Only the complete qualified workload, fault, reconciliation, privacy, migration, and restore evidence can support readiness | Nothing until every criterion is executed and recorded |

### Load, recovery, and fault contracts

These focused tests use source inspection, compiled local harnesses, or synthetic
state. They do not connect to the configured database:

```bash
python3 tests/async/test_player_load_pipeline.py
python3 tests/async/test_player_load_items.py
python3 tests/async/test_player_load_pets.py
python3 tests/async/test_player_save_pipeline.py
python3 tests/async/test_player_save_journal.py
python3 tests/async/test_critical_command_coordinator.py
python3 tests/async/test_critical_transaction_contract.py
python3 tests/async/test_world_recovery_pipeline.py
python3 tests/async/test_maintenance_scheduler.py
python3 tests/async/test_worker_fault_injection.py
python3 tests/async/test_phase01_recovery_gate.py
python3 tests/async/test_phase02_capacity_and_crash_gate.py
```

The Phase 01 and Phase 02 gates include 25/50/100/200 logical-client waves and bounded
codec/crash coverage. They are precursor evidence only. They do not perform the eight
representative workload profiles, 30-minute 200-player holds, production-clone query
measurements, or complete fault/reconciliation matrix required by Session 14.

### Lifecycle and privacy contracts

Run the source/synthetic controls first:

```bash
python3 scripts/validate_data_lifecycle.py --json
python3 tests/async/test_data_lifecycle_manifest.py
python3 tests/async/test_season_reset_manifest.py
python3 tests/async/test_lifecycle_archive_execution.py
python3 tests/async/test_personal_data_export.py
python3 tests/async/test_account_erasure.py
python3 tests/async/test_immutable_migration_runner.py
python3 tests/async/test_runtime_boot_compatibility.py
```

Then run schema behavior only through the self-contained disposable wrappers:

```bash
tests/async/run_lifecycle_archive_schema_mysql.sh
tests/async/run_personal_data_export_schema_mysql.sh
tests/async/run_account_erasure_schema_mysql.sh
tests/async/run_immutable_migration_ledger_mysql.sh
tests/async/run_runtime_compatibility_mysql.sh
RUNTIME_DB_IMAGE=mariadb:10.11 tests/async/run_runtime_compatibility_mysql.sh
```

Each wrapper must create and destroy its own isolated container. Never replace its
target with `.env` values or use a configured, shared, restored, or production
database. The checked-in lifecycle policy keeps canonical archive, export, and erasure
mutation disabled; passing tests prove the guard and synthetic contract, not policy
approval.

## Phase 03 readiness boundary

Session 14 owns the final readiness claim. Its authoritative specification is
`.spec_system/PRD/phase_03/session_14_final_200_player_and_compliance_gate.md`.
Until that session qualifies isolated representative data, runs all eight profiles at
25/50/100/200 clients with each 200-client hold lasting at least 30 minutes, injects
the complete fault matrix, reconciles every durable domain after every run, validates
privacy/restore behavior, and publishes sanitized evidence, the repository is not qualified
as 200-player ready. That limitation remains explicit until the complete Session 14 evidence
exists.

Neither `make test-all`, `make test-db`, the Phase 01/02 logical-client gates, nor a
successful server boot may be used as a substitute for that evidence.

The checked-in contract and exact execution procedure are in
[`PHASE03_READINESS.md`](PHASE03_READINESS.md). Qualification uses a separate ignored
configuration and never reads `.env` implicitly:

```bash
python3 scripts/session14_gate.py \
  --config tmp/session14-gate/config.json \
  --preflight-only
```

`UNQUALIFIED` is a safe refusal, not a failed workload. `QUALIFIED` is also not
readiness evidence; only a complete `PASS` after every minimum-duration case can
support the claim.

## Conventions for new tests

- One concern per file; name it after the feature/regression
  (`test_<feature>.py`). The root runner discovers it automatically. Add a
  `run_<feature>.sh` wrapper only when the test needs special environment setup
  or is useful as a standalone workflow.
- Keep them fast and deterministic; prefer source contracts over full boots
  when the invariant is structural.
- When you change behavior, add or update the focused regression test next to
  it — this is a stated repo convention (see `AGENTS.md`).
- Schema-related changes should extend or add a `_schema_mysql` variant so the
  contract is checked against a real database on a clone.

## What exists today (samples)

| Area | Tests |
|------|-------|
| Persistence | `run_persistence_contract_mysql.sh`, dirty-flush retry, SQL pool shutdown, `test_player_corpse_persistence_contract.py`, `run_corpse_persistence_schema_mysql.sh` |
| Crash regressions | wear-all, relic pickup, stuck command gate |
| Saves | copyover save guards, ship save guards/dedup, epic save guards |
| Phase 01 recovery gate | `test_phase01_recovery_gate.py` drives 25/50/100/200 logical-client waves with ambiguous-commit retries and enforces fork/ownership/route contracts |
| Critical commands | `test_critical_command_coordinator.py` exercises identity, codec, journal corruption/replay, multi-key ordering, duplicate attachment, exact completion, retries, fences, bounds, and lifecycle |
| Critical transactions | `test_critical_transaction_contract.py` plus guarded `run_critical_command_schema_mysql.sh` cover schema, duplicate/mismatch, atomic rollback, concurrent locking, ambiguity lookup, outbox retry/dedupe/dead-letter/restart, and reconciliation |
| Help files | class/race helpfile completeness contracts |
| Event loop | hotspot budget regression |
| Build contract | `test_compiler_warning_profile.py`, `test_message_buffer_bounds.py` |
| Untrusted input | `test_unicode_runtime.py`, `test_ansi_runtime.py`, `test_json_utils_runtime.py`, `test_ttype_runtime.py` |

The four untrusted-input suites are behavioral rather than contract-style: they
exercise the live decoders that handle network and player-visible text — UTF-8
widths and malformed/overlong/surrogate encodings, ANSI colour parsing and
gradients, JSON and GMCP escaping, and RFC 1091 terminal-type negotiation
including MTTS capability parsing. That is the shape to copy when the code under
test is a parser: every case is a real call, and every defect found while
writing them (unbounded continuation runs, invalid UTF-8 re-encoded as `U+FFFD`
and accepted as a map glyph, `MTTS 4JUNK` silently enabling capabilities,
gradient application dereferencing `end()` on an empty string) stayed as a
regression.

`test_compiler_warning_profile.py` is what keeps the `-Werror` guarantee in
[BUILDING.md](BUILDING.md#warning-profile) real: it fails if any of the six
formerly-excepted categories is suppressed again, by flag, by a reintroduced
exception variable, or by pragma.
