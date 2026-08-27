# Implementation Notes

**Session ID**: `phase03-session13-documentation-and-operator-contract`
**Started**: 2026-08-27 12:25 IDT
**Last Updated**: 2026-08-27 13:02 IDT

---

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 11 / 11 |
| Estimated Remaining | 0 hours |
| Blockers | 0 |

---

## Authoritative Claim Matrix

| Contract | Primary implementation evidence | Operator/test evidence |
|----------|---------------------------------|------------------------|
| Boot compatibility | `src/comm.c`, `src/sql.c`, `src/runtime_compatibility_contract.h` | `migrations/runtime_compatibility_manifest.json`, `migrations/verify_runtime_compatibility.sh`, `tests/async/test_runtime_boot_compatibility.py` |
| Player load | `src/player_load_pipeline.c`, `src/player_load_repository.c`, `src/player_load_materialize.c` | `tests/async/test_player_load_pipeline.py`, `tests/async/run_player_load_repository_mysql.sh` |
| Player checkpoints | `src/player_save_pipeline.c`, `src/player_save_worker.c`, `src/player_save_journal.c` | `docs/PLAYER_SAVE_PIPELINE.md`, `docs/PLAYER_SAVE_JOURNAL.md`, focused player-save tests |
| Critical commands | `src/critical_command_coordinator.c`, `src/critical_command_journal.c`, `src/critical_command_repository.c` | `docs/CRITICAL_COMMAND_PIPELINE.md`, Phase 02 domain and crash-matrix tests |
| Item ownership | `src/item_ownership_runtime.c` and typed movement/locker/auction repositories | `migrations/reconcile_item_ownership.sh`, ownership focused tests |
| World recovery | `src/world_recovery_pipeline.c`, bounded Redis publication in `src/redis.c` | `docs/WORLD_RECOVERY_PIPELINE.md`, world-recovery tests |
| Maintenance | `src/maintenance_scheduler.c`, `src/maintenance_repository.c` | `tests/async/test_maintenance_scheduler.py` |
| Lifecycle/privacy | `migrations/data_lifecycle_manifest.json` and guarded archive/export/erasure scripts | `docs/DATA_LIFECYCLE.md`, `docs/LIFECYCLE_ARCHIVE.md`, `docs/PERSONAL_DATA_EXPORT.md`, `docs/ACCOUNT_ERASURE.md` |
| Migration history | `scripts/migration_runner.py`, `migrations/migration_manifest.json` | `docs/IMMUTABLE_MIGRATIONS.md`, `tests/async/test_immutable_migration_runner.py` |

The lifecycle manifest still records controller decisions as pending. Canonical archive,
export, and erasure mutation therefore remains disabled; inspection, schema verification,
dry-run evidence, and restore preflight are the honest documented operator surfaces.

---

## Task Log

### Task T001 - Trace authoritative routes and verification commands

**Started**: 2026-08-27 12:25 IDT
**Completed**: 2026-08-27 12:26 IDT
**Duration**: 1 minute

**Notes**:
- Traced the boot, load, snapshot, journal, critical-command, ownership, world-recovery,
  maintenance, lifecycle, migration, and compatibility entry points into the claim matrix.
- Confirmed MySQL/MariaDB remains durable authority; Redis is bounded cache/recovery,
  not the dirty-player durability authority.

**Files Changed**:
- `.spec_system/specs/phase03-session13-documentation-and-operator-contract/implementation-notes.md` - recorded the source/test claim matrix.

**Verification**:
- Command/check: targeted `rg` inventory across `src/`, `migrations/`, `scripts/`, and `tests/async/`, plus `--help` on five operator scripts
  - Result: PASS - all named implementation and operator entry points exist; help paths exit successfully.
  - Evidence: source inventory includes typed load/save, critical journal, ownership, maintenance, recovery, migration, lifecycle, export, erasure, and compatibility modules.
- UI product-surface check: N/A - documentation/source tracing only.
- UI craft check: N/A - no product UI.

### Task T002 - Inventory current documentation contracts

**Started**: 2026-08-27 12:26 IDT
**Completed**: 2026-08-27 12:28 IDT
**Duration**: 2 minutes

**Notes**:
- Verified all current relative Markdown links in the maintained guide set resolve.
- Found stale database selection by listen-port claims in architecture, configuration,
  and runbook docs; source requires explicit `DB_NAME` plus `DB_ALLOWED_TARGETS`.
- Found stale generic three-worker/full-save claims and both diagrams' legacy raw-event
  and Redis dirty-save topology.
- Found the production checklist still directs credentials into `src/sql.h`, contrary
  to the implemented required environment configuration.

