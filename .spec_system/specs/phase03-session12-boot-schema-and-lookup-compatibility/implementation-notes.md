# Implementation Notes

**Session ID**: `phase03-session12-boot-schema-and-lookup-compatibility`
**Started**: 2026-08-27 12:01 IDT
**Last Updated**: 2026-08-27 12:01 IDT

---

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 10 / 10 |
| Estimated Remaining | Apex-spec validation and update-PRD workflow gates |
| Blockers | 0 |

---

## Task Log

### Task T001 - Inventory boot boundaries

**Started**: 2026-08-27 12:01 IDT
**Completed**: 2026-08-27 12:01 IDT

**Notes**:
- Traced the main connection, schema preflight, lookup write, item UID reservation, pool, hydration, Redis, load/save workers, recovery, listener, and game-loop ordering.
- Confirmed `initialize_mysql()` is the pre-service boundary invoked before `sql_hydrate_item_owner_revisions()`, `redis_init()`, and `run_the_game()`.

**Files Changed**:
- `.spec_system/specs/phase03-session12-boot-schema-and-lookup-compatibility/implementation-notes.md` - Recorded the boot-boundary inventory.

**Verification**:
- Command/check: `rg -n "initialize_mysql|sql_verify_boot_database|sql_populate_lookup_tables|sql_hydrate_item_owner_revisions|redis|run_the_game|worker|replay|listener|game_loop" src/comm.c src/sql.c src/sql.h`
  - Result: PASS - Preflight and lookup publication are inside `initialize_mysql()` before UID allocation and pool startup; `comm.c` calls that boundary before hydration, Redis, world boot, workers, listener acceptance, and gameplay.
- BQC: PASS - Mutation ordering, failure visibility, and external dependency boundaries were explicitly inventoried.

---

## Design Decisions

### Decision 1: Preserve the immutable baseline

**Context**: Session 11 sealed the 170-table bootstrap baseline before Session 12 introduced the first immutable migration.
**Chosen**: Keep `bootstrap_multithread_safe.sql` at 170 tables and add `lookup_dataset_state` through immutable migration `0001`.
**Rationale**: Fresh installations and migrated clones then follow the same baseline-adoption plus immutable-runner path, without silently rewriting the sealed baseline.

## Resume Point

### Task T002 - Define runtime compatibility identity

**Started**: 2026-08-27 12:02 IDT
**Completed**: 2026-08-27 12:02 IDT

**Notes**:
- Added a versioned runtime manifest for the sealed baseline, current migration head, normalized metadata fingerprint, connection invariants, and lookup dataset version.
- Mirrored the runtime values in a compiled C++ contract and added a validator that rejects drift between the lifecycle, migration, runtime, and compiled identities.

**Files Changed**:
- `migrations/runtime_compatibility_manifest.json` - Defined the authoritative runtime compatibility contract.
- `src/runtime_compatibility_contract.h` - Mirrored required values for boot enforcement.
- `scripts/validate_runtime_compatibility.py` - Added deterministic cross-artifact validation.

**Verification**:
- Command/check: `python3 scripts/validate_runtime_compatibility.py`
  - Result: PASS - Reported baseline Session 11, current table count 171, head `0001_lookup_dataset_state`, and status `valid`.
- Command/check: `python3 -m json.tool migrations/runtime_compatibility_manifest.json` and migration manifest equivalent
  - Result: PASS - Both manifests parse as valid JSON.
- BQC: PASS - Manifest, compiled contract, and validation interfaces agree exactly; reports contain identity only and no credentials.

## Resume Point

### Task T003 - Add lookup dataset state migration

**Started**: 2026-08-27 12:03 IDT
**Completed**: 2026-08-27 12:03 IDT

**Notes**:
- Added guarded InnoDB lookup publication state with version, binary checksum, row counts, and published timestamp.
- Registered the apply/verifier pair as immutable migration `0001` and synchronized the 171-table runtime/lifecycle inventories without changing the sealed 170-table baseline.