**Files Changed**:
- `.spec_system/specs/phase03-session13-documentation-and-operator-contract/implementation-notes.md` - recorded the pre-edit defect inventory.

**Verification**:
- Command/check: local Markdown link resolver over the seven maintained guides
  - Result: PASS - 0 missing relative links before edits.
  - Evidence: every non-HTTP Markdown target resolved inside the repository.
- Command/check: targeted stale-claim and `.env.example`/`src/sql.c` configuration scan
  - Result: PASS - exact stale claims and implemented replacement contracts identified.
  - Evidence: `src/sql.c` requires role, credentials, allow-list, and transport checks independent of listener port.
- UI product-surface check: N/A - documentation inventory only.
- UI craft check: N/A - no product UI.

### Task T003 - Correct README and documentation navigation

**Started**: 2026-08-27 12:28 IDT
**Completed**: 2026-08-27 12:30 IDT
**Duration**: 2 minutes

**Notes**:
- Replaced the single generic worker route with revisioned snapshot, critical-command,
  and bounded compatibility routes in the root architecture overview.
- Named MySQL/MariaDB as durable authority and limited Redis to optional cache/recovery.
- Expanded top-level navigation to the migration, compatibility, lifecycle, and
  critical-command contracts without making a readiness claim.

**Files Changed**:
- `README.md` - corrected architecture and integrated guide navigation.
- `docs/README.md` - corrected guide and diagram descriptions.

**Verification**:
- Command/check: local Markdown link resolution for `README.md` and `docs/README.md`
  - Result: PASS - every relative link resolves.
  - Evidence: 0 missing targets; required typed-route and durable-authority terms found.
- Command/check: added-line ASCII scan and `git diff --check`
  - Result: PASS - no non-ASCII addition or whitespace error.
- UI product-surface check: N/A - repository documentation only.
- UI craft check: N/A - no product UI.

### Task T004 - Rewrite the source-traced architecture guide

**Started**: 2026-08-27 12:30 IDT
**Completed**: 2026-08-27 12:34 IDT
**Duration**: 4 minutes

**Notes**:
- Replaced the generic worker model with distinct load, snapshot, critical-command,
  maintenance, world-recovery, and compatibility boundaries.
- Corrected database selection to explicit environment/allow-list control plus the
  port safety guard, and documented exact pre-service compatibility ordering.
- Added fail-closed login, terminal-save, operation fence, and pending archive behavior.

**Files Changed**:
- `docs/ARCHITECTURE.md` - source-traced process, boot, persistence, and recovery model.

**Verification**:
- Command/check: Markdown link resolver, stale-claim exclusions, and required source-token scan
  - Result: PASS - links resolve; old three-worker/port-selection/generic-test-only claims are absent.
  - Evidence: guide names load pipeline, compatibility gate, critical commands, maintenance, and partial-login prohibition.
- Command/check: added-line ASCII scan and `git diff --check`
  - Result: PASS - no non-ASCII addition or whitespace error.
- UI product-surface check: N/A - architecture documentation only.
- UI craft check: N/A - no product UI.

### Task T005 - Correct the database authority and operator contract

**Started**: 2026-08-27 12:34 IDT
**Completed**: 2026-08-27 12:40 IDT
**Duration**: 6 minutes

**Notes**:
- Replaced the broad three-worker save claim with a boundary table for load,
  checkpoint, critical command, ownership, maintenance, recovery, and compatibility.
- Added the consistent-read/linear-materialization contract, current owner and ledger
  transaction, lifecycle pending-policy boundary, and exact 170-to-171 migration chain.
- Added explicit warnings that the legacy migration runner has no help or dry-run mode
  and must only run on a stopped backed-up development clone.

**Files Changed**:
- `docs/DATABASE.md` - corrected connection, data authority, migration, ownership,
  lifecycle, and verification guidance.

**Verification**:
- Command/check: Markdown link resolver, required contract tokens, and stale-claim scan
  - Result: PASS - all links resolve and old generic worker claims are absent.
  - Evidence: guide names consistent read, current owner, 171-table identity, pending policy, and immutable runner commands.
- Command/check: added-line ASCII scan and `git diff --check`
  - Result: PASS - no non-ASCII addition or whitespace error.
- UI product-surface check: N/A - database documentation only.
- UI craft check: N/A - no product UI.

### Task T006 - Reconcile configuration precedence and trust boundaries

**Started**: 2026-08-27 12:40 IDT
**Completed**: 2026-08-27 12:43 IDT
**Duration**: 3 minutes

**Notes**:
- Documented explicit DB selection/allow-list precedence and the narrower port guard.
- Added both mandatory journal directories, maintenance state, exact DB session modes
  and 10-second deadlines, Redis 250/100 ms deadlines, and pre-mutation ordering.