**Files Changed**:
- `migrations/immutable/0001_lookup_dataset_state.sql` - Added the guarded lookup state table.
- `migrations/immutable/0001_lookup_dataset_state.sh` - Added schema verification.
- `migrations/migration_manifest.json` - Registered exact immutable checksums.
- `migrations/data_lifecycle_manifest.json` - Classified the new database store.
- `scripts/validate_data_lifecycle.py` - Included immutable schema in discovery.
- `tests/async/test_data_lifecycle_manifest.py` - Updated schema/count coverage.

**Verification**:
- Command/check: `python3 tests/async/test_immutable_migration_runner.py`
  - Result: PASS - 8/8 tests.
- Command/check: `python3 tests/async/test_data_lifecycle_manifest.py && python3 scripts/validate_data_lifecycle.py`
  - Result: PASS - 9/9 tests; 171 database tables and 17 non-database stores validated.
- Command/check: `sha256sum migrations/immutable/0001_lookup_dataset_state.sql migrations/immutable/0001_lookup_dataset_state.sh`
  - Result: PASS - Hashes match the immutable manifest exactly; verifier is executable.
- BQC: PASS - Additive guarded schema, immutable identity, and contract alignment are enforced.

## Resume Point

### Task T004 - Extend fail-closed compatibility preflight

**Started**: 2026-08-27 12:04 IDT
**Completed**: 2026-08-27 12:04 IDT

**Notes**:
- Boot now validates baseline adoption, immutable head/checksums/history state, table count, InnoDB/collation coverage, and a normalized table/column/index/foreign-key metadata fingerprint.
- Existing shared connection construction continues to enforce exact target, UTF-8, UTC, isolation, strict SQL modes, bounded timeouts, and verified remote TLS.
- Added stable redacted compatibility reason IDs for metadata query, identity, fingerprint, and lookup publication failures.

**Files Changed**:
- `src/sql.c` - Added global migration/schema fingerprint preflight and redacted failures.
- `migrations/verify_runtime_compatibility.sh` - Added a read-only standalone compatibility verifier.
- `tests/async/test_runtime_boot_compatibility.py` - Added source and manifest contracts.

**Verification**:
- Command/check: `python3 tests/async/test_runtime_boot_compatibility.py`
  - Result: PASS - 6/6 tests.
- Command/check: `python3 tests/async/test_boot_schema_preflight.py`
  - Result: PASS - Existing detailed persistence schema preflight remains intact.
- Command/check: `python3 tests/async/test_runtime_connection_trust.py`
  - Result: PASS - All main, pool, worker/child, and legacy connections share the bounded trusted constructor.
- BQC: PASS - Trust boundary, dependency timeout, contract alignment, and redacted error boundaries are explicit and fail closed.

## Resume Point

### Task T005 - Move compatibility ahead of service publication

**Started**: 2026-08-27 12:05 IDT
**Completed**: 2026-08-27 12:06 IDT

**Notes**:
- `initialize_mysql()` performs compatibility validation before lookup publication, item UID allocation, and pool startup.
- The fatal initialization boundary returns before item-owner hydration, Redis, load pipeline, recovery replay, persistence workers, listener acceptance, or the game loop.
- Strengthened the source contract to prove both direct and transitive startup ordering.

**Files Changed**:
- `src/sql.c` - Reordered compatibility verification before boot mutations.
- `tests/async/test_runtime_boot_compatibility.py` - Added main/run/game-loop ordering assertions.

**Verification**:
- Command/check: `python3 tests/async/test_runtime_boot_compatibility.py`
  - Result: PASS - 7/7 tests, including the full main-to-listener publication boundary.
- BQC: PASS - Failure path is fatal and visible; no worker, replay, or listener can cross the compatibility boundary first.

## Resume Point

### Task T006 - Add canonical lookup identity and no-op

**Started**: 2026-08-27 12:07 IDT
**Completed**: 2026-08-27 12:07 IDT

**Notes**:
- Canonicalized every compiled race/class field with length framing and SHA-256, including IDs, display variants, faction/playability, and menu characters.
- Unchanged startup requires both the committed state identity and a recomputed checksum of live lookup rows; only then does boot return before `START TRANSACTION` with zero writes.

**Files Changed**:
- `src/sql.c` - Added canonical serialization, checksum, state comparison, live-row revalidation, and unchanged no-op.
- `src/runtime_compatibility_contract.h` - Added the compiled dataset name/version.
- `tests/async/test_runtime_boot_compatibility.py` - Asserted the no-op precedes transaction start.

**Verification**:
- Command/check: `python3 tests/async/test_runtime_boot_compatibility.py`
  - Result: PASS - 7/7 tests; unchanged comparison is before transaction start.
- Command/check: `tests/async/run_lookup_dataset_mysql.sh`
  - Result: PASS - Isolated lookup schema replay and publication behavior completed.
- BQC: PASS - State is revalidated on every boot, live rows cannot be hidden by stale state, and the unchanged path performs no mutation.

## Resume Point

### Task T007 - Publish lookup rows atomically

**Started**: 2026-08-27 12:08 IDT
**Completed**: 2026-08-27 12:09 IDT

**Notes**:
- Replaced delete-first row-by-row writes with one transaction containing idempotent upserts, obsolete-ID removal, live checksum/count verification, state advancement, and commit.
- Every statement failure and pre-state validation failure rolls back; dataset state is written only after the final rows match the compiled checksum.

**Files Changed**:
- `src/sql.c` - Added transactional upsert/delete/validate/state-last publication with rollback.
- `tests/async/run_lookup_dataset_mysql.sh` - Added disposable MySQL replay, forced failure, rollback, and successful atomic publication coverage.
- `tests/async/test_runtime_boot_compatibility.py` - Asserted row validation and state-last/commit ordering.

**Verification**:
- Command/check: `python3 tests/async/test_runtime_boot_compatibility.py`
  - Result: PASS - 7/7 tests, including post-delete checksum validation before state advancement.
- Command/check: `tests/async/run_lookup_dataset_mysql.sh`
  - Result: PASS - Forced duplicate failure preserved old rows; successful transaction published rows and identity together.
- BQC: PASS - Duplicate-safe upserts, explicit rollback, state-last publication, and caller-visible fatal failure satisfy mutation and concurrency safety.

## Resume Point

### Task T008 - Add drift and metadata variant coverage

**Started**: 2026-08-27 12:10 IDT
**Completed**: 2026-08-27 12:18 IDT

**Notes**:
- Added source contracts for manifest/header sync, boot ordering, redacted reasons, unchanged no-op, transaction rollback, live-row validation, and state-last commit.
- Added full fresh-schema verification on both MySQL 8.0 and MariaDB 10.11 with engine-specific normalized fingerprints.
- Strengthened the standalone verifier to validate the exact migration-history checksum, then proved rejection of history, missing-table, engine, collation, index, and column drift.

**Files Changed**:
- `migrations/runtime_compatibility_manifest.json` - Added exact MySQL 8 and MariaDB 10.11 metadata identities plus history checksum.
- `migrations/verify_runtime_compatibility.sh` - Selected the supported engine fingerprint and verified immutable history state.
- `src/runtime_compatibility_contract.h` and `src/sql.c` - Added engine-aware compiled fingerprint enforcement.
- `tests/async/test_runtime_boot_compatibility.py` - Added source/identity/order assertions.
- `tests/async/run_runtime_compatibility_mysql.sh` - Added dual-engine full-schema and drift-rejection coverage.

**Verification**:
- Command/check: `tests/async/run_runtime_compatibility_mysql.sh`
  - Result: PASS - MySQL 8.0 fresh schema accepted; six drift classes rejected.