- Corrected `.env.example` so Redis is described as optional cache/recovery and exposes
  world-generation and maintenance state controls.

**Files Changed**:
- `docs/CONFIGURATION.md` - corrected configuration, defaults, and trust boundaries.
- `.env.example` - corrected Redis role and added supported recovery/maintenance keys.

**Verification**:
- Command/check: required-variable, timeout, mode, and prohibited-stale-claim scan
  - Result: PASS - every required token exists and old port-selector/dirty-save wording is absent.
  - Evidence: exact journal, TLS, metadata, Redis recovery, and maintenance variables are documented.
- Command/check: added-line ASCII scan and `git diff --check`
  - Result: PASS - no non-ASCII addition or whitespace error.
- UI product-surface check: N/A - configuration documentation only.
- UI craft check: N/A - no product UI.

### Task T007 - Build guarded operator procedures

**Started**: 2026-08-27 12:43 IDT
**Completed**: 2026-08-27 12:44 IDT
**Duration**: 1 minute

**Notes**:
- Removed source-credential and listener-port target-selection claims and documented
  the actual environment, allow-list, TLS, and pre-service fail-closed gate.
- Added qualified-clone migration, reconciliation, lifecycle/privacy inspection,
  restore/tombstone, failure containment, and backup-restore procedures.

**Files Changed**:
- `docs/RUNBOOK.md` - added guarded operator procedures and corrected production setup.

**Verification**:
- Command/check: `python3 tests/async/test_runtime_connection_trust.py` and `python3 tests/async/test_runtime_boot_compatibility.py`
  - Result: PASS - connection trust and all seven boot compatibility tests passed.
  - Evidence: source contracts prove explicit target selection, TLS/session invariants, deadlines, and pre-listener compatibility ordering.
- Command/check: local link/path/token contract, added-line ASCII scan, and `git diff --check`
  - Result: PASS - 0 missing links, all named routes exist, no stale source-credential claim, and no whitespace or ASCII addition errors.
- UI product-surface check: N/A - operator documentation only.
- UI craft check: N/A - no product UI.

### Task T008 - Map validation evidence boundaries

**Started**: 2026-08-27 12:44 IDT
**Completed**: 2026-08-27 12:47 IDT
**Duration**: 3 minutes

**Notes**:
- Added a matrix separating focused, build, repository, disposable schema,
  dual-engine, lifecycle/privacy, precursor capacity/fault, and final gate evidence.
- Made the uncompleted Session 14 workload and readiness boundary explicit; no earlier
  logical-client, build, schema, or boot result substitutes for it.

**Files Changed**:
- `docs/TESTING.md` - added exact commands, database isolation rules, and evidence limits.

**Verification**:
- Command/check: `python3 tests/async/test_phase01_recovery_gate.py`, `python3 tests/async/test_phase02_capacity_and_crash_gate.py`, and `python3 tests/async/test_data_lifecycle_manifest.py`
  - Result: PASS - Phase 01 load gate, three Phase 02 checks, and nine lifecycle manifest checks passed.
  - Evidence: precursor commands are valid and their narrower evidence boundary is documented.
- Command/check: local link/path/token contract, added-line ASCII scan, and `git diff --check`
  - Result: PASS - all links and existing named command paths resolve; the future documentation test is explicitly created by T011; no whitespace or ASCII addition errors.
- UI product-surface check: N/A - testing documentation only.
- UI craft check: N/A - no product UI.

### Task T009 - Replace the server architecture diagram

**Started**: 2026-08-27 12:47 IDT
**Completed**: 2026-08-27 12:52 IDT
**Duration**: 5 minutes

**Notes**:
- Replaced the raw persistence/Redis dirty-save model with the fail-closed boot gate,
  consistent load, revisioned snapshot, critical-command, bounded support, durable
  MySQL/MariaDB, and optional Redis cache/recovery boundaries.
- Used the approved Iron-Gall & Ember skin, direct architecture type, doc-wide size,
  balanced engineer detail, nine nodes, and twelve non-overlapping routes.

**Files Changed**:
- `docs/diagrams/duris-server-architecture.html` - rebuilt accessible static schematic.

**Verification**:
- Command/check: diagram-design `self_check.py` and diagram token/ASCII/LF contract
  - Result: PASS - accessible SVG and single-file checks passed; three prefixed markers, required topology, no stale claims, ASCII, and LF verified.
  - Evidence: repository has no `scripts/verify-geometry.py`; connector geometry was inspected from a Chromium-rendered 1400x900 screenshot.
- UI product-surface check: N/A - standalone engineering documentation figure.
- UI craft check: PASS - rendered figure preserves hierarchy, readable labels, route separation, 4 px grid, two focal nodes, and a bottom legend.

### Task T010 - Replace the database authority model

**Started**: 2026-08-27 12:52 IDT
**Completed**: 2026-08-27 12:58 IDT
**Duration**: 6 minutes

**Notes**:
- Replaced legacy event/content tables with eight grouped entities spanning account
  identity, revisioned player state, operations, current authority, ledgers,
  lifecycle/privacy metadata, and immutable schema/dataset identity.
- Kept pending controller decisions and the absent canonical mutation adapter visible;
  the dashed lifecycle route is explicitly blocked rather than presented as active.

**Files Changed**:
- `docs/diagrams/duris-database-model.html` - rebuilt accessible static ER/data model.

**Verification**:
- Command/check: diagram-design `self_check.py` and diagram token/ASCII/LF contract
  - Result: PASS - accessible SVG and single-file checks passed; eight groups, three prefixed markers, no legacy queue claims, ASCII, and LF verified.
  - Evidence: Chromium-rendered 1400x900 review confirmed readable fields, cardinalities, route separation, policy state, zones, and bottom legend.
- UI product-surface check: N/A - standalone engineering documentation figure.
- UI craft check: PASS - doc-wide balanced engineer layout uses three authority zones, two focal current-state groups, and no connector collision or clipped label.

### Task T011 - Add the documentation contract and run full verification

**Started**: 2026-08-27 12:58 IDT
**Completed**: 2026-08-27 13:02 IDT
**Duration**: 4 minutes

**Notes**:
- Added a nine-check source contract for maintained Markdown links/anchors,
  operator entry points, environment names, stale claims, mutation safeguards,
  pending lifecycle policy, diagram accessibility, and ASCII/LF invariants.
- Left `Makefile` unchanged because `tests/run_regression_tests.py` already discovers
  every `tests/async/test_*.py`; `make test-all` proved automatic inclusion.

**Files Changed**:
- `tests/async/test_documentation_contract.py` - added the focused drift gate.
- `docs/RUNBOOK.md` - tightened exact safety wording found while exercising the gate.

**Verification**:
- Command/check: 17 adjacent focused tests plus both diagram self-checks
  - Result: PASS - connection, boot, load, save, critical, recovery, maintenance, lifecycle, export, erasure, migration, and diagram contracts passed.
  - Evidence: all focused commands exited 0 without a configured database.
- Command/check: `make test-all`
  - Result: PASS - 210 Python tests and the native signal-handler suite passed; documentation test was discovered as test 54.
- Command/check: `./scripts/format.sh --check`, added-line ASCII/LF scan, and `git diff --check`
  - Result: PASS - formatting, ASCII additions, LF files, and whitespace checks are clean.
- UI product-surface check: N/A - documentation and source-contract test only.
- UI craft check: N/A - diagram craft was verified under T009 and T010.

---

## Safety Incident

### Incident 1: Legacy migration runner treated `--help` as execution

**Description**: During command-surface inspection, `./migrations/run_migration.sh
--help` ignored the argument and began against the configured development database.
It completed steps 1 and 2, then step 3 stopped at missing table `players_core`.

**Impact**: The configured development database (not production) had its database
default set to `utf8mb4_unicode_ci`; five invalid-date normalization updates ran; and
the InnoDB conversion statements preceding `players_core` may have rebuilt 22 named
tables. No later migration step ran. The prior schema/data values are unknown, so no
unsafe guessed rollback was attempted.

**Containment**: No further configured-database command is permitted in Session 13.
Documentation now states that the runner has no help/dry-run mode. Remaining command
verification is source-only or uses existing disposable database wrappers.

---

## Design Decisions

### Decision 1: Document layered typed persistence rather than one generic worker pipe

**Context**: Current top-level prose and diagrams collapse distinct persistence domains.
**Options Considered**:
1. Preserve one generic async-persistence label - concise but materially inaccurate.
2. Group typed snapshot, critical command, legacy event, maintenance, and recovery paths - accurate without enumerating every domain module.

**Chosen**: Group the typed paths and name their separate durability/recovery contracts.
**Rationale**: This preserves a readable overview while preventing operators from using
the wrong queue, journal, or recovery procedure for a domain.

## Session Completion

All 11 implementation tasks and success criteria are complete. No configured database
was accessed after the recorded legacy-runner incident. Session 14 remains the sole
owner of any 200-player readiness evidence or claim.

## Next Task

Run `creview` from base commit `8cb0684abf4f327192ccf213fd7b9bfe12bbc090`.