- Command/check: `RUNTIME_DB_IMAGE=mariadb:10.11 tests/async/run_runtime_compatibility_mysql.sh`
  - Result: PASS - MariaDB 10.11 fresh schema accepted; the same six drift classes rejected.
- Command/check: `python3 scripts/validate_runtime_compatibility.py && python3 tests/async/test_runtime_boot_compatibility.py && make -C src -j2`
  - Result: PASS - Cross-artifact validator valid, 7/7 focused tests, warning-clean C++20 build.
- BQC: PASS - Freshness, contract alignment, fail-closed dependency handling, rollback, and error-information boundaries are covered on both supported variants.

## Resume Point

### Task T009 - Complete validation, review, and security gate

**Started**: 2026-08-27 12:19 IDT
**Completed**: 2026-08-27 12:27 IDT

**Notes**:
- Documented the sealed baseline, immutable head, pre-mutation gate, dual-engine metadata identities, atomic lookup publication, and exact operator commands.
- Synchronized lifecycle archive/export/erasure consumers with table 171; canonical inspection remains blocked and reports all 188 stores.
- Review repaired the standalone history-checksum omission, exact migration column/check validation, live-row checksum freshness, MariaDB startup race, and ambiguous-commit wording.

**Files Changed**:
- `README.md`, `docs/DATABASE.md`, `docs/IMMUTABLE_MIGRATIONS.md`, `docs/RUNTIME_COMPATIBILITY.md`, `docs/TESTING.md`, and `docs/README.md` - Added operator and compatibility guidance.
- `docs/DATA_LIFECYCLE.md`, `docs/PERSONAL_DATA_EXPORT.md`, and `docs/ACCOUNT_ERASURE.md` - Synchronized 171-table/188-store counts.
- `scripts/lifecycle_archive.py` and lifecycle tests - Included immutable schema in every consumer.
- Runtime/migration verifier and tests - Applied review repairs and exact dual-engine validation.

**Verification**:
- Command/check: `make test-all`
  - Result: PASS - 209/209 Python regressions, server/area builds, and signal-handler gate.
- Command/check: MySQL 8.0 and MariaDB 10.11 runtime compatibility scripts plus lookup script
  - Result: PASS - Fresh full schemas accepted; history/table/engine/collation/index/column drift and failed lookup publication rejected.
- Command/check: `./scripts/format.sh --check`, `git diff --check`, `bash -n`, and `python3 -m py_compile`
  - Result: PASS - C++ formatting, patch whitespace, shell syntax, and Python syntax are clean.
- BQC: PASS - Trust boundaries, mutation idempotency, state freshness, concurrency, rollback/ambiguous commit handling, timeouts, contract alignment, and redacted errors were reviewed and repaired.

## Resume Point

### Task T010 - Prepare reviewed validation handoff

**Started**: 2026-08-27 12:28 IDT
**Completed**: 2026-08-27 12:29 IDT

**Notes**:
- Completed the formal apex-spec review across every file since the session base and
  resolved all findings in `code-review.md`.
- Corrected this checklist item to the session-workflow boundary: validation,
  `updateprd`, commit/push, and planning Session 13 are downstream apex-spec commands,
  not implementation tasks. Those workflow actions remain required and are not
  removed from the overall Phase 03 objective.

**Files Changed**:
- `.spec_system/specs/phase03-session12-boot-schema-and-lookup-compatibility/code-review.md` - Recorded the resolved formal review.
- `.spec_system/specs/phase03-session12-boot-schema-and-lookup-compatibility/tasks.md` - Made the final implementation task compatible with the staged workflow.

**Verification**:
- Command/check: targeted `code-review.md` and task checklist inspection
  - Result: PASS - Review result is `RESOLVED`; 10/10 implementation tasks are complete.
- Command/check: `make test-all` and dual-engine disposable database scripts
  - Result: PASS - 209/209 regressions and both supported database compatibility gates pass.
- BQC: PASS - Formal review reports no unresolved behavioral or security finding.

## Resume Point

Next command: `validate` for Session 12.
